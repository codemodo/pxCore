#ifndef __RT_REMOTE_ENDPOINT_MAPPER_SQL_H__
#define __RT_REMOTE_ENDPOINT_MAPPER_SQL_H__

#include "rtRemoteMapper.h"
#include "rtRemoteTypes.h"
#include <string>

class rtRemoteMapperSql : public virtual rtRemoteIMapper
{
public:
  rtRemoteMapperSql(rtRemoteEnvPtr env);
  //~rtRemoteMapperSql();

public:
  virtual rtError registerEndpoint(std::string const& objectId, rtRemoteEndpointPtr const& endpoint) override;
  virtual rtError deregisterEndpoint(std::string const& objectId) override;
  virtual rtError lookupEndpoint(std::string const& objectId, rtRemoteEndpointPtr& endpoint) override; 
  virtual bool    isRegistered(std::string const& objectId) override;
};

#endif