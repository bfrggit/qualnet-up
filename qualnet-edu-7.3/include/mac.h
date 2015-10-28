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

/// \defgroup Package_MAC_LAYER MAC LAYER

/// \file
/// \ingroup Package_MAC_LAYER
/// This file describes data structures and functions used by the MAC Layer.

#ifndef MAC_H
#define MAC_H

#include <assert.h>

#include "network.h"
#include "phy.h"
#include "list.h"
#include "random.h"
#include "trace.h"
#include "stats_mac.h"

#ifdef ADDON_DB
class StatsDBMacEventParam;
class StatsDBMacAggregate;
class StatsDBMacSummary;
struct StatDBLinkUtilizationFrameDescriptorPerNode;
typedef vector<StatDBLinkUtilizationFrameDescriptorPerNode> vectorFrameDesp;
#endif
class STAT_MacStatistics;

#define DEBUG_MACErr

/// Peer to Peer Propogation delay in the MAC
#define MAC_PROPAGATION_DELAY         (1 * MICRO_SECOND)

/// MAC address length
#define MAC_ADDRESS_LENGTH_IN_BYTE  6

/// Maximum MAC address length
#define MAX_MACADDRESS_LENGTH 16

/// MAC address length in byte or octets
#define  MAC_ADDRESS_DEFAULT_LENGTH 6

/// Number of
/// attribute of mac address file
#define  MAC_CONFIGURATION_ATTRIBUTE   5

/// From KA9Q NET/ROM pseudo
/// Hardware type.
#define HW_TYPE_NETROM              0

/// Ethernet 10/100Mbps
/// Hardware type Ethernet.
#define HW_TYPE_ETHER               1

/// Hardware type Experimental Ethernet
#define ARP_HRD_EETHER   2

/// Hardware type AX.25 Level 2
#define HW_TYPE_AX25     3

/// Hardware type PROnet token ring
#define HW_TYPE_PRONET   4

/// Hardware type Chaosnet
#define HW_TYPE_CHAOS    5

/// IEEE 802.2 Ethernet/TR/TB
#define HW_TYPE_IEEE802  6

/// Hardware type ARCnet
#define HW_TYPE_ARCNET      7

/// Hardware type APPLEtalk
#define HW_TYPE_APPLETLK    8

/// Frame Relay DLCI
#define HW_TYPE_DLCI        15

/// ATM 10/100Mbps
#define HW_TYPE_ATM         19

/// Hardware type HW_TYPE_METRICOM
#define HW_TYPE_METRICOM     23

/// Hardware type IEEE_1394
#define HW_TYPE_IEEE_1394    24

/// Hardware identifier
#define HW_TYPE_EUI_64       27

/// Unknown Hardware type
/// MAC protocol HARDWARE identifiers.
#define HW_TYPE_UNKNOWN     0xffff

/// Length of 4 byte MacAddress
#define MAC_IPV4_LINKADDRESS_LENGTH 4

/// Length of 2 byte MacAddress
#define MAC_NODEID_LINKADDRESS_LENGTH 2

/// Hardware identifier
#define IPV4_LINKADDRESS 28

/// Hardware identifier
#define HW_NODE_ID  29

enum MacDBEventType
{
    MAC_SendToPhy,
    MAC_ReceiveFromPhy,
    MAC_Drop,
    MAC_SendToUpper,
    MAC_ReceiveFromUpper
};

enum MacDBFailureType
{
    MAC_NoneDrop,
    MAC_ExceedRetraxCnt,
    MAC_InterfaceDisabled,
    MAC_UnknownSS,
    MAC_SSMovedOut,
    MAC_SSHandover,
    MAC_SyncLoss,
    MAC_UnkownDrop,
    MAC_DoNotReport
};

/// MAC hardware address of variable length
struct MacHWAddress
{
    unsigned char* byte;
    unsigned short hwLength;
    unsigned short hwType;

    MacHWAddress()
    {
        byte = NULL;
        hwLength = 0;
        hwType = HW_TYPE_UNKNOWN;
    }

    MacHWAddress(const MacHWAddress &srcAddr)
    {
        hwType = srcAddr.hwType;
        hwLength = srcAddr.hwLength;

        if (hwLength == 0)
        {
            byte = NULL;
        }
        else
        {
            byte = (unsigned char*) MEM_malloc(hwLength);
            memcpy(byte, srcAddr.byte, hwLength);
        }
    }

    inline MacHWAddress& operator =(const MacHWAddress& srcAddr)
    {

        if (!(hwLength == 0 ||
                srcAddr.hwLength == 0 ||
                hwLength == srcAddr.hwLength))
        {
            ERROR_ReportError("Incompatible Mac Address");
        }
        if (&srcAddr != this)
        {
        hwType = srcAddr.hwType;
        hwLength = srcAddr.hwLength;

        if (hwLength == 0)
        {
            if (byte != NULL)
            {
                MEM_free(byte);
                byte = NULL;
            }
        }
        else
        {
            if (byte == NULL)
            {
                byte = (unsigned char*) MEM_malloc(srcAddr.hwLength);
            }
            memcpy(byte, srcAddr.byte, hwLength);
            }
        }
        return *this;
    }

    inline BOOL operator ==(const MacHWAddress& macAddr)
    {
        if (hwType == macAddr.hwType &&
           hwLength == macAddr.hwLength &&
           memcmp(byte, macAddr.byte,hwLength) == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    ~MacHWAddress()
    {
        if (byte != NULL)
        {
            MEM_free(byte);
            byte = NULL;
        }
    }

};

/// INVALID MAC ADDRESS
#define INVALID_MAC_ADDRESS  MacHWAddress()



/// MAC address of size MAC_ADDRESS_LENGTH_IN_BYTE.
/// It is default Mac address of type 802
struct Mac802Address
{
    unsigned char byte[MAC_ADDRESS_LENGTH_IN_BYTE];

    Mac802Address()
    {
        memcpy(byte, INVALID_802ADDRESS, MAC_ADDRESS_LENGTH_IN_BYTE);
    }

    Mac802Address(const Mac802Address &srcAddr)
    {

        memcpy(byte, srcAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
    }

    Mac802Address(const char* srcAddr)
    {

        memcpy(byte, srcAddr, MAC_ADDRESS_LENGTH_IN_BYTE);
    }


    inline Mac802Address& operator =(const Mac802Address& srcAddr)
    {
        memcpy(byte, srcAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE);
        return *this;
    }

    inline Mac802Address& operator =(const char* srcAddr)
    {
        memcpy(byte, srcAddr, MAC_ADDRESS_LENGTH_IN_BYTE);
        return *this;
    }

    inline bool operator ==(const Mac802Address& macAddr) const
    {
        if (memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    inline BOOL operator ==(const char*  macAddr) const
    {
        if (memcmp(byte, macAddr, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    inline bool operator !=(const Mac802Address& macAddr) const
    {
        if (memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return FALSE;
        }
        return TRUE;
    }

    inline BOOL operator !=(const char*  macAddr) const
    {
        if (memcmp(byte, macAddr, MAC_ADDRESS_LENGTH_IN_BYTE) == 0)
        {
            return FALSE;
        }
        return TRUE;
    }

    inline bool operator > (const Mac802Address& macAddr) const
    {
        if (memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) > 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    inline bool operator < (const Mac802Address& macAddr) const
    {
        if (memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) < 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    inline bool operator <= (const Mac802Address& macAddr) const
    {
        if (memcmp(byte, macAddr.byte, MAC_ADDRESS_LENGTH_IN_BYTE) <= 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    inline unsigned int hash(int hashSize)
    {
        unsigned int byteSum = byte[0] + byte[1] +
            byte[2] + byte[3] + byte[4] + byte[5];
        return byteSum % hashSize;
    }

};


/// Describes one out of two possible states
/// of MAC interface - enable or disable
enum MacInterfaceState
{
    MAC_INTERFACE_DISABLE,
    MAC_INTERFACE_ENABLE
};


/// Describes different link type
enum MacLinkType
{
    WIRED = 0,
    WIRELESS = 1,
    MICROWAVE = 2,
    SATELLITE = 3
};

/// Callback funtion to report interface status
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param state  Wheather it enable or disable
typedef void (* MacReportInterfaceStatus)(
    Node *node,
    int interfaceIndex,
    MacInterfaceState state);

/// Set the MAC interface handler function to be called
/// when interface faults occurs
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param statusHandler  Pointer to status Handler
void
MAC_SetInterfaceStatusHandlerFunction(Node *node,
                                     int interfaceIndex,
                                     MacReportInterfaceStatus statusHandler);

/// To get the MACInterface status handling function
/// for the system
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
///
/// \return Pointer to status handler
MacReportInterfaceStatus
MAC_GetInterfaceStatusHandlerFunction(Node *node, int interfaceIndex);


/// Callback funtion for sending packet. It calls when network
/// layer has packet to send.
///
/// \param node  Pointer to node
/// \param interfaceIndex  index of interface
///
typedef void (* MacHasFrameToSendFn)(
    Node *node,
    int interfaceIndex);


/// Callback funtion to receive packet.
///
/// \param node  Pointer to node
/// \param interfaceIndex  index of interface
/// \param msg  Pointer to the message
///
typedef void (* MacReceiveFrameFn)(
    Node *node,
    int interfaceIndex,
    Message *msg);


/// Default VLAN TAGGING Value for a STATION node
#define STATION_VLAN_TAGGING_DEFAULT FALSE

/// Structure of VLAN in MAC sublayer
struct MacVlan
{
    unsigned short vlanId;
    BOOL sendTagged;
};

/// Structure of MAC sublayer VLAN header
struct MacHeaderVlanTag
{
    unsigned int tagProtocolId:16,
                 userPriority:3,
                 canonicalFormatIndicator:1,
                 vlanIdentifier:12;
};

/// Specifies different MAC_PROTOCOLs used
enum MAC_PROTOCOL
{
    MAC_PROTOCOL_MPLS = 1,
    MAC_PROTOCOL_CSMA = 2,
    MAC_PROTOCOL_FCSC_CSMA = 3,
    MAC_PROTOCOL_MACA = 4,
    MAC_PROTOCOL_FAMA = 5,
    MAC_PROTOCOL_802_11 = 6,
    MAC_PROTOCOL_802_3 = 7,
    MAC_PROTOCOL_DAWN = 8,
    MAC_PROTOCOL_LINK = 9,
    MAC_PROTOCOL_ALOHA = 10,
    MAC_PROTOCOL_GENERICMAC = 11,
    MAC_PROTOCOL_SWITCHED_ETHERNET = 12,
    MAC_PROTOCOL_TDMA = 13,
    MAC_PROTOCOL_GSM = 14,
    MAC_PROTOCOL_TADIL_LINK11 = 16,
    MAC_PROTOCOL_TADIL_LINK16 = 17,
    MAC_PROTOCOL_ALE = 18,
    MAC_PROTOCOL_SATTSM = 19,
    MAC_PROTOCOL_SATCOM = 20,
    MAC_PROTOCOL_SATELLITE_BENTPIPE = 22,
    MAC_SWITCH = 23,
    MAC_PROTOCOL_GARP = 24,
    MAC_PROTOCOL_DOT11 = 25,
    MAC_PROTOCOL_DOT16 = 26,
    MAC_PROTOCOL_ABSTRACT = 27,
    MAC_PROTOCOL_CELLULAR = 28,
    MAC_PROTOCOL_ANE = 29,
    MAC_PROTOCOL_WORMHOLE = 30,
    MAC_PROTOCOL_ANODR = 31,
    MAC_PROTOCOL_802_15_4 = 32,

    // Stats DB
    MAC_STATSDB_AGGREGATE = 57,
    MAC_STATSDB_SUMMARY = 58,

    MAC_PROTOCOL_LTE = 59,

    MAC_PROTOCOL_NONE // this must be the last one
};

//--------------------------------------------------------------------------
// STRUCT      :: AddressResolutionModule
// DESCRIPTION :: Structure stored in MacData where data structures related to
//                Address Resolution Protocols like ARP, RARP, etc are stored
//--------------------------------------------------------------------------


// Mac Reset functions defined to support NMS.
typedef void (*MacSetFunctionPtr)(Node*, int);
// Set rules defined
struct MacResetFunction
{
    // Corresponding set function
    MacSetFunctionPtr funcPtr;

    // the next match command
    MacResetFunction* next;
};

struct MacResetFunctionList
{
    MacResetFunction* first;
    MacResetFunction* last;
};

#define MAX_PHYS_PER_MAC 2

/// A composite structure representing MAC sublayer
/// which is typedefed to MacData in main.h
struct MacData
{
    MAC_PROTOCOL macProtocol;
    D_UInt32     macProtocolDynamic;
    int          interfaceIndex;
    BOOL         macStats;
    BOOL         promiscuousMode;
    //Added 2 more variables to support a. promiscuous mode with HLA i.e.
    //to check if the eavesdropped packets are to go out to the HLA node
    //and b. promiscuous mode with header switch i.e. to switch the dest address of the
    //outgoing eavesdropped packet to that of the eavesdropping node's physical address
    //this will mainly help for displaying video on the eavesdropped node during demo
    BOOL         promiscuousModeWithHeaderSwitch;
    BOOL         promiscuousModeWithHLA;

    Int64        bandwidth;   // In bytes.
    clocktype    propDelay;
    BOOL         interfaceIsEnabled;

    int          phyNumberArray[MAX_PHYS_PER_MAC];
    int          phyNumber;

    // Modifications
    // Ignored

    int getPhyNum(int idx = 0)
    {
      assert(idx >= 0 && idx < MAX_PHYS_PER_MAC);
      assert(phyNumberArray[idx] >= 0);

      return phyNumberArray[idx];
    }

    void setPhyNum(int value, int idx = 0)
    {
      assert(value >= 0);
      assert(idx >= 0 && idx < MAX_PHYS_PER_MAC);
      phyNumberArray[idx] = value;

      if (idx == 0)
      {
        phyNumber = value;
      }
    }

    void         *macVar;

    void         *mplsVar;

    MacHasFrameToSendFn sendFrameFn;
    MacReceiveFrameFn receiveFrameFn;

    LinkedList  *interfaceStatusHandlerList;
    NodeAddress virtualMacAddress;
    MacVlan* vlan;

    void*       bgMainStruct; //ptr of background traffic main structure
    void*       randFault; //ptr of the random link fault data structure
    short       faultCount; //flag for link fault.

    MacHWAddress                macHWAddr;
    BOOL                        isLLCEnabled;
    BOOL                        interfaceCardFailed;

#ifdef CYBER_LIB
    void         *encryptionVar;

    // This MAC interface is a wormhole eavesdropper/replayer.
    // This per-interface flag is added in particular for multi-channel
    // wormhole because of the channel-interface correspondence in QualNet.
    // Packets could be replayed in all physical channels in case
    // the victim network is using multiple channels
    BOOL        isWormhole;
#endif // CYBER_LIB

    // For Mibs
    D_UInt32 ifInOctets;
    D_UInt32 ifOutOctets;
    D_UInt32 ifOutErrors;
    D_UInt32 ifInErrors;
    D_UInt64 ifHCInOctets;
    D_UInt64 ifHCOutOctets;
    D_UInt32 ifOperStatus;

    //subnet index for subnet list data structure
    //this way you can find which subnet list this interface belongs to
    short subnetIndex;

#ifdef ADDON_NGCNMS
    MacResetFunctionList *resetFunctionList;
#endif
    STAT_MacStatistics* stats;
// Stats for MAC Aggregate Tables
#ifdef ADDON_DB
    StatsDBMacAggregate* aggregateStats;
    StatsDBMacSummary*  summaryStats;
    vectorFrameDesp* statDBGULBuffer;
#endif
#ifdef EXATA
    BOOL isIpneInterface;
    BOOL isVirtualLan;
    BOOL isReplayInterface;
#endif


#ifdef NETSNMP_INTERFACE
    int interfaceState;
    clocktype interfaceUpTime;
    clocktype interfaceDownTime;
#endif
};

#ifdef CYBER_LIB
/// Specifies different MAC_SECURITY_PROTOCOLs used
enum MAC_SECURITY
{
    MAC_SECURITY_WEP = 1,
    MAC_SECURITY_CCMP
};
#endif // CYBER_LIB


// MACROs for enabling and disabling and toggling between enabled
// and disabled interfaces.

/// Enable the MAC_interface
#define MAC_EnableInterface(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled = TRUE)

/// Disable the MAC_interface
#define MAC_DisableInterface(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled = FALSE)

/// Toggle the MAC_interface status
#define MAC_ToggleInterfaceStatus(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled ? \
         node->macData[interfaceIndex]->interfaceIsEnabled = FALSE : \
         node->macData[interfaceIndex]->interfaceIsEnabled = TRUE )

/// To query MAC_interface status is enabled or not
#define MAC_InterfaceIsEnabled(node, interfaceIndex) \
        (node->macData[interfaceIndex]->interfaceIsEnabled)

/// Handles packets from the network layer when the
/// network queue is empty
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
void
MAC_NetworkLayerHasPacketToSend(
    Node *node,
    int interfaceIndex);

/// To inform MAC that the Switch has packets to
/// to send
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
void MAC_SwitchHasPacketToSend(
    Node* node,
    int interfaceIndex);

/// Handles packets received from physical layer
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param packet  Pointer to Message
void
MAC_ReceivePacketFromPhy(
    Node *node,
    int interfaceIndex,
    Message *packet,
    int phyNum = 0);

/// Type of management request message
enum ManagementRequestType {
    ManagementRequestUnspecified = 1,
    ManagementRequestEcho,
    ManagementRequestSetGroupMembership,
    ManagementRequestSetNewRap,
    ManagementRequestSetBandwidthLimit,

    // NEW management request for MI
    ManagementRequestRequestBandwidth,
    ManagementRequestSetNeighborhood,
    ManagementRequestSetReservationMappingTable
};

/// data structure of management request
struct ManagementRequest {
    ManagementRequestType type;
    void *data;
};

/// Type of management response message
enum ManagementResponseType {
    ManagementResponseOK = 1,
    ManagementResponseUnsupported,
    ManagementResponseIllformedRequest,
    ManagementResponseUnspecifiedFailure
};

/// data structure of management response
struct ManagementResponse {
    ManagementResponseType result;
    void *data;
};

/// Deliver a network management request to the MAC
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param req  Pointer to a management request
/// \param resp  Pointer to a management response
void
MAC_ManagementRequest(Node *node, int interfaceIndex,
                      ManagementRequest *req, ManagementResponse *resp);

/// Handles status changes received from the physical layer
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param oldPhyStatus  Old status of physical layer
/// \param newPhyStatus  New status of physical layer
/// \param receiveDuration  Duration after which received
/// \param potentialIncomingPacket  Pointer to incoming message
void
MAC_ReceivePhyStatusChangeNotification(
    Node *node,
    int interfaceIndex,
    PhyStatusType oldPhyStatus,
    PhyStatusType newPhyStatus,
    clocktype receiveDuration,
    const Message *potentialIncomingPacket,
    int phyNum = 0);

/// Specifies the MAC to Physical layer delay
/// information structure
struct MacToPhyPacketDelayInfoType
{
    clocktype theDelayUntilAirborn;
};

/// Initialisation function for the User MAC_protocol
///
/// \param node  Pointer to a network node
/// \param nodeInput  Configured Inputs for the node
/// \param macProtocolName  MAC protocol name
/// \param interfaceIndex  interface index
void
MAC_InitUserMacProtocol(
    Node* node,
    const NodeInput*nodeInput,
    const char* macProtocolName,
    int interfaceIndex);

/// Finalization function for the User MAC_protocol
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
void
MacFinalizeUserMacProtocol(Node*node, int interfaceIndex);

/// Handles the MAC protocol event
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
/// \param packet  Pointer to Message
void
MAC_HandleUserMacProtocolEvent(
    Node *node,
    int interfaceIndex,
    Message *packet);

/// To check if Output queue for an interface of
/// a node if empty or not
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  index of interface
///
/// \return empty or not
BOOL
MAC_OutputQueueIsEmpty(Node *node, int interfaceIndex);

/// To notify MAC of packet drop
///
/// \param node  Pointer to a network node
/// \param nextHopAddress  Node address
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
void
MAC_NotificationOfPacketDrop(
    Node *node,
    NodeAddress nextHopAddress,
    int interfaceIndex,
    Message *msg);

/// To notify MAC of packet drop
///
/// \param node  Pointer to a network node
/// \param nextHopAddress  Node address
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
void
MAC_NotificationOfPacketDrop(
    Node *node,
    MacHWAddress nextHopAddress,
    int interfaceIndex,
    Message *msg);

/// To notify MAC of packet drop
///
/// \param node  Pointer to a network node
/// \param nextHopAddress  mac address
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
void
MAC_NotificationOfPacketDrop(
    Node *node,
    Mac802Address nextHopAddress,
    int interfaceIndex,
    Message *msg);

/// To notify MAC of priority packet arrival
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop address
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress);

/// To notify MAC of priority packet arrival
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop mac address
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    Mac802Address *destMacAddr);

/// To notify MAC of priority packet arrival
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop mac address
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr);


/// To remove the packet at the front of the specified priority
/// output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop address
/// \param networkType  network type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType);


/// To remove the packet at the front of the specified
/// priority output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop mac address
/// \param networkType  network type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType);

/// To remove the packet at the front of the specified
/// priority output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Message Priority
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop mac address
/// \param networkType  network type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType priority,
    Message** msg,
    Mac802Address* destMacAddr,
    int* networkType);

/// To allow a peek by network layer at packet
/// before processing
/// It is overloading function used for ARP packet
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  tos value
/// \param msg  Pointer to Message
/// \param destMacAddr  Dest addr Pointer
/// \param networkType  Network Type pointer
/// \param packType  packet Type pointer
///
/// \return If success TRUE
/// NOTE         : Overloaded MAC_OutputQueueDequeuePacketForAPriority()
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node* node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    MacHWAddress* destMacAddr,
    int* networkType,
    int* packType);

/// To allow a peek by network layer at packet
/// before processing
/// It is overloading function used for ARP packet
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  tos value
/// \param msg  Pointer to Message
/// \param destMacAddr  Dest addr Pointer
/// \param networkType  Network Type pointer
/// \param packType  packet Type pointer
///
/// \return If success TRUE
/// NOTE         : Overloaded MAC_OutputQueueDequeuePacketForAPriority()
BOOL MAC_OutputQueueDequeuePacketForAPriority(
    Node *node,
    int interfaceIndex,
    TosType* priority,
    Message** msg,
    NodeAddress *nextHopAddress,
    Mac802Address* destMacAddr,
    int* networkType,
    int* packetType);


/// To allow a peek by network layer at
/// packet before processing
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param prevHop  Previous Node address
/// \param destAddr  Destination Node address
/// \param messageType  Distinguish between the ARP and general message
///
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress prevHop,
    NodeAddress destAddr,
    int messageType = PROTOCOL_TYPE_IP);

/// To allow a peek by network layer at
/// packet before processing
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param prevHop  Previous Node mac address
/// \param destAddr  Destination Node mac address
/// \param arpMessageType  Distinguish between the ARP and general message
///
void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress& prevHop,
    MacHWAddress& destAddr,
    int messageType = PROTOCOL_TYPE_IP);

/// To allow a peek by network layer at
/// packet before processing
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param prevHop  Previous Node address
/// \param destAddr  Destination Node address
/// \param messageType  Distinguish between the ARP and general message
///

void
MAC_SneakPeekAtMacPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address prevHop,
    Mac802Address destAddr,
    int messageType = PROTOCOL_TYPE_IP);

/// To send acknowledgement from MAC
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHop  Pointer to Node address
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    NodeAddress nextHop);

/// To send acknowledgement from MAC
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHop  Pointer to Node address
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    MacHWAddress& nextHop);

/// To send acknowledgement from MAC
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHop  Pointer to nexthop mac address
void MAC_MacLayerAcknowledgement(
    Node* node,
    int interfaceIndex,
    const Message* msg,
    Mac802Address& nextHop);

/// Pass packet successfully up to the network layer
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param lastHopAddress  Node address
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    NodeAddress lastHopAddress);


/// Pass packet successfully up to the network layer
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param lastHopAddr  mac address


void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* lastHopAddr);

/// Pass packet successfully up to the network layer
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param lastHopAddr  mac address
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr);


/// Pass packet successfully up to the network layer
/// It is overloading function used for ARP packet
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param lastHopAddress  mac address
/// \param arpMessageType  Distinguish between ARP and general message
///
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    MacHWAddress* macAddr,
    int arpMessageType);

/// Pass packet successfully up to the network layer
/// It is overloading function used for ARP packet
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param lastHopAddress  mac address
/// \param arpMessageType  Distinguish between ARP and general message
///
void
MAC_HandOffSuccessfullyReceivedPacket(
    Node *node,
    int interfaceIndex,
    Message *msg,
    Mac802Address* lastHopAddr,
    int messageType);

/// To check packet at the top of output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop address
/// \param networkType  network type
/// \param priority  Message Priority
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int* networkType,
    TosType *priority);

/// To check packet at the top of output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop address
/// \param networkType  network type
/// \param priority  Message Priority
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *destHWAddr,
    int* networkType,
    TosType *priority);

/// To check packet at the top of output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Next hop address
/// \param networkType  network type
/// \param priority  Message Priority
///
/// \return TRUE if there is a packet, FALSE otherwise.
BOOL MAC_OutputQueueTopPacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destMacAddr,
    int* networkType,
    TosType *priority);

/// To remove packet from front of output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Pointer to Node address
/// \param networkType  network type
/// \param priority  Pointer to queuing priority type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType,
    TosType *priority);

/// To remove packet from front of output queue
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Pointer to Mac address
/// \param networkType  network type
/// \param priority  Pointer to queuing priority type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.
BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    MacHWAddress *destHWAddr,
    int *networkType,
    TosType *priority);

/// To remove packet from front of output queue,
/// Its a overloaded function
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Pointer to MacAddress address
/// \param networkType  network type
/// \param priority  Pointer to queuing priority type
///
/// \return TRUE if dequeued successfully, FALSE otherwise.

BOOL MAC_OutputQueueDequeuePacket(
    Node *node,
    int interfaceIndex,
    Message **msg,
    Mac802Address* destmacAddr,
    int *networkType,
    TosType *priority);

typedef enum
{
    MAC_DEQUEUE_OPTION_NONE = 0,
    MAC_DEQUEUE_OPTION_PACK,
    // MAC_DEQUEUE_OPTION_FRAG,
    // MAC_DEQUEUE_OPTION_COMPRESS
    // more
}MacOutputQueueDequeueOption;

typedef enum
{
    MAC_DEQUEUE_PACK_SAME_NEXT_HOP = 0, // both unicast and multicast
    MAC_DEQUEUE_PACK_ANY_PACKET = 1,
    // MAC_DEQUEUE_PACK_SAME_NEXT_HOP_SAME_CAST // only the same cast-type
    // MAC_DEQUEUE_PACK_SAME_DEST,
    // MAC_DEQUEUE_PACK_SAME_NEXT_HOP_SAME_PRIORITY,
    // MAC_DEQUEUE_PACK_SAME_DEST_SAME_PRIORITY
    // more
}MacOutputQueueDequeueCriteria;

// --------------------------------------------------------------------------
/// To remove packet(s) from front of output queue;
/// process packets with options
/// for example, pakcing multiple packets with
/// same next hop address together
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param msg  Pointer to Message
/// \param nextHopAddress  Pointer to Node address
/// \param networkType  network type
/// \param priority  Pointer to queuing priority type
/// \param dequeueOption  option
///    to dequeue output queue
/// \param dequeueCriteria  criteria
///    used when to retrive packets
/// \param numFreeByte  number of bytes can be packed in 1 transmission
/// \param numPacketPacked  number of packets packed
/// \param tracePrt  Trace Protocol Type
/// \param eachWithMacHeader  Each msg has its own MAC header?
///    if not, PHY/MAC header size should be
///    accounted before call this functions;
///    otherwise,only PHY header considered.
/// \param maxHeaderSize  max mac header size
/// \param returnPackedMsg  return Packed msg or a list of msgs
///
/// \return TRUE if dequeued successfully, FALSE otherwise.

BOOL MAC_OutputQueueDequeuePacket(
                Node* node,
                int interfaceIndex,
                Message **msg,
                NodeAddress* nextHopAddress,
                int* networkType,
                int* priority,
                MacOutputQueueDequeueOption dequeueOption,
                MacOutputQueueDequeueCriteria dequeueCriteria,
                int* numFreeByte,
                int* numPackedPackets,
                TraceProtocolType tracePrt,
                BOOL eachWithMacHeader = FALSE,
                int maxHeaderSize = 0,
                BOOL returnPackedMsg = TRUE);

// --------------------------------------------------------------------------
/// To remove packet(s) from front of output queue;
/// process packets with options
/// for example, pakcing multiple packets with
/// same next hop address together
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
/// \param priority  Pointer to queuing priority type
/// \param msg  Pointer to Message
/// \param nextHopAddress  Pointer to Node address
/// \param networkType  network type
/// \param dequeueOption  option
///    to dequeue output queue
/// \param dequeueCriteria  criteria
///    used when to retrive packets
/// \param numFreeByte  number of bytes can be packed in 1 transmission
/// \param numPacketPacked  number of packets packed
/// \param tracePrt  Trace Protocol Type
/// \param eachWithMacHeader  Each msg has its own MAC header?
///    if not, PHY/MAC header size should be
///    accounted before call this functions;
///    otherwise,only PHY header considered.
/// \param maxHeaderSize  max mac header size
/// \param returnPackedMsg  return Packed msg or a list of msgs
///
/// \return TRUE if dequeued successfully, FALSE otherwise.

BOOL MAC_OutputQueueDequeuePacketForAPriority(
                Node* node,
                int interfaceIndex,
                int priority,
                Message **msg,
                NodeAddress* nextHopAddress,
                int* networkType,
                MacOutputQueueDequeueOption dequeueOption,
                MacOutputQueueDequeueCriteria dequeueCriteria,
                int* numFreeByte,
                int* numPackedPackets,
                TraceProtocolType tracePrt,
                BOOL eachWithMacHeader = FALSE,
                int maxHeaderSize = 0,
                BOOL returnPackedMsg = TRUE);

/// Check if a packet (or frame) belongs to this node
/// Should be used only for four byte mac address
///
/// \param node  Pointer to a network node
/// \param destAddr  Destination Address
///
/// \return boolean
BOOL
MAC_IsMyUnicastFrame(Node *node, NodeAddress destAddr);

/// To check if an interface is a wired interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsWiredNetwork(Node *node, int interfaceIndex);

/// Checks if an interface belongs to Point to PointNetwork
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsPointToPointNetwork(Node *node, int interfaceIndex);

/// Checks if an interface belongs to Point to Multi-Point
/// network.
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsPointToMultiPointNetwork(Node *node, int interfaceIndex);

/// Determines if an interface is a wired broadcast interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsWiredBroadcastNetwork(Node *node, int interfaceIndex);

/// Determine if a node's interface is a wireless interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsWirelessNetwork(Node *node, int interfaceIndex);

/// Determine if a node's interface is a possible wireless
/// ad hoc interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsWirelessAdHocNetwork(Node *node, int interfaceIndex);

/// Determines if an interface is a single Hop
/// Broadcast interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interfaceIndex
///
/// \return boolean
BOOL
MAC_IsOneHopBroadcastNetwork(Node *node, int interfaceIndex);

/// To check if a node is a switch
///
/// \param node  Pointer to a network node
///
/// \return boolean
BOOL
MAC_IsASwitch(Node* node);

/// To set MAC address
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interface index
/// \param virtualMacAddress  MAC address
void MAC_SetVirtualMacAddress(Node *node,
                              int interfaceIndex,
                              NodeAddress virtualMacAddress);
/// Set Default interface Hardware Address of node
///
/// \param nodeId  Id of the input node
/// \param macAddr  Pointer to hardware structure
/// \param interfaceIndex  Interface on which the hardware address set
///
void MacSetDefaultHWAddress(
    NodeId nodeId,
    MacHWAddress* macAddr,
    int interfaceIndex);

/// To check if received mac address belongs to itself
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interface index
/// \param destAddr  dest address
///
/// \return Node Address
BOOL MAC_IsMyMacAddress(Node *node,
                        int interfaceIndex,
                        NodeAddress destAddr);


/// Checks for own MAC address.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param macAddr  Mac Address
// NOTE               :: Overloaded MAC_IsMyHWAddress()

BOOL MAC_IsMyHWAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr);

/// Validate MAC Address String after fetching from user
///
/// \param macAddrStr  Pointer to address string
/// \param macAddr  Pointer to hardware address structure
///
void MacValidateAndSetHWAddress(
    char* macAddrStr,
    MacHWAddress* macAddr);

/// Retrieve the IP Address  from Default HW Address .
/// Default HW address is equal to 6 bytes
///
/// \param node  Pointer to Node structure
/// \param macAddr  Pointer to hardware address structure
///
/// \return Ip address

NodeAddress DefaultMacHWAddressToIpv4Address(
    Node* node,
    MacHWAddress* macAddr);



/// Retrieve the Hardware Length.
///
/// \param node  Pointer to Node structure
/// \param interface  interface whose hardware length required
/// \param hwLength  Pointer to hardware string
///
void MacGetHardwareLength(
    Node* node,
    int interfaceIndex,
    unsigned short *hwLength);


/// Retrieve the Hardware Type.
///
/// \param node  Pointer to Node structure
/// \param interface  interface whose mac type requires
/// \param type  Pointer to hardware type
///
void MacGetHardwareType(
    Node* node,
    int interfaceIndex,
    unsigned short* type);

/// Retrieve the Hardware Address String.
///
/// \param node  Pointer to Node structure
/// \param interface  interface whose  hardware address retrieved
///    +macAddressString  : unsigned char* : Pointer to hardware string
///
void MacGetHardwareAddressString(
    Node* node,
    int interfaceIndex,
    unsigned char* macAddressString);

/// To add a new Interface at MAC
///
/// \param node  Pointer to a network node
/// \param interfaceAddress  interface IP add
/// \param numHostBits  No of host bits
/// \param interfaceIndex  interface index
/// \param nodeInput  node input
/// \param macProtocolName  Mac protocol of interface
void MacAddNewInterface(
    Node *node,
    NodeAddress interfaceAddress,
    int numHostBits,
    int *interfaceIndex,
    const NodeInput *nodeInput,
    const char *macProtocolName);

/// Init and read VLAN configuration from user input for
/// node and interface passed as arguments
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interface index
/// \param interfaceAddress  interface IP add
/// \param nodeInput  node input
void MacAddVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex,
    const Address* address,
    const NodeInput *nodeInput);

/// To flush VLAN info for an interface
///
/// \param node  Pointer to a network node
/// \param interfaceIndex  interface index
///
/// \return Node Address
void MacReleaseVlanInfoForThisInterface(
    Node* node,
    int interfaceIndex);



/// Checks Broadcast MAC address
///
/// \param macAddr  structure to hardware address
///
/// \return TRUE or FALSE
BOOL MAC_IsBroadcastHWAddress(
    MacHWAddress* macAddr);

/// Compares two MAC addresses
///
/// \param macAddr1  Pointer to harware address structure
/// \param macAddr2  Pointer to harware address structure
///
/// \return TRUE or FALSE
BOOL MAC_IsIdenticalHWAddress(
    MacHWAddress* macAddr1,
    MacHWAddress* macAddr2);

/// Prints interface Mac Address
///
/// \param macAddr  Mac address
/// ASSUMPTION         : Byte separator is always '-'
void MAC_PrintHWAddr(MacHWAddress* macAddr);

/// Prints interface Mac Address
///
/// \param macAddr  Mac address
/// ASSUMPTION         : Byte separator is always '-'
void MAC_PrintMacAddr(Mac802Address* macAddr);

#define RAND_TOKENSEP    " ,\t"

#define RAND_FAULT_GEN_SKIP   2


/// Describes different fault type
enum FaultType
{
    STATIC_FAULT,
    RANDOM_FAULT,
    // to identify the fault is given by using dynamic commands.
    EXTERNAL_FAULT
};

/// Fields for keeping track of interface faults
struct MacFaultInfo
{
    FaultType  faultType;
    MacHWAddress macHWAddress;
    BOOL interfaceCardFailed;
};


/// Structure containing random fault information.
struct RandFault
{
    int             repetition; //no. of interface fault
    clocktype       startTm;    //time to start first fault
    RandomDistribution<clocktype> randomUpTm;    //Up time distribution
    RandomDistribution<clocktype> randomDownTm;  //Down time distribution

    // Variables for statistics collection.
    int             numLinkfault;   // number of random link fault
    clocktype       totDuration;    // total time for random link fault
    int             totFramesDrop; //Total frames dropped due to fault
};


/// Initialization the Random Fault structure from input file
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param currentLine  pointer to the input string
///
void MAC_RandFaultInit(Node*       faultyNode,
                       int         interfaceIndex,
                       const char* currentLine);


/// IPrint the statistics of Random link fault.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
///
void MAC_RandFaultFinalize(Node* node, int interfaceIndex);

/// Returns the priority of the packet
///
/// \param msg  Node Pointer
///
/// \return priority
/// NOTE: DOT11e updates
TosType MAC_GetPacketsPriority(Node *, Message* msg);

/// Convert the Multicast ip address to multicast MAC address
///
/// \param multcastAddress  Multicast ip address
/// \param macMulticast  Pointer to mac hardware address
///
void MAC_TranslateMulticatIPv4AddressToMulticastMacAddress(
    NodeAddress multcastAddress,
    MacHWAddress* macMulticast);

/// Look at the packet at the index of the output queue.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopAddress  Next hop mac address
/// \param priority  priority
///
/// \return TRUE if the messeage found, FALSE otherwise
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    TosType *priority);

/// Look at the packet at the index of the output queue.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopAddress  Next hop mac address
/// \param priority  priority
///
/// \return TRUE if the messeage found, FALSE otherwise
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address *nextHopMacAddr,
    TosType *priority);

/// Look at the packet at the index of the output queue.
///
///    +node              : Node*       : Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopAddress  Next hop mac address
/// \param priority  priority
///
/// \return TRUE if the messeage found, FALSE otherwise
BOOL MAC_OutputQueuePeekByIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress *nextHopAddr,
    TosType *priority);

/// To remove the packet at specified index output queue.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopAddress  Next hop IP address
/// \param networkType  Type of network
///
/// \return TRUE if the messeage dequeued properly,
/// FALSE otherwise
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    NodeAddress *nextHopAddress,
    int *networkType);

/// To remove the packet at specified index output queue.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopMacAddress  Next hop mac address
/// \param networkType  Type of network
///
/// \return TRUE if the messeage dequeued properly,
/// FALSE otherwise
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    Mac802Address* nextHopMacAddr,
    int *networkType);

/// To remove the packet at specified index output queue.
///
/// \param node  Node pointer
/// \param interfaceIndex  Interface index
/// \param msgIndex  Message index
/// \param msg  Double pointer to message
/// \param nextHopMacAddress  Next hop mac address
/// \param networkType  Type of network
///
/// \return TRUE if the messeage dequeued properly,
/// FALSE otherwise
BOOL MAC_OutputQueueDequeuePacketWithIndex(
    Node *node,
    int interfaceIndex,
    int msgIndex,
    Message **msg,
    MacHWAddress* nextHopMacAddr,
    int *networkType);


/// Check the given address is Multicast address or not.
///
/// \param ipV4  ipV4 address
///
/// \return TRUE or FALSE
BOOL MAC_IPv4addressIsMulticastAddress(
    NodeAddress ipV4);


/// Checks Broadcast MAC address.
///
/// \param macAddr  Mac Address.

BOOL MAC_IsBroadcastMac802Address(Mac802Address *macAddr);

/// Retrieve the Mac802Address  from IP address.
///
/// \param node  Pointer to Node structure
/// \param index  Interface Index
/// \param ipv4Address  Ipv4 address from which the
///    hardware address resolved.
/// \param macAddr  Pointer to Mac802address structure
///

void IPv4AddressToDefaultMac802Address(
    Node *node,
    int index,
    NodeAddress ipv4Address,
    Mac802Address *macAddr);




/// Convert Variable Hardware address to Mac 802 addtess
///
/// \param node  Pointer to Node structure
/// \param macHWAddr  Pointer to hardware address structure
/// \param mac802Addr  Pointer to mac 802 address structure
BOOL ConvertVariableHWAddressTo802Address(
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr);


// FUNCTION   :: Convert802AddressToVariableHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert Mac 802 addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
void Convert802AddressToVariableHWAddress (
            Node *node,
            MacHWAddress* macHWAddr,
            Mac802Address* mac802Addr);

/// Copies Hardware address address
///
/// \param destAddr  structure to destination hardware address
/// \param srcAddr  structure to source hardware address
///

void MAC_CopyMacHWAddress(
            MacHWAddress* destAddr,
            MacHWAddress* srcAddr);


// FUNCTION   :: MAC_CopyMacHWAddress
// LAYER      :: MAC
// PURPOSE    :: Convert Mac 802 addtess to Variable Hardware address
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// + mac802Addr : Mac802Address* : Pointer to mac 802 address structure
// RETURN     :: Bool :
void MAC_CopyMac802Address (
            Mac802Address* destAddr,
            Mac802Address* srcAddr);


/// Retrieve  IP address from.Mac802Address
///
/// \param node  Pointer to Node structure
/// \param macAddr  Pointer to hardware address structure
///
/// \return Ipv4 Address
NodeAddress DefaultMac802AddressToIpv4Address (
            Node *node,
            Mac802Address* macAddr);

// FUNCTION   :: MAC_IsMyAddress
// LAYER      :: MAC
// PURPOSE    :: Check if mac address is of node.
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + interfaceIndex : int : Interface Index
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: Bool : True if mac address matches
BOOL MAC_IsMyAddress(
    Node* node,
    int interfaceIndex,
    MacHWAddress* macAddr);

// FUNCTION   :: MAC_IsMyAddress
// LAYER      :: MAC
// PURPOSE    :: Check if mac address is of node. Overloaded function
// PARAMETERS ::
// + node : Node* : Pointer to Node structure
// + macHWAddr : MacHWAddress* : Pointer to hardware address structure
// RETURN     :: Bool : True if mac address matches with any interface
BOOL MAC_IsMyAddress(
    Node* node,
    MacHWAddress* macAddr);



// FUNCTION   :: MAC_IsIdenticalMac802Address
// LAYER      :: MAC
// PURPOSE    :: Compares two MAC addresses
// PARAMETERS ::
// + macAddr1         : Mac802Address* macAddr1 : Mac Address
// + macAddr2         : Mac802Address* macAddr2 : Mac Address
// RETURN   :: Bool :
BOOL MAC_IsIdenticalMac802Address(
    Mac802Address *macAddr1,
    Mac802Address *macAddr2);

/// Converts IP address.To MacHWAddress
///
/// \param node  Pointer to Node structure
/// \param interfaceIndex  interfcae index of a node
/// \param msg  Message pointer
/// \param ipv4Address  Ipv4 address from which the
///    hardware address resolved.
///    + macAddr        : MacHWAddress* : Pointer to hardware address structure
///
/// \return Returns False when conversion fails
BOOL IPv4AddressToHWAddress(
    Node *node,
    int interfaceIndex,
    Message* msg,
    NodeAddress ipv4Address,
    MacHWAddress* macAddr);

/// This functions converts variable length Mac address
/// to IPv4 address It checks the type of hardware address
/// and based on that conversion is done.
///
/// \param node  Pointer to node which indicates the host
/// \param interfaceIndex  Interface index of a node
/// \param macAddr  Pointer to MacHWAddress Structure.
///    
///
/// \return IP address
NodeAddress MacHWAddressToIpv4Address(
                Node *node,
                int interfaceIndex,
                MacHWAddress* macAddr);

/// Convert one byte decimal number to hex number.
///
/// \param dec  decimal number
///
/// \return return correspondig hex digit string for
/// one byte decimal number
char* decToHex(int dec);

// PURPOSE:   ::  Convert 4 byte address to the variable hardware address
///
/// \param node  Pointer to node which indicates the host
/// \param interfaceIndex  Interface index of a node
/// \param macAddr  Pointer to source MacHWAddress Structure
/// \param nodeAddr  Ip address
void MAC_FourByteMacAddressToVariableHWAddress(
                    Node* node,
                    int interfaceIndex,
                    MacHWAddress* macAddr,
                    NodeAddress nodeAddr);

/// Retrieve  IP address from.MacHWAddress of type
/// IPV4_LINKADDRESS
///
/// \param node  Pointer to Node structure
/// \param macAddr  Pointer to hardware address structure
///
/// \return Ipv4 Address
NodeAddress MAC_VariableHWAddressToFourByteMacAddress (
    Node* node,
    MacHWAddress* macAddr);

/// Returns Broadcast Address of an interface
///
/// \param node  Pointer to a node
/// \param interfaceIndex  Interface of a node
///
/// \return Broadcast mac address of a interface
MacHWAddress GetBroadCastAddress(Node *node, int InterfaceIndex);

/// Returns MacHWAddress of an interface
///
/// \param node  Pointer to a node
/// \param interfaceIndex  inetrface of a node
///
/// \return Mac address of a interface
MacHWAddress GetMacHWAddress(Node* node, int interfaceIndex);

/// Returns interfaceIndex at which Macaddress is configured
///
/// \param node  Pointer to a node
/// \param macAddr  Mac Address of a node
///
/// \return interfaceIndex of node
int MacGetInterfaceIndexFromMacAddress(Node* node, MacHWAddress& macAddr);

/// Returns interfaceIndex at which Macaddress is configured
///
/// \param node  Pointer to a node
/// \param macAddr  Mac Address of a node
///
/// \return interfaceIndex of node
int MacGetInterfaceIndexFromMacAddress(Node* node, Mac802Address& macAddr);

/// Returns interfaceIndex at which Macaddress is configured
///
/// \param node  Pointer to a node
/// \param macAddr  Mac Address of a node
///
/// \return interfaceIndex of node
int MacGetInterfaceIndexFromMacAddress(Node* node, NodeAddress macAddr);


class D_Fault : public D_Object
{
    private:
        Node* m_Node;
        int m_InterfaceIndex;

    public:
        D_Fault(Node* node, int interfaceIndex) : D_Object(D_VARIABLE)
        {
            executable = TRUE;
            m_Node = node;
            m_InterfaceIndex = interfaceIndex;
        }

        void ExecuteAsString(const std::string& in, std::string& out);
};

/// Reset the Mac protocols use by the node
///
/// \param node  Pointer to the node
/// \param InterfaceIndex  interface index
void
MAC_Reset(Node* node, int interfaceIndex);

/// Add which protocols in the Mac layer to be reset to a
/// fuction list pointer.
///
/// \param node  Pointer to the node
/// \param InterfaceIndex  interface index
/// \param param  pointer to the protocols reset function
void
MAC_AddResetFunctionList(Node* node, int interfaceIndex, void *param);

//--------------------------------------------------------------------------
// FUNCTION :: MacSkipLLCandMPLSHeader
// PURPOSE ::  skips MPLS and LLC header if present.
// PARAMETERS ::
// + node : Node* : Pointer to a node
// + payload : char* : pointer to the data packet.
// RETURN  :: char* : Pointer to the data after skipping headers.
//--------------------------------------------------------------------------
char* MacSkipLLCandMPLSHeader(Node* node, char* payload);

#ifdef ADDON_DB
void HandleMacDBEvents(Node* node,
                       Message* msg,
                       int phyIndex,
                       int interfaceIndex,
                       MacDBEventType eventType,
                       MAC_PROTOCOL protocol,
                       BOOL failureTypeSpecifed = FALSE,
                       std::string failureType = "",
                       BOOL processMacHeader = TRUE);
void HandleMacDBSummary(
    Node *node, Message* msg, int interfaceIndex,
    MacDBEventType eventType, MAC_PROTOCOL protocol);
void HandleMacDBAggregate(
    Node *node, Message* msg, int interfaceIndex,
    MacDBEventType eventType, MAC_PROTOCOL protocol);
void HandleMacDBConnectivity(Node *node, int interfaceIndex,
    Message *msg, MacDBEventType eventType);
#endif

//--------------------------------------------------------------------------
//FUNCTION   :: IsSwitchCtrlPacket
//LAYER      :: MAC
//PURPOSE    :: Returns true if msg is control packet for a switch
//PARAMETERS :: Node* node
//                   Pointer to the node
//              Mac802Address 
//                  Destination MAC address
//RETURN     :: BOOL
//--------------------------------------------------------------------------
BOOL IsSwitchCtrlPacket(Node* node,
                        Mac802Address destAddr);

//--------------------------------------------------------------------------
// FUNCTION   :: GetNodeIdFromMacAddress
// LAYER      :: MAC
// PURPOSE    :: Return first four bytes of mac address
// PARAMETERS ::
// + byte[] : unsigned char : Mac address string
// RETURN     :: NodeId : unsigned int
//--------------------------------------------------------------------------
NodeId GetNodeIdFromMacAddress (unsigned char byte[]);
//---------------------------------------------------------------------------
//FUNCTION   :: Mac802AddressToStr
//LAYER      :: MAC
//PURPOSE    :: Convert addr to hex format e.g. [aa-bb-cc-dd-ee-ff]
//PARAMETERS ::
//+ addrStr   : char*         : Converted address string
//                              (This must be pre-allocated memory)
//+ addr      : Mac802Address*   : Pointer to MAC address
//RETURN     :: void
//---------------------------------------------------------------------------
void Mac802AddressToStr(char* addrStr,
                        const Mac802Address* macAddr);

#endif /* MAC_H */















