#include "rtRemoteFactory.h"
#include "rtRemoteConfig.h"
#include "rtRemoteIResolver.h"
#include "rtRemoteFileResolver.h"
#include "rtRemoteMulticastResolver.h"
#include "rtRemoteUnicastResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteUtils.h"
#include <algorithm>

static rtResolverType
rtResolverTypeFromString(std::string const& resolverType)
{
  char const* s = resolverType.c_str();

  rtResolverType t = rtResolverType::MULTICAST;
  if (s != nullptr)
  {
    if (strcasecmp(s, "multicast") == 0)
      t = rtResolverType::MULTICAST;
    else if (strcasecmp(s, "file") == 0)
      t = rtResolverType::FILE;
    else if (strcasecmp(s, "unicast") == 0)
      t = rtResolverType::UNICAST;
    else
      RT_ASSERT(false);
  }
  return t;
};

rtRemoteFactory::rtRemoteFactory(rtRemoteEnvironment* env)
: m_env(env)
, m_command_handlers()
{
  registerFunctionCreateEndpoint("tcp", &rtRemoteFactory::onCreateEndpointTcp);
  registerFunctionCreateEndpoint("udp", &rtRemoteFactory::onCreateEndpointUdp);
  registerFunctionCreateEndpoint("shmem", &rtRemoteFactory::onCreateEndpointShmem);
}

rtRemoteFactory::~rtRemoteFactory()
{
  // empty
}

rtError
rtRemoteFactory::registerFunctionCreateEndpoint(std::string const& scheme, rtError (*f) (std::string const&, rtRemoteEndpointPtr&))
{
  if (m_command_handlers.find(scheme) != m_command_handlers.end())
  {
    rtLogWarn("function for creating %s addresses already registered - use update instead", scheme.c_str());
    return RT_FAIL;
  }
  m_command_handlers.insert(AddrCommandHandlerMap::value_type(scheme, f));
  return RT_OK;
}

rtError
rtRemoteFactory::updateFunctionCreateEndpoint(std::string const& scheme, rtError (*f) (std::string const&, rtRemoteEndpointPtr&))
{
  m_command_handlers.insert(AddrCommandHandlerMap::value_type(scheme, f));
  return RT_OK;
}

rtError
rtRemoteFactory::createResolver(rtRemoteResolverPtr& resolver)
{
  rtResolverType t = rtResolverTypeFromString(m_env->Config->resolver_type());

  switch (t)
  {
    case rtResolverType::MULTICAST:
      resolver = new rtRemoteMulticastResolver(m_env);
      break;
    case rtResolverType::FILE:
      resolver = new rtRemoteFileResolver(m_env);
      break;
    case rtResolverType::UNICAST:
      resolver = new rtRemoteUnicastResolver(m_env);
      break;
    default:
      resolver = new rtRemoteMulticastResolver(m_env);
      break;
  }
  return RT_OK;
}

rtError
rtRemoteFactory::createEndpoint(std::string const& uri, rtRemoteEndpointPtr& endpoint)
{
  std::string scheme = uri.substr(0, uri.find(":"));
  
  auto itr = m_command_handlers.find(scheme);
  if (itr == m_command_handlers.end())
  {
    rtLogError("no command handler registered for: %s", scheme.c_str());
    return RT_FAIL;
  }
  return itr->second(uri, endpoint);
}

rtError
rtRemoteFactory::onCreateEndpointTcp(std::string const& uri, rtRemoteEndpointPtr& endpoint)
{
  std::string scheme;
  std::string path;
  std::string host;
  uint16_t* port = nullptr;

  rtError e;
  e = rtRemoteParseUri(uri, scheme, path, host, port);
  if (e != RT_OK)
    return e;
  
  RT_ASSERT(!scheme.empty());
  
  // double check that correct create function is being used
  char const* s = scheme.c_str();
  if (s != nullptr)
  {
    if (strcasecmp(s, "tcp") != 0)
    {
      rtLogError("failed to create endpoint addr from uri: %s.  invalid scheme", uri.c_str());
      return RT_FAIL;
    }
  }
  // make lowercase for consistency
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

  if (!path.empty())
  { // local socket
    endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, path);
  }
  else if (!host.empty() && port != nullptr)
  {
    endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme, host, *port);   
  }
  return RT_OK;
}

rtError
rtRemoteFactory::onCreateEndpointUdp(std::string const& uri, rtRemoteEndpointPtr& endpoint)
{
  std::string scheme;
  std::string path;
  std::string host;
  uint16_t* port = nullptr;

  rtError e;
  e = rtRemoteParseUri(uri, scheme, path, host, port);
  if (e != RT_OK)
    return e;
  
  RT_ASSERT(!scheme.empty());
  
  // double check that correct create function is being used
  char const* s = scheme.c_str();
  if (s != nullptr)
  {
    if (strcasecmp(s, "udp") != 0)
    {
      rtLogError("failed to create endpoint addr from uri: %s.  invalid scheme", uri.c_str());
      return RT_FAIL;
    }
  }
  // make lowercase for consistency
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

  if (!path.empty())
  { // local socket
    endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, path);
  }
  else if (!host.empty() && port != nullptr)
  {
    endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme, host, *port);   
  }
  return RT_OK;
}

rtError
rtRemoteFactory::onCreateEndpointShmem(std::string const& uri, rtRemoteEndpointPtr& endpoint)
{
  std::string scheme;
  std::string path;
  std::string host;
  uint16_t* port = nullptr;

  rtError e;
  e = rtRemoteParseUri(uri, scheme, path, host, port);
  if (e != RT_OK)
    return e;
  
  RT_ASSERT(!scheme.empty());
  RT_ASSERT(!path.empty());
  RT_ASSERT(host.empty());
  RT_ASSERT(port == nullptr);

  // double check that correct create function is being used
  char const* s = scheme.c_str();
  if (s != nullptr)
  {
    if (strcasecmp(s, "shmem") != 0)
    {
      rtLogError("failed to shmem endpoint from uri: %s.  invalid scheme", uri.c_str());
      return RT_FAIL;
    }
  }
  // make lowercase for consistency
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

  endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, path);

  return RT_OK;
}



