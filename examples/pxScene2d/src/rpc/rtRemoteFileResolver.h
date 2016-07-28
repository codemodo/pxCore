#include "rtRemoteIResolver.h"
#include "rtRemoteTypes.h"

#include <string>
#include <stdint.h>

class rtRemoteFileResolver : public rtRemoteIResolver
{
public:
  rtRemoteFileResolver(rtRemoteEnvPtr env);
  ~rtRemoteFileResolver();

public:
  virtual rtError open() override;
  virtual rtError close() override;
  virtual rtError registerObject(std::string const& name, rtRemoteEndpointPtr endpoint) override;
  virtual rtError locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t timeout) override;

private:
  FILE*             m_db_fp;
  rtRemoteEnvPtr    m_env;
};