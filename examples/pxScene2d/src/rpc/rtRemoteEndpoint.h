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

	bool isSocket() const;

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
	
	virtual std::string toUri() override;

	inline std::string host() const
	  { return m_host; }

	inline int port() const
	  { return m_port; }

protected:
	std::string         m_host;
	int                 m_port;
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
  virtual ~rtRemoteIEndpoint();

  /* Should create fd */
  virtual rtError open() = 0;
	virtual rtError close() = 0;
  
  // inline rtRemoteIAddress address() const
  //   { return *m_addr; }
  
  inline int fd() const
    { return m_fd; }

	inline void setFd(int fd)
	  { m_fd = fd; }

protected:
  rtRemoteIEndpoint(rtRemoteAddrPtr ep_addr);
  rtRemoteAddrPtr m_addr;
  int m_fd;
};

class rtRemoteIStreamEndpoint
{
	//TODO constructors and args for below methods
  virtual rtError send(int fd) = 0;
	virtual rtError receive(int fd) = 0;
};

class rtRemoteStreamServerEndpoint : public virtual rtRemoteIEndpoint, public rtRemoteIStreamEndpoint
{
public:
  rtRemoteStreamServerEndpoint(rtRemoteAddrPtr ep_addr);
  
  virtual rtError open() override;
	virtual rtError close() override;
	virtual rtError send(int fd) override;
	virtual rtError receive(int fd) override;
	rtError doBind();
	rtError doListen();
	rtError doAccept(int& new_fd, rtRemoteAddrPtr& remote_addr);

	inline sockaddr_storage sockaddr() const
	  { return m_socket; }

	sockaddr_storage m_socket;
};
/*
class rtRemoteStreamClientEndpoint : public virtual rtRemoteIEndpoint, public rtRemoteIStreamEndpoint
{
public:
  rtRemoteStreamClientEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
	//virtual rtError send(int fd) override;
	//virtual rtError receive(int fd) override;
};

class rtRemoteDatagramServerEndpoint : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteDatagramServerEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
  // rtError doBind();
	// rtError doListen();
	// rtError receive();
};

class rtRemoteDatagramClientEndpoint : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteDatagramClientEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
	// rtError send();
};

class rtRemoteSharedMemoryEndpoint : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteSharedMemoryEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteFileEndpoint : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteFileEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteNamedPipeEndpoint : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteNamedPipeEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};
*/
#endif