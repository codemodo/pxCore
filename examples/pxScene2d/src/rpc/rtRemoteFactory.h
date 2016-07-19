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
  using AddrCommandHandler = rtError (*)(std::string const&, rtRemoteIAddress*&);
  using AddrCommandHandlerMap = std::map< std::string, AddrCommandHandler >;

public:
  rtRemoteIResolver* rtRemoteCreateResolver(rtRemoteEnvironment* env);
  rtError rtRemoteAddressCreate(rtRemoteEnvironment* env, std::string const& uri, rtRemoteIAddress*& addr);
  void registerFunction(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteIAddress *&));
  
  rtRemoteEnvironment* m_env;
  AddrCommandHandlerMap m_command_handlers;
};
