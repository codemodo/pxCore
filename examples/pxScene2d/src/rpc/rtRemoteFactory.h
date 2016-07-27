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
  rtError createResolver(rtRemoteResolverPtr& resolver);
  rtError createAddress(std::string const& uri, rtRemoteAddrPtr& addr);
  static rtError onCreateAddressTcp(std::string const& uri, rtRemoteAddrPtr& addr);
  static rtError onCreateAddressUdp(std::string const& uri, rtRemoteAddrPtr& addr);
  //static rtError onCreateAddressHttp(std::string const& uri, rtRemoteAddrPtr& addr);
  //static rtError onCreateAddressFile(std::string const& uri, rtRemoteAddrPtr& addr);
  //static rtError onCreateAddressShmem(std::string const& uri, rtRemoteAddrPtr& addr);
  rtError registerFunctionCreateAddress(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteAddrPtr&));
  rtError updateFunctionCreateAddress(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteAddrPtr&));
  
  rtRemoteEnvironment* m_env;
  AddrCommandHandlerMap m_command_handlers;
};
