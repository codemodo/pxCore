#ifndef __RT_REMOTE_UTILS_H__
#define __RT_REMOTE_UTILS_H__

#include "rtRemoteTypes.h"
#include "rtRemoteEndpoint.h"
#include "rtSocketUtils.h"

rtError rtRemoteParseNetType(std::string const& host, NetType& result);
rtError rtRemoteParseCastType(std::string const& host, NetType const& net_type, CastType& result);
rtError rtRemoteEndpointAddressToSocket(rtRemoteEndpointPtr addr, sockaddr_storage& ss);
rtError rtRemoteSocketToEndpointAddress(sockaddr_storage const& ss, ConnType const& conn_type, rtRemoteEndpointPtr& endpoint);
rtError rtRemoteParseUri(std::string const& uri, std::string& scheme, std::string& path, std::string& host, uint16_t* port);
bool    rtRemoteSameEndpoint(sockaddr_storage const& first, sockaddr_storage const& second);
bool    rtRemoteSameEndpoint(rtRemoteEndpointPtr const& first, rtRemoteEndpointPtr const& second);

#endif