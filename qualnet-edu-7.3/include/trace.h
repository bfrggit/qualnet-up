// Copyright (c) 2001-2014, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

/// \defgroup Package_TRACE TRACE

/// \file
/// \ingroup Package_TRACE
/// This file describes data structures and functions used for packet tracing.

#ifndef TRACE_H
#define TRACE_H

#include <stdio.h>


//  CONSTANT    :  MAX_TRACE_LENGTH  : (4090)
/// Buffer for an XML trace record.
//TBD Attempt to reduce this or eliminate it altogether
#define MAX_TRACE_LENGTH 4090


//  CONSTANT    :  TRACE_STRING_LENGTH  : 400
/// Generic maximum length of a string. The maximum
/// length of any line in the input file is 3x this value.
#define TRACE_STRING_LENGTH 400


/// Different direction of packet tracing
enum TraceDirectionType
{
    TRACE_DIRECTION_INPUT,
    TRACE_DIRECTION_OUTPUT,
    TRACE_DIRECTION_BOTH
};


/// Different types of action on packet
enum PacketActionType
{
    SEND = 1,
    RECV,
    DROP,
    ENQUEUE,
    DEQUEUE
};


/// Direction of packet with respect to the node
enum PacketDirection
{
    PACKET_IN ,
    PACKET_OUT
};


/// Keeps track of which layer is being traced.
enum TraceLayerType
{
    TRACE_APPLICATION_LAYER,
    TRACE_TRANSPORT_LAYER,
    TRACE_NETWORK_LAYER,
    TRACE_MAC_LAYER,
    TRACE_ALL_LAYERS
};


/// Specifies if included headers are output.
enum TraceIncludedHeadersType
{
    TRACE_INCLUDED_NONE,
    TRACE_INCLUDED_ALL,
    TRACE_INCLUDED_SELECTED
};


/// Gives specific comments on the packet action here
/// packet drop.
enum PacketActionCommentType
{
   NO_COMMENT,
   DROP_QUEUE_OVERFLOW = 1,
   DROP_NO_ROUTE,
   DROP_LIFETIME_EXPIRY,
   DROP_TTL_ZERO,
   DROP_NO_CONNECTION,
   DROP_EXCEED_RETRANSMIT_COUNT,
   DROP_INTERFACE_DOWN,
   DROP_INVALID_STATE,
   DROP_DUPLICATE_PACKET,
   DROP_SELF_PACKET,
   DROP_OUT_OF_ORDER,
   DROP_EXCEEDS_METERING_UNIT,
   DROP_RANDOM_SIMULATION_DROP,
   DROP_BAD_PACKET,
   DROP_BUFFER_SIZE_EXCEED,
   DROP_NO_BUFFER_SIZE_SPECIFIED,
   DROP_LOCAL_REPAIR_FAILED,
   DROP_RREQ_FLOODED_RETRIES_TIMES,
   DROP_RETRANSMIT_TIMEOUT,
   DROP_OPTION_FIELD_MISMATCH,
   DROP_PROTOCOL_UNAVAILABLE_AT_INTERFACE,
   DROP_PACKET_AT_WRONG_INTERFACE,
   DROP_WRONG_SEQUENCE_NUMBER,
   DROP_LINK_BANDWIDTH_ZERO,
   DROP_ACCESS_LIST,
   DROP_START_FRAGMENT_OFFSET_NOT_ZERO,
   DROP_ALLFRAGMENT_NOT_COLLECTED,
   DROP_ICMP_NOT_ENABLE,
   DROP_IPFORWARD_NOT_ENABLE,
   DROP_MULTICAST_ADDR_SELF_PACKET,
   DROP_MAODV_OPTION_FIELD,
   DROP_START_FRAG_LENG_LESSTHAN_IPHEDEARTYPE,
   DROP_MORE_FRAG_IPLENG_LESSEQ_IPHEDEARTYPE,
   DROP_HOP_LIMIT_ZERO,
   DROP_INVALID_LINK_ADDRESS,
   DROP_DESTINATION_MISMATCH,
   DROP_UNREACHABLE_DEST,
   DROP_UNIDENTIFIED_SOURCE,
   DROP_UNKNOWN_MSG_TYPE,
   DROP_BAD_ICMP_LENGTH,
   DROP_BAD_ROUTER_SOLICITATION,
   DROP_BAD_NEIGHBOR_SOLICITATION,
   DROP_BAD_NEIGHBOR_ADVERTISEMENT,
   DROP_ICMP6_PACKET_TOO_BIG,
   DROP_TARGET_ADDR_MULTICAST,
   DROP_ICMP6_ERROR_OR_REDIRECT_MESSAGE,
   DROP_INVALID_NETWORK_PROTOCOL,
   DROP_EXCEED_NET_DIAMETER,
};


/// Enlisting all the possible traces
enum TraceProtocolType
{
    TRACE_UNDEFINED = 0,
    TRACE_TCP = 1,
    TRACE_UDP = 2,
    TRACE_IP = 3,
    TRACE_CBR = 4,
    TRACE_FTP = 5,
    TRACE_GEN_FTP = 6,
    TRACE_BELLMANFORD = 7,
    TRACE_BGP = 8,
    TRACE_FISHEYE = 9,
    TRACE_HTTP = 10,
    TRACE_L16_CBR = 11,
    TRACE_MCBR = 12,
    TRACE_MPLS_LDP = 13,
    TRACE_MPLS_SHIM = 14,
    TRACE_TELNET = 15,
    TRACE_MGEN = 16,
    TRACE_NEIGHBOR_PROT = 17,
    TRACE_OSPFv2 = 18,
    TRACE_LINK = 19,
    TRACE_802_11 = 20,
    TRACE_CSMA = 21,
    TRACE_DAWN = 22,
    TRACE_FCSC_CSMA = 23,
    TRACE_SPAWAR_LINK16 = 24,
    TRACE_MACA = 25,
    TRACE_SATCOM = 26,
    TRACE_SWITCHED_ETHERNET = 27,
    TRACE_TDMA = 28,
    TRACE_802_3 = 29,
    TRACE_ICMP = 30,
    TRACE_GSM = 31,
    TRACE_MOBILE_IP = 32,
    TRACE_RSVP = 33,
    TRACE_AODV = 34,
    TRACE_DSR = 35,
    TRACE_FSRL = 36,
    TRACE_DVMRP = 37,
    TRACE_IGMP = 38,
    TRACE_LAR1 = 39,
    TRACE_MOSPF = 40,
    TRACE_ODMRP = 41,
    TRACE_PIM = 42,
    TRACE_STAR = 43,
    TRACE_IGRP = 44,
    TRACE_EIGRP = 45,
    TRACE_LOOKUP = 46,
    TRACE_OLSR = 47,
    TRACE_MY_APP = 48,
    TRACE_MYROUTE = 49,
    TRACE_SEAMLSS = 50,
    TRACE_STP = 51,
    TRACE_VLAN = 52,
    TRACE_GVRP = 53,
    TRACE_HSRP = 54,
    TRACE_802_11a = 55,
    TRACE_802_11b = 56,

    TRACE_TRAFFIC_GEN = 57,
    TRACE_TRAFFIC_TRACE = 58,
    TRACE_RTP = 59,
    TRACE_H323 = 60,
    TRACE_SIP = 61,
    TRACE_NDP = 62,
    TRACE_APP_MGEN = 63,
    TRACE_AAL5 = 64,
    TRACE_SAAL = 65,
    TRACE_ATM_LAYER = 66,
    TRACE_UTIL_EXTERNAL = 67,
    TRACE_UTIL_ABSTRACT_EVENT = 68,
    // ADDON_HELLO
    TRACE_STOCHASTIC = 69,

    TRACE_MESSENGER = 70,
    TRACE_IPV6 = 71,
    TRACE_ESP = 72,
    TRACE_AH = 73,

    TRACE_VBR = 74,
    TRACE_ALOHA = 75,
    TRACE_GENERICMAC = 76,
    TRACE_BRP = 77,
    TRACE_SUPERAPPLICATION = 78,
    TRACE_RIP = 79,
    TRACE_RIPNG = 80,
    TRACE_IARP = 81,
    TRACE_ZRP = 82,
    TRACE_IERP = 83,

    //InsertPatch TRACE_VAR
    TRACE_CELLULAR = 84,
    TRACE_SATTSM = 85,
    TRACE_SATTSM_SHIM = 86,
    TRACE_HELLO = 87,
    TRACE_ARP = 88,
    TRACE_HLA = 89,
    // TUTORIAL_INTERFACE
    TRACE_INTERFACETUTORIAL = 90,

    // ALE_ASAPS_LIB
    TRACE_ALE = 91,

    TRACE_RTCP = 92,

    // ADDON_NETWARS
    TRACE_NETWARS = 93,

    TRACE_SOCKET_EXTERNAL = 103,

    TRACE_FORWARD = 124,

    // SATELLITE_LIB
    TRACE_SATELLITE_RSV = 125,
    TRACE_SATELLITE_BENTPIPE = 126,

    TRACE_DOT11 = 127,

    // ADVANCED_WIRELESS_LIB
    TRACE_DOT16 = 128,
    TRACE_MAC_DOT16 = 129,
    TRACE_PHY_DOT16 = 130,

    // MILITARY_RADIOS_LIB
    TRACE_TADIL_LINK11 = 131,
    TRACE_TADIL_LINK16 = 132,
    TRACE_TADIL_TRANSPORT = 133,
    TRACE_THREADEDAPP = 134,

    // ADDON_MAODV
    TRACE_MAODV = 135,

    TRACE_OSPFv3 = 136,

    TRACE_OLSRv2_NIIGATA = 137,

    TRACE_DYMO = 139,
    TRACE_ANE = 140,
    TRACE_ICMPV6 = 141,
    TRACE_LLC = 142,

    // Network Security
    TRACE_WORMHOLE = 143,
    TRACE_ANODR = 144,
    TRACE_SECURENEIGHBOR = 145,
    TRACE_SECURECOMMUNITY = 146,
    TRACE_MACDOT11_WEP = 147,
    TRACE_MACDOT11_CCMP = 148,
    TRACE_ISAKMP = 149,
    TRACE_MDP = 150,
    TRACE_MAC_802_15_4 = 151,
    TRACE_JAMMER = 152,
    TRACE_DOS = 153,

    // Military Radios
    TRACE_EPLRS = 154,
    TRACE_ODR = 155,
    TRACE_SDR = 156,

    TRACE_NETWORK_NGC_HAIPE = 157,

    // start cellular
    TRACE_CELLULAR_PHONE = 158,
    TRACE_UMTS_LAYER3 = 159,
    TRACE_UMTS_LAYER2 = 160,
    TRACE_UMTS_PHY = 161,

    TRACE_SOCKETLAYER = 165,

    /* EXata Related -- START */
    // SNMP
    TRACE_SNMP = 166,
    TRACE_EFTP = 167,
    TRACE_EHTTP = 168,
    TRACE_ETELNET = 169,
    /* EXata Related -- END */

    TRACE_MLS_IAHEP = 171,

    // LTE
    TRACE_RRC_LTE = 186,
    TRACE_PDCP_LTE = 187,
    TRACE_RLC_LTE = 188,
    TRACE_MAC_LTE = 189,
    TRACE_PHY_LTE = 190,
    TRACE_EPC_LTE = 191,

    TRACE_AMSDU_SUB_HDR = 192,
    TRACE_ZIGBEEAPP = 193,
    TRACE_DNS = 194,

    // DHCP
    TRACE_DHCP = 195,

    // PortScanner
    TRACE_PORTSCAN = 196,

    TRACE_IP_EXATA_NAT_NO = 197,
    TRACE_MSDP = 198,

    // Modifications
    TRACE_UP,

    // Must be last one!!!
    TRACE_ANY_PROTOCOL // = 199
};



// FUNCTION POINTER :: TracePrintXMLFn
// DESCRIPTION :: Protocol callback funtion to print trace.

typedef void (*TracePrintXMLFn)(Node* node, Message* message);


// FUNCTION POINTER :: TracePrintXMLFn
// DESCRIPTION :: Protocol callback funtion to print trace.

typedef void (*TracePrintXMLFun)(Node* node, Message* message ,NetworkType netType);




/// Keeps track of which protocol is being traced.
struct TraceData
{
    BOOL traceList[TRACE_ANY_PROTOCOL];
    BOOL traceAll;
    TraceDirectionType direction;

    BOOL layer[TRACE_ALL_LAYERS];
    TraceIncludedHeadersType traceIncludedHeaders;

    char xmlBuf[MAX_TRACE_LENGTH];
    TracePrintXMLFn xmlPrintFn[TRACE_ANY_PROTOCOL];
    TracePrintXMLFun xmlPrintFun[TRACE_ANY_PROTOCOL];
};


/// Gives details of the packet queue
struct PktQueue
{
   unsigned short interfaceID;
   unsigned char  queuePriority;
};


/// Keeps track of protocol action
struct ActionData
{
    PacketActionType   actionType;
    PacketActionCommentType actionComment;
    PktQueue pktQueue;
};


/// Initialize necessary trace information before
/// simulation starts.
///
/// \param node  this node
/// \param nodeInput  access to configuration file
///
void TRACE_Initialize(Node* node, const NodeInput* nodeInput);


/// Determine if TRACE-ALL is enabled from
/// configuration file.
///
/// \param node  this node
///
/// \return TRUE if TRACE-ALL is enabled, FALSE otherwise.
BOOL TRACE_IsTraceAll(Node* node);


/// Print trace information to file.  To be used with Tracer.
///
/// \param node  this node
/// \param message  Packet to print trace info from.
/// \param layerType  Layer that is calling this function.
/// \param pktDirection  If the packet is coming out of
///    arriving  at a node.
/// \param actionData  more details about the packet action
///
void TRACE_PrintTrace(Node* node,
                      Message* message,
                      TraceLayerType layerType,
                      PacketDirection pktDirection,
                      ActionData* actionData);

/// Print trace information to file.  To be used with Tracer.
///
/// \param node  this node
/// \param message  Packet to print trace info from.
/// \param layerType  Layer that is calling this function.
/// \param pktDirection  If the packet is coming out of
///    arriving  at a node
/// \param actionData  more details about the packet action
/// \param netType  The network type.
///
void TRACE_PrintTrace(Node* node,
                      Message* message,
                      TraceLayerType layerType,
                      PacketDirection pktDirection,
                      ActionData* actionData,
                      NetworkType netType);

/// Enable XML trace for a particular protocol.
///
/// \param node  this node
/// \param protocol  protocol to enable trace for
/// \param protocolName  name of protocol
/// \param xmlPrintFn  callback function
/// \param writeMap  flag to print protocol ID map
///
void TRACE_EnableTraceXMLFun(Node* node,
                          TraceProtocolType protocol,
                          const char* protocolName,
                          TracePrintXMLFun xmlPrintFun,
                          BOOL writeMap
                          );


/// Enable XML trace for a particular protocol.
///
/// \param node  this node
/// \param protocol  protocol to enable trace for
/// \param protocolName  name of protocol
/// \param xmlPrintFn  callback function
/// \param writeMap  flag to print protocol ID map
///
void TRACE_EnableTraceXML(Node* node,
                          TraceProtocolType protocol,
                          const char* protocolName,
                          TracePrintXMLFn xmlPrintFn,
                          BOOL writeMap);


/// Disable XML trace for a particular protocol.
///
/// \param node  this node
/// \param protocol  protocol to enable trace for
/// \param protocolName  name of protocol
/// \param writeMap  flag to print protocol ID map
///
void TRACE_DisableTraceXML(Node* node,
                           TraceProtocolType protocol,
                           const char* protocolName,
                           BOOL writeMap);


/// Write trace information to a buffer, which will then
/// be printed to a file.
///
/// \param node  This node.
/// \param buf  Content to print to trace file.
///
void TRACE_WriteToBufferXML(Node* node, char* buf);


/// Write trace header information to the partition's
/// trace file
///
/// \param fp  pointer to the trace file.
///
void TRACE_WriteXMLTraceHeader(NodeInput* nodeInput, FILE* fp);


/// Write trace tail information to the partition's
/// trace file
///
/// \param fp  pointer to the trace file.
///
void TRACE_WriteXMLTraceTail(FILE* fp);


#endif //TRACE_H
