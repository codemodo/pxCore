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
rtRemoteLocalAddress::isSocket() const
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

rtRemoteIEndpoint::rtRemoteIEndpoint(rtRemoteAddrPtr ep_addr)
: m_addr(ep_addr)
, m_fd(-1)
{
  // empty
}

rtRemoteIEndpoint::~rtRemoteIEndpoint()
{
  if (m_fd != -1)
    ::close(m_fd);
  m_fd = -1;
}

rtRemoteStreamServerEndpoint::rtRemoteStreamServerEndpoint(rtRemoteAddrPtr ep_addr)
: rtRemoteIEndpoint(ep_addr)
{
  memset(&m_socket, 0, sizeof(sockaddr_storage));
}

rtError
rtRemoteStreamServerEndpoint::open()
{
  if (m_addr == nullptr)
  {
    rtLogError("failed to open endpoint socket: endpoint address is null");
    return RT_FAIL;
  }

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

  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::close()
{
  if (m_fd != -1)
    ::close(m_fd);
  m_fd = -1;
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
rtRemoteStreamServerEndpoint::doAccept(int& new_fd, rtRemoteAddrPtr& remote_addr)
{
  sockaddr_storage remote_endpoint;
  memset(&remote_endpoint, 0, sizeof(remote_endpoint));

  socklen_t len = sizeof(sockaddr_storage);

  new_fd = accept(m_fd, reinterpret_cast<struct sockaddr*>(&remote_endpoint), &len);

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
rtRemoteStreamServerEndpoint::send(int /*fd*/)
{
  return RT_OK;
}

rtError
rtRemoteStreamServerEndpoint::receive(int /*fd*/)
{
  return RT_OK;
}
/*
rtRemoteStreamClientEndpoint::rtRemoteStreamClientEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteStreamClientEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteStreamClientEndpoint::close()
{
  // TODO
  return RT_OK;
}

rtRemoteDatagramServerEndpoint::rtRemoteDatagramServerEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteDatagramServerEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteDatagramServerEndpoint::close()
{
  // TODO
  return RT_OK;
}


rtRemoteDatagramClientEndpoint::rtRemoteDatagramClientEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteDatagramClientEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteDatagramClientEndpoint::close()
{
  // TODO
  return RT_OK;
}


rtRemoteSharedMemoryEndpoint::rtRemoteSharedMemoryEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteSharedMemoryEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteSharedMemoryEndpoint::close()
{
  // TODO
  return RT_OK;
}


rtRemoteFileEndpoint::rtRemoteFileEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteFileEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteFileEndpoint::close()
{
  // TODO
  return RT_OK;
}


rtRemoteNamedPipeEndpoint::rtRemoteNamedPipeEndpoint(const rtRemoteIAddress* const addr)
: rtRemoteIEndpoint(addr)
{
  // TODO
}

rtError
rtRemoteNamedPipeEndpoint::open()
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteNamedPipeEndpoint::close()
{
  // TODO
  return RT_OK;
}
*/