#include "rtRemoteUnicastResolver.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"
#include "rtRemoteConfig.h"
#include "rtRemoteTypes.h"
#include "rtRemoteUtils.h"
#include "rtRemoteEndpoint.h"

#include <memory>
#include <condition_variable>
#include <thread>
#include <mutex>

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
#include <unistd.h>

#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/pointer.h>

rtRemoteUnicastResolver::rtRemoteUnicastResolver(rtRemoteEnvPtr env)
  : m_static_fd(-1)
  , m_pid(getpid())
  , m_command_handlers()
  , m_env(env)
{
  memset(&m_static_endpoint, 0, sizeof(m_static_endpoint));

  m_command_handlers.insert(CommandHandlerMap::value_type(kNsMessageTypeLookupResponse, &rtRemoteUnicastResolver::onLocate));

  m_shutdown_pipe[0] = -1; m_shutdown_pipe[1] = -1;
  
  int ret = pipe2(m_shutdown_pipe, O_CLOEXEC);
  if (ret == -1)
  {
    rtError e = rtErrorFromErrno(ret);
    rtLogWarn("failed to create shutdown pipe. %s", rtStrError(e));
  }
}

rtRemoteUnicastResolver::~rtRemoteUnicastResolver()
{
  rtError err = this->close();
  if (err != RT_OK)
    rtLogWarn("failed to close resolver: %s", rtStrError(err));    
}

rtError
rtRemoteUnicastResolver::open()
{
  rtError err = init();
  if (err != RT_OK)
  {
    rtLogWarn("failed to initialize resolver. %s", rtStrError(err));
    return err;
  }

  err = openSocket();
  if (err != RT_OK)
  {
    rtLogWarn("failed to open unicast socket. %s", rtStrError(err));
    return err;
  }

  m_read_thread.reset(new std::thread(&rtRemoteUnicastResolver::runListener, this));
  return RT_OK;
}

rtError
rtRemoteUnicastResolver::registerObject(std::string const& name, rtRemoteEndpointPtr endpoint)
{
    return registerObject(name, endpoint, 3000);
}

rtError
rtRemoteUnicastResolver::registerObject(std::string const& name, rtRemoteEndpointPtr endpoint, uint32_t timeout)
{
  if (m_static_fd == -1)
  {
    rtLogError("unicast socket not opened");
    return RT_FAIL;
  }

  sockaddr_storage endpoint_sockaddr;
  rtRemoteEndpointAddressToSocket(endpoint, endpoint_sockaddr);

  std::string rpc_addr;
  uint16_t rpc_port;
  char buff[128];
  void* addr = nullptr;
  rtGetInetAddr(endpoint_sockaddr, &addr);
  
  socklen_t len;
  rtSocketGetLength(endpoint_sockaddr, &len);
  char const* p = inet_ntop(endpoint_sockaddr.ss_family, addr, buff, len);
  if (p)
    rpc_addr = p;
  rtGetPort(endpoint_sockaddr, &rpc_port);

  rtError err = RT_OK;
  rtCorrelationKey seqId = rtMessage_GetNextCorrelationKey();

  rapidjson::Document doc;
  doc.SetObject();
  doc.AddMember(kFieldNameMessageType, kNsMessageTypeRegister, doc.GetAllocator());
  doc.AddMember(kFieldNameObjectId, name, doc.GetAllocator());
  doc.AddMember(kFieldNameIp, rpc_addr, doc.GetAllocator());
  doc.AddMember(kFieldNamePort, rpc_port, doc.GetAllocator());
  doc.AddMember(kFieldNameSenderId, m_pid, doc.GetAllocator());
  doc.AddMember(kFieldNameCorrelationKey, seqId, doc.GetAllocator());

  err = rtSendDocument(doc, m_static_fd, &m_ns_dest);
  if (err != RT_OK)
    return err;

  rtJsonDocPtr searchResponse;
  RequestMap::const_iterator itr;

  auto delay = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

  // wait here until timeout expires or we get a response that matches out pid/seqid
  std::unique_lock<std::mutex> lock(m_mutex);
  itr = m_pending_searches.end();
  m_cond.wait_until(lock, delay, [this, seqId, &searchResponse]
    {
      auto itr = this->m_pending_searches.find(seqId);
      if (itr != this->m_pending_searches.end())
      {
        searchResponse = itr->second;
        this->m_pending_searches.erase(itr);
      }
      return searchResponse != nullptr;
    });
  lock.unlock();

  if (!searchResponse)
    return RT_FAIL;

  // response is in itr
  if (searchResponse)
  {
    char const* message_type = rtMessage_GetMessageType(*searchResponse);
    
    if (strcmp(message_type, kNsMessageTypeRegisterResponse) == 0)
    {    
      if (strcmp(rtMessage_GetStatusMessage(*searchResponse), kNsStatusFail) == 0)
      {
        rtLogWarn("ns register failed");
        return RT_FAIL;
      }
      else
      {
        return RT_OK;
      }
    }
    else
    {
      rtLogWarn("unexpected response to lookup request. %s", message_type);
      return RT_FAIL;
    }
  }

  return RT_OK;
}

rtError
rtRemoteUnicastResolver::locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t timeout)
{
  if (m_static_fd == -1)
  {
    rtLogError("unicast socket not opened");
    return RT_FAIL;
  }

  rtError err = RT_OK;
  rtCorrelationKey seqId = rtMessage_GetNextCorrelationKey();

  rapidjson::Document doc;
  doc.SetObject();
  doc.AddMember(kFieldNameMessageType, kNsMessageTypeLookup, doc.GetAllocator());
  doc.AddMember(kFieldNameObjectId, name, doc.GetAllocator());
  doc.AddMember(kFieldNameSenderId, m_pid, doc.GetAllocator());
  doc.AddMember(kFieldNameCorrelationKey, seqId, doc.GetAllocator());

  err = rtSendDocument(doc, m_static_fd, &m_ns_dest);
  if (err != RT_OK)
    return err;

  rtJsonDocPtr searchResponse;
  RequestMap::const_iterator itr;

  auto delay = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

  // wait here until timeout expires or we get a response that matches out pid/seqid
  std::unique_lock<std::mutex> lock(m_mutex);
  itr = m_pending_searches.end();
  
  m_cond.wait_until(lock, delay, [this, seqId, &searchResponse]
    {
      auto itr = this->m_pending_searches.find(seqId);
      if (itr != this->m_pending_searches.end())
      {
        searchResponse = itr->second;
        this->m_pending_searches.erase(itr);
      }
      return searchResponse != nullptr;
    });
  lock.unlock();

  if (!searchResponse)
  {
    rtLogInfo("no search response");
    return RT_FAIL;
  }

  // response is in itr
  if (searchResponse)
  {
    char const* message_type = rtMessage_GetMessageType(*searchResponse);
    
    if (strcmp(message_type, kNsMessageTypeLookupResponse) == 0)
    {    
      if (strcmp(rtMessage_GetStatusMessage(*searchResponse), kNsStatusFail) == 0)
      {
        rtLogWarn("ns register failed");
        return RT_FAIL;
      }
      else
      {
        RT_ASSERT(searchResponse->HasMember(kFieldNameScheme));
        RT_ASSERT(searchResponse->HasMember(kFieldNameEndpointType));
        if (std::string((*searchResponse)[kFieldNameEndpointType].GetString()).compare(kEndpointTypeLocal) == 0)
        {
          RT_ASSERT(searchResponse->HasMember(kFieldNamePath));
          std::string scheme, path;
          scheme = (*searchResponse)[kFieldNameScheme].GetString();
          path = (*searchResponse)[kFieldNamePath].GetString();
          endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme, path);
        }
        else
        {
          RT_ASSERT(searchResponse->HasMember(kFieldNameIp));
          RT_ASSERT(searchResponse->HasMember(kFieldNamePort));
          std::string scheme, host;
          int port;
          scheme = (*searchResponse)[kFieldNameScheme].GetString();
          host = (*searchResponse)[kFieldNameIp].GetString();
          port = (*searchResponse)[kFieldNamePort].GetInt();
          endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme, host, port);
        }
      }
    }
    else
    {
      rtLogWarn("unexpected response to lookup request. %s", message_type);
      return RT_FAIL;
    }
  }

  return RT_OK;
}

rtError
rtRemoteUnicastResolver::close()
{
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

  if (m_static_fd != -1)
    ::close(m_static_fd);

  m_static_fd = -1;
  m_shutdown_pipe[0] = -1;
  m_shutdown_pipe[1] = -1;

  return RT_OK;    
}

rtError
rtRemoteUnicastResolver::init()
{
  rtError err = RT_OK;
  uint16_t dstport = m_env->Config->resolver_unicast_port();
  std::string dstaddr = m_env->Config->resolver_unicast_address();

  rtLogInfo("dest address is %s", dstaddr.c_str());

  err = rtParseAddress(m_ns_dest, dstaddr.c_str(), dstport, nullptr);
  if (err != RT_OK)
  {
    rtLogWarn("failed to parse ns address: %s. %s", dstaddr.c_str(), rtStrError(err));
    return err;
  }

  // TODO change from multicast_interface eventually.  works for now
  std::string srcaddr = m_env->Config->resolver_multicast_interface();
  err = rtParseAddress(m_static_endpoint, srcaddr.c_str(), 0, nullptr);
  if (err != RT_OK)
  {
    err = rtGetDefaultInterface(m_static_endpoint, 0);
    if (err != RT_OK)
      return err;
  }

  return err;
}

rtError
rtRemoteUnicastResolver::openSocket()
{
  int ret = 0;

  m_static_fd = socket(m_static_endpoint.ss_family, SOCK_DGRAM, 0);
  if (m_static_fd < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to create unicast socket with family: %d. %s", 
      m_static_endpoint.ss_family, rtStrError(e));
    return e;
  }
  fcntl(m_static_fd, F_SETFD, fcntl(m_static_fd, F_GETFD) | FD_CLOEXEC);

  rtSocketGetLength(m_static_endpoint, &m_static_len);

  // listen on ANY port
  ret = bind(m_static_fd, reinterpret_cast<sockaddr *>(&m_static_endpoint), m_static_len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to bind unicast endpoint: %s", rtStrError(e));
    return e;
  }

  // now figure out which port we're bound to
  rtSocketGetLength(m_static_endpoint, &m_static_len);
  ret = getsockname(m_static_fd, reinterpret_cast<sockaddr *>(&m_static_endpoint), &m_static_len);
  if (ret < 0)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("failed to get socketname. %s", rtStrError(e));
    return e;
  }
  else
  {
    rtLogInfo("local udp socket bound to %s", rtSocketToString(m_static_endpoint).c_str());
  }

  return RT_OK;
}

rtError
rtRemoteUnicastResolver::onLocate(rtJsonDocPtr const& doc, sockaddr_storage const& /*soc*/)
{
  int key = rtMessage_GetCorrelationKey(*doc);

  std::unique_lock<std::mutex> lock(m_mutex);
  m_pending_searches[key] = doc;
  lock.unlock();
  m_cond.notify_all();

  return RT_OK;
}

void
rtRemoteUnicastResolver::runListener()
{
  rtSocketBuffer buff;
  buff.reserve(1024 * 1024);
  buff.resize(1024 * 1024);
  rtLogInfo("running listener");

  while (true)
  {
    int maxFd = 0;

    fd_set read_fds;
    fd_set err_fds;

    FD_ZERO(&read_fds);
    rtPushFd(&read_fds, m_static_fd, &maxFd);
    rtPushFd(&read_fds, m_shutdown_pipe[0], &maxFd);

    FD_ZERO(&err_fds);
    rtPushFd(&err_fds, m_static_fd, &maxFd);

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

    if (FD_ISSET(m_static_fd, &read_fds))
      doRead(m_static_fd, buff);
  }  
}

void
rtRemoteUnicastResolver::doRead(int fd, rtSocketBuffer& buff)
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
rtRemoteUnicastResolver::doDispatch(char const* buff, int n, sockaddr_storage* peer)
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
