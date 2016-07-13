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

class rtRemoteEndpoint
{
public:
  virtual rtRemoteEndpoint(std::string const& uri);
  virtual ~rtRemoteEndpoint();
  rtError GetUri(std::string* uri) const;
  rtError GetScheme(std::string* scheme) const;
  rtError GetFd(int* fd) const;
  virtual rtError Open(int* fd);

protected:
  std::string m_uri;
  std::string m_scheme;
  int m_fd;
};


class rtRemoteUnixEndpoint : public virtual rtRemoteEndpoint
{
public:
  virtual rtRemoteUnixEndpoint(std::string const& uri);
  virtual ~rtRemoteUnixEndpoint();
  
  virtual rtError Open(int* fd) override;
  
  rtError GetPath(std::string* path) const;

protected:
  std::string m_path; 
};


class rtRemoteInetEndpoint : public virtual rtRemoteEndpoint
{
public:
  virtual rtRemoteInetEndpoint(std::string const& uri);
  virtual ~rtRemoteInetEndpoint();
  
  virtual rtError Open(int* fd) override;
  
  rtError GetPath(std::string* path) const;

protected:
  std::string m_addr;
  int m_port;
  sockaddr_storage m_ep; 
};