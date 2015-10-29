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
 * \file mac_dot11-mgmt.h
 * \brief MAC management structures and helper functions.
 */

#ifndef MAC_DOT11_MGMT_H
#define MAC_DOT11_MGMT_H

#include "dvcs.h"
#include "mac_dot11-sta.h"
#include "mac_phy_802_11n.h"


//-------------------------------------------------------------------------
// #define's
//-------------------------------------------------------------------------
#define DOT11_R_NOT_IMPLEMENTED     -1
#define DOT11_R_NOT_SUPPORTED       -9999
#define DOT11_R_FAILED              0
#define DOT11_R_OK                  1

#define MAX_NUMBER_CHANNELLIST      96

//---------------------------Power-Save-Mode-Updates---------------------//
#define DOT11_STA_DEFAULT_CHANNEL         0
//---------------------------Power-Save-Mode-End-Updates-----------------//

#define DOT11_SCAN_DWELL_DEFAULT          200      // in TU's
#define DOT11_MANAGEMENT_DELAY_TIME       2        // in TU's
#define DOT11_PROBE_DELAY_TIME            10       // in TU's

#define DOT11_MANAGEMENT_TX_DELAY         (10 * MILLI_SECOND)

// Management header size is the same as a data frame
#define DOT11_MANAGEMENT_FRAME_HDR_SIZE     DOT11_DATA_FRAME_HDR_SIZE

#define DOT11_MANAGEMENT_STATS_LABEL      "802.11MGMT"

//---------------------------Power-Save-Mode-Updates---------------------//
#define MacDot11IsPSModeEnabledAtSTA(frameFlags)\
    ((frameFlags & DOT11_POWER_MGMT) == DOT11_POWER_MGMT)
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
// FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

//---------------------------Power-Save-Mode-Updates---------------------//
BOOL MacDot11ManagementBuildATIMFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destinationNodeAddress);

void MacDot11StationTransmitPSPollFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr);
//---------------------------Power-Save-Mode-End-Updates-----------------//
void MacDot11ManagementInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType,
    BOOL printStatistics = TRUE);

BOOL MacDot11ManagementAttemptToTransmitFrame(
    Node* node,
    MacDataDot11* dot11);

void MacDot11ManagementFrameTransmitted(
    Node* node,
    MacDataDot11* dot11);

void MacDot11ManagementRetransmitFrame(
    Node* node,
    MacDataDot11* dot11);

void MacDot11ManagementProcessFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

void MacDot11ManagementHandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg);

void MacDot11ManagementStartListeningToChannel(
    Node* node,
    int  phyNumber,
    short channelIndex,
    MacDataDot11* dot11);

void MacDot11ManagementStopListeningToChannel(
    Node* node,
    int phyNumber,
    short channelIndex);

void MacDot11ManagementPrintStats(
    Node* node,
    MacDataDot11* dot11,
    int interfaceIndex);

//--------------------------------------------------------------------------
// typedef's enums
//--------------------------------------------------------------------------

// Scan type
typedef enum {
    DOT11_SCAN_UNKNOWN,
    DOT11_SCAN_PASSIVE,
    DOT11_SCAN_ACTIVE,
    DOT11_SCAN_DISABLED
}DOT11_ScanType;


// States for station management
typedef enum {
    DOT11_S_M_INIT,               // Init on power up
    DOT11_S_M_IDLE,               // Idle
    DOT11_S_M_SCAN,               // Start scanning
    DOT11_S_M_WFSCAN,             // Waiting channel scan to finish
    DOT11_S_M_WFSCAN_PROBING,     // Waiting for a probe response
    DOT11_S_M_SCAN_FAILED,        // Scan failed to find an AP
    DOT11_S_M_SCAN_FINISHED,      // Scan finished
    DOT11_S_M_WFBEACON,           // Waiting for Beacon
    DOT11_S_M_WFAUTH,             // Waithing for Authentication
    DOT11_S_M_AUTH_FINISHED,      // Authentication finished
    DOT11_S_M_WFASSOC,            // Waithing for Association
    DOT11_S_M_ASSOC_FINISHED,     // Association finished
    DOT11_S_M_WFREASSOC,          // Waiting for Reassociation
    DOT11_S_M_JOINED,             // Station joined

    DOT11_S_M_BEACON,             // AP normal state

    DOT11_S_M_DISABLED,            // Management Disabled
//-------------------------------- Dot11e-HCCA-Updates -------------------------//
    DOT11_S_M_WFADDTS_RSP              // Waiting for ADDTS_RSP
//-------------------------------- Dot11e-HCCA-Updates -------------------------//

} DOT11_MacStationManagementStates;


typedef enum {
    DOT11_FRAME_UNKNOWN,            // Unknown
    DOT11_FRAME_CLASS1,             // Unauthenticatged/Unassociated
    DOT11_FRAME_CLASS2,             // Authenticatged/Unassociated
    DOT11_FRAME_CLASS3              // Authenticatged/Associated
} DOT11_MacFrameClass;


typedef enum {
    DOT11_RC_UNUSED,
    DOT11_RC_UNSPEC_REASON,
    DOT11_RC_AUTH_NOT_VALID,
    DOT11_RC_DEAUTH_LEAVING,
    DOT11_RC_DISASSOC_INACTIVE,
    DOT11_RC_DISASSOC_AP_BUSY,
    DOT11_RC_CLASS2_NONAUTH,
    DOT11_RC_CLASS3_NONASSOC,
    DOT11_RC_DISASSOC_STA_HAS_LEFT,
    DOT11_RC_CANT_ASSOC_NONAUTH,
//-------------------------------- Dot11e-HCCA-Updates -------------------------//
    DOT11_RC_QOS_UNSPEC_REASON=32,
    DOT11_RC_QAP_LESS_BANDWIDTH,
    DOT11_RC_EXCESS_FRAME_UNACK,
    DOT11_RC_QSTA_TXOPLIMIT_EXCEED,
    DOT11_RC_QSTA_RESETTING_QBSS,
    DOT11_RC_QSTA_NOTUSING_MECH,
    DOT11_RC_QSTA_SETTING_RQ,
    DOT11_RC_REQ_DUETO_TIMEOUT,
    DOT11_RC_CIPHER_SUIT
//-------------------------------- Dot11e-HCCA-End-Updates ---------------------//
} DOT11_ReasonCode;


typedef enum {
    DOT11_SC_SUCCESSFUL,
    DOT11_SC_UNSPEC_FAILURE,
    DOT11_SC_CAPS_UNSUPPORTED = 10,
    DOT11_SC_REASSOC_NO_ASSOC,
    DOT11_SC_ASSOC_DENIED_UNSPEC,
    DOT11_SC_UNSUPPORTED_AUTHALG,
    DOT11_SC_AUTH_SEQ_FAIL,
    DOT11_SC_CHALLANG_FAIL,
    DOT11_SC_AUTH_TIMEOUT,
    DOT11_SC_ASSOC_DENIED_BUSY,
    DOT11_SC_ASSOC_DENIED_RATES,
    DOT11_SC_ASSOC_DENIED_NOSHORT,
    DOT11_SC_ASSOC_DENIED_NOPBCC,
    DOT11_SC_ASSOC_DENIED_NOAGILITY
} DOT11_StatusCode;


typedef enum {
    DOT11_EID_SSID,
    DOT11_EID_SUPP_RATES,
    DOT11_EID_FH_PARMS,
    DOT11_EID_DS_PARMS,
    DOT11_EID_CF_PARMS,
    DOT11_EID_TIM,
    DOT11_EID_IBSS_PARMS,
    DOT11_EID_CHALLENGE = 16
} DOT11_ElementIdTypes;

//--------------------------------------------------------------------------
// typedef's struct
//--------------------------------------------------------------------------

// Element structures
typedef struct struct_mac_dot11_ElementChallengeText{
    char                    hdr;               //4  padding of 3
    DOT11_ElementIdTypes  elementId;           //4
    char                    text[64];          //64
} DOT11_ChallengeText;                        //-----
                                              // 72
// Management frame header.
// This is size 28 in the standard and matches the defined Dot11FrameHdr.
// Not defined separately.

// Probe request and response frames
typedef struct struct_mac_dot11_ProbeReqFrame {
    DOT11_FrameHdr         hdr;                          //28
    char                   SSID[DOT11_SSID_MAX_LENGTH];  //32
                                                        //----
                                                        // 60
} DOT11_ProbeReqFrame;


// Authentication request and response frames
// Authentication algorithm types

typedef struct struct_mac_dot11_AuthFrame {       //Should be       Actually
    DOT11_FrameHdr         hdr;                   // 28                 28
    unsigned short         algorithmId;           // 2                   2
    unsigned short          transSeqNo;            // 2                  2
    unsigned short         statusCode;              // 2                 2
                                                   //---                ---
                                                   //34                  34
} DOT11_AuthFrame;

//---------------------------Power-Save-Mode-Updates---------------------//
typedef  DOT11_FrameHdr  DOT11_ATIMFrame;
//---------------------------Power-Save-Mode-End-Updates-----------------//

//---------------------DOT11e--Updates------------------------------------//

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

typedef struct struct_mac_dot11_ProbeRespFrame {//Should be     Actually
    DOT11_FrameHdr         hdr;                  // 28             28

    unsigned short          beaconInterval;      //  2              2
    DOT11_CapabilityInfo    capInfo;             //  2              2
    clocktype               timeStamp;           //  8              8
    char                    ssid[32];            // 32             32
                                                 //---             ---
} DOT11_ProbeRespFrame;                          // 72              72

typedef struct struct_mac_dot11e_ProbeRespFrame { //Should be     Actually
    DOT11_FrameHdr         hdr;                   // 28              28
    unsigned short          beaconInterval;       //  2               2
    DOT11e_CapabilityInfo    capInfo;             //  2               2
    clocktype               timeStamp;            //  8               8
    char                    ssid[32];             // 32              32
                                                  //---              ---
                                                  // 72              72
} DOT11e_ProbeRespFrame;


//--------------------DOT11e-End-Updates---------------------------------//
// Association/Reassociation request and response frames
#ifndef CAP_INFO
#define CAP_INFO
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
#endif

typedef struct struct_mac_dot11_AssocReqFrame {          //Should be  Actually
    DOT11_FrameHdr           hdr;                        // 28          28
    DOT11_CapabilityInfo     capInfo;                    //  2           2
    unsigned short          listenInterval;              //  2           2
    char                    ssid[DOT11_SSID_MAX_LENGTH]; // 32          32
                                                         // ---        ---
                                                         // 64          64
} DOT11_AssocReqFrame;

//---------------------DOT11e--Updates------------------------------------//
/// Dot11e association request frame structure
typedef struct struct_mac_dot11e_AssocReqFrame {                    //Should be  Actually
    DOT11_FrameHdr          hdr;                                    // 28          28
    DOT11e_CapabilityInfo   capInfo;                                //  2           2
    unsigned short          listenInterval;                         //  2           2
    char                    ssid[DOT11_SSID_MAX_LENGTH];            // 32          32
                                                                   // ---          ---
} DOT11e_AssocReqFrame;                                            //  64          64
//--------------------DOT11e-End-Updates---------------------------------//

typedef struct struct_mac_dot11_ReassocReqFrame {              //Should be  Actually
    DOT11_FrameHdr          hdr;                               // 28          28
    DOT11_CapabilityInfo    capInfo;                           //  2           2
    unsigned short          listenInterval;                    //  2           2
    char                    ssid[DOT11_SSID_MAX_LENGTH];       // 32          32
    Mac802Address             apAddr;                          //  6           6
                                                               //---         ---
} DOT11_ReassocReqFrame;                                       // 70          70


typedef struct struct_mac_dot11e_ReassocReqFrame {
    DOT11_FrameHdr          hdr;                              //28                28
    DOT11e_CapabilityInfo   capInfo;                          // 2                 2
    unsigned short          listenInterval;                   // 2                 2
    char                    ssid[32];                         //32                32
    Mac802Address             apAddr;                         // 6                 6
                                                              //--                ---
} DOT11e_ReassocReqFrame;                                     //70                70

typedef struct struct_mac_dot11_AssocRespFrame {               //Should be        Actually
    DOT11_FrameHdr          hdr;                               // 28              28
    DOT11_CapabilityInfo    capInfo;                           //  2               2
    unsigned short          statusCode;                        //  2               2
    unsigned short          assocId;                           //  2               2
                                                               //    ---             --
} DOT11_AssocRespFrame;                                        // 34              40

//---------------------DOT11e--Updates------------------------------------//
/// Dot11e Association response frame structure
typedef struct struct_mac_dot11e_AssocRespFrame {         //Should be    Actually
    DOT11_FrameHdr          hdr;                          //28            28
    DOT11e_CapabilityInfo   capInfo;                      // 2             2
    unsigned short         statusCode;                   // 2              2
    unsigned short          assocId;                      // 2             2
} DOT11e_AssocRespFrame;
//--------------------DOT11e-End-Updates---------------------------------//

typedef struct struct_mac_dot11_DisassocFrame {       //Should be    Actually
    DOT11_FrameHdr           hdr;                     //28           28
    unsigned short          reasonCode;              // 2            2
} DOT11_DisassocFrame;


// Channel info
typedef struct struct_mac_dot11_ChannelInfoResults{
    unsigned int            channel;
    int                     rssi;
    int                     bssActive;
    int                     pcfActive;
} DOT11_ChannelInfoResults;

// Channel info results
typedef struct struct_mac_dot11_ChannelInfo{
    unsigned int                numChannels;
    unsigned int                channelList[MAX_NUMBER_CHANNELLIST];
    clocktype                   dwellTime;
    clocktype                   scanStartTime;
    unsigned int                numResults;
    DOT11_ApInfo*               headAPList;
} DOT11_ChannelInfo;

// Managment specific variables
typedef struct struct_mac_dot11_ManagementVars {
    DOT11_MacStationManagementStates    state;
    DOT11_MacStationManagementStates    prevState;
    DOT11_MacFrameClass                 frameClass;
    DOT11_ScanType                      scanType;
    int                                 currentChannel;
    Message*                            lastManagementFrame;
    DOT11_ChannelInfo*                  channelInfo;
    BOOL                                beaconHeard;
    BOOL                                probeResponseRecieved;
    Mac802Address                         apAddr;
    Mac802Address                         prevApAddr;

    BOOL                                isMap;
    Mac802Address                         currentApAddr;

    BOOL printManagementStatistics;     // Output Management stats?

    // Stats for Management

    int managementPacketsSent;          // Management packet count
    int managementPacketsGot;

    //Praposed AP to associate
    DOT11_ApInfo*               PraposedAPInfo;
    int assocReqDropped;
    int assocRespDropped;
    int reassocReqDropped;
    int reassocRespDropped;
    int ProbRespDropped;
    int ProbReqDropped;
    int AuthReqDropped;
    int AuthRespDropped;

    int assocReqSend;
    int assocRespSend;
    int reassocReqSend;
    int reassocRespSend;
    int ProbReqGnrated;
    int ProbReqSend;
    int ProbRespSend;
    int AuthReqSend;
    int AuthRespSend;

    int assocReqReceived;
    int assocRespReceived;
    int reassocReqReceived;
    int reassocRespReceived;
    int ProbReqReceived;
    int ProbRespReceived;
    int AuthReqReceived;
    int AuthRespReceived;
    clocktype shortScanTimer;
    clocktype longScanTimer;

    unsigned int ADDBSRequestSent;
    unsigned int ADDBSRequestReceived;
    unsigned int ADDBSRequestDropped;
    unsigned int ADDBSResponseSent;
    unsigned int ADDBSResponseReceived;
    unsigned int ADDBSResponseDropped;
    unsigned int DELBARequestSent;
    unsigned int DELBARequestReceived;
    unsigned int DELBARequestDropped;

} DOT11_ManagementVars;

// Define a Frame type
typedef Message              DOT11_Frame;

// Modifications
// Ignored

//--------------------------------------------------------------------------
//
//  NAME:        MacDot11CfpResetBackoffAndAckVariables
//  PURPOSE:     (Re)set some DCF variables during transit between
//               DCF and PCF.
//  PARAMETERS:  Node* node
//                  Pointer to node
//                MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
void MacDot11CfpResetBackoffAndAckVariables(
    Node* node,
     MacDataDot11* dot11);
//--------------------------------------------------------------------------
//  NAME:        MacDot11ManagementIncrementSendFrame.
//  PURPOSE:     Transmit the data frame.
//  PARAMETERS:  Node* node transmitting the data frame.
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void //inline
MacDot11ManagementIncrementSendFrame(Node* node,
                                     MacDataDot11* dot11,
                                     DOT11_MacFrameType frameType);

//--------------------------------------------------------------------------
/*!
 * \brief  Set the management state of the node.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param state     DOT11_MacStationManagementStates : State to set

 * \return          None.
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementSetState(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacStationManagementStates state)
{
    //Change in management state is allowed during
    //any of the DCF state.
        DOT11_ManagementVars * mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

        mngmtVars->prevState = mngmtVars->state;
        mngmtVars->state = state;


}// MacDot11ManagementSetState


//--------------------------------------------------------------------------
/*!
 * \brief  Set the frame class of the interface.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frameClass DOT11_MacFrameClass : Class to set

 * \return          None.
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementSetFrameClass(
    Node* node,
    MacDataDot11* dot11,
    DOT11_MacFrameClass frameClass)
{

    ERROR_Assert(dot11->state == DOT11_S_WFJOIN,
        "MacDot11ManagementSetFrameClass: "
        "Not in correct management state.\n");

   DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    mngmtVars->frameClass = frameClass;

}// MacDot11ManagementSetFrameClass


//--------------------------------------------------------------------------
/*!
 * \brief  Get the current frame class of the interface.
 *
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          DOT11_MacFrameClass  : frame class
 */
//--------------------------------------------------------------------------
static //inline
DOT11_MacFrameClass MacDot11ManagementGetFrameClass(
    MacDataDot11* dot11 )
{

   DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    return mngmtVars->frameClass;

}// MacDot11ManagementGetFrameClass


//--------------------------------------------------------------------------
/*!
 * \brief  Get the current frame class of the interface.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame* frame: Frame to send

 * \return          None
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementSetSendFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* frame)
{

   DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    // Put the frame in the current message buffer
    dot11->currentMessage = frame;

    // For a BSS station, change next hop address to be that of the AP
    if (MacDot11IsBssStation(dot11)) {
        dot11->currentNextHopAddress = mngmtVars->apAddr;
    }

    dot11->dataRateInfo =
        MacDot11StationGetDataRateEntry(
            node,
            dot11,
            dot11->currentNextHopAddress);

    ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                 dot11->currentNextHopAddress,
                 "Address does not match");

    MacDot11StationAdjustDataRateForNewOutgoingPacket(
        node,
        dot11);

}// MacDot11ManagementSetSendFrame


//--------------------------------------------------------------------------
/*!
 * \brief  Reset send frame buffer.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          None
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementResetSendFrame(
    Node* node,
    MacDataDot11* dot11)
{
//---------------------------Power-Save-Mode-Updates---------------------//
    if (dot11->currentMessage != NULL){
        MESSAGE_Free(node, dot11->currentMessage);
    }
    if ((dot11->dot11TxFrameInfo!= NULL)
        && (dot11->dot11TxFrameInfo->msg == dot11->currentMessage))
    {
        MEM_free(dot11->dot11TxFrameInfo);
        dot11->dot11TxFrameInfo = NULL;

    }
//---------------------------Power-Save-Mode-End-Updates-----------------//
    // Reset the current message buffer
    dot11->currentMessage = NULL;

}// MacDot11ManagementResetSendFrame

//--------------------------------------------------------------------------
/*!
 * \brief  Check if frame is a management frame.
 *
 * \param frame     DOT11_Frame*    : Pointer to frame to check

 * \return          BOOL        : TRUE - If a management frame.
 *                                FALSE - If NOT a management frame.
 */
//--------------------------------------------------------------------------
static
BOOL MacDot11IsManagementFrame(
    DOT11_Frame* frame)
{
    DOT11_ShortControlFrame* hdr =
        (DOT11_ShortControlFrame*) MESSAGE_ReturnPacket(frame);

 

    if (hdr->frameType <= DOT11_ACTION) {
        return TRUE;
    }

    return FALSE;

}// MacDot11IsManagementFrame


//--------------------------------------------------------------------------
/*!
 * \brief  Check if a management frame and if it is, process it.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Received message

 * \return          BOOL        : TRUE - If a management frame.
 *                                FALSE - If NOT a management frame.
 */
//--------------------------------------------------------------------------
static
BOOL MacDot11ManagementCheckFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* frame)
{

    // dot11s. IBSS stations do not init management structure
    if (dot11->mngmtVars == NULL)
    {
        MESSAGE_Free(node, frame);
        return TRUE;
    }

    MacDot11Trace(
        node,
        dot11,
        frame,
       "MacDot11ManagementCheckFrame");

    // Check if we have a management frame
    if (MacDot11IsManagementFrame(frame))
    {

        if (dot11->useDvcs) {
            BOOL nodeIsInCache;
            Mac802Address sourceNodeAddress =
                    ((DOT11_FrameHdr*)MESSAGE_ReturnPacket(frame))
                    ->sourceAddr;

            MacDot11StationUpdateDirectionCacheWithCurrentSignal(
                node,
                dot11,
                sourceNodeAddress,
                &nodeIsInCache);

        }
        dot11->mgmtSendResponse = FALSE;

        // Process the frame
        MacDot11ManagementProcessFrame(
            node,
            dot11,
            frame);

        // Update stats
        DOT11_ManagementVars * mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

        mngmtVars->managementPacketsGot++;

        // Free received message
        MESSAGE_Free(
            node,
            frame);

        return TRUE;
    }

    return FALSE;

}// MacDot11CheckForManagemantFrame


//--------------------------------------------------------------------------
/*!
 * \brief  Start a managment timer

 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param timerDelay clocktype      : Delay time

 * \return          None..
 */
//--------------------------------------------------------------------------
static //inline
void MacDot11ManagementStartTimer(
    Node* node,
    MacDataDot11* dot11,
    clocktype timerDelay)
{
    Message* newMsg;

    dot11->timerSequenceNumber++;

    newMsg = MESSAGE_Alloc(
                node,
                MAC_LAYER,
                MAC_PROTOCOL_DOT11,
                MSG_MAC_DOT11_Management);

    MESSAGE_SetInstanceId(
        newMsg,
        (short) dot11->myMacData->interfaceIndex);

    MESSAGE_InfoAlloc(
        node,
        newMsg,
        sizeof(dot11->timerSequenceNumber));

    *((int*)MESSAGE_ReturnInfo(newMsg)) = dot11->timerSequenceNumber;

    MESSAGE_Send(
        node,
        newMsg,
        timerDelay);

    dot11->managementSequenceNumber = dot11->timerSequenceNumber;

}// MacDot11ManagementStartTimer


//--------------------------------------------------------------------------
/*!
 * \brief  Return the frame type string.

 * \param frame DOT11_Frame* : The frame to get the type of

 * \return          char *   : type string.
 */
//--------------------------------------------------------------------------
static
const char * MacDot11ManagementFrameTypeName(
    DOT11_Frame* frame)
{

    const char * chrName;

    DOT11_FrameHdr* hdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(frame);

    switch (hdr->frameType) {

        case DOT11_ASSOC_REQ:
        {
            chrName = "Associate Request";
            break;
        }
        case DOT11_ASSOC_RESP:
        {
            chrName = "Associate Response";
            break;
        }
        case DOT11_REASSOC_REQ:
        {
            chrName = "Reassociate Request";
            break;
        }
        case DOT11_REASSOC_RESP:
        {
            chrName = "Reassociate Response";
            break;
        }
        case DOT11_PROBE_REQ:
        {
            chrName = "Probe Request";
            break;
        }
        case DOT11_PROBE_RESP:
        {
            chrName = "Probe Response";
            break;
        }
        case DOT11_BEACON:
        {
            chrName = "Beacon";
            break;
        }
        case DOT11_ATIM:
        {
            chrName = "ATIM";
            break;
        }
        case DOT11_DISASSOCIATION:
        {
            chrName = "Disassociate";
            break;
        }
        case DOT11_AUTHENTICATION:
        {
            chrName = "Authenticate";
            break;
        }
        case DOT11_DEAUTHENTICATION:
        {
            chrName = "Deauthenticate";
            break;
        }

        default:
        {
            char errString[MAX_STRING_LENGTH];

            sprintf(errString,
                "MacDot11ManagemantProcessFrame: "
                "Received unknown management frame type %d\n",
                hdr->frameType);
            ERROR_ReportError(errString);
            break;
        }
    }

    return chrName;

}// MacDot11ManagementFrameTypeName


//--------------------------------------------------------------------------
/*!
 * \brief  Transmit the pending management frame.

 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          char *   : type string.
 */
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationTransmitManagementFrame(
    Node* node,
    MacDataDot11* dot11)
{
    Mac802Address destAddr;
    int dataRateType;

    if (dot11->isHTEnable)
    {
       ERROR_Assert(FALSE,"not HT Enabled");
    }
    
    destAddr = dot11->currentNextHopAddress;

/*********HT START*************************************************/
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           MESSAGE_ReturnPacketSize(dot11->currentMessage),
                           DOT11_BEACON, 
                           destAddr, 
                           NULL);
        PHY_SetTxVector(node, 
                        dot11->myMacData->phyNumber,
                        tempTxVector);
    }
    else
    {
    PHY_SetTxDataRateType(
        node,
        dot11->myMacData->phyNumber,
        dot11->lowestDataRateType);

    dataRateType = PHY_GetTxDataRateType(
                        node,
                        dot11->myMacData->phyNumber);
    }
/*********HT END****************************************************/
  

    ERROR_Assert(dot11->currentMessage != NULL,
                 "MacDot11StationTransmitManagementFrame: "
                 "There is no frame in the buffer.\n");

    //dataRateInfo can be null if the frame is delayed and CAP got started
    if (dot11->dataRateInfo == NULL)
    {
        dot11->dataRateInfo =
            MacDot11StationGetDataRateEntry(
                node,
                dot11,
                dot11->currentNextHopAddress);

        ERROR_Assert(dot11->dataRateInfo->ipAddress ==
                    dot11->currentNextHopAddress,
                    "Address does not match");
    }


    clocktype transmitDelay;
    DOT11_SeqNoEntry *entry;

    DOT11_Frame* frameToPhy = MESSAGE_Duplicate(
                                 node,
                                 dot11->currentMessage);

    DOT11_FrameHdr* hdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(frameToPhy);

    dot11->waitingForAckOrCtsFromAddress = destAddr;

    entry = MacDot11StationGetSeqNo(
                node,
                dot11,
                destAddr);

    ERROR_Assert(entry,
        "MacDot11StationTransmitManagementFrame: "
        "Unable to find or create sequence number entry.\n");

    hdr->seqNo = entry->toSeqNo;
    hdr->duration = 0;
    hdr->fragId = 0;

//---------------------------Power-Save-Mode-Updates---------------------//
    if (hdr->frameType == DOT11_PS_POLL){
        if (DEBUG_PS && DEBUG_PS_PSPOLL){
            MacDot11Trace(node, dot11, NULL,
                "Station Transmitted PS POLL Frame");
        }
        MacDot11StationSetState(node, dot11, DOT11_X_PSPOLL);
    }// end of if
    else{
        MacDot11StationSetState(
            node,
            dot11,
            DOT11_X_MANAGEMENT);
    }
//---------------------------Power-Save-Mode-End-Updates-----------------//

    if (DEBUG_STATION) {
//---------------------------Power-Save-Mode-End-Updates-----------------//
        MacDot11Trace(node, dot11, dot11->currentMessage,
            "Sending frame");
//---------------------------Power-Save-Mode-Updates---------------------//
    }
    if (MESSAGE_ReturnPacketSize(dot11->currentMessage) >
        dot11->rtsThreshold)
    {
        // Since using RTS-CTS, data packets have to wait
        // an additional SIFS.
        transmitDelay = dot11->sifs;
    }
    else {
        // Not using RTS-CTS, so don't need to wait for SIFS
        // since already waited for DIFS or BO.
        transmitDelay = dot11->delayUntilSignalAirborn;
    }

    if (!dot11->useDvcs) {
        MacDot11StationStartTransmittingPacket(
            node,
            dot11,
            frameToPhy,
            transmitDelay);
    }
    else {
        BOOL isCached = FALSE;
        double directionToSend = 0.0;

        AngleOfArrivalCache_GetAngle(
            &dot11->directionalInfo->angleOfArrivalCache,
            destAddr,
            node->getNodeTime(),
            &isCached,
            &directionToSend);

        if (isCached) {
            MacDot11StationStartTransmittingPacketDirectionally(
                node,
                dot11,
                frameToPhy,
                transmitDelay,
                directionToSend);

            PHY_LockAntennaDirection(
                node,
                dot11->myMacData->phyNumber);
        }
        else {
            // Don't know which way to send, send omni.
            MacDot11StationStartTransmittingPacket(
                node,
                dot11,
                frameToPhy,
                transmitDelay);
        }
    }

}// MacDot11StationTransmitManagementFrame


//--------------------------------------------------------------------------
/*!
 * \brief  Check if a packet needs to be dequeued.

 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          None.
 */
//--------------------------------------------------------------------------
static
void MacDot11ManagementCheckForOutgoingPacket(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->currentMessage == NULL) {
        MacDot11StationMoveAPacketFromTheNetworkLayerToTheLocalBuffer(
            node,
            dot11);
    }

    MacDot11StationCheckForOutgoingPacket(
        node,
        dot11,
        FALSE);

} // MacDot11ManagementCheckForOutgoingPacket

//--------------------HCCA-Updates Start---------------------------------//
BOOL MacDot11ManagementAttemptToInitiateADDTS(
    Node* node,
    MacDataDot11* dot11,
    unsigned int tsid );

//--------------------------------------------------------------------------
/*!
 * \brief  Action frame  to  an AP.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
int MacDot11ManagementADDTSRequest(
    Node* node,
    MacDataDot11* dot11,
    unsigned int tsid );
//--------------------HCCA-Updates End-----------------------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationInformManagementOfPktDrop
//  PURPOSE:     To increment the drop count of the respective management frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               NodeAddress detsAddr
//                  Node address of destination station
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
void
MacDot11StationInformManagementOfPktDrop(Node* node,
                                         MacDataDot11* dot11,
                                         DOT11_FrameInfo* dot11TxFrameInfo);

int MacDot11ManagementProcessAssociateRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessAssociateResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessReassociateRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessReassociateResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessProbeRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessProbeResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);

int MacDot11ManagementProcessAuthenticate(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame);


#endif //MAC_DOT11_MGMT_H
