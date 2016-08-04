#include "rtRemoteIResolver.h"
#include "rtRemoteTypes.h"

#include <string>
#include <stdint.h>
#include "rtRemoteEndpointMapper.h"

class rtRemoteResolverLocal : public rtRemoteIResolver
{
public:
  rtRemoteResolverLocal(rtRemoteEnvPtr env);
  ~rtRemoteResolverLocal();

public:
  virtual rtError open() override;
  virtual rtError close() override;
  virtual rtError registerObject(std::string const& name, rtRemoteEndpointPtr endpoint) override;
  virtual rtError locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t timeout) override;
  virtual rtError deregisterObject(std::string const& name) override;

private:
  rtRemoteEnvPtr m_env;
  rtRemoteIEndpointMapper* m_endpoint_mapper;
};