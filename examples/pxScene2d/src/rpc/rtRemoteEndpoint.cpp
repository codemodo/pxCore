#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
#include "rtRemoteUtils.h"
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <set>
#include <algorithm>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <rtLog.h>
#include <dirent.h>

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

rtRemoteIEndpoint::rtRemoteIEndpoint(rtRemoteIAddress*& addr)
: m_listen_fd(-1)
, m_addr(addr)
{
  // empty
}

rtRemoteIEndpoint::~rtRemoteIEndpoint()
{
  // empty
}

rtRemoteServerEndpoint::rtRemoteServerEndpoint(rtRemoteIAddress*& addr)
: rtRemoteIEndpoint(addr)
{
  // empty
}

rtError
rtRemoteServerEndpoint::open()
{
  int ret;
  sockaddr_storage m_rpc_socket;
  rtError err = rtRemoteEndpointAddressToSocket(m_addr, m_rpc_socket);
  m_listen_fd = socket(m_rpc_socket.ss_family, SOCK_STREAM, 0);
  if (m_listen_fd < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to create socket. %s", rtStrError(e));
    return e;
  }

  fcntl(m_listen_fd, F_SETFD, fcntl(m_listen_fd, F_GETFD) | FD_CLOEXEC);

  if (m_rpc_socket.ss_family != AF_UNIX)
  {
    uint32_t one = 1;
    if (-1 == setsockopt(m_listen_fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)))
      rtLogError("setting TCP_NODELAY failed");
  }

  socklen_t len;
  rtSocketGetLength(m_rpc_socket, &len);

  ret = ::bind(m_listen_fd, reinterpret_cast<sockaddr *>(&m_rpc_socket), len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to bind socket. %s", rtStrError(e));
    return e;
  }

  rtGetSockName(m_listen_fd, m_rpc_socket);
  rtLogInfo("local rpc listener on: %s", rtSocketToString(m_rpc_socket).c_str());

  ret = fcntl(m_listen_fd, F_SETFL, O_NONBLOCK);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("fcntl: %s", rtStrError(e));
    return e;
  }

  ret = listen(m_listen_fd, 2);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to put socket in listen mode. %s", rtStrError(e));
    return e;
  }
}

rtError
rtRemoteServerEndpoint::accepts(rtRemoteIAddress*& peer)
{
  sockaddr_storage remote_endpoint;
  memset(&remote_endpoint, 0, sizeof(remote_endpoint));

  socklen_t len = sizeof(sockaddr_storage);

  int ret = accept(m_listen_fd, reinterpret_cast<sockaddr *>(&remote_endpoint), &len);
  if (ret == -1)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogWarn("error accepting new tcp connect. %s", rtStrError(e));
    return RT_FAIL;
  }
  rtLogInfo("new connection from %s with fd:%d", rtSocketToString(remote_endpoint).c_str(), ret);
  return rtRemoteSocketToEndpointAddress(remote_endpoint, ConnType::STREAM, peer);

}