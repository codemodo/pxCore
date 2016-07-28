#ifndef __RT_REMOTE_OBJECT_RESOLVER_H__
#define __RT_REMOTE_OBJECT_RESOLVER_H__

#include "rtRemoteTypes.h"

class rtRemoteIResolver
{
public:
  virtual ~rtRemoteIResolver() { }
  virtual rtError open() = 0;
  virtual rtError close() = 0;
  virtual rtError registerObject(std::string const& name, rtRemoteEndpointPtr endpoint) = 0;
  virtual rtError locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint, uint32_t timeout) = 0;
};

#endif
