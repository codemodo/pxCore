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

rtEndpointAddr const& rtRemoteEndpointCreate(rtRemoteEnvPtr env, std::string s)
{

}

struct rtEndpointAddr
{
  std::string uri;
  std::string scheme;
};

struct rtEndpointAddrUnix : rtEndpointAddr
{
  std::string path;
};

struct rtEndpoingAddrInet : rtEndpointAddr
{
  std::string ip;
  int port;
};

class rtRemoteIResource
{
public:
  rtRemoteIResource(rtEndpointAddr const& ep_addr);
  ~rtRemoteIResource();

  virtual rtError Open(int* fd) = 0;
  
  inline rtEndpointAddr GetEndpointAddr() const
    { return m_endpoint_addr; }
  
  inline int GetFd() const
    { return m_fd; }

protected:
  rtEndpointAddr m_endpoint_addr;
  int m_fd;
};

class rtRemoteILocalResource : public virtual rtRemoteIResource
{
public:
  rtRemoteILocalResource(rtEndpointAddr const& ep_addr);
  ~rtRemoteILocalResource();
};


class rtRemoteINetworkResource : public virtual rtRemoteIResource
{
public:
  rtRemoteINetworkResource(rtEndpointAddr const& ep_addr);
  ~rtRemoteINetworkResource();
};