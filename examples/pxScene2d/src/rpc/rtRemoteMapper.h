#ifndef __RT_REMOTE_ENDPOINT_MAPPER_H__
#define __RT_REMOTE_ENDPOINT_MAPPER_H__

#include "rtRemoteTypes.h"
#include <string>

class rtRemoteIMapper
{
public:
  virtual ~rtRemoteIMapper() { }

public:
  virtual rtError registerEndpoint(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) = 0;
  virtual rtError deregisterEndpoint(std::string const& objectId) = 0;
  virtual rtError lookupEndpoint(std::string const& objectId, rtRemoteEndpointPtr& endpoint) = 0; 
  virtual bool    isRegistered(std::string const& objectId) = 0;

protected:
  rtRemoteIMapper(rtRemoteEnvPtr env)
  : m_env(env)
  { }

protected:
  rtRemoteEnvPtr m_env; 
};

#endif