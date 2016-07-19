#ifndef __RT_REMOTE_ENDPOINT_H__
#define __RT_REMOTE_ENDPOINT_H__

#include <string>
#include "rtRemoteTypes.h"
#include "rtRemoteUtils.h"


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






class rtRemoteIEndpoint
{
public:
  rtRemoteIEndpoint(rtRemoteIAddress*& addr);
  virtual ~rtRemoteIEndpoint();

  virtual rtError open() = 0;
  
  inline rtRemoteIAddress* GetEndpointAddr() const
    { return m_addr; }
  
  inline int fd() const
    { return m_listen_fd; }

	inline void SetFd(int fd)
	  { m_listen_fd = fd; }

	inline void SetAddr(rtRemoteIAddress* addr)
	  { m_addr = addr; }

protected:
  rtRemoteIAddress* m_addr;
  int m_listen_fd;
};

class rtRemoteServerEndpoint : public rtRemoteIEndpoint
{
public:
  rtRemoteServerEndpoint(rtRemoteIAddress*& addr);

  virtual rtError open() override;
	rtError accepts(rtRemoteIAddress*& addr);
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