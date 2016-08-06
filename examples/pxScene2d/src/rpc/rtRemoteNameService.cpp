#include "rtRemoteNameService.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"
#include "rtRemoteConfig.h"
#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtRemoteUtils.h"
#include "rtRemoteMapper.h"
#include "rtRemoteMapperFile.h"

#include <condition_variable>
#include <thread>
#include <mutex>
#include <memory>

#include <rtLog.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/pointer.h>

rtRemoteNameService::rtRemoteNameService(rtRemoteEnvPtr env)
  : m_sock_fd(-1)
  , m_sock_len(0)
  , m_pid(getpid())
  , m_command_handlers()
  , m_env(env)
  , m_endpoint_mapper(nullptr)
{
  memset(&m_sock_addr, 0, sizeof(m_sock_addr));

  m_command_handlers.insert(CommandHandlerMap::value_type(kNsMessageTypeRegister, &rtRemoteNameService::onRegister));
  m_command_handlers.insert(CommandHandlerMap::value_type(kNsMessageTypeDeregister, &rtRemoteNameService::onDeregister));
  m_command_handlers.insert(CommandHandlerMap::value_type(kNsMessageTypeUpdate, &rtRemoteNameService::onUpdate));
  m_command_handlers.insert(CommandHandlerMap::value_type(kNsMessageTypeLookup, &rtRemoteNameService::onLookup));

  m_shutdown_pipe[0] = -1;
  m_shutdown_pipe[1] = -1;

  int ret = pipe2(m_shutdown_pipe, O_CLOEXEC);
  if (ret == -1)
  {
    rtError e = rtErrorFromErrno(ret);
    rtLogWarn("failed to create shutdown pipe. %s", rtStrError(e));
  }
}

rtRemoteNameService::~rtRemoteNameService()
{
  rtError err = this->close();
  if (err != RT_OK)
    rtLogWarn("failed to close resolver: %s", rtStrError(err));
}

rtError
rtRemoteNameService::init()
{
  rtError err = RT_OK;

  m_endpoint_mapper = new rtRemoteMapperFile(m_env);

  // get socket info ready
  uint16_t const nsport = m_env->Config->resolver_unicast_port();
  std::string nsaddr = m_env->Config->resolver_unicast_address();
  err = rtParseAddress(m_sock_addr, nsaddr.c_str(), nsport, nullptr);
  if (err != RT_OK)
  {
    err = rtGetDefaultInterface(m_sock_addr, 0);
    if (err != RT_OK)
      return err;
  }

  // open unicast socket
  err = openSocket();
  if (err != RT_OK)
  {
      rtLogWarn("failed to open name service unicast socket. %s", rtStrError(err));
      return err;
  }

  m_read_thread.reset(new std::thread(&rtRemoteNameService::runListener, this));
  return err;
}

rtError
rtRemoteNameService::close()
{
  if (m_endpoint_mapper)
    delete m_endpoint_mapper;

  if (m_shutdown_pipe[1] != -1)
  {
    char buff[] = {"shutdown"};
    ssize_t n = write(m_shutdown_pipe[1], buff, sizeof(buff));
    if (n == -1)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogWarn("failed to write. %s", rtStrError(e));
    }

    if (m_read_thread)
    {
      m_read_thread->join();
      m_read_thread.reset();
    }

    if (m_shutdown_pipe[0] != -1)
      ::close(m_shutdown_pipe[0]);
    if (m_shutdown_pipe[1] != -1)
      ::close(m_shutdown_pipe[1]);
  }

  if (m_sock_fd != -1)
    ::close(m_sock_fd);

  m_sock_fd = -1;
  m_shutdown_pipe[0] = -1;
  m_shutdown_pipe[1] = -1;

  return RT_OK;
}

rtError
rtRemoteNameService::openSocket()
{
  int ret = 0;

  m_sock_fd = socket(m_sock_addr.ss_family, SOCK_DGRAM, 0);
  if (m_sock_fd < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to create unicast socket with family: %d. %s", 
      m_sock_addr.ss_family, rtStrError(e));
    return e;
  }
  fcntl(m_sock_fd, F_SETFD, fcntl(m_sock_fd, F_GETFD) | FD_CLOEXEC);

  rtSocketGetLength(m_sock_addr, &m_sock_len);

  // listen on ANY port
  ret = bind(m_sock_fd, reinterpret_cast<sockaddr *>(&m_sock_addr), m_sock_len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to bind unicast endpoint: %s", rtStrError(e));
    return e;
  }

  // now figure out which port we're bound to
  rtSocketGetLength(m_sock_addr, &m_sock_len);
  ret = getsockname(m_sock_fd, reinterpret_cast<sockaddr *>(&m_sock_addr), &m_sock_len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to get socketname. %s", rtStrError(e));
    return e;
  }
  else
  {
    rtLogInfo("local udp socket bound to %s", rtSocketToString(m_sock_addr).c_str());
  }

  return RT_OK;
}

/**
 * Callback for registering objects and associated Well-known Endpoints
 */
rtError
rtRemoteNameService::onRegister(rtJsonDocPtr const& doc, sockaddr_storage const& /*soc*/)
{
  rtRemoteEndpointPtr objectEndpoint;
  rtRemoteDocumentToEndpoint(doc, objectEndpoint);
  char const* objectId = rtMessage_GetObjectId(*doc);
  
  return m_endpoint_mapper->registerEndpoint(objectId, objectEndpoint);
}

/**
 * Callback for deregistering objects
 */
rtError
rtRemoteNameService::onDeregister(rtJsonDocPtr const& doc, sockaddr_storage const& /*soc*/)
{
  char const* objectId = rtMessage_GetObjectId(*doc);

  return m_endpoint_mapper->deregisterEndpoint(objectId);
}

rtError
rtRemoteNameService::onUpdate(rtJsonDocPtr const& /*doc*/, sockaddr_storage const& /*soc*/)
{
  // TODO
  return RT_OK;
}

rtError
rtRemoteNameService::onLookup(rtJsonDocPtr const& doc, sockaddr_storage const& soc)
{
  auto senderId = doc->FindMember(kFieldNameSenderId);
  RT_ASSERT(senderId != doc->MemberEnd());
  if (senderId->value.GetInt() == m_pid)
    return RT_OK;

  int key = rtMessage_GetCorrelationKey(*doc);
  rtCorrelationKey seqId = rtMessage_GetNextCorrelationKey();

  char const* objectId = rtMessage_GetObjectId(*doc);
  rtRemoteEndpointPtr objectEndpoint;
  m_endpoint_mapper->lookupEndpoint(objectId, objectEndpoint);

  if (objectEndpoint)
  { // object is registered

    // create and send response
    rtJsonDocPtr doc(new rapidjson::Document());
    doc->SetObject();
    doc->AddMember(kFieldNameMessageType, kNsMessageTypeLookupResponse, doc->GetAllocator());
    doc->AddMember(kFieldNameStatusMessage, kNsStatusSuccess, doc->GetAllocator());
    doc->AddMember(kFieldNameObjectId, std::string(objectId), doc->GetAllocator());
    doc->AddMember(kFieldNameSenderId, m_pid, doc->GetAllocator());
    doc->AddMember(kFieldNameCorrelationKey, seqId, doc->GetAllocator());
    
    rtJsonDocPtr endpoint_doc(new rapidjson::Document());
    endpoint_doc->SetObject();
    rtRemoteEndpointToDocument(objectEndpoint, endpoint_doc);
    rtRemoteCombineDocuments(doc, endpoint_doc);

    rtLogInfo("\n\nNAMESERVICE: reporting location as %s\n\n", objectEndpoint->toUriString().c_str());

    return rtSendDocument(*doc, m_sock_fd, &soc);
  }
  return RT_OK;
}

void
rtRemoteNameService::runListener()
{
  rtSocketBuffer buff;
  buff.reserve(1024 * 1024);
  buff.resize(1024 * 1024);

  while (true)
  {
    int maxFd = 0;

    fd_set read_fds;
    fd_set err_fds;

    FD_ZERO(&read_fds);
    rtPushFd(&read_fds, m_sock_fd, &maxFd);
    rtPushFd(&read_fds, m_shutdown_pipe[0], &maxFd);

    FD_ZERO(&err_fds);
    rtPushFd(&err_fds, m_sock_fd, &maxFd);

    int ret = select(maxFd + 1, &read_fds, NULL, &err_fds, NULL);
    if (ret == -1)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogWarn("select failed: %s", rtStrError(e));
      continue;
    }

    if (FD_ISSET(m_shutdown_pipe[0], &read_fds))
    {
      rtLogInfo("got shutdown signal");
      return;
    }

    if (FD_ISSET(m_sock_fd, &read_fds))
      doRead(m_sock_fd, buff);
  }
}

void
rtRemoteNameService::doRead(int fd, rtSocketBuffer& buff)
{
  // we only suppor v4 right now. not sure how recvfrom supports v6 and v4
  sockaddr_storage src;
  socklen_t len = sizeof(sockaddr_in);

  #if 0
  ssize_t n = read(m_mcast_fd, &m_read_buff[0], m_read_buff.capacity());
  #endif

  ssize_t n = recvfrom(fd, &buff[0], buff.capacity(), 0, reinterpret_cast<sockaddr *>(&src), &len);
  if (n > 0)
    doDispatch(&buff[0], static_cast<int>(n), &src);
}

void
rtRemoteNameService::doDispatch(char const* buff, int n, sockaddr_storage* peer)
{
  // rtLogInfo("new message from %s:%d", inet_ntoa(src.sin_addr), htons(src.sin_port));
  // printf("read: %d\n", int(n));
  #ifdef RT_RPC_DEBUG
  rtLogDebug("read:\n***IN***\t\"%.*s\"\n", n, buff); // static_cast<int>(m_read_buff.size()), &m_read_buff[0]);
  #endif

  rtJsonDocPtr doc;
  rtError err = rtParseMessage(buff, n, doc);
  if (err != RT_OK)
    return;

  char const* message_type = rtMessage_GetMessageType(*doc);

  auto itr = m_command_handlers.find(message_type);
  if (itr == m_command_handlers.end())
  {
    rtLogWarn("no command handler registered for: %s", message_type);
    return;
  }

  err = CALL_MEMBER_FN(*this, itr->second)(doc, *peer);
  if (err != RT_OK)
  {
    rtLogWarn("failed to run command for %s. %d", message_type, err);
    return;
  }
}
