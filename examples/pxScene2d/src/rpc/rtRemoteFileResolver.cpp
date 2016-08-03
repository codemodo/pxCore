#include "rtRemoteFileResolver.h"
#include "rtRemoteTypes.h"
#include "rtRemoteFactory.h"
#include "rtSocketUtils.h"
#include "rtRemoteMessage.h"
#include "rtRemoteConfig.h"
#include "rtRemoteEndpoint.h"
#include "rtRemoteUtils.h"
#include <rtLog.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>


rtRemoteFileResolver::rtRemoteFileResolver(rtRemoteEnvPtr env)
: m_db_fp(nullptr)
, m_env(env)
{}

rtRemoteFileResolver::~rtRemoteFileResolver()
{
  rtError err = this->close();
  if (err != RT_OK)
    rtLogWarn("failed to close resolver: %s", rtStrError(err));
}

rtError
rtRemoteFileResolver::open()
{
  std::string dbPath = m_env->Config->resolver_file_db_path();
  m_db_fp = fopen(dbPath.c_str(), "r");
  if (m_db_fp == nullptr)
  {
    rtError e = rtErrorFromErrno(errno);
    rtLogError("could not open database file %s. %s", dbPath.c_str(), rtStrError(e));
    return e;
  }
  return RT_OK;
}

rtError
rtRemoteFileResolver::registerObject(std::string const& name, rtRemoteEndpointPtr endpoint)
{
  if (m_db_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  // tmp file to write to while reading/checking contents of permanent file. renamed later.
  std::string path, tmp_path;
  path = m_env->Config->resolver_file_db_path();
  tmp_path = path + ".tmp";
  
  char *line = NULL;
  size_t len = 0;
  int read = -1;

  // lock it down
  flock(fileno(m_db_fp), LOCK_EX);
  
  fseek(m_db_fp, 0, SEEK_SET);
  while ( access( tmp_path.c_str(), F_OK ) != -1 );
  FILE *tmp_fp = fopen(tmp_path.c_str(),"w");

  // read line by line
  while ( (read = getline(&line, &len, m_db_fp)) != -1)
  {
    if (line[read-1] == '\n')
      line[read-1] = '\0';
    std::string line_string(line);
    size_t index = line_string.find("::=");
    if (index != std::string::npos)
    {
      std::string objId = line_string.substr(0, index);
      // if not the one we want to register, just copy record over
      if (name.compare(objId) != 0)
      {
        fprintf(tmp_fp, "%s\n", line);
      }
      // else overwrite it
      else
      {
        rtLogWarn("overwriting existing registered endpoint: %s", line);
      }
    }
  }
  std::string result = name + "::=" + endpoint->toUriString();
  fprintf(tmp_fp, "%s\n", result.c_str());
  fclose(tmp_fp);
  fflush(m_db_fp);
  
  if (!rename(tmp_path.c_str(), path.c_str()))
    return RT_FAIL;

  flock(fileno(m_db_fp), LOCK_UN);

  return RT_OK;
}

rtError
rtRemoteFileResolver::locateObject(std::string const& name, rtRemoteEndpointPtr& endpoint,
    uint32_t)
{
  if (m_db_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  std::string result;
  char *line = NULL;
  size_t len = 0;
  int read = -1;

  // lock it down
  flock(fileno(m_db_fp), LOCK_EX);
  
  fseek(m_db_fp, 0, SEEK_SET);

  // read line by line
  while ( (read = getline(&line, &len, m_db_fp)) != -1)
  {
    if (line[read-1] == '\n')
      line[read-1] = '\0';
    std::string line_string(line);
    size_t index = line_string.find("::=");
    if (index != std::string::npos)
    {
      std::string objId = line_string.substr(0, index);
      if (name.compare(objId) == 0)
        result = line_string.substr(index+3, std::string::npos);
    }
  }
  fflush(m_db_fp);
  flock(fileno(m_db_fp), LOCK_UN);
  
  // result contains endpoint's URI (if registered)
  if (!result.empty())
    return m_env->Factory->createEndpoint(result, endpoint);
  else
    return RT_FAIL;
}

rtError
rtRemoteFileResolver::close()
{
  if (m_db_fp != nullptr)
    fclose(m_db_fp);
  return RT_OK;
}
