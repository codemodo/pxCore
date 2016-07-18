#include "rtRemoteEndpoint.h"
#include <sstream>

// BASE //
rtRemoteIAddress::rtRemoteIAddress(std::string const& scheme)
  : m_scheme(scheme)
{ }

rtRemoteIAddress::~rtRemoteIAddress() { }

// LOCAL //
rtRemoteLocalAddress::rtRemoteLocalAddress(std::string const& scheme, std::string const& path)
  : rtRemoteIAddress(scheme)
  , m_path(path)
{ }

bool
rtRemoteLocalAddress::isSocket()
{
  return m_scheme.compare("tcp") == 0 || m_scheme.compare("udp") == 0;
}

std::string
rtRemoteLocalAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_path;
  return buff.str();
}

// NETWORK //
rtRemoteNetAddress::rtRemoteNetAddress(std::string const& scheme, std::string const& host, int port)
  : rtRemoteIAddress(scheme)
  , m_host(host)
  , m_port(port)
{ }

std::string
rtRemoteNetAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_host;
  buff << ":";
  buff << m_port;
  return buff.str();
}

// NETWORK + PATH
rtRemoteDistributedAddress::rtRemoteDistributedAddress(std::string const& scheme, std::string const& host, int port, std::string const& path)
  : rtRemoteIAddress(scheme)
  , rtRemoteNetAddress(scheme, host, port)
  , rtRemoteLocalAddress(scheme, path)
{ }

std::string
rtRemoteDistributedAddress::toUri()
{
  std::stringstream buff;
  buff << m_scheme;
  buff << "://";
  buff << m_host;
  buff << ":";
  buff << m_port;
  buff << m_path;
  return buff.str();
}


// using AddrCommandHandler = rtError (*)(std::string const&);
// using AddrCommandHandlerMap = std::map< std::string, AddrCommandHandler >;
// AddrCommandHandlerMap m_command_handlers();
// m_command_handlers.insert(CommandHandlerMap::value_type("tcp", createTcpAddr));

rtRemoteIAddress* rtRemoteAddressCreate(rtRemoteEnvironment* env, std::string const& uri)
{
  // std::string scheme = uri.substr(0, uri.find(":"));
  
  // auto itr = m_command_handlers.find(scheme);
  // if (itr == m_command_handlers.end())
  // {
  //   rtLogWarn("no command handler registered for: %s", message_type);
  //   return;
  // }

  // #define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

  // return CALL_MEMBER_FN(*this, itr->second)(uri);
  return new rtRemoteNetAddress("tcp", "127.0.0.1", 800);
}

// rtRemoteIAddress* createTcpAddr(std::string const& uri)
// {
//   return new rtRemoteNetAddress("tcp", "127.0.0.1", "800");
// }