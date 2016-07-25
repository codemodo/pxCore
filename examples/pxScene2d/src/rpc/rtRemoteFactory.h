#include "rtRemoteIResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtRemoteUtils.h"
enum rtResolverType
{
  RT_RESOLVER_MULTICAST,
  RT_RESOLVER_FILE,
  RT_RESOLVER_UNICAST
};

class rtRemoteFactory
{
public:
  rtRemoteFactory(rtRemoteEnvironment* env);
  ~rtRemoteFactory();
private:
  using AddrCommandHandler = rtError (*)(std::string const&, rtRemoteAddrPtr&);
  using AddrCommandHandlerMap = std::map< std::string, AddrCommandHandler >;

public:
  rtRemoteIResolver* createResolver(rtRemoteEnvironment* env);
  rtError createAddress(std::string const& uri, rtRemoteAddrPtr& addr);
  rtError registerFunctionCreateAddress(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteAddrPtr&));
  rtError updateFunctionCreateAddress(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteAddrPtr&));
  
  rtRemoteEnvironment* m_env;
  AddrCommandHandlerMap m_command_handlers;
};
