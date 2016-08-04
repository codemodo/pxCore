#include "rtRemoteResolverLocal.h"
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
rtRemoteResolverLocal::rtRemoteResolverLocal(rtRemoteEnvPtr env)
: m_env(env)
, m_endpoint_mapper(nullptr)
{
  // empty
}

rtRemoteResolverLocal::~rtRemoteResolverLocal()
{
  rtError err = this->close();
  if (err != RT_OK)
    rtLogWarn("failed to close resolver: %s", rtStrError(err));
}

rtError
rtRemoteResolverLocal::open()
{
  //try catch TODO
  m_endpoint_mapper = new rtRemoteEndpointMapperFile(m_env);
  return RT_OK;
}

rtError
rtRemoteResolverLocal::registerObject(std::string const& name, rtRemoteEndpointPtr endpoint)
{
  return m_endpoint_mapper->registerEndpoint(name, endpoint);
}

rtError
rtRemoteResolverLocal::deregisterObject(std::string const& name)
{
  return m_endpoint_mapper->deregisterEndpoint(name);
}

rtError
rtRemoteResolverLocal::locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t /*timeout*/)
{
  return m_endpoint_mapper->lookupEndpoint(name, endpoint);
}

rtError
rtRemoteResolverLocal::close()
{
  if (m_endpoint_mapper)
    delete m_endpoint_mapper;
  return RT_OK;
}
