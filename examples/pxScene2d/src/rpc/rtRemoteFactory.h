#include "rtRemoteIResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtRemoteUtils.h"

class rtRemoteFactory
{
public:
  rtRemoteFactory(rtRemoteEnvironment* env);
  ~rtRemoteFactory();
private:
  using AddrCommandHandler = rtError (*)(std::string const&, rtRemoteEndpointPtr&);
  using AddrCommandHandlerMap = std::map< std::string, AddrCommandHandler >;

public:
  rtError createResolver(rtRemoteResolverPtr& resolver);
  rtError createEndpoint(std::string const& uri, rtRemoteEndpointPtr& addr);
  static rtError onCreateEndpointTcp(std::string const& uri, rtRemoteEndpointPtr& addr);
  static rtError onCreateEndpointUdp(std::string const& uri, rtRemoteEndpointPtr& addr);
  //static rtError onCreateEndpointHttp(std::string const& uri, rtRemoteEndpointPtr& addr);
  //static rtError onCreateEndpointFile(std::string const& uri, rtRemoteEndpointPtr& addr);
  static rtError onCreateEndpointShmem(std::string const& uri, rtRemoteEndpointPtr& addr);
  rtError registerFunctionCreateEndpoint(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteEndpointPtr&));
  rtError updateFunctionCreateEndpoint(std::string const& scheme, rtError (*func) (std::string const&, rtRemoteEndpointPtr&));
  
  rtRemoteEnvironment* m_env;
  AddrCommandHandlerMap m_command_handlers;
};
