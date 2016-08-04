#include "rtRemoteEndpointMapper.h"
#include <thread>
#include <mutex>
#include <string>

//// SIMPLE ////
rtRemoteEndpointMapperSimple::rtRemoteEndpointMapperSimple()
{
  // empty
}

rtError
rtRemoteEndpointMapperSimple::registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_hosted_objects[objectId] = endpoint;
  lock.unlock();
  return RT_OK;
}

rtError
rtRemoteEndpointMapperSimple::deregister(std::string const& objectId)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_hosted_objects[objectId] = nullptr;
  lock.unlock();
  return RT_OK;
}

rtError
rtRemoteEndpointMapperSimple::locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint)
{
  auto itr = m_hosted_objects.end();
  std::unique_lock<std::mutex> lock(m_mutex);
  itr = m_hosted_objects.find(objectId);
  lock.unlock();

  if (itr != m_hosted_objects.end())
  {
    endpoint = itr->second;
  }
  return RT_OK;
}

bool
rtRemoteEndpointMapperSimple::isRegistered(std::string const& objectId)
{
  auto itr = m_hosted_objects.end();
  std::unique_lock<std::mutex> lock(m_mutex);
  itr = m_hosted_objects.find(objectId);
  lock.unlock();

  if (itr != m_hosted_objects.end())
  {
    return true;
  }
  return false;
}

//// FILE ////
rtRemoteEndpointMapperFile::rtRemoteEndpointMapperFile()
{
  // empty
}

rtError
rtRemoteEndpointMapperFile::registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint)
{
  return RT_OK;
}

rtError
rtRemoteEndpointMapperFile::deregister(std::string const& objectId)
{
  return RT_OK;
}

rtError
rtRemoteEndpointMapperFile::locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint)
{
  return RT_OK;
}

bool
rtRemoteEndpointMapperFile::isRegistered(std::string const& objectId)
{
  return RT_OK;
}