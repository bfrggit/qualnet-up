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

/*!
 * \file mac_dot11.h
 * \brief Qualnet structures and helper functions.
 */

#ifndef MAC_DOT11_H
#define MAC_DOT11_H

#include "api.h"
#include "partition.h"
#include "dvcs.h"
#include "mac_dot11-mib.h"
#include "dynamic.h"
#include "ipv6.h"
#include "mac_dot11s-frames.h"
#include "mac_phy_802_11n.h"

#ifdef CYBER_LIB
#include "mac_security.h"
#endif // CYBER_LIB

struct DOT11s_Data;
class OutputBuffer;
class InputBuffer;

//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------
#define DEBUG_HCCA       0
#define DEBUG            0
#define DEBUG_STATION    0
#define DEBUG_AP         0
#define DEBUG_MANAGEMENT 0
#define DEBUG_BEACON     0
#define DEBUG_PCF        0
#define DEBUG_QA         0
//---------------------------Power-Save-Mode-Updates---------------------//
#define DEBUG_PS              0
#define DEBUG_PS_AP_BEACON    0
#define DEBUG_PS_IBSS_BEACON  0
#define DEBUG_PS_ATIM         0
#define DEBUG_PS_PSPOLL       0
#define DEBUG_PS_NAV          0
#define DEBUG_PS_BO           0
#define DEBUG_PS_CHANGE_STATE_ACTIVE_TO_DOZE    0
#define DEBUG_PS_CHANGE_STATE_DOZE_TO_ACTIVE    0
#define DEBUG_PS_TIMERS 0
//---------------------------Power-Save-Mode-End-Updates-----------------//

// Used to force remote nodes to yield longer for the data exchange.
// Also needed since when packets will arrive at the same time a timer
// times out.  Add delay to allow packets to be process first before
// timers get a chance to expire.  Therefore, this value to be at
// least 1.

#define EPSILON_DELAY 1


#define MAC_PHY_LOWEST_MANDATORY_RATE_TYPE 0

// FCS is accounted for in the header frame sizes
// (instead of after the data payload) for coding convenience.
#define DOT11_SHORT_CTRL_FRAME_SIZE         14
#define DOT11_LONG_CTRL_FRAME_SIZE          20

// Address4 field not considered.
#define DOT11_DATA_FRAME_HDR_SIZE           28

// Binary expon backoff lower bound and upper bound
#define DOT11_802_11a_CW_MIN          15
#define DOT11_802_11a_CW_MAX          1023
#define DOT11_802_11a_SLOT_TIME       (9 * MICRO_SECOND)
#define DOT11_802_11a_SIFS            (16 * MICRO_SECOND)
#define DOT11_802_11a_DELAY_UNTIL_SIGNAL_AIRBORN  (2 * MICRO_SECOND)

#define DOT11_802_11b_CW_MIN          31
#define DOT11_802_11b_CW_MAX          1023
#define DOT11_802_11b_SLOT_TIME       (20 * MICRO_SECOND)
#define DOT11_802_11b_SIFS            (10 * MICRO_SECOND)
#define DOT11_802_11b_DELAY_UNTIL_SIGNAL_AIRBORN  (5 * MICRO_SECOND)
#define DOT11_MANAGEMENT_ASSOC_RESPONSE_TIMEOUT (512 * MILLI_SECOND)
#define DOT11_MANAGEMENT_AUTH_RESPONSE_TIMEOUT (512 * MILLI_SECOND)
#define DOT11_MANAGEMENT_PROBE_RESPONSE_TIMEOUT (512 * MILLI_SECOND)
#define DOT11_MANAGEMENT_STATION_INACTIVE_TIMEOUT (300 * SECOND)

#define DOT11_PROPAGATION_DELAY  (1 * MICRO_SECOND)

// This is the minimum time that a packet will be in the air
// for the purpose of computing (parallel simulation) lookahead.
// It also can include the SIFS delay for the time it takes
// the ACK to get in the air (small).  This is the minimum delay
// that a blindsiding packet can jump on the channel and demand
// an ACK.
//    ONLY need for NULL message protocol with incomplete
//    topology. Otherwise we can lie on lookahead.
//
// #define DOT11_MIN_PACKET_RECEIVE_DURATION (???*MICRO_SECOND)


// If packet size (bytes) of network layer is larger than this threshold,
// RTS-CTS method is used.  0 means always use RTS-CTS method.  Broadcast
// packets NEVER use RTS-CTS method.
#define DOT11_RTS_THRESH          0

// Retransmission limits.
#define DOT11_SHORT_RETRY_LIMIT   7
#define DOT11_LONG_RETRY_LIMIT    4


// FRAG_THRESH defaults to 2346.  If packet size (bytes) network layer is
// smaller than this threshold, no fragmentation is used.  Broadcast
// packets are NEVER fragmented.
#define DOT11_FRAG_THRESH         2346
#define DOT11N_FRAG_THRESH        65535
#define DOT11AC_FRAG_THRESH       4692480
/*********HT START*************************************************/
#define DOT11N_AMSDU_THRESH         7935
/*********HT END*************************************************/

#define DOT11_MAX_NUM_MEASUREMNT  6
#define DOT11_MEASUREMENT_VALID_TIME  (3 * SECOND)
#define DOT11_HANDOVER_RSS_MARGIN 2.0
#define DOT11_MAX_DURATION (65535 * MICRO_SECOND)

// Masks used in frame headers in BSS.
#define DOT11_TO_DS                       0x80
#define DOT11_FROM_DS                     0x40
#ifdef CYBER_LIB
#define DOT11_PROTECTED_FRAME             0x02
#endif // CYBER_LIB
//---------------------------Power-Save-Mode-Updates---------------------//
#define DOT11_MORE_DATA_FIELD             0x20
#define DOT11_POWER_MGMT                  0x08

/*********HT START*************************************************/
#define DOT11_ORDER_FIELD 0x01
/*********HT END****************************************************/

#define DOT11_PS_MODE_DEFAULT_BROADCAST_QUEUE_SIZE\
        (1 * DEFAULT_APP_QUEUE_SIZE)
#define DOT11_PS_MODE_DEFAULT_UNICAST_QUEUE_SIZE\
        (1 * DEFAULT_APP_QUEUE_SIZE)
//---------------------------Power-Save-Mode-End-Updates-----------------//


// Constant duration for CF frames.
#define DOT11_CF_FRAME_DURATION           32768


// CF Parameter Set
// Total length is 8 as per standard, though field lengths differ.
#define DOT11_CF_PARAMETER_SET_ID         4
#define DOT11_CF_PARAMETER_SET_LENGTH     6

#define DOT11_TIME_UNIT                   (1024 * MICRO_SECOND)
#define DOT11_BEACON_INTERVAL_MAX         32767
#define DOT11_BEACON_INTERVAL_DEFAULT     200
#define DOT11_BEACON_START_DEFAULT        1
#define DOT11_BEACON_MISSED_MAX           3
#define DOT11_BEACON_JITTER               (512 * MICRO_SECOND)
#define DOT11_CFP_MAX_DURATION_DEFAULT    50
#define DOT11_CFP_REPETITION_INTERVAL_DEFAULT 1
#define DOT11_CFP_REPETITION_INTERVAL_MAX 10

#define DOT11_SHORT_SCAN_TIMER_DEFAULT     (6 * MILLI_SECOND)
#define DOT11_LONG_SCAN_TIMER_DEFAULT      (24 * MILLI_SECOND)

//---------------------------Power-Save-Mode-Updates---------------------//
// defalut listen interval for STAs in PS mode
#define DOT11_PS_LISTEN_INTERVAL          10
#define DOT11_DTIM_PERIOD                  3
#define DOT11_PS_IBSS_ATIM_DURATION       20
#define DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP 251
#define DOT11_MAX_BEACON_SIZE             1024
//---------------------------Power-Save-Mode-End-Updates-----------------//

#define DOT11_STATION_POLL_TYPE_DEFAULT   DOT11_STA_POLLABLE_POLL
#define DOT11_PC_DELIVERY_MODE_DEFAULT    DOT11_PC_POLL_AND_DELIVER
#define DOT11_STATION_TYPE_DEFAULT        DOT11_STA_IBSS
#define DOT11_POLL_SAVE_MODE_DEFAULT      DOT11_POLL_SAVE_BYCOUNT
#define DOT11_POLL_SAVE_MIN_COUNT_DEFAULT 1
#define DOT11_POLL_SAVE_MAX_COUNT_DEFAULT 10

#define DOT11_RELAY_FRAMES_DEFAULT        TRUE
#define DOT11_BROADCASTS_HAVE_PRIORITY_DURING_CFP  TRUE
#define DOT11_PRINT_STATS_DEFAULT         FALSE // TRUE
#define DOT11_PRINT_PC_STATS_DEFAULT      TRUE //
#define DOT11_TRACE_DEFAULT               DOT11_TRACE_NO

#define DOT11_MAC_STATS_LABEL\
        ((dot11->isVHTEnable)? ("802.11acMAC") : (dot11->isHTEnable?\
                ("802.11nMAC"):("802.11MAC")))

#define DOT11_DCF_STATS_LABEL\
        ((dot11->isVHTEnable)? ("802.11acDCF") : (dot11->isHTEnable?\
                        ("802.11nDCF"):("802.11DCF")))

//--------------------HCCA-Updates Start---------------------------------//
#define DOT11e_HC_DELIVERY_MODE_DEFAULT    DOT11e_HC_POLL_AND_DELIVER
#define DOT11e_HCCA_LOWEST_PRIORITY 4

#define DOT11e_HCCA_TSBUFFER_SIZE 1000

#define DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED (7 - \
                                            DOT11e_HCCA_LOWEST_PRIORITY + 1)
#define DOT11e_HCCA_MIN_TSID 8
#define DOT11e_HCCA_MAX_TSID (DOT11e_HCCA_MIN_TSID + \
                             DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED - 1)
#define DOT11e_HCCA_MIN_TXOP_DATAFRAME 100
//--------------------HCCA-Updates End-----------------------------------//
#define DOT11_QBSSLOAD_ELEMENT_ID 1
#define DOT11_EDCA_PARAMETER_SET_ID 1
#define DOT11_TSPEC_ID 13
//---------------------DOT11e--Updates------------------------------------//

/*********HT START*************************************************/
#define DOT11N_HT_CAPABILITIES_ELEMENT_ID 45
#define DOT11N_HT_OPERATION_ELEMENT_ID    61
#define DOT11N_HT_OPERATION_ELEMENT_LEN   22          //Total length minus length for ID and Length fields

#define DOT11N_VHT_CAPABILITIES_ELEMENT_ID 191
#define DOT11_VHT_OPERATION_ELEMENT_ID    192

/*********HT END****************************************************/


/// Utility macro for memory allocation.
#define Dot11_Malloc(type)                     \
        (type*) MEM_malloc(sizeof(type))


/// Statistic Label
#define DOT11e_MAC_STATS_LABEL           "802.11eMAC"

/// No of Access Category
#define DOT11e_NUMBER_OF_AC             4

/// Invalid Access Category index
#define DOT11e_INVALID_AC              -1

/// Background traffic Access Category index
#define DOT11e_AC_BK                    0

/// Best effort traffic Access Category index
#define DOT11e_AC_BE                    1

/// Video traffic Access Category index
#define DOT11e_AC_VI                    2

/// Voice traffic Access Category index
#define DOT11e_AC_VO                    3

//Acces Category related symbolic  constants operating on PHY 802.11a

/// Background traffic Arbitary InterFrame Sequence
#define DOT11e_802_11a_AC_BK_AIFSN      7

/// Best effort traffic Arbitary InterFrame Sequence
#define DOT11e_802_11a_AC_BE_AIFSN      3

/// Video traffic Arbitary InterFrame Sequence
#define DOT11e_802_11a_AC_VI_AIFSN      2

/// Voice traffic Arbitary InterFrame Sequence
#define DOT11e_802_11a_AC_VO_AIFSN      2

//Transmission Opportunity for each AC operating on PHY 802.11a

/// Background traffic Transmission Opportunity Limit
#define DOT11e_802_11a_AC_BK_TXOPLimit  0

/// Best effort Transmission Opportunity Limit
#define DOT11e_802_11a_AC_BE_TXOPLimit  0

/// Video traffic Transmission Opportunity Limit
#define DOT11e_802_11a_AC_VI_TXOPLimit  (3008  *  MICRO_SECOND)

/// Voice traffic Transmission Opportunity Limit
#define DOT11e_802_11a_AC_VO_TXOPLimit  (1504 * MICRO_SECOND)


//Acces Category related symbolic  constants operating on PHY 802.11b

/// Background traffic Arbitary InterFrame Sequence
#define DOT11e_802_11b_AC_BK_AIFSN      7

/// Best effort traffic Arbitary InterFrame Sequence
#define DOT11e_802_11b_AC_BE_AIFSN      3

/// Video traffic Arbitary InterFrame Sequence
#define DOT11e_802_11b_AC_VI_AIFSN      2

/// Voice traffic Arbitary InterFrame Sequence
#define DOT11e_802_11b_AC_VO_AIFSN      2

//Transmission Opportunity for each AC operating on PHY 802.11b

/// Background traffic Transmission Opportunity Limit
#define DOT11e_802_11b_AC_BK_TXOPLimit  0

/// Best effort Transmission Opportunity Limit
#define DOT11e_802_11b_AC_BE_TXOPLimit  0

/// Video traffic Transmission Opportunity Limit
#define DOT11e_802_11b_AC_VI_TXOPLimit  (6016 * MICRO_SECOND)

/// Voice traffic Transmission Opportunity Limit
#define DOT11e_802_11b_AC_VO_TXOPLimit  (3008 * MICRO_SECOND)


//--------------------DOT11e-End-Updates---------------------------------//

//--------------------------------------------------------------------------
// typedef's enums
//--------------------------------------------------------------------------

// Access Category States
enum AcState
{
    k_State_IDLE = 1,
    k_State_Contention
};

// CSMA Defferals States
enum BackoffState
{
    k_State_DIFS = 1,
    k_State_BO
};

// Ac Manager Modes
enum AcMode
{
    k_Mode_Legacy = 1,
    k_Mode_Dot11e,
    k_Mode_Dot11n,
    k_Mode_Dot11ac
};

// MAC states
typedef enum {
    DOT11_S_IDLE,                 // 0
    DOT11_S_WFNAV,                // 1
    DOT11_S_WF_DIFS_OR_EIFS,      // 2
    // Note: Same DOT11_S_WF_DIFS_OR_EIFS is used to minimize the coding
    // effect in Dot11e. However the delay is replaced by AIFS for DOT11e
    DOT11_S_BO,                   // 3
    DOT11_S_NAV_RTS_CHECK_MODE,   // 4
    DOT11_S_HEADER_CHECK_MODE,    // 5
    DOT11_S_WFJOIN,               // 6

    // Waiting For Response States
    // Make consistent with MacDot11InWaitingForResponseState()

    DOT11_S_WFCTS,                // 7 First State in range
    DOT11_S_WFDATA,               // 8
    DOT11_S_WFACK,                // 9
    DOT11_S_WFMANAGEMENT,         // 10
    DOT11_S_WFBEACON,             // 11 Last State in range

    // Transmission States:
    // Make consistent with MacDot11InTransmittingState().

    DOT11_X_RTS,                  // 12  First State in range
    DOT11_X_CTS,                  // 13
    DOT11_X_UNICAST,              // 14
    DOT11_X_BROADCAST,            // 15
    DOT11_X_ACK,                  // 16
    DOT11_X_MANAGEMENT,           // 17
    DOT11_X_BEACON,               // 18  Last State in range
//---------------------------Power-Save-Mode-Updates---------------------//
    DOT11_X_IBSSBEACON,           // 19
    DOT11_X_PSPOLL,               // 20
//---------------------------Power-Save-Mode-End-Updates-----------------//
    DOT11_CFP_START,              // 22  State during CFP
    DOT11_S_MANAGEMENT,            // 23  In Management

    //--------------------HCCA-Updates-----------------//
    DOT11e_WF_CAP_START,            //24
    DOT11e_CAP_START,               //25   State during CAP
    //--------------------HCCA-END---------------------//

//---------------------------Power-Save-Mode-Updates---------------------//
    DOT11_S_WFIBSSBEACON,
    DOT11_S_WFIBSSJITTER,
    DOT11_S_WFPSPOLL_ACK,
//---------------------------Power-Save-Mode-End-Updates-----------------//
    DOT11_CONTENDING
} DOT11_MacStates;


// States during Contention Free Period
typedef enum {
    DOT11_S_CFP_NONE,             // Idle

    DOT11_S_CFP_WF_DATA,          // Sent poll, expecting data or null
    DOT11_S_CFP_WF_ACK,           // Sent data, waiting for ack
    DOT11_S_CFP_WF_DATA_ACK,      // Sent data+poll
    DOT11_S_CFP_WFBEACON,
    DOT11_X_CFP_BROADCAST,        // Transmit states
    DOT11_X_CFP_BEACON,
    DOT11_X_CFP_UNICAST,
    DOT11_X_CFP_UNICAST_POLL,
    DOT11_X_CFP_ACK,
    DOT11_X_CFP_POLL,
    DOT11_X_CFP_NULLDATA,
    DOT11_X_CFP_END

} DOT11_CfpStates;

// Frame catagory
typedef enum {
    DOT11_FC_MGMT,
    DOT11_FC_CTRL,
    DOT11_FC_DATA
}DOT11_MacFrameCatagory;


// Type of MAC frame
typedef enum {
    // Managment frame types
    DOT11_ASSOC_REQ           = 0x00, // 00 0000
    DOT11_ASSOC_RESP          = 0x01, // 00 0001
    DOT11_REASSOC_REQ         = 0x02, // 00 0010
    DOT11_REASSOC_RESP        = 0x03, // 00 0011
    DOT11_PROBE_REQ           = 0x04, // 00 0100
    DOT11_PROBE_RESP          = 0x05, // 00 0101
    DOT11_BEACON              = 0x08, // 00 1000
    DOT11_ATIM                = 0x09, // 00 1001
    DOT11_DISASSOCIATION      = 0x0A, // 00 1010
    DOT11_AUTHENTICATION      = 0x0B, // 00 1011
    DOT11_DEAUTHENTICATION    = 0x0C, // 00 1100
    //-----------HCCA-UPDATES-------------------
    DOT11_ACTION              = 0x0D, // 00 1101
    //---------HCCA-UPDATES-END-----------------

    // Control frame types
    DOT11_PS_POLL             = 0x1A, // 01 1010
    DOT11_RTS                 = 0x1B, // 01 1011
    DOT11_CTS                 = 0x1C, // 01 1100
    DOT11_ACK                 = 0x1D, // 01 1101
    DOT11_CF_END              = 0x1E, // 01 1110
    DOT11_CF_END_ACK          = 0x1F, // 01 1111

    // Data frame types
    DOT11_DATA                = 0x20, // 10 0000
    DOT11_CF_DATA_ACK         = 0x21, // 10 0001
    DOT11_CF_DATA_POLL        = 0x22, // 10 0010
    DOT11_CF_DATA_POLL_ACK    = 0x23, // 10 0011
    DOT11_CF_NULLDATA         = 0x24, // 10 0100
    DOT11_CF_ACK              = 0x25, // 10 0101
    DOT11_CF_POLL             = 0x26, // 10 0110
    DOT11_CF_POLL_ACK         = 0x27, // 10 0111

//---------------------DOT11e--Updates------------------------------------//

    DOT11_QOS_DATA            = 0x28, // 10 1000
    DOT11_QOS_CF_DATA_ACK     = 0x29, // 10 1001
    DOT11_QOS_DATA_POLL       = 0x2A, // 10 1010
    DOT11_QOS_DATA_POLL_ACK   = 0x2B, // 10 1011
    DOT11_QOS_CF_NULLDATA     = 0x2C, // 10 1100
    DOT11_QOS_RESERVED        = 0x2D, // 10 1101
    DOT11_QOS_CF_POLL         = 0x2E, // 10 1110
    DOT11_QOS_POLL_ACK        = 0x2F, // 10 1111
//--------------------DOT11e-End-Updates---------------------------------//

    DOT11_MESH_DATA           = 0x30, // 11 0000

    DOT11N_BLOCK_ACK         = 0x31,
    DOT11N_BLOCK_ACK_REQUEST = 0x34,
    DOT11N_DELAYED_BLOCK_ACK = 0x35,

    DOT11_CF_NONE             = 0x3F  // 11 1111 Reserved
} DOT11_MacFrameType;


// Station type
typedef enum {
    DOT11_STA_UNKNOWN,
    DOT11_STA_IBSS,
    DOT11_STA_BSS,
    DOT11_STA_AP,
    DOT11_STA_PC,
//-------------------------------- DOT11e-HCCA-Updates--------------------//
    DOT11_STA_HC
//----------------------------- DOT11e-End-HCCA-Updates--------------------//
} DOT11_StationType;

// Station type
typedef enum {
    DOT11_NET_UNKNOWN,
    DOT11_NET_IBSS,
    DOT11_NET_BSS
} DOT11_NetworkType;

// PC poll behavior
typedef enum {
    DOT11_PC_DELIVER_ONLY,            // PC only sends frames during CFP
    DOT11_PC_POLL_ONLY,               // PC polls and does not deliver
    DOT11_PC_POLL_AND_DELIVER         // PC polls then delivers, default
} DOT11_PcDeliveryMode;


// Poll save mode
typedef enum {
    DOT11_POLL_SAVE_NONE,
    DOT11_POLL_SAVE_BYCOUNT
} DOT11_PollSaveMode;


// Station poll behavior
typedef enum {
    DOT11_STA_NOT_POLLABLE,
    DOT11_STA_POLLABLE_NOT_IN_LIST,
    DOT11_STA_POLLABLE_DONT_POLL,
    DOT11_STA_POLLABLE_POLL
} DOT11_StationPollType;

// ASCII Trace
typedef enum {
    DOT11_TRACE_NO,                   // No trace generated, default
    DOT11_TRACE_YES                   // Internal ascii trace file format
} DOT11_TraceType;

// Association type
typedef enum {
    DOT11_ASSOCIATION_NONE,
    DOT11_ASSOCIATION_STATIC,
    DOT11_ASSOCIATION_DYNAMIC
}DOT11_AssociationType;
//---------------------------Power-Save-Mode-Updates---------------------//
// Section 7.3.2, RFC 802.11, Information elements
enum {
    DOT11_PS_TIM_ELEMENT_ID_SSID,
    DOT11_PS_TIM_ELEMENT_ID_SUPPORTED_RATES,
    DOT11_PS_TIM_ELEMENT_ID_FH_PARAMETER_SET,
    DOT11_PS_TIM_ELEMENT_ID_DS_PARAMETER_SET,
    DOT11_PS_TIM_ELEMENT_ID_CF_PARAMETER_SET,
    DOT11_PS_TIM_ELEMENT_ID_TIM,
    DOT11_PS_TIM_ELEMENT_ID_IBSS_PARAMETER_SET,


    // else reserved
    DOT11_PS_TIM_ELEMENT_ID_RESERVED
};
typedef unsigned char DOT11_PS_ElementId;
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
// typedef's struct
//--------------------------------------------------------------------------

#define DOT11_DATA_RATE_ADJUSTMENT_TIME  (60 * MILLI_SECOND)
#define DOT11_NUM_ACKS_FOR_RATE_INCREASE 10
#define DOT11_NUM_ACKS_FOR_RATE_DECREASE 2


///** Mode
//     *  The enum types for 802.11n HT operation modes and PPDU formats



///**
//STRUCT      :: DOT11_FrameInfo
//DESCRIPTION :: Items in frame buffer/queues. Used for both
//                management & data queues. Holds all relevant
//                information to create the appropriate 3 address
//                or 4 address data or management frame, except
//                for duration and sequence number.
//                Here, the RA/TA/DA/SA fields are the logical values
//                and may not correspond to what is written to the
//                address1/2/3/4 frame fields.
//**/
struct DOT11_FrameInfo
{
    Message* msg;
    DOT11_MacFrameType frameType;
    Mac802Address RA;         //Receiving address
    Mac802Address TA;         //Transmitting address
    Mac802Address DA;         //Final destionation
    Mac802Address SA;         //Initial source
    clocktype insertTime;
    BOOL isAMPDU;
    UInt8 state;
    UInt32 ampduOverhead;
    BOOL isAmsdu;
    BOOL baActive;
    DOT11_FrameInfo()
        :   msg(NULL), frameType(DOT11_CF_NONE),
            insertTime(0), isAMPDU(FALSE), state(0), ampduOverhead(0)
    {
            RA = INVALID_802ADDRESS;
            TA = INVALID_802ADDRESS;
            DA = INVALID_802ADDRESS;
            SA = INVALID_802ADDRESS;
            isAmsdu = FALSE;
            baActive = FALSE;
    }
};


struct DOT11_DataRateEntry {
    Mac802Address ipAddress;
    int         dataRateType;
    BOOL        firstTxAtNewRate;
    short       numAcksFailed;
    short       numAcksInSuccess;
    clocktype   dataRateTimer;
    MAC_PHY_TxRxVector   txVector;
    DOT11_DataRateEntry* next;
};

// Keeps track of sequence numbers of frames.

typedef struct struct_mac_dot11_seqno_entry_t {
    Mac802Address     ipAddress;
    unsigned short    padd1: 4,
                      fromSeqNo: 12;
    unsigned short    padd2: 4,
                      toSeqNo:12;
    struct struct_mac_dot11_seqno_entry_t* next;
} DOT11_SeqNoEntry;

//STRUCT      :: DOT11Ie
//DESCRIPTION :: Utility structure for working with IEs.
//                Length consists of entire IE, including header.
struct DOT11Ie
{
    int length;
    unsigned char* data;
};

/// Structure to hold QoS control filed related information
typedef struct {
    unsigned short  TID:4,
                    EOSP:1,
                    AckPolicy:2,
                    Reserved:1,
                    TXOP:8;
}DOT11e_QoSControl;

/// Structure to hold Access Category's records
typedef struct {
    unsigned char AIFS:4,
                  ACM:1,
                  ACI:2,
                  reserved:1;

    unsigned char ECWmin:4,
                  ECWmax:4;

    unsigned short TXOPLimit;
}DOT11e_ACParameterRecordFields;


/// Structure to hold QoS related informaton
typedef struct {
    unsigned char   EDCFParameterSetUpdateCount: 4,
                    QACK:1,
                    QueueRequest:1,
                    TXOPRequest:1,
                    Reserved:1;
}DOT11e_QosInfoField;

/// Structure to hold EDCF Parameter Set
typedef struct {
    DOT11e_QosInfoField qosInfo;
    unsigned char reserved;
    DOT11e_ACParameterRecordFields ACs[DOT11e_NUMBER_OF_AC];

}DOT11e_EDCAParameterSet;

/// Structure to hold QoS enable Base Stations load elements
typedef struct {
    unsigned short stationCount;
    unsigned char  channelUtilization;
    unsigned char  availableAdmissionControl;
}DOT11e_QBSSLoadElement;

//--------------------DOT11e-End-Updates---------------------------------//

// RD: THIS STRUCT NEEDS TO BE PACKED
typedef struct qos_control {
    unsigned short  qc_pending;
    unsigned short  qc_fec;
    unsigned short  qc_non_final;
    unsigned short  qc_no_ack;
    unsigned short  qc_tid;
}QoS_Control;

//
// Following typedef's define MAC headers.
// NOTE:
//   The size of each structure is not used for computation of transmission
//   duration. Instead, size definitions (i.e., DOT11_DATA_FRAME_SIZE,
//   DOT11_SHORT_CTRL_FRAME_SIZE, DOT11_LONG_CTRL_FRAME_SIZE) are used
//   for such purposes. Therefore, the structure sizes do not have to be
//   identical to the sizes in the standard, but THEY MUST BE LESS THAN THE
//   ACTUAL SIZES.
//

//
// CTS and ACK frames.
// Note: - All frames types must match the short control (this one)
//         exactly for its first four (universal) fields.
//       - FCS is supposed to be after data payload.  However,
//         we include FCS within the header for coding convenience.
//       - Actual frame size need not match standard here since
//         we use DOT11_SHORT_CTRL_FRAME_SIZE to determine size.

typedef struct {
                                    //  Should Be  Actually
    UInt8             frameType;    //      -         -
    UInt8             frameFlags;   //      2         2
    UInt16            duration;     //      2         2
    Mac802Address     destAddr;     //      6         6
    char              FCS[4];       //      4         4
} DOT11_ShortControlFrame;          // ---------------------
                                    //     14        14
//---------------------------Power-Save-Mode-End-Updates-----------------//

// RTS frames.
typedef struct {
                                    //  Should Be  Actually
    UInt8             frameType;    //      -         -
    UInt8             frameFlags;   //      2         2
    UInt16            duration;     //      2         2
    Mac802Address     destAddr;     //      6         6
    Mac802Address     sourceAddr;   //      6         6
    char              FCS[4];       //      4         4
} DOT11_LongControlFrame;           //----------------------
                                    //     20        20

// Data frame header.
// Note: - Actual frame size need not match standard here since
//         we use DOT11_DATA_FRAME_HDR_SIZE to determine frame size.
//       - FCS is supposed to be after data payload.  However,
//         we include FCS within the header for coding convenience.

struct DOT11_FrameHdr{
                                    //  Should Be  Actually
    UInt8             frameType;    //      -         -
    UInt8             frameFlags;   //      2         2
    UInt16            duration;     //      2         2
    Mac802Address     destAddr;     //      6         6
    Mac802Address     sourceAddr;   //      6         6
    Mac802Address     address3;     //      6         6
    UInt16            fragId: 4,    //      -         -
                      seqNo: 12 ;   //      2         2
    char              FCS[4];       //      4         4
                                    //---------------------
                                    //     28        28
};

// QoS Data frame header.
// Note: - Actual frame size need not match standard here since
//         we use DOT11_DATA_FRAME_HDR_SIZE to determine frame size.
//       - FCS is supposed to be after data payload.  However,
//         we include FCS within the header for coding convenience.

typedef struct{
                                    //  Should Be  Actually
    UInt8             frameType;    //      -         -
    UInt8             frameFlags;   //      2         2
    UInt16            duration;     //      2         2
    Mac802Address     destAddr;     //      6         6
    Mac802Address     sourceAddr;   //      6         6
    Mac802Address     address3;     //      6         6
    UInt16            fragId: 4,    //      -         -
                      seqNo: 12 ;   //      2         2
    DOT11e_QoSControl qoSControl;   //      2         2
    char              FCS[4];       //      4         4 //Changed temp to match stats
} DOT11e_FrameHdr;                  //---------------------
                                    //     30        30

// Data frames.
struct  DOT11_MacFrame {
    DOT11_FrameHdr     hdr;
    char            payload[MAX_NW_PKT_SIZE];
};

/*********HT START*************************************************/
typedef struct struct_mac_dot11_HT_CapabilityInfo{
    unsigned   short ldpcCodingCapability: 1,
                     supportedChannelWidthSet: 1,
                     smPowerSave: 2,
                     htGreenfield: 1,
                     shortGiFor20Mhz: 1,
                     shortGiFor40Mhz: 1,
                     txStbc: 1,
                     rxStbc: 2,
                     htDelayedBlockAck: 1,
                     maxAmsduLength: 1,
                     dsssCckMode40Mhz: 1,                    
                     reserved: 1, 
                     fortyMhzIntolerent: 1,
                     lsigTxopProtectionSupport: 1;

}DOT11_HT_CapabilitiesInfo;

union DOT11_HT_SupportedMCSSet
{
    UInt64 bitArray[2];         //Total is 128 bits
    struct McsSet               //supported MCS set information, non-standard format 
    {
        UInt8   maxMcsIdx;       //Maximum supported mcs index
    } mcsSet;
};

typedef struct struct_mac_dot11_HT_ExtendedCapabilities{
    unsigned   short pco: 1,
                     pcoTransitionTime: 2,
                     reserved: 5,
                     mcsFeedback: 2,
                     htcSupport: 1,
                     rdResponder: 1,
                     reserved2: 4;     

}DOT11_HT_ExtendedCapabilities;

typedef struct struct_mac_dot11_HT_TransmitBeamFrmgCapabilities{
    double tramsmitBeamformingCapabilities;

}DOT11_HT_TransmitBeamFrmgCapabilities;

typedef struct struct_mac_dot11_HT_ASESCapabilities{
    UInt8   antennaSelectionCapable: 1,
                 explicitCsibasedAselCapable: 1,
                 antennaIndicesbasedAselCapable: 1,
                 explicitCsiFeedbackCapable: 1,
                antennaIndicesFeedbackCapable: 1,
                 receiveAselCapable: 1,
                transmitSoundingPpduCapable: 1,
                reserved:1;
}DOT11_HT_ASESCapabilities;


typedef struct struct_mac_dot11_HT_AmpduParams
{
    UInt8 ampduMaxLengthExponent: 2,
              minAmpduStartSpacing: 3,
              reserved: 3; 
}DOT11_HT_AmpduParams;

typedef struct struct_mac_dot11_HT_CapabilityElement{
    UInt8 elementId;
    UInt8 length;
    DOT11_HT_AmpduParams ampduParams;
    DOT11_HT_ASESCapabilities asesCapabilities;
    DOT11_HT_CapabilitiesInfo htCapabilitiesInfo;
    DOT11_HT_ExtendedCapabilities extendedCapabilities;
    DOT11_HT_TransmitBeamFrmgCapabilities transmitBeamFrmgCapabilities;
    DOT11_HT_SupportedMCSSet supportedMCSSet;
} DOT11_HT_CapabilityElement;

struct DOT11_HT_OperationElement    //HT operation structures
{
    ChBandwidth   opChBwdth;        //operation channel bandwidth  
    BOOL          rifsMode;         //most other parameters are neglected
    UInt8 primaryCh;
    UInt8 secondaryChOffset;

    DOT11_HT_OperationElement() {
        opChBwdth = CHBWDTH_20MHZ;
        rifsMode  = FALSE;
    }

    void ToPacket(char* pkt, size_t* length) const
    {
        *length = DOT11N_HT_OPERATION_ELEMENT_LEN;
        memset(pkt, 0, *length); 

        if (opChBwdth != CHBWDTH_20MHZ) {
            *(pkt+1) |= 1 << 5;
        }
        if (rifsMode) {
            *(pkt+1) |= 1 << 4;
        }
    }

    void FromPacket(const char* pkt, size_t length)
    {
        opChBwdth = (ChBandwidth)((*(pkt+1))>>5 & 1);
        rifsMode =  (*(pkt+1))>>4 & 1;
    }

    void Print(char* strBuf, size_t bufSize) 
    {
        snprintf(strBuf, bufSize, 
            "Channel Bandwidth ID: %d, Enable RIFS: %d\n", opChBwdth, rifsMode);  
    }
};

struct VHT_CapabilitiesInfo
{
    UInt32  m_maxMpduLen: 2,
            m_chWidth: 2,
            m_rxLdpc: 1,
            m_shortGi80Mhz: 1,
            m_shortGi160Mhz: 1,
            m_txStbc: 1,
            m_rxStbc: 3,
            m_suBformerCapable: 1,
            m_suBformeeCapable: 1,
            m_bformeeStsCapability: 3,
            m_numSoundingDmns: 3,
            m_muBformerCapable: 1, 
            m_mubformeeCapable: 1, 
            m_vhtTxopPs: 1,
            m_htcvhtCapable: 1,
            m_maxAmpduLenExponent: 3, 
            m_vhtLinkAdapCapable: 2,
            m_rxAntennaPatternCsty:1, 
            m_txAntennaPatternCsty:1, 
            m_reserved: 2;
};

struct VHT_CapabilitiesElement
{
    VHT_CapabilitiesInfo m_capabilitiesInfo;
    UInt32 m_mcsNssSet;
    UInt32 m_mcsNssSet2;
};

struct VHT_OperationInfo
{
    UInt8 m_chWidth;
    UInt8 m_chCenterFreqSeg0;
    UInt8 m_chCenterFreqSeg1;

    VHT_OperationInfo()
    {
        m_chWidth = 0;
        m_chCenterFreqSeg0 = 0;
        m_chCenterFreqSeg1 = 0;
    }

    void setVhtOperationInfo(UInt8 chWidth,
        UInt8 chCenterFreqSeq0, UInt8 chCenterFreqSeq1)
    {
        m_chWidth = chWidth;
        m_chCenterFreqSeg0 = chCenterFreqSeq0;
        m_chCenterFreqSeg1 = chCenterFreqSeq1;
    }
};

struct VHT_OperationElement
{
    VHT_OperationInfo m_operationInfo;
    UInt16 m_basicMcsNssSet;

    VHT_OperationElement()
    {
        m_basicMcsNssSet = 0;
    }

    VHT_OperationInfo& getVhtOperationInfo()
    {
        return m_operationInfo;
    }

    UInt16 getBasicMcsNssSet()
    {
        return m_basicMcsNssSet;
    }
};

struct VHTInfo
{
    int vhtMcs;
    VHT_CapabilitiesElement staVhtCapabilityElement;
    VHT_OperationElement vhtOperationElement;
    int chIdx[4];
};

// Neigbhor information regarding to HT 
struct DOT11_NeighborHtInfo 
{
    Mac802Address       address;            //Key
    BOOL                isAp;               //whether it is an AP 

    // HT capabilities information
    int                 maxMcsIdx;          //For simplicity, not using MCS supported set
    BOOL                stbcCapable; 
    BOOL                shortGiCapable;

    // Information used for link adaptation
    int                 curMcsIdx;          // Current MCS index used to send data frame
    BOOL                isFortyMhzIntolerant;
    BOOL                isOneSixtyMhzIntolerent;
    DOT11_NeighborHtInfo(const Mac802Address& addr,
                         BOOL                 ap = FALSE,
                         int                  maxMcs = -1,
                         BOOL                 stbc   = FALSE,
                         BOOL                 shortGi= FALSE,
                         BOOL                 FortyMhzIntolerant = TRUE,
                         BOOL                 OneSixtyMhzIntolerent = TRUE)
        : address(addr), isAp(ap), maxMcsIdx(maxMcs), 
          stbcCapable(stbc), shortGiCapable(shortGi),
          isFortyMhzIntolerant(FortyMhzIntolerant),
          isOneSixtyMhzIntolerent(OneSixtyMhzIntolerent)
    {
        curMcsIdx = -1;
    }
};

typedef std::list<DOT11_NeighborHtInfo*>  DOT11_NeighborHtInfoList;

/*********HT END****************************************************/

/// Structure to hold capability related information of 802.11e
#ifndef CAP_INFO
# define CAP_INFO
typedef struct {
    unsigned  short   ESS:1,
                      IBSS:1,
                      CFPollable:1,
                      CFPollRequest:1,
                      Privacy:1,
                      ShortPreamble:1,
                      PBCC:1,
                      ChannelAgility:1,
                      SpectrumManagement:1,
                      QoS:1,
                      ShortSlotTime:1,
                      APSD:1,
                      Reserved:1,
                      DSSS_OFDM:1,
                      DelayedBlockACK:1,
                      ImmidiateBlockACK:1;
}DOT11e_CapabilityInfo;
#endif


// AP related structures
// AP station list
typedef struct struct_mac_dot11_ApStation {
    Mac802Address              macAddr;
    DOT11_StationType     stationType;
//---------------------------Power-Save-Mode-Updates---------------------//
    BOOL                    isSTAInPowerSaveMode;
    unsigned short          listenInterval;
    int                     noOfBufferedPacket;
    Queue*                  queue;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    int                     assocId;        // Stations AID  0 - unassigned
    unsigned char           flags;          // Can be used for various purposes
    // DOT11e this flags is set as 1 if the station is QoS enabled.
    clocktype            LastFrameReceivedTime;
    
/*********HT START*************************************************/
    BOOL isHTEnabledSta;
    UInt8 mscIndex;
    DOT11_HT_CapabilityElement staHtCapabilityElement;
    DOT11e_CapabilityInfo staCapInfo;
    BOOL rifsMode;
/*********HT END****************************************************/

    BOOL isVHTEnabledSta;
    VHTInfo vhtInfo;
} DOT11_ApStation;


typedef struct struct_mac_dot11_ApStationListItem {
    DOT11_ApStation* data;
    struct struct_mac_dot11_ApStationListItem* prev;
    struct struct_mac_dot11_ApStationListItem* next;
} DOT11_ApStationListItem;


typedef struct struct_mac_dot11_ApStationList {
    int size;
    DOT11_ApStationListItem*     first;      // First item in list
    DOT11_ApStationListItem*     last;       // Last item in list
} DOT11_ApStationList;

// AP specific variables

// TIM Frame when not in PS Mode, section 7.3.2.6 RFC 802.11
typedef struct struct_mac_dot11_TIMFrame {
    // section 7.2.3.1 RFC 802.11
    unsigned char         dTIMCount;
    unsigned char         dTIMPeriod;
    unsigned char         bitmapControl;
    unsigned char          partialVirtualBitMap;
} DOT11_TIMFrame;

typedef struct struct_mac_dot11_ApVars {
    BOOL                        broadcastsHavePriority; // Priority of broadcasts
//---------------------------Power-Save-Mode-Updates---------------------//
    // PS mode variables
    BOOL            isAPSupportPSMode;
    BOOL            isBroadcastPacketTxContinue;
    unsigned int    dtimPeriod;
    int             lastDTIMCount;
    int             DTIMCount;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    int                         stationCount;           // Count of stations assigned to AP
    DOT11_ApStationList*      apStationList;          // List of stations
    DOT11_ApStationListItem*  firstStationItem;       // First polled in a CFP
    DOT11_ApStationListItem*  prevStationItem;        // Prev polled this CFP
    DOT11_TIMFrame            timFrame;
    unsigned char              AssociationVector[DOT11_PS_SIZE_OF_MAX_PARTIAL_VIRTUAL_BITMAP];
} DOT11_ApVars;

typedef struct struct_mac_dot11_CFParameterSet {

    //char                  padding[2];     // cfpCount and cfpPeriod
    int                     cfpCount;
    int                     cfpPeriod;
    unsigned short          cfpMaxDuration; // CFP Duration
    int                     cfpBalDuration; // CFP Balance duration
} DOT11_CFParameterSet;

//---------------------------Power-Save-Mode-Updates---------------------//
// TIM Frame, section 7.3.2.6 RFC 802.11
typedef struct struct_mac_dot11_PS_TIMFrame {
    // section 7.2.3.1 RFC 802.11
    DOT11_PS_ElementId    elementId;
    unsigned char         length;
    unsigned char         dTIMCount;
    unsigned char         dTIMPeriod;
    unsigned char         bitmapControl;
} DOT11_PS_TIMFrame;

typedef struct struct_mac_dot11_PS_IBSSParameter {
    // section 7.2.3.1 RFC 802.11
    DOT11_PS_ElementId    elementId;
    unsigned char         length;
    unsigned short           atimWindow;
} DOT11_PS_IBSSParameter;

//---------------------------Power-Save-Mode-End-Updates-----------------//
// STRUCT      :: DOT11e_CapabilityInfo
// DESCRIPTION :: Structure to hold capability related information of 802.11e
#ifndef CAP_INFO
# define CAP_INFO
typedef struct {
    unsigned  short   ESS:1,
                      IBSS:1,
                      CFPollable:1,
                      CFPollRequest:1,
                      Privacy:1,
                      ShortPreamble:1,
                      PBCC:1,
                      ChannelAgility:1,
                      SpectrumManagement:1,
                      QoS:1,
                      ShortSlotTime:1,
                      APSD:1,
                      Reserved:1,
                      DSSS_OFDM:1,
                      DelayedBlockACK:1,
                      ImmidiateBlockACK:1;
}DOT11e_CapabilityInfo;
#endif
/**/
//--------------------DOT11e-End-Updates---------------------------------//

// Association/Reassociation request and response frames
typedef struct struct_mac_dot11_CapabilityInfo {
    unsigned   short ess: 1,
                     ibbs: 1,
                     cfPollable: 1,
                     cfPollRequest: 1,
                     privacy: 1,
                     shortPreamble: 1,
                     pbcc: 1,
                     channelAgility: 1,
                     reserved:8;
} DOT11_CapabilityInfo;


// Beacon frame
typedef struct struct_mac_dot11_BeaconFrame {
    DOT11_FrameHdr           hdr;
    unsigned short          beaconInterval; // ~ 1ms units - default 100ms
    DOT11_CapabilityInfo  capability ;
    clocktype               timeStamp;
    char                    ssid[32];
} DOT11_BeaconFrame;

// CF Pollable station list
typedef struct struct_mac_dot11_CfPollStation {
    Mac802Address             macAddr;
    DOT11_StationPollType pollType;
    int                     nullDataCount;  // Number of null data responses
    int                     pollCount;      // Number of polls sent
    unsigned char           flags;          // Can be used for various purposes
    int                     assocId;
} DOT11_CfPollStation;


typedef struct struct_mac_dot11_CfPollListItem {
    DOT11_CfPollStation* data;
    struct struct_mac_dot11_CfPollListItem* prev;
    struct struct_mac_dot11_CfPollListItem* next;
} DOT11_CfPollListItem;


typedef struct struct_mac_dot11_CfPollList {
    int size;
    DOT11_CfPollListItem*     first;      // First item in list
    DOT11_CfPollListItem*     last;       // Last item in list
} DOT11_CfPollList;


// PC specific variables
typedef struct struct_mac_dot11_PcVars {
    DOT11_PcDeliveryMode     deliveryMode;           // Deliver only or poll+deliver
    BOOL                    broadcastsHavePriority; // Priority of broadcasts over polls

    DOT11_CfPollList*        cfPollList;             // List of stations
    int                     pollableCount;          // Count of pollable stations
    DOT11_CfPollListItem*    firstPolledItem;        // First polled in a CFP
    DOT11_CfPollListItem*    prevPolledItem;         // Prev polled this CFP
    BOOL                    allStationsPolled;      // If all stations polled once

    DOT11_PollSaveMode       pollSaveMode;           // Poll save mode
    int                     pollSaveMinCount;       // Min count for poll save
    int                     pollSaveMaxCount;       // Max count for poll save
    BOOL                    isBeaconDelayed;         // if CFP start Beacon is Delayed
    int                     dataRateTypeForAck;     // Data rate for ACK during PCF
    int                     numberOfDTIMFrameSent;   //Number of DTIM frame sent in                                                    //current CFP repetetion interval
    int                     lastCfpCount;    // To store Last Cfp Count
    DOT11_CFParameterSet    cfSet;
    DOT11e_EDCAParameterSet EDCAParameterSet;
    DOT11e_QBSSLoadElement  QBSSLOADElementSet;
    clocktype cfpEndTime;

} DOT11_PcVars;

//---------------------DOT11e--Updates------------------------------------//


/// Structure to hold Access Category related information
typedef struct AccessCatagoryData {

    clocktype cwMax;
    clocktype CW;
    clocktype cwMin;
    clocktype AIFS;
    clocktype TXOPLimit;
    clocktype AIFSN;
    clocktype slotTime;
    clocktype BO;
    clocktype lastBOTimeStamp;

    DOT11_SeqNoEntry* seqNoHead;
    int QSRC;
    int QLRC;
    // Extra variable to merge single frame Mac Queue
    // into ACs. to send a frame;
    DOT11_FrameInfo* frameInfo;

    Message* frameToSend;
    TosType priority;
    Mac802Address currentNextHopAddress;
    int networkType;
/*********HT START*************************************************/
    OutputBuffer *outputBuffer;
    BOOL isACHasPacket;
/*********HT END*************************************************/


    // Statistics Variable:
    // No of this category type of frame Queued
    unsigned int totalNoOfthisTypeFrameQueued;
    unsigned int totalNoOfthisTypeFrameQueuedUnderBAA;
    unsigned int totalNoOfthisTypeFrameDeQueued;
    unsigned int totalNoOfthisTypeFrameRetried;

} DOT11e_ACData;

//--------------------DOT11e-End-Updates---------------------------------//
//---------------------------Power-Save-Mode-Updates---------------------//
/// Structure to hold attach node information in IBSS mode
typedef struct struct_mac_dot11_AttachedNodeList {
    Mac802Address    nodeMacAddr;
    BOOL           atimFramesSend;
    BOOL           atimFramesACKReceived;
    Queue*         queue;
    unsigned int noOfBufferedUnicastPacket;
    struct_mac_dot11_AttachedNodeList* next;
} DOT11_AttachedNodeList;

/// Structure to hold receive ATIM frame information in IBSS mode
typedef struct struct_mac_dot11_ReceivedATIMFrameList {
    Mac802Address    nodeMacAddr;
    BOOL           atimFrameReceive;
    struct_mac_dot11_ReceivedATIMFrameList* next;
} DOT11_ReceivedATIMFrameList;
//---------------------------Power-Save-Mode-End-Updates-----------------//
//------------------HCCA-Update----------//
//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------

#define DOT11e_HCCA_CFP_MAX_DURATION_DEFAULT    (50* DOT11_TIME_UNIT)

#define DOT11e_HCCA_STATION_POLL_TYPE_DEFAULT   DOT11e_STA_POLLABLE_POLL
#define DOT11e_HCCA_CF_FRAME_DURATION           32768

//--------------------------------------------------------------------------
// typedef's enums
//--------------------------------------------------------------------------
//States during Controlled Access Phase
typedef enum {
    //DOT11e_S_IDLE,                 // 0
    DOT11e_S_CFP_NONE,                         //idle
    DOT11e_S_CFP_WF_DATA_ACK,      // Sent data,waiting for ACK
    DOT11e_S_CFP_WF_DATA,
    DOT11e_S_CFP_WF_ACK,

    DOT11e_X_QoS_CFP_POLL,        // Transmit states
    DOT11e_X_QoS_CFP_POLL_To_HC,
    DOT11e_X_QoS_DATA_POLL,
    DOT11e_X_QoS_DATA_ACK,
    DOT11e_X_ADDTS_REQ,
    DOT11e_X_ADDTS_RESP,
    DOT11e_X_POLL_DATA_ACK ,         //send ACK of the poll and DATA
    DOT11e_X_CFP_UNICAST,
    DOT11e_X_CFP_NULLDATA,
    DOT11e_X_ACK,
    DOT11e_X_CFP_END

} DOT11e_CapStates;


//enumeration to support QoS CAP frame
typedef enum {
    DOT11e_HC_DELIVER_ONLY,            // HC only sends frames during CAP
    DOT11e_HC_POLL_ONLY,                   // HC polls and does not deliver
    DOT11e_HC_POLL_AND_DELIVER    // HC polls then delivers, default
} DOT11e_HcDeliveryMode;

typedef enum
{
    DOT11e_UPLINK,
    DOT11e_DOWNLINK
}DOT11e_Direction;

// QOS Action fields
typedef enum {
    DOT11e_ADDTS_Request,
    DOT11e_ADDTS_Response,
    DOT11e_DELTS,
    DOT11e_Schedule,
    DOT11e_Reserved
} DOT11e_QoSActionField;


typedef enum {
    DOT11e_TS_None,
    DOT11e_TS_Requested,
    DOT11e_TS_Active
} DOT11e_TSStatusType;

//--------------------------------------------------------------------------
// typedef's struct
//--------------------------------------------------------------------------

// Structure to hold HCCA Parameter Set
typedef struct
{
    clocktype cwMax;
    clocktype CW;
    clocktype cwMin;
    clocktype AIFS;
    clocktype TXOPLimit;
    clocktype AIFSN;
    clocktype slotTime;
    clocktype BO;
    clocktype lastBOTimeStamp;

}DOT11e_HCCAParameterSet;

// structure to hold traffic stream information
 typedef struct {
    unsigned short    trafficType:1,
                      tsid:4,
                      direction:2,
                      accessPolicy:2,
                      aggregation:1,
                      apsd:1,
                      priority:3,
                      tsInfoAckPolicy:2;
     unsigned char    schedule:1,
                      reserved:7;
}TSinfo;

  // Structure to hold HCCA table
 typedef struct struct_mac_dot11e_HCCATable{
    unsigned int dot11HCCATableIndex;
    unsigned int dot11HCCATableCWmin;
    unsigned int dot11HCATableCWmax;
    unsigned int dot11HCCATableAIFSN;
} DOT11e_HCCATable;

// Structure to hold traffic specification
 typedef struct {
    TSinfo              info;
    unsigned short      nominalMSDUSize;
    unsigned short      maximumMSDUSize;
    unsigned int        minimumServiceInterval;
    unsigned int        maximumServiceInterval;
    unsigned int        inactivityInterval;
    unsigned int        suspensionInterval;
    unsigned int        serviceStartTime;
    unsigned int        minimumDataRate;
    unsigned int        meanDataRate;
    unsigned int        peakDataRate;
    unsigned int        burstSize;
    unsigned int        delayBound;
    unsigned int        minPhyRate;
    unsigned short      surplusBandwidthAllowance;
    unsigned short      mediumTime;
} TrafficSpec;


//  Structure to hold HCCA pollable Station
typedef struct struct_mac_Dot11e_HCCA_CapPollStation {
    Mac802Address         macAddr;
    int                 nullDataCount;  // Number of null data responses
} DOT11e_HCCA_CapPollStation;


//  Structure to hold HCCA poll list item
typedef struct struct_mac_Dot11e_HCCA_CapPollListStationItem {
    DOT11e_HCCA_CapPollStation* data;
    struct struct_mac_Dot11e_HCCA_CapPollListStationItem *prev;
    struct struct_mac_Dot11e_HCCA_CapPollListStationItem *next;
} Dot11e_HCCA_cappollListStationItem;



//     Structure to hold Traffic stream identifier
typedef struct struct_mac_Dot11e_HCCA_TrafficStream{
    int                     hccaTXOP;
    int                     tsid;
    int                     size;
    Dot11e_HCCA_cappollListStationItem* first;
    Dot11e_HCCA_cappollListStationItem* last;
    Dot11e_HCCA_cappollListStationItem* firstPolledItem;
} Dot11e_HCCA_TrafficStream;



//  Structure to hold HCCA poll list

typedef struct struct_mac_dot11e_CapPollList {
    unsigned int size;
    Dot11e_HCCA_TrafficStream TS[DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED];
} DOT11e_HCCA_CapPollList;


//Structure for downlink list
typedef struct struct_mac_dot11e_CapDownlink_PollList
{
    Mac802Address                    macAddr;
    Queue                       *queue;
    struct_mac_dot11e_CapDownlink_PollList *next;
    struct_mac_dot11e_CapDownlink_PollList *prev;
}DOT11e_HCCA_CapDownlinkPollList;

  // Structure to hold TS data buffer item
 typedef struct struct_mac_DOT11e_HCCA_TSDataBufferItem{
    DOT11_FrameInfo* frameInfo;
    Mac802Address nextHopAddress;
    Message* msg;
    struct_mac_DOT11e_HCCA_TSDataBufferItem* prev;
    struct_mac_DOT11e_HCCA_TSDataBufferItem* next;
} DOT11e_HCCA_TSDataBufferItem;



 // Structure to hold TS data buffer
 typedef struct struct_mac_DOT11e_HCCA_TSDataBuffer{
    int size;
    TrafficSpec trafficSpec;
    DOT11e_TSStatusType tsStatus;
    DOT11_SeqNoEntry* seqNoHead;
    struct_mac_DOT11e_HCCA_TSDataBufferItem* first;
    struct_mac_DOT11e_HCCA_TSDataBufferItem* last;
} DOT11e_HCCA_TSDataBuffer;

// Structure to hold HC Data.
typedef struct struct_mac_dot11e_HcVars {
    // Deliver only or poll+deliver
    DOT11e_HcDeliveryMode            deliveryMode;

    // Priority of broadcasts over polls
    BOOL                             broadcastsHavePriority;

    // List of stations in Traffic Stream
    DOT11e_HCCA_CapPollList*         cfPollList;

    //list of stations and packets received per Station
    DOT11e_HCCA_CapDownlinkPollList* cfDownlinkList;

    // Current station that got polled.
    Dot11e_HCCA_cappollListStationItem *      currPolledItem;

    //TSID of the current polled station.
    int                              currentTSID;

    //All stations in current TSID are polled
    BOOL                             allCurrTSIDStationsPolled;

    // If all stations polled once
    BOOL                             allStationsPolledOnce;

    // Count of pollable stations in current CAP
    BOOL                            hasPollableStations;

    //Time when current poll was sent
    clocktype                        polledTime;

    //Amount of TXOP Granted to current STA that got poll
    clocktype                        txopDurationGranted;

    // Data rate for ACK during HCCA
    int                                 dataRateTypeForAck;

    //Data buffer at station
    DOT11e_HCCA_TSDataBuffer*
        staTSBufferItem[DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED];

    //Max CAP duration
    clocktype                        capMaxDuration;

    //CAP Start time
    clocktype                        capStartTime;

    //Time when current CAP should end
    clocktype                        capEndTime;

    //Total period used in current CAP
    clocktype                        capPeriodExpired;

    //Time when current TXOP was started
    clocktype                        txopStartTime;
    Mac802Address                      nextHopAddress;

    //Current poll is for HC. This is only used for setting correct
    //X-mit state i.e DOT11e_X_QoS_CFP_POLL_To_HC, while polling to self
    BOOL                             pollingToHc;
} DOT11e_HcVars;


 //ACTION FRAME
 typedef struct
 {
     DOT11_FrameHdr              hdr;
     int                      category;
     DOT11e_QoSActionField    qosAction;
 }DOT11e_Action_frame;


 //  ADD TS Responce
 typedef struct {
     DOT11_FrameHdr              hdr;
     int                      category;
     DOT11e_QoSActionField    qosAction;
     unsigned char            statusCode;
     unsigned char            TSDelay;

} DOT11e_ADDTS_Response_frame;

 // ADD TS Request
 typedef struct  {
     DOT11_FrameHdr              hdr;
     int                      category;
     DOT11e_QoSActionField    qosAction;
     unsigned char            DialogToken;
}DOT11e_ADDTS_Request_frame;

//----------------------------------------HCCA-End-------------------------//

// -------------------------------------------------------------------------
// Basic structure for signal measurement
// -------------------------------------------------------------------------
/// data structure for signal measurement
typedef struct {
    double rssi;
    double cinr;
    clocktype measTime;
    BOOL isActive;
}DOT11_SignalMeasurementInfo;

/// The information of an AP maintained at STA
typedef struct dot11_ap_info_str{
    Mac802Address bssAddr;          // Mac address of AP
    int channelId;                // Channel of AP
    BOOL qosEnabled;              // AP support QoS
    clocktype lastMeaTime;        // time, last signal quality measure
    double cinrMean;              // avg  SINR, in dB
    double rssMean;               // avg RSS, in dBm
    DOT11_CFParameterSet cfSet;
    clocktype beaconInterval;
    clocktype lastCfpBeaconTime;

/*********HT START*************************************************/
    BOOL htEnableAP;
    int mcsIndex;
    DOT11_HT_CapabilityElement staHtCapabilityElement;
    DOT11e_CapabilityInfo apCapInfo;
    BOOL rifsMode;
/*********HT END****************************************************/

     BOOL vhtEnableAP;
     VHTInfo vhtInfo;
     int vhtMcsIndex;

    // measurement histroy
    DOT11_SignalMeasurementInfo measInfo[DOT11_MAX_NUM_MEASUREMNT];

    // pointer to possible next AP
    struct dot11_ap_info_str* next;
}DOT11_ApInfo;


class AmsduBuffer;
struct DOT11n_IBSS_Station_Info;


// Dot11 data structure
struct MacDataDot11 {
    MacData* myMacData;

    // DVCS (Directional Virtual Carrier Sensing) for directional transmission
    BOOL useDvcs;

    // Mac states
    DOT11_MacStates state;
    DOT11_MacStates prevState;
    //-------------HCCA-Updates--------------------//
    DOT11e_CapStates cfstate;
    DOT11e_CapStates cfprevstate;
    //------------HCCA-Updates-End----------------//

    BOOL IsInExtendedIfsMode;

    clocktype noResponseTimeoutDuration;

    // Backoff value at this station
    clocktype CW;
    clocktype BO;
    clocktype lastBOTimeStamp;

    unsigned int timerSequenceNumber;

    char PartialFrame[MAX_NW_PKT_SIZE];

    DOT11_DataRateEntry* dataRateHead;
    DOT11_SeqNoEntry* seqNoHead;

    clocktype NAV;
    clocktype noOutgoingPacketsUntilTime;
    int shortRetryLimit;
    int longRetryLimit;
    int directionalShortRetryLimit;
    BOOL stopReceivingAfterHeaderMode;
    int rtsThreshold;
    int SSRC;
    int SLRC;
    Mac802Address waitingForAckOrCtsFromAddress;

    D_Clocktype extraPropDelay;

    int cwMin;
    int cwMax;
    clocktype slotTime;
    clocktype sifs;
    clocktype delayUntilSignalAirborn;
    clocktype txSifs;
    D_Clocktype propagationDelay;
    clocktype difs;
    clocktype txDifs;
    clocktype pifs;
    clocktype txPifs;
    clocktype extendedIfsDelay;

    int lowestDataRateType;
    int highestDataRateType;
    int highestDataRateTypeForBC;

/*********HT START*************************************************/
    MAC_PHY_TxRxVector txVectorForBC;
    MAC_PHY_TxRxVector txVectorForHighestDataRate;
    MAC_PHY_TxRxVector txVectorForLowestDataRate;
/*********HT END****************************************************/ 

    clocktype rateAdjustmentTime;

    Message* currentMessage;            // Message from network layer
    Message* instantMessage;            // Send Frame Instantly
    Mac802Address currentNextHopAddress;
    DOT11_DataRateEntry* dataRateInfo;

    const Message* potentialIncomingMessage;

    DirectionalAntennaMacInfoType* directionalInfo;

    // Infrastructure variables
    DOT11_StationType stationType;      // Stations type
    Mac802Address bssAddr;                // IP/Mac address of AP
    Mac802Address selfAddr;               // Own IP/Mac address of interface
    Mac802Address ipNextHopAddr;          // Next hop address given by Network
    Mac802Address ipSourceAddr;           // Originating source address
    Mac802Address ipDestAddr;             // Final destination address

    // information of the current AP
    DOT11_ApInfo* associatedAP;

    BOOL relayFrames;                   // Relay by AP/PC outside BSS?

    // CFP related variables
    DOT11_CfpStates cfpState;
    DOT11_CfpStates cfpPrevState;
    int cfPollable;
    int cfPollRequest;
    DOT11_StationPollType pollType;
    BOOL   pcfEnable;
    BOOL cfpBeaconIsDue;                // Beacon is pending
    unsigned int cfpBeaconSequenceNumber;//Timer sequence ID
    clocktype cfpMaxDuration;           // Max duration of CF period
    clocktype cfpBeaconInterval;        // Target beacon interval
    clocktype cfpRepetionInterval;       // After this time CF Duration will repeat
    clocktype cfpBeaconTime;            // Last or next beacon time
    int       cfpPeriod;            // CFP Repetition interval in terms of number
                                    //of DTIM periods
    BOOL beaconSet;
    clocktype cfpBalanceDuration;
    unsigned int    DTIMperiod;
    unsigned char DTIMCount;            // To store DTIM Count

    unsigned int cfpEndSequenceNumber;  // CFP end timer sequence ID
    clocktype cfpEndTime;               // CFP end time for stations

    clocktype cfpTimeout;               // CFP timer delay for ack/data/etc
    BOOL cfpAckIsDue;                   // Waiting for an ack?

    DOT11_MacFrameType cfpLastFrameType;


    // PC specific variables
    DOT11_PcVars* pcVars;


    // AP specific variables
    DOT11_ApVars* apVars;
    //dot11s enable
    BOOL isDot11sEnabled;

/*********HT START*************************************************/
    UInt8 mcsIndexToUse;
    DOT11_NeighborHtInfoList*   htInfoList;
/*********HT END****************************************************/

    // Management specific variables.
    void* mngmtVars;                            // Structure for managment vars
    unsigned int managementSequenceNumber;      // Management timer sequence ID
    DOT11_StationConfigTable*   stationMIB;     // Station configuration MIB

    DOT11_AssociationType associationType;      // Station association type
    DOT11_NetworkType networkType;              // Network type

    BOOL  stationAuthenticated;         // Station is Autenticated
    BOOL  stationAssociated;            // Station is Associated
    BOOL BeaconAttempted;               // Once Attepted To transmit Beacon
                     //But still not Transmitted due to busy Medium
    BOOL beaconIsDue;                   // Beacon is pending
    unsigned int beaconSequenceNumber;  // Beacon timer sequence ID
    D_Clocktype beaconInterval;         // Target beacon interval
    clocktype lastCfpBeaconTime;     // last recieved Cfp start beacon
    clocktype NextCfpBeaconTime;    // Next Cfp Start Beacon
    clocktype beaconTime;               // Last or next beacon time
    clocktype lastBeaconRecieve;           // Last beacon time
    unsigned int beaconsMissed;         // Number of missed beacons


    // Statistics collection variables.
    BOOL printApStatistics;             // Output AP stats?
    BOOL printCfpStatistics;            // Output PCF stats?

    D_Int32 pktsToSend;                 // As received from Network layer
    //int pktsLostOverflow;             // Not used


    // Stats for DCF
    D_Int32 unicastPacketsSentDcf;
    D_Int32 broadcastPacketsSentDcf;

    int unicastPacketsGotDcf;
    int broadcastPacketsGotDcf;

    int ctsPacketsSentDcf;
    int rtsPacketsSentDcf;
    int ackPacketsSentDcf;

    int retxDueToCtsDcf;                // Retransmits due to CTS
    int retxDueToAckDcf;                // Retransmits due to ACK

    int retxAbortedRtsDcf;              // RTS retransmits aborted
    int pktsDroppedDcf;

    int beaconPacketsSent;              // Beacons
    int beaconPacketsGot;
//---------------------------Power-Save-Mode-Updates---------------------//
    // Statistics data in ad-hoc mode
    int atimFramesACKReceived;

    // Statistics data in PS mode
    // For STAs in Adhoc mode
    int atimFramesSent;
    int atimFramesReceived;

    // For STAs in infrastructure mode
    int psPollPacketsSent;
    int psModeDTIMFrameReceived;
    int psModeTIMFrameReceived;

    // For AP
    int psModeDTIMFrameSent;
    int psModeTIMFrameSent;
    int psPollRequestsReceived;
    // For AP mode and Adhoc mode
    int unicastDataPacketSentToPSModeSTAs;
    int broadcastDataPacketSentToPSModeSTAs;

    // For AP and STA in adhoc mode
    int psModeMacQueueDropPacket;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // Stats during PCF
    int beaconPacketsSentCfp;           // Beacons that start CFP
    int beaconPacketsGotCfp;

    int endPacketsSentCfp;
    int endPacketsGotCfp;

    int unicastPacketsSentCfp;
    int broadcastPacketsSentCfp;

    int unicastPacketsGotCfp;
    int broadcastPacketsGotCfp;

    int dataPacketsSentCfp;
    int dataPollPacketsSentCfp;
    int pollPacketsSentCfp;
    int nulldataPacketsSentCfp;
    //int ackPacketsSentCfp;            // Hard to isolate

    int dataPacketsGotCfp;
    int dataPollPacketsGotCfp;
    int pollPacketsGotCfp;
    int nulldataPacketsGotCfp;
    //int ackPacketsGotCfp;             // Hard to isolate

    int pollsSavedCfp;                  // Polls skipped by the PC


//---------------------DOT11e--Updates------------------------------------//

    BOOL             isQosEnabled;
    BOOL             associatedWithQAP;

    int currentACIndex;

    DOT11e_QBSSLoadElement  qBSSLoadElement;
    DOT11e_EDCAParameterSet edcaParamSet;
    TrafficSpec             TSPEC;
//---------------------DOT11e--HCCA Updates-------------------------------//
    Message* TxopMessage;               //First TS buffer packet at QSTA
    BOOL             isHCCAEnable;
    BOOL             associatedWithHC;
    DOT11e_HcVars*        hcVars;
//---------------------DOT11e--HCCA End-Updates--------------------------//

/*********HT START*************************************************/
    BOOL             isHTEnable;
    InputBuffer *inputBuffer;
    AmsduBuffer *amsduBuffer;
    
    BOOL isAmsduEnable;
    BOOL enableBigAmsdu;
    UInt16 maxAmsduSize;

    UInt16 vhtMaxMpduSize;
    UInt8 vhtApmduLengthExponent;
    UInt32 vhtMaxAmpduSize;

    BOOL isAmpduEnable;
    UInt8 ampduLengthExponent;
    UInt16 maxAmpduSize;

    BOOL enableImmediateBAAgreement;
    BOOL enableDelayedBAAgreement;
    BOOL enableHTDelayedBAAgreement;
    
    BOOL rifsMode;
    clocktype rifs;
    
    clocktype blockAckPolicyTimeout;
    
    clocktype inputBufferTimerExpireInterval;
    clocktype amsduBufferTimerExpireInterval;
    UInt32 macOutputQueueSize;

    DOT11n_IBSS_Station_Info *ibssConnectedStationsInfo;
/*********HT END*************************************************/

     BOOL isVHTEnable;

    // QoS statistics
    unsigned int totalNoOfQoSDataFrameSend;
    unsigned int totalNoOfnQoSDataFrameSend;
    unsigned int totalNoOfQoSDataFrameReceived;
    unsigned int totalNoOfnQoSDataFrameReceived;

//---------------------DOT11e--HCCA Updates---------------------------//
    unsigned int  totalNoOfQoSADDTS_REQFrameSent;
    unsigned int  totalNoOfQoSADDTS_RSPFrameSent;
    unsigned int  totalNoOfQoSADDTS_REQFrameReceived;
    unsigned int  totalNoOfQoSADDTS_RSPFrameReceived;

    // Total QoS NullData received
    unsigned int totalNoOfQoSDataCFPollsTransmitted;
    unsigned int totalNoOfQoSCFPollsTransmitted;
    unsigned int totalNoOfQoSCFAckTransmitted;
    //unsigned int totalQoSDataCFAckTransmitted;
    unsigned int totalQoSNullDataTransmitted;
    unsigned int totalNoOfQoSCFDataFrameSend;

    unsigned int totalQoSNullDataReceived;
    unsigned int totalNoOfQoSCFDataFramesReceived;
    unsigned int totalNoOfQoSDataCFPollsReceived;
    unsigned int totalNoOfQoSCFPollsReceived;
    unsigned int totalNoOfQoSCFAckReceived;

    unsigned int numPktsQueuedInAmsduBuffer;
    unsigned int numPktsDeQueuedFromAmsduBufferAsAmsdu;
    unsigned int numPktsDeQueuedFromAmsduBufferAsMsdu;

    unsigned int numPktsQueuedInOutputBuffer;
    unsigned int numPktsDequeuedFromOutputBuffer;

    unsigned int numPktsQueuedInInputBuffer;
    unsigned int numPktsDequeuedFromInputBuffer;

    unsigned int numPktsDroppedDueToSeqMismatch;
    unsigned int numPktsDroppedDueToInvalidState;

    unsigned int numAmpdusCreated;
    unsigned int numAmsdusSentUnderAmpdu;
    unsigned int numMpdusSentUnderAmpdu;
    unsigned int numAmpdusSent;
    unsigned int numAmpdusReceived;
    unsigned int numAmpdusRetried;
    unsigned int numAmpdusDropped;

    unsigned int numAmsdusCreated;
    unsigned int numMsdusSentUnderAmsdu;
    unsigned int numAmsdusSent;
    unsigned int numAmsdusReceived;
    unsigned int numAmsdusRetried;
    unsigned int numAmsdusDropped;

    unsigned int totalNoOfBlockAckReceived;
    unsigned int totalNoOfBlockAckSent;

    unsigned int totalNoOfCtsReceived;
    unsigned int totalNoOfCtsDropped;

    unsigned int totalNoPacketsDroppedDueToNoCts;
    unsigned int numPktsDroppedFromOutputBuffer;
    unsigned int numPktsDroppedFromAmsduBuffer;

    unsigned int totalNoPacketsSentAckPolicyBA;
    unsigned int totalNoPacketsReceivedAckPolicyBA;

    unsigned int totalNoImmBlockAckRequestSent;
    unsigned int totalNoImmBlockAckRequestReceived;
    unsigned int totalNoImmBlockAckRequestDropped;

    unsigned int totalNoDelBlockAckRequestSent;
    unsigned int totalNoDelBlockAckRequestReceived;
    unsigned int totalNoDelBlockAckRequestDropped;
   
     unsigned int totalNoDelBlockAckSent;
      unsigned int totalNoDelBlockAckDropped;
//---------------------DOT11e--HCCA End-Updates---------------------------//

//--------------------DOT11e-End-Updates---------------------------------//
//---------------------------Power-Save-Mode-Updates---------------------//
    BOOL            isIBSSATIMDurationActive;
    BOOL            isIBSSATIMDurationNeedToEnd;
    BOOL            isMoreDataPresent;

    // use to identify, that power save mode enabled at STAs
    BOOL            isPSModeEnabled;
    BOOL            joinedAPSupportPSMode;
    // For al STAs in PS mode
    BOOL            phyStatus;
    // These variables is used for STAs in Infrastructure mode
    BOOL            isReceiveDTIMFrame;
    BOOL            apHasUnicastDataToSend;
    BOOL            apHasBroadcastDataToSend;
    int             listenIntervals;
    int             assignAssociationId;
    clocktype       beaconExpirationTime;
    unsigned int    awakeTimerSequenceNo;
    clocktype       nextAwakeTime;
    // Used for AP, true if any joined STA in PS mode
    BOOL            isAnySTASupportPSMode;
    // Store last received beacon info
    int             beaconDTIMCount;
    int             beaconDTIMPeriod;
    // Keep the broadcast packets in this queue
    // This queue will use in both of the case ( AP mode and Adhoc mode )
    unsigned int    noOfBufferedBroadcastPacket;
    unsigned int    noOfReceivedBroadcastATIMFrame;
    Mac802Address     lastTxATIMDestAddress;
    BOOL            transmitBroadcastATIMFrame;
    Queue*          broadcastQueue;

    // These variables is used only in Ad hoc mode
    BOOL            stationHasReceivedBeconInAdhocMode;
    BOOL            isPSModeEnabledInAdHocMode;
    BOOL            receiveOrTxPAcketInATIMWindow;
    unsigned short  atimDuration;
    DOT11_AttachedNodeList* attachNodeList;
    DOT11_AttachedNodeList* roundRobinItr;
    DOT11_ReceivedATIMFrameList* receiveNodeList;
    // Temporary hold current message and NextHopAddress information
    Queue*          tempQueue;
    DOT11_AttachedNodeList* tempAttachNodeList;
    UInt32 broadcastQueueSize;
    UInt32 unicastQueueSize;
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // Trace related variables
    DOT11_TraceType trace;
    int currentTsid;

    RandomSeed seed;

    BOOL            QosFrameSent;
    int             TID;
    int             TSID;

    // dot11s. Mesh core Dot11 variables.
    BOOL isMP;
    BOOL isMAP;
    DOT11s_Data* mp;

    // Currently used only by 802.11s.
    LinkedList* mgmtQueue;
    LinkedList* dataQueue;
    DOT11s_FrameInfo* txFrameInfo;

    LinkedList* mngmtQueue;             //use these in Dot11 Management
    BOOL ActiveScanShortTimerFunctional;
    BOOL MayReceiveProbeResponce;
    BOOL managementFrameDequeued;
    DOT11_FrameInfo* dot11TxFrameInfo;
    BOOL mgmtSendResponse;
    BOOL ManagementResetPending;
    double thresholdSignalStrength;
    BOOL ScanStarted;
    BOOL stationCheckTimerStarted;

    BOOL waitForProbeDelay;
    void* macController;
};


//--------------------------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------------------------

/**
FUNCTION   :: MacDot11MacAddressToStr
LAYER      :: MAC
PURPOSE    :: Convert addr to hex format e.g. [aa-bb-cc-dd-ee-ff]
PARAMETERS ::
+ addrStr   : char*         : Converted address string
+ addr      : Mac802Address   : Address of node
RETURN     :: void
NOTES      :: This is useful in debug trace for consistent
                columnar formatting.
**/
static
void MacDot11MacAddressToStr(
    char* addrStr,
    const Mac802Address* macAddr)
{
    sprintf(addrStr, "[%02X-%02X-%02X-%02X-%02X-%02X]",
        macAddr->byte[0],
        macAddr->byte[1],
        macAddr->byte[2],
        macAddr->byte[3],
        macAddr->byte[4],
        macAddr->byte[5]);
}
//--------------------------------------------------------------------------
//  NAME:        MacDot11DeleteAssociationId.
//  PURPOSE:     Handle timers and layer messages.
//  PARAMETERS:  Node* node
//                  Node handling the incoming messages
//               int interfaceIndex
//                  Interface index
//               Message* msg
//                  Message for node to interpret.
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void MacDot11DeleteAssociationId(Node* node,
     MacDataDot11* dot11,
     DOT11_ApStationListItem* stationItem);

//--------------------------------------------------------------------------
//  NAME:        MacDot11Trace
//  PURPOSE:     Common entry routine for trace.
//
//               Placeholder for code to be inserted.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message* msg
//                  Flag which is either "Send" or "Receive" or comment
//               FILE* fp
//                  File pointer
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11Trace(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    const char *flag)
{

    // For DEBUG_QA
    if (!DEBUG_QA)
    {
        return;
    }

    if (strcmp(flag, "Send") && strcmp(flag, "Receive"))
    {
        printf("\tNode %d -- %s\n", node->nodeId, flag);
        return;
    }

    unsigned char* frame_type;
    DOT11_ShortControlFrame* hdr ;
    DOT11e_FrameHdr* hdr1;
    DOT11_FrameHdr* hdr2;
    DOT11s_FrameHdr* meshHdr;

    DOT11s_FrameHdr sHdr;

    char timeStr[100];
    ctoa(node->getNodeTime(), timeStr);
    printf("%d Current time is %s\n", node->nodeId,
        timeStr);

    char srcAdd[24],destAdd[24],addr3[24],addr4[24];

    if (msg != NULL)
    {
        char buf[20];
        char buf1[20];

        frame_type = (unsigned char *)MESSAGE_ReturnPacket(msg);
        hdr = (DOT11_ShortControlFrame *)MESSAGE_ReturnPacket(msg);
        MacDot11MacAddressToStr(destAdd, &hdr->destAddr);


        {
            switch(hdr->frameType)
            {
            case DOT11_ASSOC_REQ:
                sprintf(buf, "ASSOC_REQ");
                break;
            case DOT11_ASSOC_RESP:
                sprintf(buf, "ASSOC_RESP");
                break;
            case DOT11_REASSOC_REQ:
                sprintf(buf, "REASSOC_REQ");
                break;
            case DOT11_REASSOC_RESP:
                sprintf(buf, "REASSOC_RESP");
                break;
            case DOT11_BEACON :
                sprintf(buf, "BEACON");
                break;
            case DOT11_AUTHENTICATION:
                sprintf(buf, "AUTHENTICATION");
                break;

//---------------------DOT11e--HCCA-Updates-------------------------------//
            case DOT11_ACTION:
                sprintf(buf, "ACTION");
                break;
//---------------------DOT11e--HCCA-End-Updates---------------------------//
            case DOT11_RTS:
                sprintf(buf, "RTS");
                break;
            case DOT11_CTS:
                sprintf(buf, "CTS");
                break;
            case DOT11_ACK :
                sprintf(buf, "ACK");
                break;
            case DOT11_DATA:
                sprintf(buf, "DATA");
                hdr2 = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
                MacDot11MacAddressToStr(destAdd, &hdr2->destAddr);
                MacDot11MacAddressToStr(srcAdd, &hdr2->sourceAddr);
                MacDot11MacAddressToStr(addr3, &hdr2->address3);
                break;
            case DOT11_QOS_DATA:
                sprintf(buf, "QOS_DATA");
                hdr1 = (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(msg);
                MacDot11MacAddressToStr(destAdd, &hdr1->destAddr);
                MacDot11MacAddressToStr(srcAdd, &hdr1->sourceAddr);
                MacDot11MacAddressToStr(addr3, &hdr1->address3);
                break;
//---------------------------Power-Save-Mode-Updates---------------------//
            case DOT11_ATIM:
                sprintf(buf, "ATIM");
                hdr2 = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
                MacDot11MacAddressToStr(destAdd, &hdr2->destAddr);
                MacDot11MacAddressToStr(srcAdd, &hdr2->sourceAddr);
                MacDot11MacAddressToStr(addr3, &hdr2->address3);
                break;
            case DOT11_PS_POLL:
                sprintf(buf, "PS_POLL");
                hdr2 = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
                MacDot11MacAddressToStr(destAdd, &hdr2->destAddr);
                MacDot11MacAddressToStr(srcAdd, &hdr2->sourceAddr);
                MacDot11MacAddressToStr(addr3, &hdr2->address3);
                break;
//---------------------------Power-Save-Mode-End-Updates-----------------//
            // dot11s. Trace Mesh data.
            case DOT11_MESH_DATA:
                sprintf(buf, "MESH_DATA");
                memcpy(&sHdr,MESSAGE_ReturnPacket(msg),
                    sizeof(DOT11s_FrameHdr));
                meshHdr = &sHdr;
                MacDot11MacAddressToStr(destAdd, &meshHdr->destAddr);
                MacDot11MacAddressToStr(srcAdd, &meshHdr->sourceAddr);
                MacDot11MacAddressToStr(addr3, &meshHdr->address3);
                MacDot11MacAddressToStr(addr4, &meshHdr->address4);
                break;
            default:
                sprintf( buf, "%d", hdr->frameType);
                break;
            }

            switch(dot11->state)
            {
            case DOT11_S_IDLE:
                sprintf(buf1, "IDLE");
                break;
            case DOT11_S_WFNAV:
                sprintf(buf1, "WFNAV");
                break;
            case DOT11_S_WFJOIN:
                sprintf(buf1, "WFJOIN");
                break;
            case DOT11_S_WFCTS:
                sprintf(buf1, "WFCTS");
                break;
            case DOT11_S_WFDATA:
                sprintf(buf1, "WFDATA");
                break;
            case DOT11_S_WFACK:
                sprintf(buf1, "WFACK");
                break;
            case DOT11_S_WFMANAGEMENT:
                sprintf(buf1, "WFMGMT");
                break;
            case DOT11_S_WFBEACON:
                sprintf(buf1, "WFBEACON");
                break;
            case DOT11_X_RTS:
                sprintf(buf1, "RTS");
                break;
            case DOT11_X_CTS:
                sprintf(buf1, "CTS");
                break;
            case DOT11_X_UNICAST:
                sprintf(buf1, "UNICAST");
                break;
            case DOT11_X_BROADCAST:
                sprintf(buf1, "BROADCAST");
                break;
            case DOT11_X_ACK:
                sprintf(buf1, "ACK");
                break;
            case DOT11_X_MANAGEMENT:
                sprintf(buf1, "MGMT");
                break;
            case DOT11_X_BEACON:
                sprintf(buf1, "BEACON");
                break;
//---------------------------Power-Save-Mode-Updates---------------------//
            case DOT11_S_WFIBSSBEACON:
                sprintf(buf1, "WFIBSSBEACON");
                break;
            case DOT11_S_WFIBSSJITTER:
                sprintf(buf1, "WFIBSSJITTER");
                break;
            case DOT11_X_IBSSBEACON:
                sprintf(buf1, "IBSSBEACON");
                break;
            case DOT11_X_PSPOLL:
                sprintf(buf1, "PSPOLL");
                break;
//---------------------------Power-Save-Mode-End-Updates-----------------//
            default:
                sprintf( buf1, "%d", dot11->state );
                break;
            }

            if (*frame_type == DOT11_QOS_DATA)
            {
                printf("%3u\t %6s\t %5d\t %20s\t %19s\t %20s\t %2d\t %2d\t %2d|%2d\t %s (%s)\n",
                    node->nodeId,
                    buf1,
                    hdr1->duration,
                    srcAdd,
                    destAdd,
                    addr3,
                    hdr1->seqNo,
                    hdr1->fragId,
                    hdr1->qoSControl.TID,
                    hdr1->qoSControl.TXOP,
                    buf,
                    flag);
            }
            else if ((*frame_type == DOT11_DATA)
                || (*frame_type == DOT11_ATIM)
                || (*frame_type == DOT11_PS_POLL)
                )

            {
                printf("%3u\t %6s\t %5d\t %19s\t %19s\t %19s\t %2d\t %2d\t %2d|%2d\t %s (%s)\n",
                    node->nodeId,
                    buf1,
                    hdr2->duration,
                    srcAdd,
                    destAdd,
                    addr3,
                    hdr2->seqNo,
                    hdr2->fragId,
                    -1, -1,
                    buf,
                    flag);
            }
            // dot11s. Trace mesh data details.
            else if (*frame_type == DOT11_MESH_DATA)
            {
                printf("%3u\t %6s\t %5d\t %19s\t %19s\t %19s\t %19s\t %2d\t %2d\t %3d | %3d | %1d | %1d\t %s(%s)\n",
                    node->nodeId,
                    buf1,
                    meshHdr->duration,
                    srcAdd,
                    destAdd,
                    addr3,
                    addr4,
                    meshHdr->seqNo,
                    meshHdr->fragId,
                    meshHdr->fwdControl.GetE2eSeqNo(),
                    meshHdr->fwdControl.GetTTL(),
                    meshHdr->fwdControl.GetUsesTbr(),
                    meshHdr->fwdControl.GetExMesh(),
                    buf,
                    flag);
            }
            else
            {
                printf("%3u\t %6s\t %5d\t %3x\t %19s\t %3x\t %2d\t %2d\t %2d|%2d\t %s (%s)\n",
                    node->nodeId,
                    buf1,
                    hdr->duration,
                    -1,
                    destAdd,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    buf,
                    flag);

            }
       }

       return;
    }
//---------------------------Power-Save-Mode-Updates---------------------//
    else{
        if (DEBUG_PS){
            if (dot11->isPSModeEnabled || dot11->isPSModeEnabledInAdHocMode
                || ((dot11->stationType == DOT11_STA_AP)
                && (dot11->apVars->isAPSupportPSMode == TRUE))){
                printf("Node: %3u\t (%s)\n",node->nodeId,flag);
            }
        }
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11TraceInit
//  PURPOSE:     Initialize trace setting from user configuration file.
//
//               Placeholder for code to be inserted.
//
//  PARAMETERS:  Node* node
//                  Pointer to node
//               const NodeInput* nodeInput
//                  Pointer to node input
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
void MacDot11TraceInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11)
{
    // For DEBUG_QA
    if (!DEBUG_QA)
    {
        return;
    }

    static int i = 0;

    if (i == 0)
    {
        printf(" Dot11e header format as shown in Trace \n "
            "node->nodeId \n "
            "dot11->state \n "
            "hdr->duration \n "
            "hdr->sourceAddr \n "
            "hdr->destAddr \n "
            "hdr->address3 \n "
            "hdr->seqNo \n "
            "hdr->fragId \n "
            "hdr->QoS \n"
            "hdr->frameType (Sending or Recv) \n ");

            i++;
    }

    return;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11MicroToNanosecond
//  PURPOSE:     Convert a value in micro-seconds to clocktype.
//  PARAMETERS:  int us
//                  Value in micro-seconds
//  RETURN:      Value in nano-seconds
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
clocktype MacDot11MicroToNanosecond(int us)
{
    return (clocktype) us * MICRO_SECOND;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11NanoToMicrosecond
//  PURPOSE:     Convert a value from clocktype to micro-seconds.
//               Return value is rounded up for fractional micro-seconds.
//  PARAMETERS:  clocktype ns
//                  Value in nano-seconds
//  RETURN:      Value in micro-seconds
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
int MacDot11NanoToMicrosecond(clocktype ns)
{
    return (int) ((ns + 999) / MICRO_SECOND);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11TUsToClocktype
//  PURPOSE:     Convert a value in Time Units to clocktype
//               (1 TU = 1024 micro-seconds)
//  PARAMETERS:  unsigned short tu
//                  Value in TUs (1 to 32767)
//  RETURN:      Value in nanoseconds
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
clocktype MacDot11TUsToClocktype(unsigned short tu)
{
    return (clocktype) tu * DOT11_TIME_UNIT;
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11ClocktypeToTUs
//  PURPOSE:     Convert time from clocktype to Time Units
//               (1 TU = 1024 micro-seconds)
//  PARAMETERS:  clocktype timeValue
//                  Value in nanoseconds
//  RETURN:      Value in TUs
//  ASSUMPTION:  Integer division
//--------------------------------------------------------------------------
static //inline
unsigned short MacDot11ClocktypeToTUs(
    clocktype timeValue)
{
    return (unsigned short) (timeValue / DOT11_TIME_UNIT);
}


//--------------------------------------------------------------------------
//  NAME:        MacDot11IsFrameAUnicastDataTypeForPromiscuousMode
//  PURPOSE:     Determine if the frame is one of those which
//               carries data e.g. CF_DATA_ACK or DOT11_CF_DATA_POLL.
//
//               A directed frame from a BSS station sent to an AP/PC
//               with final destination as broadcast in not considered
//               a unicast data type for promiscuous mode.
//
//  PARAMETERS:  DOT11MacFrameType frameType
//                  Frame type.
//               Message* msg
//                  Received message.
//  RETURN:      TRUE if frame type is one of those which contains data
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
static //inline
BOOL MacDot11IsFrameAUnicastDataTypeForPromiscuousMode(
    DOT11_MacFrameType frameType,
    Message* msg)
{
    BOOL isDataType = FALSE;


    switch (frameType)
    {
        case DOT11_DATA:

//--------------------DOT11e-End-Updates---------------------------------//
        case DOT11_QOS_DATA:
//--------------------DOT11e-End-Updates---------------------------------//

        case DOT11_CF_DATA_ACK:
        case DOT11_CF_DATA_POLL:
        case DOT11_CF_DATA_POLL_ACK:
            isDataType = TRUE;
            break;

        default:
            break;

    } //switch

    if (isDataType) {
        // Filter out broadcasts sent by BSS stations to
        // the AP/PC as unicasts.

//---------------------DOT11e--Updates------------------------------------//
        if (frameType == DOT11_QOS_DATA)
        {
            DOT11e_FrameHdr* fHdr;
            fHdr = (DOT11e_FrameHdr*) MESSAGE_ReturnPacket(msg);
            if (fHdr->address3 == ANY_MAC802) {
                isDataType = FALSE;
            }
        }
//--------------------DOT11e-End-Updates---------------------------------//
        else {
            DOT11_FrameHdr* fHdr;
            fHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
            if (fHdr->address3 == ANY_MAC802) {
                isDataType = FALSE;
            }
        }
    }

    return isDataType;
}

/**
FUNCTION   :: MacDot11sMgmtQueue_PeekPacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the management queue,
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
TODO       :: Add Dot11 stats if required
**/

BOOL MacDot11MgmtQueue_PeekPacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo** frameInfo);
/**
FUNCTION   :: MacDot11sMgmtQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the management queue,
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
TODO       :: Add Dot11 stats if required
**/

BOOL MacDot11MgmtQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: MacDot11MgmtQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the management queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
TODO       :: Add stats if required
**/

BOOL MacDot11MgmtQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo* frameInfo);
/**
FUNCTION   :: MacDot11MgmtQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Deletes items in management queue & queue itself.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

void MacDot11MgmtQueue_Finalize(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: MacDot11DataQueue_DequeuePacket
LAYER      :: MAC
PURPOSE    :: Dequeue a frame from the data queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11_FrameInfo** : Parameter for dequeued frame info
RETURN     :: BOOL          : TRUE if dequeue is successful
TODO       :: Add stats if required
**/

BOOL MacDot11DataQueue_DequeuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo** frameInfo);

/**
FUNCTION   :: MacDot11DataQueue_EnqueuePacket
LAYER      :: MAC
PURPOSE    :: Enqueue a frame in the single data queue.
                Notifies if it is the only item in the queue.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ frameInfo : DOT11_FrameInfo* : Enqueued frame info
RETURN     :: BOOL          : TRUE if enqueue is successful.
TODO       :: Add stats if required
**/

BOOL MacDot11DataQueue_EnqueuePacket(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo* frameInfo);

/**
FUNCTION   :: MacDot11DataQueue_Finalize
LAYER      :: MAC
PURPOSE    :: Free the local data queue, including items in it.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
RETURN     :: void
**/

void MacDot11DataQueue_Finalize(
    Node* node,
    MacDataDot11* dot11);

/**
FUNCTION   :: MacDot11ReceiveNetworkLayerPacket
LAYER      :: MAC
PURPOSE    :: Process packet from Network layer at Node.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ nextHopAddr   : Mac802Address : address of next hop
+ ipNextHopAddr : Mac802Address : address of next hop from IP
+ ipDestAddr    : Mac802Address : destination address from IP
+ ipSourceAddr  : Mac802Address : source address from IP
RETURN     :: void
**/

static
void MacDot11ReceiveNetworkLayerPacket(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    Mac802Address nextHopAddr,
    Mac802Address ipNextHopAddr,
    Mac802Address ipDestAddr,
    Mac802Address ipSourceAddr);
//---------------------DOT11e--Updates------------------------------------//

/// 
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
/// \param msg  Pointer to the Message
/// \param priority  Priority
/// \param currentNextHopAddress  next hop address
/// \param networkType  Pointer to network type
///
void MacDot11eTrafficSpecification(
        Node* node,
        MacDataDot11* dot11,
        Message* msg,
        TosType priority,
        Mac802Address currentNextHopAddress,
        int networkType);

/// 
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
/// \return TRUE or FALSE
static BOOL MacDot11IsQoSEnabled(Node* node, MacDataDot11* dot11)
{
    return (dot11->isQosEnabled);
}



/// 
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
/// \return TRUE or FALSE
static BOOL MacDot11IsAssociatedWithQAP(Node* node, MacDataDot11* dot11)
{
    return (dot11->associatedWithQAP);
}

/// 
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
/// \param msg  Pointer to the Message
/// \param priority  Priority
/// \param currentNextHopAddress  next hop address
/// \param networkType  Pointer to network type
///
void MacDot11eEDCA_TOXPSchedulerRetrieveFrame(
        Node* node,
        MacDataDot11* dot11,
        Message** msg,
        TosType* priority,
        Mac802Address* currentNextHopAddress,
        int* networkType);

/// To set the Current message varriables
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
//void MacDot11eSetCurrentMessage(
//        Node* node,
//        MacDataDot11* dot11);

/// Set current Message after negociating in between the ACs.
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
/// \param networkType  Network type
///
void MacDot11SetCurrentMessage(
        Node* node,
        MacDataDot11* dot11);

/// To print statistics
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
void MacDot11ACPrintStats(Node* node, MacDataDot11* dot11,
    int interfaceIndex);

/// 
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
/// \return TRUE or FALSE
BOOL MacDot11eIsAssociatedWithQAP(Node* node, MacDataDot11* dot11);

//--------------------DOT11e-End-Updates---------------------------------//

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11HandlePromiscuousMode.
//  PURPOSE:     Send remote packet to network layer.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Message *frame
//                  Frame
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline
void MacDot11HandlePromiscuousMode(
    Node* node,
    MacDataDot11* dot11,
    Message *frame,
    Mac802Address nextHop,
    Mac802Address destAddr)
{
    // DOT11e Updates
    DOT11_LongControlFrame* hdr =
        (DOT11_LongControlFrame*) MESSAGE_ReturnPacket(frame);
    BOOL frameTpeQoS = FALSE;
    BOOL frameTypeMesh = FALSE;
    #ifdef CYBER_LIB
    BOOL isDecrypt = TRUE;
    #endif
    Mac802Address sourceAddr = hdr->sourceAddr;

    if (hdr->frameType == DOT11_QOS_DATA)
    {
        frameTpeQoS = TRUE;
        MESSAGE_RemoveHeader(
                    node,
                    frame,
                    sizeof(DOT11e_FrameHdr),
                    TRACE_DOT11);
    }
    // dot11s. Remove header for promiscuous mode.
    else if (hdr->frameType == DOT11_MESH_DATA) {
        frameTypeMesh = TRUE;
        MESSAGE_RemoveHeader(
            node,
            frame,
            DOT11s_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);

    }
    else {
#ifdef CYBER_LIB
            BOOL protection = FALSE;
            if (hdr->frameFlags & DOT11_PROTECTED_FRAME)
            {
               protection = TRUE;
            }
#endif // CYBER_LIB

        MESSAGE_RemoveHeader(
            node,
            frame,
            sizeof(DOT11_FrameHdr),
            TRACE_DOT11);
#ifdef CYBER_LIB
        if (protection)
        {
            //If WEP/CCMP is OFF and Protection is also OFF
            isDecrypt = FALSE;
        }
#endif // CYBER_LIB
        //---------------------Bug Fix changes end-------------------------//
    }

#ifdef CYBER_LIB
    if (isDecrypt)
    {
#endif
    MAC_SneakPeekAtMacPacket(
        node,
        dot11->myMacData->interfaceIndex,
        frame,
        nextHop,
        destAddr);
#ifdef CYBER_LIB
    }
#endif


    if (frameTpeQoS)
    {
        MESSAGE_AddHeader(
                    node,
                    frame,
                    sizeof(DOT11e_FrameHdr),
                    TRACE_DOT11);
    }
    // dot11s. Add back header after promiscuous mode
    else if (frameTypeMesh) {
        MESSAGE_AddHeader(
            node,
            frame,
            DOT11s_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);
    }
    else {
        MESSAGE_AddHeader(
            node,
            frame,
            DOT11_DATA_FRAME_HDR_SIZE,
            TRACE_DOT11);
    }
}



//--------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// FUNCTION     MacDot11Init
// PURPOSE      Initialization function for dot11 protocol of MAC layer
// PARAMETERS   Node* node
//                  Node being initialized.
//              NodeInput* nodeInput
//                  Structure containing contents of input file.
//              SubnetMemberData* subnetList
//                  Number of nodes in subnet.
//              int nodesInSubnet
//                  Number of nodes in subnet.
//              int subnetListIndex
//              NodeAddress subnetAddress
//                  Subnet address.
//              int numHostBits
//                  number of host bits.
//              BOOL isQosEnabled
//                  True for DOT11e
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11Init(
   Node* node, int interfaceIndex, const NodeInput* nodeInput,
   PhyModel phyModel,
   SubnetMemberData* subnetList, int nodesInSubnet, int subnetListIndex,
   NodeAddress subnetAddress, int numHostBits,
   BOOL isQosEnabled = FALSE,
   NetworkType networkType = NETWORK_IPV4,
   in6_addr* ipv6SubnetAddr = 0,
   unsigned int prefixLength = 0);

//--------------------------------------------------------------------------
// FUNCTION     MacDot11Layer
// PURPOSE      Models the behaviour of the MAC layer with the Dot11
//              protocol on receiving the message enclosed in msgHdr
// PARAMETERS   Node *node
//                  Node which received the message.
//              int interfaceIndex
//                  Interface index.
//              Message* msg
//                  Message received by the layer.
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11Layer(Node* node, int interfaceIndex, Message* msg);


//--------------------------------------------------------------------------
// FUNCTION     MacDot11Finalize
// PURPOSE      Called at the end of simulation to collect the results of
//              the simulation of Dot11 protocol of the MAC Layer.
// PARAMETERS   Node* node
//                  Node which received the message.
//              int interfaceIndex
//                  Interface index.
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11Finalize(Node* node, int interfaceIndex);


//--------------------------------------------------------------------------
// NAME         MacDot11NetworkLayerHasPacketToSend
// PURPOSE      To notify dot11 that network has something to send
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11NetworkLayerHasPacketToSend(
   Node* node, MacDataDot11* dot11);


//--------------------------------------------------------------------------
// NAME         MacDot11ReceivePacketFromPhy
// PURPOSE      To recieve packet from the physical layer
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
//              Message* msg
//                  Message received by the layer.
// RETURN       None.
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11ReceivePacketFromPhy(
   Node* node, MacDataDot11* dot11, Message* msg);


//--------------------------------------------------------------------------
// NAME         MacDot11ReceivePhyStatusChangeNotification
// PURPOSE      Receive notification of status change in physical layer
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure.
//              PhyStatusType oldPhyStatus
//                  The old physical status.
//              PhyStatusType newPhyStatus
//                  The new physical status.
//              clocktype receiveDuration
//                  The receiving duration.
//              Message* potentialIncomingPacket
//                  Incoming packet.
// RETURN       None.
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11ReceivePhyStatusChangeNotification(
   Node* node,
   MacDataDot11* dot11,
   PhyStatusType oldPhyStatus,
   PhyStatusType newPhyStatus,
   clocktype receiveDuration,
   const Message* potentialIncomingPacket);

//--------------------------------------------------------------------------
// NAME         MacDot11DataQueueHasPacketToSend
// PURPOSE      Notify dot11 that local data queue has something to send
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11DataQueueHasPacketToSend(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
// NAME         MacDot11MgmtQueueHasPacketToSend
// PURPOSE      Notify dot11 that management queue has something to send
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11MgmtQueueHasPacketToSend(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11MngmtQueueHasPacketToSend
//  PURPOSE:     Called when management queue transition from empty.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//  NOTES:       This queue is currently used only by Mesh Points
//               and does not have QoS considerations.
//--------------------------------------------------------------------------
// dot11. New function for Mgmt queue notification.
void MacDot11MngmtQueueHasPacketToSend(
    Node* node,
    MacDataDot11* dot11);

DOT11_NeighborHtInfo* MacDot11nAddNeighHtInfo(
    DOT11_NeighborHtInfoList* htInfoList,
    const Mac802Address& address,
    BOOL  isAp,
    unsigned int maxMcsIdx,
    BOOL   stbcCapable,
    BOOL   shortGiCapable);

DOT11_NeighborHtInfo* MacDot11nGetNeighHtInfo(
    DOT11_NeighborHtInfoList* htInfoList,
    const Mac802Address& neighAddr);

void MacDot11nRemoveNeighHtInfo(
    DOT11_NeighborHtInfoList* htInfoList,
    const Mac802Address& neighAddr);

void MacDot11nClearNeighHtInfoList(MacDataDot11* dot11);

void MacDot11nUpdNeighborHtInfo(
        Node* node, 
        MacDataDot11* dot11,
        const Mac802Address& neighborAddr, 
        const DOT11_HT_CapabilityElement& htCapabilityElement);

void MacDot11nAdjMscForOutgoingPacket(
    Node* node, MacDataDot11* dot11, DOT11_DataRateEntry* rateInfo);

void MacDot11nAdjMscForResponseTimeout(
    Node* node, MacDataDot11* dot11, DOT11_DataRateEntry* rateEntry);

MAC_PHY_TxRxVector MacDot11nSetTxVector(
    Node* node, 
    MacDataDot11* dot11,    
    unsigned int pktLength,
    DOT11_MacFrameType frameType,
    const Mac802Address& destAddr,
    const MAC_PHY_TxRxVector* rxFrameVector,
    BOOL tempCalc = FALSE);

#ifdef ADDON_DB
void MacDot11GetPacketProperty(Node *node, Message *msg, 
    int interfaceIndex, MacDBEventType eventType,
    StatsDBMacEventParam &macParam,
    BOOL &isMyFrame, BOOL &isAnyFrame);    
#endif

// Called by the Mac Summary Stats DB
//
// \param node  Pointer to the node
// \param dot11  Pointer to the input message
//
// \return TRUE or FALSE
void MacDot11GetPacketProperty(Node *node, Message *msg,
    int interfaceIndex, int & headerSize,
    Mac802Address &dstAddr,
    NodeId &destNodeId,
    BOOL &isCtrlFrame,
    BOOL &isMyFrame,
    BOOL &isAnyFrame);

// Called by the Mac Summary Stats DB
//
// \param node  Pointer to the node
// \param dot11  Pointer to the input message
//
// \return TRUE or FALSE
void MacDot11GetPacketProperty(Node *node, Message *msg, 
    int interfaceIndex, int & headerSize,
    std::string &dstAddr,
    BOOL &isCtrlFrame,
    BOOL &isMyFrame,
    BOOL &isAnyFrame);

//--------------------------------------------------------------------------
// NAME         MacDot11UpdateStatsSend
// PURPOSE      Update sending statistics
// PARAMETERS   Node* node
//                  Node which sent the message.
//              Message* msg
//                  The sent message
//              int interfaceIndex
//                  The interface index the packet was sent on
//              BOOL broadcast
//              BOOL uncast
//              BOOL data
//                  if it is a data frame
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11UpdateStatsSend(
    Node *node,
    Message* msg,
    int interfaceIndex,
    BOOL broadcast,
    BOOL unicast,
    BOOL data);
//--------------------------------------------------------------------------
// NAME         MacDot11UpdateStatsSend
// PURPOSE      Update sending statistics
// PARAMETERS   Node* node
//                  Node which sent the message.
//              Message* msg
//                  The sent message
//              int interfaceIndex
//                  The interface index the packet was sent on
//              BOOL broadcast
//              BOOL uncast
//              BOOL data
//                  if it is a data frame
//              Mac802Address destAddr
//                  Mac Adrress of the node that will receive this frame
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11UpdateStatsSend(
    Node *node,
    Message* msg,
    int interfaceIndex,
    BOOL broadcast,
    BOOL unicast,
    BOOL data,
    Mac802Address destAddr);
//--------------------------------------------------------------------------
// NAME         MacDot11UpdateStatsReceive
// PURPOSE      Update receiving statistics
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
//              Message* msg
//                  The sent message
//              int interfaceIndex
//                  The interface index the packet was received on
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11UpdateStatsReceive(
    Node *node,
    MacDataDot11* dot11,
    Message* msg,
    int interfaceIndex);
//--------------------------------------------------------------------------
// NAME         MacDot11UpdateStatsLeftQueue
// PURPOSE      Update mac queue statistics
// PARAMETERS   Node* node
//                  Node which received the message.
//              MacDataDot11* dot11
//                  Dot11 data structure
//              Message* msg
//                  The sent message
//              int interfaceIndex
//                  The interface index the packet was received on
//              Mac802Address sourceAddr
//                  Mac address that sent the packet originally
// RETURN       None
// NOTES        None
//--------------------------------------------------------------------------
void MacDot11UpdateStatsLeftQueue(
    Node *node,
    MacDataDot11* dot11,
    Message* msg,
    int interfaceIndex,
    Mac802Address sourceAddr);

/// \brief Function to check if Ac's has packet
///
/// \param node  Pointer to the node
/// \param dot11  Pointer to the MacData structure
///
/// \return TRUE or FALSE
BOOL MacDot11IsACsHasMessage(Node* node, MacDataDot11* dot11);

//--------------------------------------------------------------------------
//  NAME:        MacDot11ProcessNotMyFrame.
//  PURPOSE:     Handle frames that don't belong to this node.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               clocktype duration
//                  time
//               BOOL isARtsPacket
//                  Is a RTS packet
//               BOOL isAPollPacket
//                  Is a QoS Poll Packet
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ProcessNotMyFrame(
   Node* node, MacDataDot11* dot11,
   clocktype duration, BOOL isARtsPacket,BOOL isAPollPacket );

//--------------------------------------------------------------------------
//  NAME:        MacDot11ProcessMyFrame
//  PURPOSE:     Process a frame unicast frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address address
//                  Node address of station
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11ProcessMyFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

#endif /*MAC_DOT11_H*/
