#include "rtRpcTransport.h"
#include "rtSocketUtils.h"

#include <rtLog.h>

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <rapidjson/document.h>

rtRpcTransport::rtRpcTransport(sockaddr_storage const& ss)
  : m_fd(-1)
{
  m_remote_endpoint = ss;
}

rtRpcTransport::~rtRpcTransport()
{
  if (m_fd > 0)
  {
    shutdown(m_fd, SHUT_RDWR);
    close(m_fd);
  }
}

rtError
rtRpcTransport::start()
{
  rtError err = connect_rpc_endpoint();
  if (err != RT_OK)
  {
    rtLogWarn("failed to connect to rpc endpoint");
    return err;
  }

  pthread_create(&m_thread, NULL, &rtRpcTransport::run_listener, this);
  return RT_OK;
}

rtError
rtRpcTransport::connect_rpc_endpoint()
{
  m_fd = socket(m_remote_endpoint.ss_family, SOCK_STREAM, 0);
  if (m_fd < 0)
  {
    rtLogError("failed to create socket. %s", strerror(errno));
    return RT_FAIL;
  }

  socklen_t len;
  rtSocketGetLength(m_remote_endpoint, &len);

  int ret = connect(m_fd, reinterpret_cast<sockaddr *>(&m_remote_endpoint), len);
  if (ret < 0)
  {
    rtLogError("failed to connect to remote rpc endpoint. %s", strerror(errno));
    return RT_FAIL;
  }

  rtLogInfo("new tcp connection to: %s", rtSocketToString(m_remote_endpoint).c_str());
  return RT_OK;
}

rtError
rtRpcTransport::start_session(std::string const& object_id)
{
  rapidjson::Document doc;
  doc.SetObject();
  doc.AddMember("type", "open-session", doc.GetAllocator());
  doc.AddMember("object-id", object_id, doc.GetAllocator());
  return rtSendDocument(doc, m_fd, NULL);
}

rtError
rtRpcTransport::send_keep_alive()
{
  rapidjson::Document doc;
  doc.SetObject();
  doc.AddMember("type", "keep-alive", doc.GetAllocator());
  rapidjson::Value ids(rapidjson::kArrayType);
  for (auto const& id : m_object_list)
    ids.PushBack(rapidjson::Value().SetString(id.c_str(), id.size()), doc.GetAllocator());
  doc.AddMember("ids", ids, doc.GetAllocator());
  return rtSendDocument(doc, m_fd, NULL);
}

void*
rtRpcTransport::run_listener(void* argp)
{
  rtRpcTransport* obj = reinterpret_cast<rtRpcTransport *>(argp);
  obj->run_listener();
  return NULL;
}

rtError
rtRpcTransport::run_listener()
{
  rt_sockbuf_t buff;
  buff.reserve(1024 * 4);
  buff.resize(1024 * 4);

  while (true)
  {
    int maxFd = 0;

    fd_set read_fds;
    fd_set err_fds;

    FD_ZERO(&read_fds);
    rtPushFd(&read_fds, m_fd, &maxFd);

    FD_ZERO(&err_fds);
    rtPushFd(&err_fds, m_fd, &maxFd);

    int ret = select(maxFd + 1, &read_fds, NULL, &err_fds, NULL);
    if (ret == -1)
    {
      int err = errno;
      rtLogWarn("select failed. %s", strerror(errno));
      if (err == EBADF)
        return RT_OK;
      else
        continue;
    }

    if (FD_ISSET(m_fd, &read_fds))
      readn(m_fd, buff);
  }

  return RT_OK;
}

rtError
rtRpcTransport::readn(int fd, rt_sockbuf_t& buff)
{
  docptr_t doc;
  rtError err = rtReadMessage(fd, buff, doc);
  if (err != RT_OK)
    return err;

  if (!doc->HasMember("type"))
  {
    rtLogWarn("received JSON message with no type");
    return RT_FAIL;
  }

  std::string cmd = (*doc)["type"].GetString();

  auto itr = m_command_handlers.find(cmd);
  if (itr == m_command_handlers.end())
  {
    rtLogWarn("no command handler registered for: %s", cmd.c_str());
    return RT_FAIL;
  }

  // https://isocpp.org/wiki/faq/pointers-to-members#macro-for-ptr-to-memfn
  #define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
  return CALL_MEMBER_FN(*this, itr->second)(doc); // , peer);
}