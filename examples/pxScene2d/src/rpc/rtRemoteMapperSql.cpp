#include "rtRemoteMapperSql.h"
#include "rtRemoteTypes.h"
#include <string>

rtRemoteMapperSql::rtRemoteMapperSql(rtRemoteEnvPtr env)
: rtRemoteIMapper(env)
{
  // TODO
}

rtError
rtRemoteMapperSql::registerEndpoint(std::string const& /*objectId*/, rtRemoteEndpointPtr const& /*endpoint*/)
{
  // TODO
  return RT_FAIL;
}

rtError
rtRemoteMapperSql::deregisterEndpoint(std::string const& /*objectId*/)
{
  // TODO
  return RT_FAIL;
}

rtError
rtRemoteMapperSql::lookupEndpoint(std::string const& /*objectId*/, rtRemoteEndpointPtr& /*endpoint*/)
{
  // TODO
  return RT_FAIL;
}

bool
rtRemoteMapperSql::isRegistered(std::string const& /*objectId*/)
{
  // TODO
  return false;
}