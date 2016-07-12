class rtRemoteEndpoint
{
public:
  virtual rtRemoteEndpoint(std::string const& uri);
  virtual ~rtRemoteEndpoint();
  rtError GetUri(std::string* uri) const;
  rtError GetScheme(std::string* scheme) const;
  rtError GetFd(int* fd) const;
  virtual rtError Open(int* fd);

protected:
  std::string m_uri;
  std::string m_scheme;
  int m_fd;
};

rtRemoteEndpoint::rtRemoteEndpoint(std::string const& uri)
: m_uri(uri)
, m_scheme(nullptr)
, m_fd(-1)
{
  //pull out the scheme here
  m_scheme = "tcp"; // temp faking it
}

rtRemoteEndpoint::~rtRemoteEndpoint() { }

rtError
rtRemoteEndpoint::GetUri(std::string* uri) const
{
  if (m_uri == nullptr)
    return RT_FAIL;

  *uri = m_uri;
  return RT_OK;
}

rtError
rtRemoteEndpoint::GetScheme(std::string* scheme) const
{
  if (m_scheme == nullptr)
    return RT_FAIL;

  *scheme = m_scheme;
  return RT_OK;
}

rtError
rtRemoteEndpoint::GetFd(int* fd) const
{
  if (m_fd == -1)
    return RT_FAIL;

  *fd = m_fd;
  return RT_OK;
}

rtError
rtRemoteEndpoint::Open(int* fd)
{
  return RT_FAIL; //should be implemented in subclass
}




class rtRemoteUnixEndpoint : public virtual rtRemoteEndpoint
{
public:
  virtual rtRemoteUnixEndpoint(std::string const& uri);
  virtual ~rtRemoteUnixEndpoint();
  
  virtual rtError Open(int* fd) override;
  
  rtError GetPath(std::string* path) const;

protected:
  std::string m_path; 
};

rtRemoteUnixEndpoint::rtRemoteUnixEndpoint(std::string const& uri)
: rtRemoteEndpoint(uri)
, m_path(nullptr)
{
    // plus pull out the path
    m_path = ""; //tmp faking it
}

rtRemoteUnixEndpoint::~rtRemoteUnixEndpoint() { }

rtError
rtRemoteUnixEndpoint::Open(int* fd)
{
  return RT_FAIL; // should be implemented by subclass
}

rtError
rtRemoteUnixEndpoint::GetPath(std::string* path) const
{
  if (m_path == nullptr)
    return RT_FAIL;

  *path = m_path;
  return RT_OK;   
}


class rtRemoteInetEndpoint : public virtual rtRemoteEndpoint
{
public:
  virtual rtRemoteInetEndpoint(std::string const& uri);
  virtual ~rtRemoteInetEndpoint();
  
  virtual rtError Open(int* fd) override;
  
  rtError GetPath(std::string* path) const;

protected:
  std::string m_path; 
};

rtRemoteInetEndpoint::rtRemoteInetEndpoint(std::string const& uri)
: rtRemoteEndpoint(uri)
, m_addr(nullptr)
, m_port(-1)
{
    // plus pull out the addr and port
    m_addr = ""; //tmp faking it
    m_port = 1000;
}

rtRemoteInetEndpoint::~rtRemoteInetEndpoint() { }

rtError
rtRemoteInetEndpoint::Open(int* fd)
{
  rtError err = RT_OK;

  sockaddr_storage ep;
  err = rtParseAddress(ep, m_addr.c_str(), m_port, nullptr);
  if (err != RT_OK)
  {
    rtLogDebug("failed to parse endpoint: %s:%d %s", m_addr, m_port, rtStrError(err));
    return err;
  }
  
  m_fd = socket(endpoint.ss_family, SOCK_STREAM, 0);
  if (m_fd == -1)
  {
    rtLogDebug("failed to create file descriptor for endpoint: %s", m_scheme);
    return RT_FAIL;
  }
  *fd = m_fd;
  return RT_OK; // should be implemented by subclass
}

rtError
rtRemoteInetEndpoint::GetAddr(std::string* addr) const
{
  if (m_addr == nullptr)
    return RT_FAIL;

  *addr = m_addr;
  return RT_OK;   
}

rtError
rtRemoteInetEndpoint::GetPort(int* port) const
{
  if (m_port == nullptr)
    return RT_FAIL;

  *port = m_port;
  return RT_OK;   
}