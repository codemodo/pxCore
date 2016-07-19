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
}

rtRemoteFactory::~rtRemoteFactory()
{
  //TODO check this
  delete m_env;
}

void
rtRemoteFactory::registerFunction(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteIAddress *&))
{
  m_command_handlers.insert(AddrCommandHandlerMap::value_type(scheme, func));
}

rtRemoteIResolver*
rtRemoteFactory::rtRemoteCreateResolver(rtRemoteEnvironment* env)
{
  rtRemoteIResolver* resolver = nullptr;
  rtResolverType t = rtResolverTypeFromString(env->Config->resolver_type());
  
  t = RT_RESOLVER_FILE;
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
rtRemoteFactory::rtRemoteAddressCreate(rtRemoteEnvironment* env, std::string const& uri, rtRemoteIAddress*& endpoint_addr)
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



