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
class rtRemoteIEndpoint
{
public:
	rtRemoteIEndpoint(std::string const& scheme);
	virtual ~rtRemoteIEndpoint();

	virtual std::string toUri() = 0;

	inline std::string scheme() const
	  { return m_scheme; }

protected:
	std::string m_scheme;
};

/* Local endpoints.
 * Used to stored address information for unix domain sockets,
 * named pipes, files, shared memory, etc.
 */
class rtRemoteEndpointLocal : public virtual rtRemoteIEndpoint
{
public:
	rtRemoteEndpointLocal(std::string const& scheme, std::string const& path);
	
	virtual std::string toUri() override;

	bool isSocket() const;

	inline std::string path() const
	  { return m_path; }

protected:
	std::string m_path;
};

/* Remote endpoints.
 * Used to stored address information for remote sockets
 * (tcp, udp, http, etc.)
 */
class rtRemoteEndpointRemote : public virtual rtRemoteIEndpoint
{
public:
  rtRemoteEndpointRemote(std::string const& scheme, std::string const& host, int port);
	
	virtual std::string toUri() override;

	inline std::string host() const
	  { return m_host; }

	inline int port() const
	  { return m_port; }

protected:
	std::string         m_host;
	int                 m_port;
};

/* Remote endpoints that include path information*/
class rtRemoteEndpointDistributed : public rtRemoteEndpointRemote, public rtRemoteEndpointLocal
{
public:
	rtRemoteEndpointDistributed(std::string const& scheme, std::string const& host, int port, std::string const& path);
	virtual std::string toUri() override;
};



/////////////////////////
// Endpoint handles                 
/////////////////////////

class rtRemoteIEndpointHandle
{
public:
  virtual ~rtRemoteIEndpointHandle();

  /* Should create fd */
  virtual rtError open() = 0;
	virtual rtError close() = 0;
  
  // inline rtRemoteIEndpoint address() const
  //   { return *m_addr; }
  
  inline int fd() const
    { return m_fd; }

	inline void setFd(int fd)
	  { m_fd = fd; }

protected:
  rtRemoteIEndpointHandle(rtRemoteEndpointPtr endpoint);
  rtRemoteEndpointPtr m_addr;
  int m_fd;
};

class rtRemoteIStreamEndpoint
{
	//TODO constructors and args for below methods
  virtual rtError send(int fd) = 0;
	virtual rtError receive(int fd) = 0;
};

class rtRemoteStreamServerEndpoint : public virtual rtRemoteIEndpointHandle, public rtRemoteIStreamEndpoint
{
public:
  rtRemoteStreamServerEndpoint(rtRemoteEndpointPtr endpoint);
  
  virtual rtError open() override;
	virtual rtError close() override;
	virtual rtError send(int fd) override;
	virtual rtError receive(int fd) override;
	rtError doBind();
	rtError doListen();
	rtError doAccept(int& new_fd, rtRemoteEndpointPtr& remote_addr);

	inline sockaddr_storage sockaddr() const
	  { return m_socket; }

	sockaddr_storage m_socket;
};
/*
class rtRemoteStreamClientEndpoint : public virtual rtRemoteIEndpointHandle, public rtRemoteIStreamEndpoint
{
public:
  rtRemoteStreamClientEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
	//virtual rtError send(int fd) override;
	//virtual rtError receive(int fd) override;
};

class rtRemoteDatagramServerEndpoint : public virtual rtRemoteIEndpointHandle
{
public:
  rtRemoteDatagramServerEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
  // rtError doBind();
	// rtError doListen();
	// rtError receive();
};

class rtRemoteDatagramClientEndpoint : public virtual rtRemoteIEndpointHandle
{
public:
  rtRemoteDatagramClientEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
	// rtError send();
};

class rtRemoteSharedMemoryEndpoint : public virtual rtRemoteIEndpointHandle
{
public:
  rtRemoteSharedMemoryEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteFileEndpoint : public virtual rtRemoteIEndpointHandle
{
public:
  rtRemoteFileEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};

class rtRemoteNamedPipeEndpoint : public virtual rtRemoteIEndpointHandle
{
public:
  rtRemoteNamedPipeEndpoint(const rtRemoteIAddress* const addr);
	virtual rtError open() override;
	virtual rtError close() override;
};
*/
#endif