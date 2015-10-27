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

#include "dynamic.h"
#include "list.h"
#include "mapping.h"


class HITLInfo;

/// \defgroup Package_APPLICATION_LAYER APPLICATION LAYER

/// \file
/// \ingroup Package_APPLICATION_LAYER
/// This file describes data structures and functions used by the Application Layer.

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#ifdef ADDON_DB
class StatsDBAppEventParam;
#endif

// dynamic address
struct AppToTcpOpen;

// Pseudo Traffic sender layer
class AppTrafficSender;

/// Application default tos value
#define APP_DEFAULT_TOS  0x00


/// Maximum size of data unit
#define APP_MAX_DATA_SIZE  ((IP_MAXPACKET)-(MSG_MAX_HDR_SIZE))

/// Default size of Application layer queue (in byte)
#define DEFAULT_APP_QUEUE_SIZE                  640000

/// Prime number indicating port table size
#define PORT_TABLE_HASH_SIZE    503

#ifdef MILITARY_RADIOS_LIB

/// Maximum fragment size supported by LINK16 MAC protocol.
/// For Link16, it seems the fragment size should be
/// 8 * 9 bytes = 72 bytes
#define     MAC_LINK16_FRAG_SIZE            72

/// Default interface of MAC layer.
// ASSUMPTION  :: Source and Destination node must have only one
/// interface with TADIL network.
#define     MAC_DEFAULT_INTERFACE           0

/// Store Link16/IP gateway forwarding table
struct Link16GatewayData;
#endif // MILITARY_RADIOS_LIB

/// Enumerates the type of application protocol
enum AppType
{
    /* Emulated applications */
    APP_EFTP_SERVER_DATA = 20,
    APP_EFTP_SERVER = 21,
    APP_ETELNET_SERVER = 23,
    APP_EHTTP_SERVER = 80,
    /* Application models */
    APP_FTP_SERVER_DATA = 8020,
    APP_FTP_SERVER = 8021,
    APP_TELNET_SERVER = 8023,
    APP_HTTP_SERVER = 8080,
    APP_FTP_CLIENT = 22,
    APP_TELNET_CLIENT = 24,
    APP_GEN_FTP_SERVER,
    APP_GEN_FTP_CLIENT,
    APP_CBR_SERVER = 59,
    APP_CBR_CLIENT = 60,
    APP_MCBR_SERVER,
    APP_MCBR_CLIENT,
    APP_MGEN3_SENDER,
    APP_DREC3_RECEIVER,
    APP_LINK_16_CBR_SERVER,
    APP_LINK_16_CBR_CLIENT,

    // Dynamic address
    APP_DHCP_SERVER = 67,
    APP_DHCP_CLIENT = 68,
    APP_DHCP_RELAY  = 69,


    APP_MDP = 7000,
    APP_HTTP_CLIENT = 81,
    /* Application with random distribution */
    APP_TRAFFIC_GEN_CLIENT,
    APP_TRAFFIC_GEN_SERVER,
    APP_TRAFFIC_TRACE_CLIENT,
    APP_TRAFFIC_TRACE_SERVER,
    APP_NEIGHBOR_PROTOCOL,
    APP_LOOKUP_SERVER,
    APP_LOOKUP_CLIENT,
    /* Tutorial application type */
    APP_MY_APP,
    /* Application-layer routing protocols */
    APP_ROUTING_BELLMANFORD = 519,
    APP_ROUTING_FISHEYE = 160,      // IP protocol number
    APP_SNMP_AGENT = 161,       //SNMP protocol
    APP_SNMP_TRAP = 162,        //SNMP TRAP

    /* Static routing */
    APP_ROUTING_STATIC,

    APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4 = 179,

    APP_ROUTING_HSRP = 1985,

    APP_MSDP = 639,

    APP_MPLS_LDP = 646,
    APP_ROUTING_OLSR_INRIA = 698,

    APP_MESSENGER = 702,

    APP_H323,
    APP_SIP = 5060, // default SIP port
    APP_VOIP_CALL_INITIATOR,
    APP_VOIP_CALL_RECEIVER,
    // even/odd UDP ports pair for RTP/RTCP
    APP_RTP = 5004, // must be even
    APP_RTCP = 5005, // must be one more than RTP port
    APP_H225_RAS_MULTICAST = 1718,
    APP_H225_RAS_UNICAST,
    APP_MGEN,


    // ADDON_HELLO
    APP_HELLO,
    APP_STOCHASTIC_SERVER,
    APP_STOCHASTIC_CLIENT,

    // Modifications
    APP_UP_SERVER,
    APP_UP_CLIENT,

    // HLA_INTERFACE
    APP_HLA = 1800,

    //APP_ROUTING_RIP,
    APP_ROUTING_RIP=520,
    APP_ROUTING_RIPNG=1807,

    APP_FORWARD = APP_ROUTING_RIPNG + 6,

    APP_VBR_CLIENT,
    APP_VBR_SERVER,

    APP_ROUTING_OLSRv2_NIIGATA,

    // TUTORIAL_INTERFACE
    APP_INTERFACETUTORIAL = 6001,

    APP_TRIGGER_APP,

    APP_CELLULAR_ABSTRACT,
    APP_UMTS_CALL,

    // Military
    APP_THREADEDAPP_CLIENT,
    APP_THREADEDAPP_SERVER,
    APP_DYNAMIC_THREADEDAPP,

    // Stats DB
    APP_STATSDB_CONN,
    APP_STATSDB_AGGREGATE,
    APP_STATSDB_SUMMARY,
    QOS_STATSDB_AGGREGATE,
    QOS_STATSDB_SUMMARY,

    // so that we dont have any port conflicts with these types
    APP_SUPERAPPLICATION_CLIENT = 2,
    APP_SUPERAPPLICATION_SERVER = 3,

    APP_SOCKET_LAYER,

    APP_TIMER,

    // Cyber
    APP_JAMMER,
    APP_DOS_ATTACKER,
    APP_DOS_VICTIM,
    APP_FIREWALL,
    APP_CHANNEL_SCANNER,
    APP_SIGINT,
    APP_EAVESDROP,

    APP_GUI_HITL = APP_EAVESDROP + 3,

    // Sensor Networks
    APP_ZIGBEEAPP_CLIENT = 836, // was automatically assigned
    APP_ZIGBEEAPP_SERVER,

    APP_EPC_LTE = 8514, // was automatically assigned

    APP_DNS_CLIENT,
    APP_DNS_SERVER,
    APP_TRAFFIC_SENDER,

    APP_PORTSCANNER,
    APP_PROTOCOL_ISAKMP = 500,
    APP_PLACEHOLDER
};

/// Information relevant to specific app layer protocol
struct AppInfo
{
    AppType  appType;          /* type of application */
    void*    appDetail;          /* statistics of the application */
    AppInfo* appNext; /* link to the next app of the node */
};


/// Store port related information
struct PortInfo
{
    AppType appType;
    unsigned short portNumber;
    PortInfo* next;
};

/*! An identifier for an application.  Not necessarily unique. */
struct AppIdentifier
{
    Address sourceAddress;
    int id;

    bool operator < (const AppIdentifier& rhs) const
    {
        if (id < rhs.id)
        {
            return true;
        }
        if (id > rhs.id)
        {
            return false;
        }

        // Finally compare IPv4 address
        if (sourceAddress.networkType == NETWORK_IPV4)
        {
            return GetIPv4Address(sourceAddress)
                < GetIPv4Address(rhs.sourceAddress);
        }
        if (sourceAddress.networkType == NETWORK_IPV6)
        {
            if (Ipv6CompareAddr6(rhs.sourceAddress.interfaceAddr.ipv6,
                sourceAddress.interfaceAddr.ipv6)> 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            ERROR_ReportError("other types not supported");
        }

        return false;
    }
};

struct AppMap
{
private:
    std::map<AppIdentifier, AppInfo*> m_map;

    std::map<int, AppInfo*> m_mapById;

public:
    /*! Push app detail onto the list.
     * \param sourceAddress   the source address of the application
     * \param id              A non-unique identifier for the application.
     *                        portNumber may be source port for udp or
     *                        connection id for tcp.  It should be as unique
     *                        as possible.
     * \param type            The application type.  Same as what is put in
     *                        appInfo.
     * \param detail          The app detail.  Pointer to the same block of
     *                        memory that is put in appInfo.
     */
    void pushDetail(
        const Address& sourceAddress,
        int id,
        AppType type,
        void* detail);

    //! Overloaded pushDetail, like the above but with no address
    void pushDetail(
        int id,
        AppType type,
        void* detail);

    /*! Return a list of AppInfo structures matching the sourceAddress and
     *  id.  The application must loop through this list and search for its
     *  specific AppInfo.  Returns NULL if none found. */
    AppInfo* getAppInfo(const Address& sourceAddress, int id);

    //! Overloaded getAppInfo, like the above but with no address
    AppInfo* getAppInfo(int id);
};

//-------- Function Pointer definitions for multimedia signalling ---------

/// Multimedia callback funtion to open request for a TCP
/// connection from the initiating terminal
///
/// \param node  Pointer to the node
/// \param voip  Pointer to the voip application
///
typedef
void (*InitiateConnectionType) (Node* node, void* voip);


/// Multimedia callback funtion to close TCP connection as
/// requested by VOIP application
///
/// \param node  Pointer to the node
/// \param voip  Pointer to the voip application
///
typedef
void (*TerminateConnectionType)(Node* node, void* voip);


/// Multimedia callback funtion to check whether node is
/// in initiator mode
///
/// \param node  Pointer to the node
///
/// \return TRUE if the node is initiator, FALSE otherwise
typedef
BOOL (*IsHostCallingType) (Node* node, short localPort);


/// Multimedia callback funtion to check whether node is
/// in receiver mode
///
/// \param node  Pointer to the node
///
/// \return TRUE if the node is receiver, FALSE otherwise
typedef
BOOL (*IsHostCalledType) (Node* node, short localPort);


/// Store multimedia signalling related information
struct AppMultimedia
{
    // MULTIMEDIA SIGNALLING related structures
    // 0-INVALID, 1-H323, 2-SIP
    unsigned char sigType;

    // currently SIP and H323 are available, H323Data and SipData
    // structure pointer is stored depending on the signalling type
    void* sigPtr;

    InitiateConnectionType  initiateFunction;
    TerminateConnectionType terminateFunction;

    IsHostCallingType       hostCallingFunction;
    IsHostCalledType        hostCalledFunction;

    double                  totalLossProbablity;
    clocktype               connectionDelay;
    clocktype               callTimeout;
};

// App Reset functions defined to support NMS
typedef void (*AppSetFunctionPtr)(Node*, const NodeInput*);

// Set rules defined
struct AppResetFunction
{
    // Corresponding set function
    AppSetFunctionPtr setFuncPtr;

    // the next match command
    AppResetFunction* next;
};

struct
AppResetFunctionList
{
    AppResetFunction* first;
    AppResetFunction* last;
};

/// Details of application data structure in node
/// structure
struct AppData
{
    AppInfo* appPtr;         /* pointer to the list of app info */

    /*! A map of applications for quick lookup.  Contains duplicate data of
     *  appPtr but in a quickly accessible form. */
    AppMap* appMap;
    PortInfo* portTable;     /* pointer to the port table */
    unsigned short    nextPortNum;    /* next available port number */
    BOOL     appStats;       /* whether app stat is enabled */
    AppType exteriorGatewayProtocol;
    BOOL routingStats;
    void* routingVar;        /* deprecated */
    void* bellmanford;
    void* olsr;
    void* olsrv2;
    void* mdp;
    void* hsrp;
    void* exteriorGatewayVar;
    void* userApplicationData;

    void* voipCallReceiptList;  // Maintain a list of receipt call.
    void* rtpData;

    AppMultimedia* multimedia;
    // dns
    void* dnsData;
    // dns

// startCellular
    void* appCellularAbstractVar;
    void* umtscallVar;
// endCellular

    /*
     * Application statistics for the node using TCP.
     */
    Int32 numAppTcpFailure;     /* # of apps terminated due to failure */

    /* Used to determine unique client/server pair. */
    Int32 uniqueId;

    /*
     * User specified session parameters.
     */
    clocktype telnetSessTime;/* duration of a telnet session */

    void* triggerAppData;

#ifdef CYBER_CORE
    unsigned char* isakmpEnabledInterface;
    void* isakmpData;
#endif // CYBER_CORE

#ifdef MILITARY_UNRESTRICTED_LIB
    void* triggerThreadData;
#endif // MILITARY_UNRESTRICTED_LIB

#ifdef MILITARY_RADIOS_LIB
    Link16GatewayData* link16Gateway;   // data for link16 gateway
#endif // MILITARY_RADIOS_LIB

#ifdef ADDON_NGCNMS
    void *mopData;
    AppResetFunctionList *resetFunctionList;
#endif

  // superapplication client pointer list
  // pointer to a structure which is a vector of client list
  void* clientPtrList;

  // create a SuperAppConfigurationData type pointer for maintaing
  // configuration data in the below structure.
  LinkedList* superAppconfigData;

#ifdef ADDON_DB
    // APPL_MULTICAST_MAP is a 2 level map. The first key is the multicast
    // address and the second key is the NodeId belonging to that multicast
    // group.
    typedef std::map<NodeId, Int32> APPL_MULTICAST_NODE_MAP;
    typedef std::map<NodeAddress, APPL_MULTICAST_NODE_MAP* > APPL_MULTICAST_MAP;
    APPL_MULTICAST_MAP* multicastMap;
    typedef APPL_MULTICAST_MAP::iterator APPL_MULTICAST_ADDR_ITER;
    typedef APPL_MULTICAST_NODE_MAP::iterator APPL_MULTICAST_NODE_ITER;
#endif
    void *ncAgent;

    // Pseudo Traffic Sender layer
    AppTrafficSender* appTrafficSender;

    void* msdpData;
};

#define APP_TIMER_SEND_PKT     (1)  /* for sending a packet */
#define APP_TIMER_UPDATE_TABLE (2)  /* for updating local tables */
#define APP_TIMER_CLOSE_SESS   (3)  /* for closing a session */
#define APP_TIMER_DNS_SEND_PKT (4)  // for sending a packet  to DNS

/// Timer structure used by applications
struct AppTimer
{
    int type;        /* timer type */
    int connectionId;         /* which connection this timer is meant for */
    unsigned short sourcePort;         /* the port of the session this timer is meant for */
    NodeAddress address;      /* address and port combination identify the session */
};

// STRUCT      :: AppGenericTimer
// DESCRIPTION :: Timer structure used by applications
// Added to support generic address for IPv6
struct AppGenericTimer
{
    int type;        /* timer type */
    int connectionId;         /* which connection this timer is meant for */
    unsigned short sourcePort;         /* the port of the session this timer is meant for */
    Address address;      /* address and port combination identify the session */
};

void
APP_Reset(Node *node, const NodeInput *nodeInput);

void APP_AddResetFunctionList(
        Node* node, void *param);
#define UDP_PORT_UNREACHABLE    0
#define TCP_PORT_UNREACHABLE    1

#ifdef EXATA
void
APP_InitiateIcmpMessage(Node *node, Message *msg, BOOL flag);
#endif

#ifdef CYBER_LIB
void 
APP_InitializeCyberApplication(Node* firstNode,
                               const char* cyberInput,
                               BOOL fromGUI);

/// \brief initialize the cyber application
///
/// \param firstNode pointer to the node
/// \param cyberInput Input command string
/// \param fromGUI Flag which indicates whether the command
/// is from GUI or not
/// \param apps Container in which information about this command's
/// after-effects will be saved
/// \param hid Unique Identifier for this HITL command
void 
APP_InitializeCyberApplication(Node* firstNode,
                               std::string& cyberInput,
                               BOOL fromGUI,
                               std::vector<HITLInfo>& apps,
                               const std::string& hid);
#endif // CYBER_LIB

// Dynamic address
// ENUM        :: AppAddressState
// Description :: enumerates the current state of the address of client
//                and server

typedef enum
{
    ADDRESS_INVALID = 0, // either client or server in invalid address state
    ADDRESS_FOUND,    // both client and server are having valid address
}AppAddressState;

//--------------------------------------------------------------------------
// FUNCTION    :: AppTcpValidServerAddressState
// PURPOSE     :: To check if TCP application server is in valid address
//                state to start listening
// PARAMETERS   ::
// + node             : Node*    : Pointer to node.
// + msg              : Message* : listen timer message
// + retServerAddress : Address* : valid server address
// RETURN       :
// bool         : true if in valid address state
//                false if in invalid address state
//--------------------------------------------------------------------------
bool
AppTcpValidServerAddressState(
                            Node* node,
                            Message* msg,
                            Address* retServerAddress);

//---------------------------------------------------------------------------
// FUNCTION              : AppStartTcpAppSessionServerListenTimer
// PURPOSE               : start the TCP application session server
//                         listening timer.
// PARAMETERS            ::
// + node                : Node*         : node
// + appType             : AppType       : applicationType
// + destAddr            : Address       : destination address
// + destString          : char*         : destination address string to
//                                         check if it is url
// + startTime           : clocktype     : start time of the application
// + serverDnsClientData : struct DnsClientData : dns client data at this node
// RETURN                : NONE
//--------------------------------------------------------------------------
void
AppStartTcpAppSessionServerListenTimer(
                    Node* node,
                    AppType appType,
                    Address destAddr,
                    char* destString,
                    clocktype startTime);

//---------------------------------------------------------------------------
// FUNCTION               : AppTcpGetSessionAddressState.
// PURPOSE                : get the current address sate of client and server
// PARAMETERS:
// + node                 : Node*         : pointer to the node
// + destNodeId           : NodeId        : destination node id for this app
//                                         session
// + clientInterfaceIndex : Int16         : client interface index for this
//                                          app session
// + destInterfaceIndex   : Int16         : destination interface index for
//                                          this app
// + appOpen              : AppToTcpOpen* : TCP Open info
// RETRUN:
// addressState         : AppAddressState :
//                        ADDRESS_FOUND : if both client and server
//                                        are having valid address
//                        ADDRESS_INVALID : if either of them are in
//                                          invalid address state
//---------------------------------------------------------------------------
AppAddressState
AppTcpGetSessionAddressState(
                            Node* node,
                            NodeId destNodeId,
                            Int16 clientInterfaceIndex,
                            Int16 destInterfaceIndex,
                            AppToTcpOpen* appOpen);

//---------------------------------------------------------------------------
// FUNCTION              : AppUpdateUrlServerNodeIdAndAddress
// PURPOSE               : updates the server address and node if URl is
//                         specified. This will be used for parallel case
//                         when client and server are in different parition
//                         and are not initialized sequentially
// PARAMETERS            ::
// + node                : Node*         : node
// + destString          : char*         : destination address string to
// + destAddr            : Address*      : destination address to be updated
// + destNodeId          : NodeId*       : destination node id to be updated
// RETURN                : NONE
//--------------------------------------------------------------------------
void
AppUpdateUrlServerNodeIdAndAddress(
                    Node* node,
                    char* destString,
                    Address* destAddr,
                    NodeId* destNodeId);

#ifndef ERROR_SmartReportError
#define ERROR_SmartReportError(msg, shouldAbort) \
    ERROR_WriteError(shouldAbort ? ERROR_ERROR : ERROR_WARNING, \
            (const char*) 0, msg, __FILE__, __LINE__);
#endif // ERROR_SmartReportError

#endif /* _APPLICATION_H_ */
