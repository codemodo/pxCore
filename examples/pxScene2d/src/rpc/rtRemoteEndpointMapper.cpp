#include "rtRemoteEndpointMapper.h"
#include <thread>
#include <mutex>
#include <string>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rtRemoteConfig.h"
#include "rtRemoteFactory.h"

//// SIMPLE ////
rtRemoteEndpointMapperSimple::rtRemoteEndpointMapperSimple(rtRemoteEnvPtr env)
: rtRemoteIEndpointMapper(env)
{
}

rtError
rtRemoteEndpointMapperSimple::init()
{
  return RT_OK;
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
rtRemoteEndpointMapperFile::rtRemoteEndpointMapperFile(rtRemoteEnvPtr env)
: rtRemoteIEndpointMapper(env)
{
}

rtError
rtRemoteEndpointMapperFile::init()
{
  std::string filePath = m_env->Config->resolver_file_db_path();
  m_fp = fopen(filePath.c_str(), "r");
  if (m_fp == nullptr)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("could not open database file %s. %s", filePath.c_str(), rtStrError(e));
    return e;
  }
  return RT_OK;
}

rtError
rtRemoteEndpointMapperFile::registers(std::string const& objectId, rtRemoteEndpointPtr const& endpoint)
{
  if (m_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  // tmp file to write to while reading/checking contents of permanent file. renamed later.
  std::string path, tmpPath;
  path = m_env->Config->resolver_file_db_path();
  tmpPath = path + ".tmp";
  
  char *line = NULL;
  size_t len = 0;
  int read = -1;

  // lock it down
  flock(fileno(m_fp), LOCK_EX);
  
  fseek(m_fp, 0, SEEK_SET);
  while ( access( tmpPath.c_str(), F_OK ) != -1 );
  FILE *tmpFp = fopen(tmpPath.c_str(),"w");

  // read line by line
  while ( (read = getline(&line, &len, m_fp)) != -1)
  {
    if (line[read-1] == '\n')
      line[read-1] = '\0';
    std::string lineString(line);
    size_t index = lineString.find("::=");
    if (index != std::string::npos)
    {
      std::string readId = lineString.substr(0, index);
      // if not the one we want to register, just copy record over
      if (objectId.compare(readId) != 0)
      {
        fprintf(tmpFp, "%s\n", line);
      }
      // else overwrite it
      else
      {
        rtLogWarn("overwriting existing registered endpoint: %s", line);
      }
    }
  }
  std::string result = objectId + "::=" + endpoint->toUriString();
  fprintf(tmpFp, "%s\n", result.c_str());
  fclose(tmpFp);
  fflush(m_fp);
  
  if (rename(tmpPath.c_str(), path.c_str()))
  {
    rtLogError("failed to save file");
    return RT_FAIL;
  }

  flock(fileno(m_fp), LOCK_UN);

  return RT_OK;
}

rtError
rtRemoteEndpointMapperFile::deregister(std::string const& objectId)
{
  rtError err = RT_OK;
  if (m_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  // tmp file to write to while reading/checking contents of permanent file. renamed later.
  std::string path, tmpPath;
  path = m_env->Config->resolver_file_db_path();
  tmpPath = path + ".tmp";
  
  char *line = NULL;
  size_t len = 0;
  int read = -1;

  // lock it down
  flock(fileno(m_fp), LOCK_EX);
  
  fseek(m_fp, 0, SEEK_SET);
  while ( access( tmpPath.c_str(), F_OK ) != -1 );
  FILE *tmpFp = fopen(tmpPath.c_str(),"w");

  // read line by line
  while ( (read = getline(&line, &len, m_fp)) != -1)
  {
    if (line[read-1] == '\n')
      line[read-1] = '\0';
    std::string lineString(line);
    size_t index = lineString.find("::=");
    if (index != std::string::npos)
    {
      std::string readId = lineString.substr(0, index);
      // copy over everything except for the one to be deregistered
      if (objectId.compare(readId) != 0)
      {
        fprintf(tmpFp, "%s\n", line);
      }
      else
      {
        rtLogInfo("deleting record for %s", readId.c_str());
        err = RT_OK;
      }
    }
  }
  fclose(tmpFp);
  fflush(m_fp);
  
  if (rename(tmpPath.c_str(), path.c_str()))
  {
    rtLogError("failed to save file");
    return RT_FAIL;
  }
    
  flock(fileno(m_fp), LOCK_UN);

  return err;
}

rtError
rtRemoteEndpointMapperFile::locate(std::string const& objectId, rtRemoteEndpointPtr& endpoint)
{
  if (m_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  std::string result;
  char *line = NULL;
  size_t len = 0;
  int read = -1;

  // lock it down
  flock(fileno(m_fp), LOCK_EX);
  
  fseek(m_fp, 0, SEEK_SET);

  // read line by line
  while ( (read = getline(&line, &len, m_fp)) != -1)
  {
    if (line[read-1] == '\n')
      line[read-1] = '\0';
    std::string lineString(line);
    size_t index = lineString.find("::=");
    if (index != std::string::npos)
    {
      std::string readId = lineString.substr(0, index);
      if (objectId.compare(readId) == 0)
        result = lineString.substr(index+3, std::string::npos);
    }
  }
  fflush(m_fp);
  flock(fileno(m_fp), LOCK_UN);
  
  // result contains endpoint's URI (if registered)
  if (!result.empty())
    return m_env->Factory->createEndpoint(result, endpoint);
  else
    return RT_FAIL;
}

bool
rtRemoteEndpointMapperFile::isRegistered(std::string const& objectId)
{
  return false;
}