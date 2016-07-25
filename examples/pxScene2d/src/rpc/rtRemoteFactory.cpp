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
rtRemoteFactory::registerFunctionCreateAddress(std::string const& scheme, rtError (rtRemoteFactory::* f) (std::string const&, rtRemoteAddrPtr&))
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
rtRemoteFactory::updateFunctionCreateAddress(std::string const& scheme, rtError (rtRemoteFactory::* f) (std::string const&, rtRemoteAddrPtr&))
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
    case RT_RESOLVER_MULTICAST:
      resolver = new rtRemoteMulticastResolver(m_env);
      break;
    case RT_RESOLVER_FILE:
      resolver = new rtRemoteFileResolver(m_env);
      break;
    case RT_RESOLVER_UNICAST:
      resolver = new rtRemoteUnicastResolver(m_env);
      break;
    default:
      resolver = new rtRemoteMulticastResolver(m_env);
      break;
  }
  return RT_OK;
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
  return CALL_MEMBER_FN(*this, itr->second)(uri, endpoint_addr);
}

rtError
rtRemoteFactory::onCreateAddressTcp(std::string const& uri, rtRemoteAddrPtr& endpoint_addr)
{
  std::string scheme;
  std::string path;
  std::string host;
  uint16_t* port;

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
    endpoint_addr = std::make_shared<rtRemoteLocalAddress>(scheme, path);
  }
  else if (!host.empty() && port != nullptr)
  {
    endpoint_addr = std::make_shared<rtRemoteNetAddress>(scheme, host, *port);   
  }
  return RT_OK;
}

// TODO potentially could modularize since this is virtually identical to tcp
// Other schemes (file, shmem, etc.) will obviously differ in more meaningful ways
rtError
rtRemoteFactory::onCreateAddressUdp(std::string const& uri, rtRemoteAddrPtr& endpoint_addr)
{
  size_t index = uri.find("://");
  if (index == std::string::npos)
  {
   rtLogError("Invalid uri: %s. Expected: <scheme>://<host>[:<port>][<path>]", uri.c_str());
   return RT_FAIL;
  }
  
  // extract scheme
  std::string scheme = uri.substr(0, index);
  // double check that correct create function is being used
  char const* s = scheme.c_str();
  if (s != nullptr)
  {
    if (strcasecmp(s, "udp") != 0)
    {
      rtLogError("Cannot create tcp addr from uri: %s", uri.c_str());
      return RT_FAIL;
    }
  }
  // make lowercase for consistency
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);

  // We either have a path or host now.  Let's pull the remaining info.
  index += 3;
  char ch = uri.at(index);
  if (ch == '/' || ch == '.')
  { // local socket
    std::string path = uri.substr(index, std::string::npos);
    endpoint_addr = std::make_shared<rtRemoteLocalAddress>(scheme, path);
  }
  else
  { // network socket
    // get port
    std::string port_string;
    size_t index_port = uri.find_last_of(":");
    if (index_port == std::string::npos // no port. no colon found
      || uri.at(index_port-1) == ':' // no port. colon was part of ipv6 addr
      || index_port == index-3) // no port.  last colon equals colon in ://
    {
      rtLogWarn("No port included included in URI: %s. Defaulting to 0", uri.c_str());
      port_string = "0";
      index_port = std::string::npos; // set this for host extraction below
    }
    else
    {
      port_string = uri.substr(index_port+1, std::string::npos);
    }
    int port = stoi(port_string);
    
    // get host
    std::string host;
    host = uri.substr(index, index_port - index);

    endpoint_addr = std::make_shared<rtRemoteNetAddress>(scheme, host, port);   
  }
  return RT_OK;
}


