#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <stdint.h>
#include <netinet/in.h>
#include <rtObject.h>
#include <rapidjson/document.h>

#include "rtRemoteTypes.h"
#include "rtSocketUtils.h"
#include "rtRemoteFileResolver.h"

class rtRemoteNameService
{
public:
  rtRemoteNameService(rtRemoteEnvPtr env);
  ~rtRemoteNameService();

public:
  rtError init();
  rtError close();

private:
  using CommandHandler = rtError (rtRemoteNameService::*)(rtJsonDocPtr const&, sockaddr_storage const&);
  using CommandHandlerMap = std::map< std::string, CommandHandler >;
  using RequestMap = std::map< rtCorrelationKey, rtJsonDocPtr >;
  using RegisteredObjectsMap = std::map< std::string, rtRemoteEndpointPtr>;

  rtError onRegister(rtJsonDocPtr const& doc, sockaddr_storage const& soc);
  rtError onDeregister(rtJsonDocPtr const& doc, sockaddr_storage const& soc);
  rtError onUpdate(rtJsonDocPtr const& doc, sockaddr_storage const& soc);
  rtError onLookup(rtJsonDocPtr const& doc, sockaddr_storage const& soc);

  void runListener();
  void doRead(int fd, rtSocketBuffer& buff);
  void doDispatch(char const* buff, int n, sockaddr_storage* peer);
  // rtError openDbConnection();
  rtError openNsSocket();

private:
  sockaddr_storage  m_ns_endpoint;
  int               m_ns_fd;
  socklen_t         m_ns_len;
  
  pid_t                        m_pid;
  CommandHandlerMap    m_command_handlers;
  RequestMap           m_pending_requests;
  RegisteredObjectsMap m_registered_objects;

  std::mutex                   m_mutex;
  std::unique_ptr<std::thread> m_read_thread;
  int		                       m_shutdown_pipe[2];
  rtRemoteEnvPtr               m_env;
  rtRemoteFileResolver*        m_file_resolver;
};
