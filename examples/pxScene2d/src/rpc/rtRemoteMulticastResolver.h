#include "rtRemoteIResolver.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <memory>

#include <stdint.h>
#include <netinet/in.h>
#include <rtObject.h>
#include <rapidjson/document.h>

#include "rtRemoteTypes.h"
#include "rtSocketUtils.h"
#include "rtRemoteEndpointMapper.h"


class rtRemoteMulticastResolver : public rtRemoteIResolver
{
public:
  rtRemoteMulticastResolver(rtRemoteEnvPtr env);
  ~rtRemoteMulticastResolver();

public:
  virtual rtError open() override;
  virtual rtError close() override;
  virtual rtError registerObject(std::string const& name, rtRemoteEndpointPtr endpoint) override;
  virtual rtError deregisterObject(std::string const& name) override;
  virtual rtError locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t timeout) override;

private:
  using CommandHandler = rtError (rtRemoteMulticastResolver::*)(rtJsonDocPtr const&, sockaddr_storage const&);
  using HostedObjectsMap = std::map< std::string, rtRemoteEndpointPtr >;
  using CommandHandlerMap = std::map< std::string, CommandHandler >;
  using RequestMap = std::map< rtCorrelationKey, rtJsonDocPtr >;

  void runListener();
  void doRead(int fd, rtSocketBuffer& buff);
  void doDispatch(char const* buff, int n, sockaddr_storage* peer);

  rtError init();
  rtError openUnicastSocket();
  rtError openMulticastSocket();

  // command handlers
  rtError onSearch(rtJsonDocPtr const& doc, sockaddr_storage const& soc);
  rtError onLocate(rtJsonDocPtr const& doc, sockaddr_storage const& soc);

private:
  sockaddr_storage  m_mcast_dest;
  sockaddr_storage  m_mcast_src;
  int               m_mcast_fd;
  uint32_t          m_mcast_src_index;

  sockaddr_storage  m_ucast_endpoint;
  int               m_ucast_fd;
  socklen_t         m_ucast_len;

  std::unique_ptr<std::thread> m_read_thread;
  std::condition_variable m_cond;
  std::mutex        m_mutex;
  pid_t             m_pid;
  CommandHandlerMap m_command_handlers;
  HostedObjectsMap  m_hosted_objects;
  RequestMap	      m_pending_searches;
  int		            m_shutdown_pipe[2];
  rtRemoteEnvPtr    m_env;

  rtRemoteEndpointMapperSimple m_endpoint_mapper;
};