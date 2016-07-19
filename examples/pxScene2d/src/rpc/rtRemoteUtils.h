#ifndef __RT_REMOTE_UTILS_H__
#define __RT_REMOTE_UTILS_H__

#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
class rtRemoteIAddress;
NetType rtRemoteParseNetType(std::string const& host);
CastType rtRemoteParseCastType(std::string const& host, NetType net_type);
rtError rtRemoteEndpointAddressToSocket(rtRemoteIAddress*& addr, sockaddr_storage& ss);
rtError rtRemoteSocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteIAddress*& endpoint_addr);
rtError createTcpAddress(std::string const& uri, rtRemoteIAddress*& addr);

#endif