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


/////////////////////////
// Endpoint abstractions                 
/////////////////////////

rtRemoteIEndpoint::rtRemoteIEndpoint(rtRemoteIAddress* const addr)
: m_addr(addr)
, m_fd(-1)
{
  // empty
}

rtRemoteIEndpoint::~rtRemoteIEndpoint()
{
}

rtRemoteStreamServerEndpoint::rtRemoteStreamServerEndpoint(rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  memset(&m_socket, 0, sizeof(sockaddr_storage));
}

rtError
rtRemoteStreamServerEndpoint::open()
{
  // TODO this should prob go in constructor, but need to throw exception in that case
  rtError e = rtRemoteEndpointAddressToSocket(m_addr, m_socket);
  if (e != RT_OK)
  {
    rtLogError("failed to convert from endpoint address to sockaddr");
    return e;
  }
  
  // open socket
  m_fd = socket(m_socket.ss_family, SOCK_STREAM, 0);
  if (m_fd < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to create socket. %s", rtStrError(e));
    return e;
  }

  fcntl(m_fd, F_SETFD, fcntl(m_fd, F_GETFD) | FD_CLOEXEC);

  if (m_socket.ss_family != AF_UNIX)
  {
    uint32_t one = 1;
    if (-1 == setsockopt(m_fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)))
      rtLogError("setting TCP_NODELAY failed");
  }

  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::close()
{
  if (m_fd != -1)
    ::close(m_fd);
  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::doBind()
{
  int ret;
  socklen_t len;
  rtSocketGetLength(m_socket, &len);

  ret = bind(m_fd, (struct sockaddr*)(&m_socket), len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to bind socket. %s", rtStrError(e));
    return e;
  }

  rtGetSockName(m_fd, m_socket);
  rtLogInfo("local rpc listener on: %s", rtSocketToString(m_socket).c_str());

  ret = fcntl(m_fd, F_SETFL, O_NONBLOCK);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("fcntl: %s", rtStrError(e));
    return e;
  }
  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::doListen()
{
  int ret;
  ret = listen(m_fd, 2);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to put socket in listen mode. %s", rtStrError(e));
    return e;
  }
  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::doAccept(int& new_fd, rtRemoteIAddress*& remote_addr)
{
  sockaddr_storage remote_endpoint;
  memset(&remote_endpoint, 0, sizeof(remote_endpoint));

  socklen_t len = sizeof(sockaddr_storage);

  new_fd = accept(m_fd, (struct sockaddr*)(&remote_endpoint), &len);

  if (new_fd == -1)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogWarn("error accepting new tcp connect. %s", rtStrError(e));
    return RT_FAIL;
  }
  rtLogInfo("new connection from %s with fd:%d", rtSocketToString(remote_endpoint).c_str(), new_fd);
  return rtRemoteSocketToEndpointAddress(remote_endpoint, ConnType::STREAM, remote_addr);
}

rtError
rtRemoteStreamServerEndpoint::doAccept(int& new_fd, sockaddr_storage& remote_endpoint)
{


  socklen_t len = sizeof(sockaddr_storage);

  new_fd = accept(m_fd, (struct sockaddr*)(&remote_endpoint), &len);

  if (new_fd == -1)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogWarn("error accepting new tcp connect. %s", rtStrError(e));
    return RT_FAIL;
  }
  rtLogInfo("new connection from %s with fd:%d", rtSocketToString(remote_endpoint).c_str(), new_fd);
  return RT_OK;
  //return rtRemoteSocketToEndpointAddress(remote_endpoint, ConnType::STREAM, remote_addr);
}

rtError
rtRemoteStreamServerEndpoint::send(int /*fd*/)
{
  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::receive(int /*fd*/)
{
  return RT_OK;
}