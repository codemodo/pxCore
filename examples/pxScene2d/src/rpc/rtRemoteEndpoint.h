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

endpoint_addr const& rtEndpointAddrCreate(std::string uri)
{
  
}

// class rtIEndpoint
// class rtIEndpointConnection
// class rtIEndpointLocal
// class rtIEndpointRemote

// concrete implementations
// class rtEndpointPipe
struct endpoint_addr
{
  endpoint_family_t family;
  std::string uri; // maybe have array of key:value pairs here?
};

struct endpoint_addr_local : endpoint_addr
{
  std::string path;
};

struct endpoint_addr_remote : endpoint_addr, endpoint_addr_conn
{
  std::string ip;
  int port;
};

struct endpoint_addr_conn: endpoint_addr
{
  endpoint_connection_t connection;
}

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