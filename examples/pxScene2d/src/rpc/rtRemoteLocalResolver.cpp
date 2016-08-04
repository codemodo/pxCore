#include "rtRemoteLocalResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteFactory.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"
#include "rtRemoteConfig.h"
#include "rtRemoteEndpoint.h"
#include "rtRemoteUtils.h"
#include <rtLog.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>

//TODO
// cleanup resolver method stubs
rtRemoteLocalResolver::rtRemoteLocalResolver(rtRemoteEnvPtr env)
: m_env(env)
, m_endpoint_mapper(nullptr)
{
  // empty
}

rtRemoteLocalResolver::~rtRemoteLocalResolver()
{
  rtError err = this->close();
  if (err != RT_OK)
    rtLogWarn("failed to close resolver: %s", rtStrError(err));
}

rtError
rtRemoteLocalResolver::open()
{
  //try catch TODO
  m_endpoint_mapper = new rtRemoteEndpointMapperFile(m_env);
  return RT_OK;
}

rtError
rtRemoteLocalResolver::registerObject(std::string const& name, rtRemoteEndpointPtr endpoint)
{
  return m_endpoint_mapper->registerEndpoint(name, endpoint);
}

rtError
rtRemoteLocalResolver::deregisterObject(std::string const& name)
{
  return m_endpoint_mapper->deregisterEndpoint(name);
}

rtError
rtRemoteLocalResolver::locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t /*timeout*/)
{
  return m_endpoint_mapper->lookupEndpoint(name, endpoint);
}

rtError
rtRemoteLocalResolver::close()
{
  if (m_endpoint_mapper)
    delete m_endpoint_mapper;
  return RT_OK;
}
