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
rtRemoteFileResolver::registerObject(std::string const& name, rtRemoteEndpointPtr /*endpoint*/)
{
  if (m_db_fp == nullptr)
  {
    rtLogError("no database connection");
    return RT_ERROR_INVALID_ARG;
  }

  // TODO: don't put such large buffers on stack

  // read in existing records into DOM
  rapidjson::Document doc;
  char readBuffer[65536];
  fseek(m_db_fp, 0, SEEK_SET);
  flock(fileno(m_db_fp), LOCK_EX);
  rapidjson::FileReadStream is(m_db_fp, readBuffer, sizeof(readBuffer));
  doc.ParseStream(is);

  //TODO
  rapidjson::Pointer("/" + name + "/" + kFieldNameIp).Set(doc, "");
  rapidjson::Pointer("/" + name + "/" + kFieldNamePort).Set(doc, 11);

  // write updated json back to file
  char writeBuffer[65536];
  fseek(m_db_fp, 0, SEEK_SET);  
  rapidjson::FileWriteStream os(m_db_fp, writeBuffer, sizeof(writeBuffer));
  rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
  doc.Accept(writer);
  flock(fileno(m_db_fp), LOCK_UN);
  fflush(m_db_fp);

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
  if (strcmp(epType->GetString(), kEndpointTypeNet) == 0)
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
