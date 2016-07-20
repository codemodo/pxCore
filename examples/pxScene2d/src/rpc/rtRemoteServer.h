#ifndef __RT_REMOTE_OBJECT_LOCATOR_H__
#define __RT_REMOTE_OBJECT_LOCATOR_H__

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
#include "rtRemoteIResolver.h"
#include "rtSocketUtils.h"
#include "rtRemoteEndpoint.h"

class rtRemoteClient;

class rtRemoteServer
{
public:
  rtRemoteServer(rtRemoteEnvironment* env);
  ~rtRemoteServer();

public:
  rtError open();
  rtError registerObject(std::string const& name, rtObjectRef const& obj);
  rtError findObject(std::string const& name, rtObjectRef& obj, uint32_t timeout);
  rtError removeStaleObjects();
  rtError processMessage(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& msg);

private:
  struct connected_client
  {
    sockaddr_storage    peer;
    int                 fd;
  };

  void runListener();
  void doAccept(int fd);

  rtError onIncomingMessage(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& msg);

  // command handlers
  rtError start();
  rtError onOpenSession(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& doc);
  rtError onGet(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& doc);
  rtError onSet(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& doc);
  rtError onMethodCall(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& doc);
  rtError onKeepAlive(std::shared_ptr<rtRemoteClient>& client, rtJsonDocPtr const& doc);
  rtError openRpcListener();

private:
  struct ObjectReference
  {
    rtObjectRef       object;
    std::vector<int>  client_fds;
    time_t            last_used;
    bool              owner_removed;
  };

  using ClientMap = std::map< std::string, std::shared_ptr<rtRemoteClient> >;
  using ClientList = std::vector< std::shared_ptr<rtRemoteClient > >;
  using CommandHandlerMap = std::map< std::string, rtRemoteMessageHandler >;
  using ObjectRefeMap = std::map< std::string, ObjectReference >;

  rtRemoteIAddress*             m_rpc_endpoint;
  rtRemoteStreamServerEndpoint* m_srv_endpoint;

  std::unique_ptr<std::thread>  m_thread;
  mutable std::mutex            m_mutex;
  CommandHandlerMap             m_command_handlers;

  rtRemoteIResolver*            m_resolver;
  ClientMap                     m_object_map;
  ClientList                    m_connected_clients;
  int                           m_shutdown_pipe[2];
  uint32_t                      m_keep_alive_interval;
  rtRemoteEnvironment*          m_env;
  bool                          m_queueing_mode;
};

#endif
