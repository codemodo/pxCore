#ifndef __RT_REMOTE_ENDPOINT_H__
#define __RT_REMOTE_ENDPOINT_H__

#include "rtRemoteTypes.h"
#include <sys/socket.h>
#include <string>
#include <sstream>
#include <stdint.h>

/* Abstract base class for endpoint addresses */
class rtRemoteIEndpoint
{
public:
	rtRemoteIEndpoint(std::string const& scheme): m_scheme(scheme) { };
	virtual ~rtRemoteIEndpoint() { };

	virtual std::string toUriString() = 0;

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
  rtRemoteEndpointLocal(std::string const& scheme, std::string const& path)
  : rtRemoteIEndpoint(scheme)
  , m_path(path)
  { }
	
  virtual std::string toUriString() override
  {
    std::stringstream buff;
    buff << m_scheme;
    buff << "://";
    buff << m_path;
    return buff.str();
  }
	
  inline bool operator==(rtRemoteEndpointLocal const& rhs) const
    { return m_path.compare(rhs.path()) == 0
		  && m_scheme.compare(rhs.scheme()) == 0;
	}

  inline std::string path() const
	{ return m_path; }

  inline bool isSocket() const
    { return m_scheme.compare("tcp") == 0 || m_scheme.compare("udp") == 0; }

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
  rtRemoteEndpointRemote(std::string const& scheme, std::string const& host, int port)
    : rtRemoteIEndpoint(scheme)
    , m_host(host)
    , m_port(port)
  { }
	
  virtual std::string toUriString() override
  {
    std::stringstream buff;
    buff << m_scheme;
    buff << "://";
    buff << m_host;
    buff << ":";
    buff << m_port;
    return buff.str();
  }

  inline bool operator==(rtRemoteEndpointRemote const& rhs) const
    { return m_host.compare(rhs.host()) == 0
	      && m_port == rhs.port()
          && m_scheme.compare(rhs.scheme()) == 0;
	}

  inline std::string host() const
	{ return m_host; }

  inline int port() const
    { return m_port; }

protected:
  std::string         m_host;
  int                 m_port;
};


/* Remote endpoints that include path information
 *
 * Currently not used.  If use case arises, must be integrated throughout the code.
 */
class rtRemoteEndpointDistributed : public rtRemoteEndpointRemote, public rtRemoteEndpointLocal
{
public:
  rtRemoteEndpointDistributed(std::string const& scheme, std::string const& host, int port, std::string const& path)
    : rtRemoteIEndpoint(scheme)
    , rtRemoteEndpointRemote(scheme, host, port)
    , rtRemoteEndpointLocal(scheme, path)
  { }
	
  virtual std::string toUriString() override
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

  inline bool operator==(rtRemoteEndpointDistributed const& rhs) const
	{ return m_host.compare(rhs.host()) == 0
		  && m_port == rhs.port()
		  && m_path.compare(rhs.path()) == 0
		  && m_scheme.compare(rhs.scheme()) == 0;
    }
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