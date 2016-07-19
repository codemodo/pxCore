#include "rtRemoteUtils.h"
#include <sstream>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>

NetType
rtRemoteParseNetType(std::string const& host)
{
  NetType result;

  struct addrinfo hint, *res = NULL;
  int ret;
  memset(&hint, 0, sizeof hint);
  
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  ret = getaddrinfo(host.c_str(), NULL, &hint, &res);
  if (ret) {
    rtLogWarn("uri contains invalid host address format: %s", host.c_str());
    return NetType::UNK;
  }
  if(res->ai_family == AF_INET) {
    result = NetType::IPV4;
  } else if (res->ai_family == AF_INET6) {
    result = NetType::IPV6;
  } else {
    rtLogWarn("uri contains unknown host address format: %s", host.c_str());
    return NetType::UNK;
  }
  freeaddrinfo(res);

  return result;
}

CastType
rtRemoteParseCastType(std::string const& host, NetType net_type)
{
  CastType result;

  std::string prefix;
  if (net_type == NetType::IPV4)
  {
    prefix = host.substr(0, host.find('.'));
    if (stoi(prefix) >= 224 || stoi(prefix) <= 239)
      result = CastType::MULTI;
    else
      result = CastType::UNI;
  }
  else if (net_type == NetType::IPV6)
  {
    prefix = host.substr(0, 2);
    if (prefix.compare("FF") == 0)
      result = CastType::MULTI;
    else
      result = CastType::UNI;
  }
  else
   result = CastType::UNK;
  
  return result;       
}

rtError
EndpointAddressToSocket(rtRemoteIAddress& addr, sockaddr_storage& ss)
{
  if (typeid(addr) == typeid(rtRemoteLocalAddress))
  {
    rtRemoteLocalAddress* tmp = reinterpret_cast<rtRemoteLocalAddress*>(&addr);
    if (!tmp->isSocket())
    {
      rtLogError("local address is not unix domain socket");
      return RT_FAIL;
    }
    return rtParseAddress(ss, tmp->path().c_str(), 0, nullptr);
  }
  else if (typeid(addr) == typeid(rtRemoteNetAddress))
  {
    rtRemoteNetAddress* tmp = reinterpret_cast<rtRemoteNetAddress*>(&addr);
    return rtParseAddress(ss,tmp->host().c_str(), tmp->port(), nullptr);
  }
  else
  {
    return RT_FAIL;
  }
}

rtError
SocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteIAddress*& endpoint_addr)
{
  std::stringstream buff;
  
  std::string scheme;
  if (conn_type == ConnType::STREAM)
  {
    scheme = "tcp";
  }
  else if (conn_type == ConnType::DGRAM)
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

  // TODO need to figure out env thing
  if (ss.ss_family == AF_UNIX)
  {
    strncpy(addr_buff, (const char*)addr, sizeof(addr_buff) -1);
    buff << addr_buff;
    return createTcpAddress(buff.str(), endpoint_addr);
  }
  else
  {
    inet_ntop(ss.ss_family, addr, addr_buff, sizeof(addr_buff));
    uint16_t port;
    rtGetPort(ss, &port);
    buff << addr_buff;
    buff << ":";
    buff << port;
    return createTcpAddress(buff.str(), endpoint_addr);
  }
  return RT_OK;
}

rtError
createTcpAddress(std::string const& uri, rtRemoteIAddress*& endpoint_addr)
{
  int index;
  index = uri.find("://");
  if (index == std::string::npos)
  {
   rtLogError("invalid uri: %s", uri.c_str());
   return RT_FAIL;
  }
  
  // extract scheme
  std::string scheme;
  scheme = uri.substr(0, index);
  char const* s = scheme.c_str();
  if (s != nullptr)
  {
    if (strcasecmp(s, "tcp") != 0)
    {
      rtLogError("cannot create tcp addr from uri: %s", uri.c_str());
      return RT_FAIL;
    }
  }

  // extract remaining info
  index += 3;
  char ch = uri.at(index);
  if (ch == '/' || ch == '.')
  {
    // local socket
    std::string path = uri.substr(index, std::string::npos);
    endpoint_addr = new rtRemoteLocalAddress(scheme, path);
  }
  else
  {
    // network socket
    std::string port_string;
    int index_port = uri.find_last_of(":");
    port_string = uri.substr(index_port+1, std::string::npos);
    int port = stoi(port_string);
    // TODO should check to make sure previous char wasn't also a colon
    // as well as if any result came back at all

    std::string host;
    host = uri.substr(index, index_port - index);

    NetType net_type = rtRemoteParseNetType(host);
    CastType cast_type = rtRemoteParseCastType(host, net_type);

    rtLogWarn("\n\nCreating address with %s, %d\n\n", host.c_str(), port);
    endpoint_addr = new rtRemoteNetAddress(scheme, host, port, net_type, cast_type);   
  }
  return RT_OK;
}