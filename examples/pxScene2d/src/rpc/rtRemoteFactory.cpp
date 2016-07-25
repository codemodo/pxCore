#include "rtRemoteFactory.h"
#include "rtRemoteConfig.h"
#include "rtRemoteIResolver.h"
#include "rtRemoteFileResolver.h"
#include "rtRemoteMulticastResolver.h"
#include "rtRemoteNsResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteUtils.h"

static rtResolverType
rtResolverTypeFromString(std::string const& resolverType)
{
  char const* s = resolverType.c_str();

  rtResolverType t = RT_RESOLVER_MULTICAST;
  if (s != nullptr)
  {
    if (strcasecmp(s, "multicast") == 0)
      t = RT_RESOLVER_MULTICAST;
    else if (strcasecmp(s, "file") == 0)
      t = RT_RESOLVER_FILE;
    else if (strcasecmp(s, "unicast") == 0)
      t = RT_RESOLVER_UNICAST;
    else
      RT_ASSERT(false);
  }
  return t;
};


rtRemoteFactory::rtRemoteFactory(rtRemoteEnvironment* env)
: m_env(env)
, m_command_handlers()
{
  // empty
  // The thought was that here we'd read in any functions specified in the config
  // and insert them into the command handlers
}

rtRemoteFactory::~rtRemoteFactory()
{
  // empty
}

rtError
rtRemoteFactory::registerFunctionCreateAddress(std::string const& scheme, rtError (*f) (std::string const&, rtRemoteAddrPtr&))
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
rtRemoteFactory::updateFunctionCreateAddress(std::string const& scheme, rtError (*f) (std::string const&, rtRemoteAddrPtr&))
{
  m_command_handlers.insert(AddrCommandHandlerMap::value_type(scheme, f));
  return RT_OK;
}

rtRemoteIResolver*
rtRemoteFactory::createResolver(rtRemoteEnvironment* env)
{
  rtRemoteIResolver* resolver = nullptr;
  rtResolverType t = rtResolverTypeFromString(env->Config->resolver_type());

  switch (t)
  {
    case RT_RESOLVER_MULTICAST:
      resolver = new rtRemoteMulticastResolver(env);
      break;
    case RT_RESOLVER_FILE:
      resolver = new rtRemoteFileResolver(env);
      break;
    case RT_RESOLVER_UNICAST:
      resolver = new rtRemoteNsResolver(env);
      break;
    default:
      resolver = new rtRemoteMulticastResolver(env);
      break;
  }
  return resolver;
}

rtError
rtRemoteFactory::createAddress(std::string const& uri, rtRemoteAddrPtr& endpoint_addr)
{
  std::string scheme = uri.substr(0, uri.find(":"));
  
  auto itr = m_command_handlers.find(scheme);
  if (itr == m_command_handlers.end())
  {
    rtLogError("no command handler registered for: %s", scheme.c_str());
    return RT_FAIL;
  }
  
  #define CALL_MEMBER_FN(ptrToMember)  (*(ptrToMember))
  return CALL_MEMBER_FN(itr->second)(uri, endpoint_addr);
}



