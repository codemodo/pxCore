#include "rtRemoteUtils.h"
#include "rtRemoteTypes.h"
#include <sstream>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <algorithm>
#include <memory>

rtError
rtRemoteParseNetType(std::string const& host, NetType& result)
{
  int ret;
  struct addrinfo hint, *res = NULL;
  memset(&hint, 0, sizeof hint);
  
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  ret = getaddrinfo(host.c_str(), NULL, &hint, &res);
  if (ret)
  {
    result = NetType::UNK;
    freeaddrinfo(res);
    rtError e = rtErrorFromErrno(errno);
    rtLogWarn("unable to determine NetType (network layer protocol) for host: %s", host.c_str());
    return e;
  }
  else
  {
    if (res->ai_family == AF_INET)
      result = NetType::IPV4;
    else if (res->ai_family == AF_INET6)
      result = NetType::IPV6;
    else
    {
      result = NetType::UNK;
      freeaddrinfo(res);
      rtLogWarn("unable to determine NetType (network layer protocol) for host: %s", host.c_str());
      return RT_FAIL;
    }
  }
  return RT_OK;
}

rtError
rtRemoteParseCastType(std::string const& host, NetType const& net_type, CastType& result)
{
  std::string prefix;
  if (net_type == NetType::IPV4)
  {
    prefix = host.substr(0, host.find('.'));
    if (stoi(prefix) >= 224 && stoi(prefix) <= 239)
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

  return RT_OK;       
}

rtError
rtRemoteEndpointAddressToSocket(rtRemoteAddrPtr addr, sockaddr_storage& ss)
{
  if (auto local = dynamic_pointer_cast<rtRemoteLocalAddress>(addr))
  {
    if (!local->isSocket())
    {
      rtLogError("local address is not unix domain socket");
      return RT_FAIL;
    }
    return rtParseAddress(ss, local->path().c_str(), 0, nullptr);
  }
  else if (auto net = dynamic_pointer_cast<rtRemoteNetAddress>(addr))
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
rtRemoteSocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteAddrPtr& endpoint_addr)
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

  if (ss.ss_family == AF_UNIX)
  {
    strncpy(addr_buff, (const char*)addr, sizeof(addr_buff) -1);
    buff << addr_buff;
    endpoint_addr = std::make_shared<rtRemoteLocalAddress>(scheme, addr_buff);
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
    endpoint_addr = std::make_shared<rtRemoteNetAddress>(scheme, addr_buff, port);
    return RT_OK;
  }
  return RT_OK;
}