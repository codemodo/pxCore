#include "rtRemoteIResolver.h"
#include "rtRemoteTypes.h"

#include <string>
#include <stdint.h>

class rtRemoteLocalResolver : public rtRemoteIResolver
{
public:
  rtRemoteLocalResolver(rtRemoteEnvPtr env);
  ~rtRemoteLocalResolver();

public:
  virtual rtError open() override;
  virtual rtError close() override;
  virtual rtError registerObject(std::string const& name, rtRemoteEndpointPtr endpoint) override;
  virtual rtError locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t timeout) override;
  virtual rtError deregisterObject(std::string const& name) override;

private:
  FILE*          m_db_fp;
  rtRemoteEnvPtr m_env;
};