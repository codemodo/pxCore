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
  virtual rtError registerEndpoint(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) = 0;
  virtual rtError deregisterEndpoint(std::string const& objectId) = 0;
  virtual rtError lookupEndpoint(std::string const& objectId, rtRemoteEndpointPtr& endpoint) = 0; 
  virtual bool isRegistered(std::string const& objectId) = 0;

protected:
  rtRemoteIEndpointMapper(rtRemoteEnvPtr env)
  : m_env(env)
  { }
protected:
  rtRemoteEnvPtr m_env; 
};

class rtRemoteEndpointMapperSimple : public virtual rtRemoteIEndpointMapper
{
public:
  rtRemoteEndpointMapperSimple(rtRemoteEnvPtr env);
public:
  virtual rtError registerEndpoint(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) override;
  virtual rtError deregisterEndpoint(std::string const& objectId) override;
  virtual rtError lookupEndpoint(std::string const& objectId, rtRemoteEndpointPtr& endpoint) override; 
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
  rtRemoteEndpointMapperFile(rtRemoteEnvPtr env);
  ~rtRemoteEndpointMapperFile();
public:
  virtual rtError registerEndpoint(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) override;
  virtual rtError deregisterEndpoint(std::string const& objectId) override;
  virtual rtError lookupEndpoint(std::string const& objectId, rtRemoteEndpointPtr& endpoint) override; 
  virtual bool isRegistered(std::string const& objectId) override;
private:
  std::string m_file_path;
};

// class rtRemoteEndpointMapperDatabase : public rtRemoteIEndpointMapper
// {

// }
#endif