#include "rtRemoteIResolver.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <stdint.h>
#include <netinet/in.h>
#include <rtObject.h>
#include <rapidjson/document.h>

#include "rtRemoteTypes.h"
#include "rtRemoteIResolver.h"
#include "rtSocketUtils.h"


class rtRemoteFileResolver : public rtRemoteIResolver
{
public:
  rtRemoteFileResolver(rtRemoteEnvPtr env);
  ~rtRemoteFileResolver();

public:
  virtual rtError open(rtRemoteIAddress const& endpoint_address) override;
  virtual rtError close() override;
  virtual rtError registerObject(std::string const& name, rtRemoteIAddress const& endpoint_address) override;
  virtual rtError locateObject(std::string const& name, rtRemoteIAddress*& endpoint_address,
    uint32_t timeout) override;

private:
  std::string       m_rpc_addr;
  uint16_t          m_rpc_port;
  FILE*             m_db_fp;
  rtRemoteEnvPtr    m_env;
};