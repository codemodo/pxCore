#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

// BASE //
rtRemoteIAddress::rtRemoteIAddress(std::string const& scheme)
  : m_scheme(scheme)
{ }

rtRemoteIAddress::~rtRemoteIAddress() { }

// LOCAL //
rtRemoteLocalAddress::rtRemoteLocalAddress(std::string const& scheme, std::string const& path)
  : rtRemoteIAddress(scheme)
  , m_path(path)
{ }

bool
rtRemoteLocalAddress::isSocket()
{
  return m_scheme.compare("tcp") == 0 || m_scheme.compare("udp") == 0;
}

std::string
rtRemoteLocalAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_path;
  return buff.str();
}

// NETWORK //

rtRemoteNetAddress::rtRemoteNetAddress(std::string const& scheme, std::string const& host, int port)
  : rtRemoteIAddress(scheme)
  , m_host(host)
  , m_port(port)
  , m_net_type(NetType::UNK)
  , m_cast_type(CastType::UNK)
{ }

rtRemoteNetAddress::rtRemoteNetAddress(std::string const& scheme, std::string const& host, int port, NetType nt, CastType ct)
  : rtRemoteIAddress(scheme)
  , m_host(host)
  , m_port(port)
  , m_net_type(nt)
  , m_cast_type(ct)
{ }

std::string
rtRemoteNetAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_host;
  buff << ":";
  buff << m_port;
  return buff.str();
}

// NETWORK + PATH
rtRemoteDistributedAddress::rtRemoteDistributedAddress(std::string const& scheme, std::string const& host, int port, std::string const& path)
  : rtRemoteIAddress(scheme)
  , rtRemoteNetAddress(scheme, host, port)
  , rtRemoteLocalAddress(scheme, path)
{ }

std::string
rtRemoteDistributedAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_host;
  buff << ":";
  buff << m_port;
  buff << m_path;
  return buff.str();
}

// static NetType rtRemoteParseNetType(std::string const& host);
// static CastType rtRemoteParseCastType(std::string const& host, NetType net_type);
// static rtError EndpointAddressToSocket(rtRemoteIAddress const& addr, sockaddr_storage* sock);
// static rtError SocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteIAddress*& endpoint_addr);

// NetType
// rtRemoteParseNetType(std::string const& host)
// {
//   NetType result;

//   struct addrinfo hint, *res = NULL;
//   int ret;
//   memset(&hint, 0, sizeof hint);
  
//   hint.ai_family = AF_UNSPEC;
//   hint.ai_flags = AI_NUMERICHOST;

//   ret = getaddrinfo(host.c_str(), NULL, &hint, &res);
//   if (ret) {
//     rtLogWarn("uri contains invalid host address format: %s", host.c_str());
//     return NetType::UNK;
//   }
//   if(res->ai_family == AF_INET) {
//     result = NetType::IPV4;
//   } else if (res->ai_family == AF_INET6) {
//     result = NetType::IPV6;
//   } else {
//     rtLogWarn("uri contains unknown host address format: %s", host.c_str());
//     return NetType::UNK;
//   }
//   freeaddrinfo(res);

//   return result;
// }

// CastType
// rtRemoteParseCastType(std::string const& host, NetType net_type)
// {
//   CastType result;

//   std::string prefix;
//   if (net_type == NetType::IPV4)
//   {
//     prefix = host.substr(0, host.find('.'));
//     if (stoi(prefix) >= 224 || stoi(prefix) <= 239)
//       result = CastType::MULTI;
//     else
//       result = CastType::UNI;
//   }
//   else if (net_type == NetType::IPV6)
//   {
//     prefix = host.substr(0, 2);
//     if (prefix.compare("FF") == 0)
//       result = CastType::MULTI;
//     else
//       result = CastType::UNI;
//   }
//   else
//    result = CastType::UNK;
  
//   return result;       
// }

// rtError
// EndpointAddressToSocket(rtRemoteIAddress& addr, sockaddr_storage& ss)
// {
//   if (typeid(addr) == typeid(rtRemoteLocalAddress))
//   {
//     rtRemoteLocalAddress* tmp = reinterpret_cast<rtRemoteLocalAddress*>(&addr);
//     if (!tmp->isSocket())
//     {
//       rtLogError("local address is not unix domain socket");
//       return RT_FAIL;
//     }
//     return rtParseAddress(ss, tmp->path().c_str(), 0, nullptr);
//   }
//   else if (typeid(addr) == typeid(rtRemoteNetAddress))
//   {
//     rtRemoteNetAddress* tmp = reinterpret_cast<rtRemoteNetAddress*>(&addr);
//     return rtParseAddress(ss,tmp->host().c_str(), tmp->port(), nullptr);
//   }
//   else
//   {
//     return RT_FAIL;
//   }
// }

// rtError
// SocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteIAddress*& endpoint_addr)
// {
//   std::stringstream buff;
  
//   std::string scheme;
//   if (conn_type == ConnType::STREAM)
//   {
//     scheme = "tcp";
//   }
//   else if (conn_type == ConnType::DGRAM)
//   {
//     scheme = "udp";
//   }
//   else
//   {
//     rtLogError("no connection protocol indicated while converting from socket to endpoint address");
//     return RT_FAIL;
//   }

//   buff << scheme;
//   buff << "://";

//   void* addr = NULL;
//   rtGetInetAddr(ss, &addr);

//   char addr_buff[128];
//   memset(addr_buff, 0, sizeof(addr_buff));

//   // TODO need to figure out env thing
//   if (ss.ss_family == AF_UNIX)
//   {
//     strncpy(addr_buff, (const char*)addr, sizeof(addr_buff) -1);
//     buff << addr_buff;
//     return createTcpAddress(buff.str(), endpoint_addr);
//   }
//   else
//   {
//     inet_ntop(ss.ss_family, addr, addr_buff, sizeof(addr_buff));
//     uint16_t port;
//     rtGetPort(ss, &port);
//     buff << addr_buff;
//     buff << ":";
//     buff << port;
//     return createTcpAddress(buff.str(), endpoint_addr);
//   }
//   return RT_OK;
// }