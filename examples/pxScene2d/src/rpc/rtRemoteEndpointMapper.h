#ifndef __RT_REMOTE_ENDPOINT_MAPPER_H__
#define __RT_REMOTE_ENDPOINT_MAPPER_H__

#include "rtRemoteTypes.h"
#include <string>
#include <mutex>
#include <map>

class rtRemoteIEndpointMapper
{
public:
  virtual ~rtRemoteIEndpointMapper() { }
  virtual rtError registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) = 0;
  virtual rtError deregister(std::string const& objectId) = 0;
  virtual rtError locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint) = 0; 
  virtual bool isRegistered(std::string const& objectId) = 0; 
};

class rtRemoteEndpointMapperSimple : public virtual rtRemoteIEndpointMapper
{
public:
  rtRemoteEndpointMapperSimple();
public:
  virtual rtError registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) override;
  virtual rtError deregister(std::string const& objectId) override;
  virtual rtError locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint) override; 
  virtual bool isRegistered(std::string const& objectId) override;
private:
  using HostedObjectsMap = std::map< std::string, rtRemoteEndpointPtr >;
private: 
  HostedObjectsMap m_hosted_objects;
  std::mutex       m_mutex;
};

class rtRemoteEndpointMapperFile : public virtual rtRemoteIEndpointMapper
{
public:
  rtRemoteEndpointMapperFile();
public:
  virtual rtError registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) override;
  virtual rtError deregister(std::string const& objectId) override;
  virtual rtError locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint) override; 
  virtual bool isRegistered(std::string const& objectId) override;
};

// class rtRemoteEndpointMapperDatabase : public rtRemoteIEndpointMapper
// {

// }
#endif