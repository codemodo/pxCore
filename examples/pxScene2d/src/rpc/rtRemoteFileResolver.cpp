#include "rtRemoteFileResolver.h"
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

#include <rapidjson/document.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/pointer.h>


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
  m_db_fp = fopen(dbPath.c_str(), "r+");
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

  std::string path, tmp_path;
  path = m_env->Config->resolver_file_db_path();
  tmp_path = path + ".tmp";

  fseek(m_db_fp, 0, SEEK_SET);
  flock(fileno(m_db_fp), LOCK_EX);
  FILE *tmp_fp = fopen(tmp_path.c_str(),"w");
  char *line = NULL;
  size_t len = 0;
  while (getline(&line, &len, m_db_fp) != -1)
  {
    std::string line_string(line);
    size_t index = line_string.find("::=");
    if (index != std::string::npos)
    {
      std::string objId = line_string.substr(0, index);
      if (name.compare(objId) != 0)
        fprintf(tmp_fp, "%s\n", line);
    }
  }
  std::string result = name + "::=" + endpoint->toUri();
  fprintf(tmp_fp, "%s", result.c_str());
  fclose(tmp_fp);
  flock(fileno(m_db_fp), LOCK_UN);
  fflush(m_db_fp);
  
  if (!rename(tmp_path.c_str(), path.c_str()))
    return RT_FAIL;

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

  // read file into DOM object
  rapidjson::Document doc;
  char readBuffer[65536];
  flock(fileno(m_db_fp), LOCK_EX);
  fseek(m_db_fp, 0, SEEK_SET);  
  rapidjson::FileReadStream is(m_db_fp, readBuffer, sizeof(readBuffer));
  doc.ParseStream(is);
  flock(fileno(m_db_fp), LOCK_UN);
  
  // check if name is registered
  if (!rapidjson::Pointer("/" + name).Get(doc))
    return RT_FAIL;
  
  // pull registered IP and port
  rapidjson::Value* scheme = rapidjson::Pointer("/" + name + "/" + kFieldNameScheme).Get(doc);
  rapidjson::Value* epType = rapidjson::Pointer("/" + name + "/" + kFieldNameEndpointType).Get(doc);
  RT_ASSERT(epType != nullptr);
  if (strcmp(epType->GetString(), kEndpointTypeRemote) == 0)
  {
    rapidjson::Value *ip = rapidjson::Pointer("/" + name + "/" + kFieldNameIp).Get(doc);
    rapidjson::Value *port = rapidjson::Pointer("/" + name + "/" + kFieldNamePort).Get(doc);
    endpoint = std::make_shared<rtRemoteEndpointRemote>(scheme->GetString(), ip->GetString(), port->GetInt());
  }
  else if (strcmp(epType->GetString(), kEndpointTypeLocal) == 0)
  {
    rapidjson::Value *path = rapidjson::Pointer("/" + name + "/" + kFieldNamePath).Get(doc);
    endpoint = std::make_shared<rtRemoteEndpointLocal>(scheme->GetString(), path->GetString());
  }
  else
  {
    return RT_FAIL;
  }
    
  return RT_OK;
}

rtError
rtRemoteFileResolver::close()
{
  if (m_db_fp != nullptr)
    fclose(m_db_fp);
  return RT_OK;
}
