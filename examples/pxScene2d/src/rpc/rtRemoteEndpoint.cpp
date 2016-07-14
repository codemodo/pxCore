#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"
#include "rtRemoteConfig.h"

#include <condition_variable>
#include <thread>
#include <mutex>

#include <rtLog.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

rtRemoteIResource::rtRemoteIResource(rtEndpointAddr const& ep_addr)
: m_uri(uri)
, m_scheme(nullptr)
, m_fd(-1)
{
  //pull out the scheme here
  m_scheme = "tcp"; // temp faking it
}

rtRemoteIResource::~rtRemoteIResource() { }


///////////////////////////////
///////////////////////////////


rtRemoteILocalResource::rtRemoteILocalResource(rtEndpointAddr const& ep_addr)
: rtRemoteIResource(ep_addr)
{ }

rtRemoteILocalResource::~rtRemoteILocalResource() { }

///////////////////////////////
///////////////////////////////

rtRemoteINetworkResource::rtRemoteINetworkResource(rtEndpointAddr const& ep_addr)
: rtRemoteIResource(ep_addr)
{
    // plus pull out the addr and port
    m_addr = "127.0.0.1"; //tmp faking it
    m_port = 49118;
    err = rtParseAddress(m_ep, m_addr.c_str(), m_port, nullptr);
}

rtRemoteINetworkResource::~rtRemoteINetworkResource() { }

//TODOfiuk this stuff needs to move out into server/client subclasses
rtError
rtRemoteINetworkResource::open(int* fd)
{
  // create socket
  rtError err = RT_OK;

  sockaddr_storage sckaddr;
  err = rtParseAddress(sckaddr, m_addr.c_str(), m_port, nullptr);
  if (err != RT_OK)
  {
    rtLogDebug("failed to parse endpoint: %s:%d %s", m_addr, m_port, rtStrError(err));
    return err;
  }
  
  m_fd = socket(sckaddr.ss_family, SOCK_STREAM, 0);
  if (m_fd < 0)
  {
    rtLogDebug("failed to create file descriptor for endpoint: %s", m_scheme);
    return RT_FAIL;
  }
  *fd = m_fd;

  // bind
  socklen_t len;
  rtSocketGetLength(sckaddr, &len);

  ret = bind(m_fd, reinterpret_cast<sockaddr *>(&sckaddr), len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to bind socket. %s", rtStrError(e));
    return e;
  }

  rtGetSockName(m_fd, sckaddr);
  rtLogInfo("local rpc listener on: %s", rtSocketToString(sckaddr).c_str());

  ret = fcntl(m_fd, F_SETFL, O_NONBLOCK);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("fcntl: %s", rtStrError(e));
    return e;
  }

  // start listen mode
  ret = listen(m_fd, 2);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to put socket in listen mode. %s", rtStrError(e));
    return e;
  }

  

  return RT_OK; // should be implemented by subclass
}

rtError
rtRemoteInetEndpoint::Connect()
{
  socklen_t len;
  rtSocketGetLength(m_sockaddr, &len);

  int ret = connect(m_fd, reinterpret_cast<sockaddr const *>(&m_sockaddr), len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to connect to remote rpc endpoint. %s", rtStrError(e));
    return e;
  }

  return RT_OK;
}