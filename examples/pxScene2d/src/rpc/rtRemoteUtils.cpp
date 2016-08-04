#include "rtRemoteUtils.h"
#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"

#include <sstream>
#include <string>
#include <memory>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


//TODO Maybe remove CastType and NetType from rtRemoteIAddress
//     and instead have free functions here that parse those
//     given an address object

rtError
rtRemoteEndpointAddressToSocket(rtRemoteEndpointPtr addr, sockaddr_storage& ss)
{
  if (auto local = dynamic_pointer_cast<rtRemoteEndpointLocal>(addr))
  {
    if (!local->isSocket())
    {
      rtLogError("local address is not unix domain socket");
      return RT_FAIL;
    }
    return rtParseAddress(ss, local->path().c_str(), 0, nullptr);
  }
  else if (auto net = dynamic_pointer_cast<rtRemoteEndpointRemote>(addr))
  {
    return rtParseAddress(ss, net->host().c_str(), net->port(), nullptr);
  }
  else
  {
    return RT_FAIL;
  }
}

//TODO Better error handling here
rtError
rtRemoteSocketToEndpointAddress(sockaddr_storage const& ss, rtConnType const& conn_type, rtRemoteEndpointPtr& endpoint)
{
  std::stringstream buff;
  
  std::string scheme;
  if (conn_type == rtConnType::STREAM)
  {
    scheme = "tcp";
  }
  else if (conn_type == rtConnType::DGRAM)
  {
    scheme = "udp";
  }
  else
  {
    rtLogError("no connection protocol indicated while converting from socket to endpoint address");
    return RT_FAIL;
  }

  buff << scheme;
  buff << "://";

  void* addr = NULL;
  rtGetInetAddr(ss, &addr);

  char addr_buff[128];
  memset(addr_buff, 0, sizeof(addr_buff));

  if (ss.ss_family == AF_UNIX)
  {
    strncpy(addr_buff, (const char*)addr, sizeof(addr_buff) -1);
    buff << addr_buff;
    endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, addr_buff);
    return RT_OK;
  }
  else
  {
    inet_ntop(ss.ss_family, addr, addr_buff, sizeof(addr_buff));
    uint16_t port;
    rtGetPort(ss, &port);
    buff << addr_buff;
    buff << ":";
    buff << port;
    endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme, addr_buff, port);
    return RT_OK;
  }
  return RT_OK;
}

//TODO Better error handling here
rtError
rtRemoteParseUri(std::string const& uri, std::string& scheme, std::string& path, std::string& host, uint16_t* port)
{
  size_t index = uri.find("://");
  if (index == std::string::npos)
  {
   rtLogError("Invalid uri: %s. Expected: <scheme>://<host>[:<port>][<path>]", uri.c_str());
   return RT_FAIL;
  }

  // extract scheme
  scheme = uri.substr(0, index);

  // We either have a path or host now.  Let's pull the remaining info.
  index += 3;
  char ch = uri.at(index);
  if (ch == '/' || ch == '.')
  { // local socket
    path = uri.substr(index, std::string::npos);
  }
  else
  { // network socket
    // get port
    std::string port_string;
    size_t index_port = uri.find_last_of(":");
    if (index_port == std::string::npos // no port. no colon found
      || uri.at(index_port-1) == ':' // no port. colon was part of ipv6 addr
      || index_port == index-3) // no port.  last colon equals colon in ://
    {
      rtLogWarn("No port included included in URI: %s. Defaulting to 0", uri.c_str());
      port_string = "0";
      index_port = std::string::npos; // set this for host extraction below
    }
    else
    {
      port_string = uri.substr(index_port+1, std::string::npos);
    }
    *port = stoi(port_string);
    
    // get host
    host = uri.substr(index, index_port - index);
  }
  return RT_OK;
}

bool
rtRemoteSameEndpoint(sockaddr_storage const& first, sockaddr_storage const& second)
{
  if (first.ss_family != second.ss_family)
    return false;

  if (first.ss_family == AF_INET)
  {
    sockaddr_in const* in1 = reinterpret_cast<sockaddr_in const*>(&first);
    sockaddr_in const* in2 = reinterpret_cast<sockaddr_in const*>(&second);

    if (in1->sin_port != in2->sin_port)
      return false;

    return in1->sin_addr.s_addr == in2->sin_addr.s_addr;
  }

  if (first.ss_family == AF_UNIX)
  {
    sockaddr_un const* un1 = reinterpret_cast<sockaddr_un const*>(&first);
    sockaddr_un const* un2 = reinterpret_cast<sockaddr_un const*>(&second);

    return 0 == strncmp(un1->sun_path, un2->sun_path, UNIX_PATH_MAX);
  }

  RT_ASSERT(false);
  return false;
}

bool
rtRemoteSameEndpoint(rtRemoteEndpointPtr const& first, rtRemoteEndpointPtr const& second)
{
  if (auto firstLocal = dynamic_pointer_cast<rtRemoteEndpointLocal>(first))
  {
    if (auto secondLocal = dynamic_pointer_cast<rtRemoteEndpointLocal>(second))
      return *firstLocal == *secondLocal;
    else
      return false;
  }
  else if (auto firstRemote = dynamic_pointer_cast<rtRemoteEndpointRemote>(first))
  {
    if (auto secondRemote = dynamic_pointer_cast<rtRemoteEndpointRemote>(second))
      return *firstRemote == *secondRemote;
    else
      return false;
  }
  else
  {
    RT_ASSERT(false);
    return false;
  }
}

rtNetType
rtRemoteParseNetType(std::string const& host)
{
  int ret;
  struct addrinfo hint, *res = NULL;
  memset(&hint, 0, sizeof hint);
  
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  ret = getaddrinfo(host.c_str(), NULL, &hint, &res);
  if (ret)
  {
    freeaddrinfo(res);
    rtLogWarn("unable to determine NetType (network layer protocol) for host: %s", host.c_str());
    return rtNetType::NONE;
  }
  else
  {
    if (res->ai_family == AF_INET)
      return rtNetType::IPV4;
    else if (res->ai_family == AF_INET6)
      return rtNetType::IPV6;
    else
    {
      freeaddrinfo(res);
      rtLogWarn("unable to determine NetType (network layer protocol) for host: %s", host.c_str());
      return rtNetType::NONE;
    }
  }
}

rtCastType
rtRemoteParseCastType(std::string const& host)
{
  std::string prefix;
  rtNetType net_type = rtRemoteParseNetType(host);
  if (net_type == rtNetType::IPV4)
  {
    prefix = host.substr(0, host.find('.'));
    if (stoi(prefix) >= 224 && stoi(prefix) <= 239)
      return rtCastType::MULTICAST;
    else
      return rtCastType::UNICAST;
  }
  else if (net_type == rtNetType::IPV6)
  {
    prefix = host.substr(0, 2);
    if (prefix.compare("FF") == 0)
      return rtCastType::MULTICAST;
    else
      return rtCastType::UNICAST;
  }
  else
  {
    return rtCastType::NONE;
  }       
}

rtError
rtRemoteEndpointToDocument(rtRemoteEndpointPtr& endpoint, rtJsonDocPtr& doc)
{
  if (auto remote_endpoint = dynamic_pointer_cast<rtRemoteEndpointRemote>(endpoint))
  {
    doc->AddMember(kFieldNameEndpointType, kEndpointTypeRemote, doc->GetAllocator());
    doc->AddMember(kFieldNameScheme, remote_endpoint->scheme(), doc->GetAllocator());
    doc->AddMember(kFieldNameIp, remote_endpoint->host(), doc->GetAllocator());
    doc->AddMember(kFieldNamePort, remote_endpoint->port(), doc->GetAllocator());
    return RT_OK;
  }
  else if (auto local_endpoint = dynamic_pointer_cast<rtRemoteEndpointLocal>(endpoint))
  {
    doc->AddMember(kFieldNameEndpointType, kEndpointTypeLocal, doc->GetAllocator());
    doc->AddMember(kFieldNameScheme, local_endpoint->scheme(), doc->GetAllocator());
    doc->AddMember(kFieldNamePath, local_endpoint->path(), doc->GetAllocator());
    return RT_OK;
  }
  else
  {
    return RT_FAIL;
  }
  return RT_OK;
}

rtError
rtRemoteDocumentToEndpoint(rtJsonDocPtr const& doc, rtRemoteEndpointPtr& endpoint)
{
  RT_ASSERT(doc->HasMember(kFieldNameScheme));
  RT_ASSERT(doc->HasMember(kFieldNameEndpointType));
  std::string type, scheme;
  type   = (*doc)[kFieldNameEndpointType].GetString();
  scheme = (*doc)[kFieldNameScheme].GetString();
  
  if (type.compare(kEndpointTypeLocal) == 0)
  {
    RT_ASSERT(doc->HasMember(kFieldNamePath));
    std::string path;
    path = (*doc)[kFieldNamePath].GetString();
    // create and return local endpoint address
    endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, path);
    return RT_OK;
  }
  else if (type.compare(kEndpointTypeRemote) == 0)
  {
    RT_ASSERT(doc->HasMember(kFieldNameIp));
    RT_ASSERT(doc->HasMember(kFieldNamePort));
    std::string host;
    uint16_t port;
    host = (*doc)[kFieldNameIp].GetString();
    port = (*doc)[kFieldNamePort].GetInt();
    // create and return net endpoint address
    endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme, host, port);
    return RT_OK;
  }
  else
  {
    rtLogError("unknown endpoint type: %s", type.c_str());
    return RT_ERROR;
  }
  return RT_OK;  
}

rtError
rtRemoteCombineDocuments(rtJsonDocPtr& target, rtJsonDocPtr& source)
{
  RT_ASSERT(target->IsObject());
  RT_ASSERT(source->IsObject());
  for (rapidjson::Value::MemberIterator itr = source->MemberBegin(); itr != source->MemberEnd(); ++itr)
        target->AddMember(itr->name, itr->value, target->GetAllocator());
  return RT_OK;
}