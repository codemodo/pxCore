#ifndef __RT_REMOTE_ENDPOINT_H__
#define __RT_REMOTE_ENDPOINT_H__

#include <string>
#include "rtRemoteTypes.h"
#include "rtRemoteUtils.h"
#include <sys/socket.h>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <stdint.h>
#include <netinet/in.h>
#include <rtObject.h>
#include <rapidjson/document.h>

#include "rtRemoteIResolver.h"
#include "rtSocketUtils.h"
#include "rtRemoteEndpoint.h"




/* Abstract base class for endpoint addresses */
class rtRemoteIAddress
{
public:
	rtRemoteIAddress(std::string const& scheme);
	virtual ~rtRemoteIAddress();

	virtual std::string toUri() = 0;

	inline std::string scheme() const
	  { return m_scheme; }

protected:
	std::string m_scheme;
};

/* Local endpoint addresses */
class rtRemoteLocalAddress : public virtual rtRemoteIAddress
{
public:
	rtRemoteLocalAddress(std::string const& scheme, std::string const& path);
	
	virtual std::string toUri() override;

	bool isSocket();

	inline std::string path() const
	  { return m_path; }

protected:
	std::string m_path;
};

/* Remote endpoint addresses */
class rtRemoteNetAddress : public virtual rtRemoteIAddress
{
public:
  rtRemoteNetAddress(std::string const& scheme, std::string const& host, int port);
	rtRemoteNetAddress(std::string const& scheme, std::string const& host, int port, NetType nt, CastType ct);
	
	virtual std::string toUri() override;

	inline std::string host() const
	  { return m_host; }

	inline int port() const
	  { return m_port; }

	inline NetType netType() const
	  { return m_net_type; }

	inline CastType castType() const
	  { return m_cast_type; }

protected:
	std::string         m_host;
	int                 m_port;
	NetType  m_net_type;
	CastType m_cast_type;
};

/* Remote endpoint addresses with path */
class rtRemoteDistributedAddress : public rtRemoteNetAddress, public rtRemoteLocalAddress
{
public:
	rtRemoteDistributedAddress(std::string const& scheme, std::string const& host, int port, std::string const& path);
	virtual std::string toUri() override;
};




/////////////////////////
// Endpoint abstractions                 
/////////////////////////

class rtRemoteIEndpoint
{
public:
  rtRemoteIEndpoint(rtRemoteIAddress* const addr);
  virtual ~rtRemoteIEndpoint();

  /* Should create fd */
  virtual rtError open() = 0;
	virtual rtError close() = 0;
  
  inline rtRemoteIAddress* address() const
    { return m_addr; }
  
  inline int fd() const
    { return m_fd; }

	inline void setFd(int fd)
	  { m_fd = fd; }

	inline void setAddr(rtRemoteIAddress* addr)
	  { m_addr = addr; }

protected:
  rtRemoteIAddress* m_addr;
  int m_fd;
};

class rtRemoteStreamEndpoint
{
	//TODO constructors and args for below methods
  virtual rtError send(int fd) = 0;
	virtual rtError receive(int fd) = 0;
};

class rtRemoteStreamServerEndpoint : public rtRemoteStreamEndpoint, public virtual rtRemoteIEndpoint
{
public:
  rtRemoteStreamServerEndpoint(rtRemoteIAddress* const addr);
  
  virtual rtError open() override;
	virtual rtError close() override;
	virtual rtError send(int fd) override;
	virtual rtError receive(int fd) override;
	rtError doBind();
	rtError doListen();
	rtError doAccept(int& new_fd, rtRemoteIAddress*& remote_addr);

	inline sockaddr_storage sockaddr() const
	  { return m_socket; }

	sockaddr_storage m_socket;
};

class rtRemoteStreamClientEndpoint : public virtual rtRemoteIEndpoint
{
public:
  //rtRemoteStreamClientEndpoint(rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteDatagramServerEndpoint : public virtual rtRemoteIEndpoint
{
public:
  //rtRemoteDatagramServerEndpoint(rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteDatagramClientEndpoint : public virtual rtRemoteIEndpoint
{
public:
  //rtRemoteDatagramServerEndpoint(rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};


#endif

// class rtRemoteILocalResource : public virtual rtRemoteIResource
// {
// public:
//   rtRemoteILocalResource(rtEndpointAddr const& ep_addr);
//   ~rtRemoteILocalResource();
// };


// class rtRemoteINetworkResource : public virtual rtRemoteIResource
// {
// public:
//   rtRemoteINetworkResource(rtEndpointAddr const& ep_addr);
//   ~rtRemoteINetworkResource();
// };