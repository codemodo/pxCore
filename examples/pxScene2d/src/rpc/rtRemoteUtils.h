#ifndef __RT_REMOTE_UTILS_H__
#define __RT_REMOTE_UTILS_H__

#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"
class rtRemoteIAddress;
rtError rtRemoteParseNetType(std::string const& host, NetType& result);
rtError rtRemoteParseCastType(std::string const& host, NetType const& net_type, CastType& result);
rtError rtRemoteEndpointAddressToSocket(rtRemoteAddrPtr addr, sockaddr_storage& ss);
rtError rtRemoteSocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteAddrPtr& endpoint_addr);

#endif