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
 * \file mac_dot11-mgmt.cpp
 * \brief MAC management related routines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "api.h"
#include "mac_dot11-mgmt.h"
#include "mac_dot11-mib.cpp"
//---------------------HCCA-Updates------------------------------------------
#include "mac_dot11-hcca.h"
#include "phy_802_11.h"
#include "phy_abstract.h"
#include "mac_phy_802_11n.h"
//--------------------HCCA-Updates-End---------------------------------------
//-------------------------------DEFINITIONS--------------------------------
//-------------------------------------------------------------------------
//--------------------------------------------------------------------------
/*!
 * \brief  Signal the end of Authentication
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
void MacDot11AuthenticationCompleted(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
/*!
 * \brief  Signal the end of Association
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
void MacDot11AssociationCompleted(
    Node* node,
    MacDataDot11* dot11);
//--------------------------------------------------------------------------
/*!
 * \brief  Join has completed. Assign AP
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementJoinCompleted(
    Node* node,
    MacDataDot11* dot11);

//--------------------------------------------------------------------------
/*!
 * \brief  Enable management
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------


int MacDot11ManagementEnable(
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
                                     DOT11_MacFrameType frameType)
{
     DOT11_ManagementVars * mngmtVars =
            (DOT11_ManagementVars*) dot11->mngmtVars;

     switch(frameType)
     {
         case DOT11_ASSOC_REQ:
            mngmtVars->assocReqSend++;
            break;
         case DOT11_ASSOC_RESP:
             mngmtVars->assocRespSend++;
             break;
         case DOT11_REASSOC_REQ:
             mngmtVars->reassocReqSend++;
             break;
         case DOT11_REASSOC_RESP:
            mngmtVars->reassocRespSend++;
            break;
         case DOT11_PROBE_REQ:
            mngmtVars->ProbReqSend++;
            break;
         case DOT11_PROBE_RESP:
             mngmtVars->ProbRespSend++;
             break;
         case DOT11_AUTHENTICATION:
             if (MacDot11IsAp(dot11))
             {
                 mngmtVars->AuthRespSend++;
             }
             else
             {
                mngmtVars->AuthReqSend++;
             }
             break;
         default:
             break;
     }
}//MacDot11ManagementIncrementSendFrame
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
     MacDataDot11* dot11)
{
    dot11->BO = 0;
    MacDot11StationResetCW(node, dot11);
    MacDot11StationResetAckVariables(dot11);

    if (dot11->state == DOT11_CFP_START){
        if (dot11->currentACIndex >= DOT11e_AC_BK) {
                    MacDot11PauseOtherAcsBo(
                         node, dot11, getAc(dot11,dot11->currentACIndex).m_BO);
                    getAc(dot11,dot11->currentACIndex).m_BO = 0;
        }
    }
}
//--------------------------------------------------------------------------
/*!
 * \brief Select a channel and start listening.
 *
 * \param node          Node*   : Pointer to node
 * \param phyNumber     int     : Phy to listen to
 * \param channelIndex  short   : The new channel
 * \param dot11         MacDataDot11* : Pointer to MacDataDot11
 * \return              None.
 */
//--------------------------------------------------------------------------
void MacDot11ManagementStartListeningToChannel(
        Node            *node,
        int             phyNumber,
        short           channelIndex,
        MacDataDot11*   dot11)
{
    BOOL phyIsListening = TRUE;

    phyIsListening =
        PHY_IsListeningToChannel(
            node,
            phyNumber,
            channelIndex);

    if (phyIsListening == FALSE)
    {
        PHY_StartListeningToChannel(
            node,
            phyNumber,
            channelIndex);
    }

    if (!MacDot11IsAp(dot11))
    {
        MacDot11StationStartTimerOfGivenType(
            node,
            dot11,
            (clocktype)MacDot11TUsToClocktype(DOT11_PROBE_DELAY_TIME),
            MSG_MAC_DOT11_Probe_Delay_Timer);

        dot11->waitForProbeDelay = TRUE;
    }
} // End of MacDot11ManagementStartListeningToChannel //


//--------------------------------------------------------------------------
/*!
 * \brief Stop listening to a channel
 *
 * \param node          Node*   : Pointer to node
 * \param phyNumber     int     : Phy to listen to
 * \param channelIndex  short   : The new channel

 * \return              None.
 */
//--------------------------------------------------------------------------
void MacDot11ManagementStopListeningToChannel(
        Node        *node,
        int         phyNumber,
        short       channelIndex)
{
    BOOL phyIsListening = FALSE;

    phyIsListening =
        PHY_IsListeningToChannel(
            node,
            phyNumber,
            channelIndex);

    if (phyIsListening == TRUE)
    {
        PHY_StopListeningToChannel(
            node,
            phyNumber,
            channelIndex);
    }
} // End of MacDot11ManagementStopListeningToChannel //


//--------------------------------------------------------------------------
/*!
 * \brief Change to a new channel.
 *
 * \param node          Node*   : Pointer to node
 * \param dot11         MacDataDot11* : Pointer to Dot11 structure
 * \param channelIndex  short   : The new channel

 * \return              None.
 */
//--------------------------------------------------------------------------
void MacDot11ManagementChangeToChannel(
        Node*           node,
        MacDataDot11*   dot11,
        unsigned int    channelIndex)
{
   unsigned int phyNumber = dot11->myMacData->phyNumber;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    if (mngmtVars->currentChannel == channelIndex)
        return;

    /// Check if able to use channel
    BOOL phyCanListen =
        PHY_CanListenToChannel(
            node,
            phyNumber,
            channelIndex);

    if (!phyCanListen){
        ERROR_ReportError("MacDot11ManagementChangeToChannel: "
                "Channel is invalid.\n");
    }

    MacDot11ManagementStopListeningToChannel(
        node,
        phyNumber,
        (short)mngmtVars->currentChannel);

    // Set new transmit channel
    mngmtVars->currentChannel = channelIndex;
//---------------------------Power-Save-Mode-Updates---------------------//
    dot11->phyStatus = MAC_DOT11_PS_ACTIVE;
//---------------------------Power-Save-Mode-End-Updates-----------------//

    MacDot11ManagementStartListeningToChannel(
        node,
        phyNumber,
        (short)mngmtVars->currentChannel,
        dot11);

    PHY_SetTransmissionChannel(
        node,
        phyNumber,
        mngmtVars->currentChannel);

}// MacDot11ManagementChangeToChannel

DOT11_ApInfo* MacDot11ManagementGetAPInfo(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address bssAddr,
    DOT11_CFParameterSet cfSet,
    clocktype beaconInterval)
{
    DOT11_ApInfo* apInfo;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    // check if the neighbor already in the list
    apInfo = mngmtVars->channelInfo->headAPList;
    while (apInfo != NULL)
    {
        if (apInfo->bssAddr == bssAddr)
        {
            break;
        }

        apInfo = apInfo->next;
    }

    if (apInfo == NULL)
    {
        // not in neighbor list, create it
        apInfo = (DOT11_ApInfo*) MEM_malloc(sizeof(DOT11_ApInfo));
        ERROR_Assert(apInfo != NULL, "MAC 802.11: Out of memory!");
        memset((char*) apInfo, 0, sizeof(DOT11_ApInfo));

        apInfo->bssAddr = bssAddr;
        if (dot11->pcfEnable)
        {
        apInfo->cfSet = cfSet;
        }
        apInfo->beaconInterval= beaconInterval;

        // add AP in the list
        apInfo->next = mngmtVars->channelInfo->headAPList;
        mngmtVars->channelInfo->headAPList = apInfo;
    }

    return apInfo;
}
//--------------------------------------------------------------------------
// Build Management Frame Functions
//--------------------------------------------------------------------------

//---------------------DOT11e-HCCA-Updates---------------------------------//
//--------------------------------------------------------------------------
/*!
 * \brief Build an ADDTS request frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Pointer to a new empty frame
 *\ param tsid      unsigned int    : Traffic Stream ID

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildADDTSRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_FrameInfo** frameInfo,
    unsigned int tsid)
{
    int result = DOT11_R_FAILED;
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    // Allocate new addTSFrame
    DOT11e_ADDTS_Request_frame* addTSFrame;
    int ADDTSFrameSize = 0;
    DOT11_MacFrame ADDTS;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);

    ADDTSFrameSize = sizeof(DOT11e_ADDTS_Request_frame);
    MacDot11AddTrafficSpecToframe(
        node,
        dot11,
        element,
        &ADDTS,
        &ADDTSFrameSize,
        &dot11->hcVars->staTSBufferItem[tsid -
        DOT11e_HCCA_MIN_TSID]->trafficSpec);


    // Allocate new Probe frame
            // Alloc ActionFrame
        Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

            MESSAGE_PacketAlloc(
                node,
                msg,
                ADDTSFrameSize,
                TRACE_DOT11);

    addTSFrame = (DOT11e_ADDTS_Request_frame*)MESSAGE_ReturnPacket(msg);
    memset(addTSFrame, 0, (size_t)ADDTSFrameSize);
    memcpy(addTSFrame, &ADDTS, (size_t)ADDTSFrameSize);
    MEM_free(element);

    // Fill in header
    addTSFrame->hdr.frameType = DOT11_ACTION;
    addTSFrame->hdr.destAddr = mngmtVars->apAddr;
    addTSFrame->hdr.sourceAddr = dot11->selfAddr;
    addTSFrame->hdr.address3 = mngmtVars->apAddr;

    // Fill in frame
    // category info
    addTSFrame->category = 1;

    // qosAction information

    addTSFrame->qosAction = DOT11e_ADDTS_Request;

    DOT11_FrameInfo* newFrameInfo =
    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));

    newFrameInfo->msg = msg;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = mngmtVars->apAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->DA = mngmtVars->apAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->insertTime = node->getNodeTime();
    *frameInfo = newFrameInfo;

    result = DOT11_R_OK;

    return result;

}// MacDot11ManagementBuildADDTSRequestFrame
//--------------------DOT11e-HCCA-End-Updates-----------------------------//


//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11ManagementBuildATIMFrame
//  PURPOSE:     Build ATIM frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address destinationNodeAddress
//                  destination node address
//  RETURN:      BOOL
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
BOOL MacDot11ManagementBuildATIMFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destinationNodeAddress)
{
    BOOL result = FALSE;
    // Alloc ATIM frame
    DOT11_Frame* msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(
        node,
        msg,
        sizeof(DOT11_ATIMFrame),
        TRACE_DOT11);

    if (msg != NULL)
    {

        DOT11_ATIMFrame* atimFrame =
            (DOT11_ATIMFrame *) MESSAGE_ReturnPacket(msg);

        memset(atimFrame, 0, sizeof(DOT11_ATIMFrame));

        // Fill in Header

        atimFrame->frameType = DOT11_ATIM;
        atimFrame->destAddr = destinationNodeAddress;
        atimFrame->sourceAddr = dot11->selfAddr;
        dot11->currentNextHopAddress = destinationNodeAddress;
        dot11->lastTxATIMDestAddress = destinationNodeAddress;
        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = msg;
        newFrameInfo->frameType = DOT11_ATIM;
        newFrameInfo->RA = destinationNodeAddress;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = destinationNodeAddress;
        newFrameInfo->SA = INVALID_802ADDRESS;
        newFrameInfo->insertTime = node->getNodeTime();
        dot11->dot11TxFrameInfo = newFrameInfo;
        dot11->currentACIndex = DOT11e_INVALID_AC;

        // Set send message
        MacDot11ManagementSetSendFrame(
            node,
            dot11,
            msg);

        result = TRUE;
    }
    return result;
}// MacDot11ManagementBuildATIMFrame

//---------------------------Power-Save-Mode-End-Updates-----------------//
//--------------------------------------------------------------------------
/*!
 * \brief Build an authentication frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param newFrame  DOT11_Frame*    : Pointer to a new empty frame
 * \param reqFrame  DOT11_AuthFrame* :Pointer to request frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildAuthenticateFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_FrameInfo** frameInfo,
    DOT11_AuthFrame* reqFrame)
{

    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    /// Allocate new Authenticate frame
    DOT11_AuthFrame * authFrame =
        (DOT11_AuthFrame *) MESSAGE_ReturnPacket(msg);

    memset(authFrame, 0, sizeof(DOT11_AuthFrame));

    /// This is a simple Open System authenticate
    // Fill it in
    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_Assert(msg != NULL,
            "MacDot11ManagementBuildAuthenticateFrame: "
            "Request authenticate frame is empty.\n");

        // Fill in header
        authFrame->hdr.frameType = DOT11_AUTHENTICATION;
        authFrame->hdr.destAddr = reqFrame->hdr.sourceAddr;
        authFrame->hdr.sourceAddr = dot11->selfAddr;
        authFrame->hdr.address3 = dot11->selfAddr;

        // Fill in frame
        authFrame->algorithmId = DOT11_AUTH_OPEN_SYSTEM;
        authFrame->transSeqNo = 2;
        authFrame->statusCode = DOT11_SC_SUCCESSFUL;

        result = DOT11_R_OK;
    }
    else {  // STATION

        // Fill in header
        authFrame->hdr.frameType = DOT11_AUTHENTICATION;
        authFrame->hdr.destAddr = mngmtVars->apAddr;
        authFrame->hdr.sourceAddr = dot11->selfAddr;
        authFrame->hdr.address3 = mngmtVars->apAddr;

        // Fill in frame
        authFrame->algorithmId = DOT11_AUTH_OPEN_SYSTEM;
        authFrame->transSeqNo = 1;
        result = DOT11_R_OK;
    }

      DOT11_FrameInfo* newframeInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
       memset(newframeInfo, 0, sizeof(DOT11_FrameInfo));

    newframeInfo->msg = msg;
    newframeInfo->frameType = DOT11_AUTHENTICATION;
    newframeInfo->RA = authFrame->hdr.destAddr;
    newframeInfo->TA = dot11->selfAddr;
    newframeInfo->DA = authFrame->hdr.destAddr;
    newframeInfo->SA = dot11->selfAddr;
    newframeInfo->insertTime = node->getNodeTime();
    *frameInfo = newframeInfo;
    return result;
}// MacDot11ManagementBuildAuthenticateFrame



//--------------------------------------------------------------------------
/*!
 * \brief Build a deauthenticate notification frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildDeauthenticateFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* frame)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        // TODO: Not supported yet
        ERROR_ReportError("MacDot11ManagementBuildDeauthenticateFrame:"
                          " Not implemented for AP.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

    } else { // STATION

        ERROR_ReportError("MacDot11ManagementBuildDeauthenticateFrame:"
                          " Station cannot build deauthenticate frame.\n");
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementBuildDeauthenticateFrame


//--------------------------------------------------------------------------
/*!
 * \brief Build an associate request frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildAssociateRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    Message** msg,
    DOT11_FrameInfo** frameInfo)
{
    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    // Allocate new Associatation frame
    if (MacDot11IsQoSEnabled(node, dot11))
    {
//---------------------DOT11e--Updates------------------------------------//
            
        DOT11_MacFrame assoc;
        memset(&assoc, 0, sizeof(DOT11_MacFrame));
        DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);
        int assocFrameSize = sizeof(DOT11e_AssocReqFrame);
        if (dot11->isHTEnable)
        {        
            MacDot11AddHTCapability(node,
                                    dot11,
                                    element,
                                    &assoc,
                                    &assocFrameSize);
            DOT11_HT_OperationElement  operElem;
            if (dot11->isVHTEnable)
            {
                operElem.opChBwdth = CHBWDTH_40MHZ;
            }
            else
            {
                operElem.opChBwdth =
                (ChBandwidth)Phy802_11nGetOperationChBwdth(
                    node->phyData[dot11->myMacData->phyNumber]);
            }
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node,
                                   dot11,
                                   &operElem, 
                                   &assoc,
                                   &assocFrameSize);
            if (dot11->isVHTEnable)
            {
                MacDot11AddVHTCapability(node,
                                    dot11,
                                    element,
                                    &assoc,
                                    &assocFrameSize);
            }
        }
        *msg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(
                node,
                *msg,
                assocFrameSize,
                TRACE_DOT11);

        DOT11e_AssocReqFrame * assocFrame =
                (DOT11e_AssocReqFrame *) MESSAGE_ReturnPacket((*msg));
            memset(assocFrame, 0, (size_t)assocFrameSize);
            memcpy(assocFrame, &assoc, (size_t)assocFrameSize);

            MEM_free(element);

        // Fill in header
        assocFrame->hdr.frameType = DOT11_ASSOC_REQ;
        assocFrame->hdr.destAddr = mngmtVars->apAddr;
        assocFrame->hdr.sourceAddr = dot11->selfAddr;
        assocFrame->hdr.address3 = mngmtVars->apAddr;
        assocFrame->capInfo.CFPollable = 0;
        assocFrame->capInfo.CFPollRequest = 0;

        if (dot11->isHTEnable)
        {
            assocFrame->capInfo.ImmidiateBlockACK =
                dot11->enableImmediateBAAgreement;
            assocFrame->capInfo.DelayedBlockACK =
                dot11->enableDelayedBAAgreement;
        }

        // Add capability info here
        if (dot11->cfPollable){
            assocFrame->capInfo.CFPollable = 1;
        }
        if (dot11->cfPollRequest){
            assocFrame->capInfo.CFPollRequest = 1 ;
        }
        assocFrame->capInfo.QoS = 1;

        assocFrame->listenInterval = 1;
        strcpy(assocFrame->ssid, dot11->stationMIB->dot11DesiredSSID);
        result = DOT11_R_OK;
//--------------------DOT11e-End-Updates---------------------------------//
    } 
    else {
            
        *msg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(
            node,
            *msg,
            sizeof(DOT11_AssocReqFrame),
            TRACE_DOT11);
            DOT11_AssocReqFrame * assocFrame =
                    (DOT11_AssocReqFrame *) MESSAGE_ReturnPacket((*msg));
            memset(assocFrame, 0, sizeof(DOT11_AssocReqFrame));
            // Set the TxRates

            // Set the PortType
            // ess port

            // Fill in header
            assocFrame->hdr.frameType = DOT11_ASSOC_REQ;
            assocFrame->hdr.destAddr = mngmtVars->apAddr;
            assocFrame->hdr.sourceAddr = dot11->selfAddr;
            assocFrame->hdr.address3 = mngmtVars->apAddr;

        if (dot11->cfPollable){
            assocFrame->capInfo.cfPollable= 1;
        }
        if (dot11->cfPollRequest){
            assocFrame->capInfo.cfPollRequest = 1 ;
        }

        // Fill in frame
//---------------------------Power-Save-Mode-Updates---------------------//
        // Add listen interval information in association frame request frame
        assocFrame->listenInterval = (unsigned short)dot11->listenIntervals;
        // SEND Power save Information to AP
        if (MacDot11IsStationSupportPSMode(dot11)){
            assocFrame->hdr.frameFlags |= DOT11_POWER_MGMT;
        }
//---------------------------Power-Save-Mode-End-Updates-----------------//
        strcpy(assocFrame->ssid, dot11->stationMIB->dot11DesiredSSID);

        result = DOT11_R_OK;

    }
    if (result == DOT11_R_OK)
        {
            DOT11_FrameInfo* newframeInfo =
            (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
             memset(newframeInfo, 0, sizeof(DOT11_FrameInfo));

            newframeInfo->msg = *msg;
            newframeInfo->frameType = DOT11_ASSOC_REQ;
            newframeInfo->RA = mngmtVars->apAddr;
            newframeInfo->TA = dot11->selfAddr;
            newframeInfo->DA = mngmtVars->apAddr;
            newframeInfo->SA = INVALID_802ADDRESS;
            newframeInfo->insertTime = node->getNodeTime();
            *frameInfo = newframeInfo;
        }
    return result;

}// MacDot11ManagementBuildAssociateRequestFrame

//--------------------------------------------------------------------------
/*!
 * \brief Build an associate response frame from an ESS.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame
 * \param reqFrame  DOT11_AssocReqFrame*    : Received the request frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildAssociateResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_FrameInfo** frameInfo,
    DOT11_AssocReqFrame* reqFrame)
{
    int assocID = 0;
    DOT11_AssocRespFrame* assocFrame;
    int result = DOT11_R_FAILED;
    *frameInfo = NULL;
    // Allocate new Authenticate frame
    assocFrame = (DOT11_AssocRespFrame *)MESSAGE_ReturnPacket(msg);
    memset(assocFrame, 0, sizeof(DOT11_AssocRespFrame));

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_Assert(reqFrame != NULL,
            "MacDot11ManagementBuildAuthenticateFrame: "
            "Request associate response frame is empty.\n");


        // Fill in header
        assocFrame->hdr.frameType = DOT11_ASSOC_RESP;
        assocFrame->hdr.destAddr = reqFrame->hdr.sourceAddr;
        assocFrame->hdr.sourceAddr = dot11->selfAddr;
        assocFrame->hdr.address3 = dot11->selfAddr;

        dot11->cfPollable = reqFrame->capInfo.cfPollable;
        dot11->cfPollRequest = reqFrame->capInfo.cfPollRequest;
        if (dot11->stationCheckTimerStarted == FALSE)
        {

            clocktype timerDelay = DOT11_MANAGEMENT_STATION_INACTIVE_TIMEOUT;

             MacDot11StationStartStationCheckTimer(
                 node,
                 dot11,
                 timerDelay,
                 MSG_MAC_DOT11_Station_Inactive_Timer);
             dot11->stationCheckTimerStarted = TRUE;
        }

        // Fill in frame
        if (strcmp(reqFrame->ssid, dot11->stationMIB->dot11DesiredSSID) != 0 ){

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

                sprintf(errString,
                    "MacDot11ManagementBuildAssociateResponseFrame: "
                    "Station %s SSID (%s) does not match received %s ",
                    macAdder,
                    dot11->stationMIB->dot11DesiredSSID,
                    reqFrame->ssid);

                ERROR_ReportWarning(errString);
            }

            assocFrame->assocId = 0;
            assocFrame->statusCode = DOT11_SC_UNSPEC_FAILURE;

        } else {

            DOT11_ApVars* ap = dot11->apVars;
            DOT11_ApStationListItem* stationItem = NULL;
        if (!MacDot11ApStationListIsEmpty(node, dot11, ap->apStationList))
        {
            stationItem = MacDot11ApStationListGetItemWithGivenAddress(node,
                dot11,
                reqFrame->hdr.sourceAddr);
            if (stationItem != NULL)
            {
                // reuse the existing association ID
                assocID = stationItem->data->assocId;
                stationItem->data->LastFrameReceivedTime = node->getNodeTime();
            }
        }
            if (stationItem == NULL)
            {
                // Generate new asscociation id
                assocID = MacDot11ApStationListAddStation(
                            node,
                            dot11,
                            reqFrame->hdr.sourceAddr,
//---------------------------Power-Save-Mode-Updates---------------------//
                            0, // QOS deisabled
                            reqFrame->listenInterval,
                            MacDot11IsPSModeEnabledAtSTA(
                                reqFrame->hdr.frameFlags));
                if (assocID == 0)
                {
                    return DOT11_R_FAILED;
                }
            }
//---------------------------Power-Save-Mode-End-Updates-----------------//

            assocFrame->assocId = (unsigned short)assocID;
            assocFrame->statusCode = DOT11_SC_SUCCESSFUL;

            // dot11s. Notify MAP of associaton.
            if (dot11->isMAP)
            {
                Dot11s_StationAssociateEvent(node, dot11,
                    reqFrame->hdr.sourceAddr, INVALID_802ADDRESS);
            }
        }

        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
         memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = msg;
        newFrameInfo->frameType = DOT11_ASSOC_RESP;
        newFrameInfo->RA = reqFrame->hdr.sourceAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = reqFrame->hdr.sourceAddr;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();
        *frameInfo = newFrameInfo;

        result = DOT11_R_OK;

    } else {  // STATION

        ERROR_ReportError("MacDot11ManagementBuildAssociateResponseFrame:"
                          " Station cannot build association response frame.\n");
        result = DOT11_R_NOT_SUPPORTED;

    }

    return result;

}// MacDot11ManagementBuildAssociateResponseFrame

/*FUNCTION   :: Dot11_CreateProbeRequestFrame
LAYER      :: MAC
PURPOSE    :: Create a probe request frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
NOTES      :: This function partially creates the beacon frame.
                It adds mesh related elements. Fixed length fields
                are filled in separately.
**/

void Dot11_CreateAssocRespFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Ie* element,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{
    *txFrameSize = sizeof(DOT11e_AssocRespFrame*);
    element->data = (unsigned char*)txFrame + *txFrameSize;
    //Add Cfp Parameter set

    if (dot11->stationType == DOT11_STA_PC){
        MacDot11CfpParameterSetUpdate(node, dot11);
        Dot11_CreateCfParameterSet(node, element, &dot11->pcVars->cfSet);
        *txFrameSize += element->length;
    }
}


//MacDot11EnhacementEnd

//---------------------DOT11e--Updates------------------------------------//
// Build an associate response frame from an ESS
//
// \param node  Node Pointer
// \param dot11  Pointer to Dot11 structure
// \param param frame  Pointer to a new empty frame
// \param param reqFrame  Received the request frame
//
// \return 0 or 1

static // inline
int MacDot11eManagementBuildAssociateResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* frame,
    DOT11_FrameInfo** frameInfo,
    DOT11_Frame* msg)
{
    int result = DOT11_R_FAILED;
    DOT11e_AssocRespFrame* assocFrame;
    DOT11e_AssocReqFrame *reqFrame =
        (DOT11e_AssocReqFrame *)MESSAGE_ReturnPacket(msg);
    int assocFrameSize = 0;
    DOT11_MacFrame assoc;
    memset(&assoc, 0, sizeof(DOT11_MacFrame));
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);
    *frameInfo = NULL;

    DOT11_HT_CapabilityElement htCapabilityElement;

    VHT_CapabilitiesElement vhtCapabilityElement;
    VHT_OperationElement vhtOperationElement;
    BOOL isVHTSTA = FALSE;
    
    BOOL isHTSTA = FALSE;
    BOOL rifsMode = FALSE;
    if (dot11->isHTEnable)
    {
         DOT11_HT_OperationElement operElem;
        BOOL isPresent = Dot11_GetHTOperation(node,
                                              dot11,
                                              msg,
                                              &operElem,
                                              sizeof(DOT11e_AssocReqFrame));
        if (isPresent)
        {
            rifsMode = operElem.rifsMode;
        }

         isPresent = Dot11_GetHTCapabilities(
            node,
            dot11,
            msg,
            &htCapabilityElement,
            sizeof(DOT11e_AssocReqFrame));
        if (isPresent)
        {
            isHTSTA = TRUE;
            MacDot11nUpdNeighborHtInfo(
                node, dot11, reqFrame->hdr.sourceAddr, htCapabilityElement);
        }

        if (dot11->isVHTEnable)
        {
            isPresent = Dot11_GetVHTCapabilities(
                            node,
                            dot11,
                            msg,
                            &vhtCapabilityElement,
                            sizeof(DOT11e_AssocReqFrame));
            ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht capability"
                   "element in management frame");
            isVHTSTA = TRUE;
            DOT11_NeighborHtInfo* neighHtInfo =
                 MacDot11nGetNeighHtInfo(dot11->htInfoList,
                         reqFrame->hdr.sourceAddr);
            neighHtInfo->isOneSixtyMhzIntolerent
             = vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;
            neighHtInfo->maxMcsIdx = (int)vhtCapabilityElement.m_mcsNssSet;
        }
    }

    assocFrameSize = sizeof(DOT11e_AssocRespFrame);

       MacDot11AddEDCAParametersetToBeacon(node, dot11, element,
                  &assoc, &assocFrameSize);

       if (dot11->isHTEnable)
       {
            MacDot11AddHTCapability(node, dot11, element,
                  &assoc, &assocFrameSize);

            DOT11_HT_OperationElement  operElem;
            if (dot11->isVHTEnable)
            {
                operElem.opChBwdth = CHBWDTH_40MHZ;
                operElem.primaryCh
                    = PHY802_11GetPrimary20MHzChIdx(node->phyData[dot11->myMacData->phyNumber]);
                operElem.secondaryChOffset
                    = PHY802_11GetSecondaryChOffset(node->phyData[dot11->myMacData->phyNumber]);
            }
            else
            {
                operElem.opChBwdth = (ChBandwidth)Phy802_11nGetOperationChBwdth(
                                    node->phyData[dot11->myMacData->phyNumber]);
            }
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node, dot11, &operElem, 
                &assoc, &assocFrameSize);

             if (dot11->isVHTEnable)
             {
                MacDot11AddVHTCapability(node,
                                    dot11,
                                    element,
                                    &assoc,
                                    &assocFrameSize);
                MacDot11AddVHTOperation(node,
                                    dot11,
                                    element,
                                    &assoc,
                                    &assocFrameSize);
             }
       }
       MESSAGE_PacketAlloc(
                    node,
                    frame,
                    assocFrameSize,
                    TRACE_DOT11);

    // Allocate new Probe frame
    assocFrame = (DOT11e_AssocRespFrame *)MESSAGE_ReturnPacket(frame);
    memset(assocFrame, 0, (size_t)assocFrameSize);
    memcpy(assocFrame, &assoc, (size_t)assocFrameSize);

    MEM_free(element);

    ERROR_Assert(reqFrame != NULL,
        "MacDot11ManagementBuildAuthenticateFrame: "
        "Request associate response frame is empty.\n");


    // Fill in header
    assocFrame->hdr.frameType = DOT11_ASSOC_RESP;
    assocFrame->hdr.destAddr = reqFrame->hdr.sourceAddr;
    assocFrame->hdr.sourceAddr = dot11->selfAddr;
    assocFrame->hdr.address3 = dot11->selfAddr;


    // Add capability info here
    assocFrame->capInfo.QoS = 1;
    if (dot11->stationCheckTimerStarted == FALSE)
    {

        clocktype timerDelay = DOT11_MANAGEMENT_STATION_INACTIVE_TIMEOUT;

         MacDot11StationStartStationCheckTimer(
             node,
             dot11,
             timerDelay,
             MSG_MAC_DOT11_Station_Inactive_Timer);
         dot11->stationCheckTimerStarted = TRUE;
    }
    assocFrame->capInfo.CFPollable = 0;
    assocFrame->capInfo.CFPollRequest = 0;

    if (dot11->isHTEnable)
    {
        assocFrame->capInfo.ImmidiateBlockACK 
                                    = dot11->enableImmediateBAAgreement;
        assocFrame->capInfo.DelayedBlockACK 
                                         = dot11->enableDelayedBAAgreement;
    }
//--------------------HCCA-Updates Start---------------------------------//
    if (MacDot11IsHC(dot11))
    {
        //Use value 01 to specify that HC is present at AP
        assocFrame->capInfo.CFPollable = 0;
        assocFrame->capInfo.CFPollRequest = 1;
    }
//--------------------HCCA-Updates End-----------------------------------//

    // Check SSID is correct
    if (strcmp(reqFrame->ssid,
        dot11->stationMIB->dot11DesiredSSID) != 0 ){

        if (DEBUG_MANAGEMENT){
            char errString[MAX_STRING_LENGTH];
            char macAdder[24];
            MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

            sprintf(errString,
                "MacDot11eManagementBuildAssociateResponseFrame: "
                "Station %s SSID (%s) does not match received %s ",
                macAdder,
                dot11->stationMIB->dot11DesiredSSID,
                reqFrame->ssid);

            ERROR_ReportWarning(errString);
        }

        assocFrame->assocId = 0;
        assocFrame->statusCode = DOT11_SC_UNSPEC_FAILURE;

    } else {

//---------------------DOT11e--Updates------------------------------------//
            // Add station to list
            //ADD Station to PC
            unsigned char qoSEnabled = 1;
            
            int assocID = MacDot11ApStationListAddStation(
                            node,
                            dot11,
                            reqFrame->hdr.sourceAddr,
                            qoSEnabled,
                            0, //listen interval
                            0 //psmode
                            );

            // update the HT capability info of the station at AP
            if (dot11->isHTEnable && isHTSTA)
            {
                DOT11_ApStationListItem* stationItem = NULL;
                stationItem = 
                    MacDot11ApStationListGetItemWithGivenAddress(
                        node,
                        dot11,
                        reqFrame->hdr.sourceAddr);
                stationItem->data->staHtCapabilityElement = 
                    htCapabilityElement;
                stationItem->data->isHTEnabledSta = isHTSTA;
                stationItem->data->staCapInfo = reqFrame->capInfo;
                stationItem->data->rifsMode = rifsMode;

                // Use mcs which is smaller from what this AP supports and 
                // what obtained via htcapability

                if (htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx
                                    < dot11->txVectorForHighestDataRate.mcs)
                {
                    stationItem->data->mscIndex = 
                      htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx;
                }
                else
                {
                    stationItem->data->mscIndex 
                                    = dot11->txVectorForHighestDataRate.mcs;
                }

                // Update the data rate entry
                 DOT11_DataRateEntry* neighborEntry = 
                    MacDot11StationGetDataRateEntry(node, 
                                                    dot11,
                                                    reqFrame->hdr.sourceAddr);
                 neighborEntry->txVector.mcs = stationItem->data->mscIndex - 7;
                 neighborEntry->txVector.format = 
                     htCapabilityElement.htCapabilitiesInfo.htGreenfield ? 
                            (Mode)MODE_HT_GF: (Mode)MODE_NON_HT;
                 neighborEntry->txVector.chBwdth 
                 =  htCapabilityElement.htCapabilitiesInfo.fortyMhzIntolerent ? 
                     (ChBandwidth)CHBWDTH_20MHZ: (ChBandwidth)CHBWDTH_40MHZ;
                 neighborEntry->txVector.stbc = 
                        (UInt8)htCapabilityElement.htCapabilitiesInfo.rxStbc;

                 DOT11_NeighborHtInfo* neighborHtInfo = 
                     MacDot11nGetNeighHtInfo(dot11->htInfoList, reqFrame->hdr.sourceAddr);
                 assert(neighborHtInfo);

                 if (dot11->isVHTEnable && isVHTSTA)
                 {
                     stationItem->data->isVHTEnabledSta = TRUE;
                     stationItem->data->vhtInfo.staVhtCapabilityElement
                          = vhtCapabilityElement; 
                     neighborHtInfo->maxMcsIdx
                         = (int)vhtCapabilityElement.m_mcsNssSet;
                     neighborHtInfo->curMcsIdx
                      = std::min(neighborHtInfo->maxMcsIdx,
                              Phy802_11GetNumAtnaElems(
                       node->phyData[dot11->myMacData->phyNumber])*10 - 1);
                     neighborHtInfo->isOneSixtyMhzIntolerent
                     = vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;
                 }
                 else
                 {
                     ERROR_Assert(stationItem->data->mscIndex >= 7,
                                  "Incorrect MCS index");
                     neighborHtInfo->curMcsIdx
                         = stationItem->data->mscIndex - 7;
                 }
            }

            if (assocID == 0)
            {
                return DOT11_R_FAILED;
            }
            assocFrame->assocId =(unsigned short) assocID;
            assocFrame->statusCode = DOT11_SC_SUCCESSFUL;


            // Update QBSS Load's station count;
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                dot11->qBSSLoadElement.stationCount++;
            }
//--------------------DOT11e-End-Updates---------------------------------//
        }
    DOT11_FrameInfo* newFrameInfo =
    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
     memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

    newFrameInfo->msg = frame;
    newFrameInfo->frameType = DOT11_ASSOC_RESP;
    newFrameInfo->RA = reqFrame->hdr.sourceAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->DA = INVALID_802ADDRESS;
    newFrameInfo->SA = dot11->selfAddr;
    newFrameInfo->insertTime = node->getNodeTime();
    *frameInfo = newFrameInfo;

    result = DOT11_R_OK;

    return result;

}// MacDot11eManagementBuildAssociateResponseFrame

//---------------------DOT11e--Updates------------------------------------//
// Build an associate response frame from an ESS
//
// \param node  Node Pointer
// \param dot11  Pointer to Dot11 structure
// \param param frame  Pointer to a new empty frame
// \param param reqFrame  Received the request frame
//
// \return 0 or 1

static // inline
int MacDot11eManagementBuildReassociateResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* frame,
    DOT11_FrameInfo** frameInfo,
    DOT11e_ReassocReqFrame* reqFrame,
     DOT11_Frame* msg)
{

    //DOT11e_AssocRespFrame* reassocFrame;
    int result = DOT11_R_FAILED;
    DOT11e_AssocRespFrame* reassocFrame;
    int reassocFrameSize = 0;
    DOT11_MacFrame reassoc;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);
    reassocFrameSize = sizeof(DOT11e_AssocRespFrame);

    DOT11_HT_CapabilityElement htCapabilityElement;

    VHT_CapabilitiesElement vhtCapabilityElement;

    BOOL isHTSTA = FALSE;
    if (dot11->isHTEnable)
    {
        BOOL isPresent = Dot11_GetHTCapabilities(
            node,
            dot11,
            msg,
            &htCapabilityElement,
            sizeof(DOT11e_ReassocReqFrame));
        if (isPresent)
        {
            isHTSTA = TRUE;
            MacDot11nUpdNeighborHtInfo(node, dot11, 
                reqFrame->hdr.sourceAddr, htCapabilityElement);
        }

        if (dot11->isVHTEnable)
        {
            isPresent = Dot11_GetVHTCapabilities(
                            node,
                            dot11,
                            msg,
                            &vhtCapabilityElement,
                            sizeof(DOT11e_ReassocReqFrame));
            ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht capability"
                   "element in management frame");
        }
    }

       MacDot11AddEDCAParametersetToBeacon(node, dot11, element,
                  &reassoc, &reassocFrameSize);

        if (dot11->isHTEnable)
        {
            MacDot11AddHTCapability(node, dot11, element,
                  &reassoc, &reassocFrameSize);

            DOT11_HT_OperationElement  operElem;
            if (dot11->isVHTEnable)
            {
                operElem.opChBwdth = CHBWDTH_40MHZ;
                operElem.primaryCh
                    = PHY802_11GetPrimary20MHzChIdx(
                            node->phyData[dot11->myMacData->phyNumber]);
                operElem.secondaryChOffset
                    = PHY802_11GetSecondaryChOffset(
                            node->phyData[dot11->myMacData->phyNumber]);
            }
            else
            {
                operElem.opChBwdth
                    = (ChBandwidth)Phy802_11nGetOperationChBwdth(
                            node->phyData[dot11->myMacData->phyNumber]);
            }
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node, dot11, &operElem, 
                &reassoc, &reassocFrameSize);

            if (dot11->isVHTEnable)
            {
                MacDot11AddVHTCapability(node,
                                    dot11,
                                    element,
                                    &reassoc,
                                    &reassocFrameSize);
                MacDot11AddVHTOperation(node,
                                    dot11,
                                    element,
                                    &reassoc,
                                    &reassocFrameSize);
            }
       }

    MESSAGE_PacketAlloc(
                node,
                frame,
                reassocFrameSize,
                TRACE_DOT11);

    // Allocate new Probe frame
    reassocFrame = (DOT11e_AssocRespFrame *)MESSAGE_ReturnPacket(frame);
    memset(reassocFrame, 0, (size_t)reassocFrameSize);
    memcpy(reassocFrame, &reassoc, (size_t)reassocFrameSize);
    MEM_free(element);

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_Assert(reqFrame != NULL,
            "MacDot11ManagementBuildAuthenticateFrame: "
            "Request associate response frame is empty.\n");


        // Fill in header
        reassocFrame->hdr.frameType = DOT11_REASSOC_RESP;
        reassocFrame->hdr.destAddr = reqFrame->hdr.sourceAddr;
        reassocFrame->hdr.sourceAddr = dot11->selfAddr;
        reassocFrame->hdr.address3 = dot11->selfAddr;


        // Add capability info here
        reassocFrame->capInfo.QoS = 1;
        if (dot11->stationCheckTimerStarted == FALSE)
        {

         clocktype timerDelay = DOT11_MANAGEMENT_STATION_INACTIVE_TIMEOUT;

             MacDot11StationStartStationCheckTimer(
                 node,
                 dot11,
                 timerDelay,
                 MSG_MAC_DOT11_Station_Inactive_Timer);
             dot11->stationCheckTimerStarted = TRUE;
        }
        reassocFrame->capInfo.CFPollable = 0;
        reassocFrame->capInfo.CFPollRequest = 0;

         if (dot11->isHTEnable)
        {
            reassocFrame->capInfo.ImmidiateBlockACK 
                                        = dot11->enableImmediateBAAgreement;
            reassocFrame->capInfo.DelayedBlockACK 
                                             = dot11->enableDelayedBAAgreement;
        }

//--------------------HCCA-Updates Start---------------------------------//
        if (MacDot11IsHC(dot11))
        {
            //Use value 01 to specify that HC is present at AP
            reassocFrame->capInfo.CFPollable = 0;
            reassocFrame->capInfo.CFPollRequest = 1;
        }
//--------------------HCCA-Updates End-----------------------------------//

        // Check SSID is correct
        if (strcmp(reqFrame->ssid, dot11->stationMIB->dot11DesiredSSID) != 0 ){

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

                sprintf(errString,
                    "MacDot11eManagementBuildReassociateResponseFrame: "
                    "Station %s SSID (%s) does not match received %s ",
                    macAdder,
                    dot11->stationMIB->dot11DesiredSSID,
                    reqFrame->ssid);

                ERROR_ReportWarning(errString);
            }

            reassocFrame->assocId = 0;
            reassocFrame->statusCode = DOT11_SC_UNSPEC_FAILURE;

        } else {

//---------------------DOT11e--Updates------------------------------------//
            // Add station to list
            unsigned char qoSEnabled = 1;
            int assocID = MacDot11ApStationListAddStation(
                            node,
                            dot11,
                            reqFrame->hdr.sourceAddr,
                            qoSEnabled);

            reassocFrame->assocId = (unsigned short)assocID;
            reassocFrame->statusCode = DOT11_SC_SUCCESSFUL;

            // update the HT capability info of the station at AP
            if (dot11->isHTEnable && isHTSTA)
            {
                DOT11_ApStationListItem* stationItem = NULL;
                stationItem = 
                    MacDot11ApStationListGetItemWithGivenAddress(
                                                node,
                                                dot11,
                                                reqFrame->hdr.sourceAddr);
                stationItem->data->staHtCapabilityElement = 
                                                        htCapabilityElement;
                stationItem->data->isHTEnabledSta = isHTSTA;
                stationItem->data->staCapInfo = reqFrame->capInfo;
                
                // Use mcs which is smaller from what this AP supports and 
                // what obtained via htcapability
                if (htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx 
                                    < dot11->txVectorForHighestDataRate.mcs)
                {
                    stationItem->data->mscIndex 
                                = htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx; 
                }
                else
                {
                    stationItem->data->mscIndex 
                                    = dot11->txVectorForHighestDataRate.mcs;
                }

                // Update the data rate entry!
                 DOT11_DataRateEntry* neighborEntry = 
                    MacDot11StationGetDataRateEntry(node, 
                                                    dot11,
                                                    reqFrame->hdr.sourceAddr);
                
               //neighborEntry->txVector.mcs = stationItem->data->mscIndex;
                 //assert(stationItem->data->mscIndex >= 7);
                 neighborEntry->txVector.mcs = stationItem->data->mscIndex-7;
                 neighborEntry->txVector.format = 
                     htCapabilityElement.htCapabilitiesInfo.htGreenfield ? 
                            (Mode)MODE_HT_GF: (Mode)MODE_NON_HT;
                 neighborEntry->txVector.chBwdth =
                 htCapabilityElement.htCapabilitiesInfo.fortyMhzIntolerent?
                     (ChBandwidth)CHBWDTH_20MHZ: (ChBandwidth)CHBWDTH_40MHZ;
                 neighborEntry->txVector.stbc = 
                        (UInt8)htCapabilityElement.htCapabilitiesInfo.rxStbc;

                 DOT11_NeighborHtInfo* neighborHtInfo = 
                     MacDot11nGetNeighHtInfo(dot11->htInfoList,
                                         reqFrame->hdr.sourceAddr);
                 ERROR_Assert(neighborHtInfo,"neighborHtInfo should not be NULL");
                 if (dot11->isVHTEnable)
                 {
                     stationItem->data->isVHTEnabledSta = TRUE;
                     stationItem->data->vhtInfo.staVhtCapabilityElement
                          = vhtCapabilityElement; 
                     neighborHtInfo->maxMcsIdx
                         = (int)vhtCapabilityElement.m_mcsNssSet;
                     neighborHtInfo->curMcsIdx
                     = std::min(neighborHtInfo->maxMcsIdx,
                             Phy802_11GetNumAtnaElems(
                         node->phyData[dot11->myMacData->phyNumber])*10 - 1);
                     neighborHtInfo->isOneSixtyMhzIntolerent
                      = vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;

                 }
                 else
                 {
                      ERROR_Assert(stationItem->data->mscIndex >= 7,
                                   "Incorrect MCS index");
                      neighborHtInfo->curMcsIdx = stationItem->data->mscIndex - 7;
                 }
            }

            // Update QBSS Load's station count;
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                dot11->qBSSLoadElement.stationCount++;
            }
//--------------------DOT11e-End-Updates---------------------------------//
        }

        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
         memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = frame;
        newFrameInfo->frameType = DOT11_REASSOC_RESP;
        newFrameInfo->RA = reqFrame->hdr.sourceAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = INVALID_802ADDRESS;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();
        *frameInfo = newFrameInfo;


        result = DOT11_R_OK;

    } else {  // STATION

        ERROR_ReportError("MacDot11eManagementBuildReassociateResponseFrame:"
                          " Station cannot build reassociation response frame.\n");
        result = DOT11_R_NOT_SUPPORTED;

    }

    return result;

}// MacDot11eManagementBuildReassociateResponseFrame
//--------------------DOT11e-End-Updates---------------------------------//

//--------------------------------------------------------------------------
/*!
 * \brief Build probe request frame to associate with an ESS.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildProbeRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* frame)

{
    int result = DOT11_R_FAILED;


    if (MacDot11IsAp(dot11)) { // ACCESS POINT

        ERROR_ReportError("MacDot11ManagementBuildProbeRequestFrame:"
                          " AP cannot build probe request frame.\n");
        result = DOT11_R_NOT_SUPPORTED;

    } else { // STATION

        // Set the TxRates. Set the PortType

        ERROR_ReportError("MacDot11ManagementBuildProbeRequestFrame:"
                          " Not implemented for station.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

    }

    return result;

}// MacDot11ManagementBuildProbeRequestFrame

/*FUNCTION   :: Dot11_CreateProbeRequestFrame
LAYER      :: MAC
PURPOSE    :: Create a probe request frame.
PARAMETERS ::
+ node      : Node*         : pointer to node
+ dot11     : MacDataDot11* : pointer to Dot11 data structure
+ txFrame   : DOT11_MacFrame* : Tx frame to be created
+ txFrameSize : int*        : frame size after creation
RETURN     :: void
NOTES      :: This function partially creates the beacon frame.
                It adds mesh related elements. Fixed length fields
                are filled in separately.
**/

void Dot11_CreateProbeRespFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Ie* element,
    DOT11_MacFrame* txFrame,
    int* txFrameSize)
{

    *txFrameSize = sizeof(DOT11_ProbeRespFrame);


    // Add Beacon related IEs
    element->data = (unsigned char*)txFrame + *txFrameSize;

    if (MacDot11IsPC(dot11))
    {
        MacDot11CfpParameterSetUpdate(node, dot11);
        Dot11_CreateCfParameterSet(node, element,
                        &dot11->pcVars->cfSet);
        *txFrameSize += element->length;
    }
}

// Dot11_CreateCfParameterSet

//--------------------------------------------------------------------------
/*!
 * \brief Build probe response frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildProbeResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_FrameInfo** frameInfo,
    DOT11_ProbeReqFrame* reqFrame)
{
    int result = DOT11_R_FAILED;

    DOT11_ProbeRespFrame* probe;
    int probeFrameSize = 0;
    DOT11_MacFrame probeFrame;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);

    Dot11_CreateProbeRespFrame(node, dot11, element, &probeFrame, &probeFrameSize);

    MESSAGE_PacketAlloc(
            node,
            msg,
            probeFrameSize,
            TRACE_DOT11);

    // Allocate new Probe frame
    probe = (DOT11_ProbeRespFrame *)MESSAGE_ReturnPacket(msg);
    memset(probe, 0, (size_t)probeFrameSize);
    memcpy(probe, &probeFrame, (size_t)probeFrameSize);
    MEM_free(element);

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_Assert(reqFrame != NULL,
            "MacDot11ManagementBuildProbeResponseFrame: "
            "Request probe response frame is empty.\n");

        // Fill in header
        probe->hdr.frameType = DOT11_PROBE_RESP;
        probe->hdr.destAddr = reqFrame->hdr.sourceAddr;
        probe->hdr.sourceAddr = dot11->selfAddr;
        probe->hdr.address3 = dot11->selfAddr;

        // Fill in frame
        probe->capInfo.ess = 1;         // Add capability info here
        probe->capInfo.ibbs = 0;
        
        probe->beaconInterval = 
            MacDot11ClocktypeToTUs(dot11->beaconInterval);

        // Check SSID is correct
        if (strcmp(reqFrame->SSID, dot11->stationMIB->dot11DesiredSSID) != 0 ){

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

                sprintf(errString,
                    "MacDot11ManagementBuildProbeResponseFrame: "
                    "Station %s SSID (%s) does not match received %s ",
                    macAdder,
                    dot11->stationMIB->dot11DesiredSSID,
                    reqFrame->SSID);

                ERROR_ReportWarning(errString);
            }
        }
        strcpy(probe->ssid, dot11->stationMIB->dot11DesiredSSID);
        probe->timeStamp = node->getNodeTime() + dot11->delayUntilSignalAirborn;
        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = msg;
        newFrameInfo->frameType = DOT11_PROBE_RESP;
        newFrameInfo->RA = reqFrame->hdr.sourceAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = reqFrame->hdr.sourceAddr;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();
        *frameInfo = newFrameInfo;

        result = DOT11_R_OK;


    } else { // STATION
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementBuildProbeResponseFrame

// Build an associate response frame from an ESS
//
// \param node  Node Pointer
// \param dot11  Pointer to Dot11 structure
// \param param frame  Pointer to a new empty frame
// \param param reqFrame  Received the request frame
//
// \return 0 or 1

static // inline
int MacDot11eManagementBuildProbeResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* frame,
    DOT11_FrameInfo** frameInfo,
    DOT11_ProbeReqFrame* reqFrame)
{

    //DOT11e_ProbeRespFrame* probeFrame;
    int result = DOT11_R_FAILED;

    DOT11e_ProbeRespFrame* probe;
    int probeFrameSize = 0;
    DOT11_MacFrame probeFrame;
    memset(&probeFrame, 0, sizeof(DOT11_MacFrame));
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);
    memset(element, 0, sizeof(DOT11_Ie));

    Dot11_CreateProbeRespFrame(node, dot11, element, &probeFrame, &probeFrameSize);


    MacDot11AddQBSSLoadElementToBeacon(node, dot11, element,
                  &probeFrame, &probeFrameSize);

    MacDot11AddEDCAParametersetToBeacon(node, dot11, element,
                  &probeFrame, &probeFrameSize);

        if (dot11->isHTEnable)
        {
            MacDot11AddHTCapability(node, dot11, element,
                  &probeFrame, &probeFrameSize);

            DOT11_HT_OperationElement  operElem;
            if (dot11->isVHTEnable)
            {
                operElem.opChBwdth = CHBWDTH_40MHZ;
            }
            else
            {
                operElem.opChBwdth = (ChBandwidth)Phy802_11nGetOperationChBwdth(
                                    node->phyData[dot11->myMacData->phyNumber]);
            }
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node, dot11, &operElem, 
                &probeFrame, &probeFrameSize);

            if (dot11->isVHTEnable)
            {
                MacDot11AddVHTCapability(node,
                                    dot11,
                                    element,
                                    &probeFrame,
                                    &probeFrameSize);
                MacDot11AddVHTOperation(node,
                                    dot11,
                                    element,
                                    &probeFrame,
                                    &probeFrameSize);
            }
        }

    // Allocate new Probe frame
    MESSAGE_PacketAlloc(
                    node,
                    frame,
                    probeFrameSize,
                    TRACE_DOT11);

    probe = (DOT11e_ProbeRespFrame*)MESSAGE_ReturnPacket(frame);
    memset(probe, 0, (size_t)probeFrameSize);
    memcpy(probe, &probeFrame, (size_t)probeFrameSize);
    MEM_free(element);

    if (MacDot11IsAp(dot11) || dot11->stationType == DOT11_STA_IBSS) {  

        ERROR_Assert(reqFrame != NULL,
            "MacDot11ManagementBuildAuthenticateFrame: "
            "Request associate response frame is empty.\n");


        // Fill in header
        probe->hdr.frameType = DOT11_PROBE_RESP;
        probe->hdr.destAddr = reqFrame->hdr.sourceAddr;
        probe->hdr.sourceAddr = dot11->selfAddr;
        probe->hdr.address3 = dot11->selfAddr;


        // Add capability info here
        probe->capInfo.QoS = 1;
        probe->capInfo.CFPollable = 0;
        probe->capInfo.CFPollRequest = 0;

        if (dot11->isHTEnable)
        {
            probe->capInfo.ImmidiateBlockACK 
                                        = dot11->enableImmediateBAAgreement;
            probe->capInfo.DelayedBlockACK 
                                             = dot11->enableDelayedBAAgreement;
        }


//--------------------HCCA-Updates Start---------------------------------//
        if (MacDot11IsHC(dot11))
        {
            //Use value 01 to specify that HC is present at AP
            probe->capInfo.CFPollable = 0;
            probe->capInfo.CFPollRequest = 1;
        }
//--------------------HCCA-Updates End-----------------------------------//
       probe->beaconInterval = MacDot11ClocktypeToTUs(dot11->beaconInterval);

        // Check SSID is correct
        if (strcmp(reqFrame->SSID, dot11->stationMIB->dot11DesiredSSID) != 0 ){

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

                sprintf(errString,
                    "MacDot11eManagementBuildProbeResponseFrame: "
                    "Station %s SSID (%s) does not match received %s ",
                    macAdder,
                    dot11->stationMIB->dot11DesiredSSID,
                    reqFrame->SSID);

                ERROR_ReportWarning(errString);
            }

        } else {

            strcpy(probe->ssid,dot11->stationMIB->dot11DesiredSSID);

            // Update QBSS Load's station count;
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                dot11->qBSSLoadElement.stationCount++;
            }

        }


        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
         memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = frame;
        newFrameInfo->frameType = DOT11_PROBE_RESP;
        newFrameInfo->RA = reqFrame->hdr.sourceAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = INVALID_802ADDRESS;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();
        *frameInfo = newFrameInfo;

        result = DOT11_R_OK;

    } else {  // STATION

        ERROR_ReportError("MacDot11ManagementBuildAssociateResponseFrame:"
                          " Station cannot build association response frame.\n");
        result = DOT11_R_NOT_SUPPORTED;

    }

    return result;

}// MacDot11eManagementBuildAssociateResponseFrame
//--------------------------------------------------------------------------
/*!
 * \brief Build a reassociation frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildReassociateRequestFrame(
    Node* node,
    MacDataDot11* dot11,
    Message** msg,
    DOT11_FrameInfo** frameInfo)
{
    int result = DOT11_R_FAILED;

    // dot11s. Reassociate within a mesh.
    DOT11_ManagementVars* mngmtVars =
    (DOT11_ManagementVars*) dot11->mngmtVars;

    // Allocate new Associatation frame
    if (MacDot11IsQoSEnabled(node, dot11))
    {
//---------------------DOT11e--Updates------------------------------------//
        DOT11_MacFrame assoc;
        DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);  
        int reAssocFrameSize = sizeof(DOT11e_ReassocReqFrame);
        if (dot11->isHTEnable)
        {
            MacDot11AddHTCapability(node, dot11, element,
                                                        &assoc, &reAssocFrameSize);
              DOT11_HT_OperationElement  operElem;
            if (dot11->isVHTEnable)
            {
                operElem.opChBwdth = CHBWDTH_40MHZ;
            }
            else
            {
                operElem.opChBwdth = (ChBandwidth)Phy802_11nGetOperationChBwdth(
                                    node->phyData[dot11->myMacData->phyNumber]);
            }
            operElem.rifsMode = dot11->rifsMode;
            MacDot11AddHTOperation(node, dot11, &operElem, 
                &assoc, &reAssocFrameSize);

            if (dot11->isVHTEnable)
            {
                MacDot11AddVHTCapability(node,
                                    dot11,
                                    element,
                                    &assoc,
                                    &reAssocFrameSize);
            }
        }

        *msg = MESSAGE_Alloc(node, 0, 0, 0);
        MESSAGE_PacketAlloc(
            node,
            *msg,
            reAssocFrameSize,
            TRACE_DOT11);
        
        DOT11e_ReassocReqFrame* reassocFrame =
            (DOT11e_ReassocReqFrame*) MESSAGE_ReturnPacket((*msg));
        memset(reassocFrame, 0, reAssocFrameSize);
         memcpy(reassocFrame, &assoc, (size_t)reAssocFrameSize);
        MEM_free(element);

        // Fill in header
        reassocFrame->hdr.frameType = DOT11_REASSOC_REQ;
        reassocFrame->hdr.destAddr = mngmtVars->apAddr;
        reassocFrame->hdr.sourceAddr = dot11->selfAddr;
        reassocFrame->hdr.address3 = mngmtVars->apAddr;
        reassocFrame->apAddr = mngmtVars->prevApAddr;

        // Add capability info here
        reassocFrame->capInfo.CFPollable = 0;
        reassocFrame->capInfo.CFPollRequest = 0;
        reassocFrame->capInfo.QoS = 1;

         if (dot11->isHTEnable)
        {
            reassocFrame->capInfo.ImmidiateBlockACK =
                dot11->enableImmediateBAAgreement;
            reassocFrame->capInfo.DelayedBlockACK =
                dot11->enableDelayedBAAgreement;
        }

        reassocFrame->listenInterval = 1;
        strcpy(reassocFrame->ssid, dot11->stationMIB->dot11DesiredSSID);

         DOT11_FrameInfo* newframeInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
         memset(newframeInfo, 0, sizeof(DOT11_FrameInfo));

        newframeInfo->msg = *msg;
        newframeInfo->frameType = DOT11_REASSOC_REQ;
        newframeInfo->RA = mngmtVars->apAddr;
        newframeInfo->TA = dot11->selfAddr;
        newframeInfo->DA = mngmtVars->apAddr;
        newframeInfo->SA = INVALID_802ADDRESS;
        newframeInfo->insertTime = node->getNodeTime();
        *frameInfo = newframeInfo;
        result = DOT11_R_OK;
//--------------------DOT11e-End-Updates---------------------------------//
    } else {

            *msg = MESSAGE_Alloc(node, 0, 0, 0);
            MESSAGE_PacketAlloc(
            node,
            *msg,
            sizeof(DOT11_ReassocReqFrame),
            TRACE_DOT11);

        DOT11_ReassocReqFrame* reassocFrame =
                (DOT11_ReassocReqFrame*) MESSAGE_ReturnPacket((*msg));
        memset(reassocFrame, 0, sizeof(DOT11_ReassocReqFrame));
        // Set the TxRates

        // Set the PortType
        // ess port

        // Fill in header
        reassocFrame->hdr.frameType = DOT11_REASSOC_REQ;
        reassocFrame->hdr.destAddr = mngmtVars->apAddr;
        reassocFrame->hdr.sourceAddr = dot11->selfAddr;
        reassocFrame->hdr.address3 = mngmtVars->apAddr;
        reassocFrame->apAddr = mngmtVars->prevApAddr;

//---------------------------Power-Save-Mode-Updates---------------------//
        // Add listen interval information in association frame request frame
        reassocFrame->listenInterval = (unsigned short)dot11->listenIntervals;
        // SEND Power save Information to AP
        if (MacDot11IsStationSupportPSMode(dot11)){
            reassocFrame->hdr.frameFlags |= DOT11_POWER_MGMT;
        }
//---------------------------Power-Save-Mode-End-Updates-----------------//
        strcpy(reassocFrame->ssid, dot11->stationMIB->dot11DesiredSSID);
        DOT11_FrameInfo* newframeInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(newframeInfo, 0, sizeof(DOT11_FrameInfo));

        newframeInfo->msg = *msg;
        newframeInfo->frameType = DOT11_REASSOC_REQ;
        newframeInfo->RA = mngmtVars->apAddr;
        newframeInfo->TA = dot11->selfAddr;
        newframeInfo->DA = mngmtVars->apAddr;
        newframeInfo->SA = INVALID_802ADDRESS;
        newframeInfo->insertTime = node->getNodeTime();
        *frameInfo = newframeInfo;
        // SA at default
        // fwdControl at default
         result = DOT11_R_OK;

        }
    return result;

}// MacDot11ManagementBuildReassociateRequestFrame


//--------------------------------------------------------------------------
/*!
 * \brief Build a reassociation frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildReassociateResponseFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* frame,
    DOT11_FrameInfo** frameInfo,
    DOT11_ReassocReqFrame* reqFrame)
{
    DOT11_AssocRespFrame* reassocFrame;
    int result = DOT11_R_FAILED;

    // Allocate new Authenticate frame
    reassocFrame = (DOT11_AssocRespFrame *)MESSAGE_ReturnPacket(frame);
    memset(reassocFrame, 0, sizeof(DOT11_AssocRespFrame));

    if (MacDot11IsAp(dot11)) { // ACCESS POINT

        ERROR_Assert(reqFrame != NULL,
            "MacDot11ManagementBuildReassociateResponseFrame: "
            "Request associate response frame is empty.\n");


        // Fill in header
        reassocFrame->hdr.frameType = DOT11_REASSOC_RESP;
        reassocFrame->hdr.destAddr = reqFrame->hdr.sourceAddr;
        reassocFrame->hdr.sourceAddr = dot11->selfAddr;
        reassocFrame->hdr.address3 = dot11->selfAddr;
        if (dot11->stationCheckTimerStarted == FALSE)
        {
         clocktype timerDelay = DOT11_MANAGEMENT_STATION_INACTIVE_TIMEOUT;

             MacDot11StationStartStationCheckTimer(
                 node,
                 dot11,
                 timerDelay,
                 MSG_MAC_DOT11_Station_Inactive_Timer);
             dot11->stationCheckTimerStarted = TRUE;
        }

        // Fill in frame
        // Check SSID is correct
        if (strcmp(reqFrame->ssid, dot11->stationMIB->dot11DesiredSSID) != 0 ){

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&reqFrame->hdr.destAddr);

                sprintf(errString,
                    "MacDot11ManagementBuildReassociateResponseFrame: "
                    "Station %s SSID (%s) does not match received %s ",
                    macAdder,
                    dot11->stationMIB->dot11DesiredSSID,
                    reqFrame->ssid);

                ERROR_ReportWarning(errString);
            }

            reassocFrame->assocId = 0;
            reassocFrame->statusCode = DOT11_SC_UNSPEC_FAILURE;

        } else {

            // Add station to list
            int assocID = MacDot11ApStationListAddStation(
                            node,
                            dot11,
                            reqFrame->hdr.sourceAddr,
//---------------------------Power-Save-Mode-Updates---------------------//
                            0, // QOS deisabled
                            reqFrame->listenInterval,
                            MacDot11IsPSModeEnabledAtSTA(
                                reqFrame->hdr.frameFlags));
//---------------------------Power-Save-Mode-End-Updates-----------------//

            reassocFrame->assocId = (unsigned short)assocID;
            reassocFrame->statusCode = DOT11_SC_SUCCESSFUL;

            // dot11s. Notify reassociaton
            if (dot11->isMAP)
            {
                Dot11s_StationAssociateEvent(node, dot11,
                    reqFrame->hdr.sourceAddr, reqFrame->apAddr);
            }
        }

        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = frame;
        newFrameInfo->frameType = DOT11_REASSOC_RESP;
        newFrameInfo->RA = reqFrame->hdr.sourceAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = reqFrame->hdr.sourceAddr;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();
        *frameInfo = newFrameInfo;

        result = DOT11_R_OK;

    } else {  // STATION

        ERROR_ReportError("MacDot11ManagementBuildRessociateResponseFrame: "
                          "Station cannot build association response frame.\n");
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementBuildReassociateResponseFrame


//--------------------------------------------------------------------------
/*!
 * \brief Build a disassociation notification frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param frame     DOT11_Frame*    : Pointer to a new empty frame

 * \return              None.
 */
//--------------------------------------------------------------------------
static // inline
int MacDot11ManagementBuildDisassociateFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* frame)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        // TODO: Not supported yet...not sure how to do it
        ERROR_ReportError("MacDot11ManagementBuildDisassociateFrame:"
                          " Not implemented for AP.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

    } else { // STATION

        ERROR_ReportError("MacDot11ManagementBuildDisassociateFrame:"
                          " Station cannot build disassociate frame.\n");
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementBuildDisassociateFrame


//--------------------------------------------------------------------------
/*!
 * \brief buils and Transmit probe request frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \return              None.
 */
//--------------------------------------------------------------------------
void MacDot11TransmitProbeRequestFrame(Node* node,MacDataDot11* dot11)
{
    DOT11_ProbeReqFrame* probeFrame;
    //int result = DOT11_R_FAILED;

    // Allocate new probe Request frame
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(
            node,
            msg,
            sizeof(DOT11_ProbeReqFrame),
            TRACE_DOT11);

    probeFrame = (DOT11_ProbeReqFrame*)MESSAGE_ReturnPacket(msg);
    memset(probeFrame, 0, sizeof(DOT11_ProbeReqFrame));

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_Assert(probeFrame == NULL,
            "MacDot11TransmitProbeRequestFrame: "
            "AP Transmitting wrong probe request Frame.\n");
        }
    else{

        // Fill in header
        probeFrame->hdr.frameType = DOT11_PROBE_REQ;
        probeFrame->hdr.destAddr = ANY_MAC802;
        probeFrame->hdr.sourceAddr = dot11->selfAddr;
        probeFrame->hdr.address3 = dot11->selfAddr;
        strcpy(probeFrame->SSID,dot11->stationMIB->dot11DesiredSSID);

        DOT11_FrameInfo* newFrameInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));
        memset(newFrameInfo, 0, sizeof(DOT11_FrameInfo));

        newFrameInfo->msg = msg;
        newFrameInfo->frameType = DOT11_PROBE_REQ;
        newFrameInfo->RA = probeFrame->hdr.destAddr;
        newFrameInfo->TA = dot11->selfAddr;
        newFrameInfo->DA = probeFrame->hdr.destAddr;
        newFrameInfo->SA = dot11->selfAddr;
        newFrameInfo->insertTime = node->getNodeTime();

        dot11->mgmtSendResponse = FALSE;
        MacDot11MgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);
    }
}
//--------------------------------------------------------------------------
// Process Management Frame Functions
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitMgmtResponse
//  PURPOSE:     Transmit Management Response frames.
//               Management response are also used as ACK
//               This assumption should be removed
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address detsAddr
//                  Node address of destination station
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------
static //inline//
void MacDot11StationTransmitMgmtResponse(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr,
    Message* pktToPhy)
{
    // int dataRateType;
    if (dot11->isHTEnable)
    {
        MAC_PHY_TxRxVector tempTxVector;
        tempTxVector = MacDot11nSetTxVector(
                           node, 
                           dot11, 
                           MESSAGE_ReturnPacketSize(pktToPhy),
                           DOT11_ASSOC_RESP, 
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
    }
    MacDot11StationSetState(
            node,
            dot11,
            DOT11_X_MANAGEMENT);

    if (!dot11->useDvcs) {
        MacDot11StationStartTransmittingPacket(
            node,
            dot11,
            pktToPhy,
            dot11->sifs);
    }
    else {
        MacDot11StationStartTransmittingPacketDirectionally(
            node, dot11, pktToPhy, dot11->sifs,
            MacDot11StationGetLastSignalsAngleOfArrivalFromRadio(node, dot11));
    }
}// MacDot11StationTransmitAck

//--------------------------------------------------------------------------
/*!
 * \brief Process a beacon frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessBeacon(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    dot11->pcfEnable = FALSE;
    int result = DOT11_R_FAILED;
    clocktype newNAV = 0;
    int offset;
    DOT11_BeaconFrame* beacon =
            (DOT11_BeaconFrame*) MESSAGE_ReturnPacket(rxFrame);

    DOT11_CFParameterSet cfSet ;
    DOT11_TIMFrame timFrame = {0};
      if (Dot11_GetCfsetFromBeacon(node, dot11, rxFrame, &cfSet))
            {
          dot11->pcfEnable = TRUE;
           // If Cf set is present then set Offset to Point Tim frame

         offset = (sizeof(DOT11_BeaconFrame)+ sizeof(DOT11_CFParameterSet)+
                    DOT11_IE_HDR_LENGTH);

        Dot11_GetTimFromBeacon(node, dot11, rxFrame, offset, &timFrame);
        dot11->DTIMCount =  timFrame.dTIMCount;

        clocktype beaconTransmitTime = 0;

        if (dot11->isHTEnable)
        {
            MAC_PHY_TxRxVector rxVector;
            PHY_GetRxVector(node,
                            dot11->myMacData->phyNumber,
                            rxVector);
            beaconTransmitTime = 
                node->getNodeTime() - PHY_GetTransmissionDuration(
                                        node,
                                        dot11->myMacData->phyNumber,
                                        rxVector);
        }
        else
        {
            int dataRateType = 
                          PHY_GetRxDataRateType(node,
                                                dot11->myMacData->phyNumber);

            beaconTransmitTime = node->getNodeTime() -
                                        PHY_GetTransmissionDuration(
                                            node,
                                            dot11->myMacData->phyNumber,
                                            dataRateType,
                                            MESSAGE_ReturnPacketSize(rxFrame));
        }

         newNAV = beaconTransmitTime;


         if ((unsigned short)cfSet.cfpBalDuration > 0)
         {
                newNAV +=
                    MacDot11TUsToClocktype((unsigned short)cfSet.cfpBalDuration)
            + EPSILON_DELAY;
         }

    }


    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
      if (MacDot11IsAp(dot11) && !MacDot11IsPC(dot11))
        {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementProcessBeacon:"
                          " AP mode beacon process not implimented.\n");
            if (dot11->pcfEnable){
              if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0){
                if (newNAV > dot11->NAV)
                {
                    dot11->NAV = newNAV;
                    mc(dot11)->updateStats("NavUpdated", dot11->NAV);
                }
              }
            }
            result = DOT11_R_NOT_IMPLEMENTED;

        }
        // Point Coordinator never Set NAV
        if (MacDot11IsPC(dot11))
        {
            ERROR_ReportError("MacDot11ManagementProcessBeacon:"
                          " PC mode beacon process not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

        }



                    //else {  // STATION
    if (MacDot11IsBssStation (dot11)){
        //ToDo: change the below code
        if (MacDot11IsAssociationDynamic(dot11) &&
            mngmtVars->state >= DOT11_S_M_SCAN_FINISHED &&
            mngmtVars->state < DOT11_S_M_JOINED)
        {
            DOT11_FrameHdr * hdr =
                (DOT11_FrameHdr *) &beacon->hdr;
            //Auth Process already started ignore beacons
            //from other APs in the BSS
            if (mngmtVars->apAddr == hdr->sourceAddr)
            {
                if (dot11->pcfEnable)
                {
                    if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0){
                       if (newNAV > dot11->NAV)
                       {
                            dot11->NAV = newNAV;
                            mc(dot11)->updateStats("NavUpdated", dot11->NAV);
                       }
                       if (DEBUG_PCF && DEBUG_BEACON){
                        MacDot11Trace(node, dot11, NULL,
                        "Station recieved CFP Start Beacon and"
                        "set NAV for CF Duration");
                       }
                    }
                }

                // clocktype beaconTimeStamp = beacon->timeStamp;
                // dot11->NAV = 0;
                 dot11->beaconTime = beacon->timeStamp;

                // Don't reset the beacon missed. It is assumed that STA will be able to
                // authenticate, associate and join whithing 3 beacon period.
                // STA will reset itself if unable to join.
                // dot11->beaconsMissed = 0;

                MacDot11StationCancelBeaconTimer(node, dot11);
                MacDot11StationCancelTimer(node, dot11);

                // Set new beacon interval
                dot11->beaconInterval =
                    MacDot11TUsToClocktype((unsigned short)beacon->beaconInterval);

                // Start timer for next expected beacon
                MacDot11StationStartBeaconTimer(node, dot11);
            }
        }
        else
        if (mngmtVars->state == DOT11_S_M_WFSCAN &&
            strcmp(beacon->ssid, dot11->stationMIB->dot11DesiredSSID) == 0 )
        {

             // BSS station (also AP) receives beacon from outside BSS.
             // IBSS station receives beacon.
                  if (dot11->pcfEnable)
                    {
                     if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0){
                         if (newNAV > dot11->NAV)
                         {
                            dot11->NAV = newNAV;
                            mc(dot11)->updateStats("NavUpdated", dot11->NAV);
                         }
                         if (DEBUG_PCF && DEBUG_BEACON){
                        MacDot11Trace(node, dot11, NULL,
                        "Station recieved CFP Start Beacon and"
                        "set NAV for CF Duration");
                       }

                    }
                }

            DOT11_FrameHdr * hdr =
                (DOT11_FrameHdr *) &beacon->hdr;

            //Add or update AP info for channel scan
            DOT11_ApInfo* apInfo =
                MacDot11ManagementGetAPInfo(node, dot11,
                                        hdr->sourceAddr,cfSet,
                                        beacon->beaconInterval);

            if (dot11->pcfEnable)
            {
            if (cfSet.cfpCount == 0 && timFrame.dTIMCount == 0){
                apInfo->lastCfpBeaconTime = beacon->timeStamp;
                }
            }
            apInfo->channelId = mngmtVars->currentChannel;
            apInfo->qosEnabled = FALSE;
            
            if (MacDot11IsQoSEnabled(node, dot11))
            {
                DOT11e_CapabilityInfo *capInfo =
                    (DOT11e_CapabilityInfo *)(&beacon->capability);
                // Check for QAP
                if (capInfo->QoS)
                {
                    apInfo->qosEnabled = TRUE;
                }
                else
                {
                    apInfo->qosEnabled = FALSE;
                }
            }

        if (dot11->isHTEnable)
        {
            DOT11e_CapabilityInfo *capInfo =
            (DOT11e_CapabilityInfo *)(&beacon->capability);
            memcpy(&apInfo->apCapInfo, capInfo, sizeof(DOT11e_CapabilityInfo));
            DOT11_HT_CapabilityElement htCapabilityElement;
            BOOL isPresent = Dot11_GetHTCapabilities(
                node,
                dot11,
                rxFrame,
                &htCapabilityElement,
                 sizeof(DOT11_BeaconFrame));
            if (isPresent)
            {
                apInfo->htEnableAP = TRUE;
                apInfo->staHtCapabilityElement = htCapabilityElement;

                //Use mcs which is smaller from what this 
                //AP supports and what obtained via htcapability
                if (htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx 
                                    < dot11->txVectorForHighestDataRate.mcs)
                {
                    apInfo->mcsIndex =
                        htCapabilityElement.supportedMCSSet.mcsSet.maxMcsIdx;
                }
                else
                {
                    apInfo->mcsIndex =
                        dot11->txVectorForHighestDataRate.mcs;
                }
                MacDot11nUpdNeighborHtInfo(node, dot11, 
                    hdr->sourceAddr, htCapabilityElement);
            }
            else
            {
                apInfo->htEnableAP = FALSE;
                apInfo->mcsIndex = 0;
            }
            if (dot11->isVHTEnable)
            {
                VHT_CapabilitiesElement vhtCapabilityElement;
                isPresent = Dot11_GetVHTCapabilities(
                                node,
                                dot11,
                                rxFrame,
                                &vhtCapabilityElement,
                                sizeof(DOT11e_ProbeRespFrame));
                ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht capability"
                   "element in management frame");
                apInfo->vhtEnableAP = TRUE;
                apInfo->vhtInfo.staVhtCapabilityElement
                    = vhtCapabilityElement;
                VHT_OperationElement vhtOperationElement;
                isPresent = Dot11_GetVHTOperation(
                                        node,
                                        dot11,
                                        rxFrame,
                                        &vhtOperationElement,
                                        sizeof(DOT11e_ProbeRespFrame));
                ERROR_Assert(isPresent,
                    "VHT Enabled node must contain vht operation element"
                    "in management response frame");
                apInfo->vhtInfo.vhtOperationElement
                    = vhtOperationElement;
                DOT11_NeighborHtInfo* neighHtInfo =
                        MacDot11nGetNeighHtInfo(dot11->htInfoList,
                                       hdr->sourceAddr);
                ERROR_Assert(neighHtInfo,"neighHtInfo should not be NULL");
                neighHtInfo->maxMcsIdx
                    = (int)vhtCapabilityElement.m_mcsNssSet;
                neighHtInfo->curMcsIdx
                 = std::min(neighHtInfo->maxMcsIdx,
                         Phy802_11GetNumAtnaElems(
                     node->phyData[dot11->myMacData->phyNumber])*10 - 1);
                neighHtInfo->isOneSixtyMhzIntolerent
                = vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;
            }
        }

            //Update signal strength
            MacDot11StationUpdateAPMeasurement(node, dot11, apInfo, rxFrame);

//---------------------DOT11e--Updates------------------------------------//
            // Note: Since AC's are one packet buffer thus
            // QOSLoad info and EDCF Parameter set would not give
            // any effect as they will be same as default so it is
            // commented out to have some speed.
            // if (MacDot11IsQoSEnabled(node, dot11))
            // memcpy(&dot11->qBSSLoadElement, &beacon->qBSSLoadElement,
            //  sizeof(DOT11e_QBSSLoadElement));
            // memcpy(&dot11->edcaParamSet, &beacon->edcaParamSet,
            //  sizeof(DOT11e_EDCAParameterSet));
//--------------------DOT11e-End-Updates---------------------------------//
        }

        result = DOT11_R_OK;

    }

    return result;

}// MacDot11ManagementProcessBeacon


//--------------------------------------------------------------------------
/*!
 * \brief Process an authentication frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessAuthenticate(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;


    // Simplistic authentication routine
    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        MacDot11Trace(node, dot11, rxFrame, "Receive");

        // Get the request frame
        DOT11_AuthFrame * respFrame =
            (DOT11_AuthFrame *) MESSAGE_ReturnPacket(rxFrame);

        // Alloc Authenticate response frame
        Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

        MESSAGE_PacketAlloc(
            node,
            msg,
            sizeof(DOT11_AuthFrame),
            TRACE_DOT11);

        DOT11_FrameInfo* frameInfo = NULL;
        // Build response frame
        if (MacDot11ManagementBuildAuthenticateFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                respFrame) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementProcessAuthenticate:"
                                "Build authenticate frame failed. \n");
        }

        //Don't set the send frame. Response frame are
        //getting transmitted after SIFS time

        dot11->mgmtSendResponse = TRUE;
        MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);

        result = DOT11_R_OK;

    } else {  // STATION

        //
        DOT11_AuthFrame * respFrame =
            (DOT11_AuthFrame *) MESSAGE_ReturnPacket(rxFrame);

        if ((respFrame->statusCode == DOT11_SC_SUCCESSFUL) &&
            (respFrame->transSeqNo == 2) )
        {

            MacDot11Trace(node, dot11, rxFrame, "Receive");
            if (dot11->isHTEnable)
            {
            }
            else
            {
            MacDot11StationTransmitAck(node,dot11,respFrame->hdr.sourceAddr);
            }
            MacDot11AuthenticationCompleted(node,dot11);

        } else {

            MacDot11Trace(
                node,
                dot11,
                NULL,
                "WARNING authentication failed.");

            if (DEBUG_MANAGEMENT){
                char errString[MAX_STRING_LENGTH];
                char macAdder[24];
                MacDot11MacAddressToStr(macAdder,&respFrame->hdr.destAddr);
                char macAdderTemp[24];
                MacDot11MacAddressToStr(macAdder,&respFrame->hdr.sourceAddr);

                sprintf(errString,
                    "MacDot11ManagementProcessAuthenticate: "
                    "STA %s failed authentication with AP %s with code %u ",
                    macAdder, macAdderTemp, respFrame->statusCode);

                ERROR_ReportWarning(errString);
            }
        }
    }

    return result;

}// MacDot11ManagementProcessAuthenticate


//--------------------------------------------------------------------------
/*!
 * \brief Process an deauthenticate frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessDeauthenticate(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementProcessDeauthenticate:"
                          " AP mode deauthenticate not implimented.\n");

        result = DOT11_R_NOT_IMPLEMENTED;

    } else {  // STATION
        ERROR_ReportError("MacDot11ManagementProcessDeauthenticate:"
                          " AP mode deauthenticate not implimented.\n");

        result = DOT11_R_NOT_IMPLEMENTED;
    }

    return result;

}// MacDot11ManagementProcessDeauthenticate


//--------------------------------------------------------------------------
/*!
 * \brief Process an associate frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessAssociateRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    DOT11_FrameInfo* frameInfo = NULL;

    DOT11e_AssocReqFrame* rxARqFrame =
        (DOT11e_AssocReqFrame*)MESSAGE_ReturnPacket(rxFrame);

    BOOL isQSTA = FALSE;
    if (rxARqFrame->capInfo.QoS)
    {
        isQSTA = TRUE;
    }

    MacDot11Trace(node, dot11, rxFrame, "Receive");

    // Alloc Association response frame
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

 //---------------------DOT11e--Updates------------------------------------//
    if (MacDot11IsQoSEnabled(node, dot11) && isQSTA)
    {

        if (MacDot11eManagementBuildAssociateResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                rxFrame) != DOT11_R_OK )
        {
            MESSAGE_Free(node, msg);
            ERROR_ReportWarning("MacDot11eManagementProcessAssociateRequest:"
                                "Build associate response failed. \n");
            return DOT11_R_FAILED;
      //  }
       }

        if (dot11->isHTEnable)
        {
              DOT11_ApStationListItem* stationItem = NULL;
              stationItem =
              MacDot11ApStationListGetItemWithGivenAddress(
                                            node,
                                            dot11,
                                            rxARqFrame->hdr.sourceAddr);
            if (stationItem)
            {
                InputBuffer *inputBuffer = dot11->inputBuffer;
                inputBuffer->tmpKey.first = rxARqFrame->hdr.sourceAddr;
                for (int i =0; i < 8; i++)
                {
                    inputBuffer->tmpKey.second = (TosType)i;
                    if (inputBuffer->checkAndGetKeyPosition())
                    {
                         inputBuffer->Mapitr->second.winStarts = 0;
                         inputBuffer->Mapitr->second.winEnds = 63;
                         inputBuffer->Mapitr->second.winSizes = 64;
                         inputBuffer->Mapitr->second.aggSize = 0;
                         inputBuffer->Mapitr->second.seqNum = 0;
                         inputBuffer->Mapitr->second.winStartr =
                             MACDOT11N_INVALID_SEQ_NUM;
                         inputBuffer->Mapitr->second.winEndr =
                             MACDOT11N_INVALID_SEQ_NUM;
                         inputBuffer->Mapitr->second.winSizer = 0;
                         inputBuffer->Mapitr->second.BABitmap = 0;
                         if (inputBuffer->Mapitr->second.blockAckVrbls)
                         {
                            MacDot11nResetbap(inputBuffer->Mapitr);
                         }
                        if (inputBuffer->Mapitr->second.timerMsg)
                        {
                            MESSAGE_CancelSelfMsg(node,
                                inputBuffer->Mapitr->second.timerMsg);
                            inputBuffer->Mapitr->second.timerMsg = NULL;
                        }
                         std::list<struct DOT11_FrameInfo*>::iterator itr =
                             inputBuffer->Mapitr->second.frameQueue.begin();
                         while (itr != inputBuffer->Mapitr->second.frameQueue.end())
                         {
                              DOT11_FrameInfo *frameInfo = (*itr);
                              MacDot11nHandleDequeuePacketFromInputBuffer(node,
                                  dot11, frameInfo);
                              dot11->inputBuffer->Mapitr->
                                  second.frameQueue.erase(itr);
                              dot11->numPktsDequeuedFromInputBuffer++;
                              inputBuffer->Mapitr->second.numPackets--;
                               if (inputBuffer->Mapitr->second.frameQueue.size() > 0)
                               {
                                   itr =
                                     inputBuffer->Mapitr->second.frameQueue.begin();
                               }
                               else
                               {
                                   ERROR_Assert(inputBuffer->
                                       Mapitr->second.numPackets == 0,
                                       "More packets in input buffer");
                                   break;
                               }
                           }
                        }
                    }
                }
            }

//--------------------DOT11e-End-Updates---------------------------------//
    }
    else
    {

        // Get the request frame
        DOT11_AssocReqFrame * respFrame =
            (DOT11_AssocReqFrame *) MESSAGE_ReturnPacket(rxFrame);

        MESSAGE_PacketAlloc(
            node,
            msg,
            sizeof(DOT11_AssocRespFrame),
            TRACE_DOT11);

        // Build response frame
        if (MacDot11ManagementBuildAssociateResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                    respFrame) != DOT11_R_OK)
         {
            MESSAGE_Free(node, msg);
            ERROR_ReportWarning("MacDot11ManagementProcessAssociateRequest:"
                                "Build associate response failed. \n");
            return DOT11_R_FAILED;
         }
    }

    //Don't set the send frame. Response frame are
    //getting transmitted after SIFS time

    MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);

    result = DOT11_R_OK;
    return result;

}// MacDot11ManagementProcessAssociateRequest


//--------------------------------------------------------------------------
/*!
 * \brief Process an associate response.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessAssociateResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    DOT11_AssocRespFrame * respFrame =
        (DOT11_AssocRespFrame *) MESSAGE_ReturnPacket(rxFrame);

    if ((respFrame->statusCode == DOT11_SC_SUCCESSFUL) &&
        (respFrame->assocId > 0) )
    {

        MacDot11Trace(node, dot11, rxFrame, "Receive");

        // Set that station is authenticated
        dot11->stationAssociated = TRUE;
//---------------------------Power-Save-Mode-Updates---------------------//
// Receive the association ID value from AP and store this value at STAs
        dot11->assignAssociationId = respFrame->assocId;
//---------------------------Power-Save-Mode-End-Updates-----------------//
        MacDot11AssociationCompleted(node,dot11);
        // DOT11e Updates
        if (MacDot11IsQoSEnabled(node, dot11))
        {
            // Check for QAP
            DOT11e_AssocRespFrame* dot11erspFrame =
                (DOT11e_AssocRespFrame *) MESSAGE_ReturnPacket(rxFrame);


//--------------------HCCA-Updates Start---------------------------------//
            if (dot11erspFrame->capInfo.QoS)
            {
                dot11->associatedWithQAP = TRUE;
                if (dot11erspFrame->capInfo.CFPollRequest ||
                    dot11erspFrame->capInfo.CFPollable)
                {
                    dot11->associatedWithHC = TRUE;
                }
                else
                {
                    dot11->associatedWithHC = FALSE;
                }
            } else {
                dot11->associatedWithQAP = FALSE;
                dot11->associatedWithHC = FALSE;
            }
            if (dot11->isHTEnable)
        {

        DOT11e_CapabilityInfo *capInfo =
                    (DOT11e_CapabilityInfo *)(&dot11erspFrame->capInfo);
        memcpy(&dot11->associatedAP->apCapInfo, capInfo, sizeof(DOT11e_CapabilityInfo));
        BOOL rifsMode = FALSE;
        BOOL offset = FALSE;
        DOT11_HT_OperationElement operElem;
        BOOL isPresent = Dot11_GetHTOperation(node, dot11, rxFrame, &operElem,
                            sizeof(DOT11e_AssocRespFrame));
        if (isPresent)
        {
            rifsMode = operElem.rifsMode;
        }
        if (dot11->isVHTEnable)
        {
            VHT_CapabilitiesElement vhtCapabilityElement;
            BOOL isPresent = Dot11_GetVHTCapabilities(
                            node,
                            dot11,
                            rxFrame,
                            &vhtCapabilityElement,
                            sizeof(DOT11e_AssocRespFrame));
             ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht capability"
                   "element in management frame");

             if ((ChBandwidth)Phy802_11GetOperationChBwdth(
               node->phyData[dot11->myMacData->phyNumber]) == CHBWDTH_160MHZ
                && !vhtCapabilityElement.m_capabilitiesInfo.m_chWidth)
             {
                 Phy802_11SetOperationChBwdth(
                             node->phyData[dot11->myMacData->phyNumber],
                             (ChBandwidth)CHBWDTH_80MHZ);
             }
#ifdef SPECTRAL_ALLIGN
             VHT_OperationElement vhtOperationElement;
             isPresent = Dot11_GetVHTOperation(
                                node,
                                dot11,
                                rxFrame,
                                &vhtOperationElement,
                                sizeof(DOT11e_AssocRespFrame));
             ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht operation "
                   "element in management response frame");

             PHY802_11AllignSpectralBand(node->phyData[dot11->myMacData->phyNumber],
                    operElem.secondaryChOffset,
                    (ChBandwidth)vhtCapabilityElement.m_capabilitiesInfo.m_chWidth,
                    vhtOperationElement.m_operationInfo.m_chCenterFreqSeg0);
#endif
        }
        else
        {
            // Setting physical operational channel bandwidth according
            // to the setting of AP

            if ((ChBandwidth)Phy802_11nGetOperationChBwdth(
                node->phyData[dot11->myMacData->phyNumber])
                != (ChBandwidth)operElem.opChBwdth)
            {
                Phy802_11nSetOperationChBwdth(
                    node->phyData[dot11->myMacData->phyNumber],
                    (ChBandwidth)CHBWDTH_20MHZ);
            }
            else
            {
                Phy802_11nSetOperationChBwdth(
                node->phyData[dot11->myMacData->phyNumber],
                (ChBandwidth)operElem.opChBwdth);
            }
        }
        DOT11_HT_CapabilityElement htCapabilityElement;
        isPresent = Dot11_GetHTCapabilities(
            node,
            dot11,
            rxFrame,
            &htCapabilityElement,
             sizeof(DOT11e_AssocRespFrame));
        
         if (isPresent)
        {
            // Update the data rate entry!
            ERROR_Assert(dot11->associatedAP != NULL,"not associated");
            DOT11_DataRateEntry* neighborEntry = 
                MacDot11StationGetDataRateEntry(node, 
                                                dot11,
                                                  dot11->associatedAP->bssAddr);
            neighborEntry->txVector.mcs = dot11->associatedAP->mcsIndex;
            neighborEntry->txVector.format =
                    htCapabilityElement.htCapabilitiesInfo.htGreenfield?
                    (Mode)MODE_HT_GF: (Mode)MODE_NON_HT;
            neighborEntry->txVector.chBwdth = 
                htCapabilityElement.htCapabilitiesInfo.fortyMhzIntolerent?
                    (ChBandwidth)CHBWDTH_20MHZ: (ChBandwidth)CHBWDTH_40MHZ;
            neighborEntry->txVector.stbc =
                    (UInt8)htCapabilityElement.htCapabilitiesInfo.rxStbc;
            dot11->associatedAP->rifsMode = rifsMode;
        }


        
    }

//--------------------HCCA-Updates End-----------------------------------//
        }

        } else {

            // TODO: Need some smarts here to handle a failed association

            MacDot11Trace(node, dot11, NULL,
                "WARNING association failed.");

            char errString[MAX_STRING_LENGTH];
            char srcAddr[24],destAddr[24];
            MacDot11MacAddressToStr(srcAddr, &respFrame->hdr.sourceAddr);
            MacDot11MacAddressToStr(destAddr, &respFrame->hdr.destAddr);
            sprintf(errString,
                    "MacDot11ManagementProcessAssociateResponse: "
                    "STA %s failed to associate with AP %s with code %u ",
                    destAddr,
                    srcAddr,
                    respFrame->statusCode);

            ERROR_ReportWarning(errString);
         }
    return result;

}// MacDot11ManagementProcessAssociateResponse

 //---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11ManagementProcessATIMFrame
//  PURPOSE:     Management Process ATIM frame
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               DOT11_Frame* rxFrame
//                  Receive frame header
//  RETURN:      int
//  ASSUMPTION:  None
//--------------------------------------------------------------------------
int MacDot11ManagementProcessATIMFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    DOT11_ATIMFrame* atimFrame =
        (DOT11_ATIMFrame *) MESSAGE_ReturnPacket(rxFrame);

    if (atimFrame->destAddr == ANY_MAC802){
        // No need to transmit ACK for broadcast ATIM frame
        result = DOT11_R_OK;
        dot11->noOfReceivedBroadcastATIMFrame++;
            // Process ATIM frame here
            if (DEBUG_PS && DEBUG_PS_ATIM)
                MacDot11Trace(node, dot11, rxFrame, "Receive Broadcast ATIM Frame");
    }
    else {
        MacDot11SetReceiveATIMFrame(dot11, atimFrame->sourceAddr);
        // transmit ACK frame
        MacDot11StationCancelTimer(node, dot11);
        MacDot11StationTransmitAck(node, dot11, atimFrame->sourceAddr);
        result = DOT11_R_OK;
    }
    return result;
}// MacDot11ManagementProcessATIMFrame
//---------------------------Power-Save-Mode-End-Updates-----------------//

//--------------------------------------------------------------------------
/*!
 * \brief Process a probe request.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessProbeRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;

    DOT11_FrameInfo* frameInfo = NULL;

    MacDot11Trace(node, dot11, rxFrame, "Receive");

        // Alloc Association response frame
        Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

    if (MacDot11IsQoSEnabled(node, dot11))
    {
        DOT11_ProbeReqFrame* respFrame =
        (DOT11_ProbeReqFrame *) MESSAGE_ReturnPacket(rxFrame);

        // Build response frame
        if (MacDot11eManagementBuildProbeResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                respFrame) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementProcessProbeRequest:"
                                "Build probe response failed. \n");
            }

            MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);
            result = DOT11_R_OK;
    }
    else{
        DOT11_ProbeReqFrame * respFrame =
        (DOT11_ProbeReqFrame *) MESSAGE_ReturnPacket(rxFrame);

        // Build response frame
        if (MacDot11ManagementBuildProbeResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                respFrame) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementProcessProbeRequest:"
                                "Build probe response failed. \n");
        }
        MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);
        result = DOT11_R_OK;
     }

    result = DOT11_R_NOT_SUPPORTED;
    return result;

}// MacDot11ManagementProcessProbeRequest

/// ---------------------------------------------------------
// FUNCTION   :: Dot11_GetCfsetFromBeacon
// LAYER      :: MAC
// PURPOSE    :: Receive and process a beacon frame if it has CfSet elements.
// PARAMETERS ::
//   + node      : Node*         : pointer to node
//   + dot11     : MacDataDot11* : pointer to Dot11 data structure
//   + msg       : Message*      : received beacon message
//   + cfSet     : DOT11_CFParameterSet* cfSet : wReturn the CfSet
// RETURN     ::BOOL
//-------------------------------------------------------------------

static
BOOL Dot11_GetCfsetFromProbeResponse(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    DOT11_CFParameterSet* cfSet)
{
    BOOL isFound = FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    ERROR_Assert(frameHdr->frameType == DOT11_PROBE_RESP,
        "Dot11_GetCfsetFromBeacon: Wrong frame type.\n");

    int offset = sizeof(DOT11_ProbeRespFrame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        return isFound;
    }

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;

    // The first expected IE is cfSet
    while (ieHdr.id != DOT11_CF_PARAMETER_SET_ID)
    {
        offset += ieHdr.length + 2;
        if (offset >= rxFrameSize)
        {
            return isFound;
        }
        ieHdr = rxFrame + offset;
    }

    if (ieHdr.id == DOT11_CF_PARAMETER_SET_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11_ParseCfsetIdElement(node, &element, cfSet);

        isFound = TRUE;
    }
    return isFound;
}//Dot11_GetCfsetFromBeacon

//--------------------------------------------------------------------------
/*!
 * \brief Process a probe response.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessProbeResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    DOT11_CFParameterSet cfSet;

    DOT11_ProbeRespFrame * respFrame =
    (DOT11_ProbeRespFrame *) MESSAGE_ReturnPacket(rxFrame);
    MacDot11Trace(node, dot11, rxFrame, "Receive");
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
    Dot11_GetCfsetFromProbeResponse(node,dot11,rxFrame,&cfSet);
    if (dot11->stationType == DOT11_STA_IBSS)
    {
         DOT11e_ProbeRespFrame * respFrame =
             (DOT11e_ProbeRespFrame *) MESSAGE_ReturnPacket(rxFrame); 
        DOT11n_IBSS_Station_Info *stationInfo =  
                        MacDot11nGetIbssStationInfo(dot11,
                                                    respFrame->hdr.sourceAddr);
        if (stationInfo->probeStatus == IBSS_PROBE_IN_PROGRESS)
        {
           ERROR_Assert(stationInfo->timerMessage,"timer not present");
           MESSAGE_CancelSelfMsg(node, stationInfo->timerMessage);
           stationInfo->timerMessage = NULL;
           stationInfo->probeStatus = IBSS_PROBE_COMPLETED;
           DOT11e_CapabilityInfo *capInfo 
               = (DOT11e_CapabilityInfo *)(&respFrame->capInfo);
           memcpy(&stationInfo->staCapInfo, 
                            capInfo, 
                            sizeof(DOT11e_CapabilityInfo));
            DOT11_HT_CapabilityElement htCapabilityElement;
            DOT11_HT_OperationElement operElem;
            BOOL isPresent
                = Dot11_GetHTOperation(node,
                                       dot11,
                                       rxFrame,
                                       &operElem,
                                       sizeof(DOT11e_ProbeRespFrame));
            if (isPresent)
            {
                stationInfo->rifsMode = operElem.rifsMode;
            }
            isPresent = Dot11_GetHTCapabilities(
                                            node,
                                            dot11,
                                            rxFrame,
                                            &htCapabilityElement,
                                            sizeof(DOT11e_ProbeRespFrame));
            if (isPresent)
            {
                memcpy(&stationInfo->staHtCapabilityElement, 
                                &htCapabilityElement,
                                sizeof(DOT11_HT_CapabilityElement));
            }
            else
            {
                ERROR_Assert(FALSE,"invalid condition");
            }
            MacDot11nUpdNeighborHtInfo(node, dot11,
                    respFrame->hdr.sourceAddr, htCapabilityElement);
            if (dot11->isVHTEnable)
            {
                VHT_CapabilitiesElement vhtCapabilityElement;
                isPresent = Dot11_GetVHTCapabilities(
                                    node,
                                    dot11,
                                    rxFrame,
                                    &vhtCapabilityElement,
                                    sizeof(DOT11e_ProbeRespFrame));
                ERROR_Assert(isPresent,
                   "VHT Enabled node must contain vht capability"
                   "element in management frame");
                stationInfo->vhtInfo.staVhtCapabilityElement
                        = vhtCapabilityElement;
                DOT11_NeighborHtInfo* neighHtInfo =
                                MacDot11nGetNeighHtInfo(dot11->htInfoList,
                                                respFrame->hdr.sourceAddr);
                neighHtInfo->maxMcsIdx =
                                     (int)vhtCapabilityElement.m_mcsNssSet;
                neighHtInfo->curMcsIdx =
                  std::min(neighHtInfo->maxMcsIdx, Phy802_11GetNumAtnaElems(
                         node->phyData[dot11->myMacData->phyNumber])*10 - 1);
                neighHtInfo->isOneSixtyMhzIntolerent =
                     vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;
            }
            if ((dot11->enableImmediateBAAgreement
                || dot11->enableDelayedBAAgreement)
                && (stationInfo->staCapInfo.ImmidiateBlockACK
                || stationInfo->staCapInfo.DelayedBlockACK))
            {
                for (int i=0; i <4; i++)
                {
                    OutputBuffer *outputBuffer
                        = getAc(dot11, i).m_outputBuffer;
                    std::pair<Mac802Address,TosType> tmpKey;
                    tmpKey.first = respFrame->hdr.sourceAddr;
                    for (int j=0; j < 8; j++)
                    {
                        tmpKey.second = j;
                        std::map<std::pair<Mac802Address,TosType>,
                            MapValue>::iterator keyItr;
                        keyItr = outputBuffer->OutputBufferMap.find(tmpKey);
                        if (keyItr != outputBuffer->OutputBufferMap.end())
                        {
                            if (!keyItr->second.blockAckVrbls)
                            {
                                keyItr->second.blockAckVrbls
                                    = (DOT11n_BlockAckAggrementVrbls*)
                                MEM_malloc(
                                    sizeof(DOT11n_BlockAckAggrementVrbls));
                                memset(keyItr->second.blockAckVrbls, 
                                       0,
                                       sizeof(
                                       DOT11n_BlockAckAggrementVrbls));
                                if (stationInfo->
                                        staCapInfo.DelayedBlockACK)
                                {
                                    keyItr->second.blockAckVrbls->type
                                        = BA_DELAYED;
                                }
                                else
                                {
                                    keyItr->second.blockAckVrbls->type
                                        = BA_IMMEDIATE;
                                }
                                keyItr->second.blockAckVrbls->startingSeqNo 
                                    = MACDOT11N_INVALID_SEQ_NUM;
                                keyItr->second.blockAckVrbls->
                                    numPktsNegotiated = 0;
                                keyItr->second.blockAckVrbls->
                                    numPktsSent = 0;
                                keyItr->second.blockAckVrbls->
                                    numPktsLeftToBeSentInCurrentTxop = 0;
                                keyItr->second.blockAckVrbls->
                                    numPktsToBeSentInCurrentSession = 0;
                                keyItr->second.blockAckVrbls->state
                                    = BAA_STATE_DISABLED;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // Ignore the received probe response as this nterface is not
            // expecting any probe response right now.
        }
    }
    else
    {
        //Add or update AP info for channel scan
        DOT11_ApInfo* apInfo =
        MacDot11ManagementGetAPInfo(node,
                                    dot11,
                                    respFrame->hdr.sourceAddr,cfSet,
                                    respFrame->beaconInterval);

        apInfo->channelId = mngmtVars->currentChannel;
        if (MacDot11IsQoSEnabled(node, dot11))
        {
            DOT11e_CapabilityInfo *capInfo =
                            (DOT11e_CapabilityInfo *)(&respFrame->capInfo);
            // Check for QAP
            if (capInfo->QoS)
            {
                apInfo->qosEnabled = TRUE;
            }
            else
            {
                apInfo->qosEnabled = FALSE;
            }
            if (dot11->isHTEnable)
            {
                DOT11e_CapabilityInfo *capInfo =
                (DOT11e_CapabilityInfo *)(&respFrame->capInfo);
                memcpy(&apInfo->apCapInfo, capInfo, sizeof(DOT11e_CapabilityInfo));
                DOT11_HT_CapabilityElement htCapabilityElement;

                DOT11_HT_OperationElement operElem;
                BOOL isPresent = Dot11_GetHTOperation(node,
                                                      dot11,
                                                      rxFrame,
                                                      &operElem,
                sizeof(DOT11_ProbeRespFrame));
                if (isPresent)
                {
                    apInfo->rifsMode = operElem.rifsMode;
                }

                isPresent = Dot11_GetHTCapabilities(node,
                                                dot11,
                                                rxFrame,
                                                &htCapabilityElement,
                                                sizeof(DOT11e_ProbeRespFrame));
                if (isPresent)
                {
                    apInfo->htEnableAP = TRUE;
                    apInfo->staHtCapabilityElement =
                        htCapabilityElement;
                    if (htCapabilityElement.supportedMCSSet.
                        mcsSet.maxMcsIdx < dot11->txVectorForHighestDataRate.mcs)
                    {
                        apInfo->mcsIndex = htCapabilityElement.
                            supportedMCSSet.mcsSet.maxMcsIdx;
                    }
                    else
                    {
                        apInfo->mcsIndex =
                            dot11->txVectorForHighestDataRate.mcs;
                    }

                    DOT11e_ProbeRespFrame* respFrame =
                        (DOT11e_ProbeRespFrame*) MESSAGE_ReturnPacket(rxFrame);

                    apInfo->apCapInfo = respFrame->capInfo;

                    MacDot11nUpdNeighborHtInfo(node, dot11, 
                        respFrame->hdr.sourceAddr, htCapabilityElement);
                }
                else
                {
                    apInfo->htEnableAP = FALSE;
                    apInfo->mcsIndex = 0;
                }
                if (dot11->isVHTEnable)
                {
                    VHT_CapabilitiesElement vhtCapabilityElement;
                    isPresent = Dot11_GetVHTCapabilities(
                                    node,
                                    dot11,
                                    rxFrame,
                                    &vhtCapabilityElement,
                                    sizeof(DOT11e_ProbeRespFrame));
                    ERROR_Assert(isPresent,
                               "VHT Enabled node must contain vht capability"
                               "element in management frame");
                    apInfo->vhtEnableAP = TRUE;
                    apInfo->vhtInfo.staVhtCapabilityElement
                        = vhtCapabilityElement;
                    VHT_OperationElement vhtOperationElement;
                    isPresent = Dot11_GetVHTOperation(
                                            node,
                                            dot11,
                                            rxFrame,
                                            &vhtOperationElement,
                                            sizeof(DOT11e_ProbeRespFrame));
                    ERROR_Assert(isPresent,
                                "VHT Enabled node must contain vht operation"
                                "element in management response frame");
                    apInfo->vhtInfo.vhtOperationElement
                        = vhtOperationElement;
                    DOT11_NeighborHtInfo* neighHtInfo = 
                        MacDot11nGetNeighHtInfo(dot11->htInfoList,
                                       respFrame->hdr.sourceAddr);
                    ERROR_Assert(neighHtInfo, "neighHtInfo should not be NULL");
                    neighHtInfo->maxMcsIdx =
                            (int)vhtCapabilityElement.m_mcsNssSet;
                    neighHtInfo->curMcsIdx =
                    std::min(neighHtInfo->maxMcsIdx,
                            Phy802_11GetNumAtnaElems(
                       node->phyData[dot11->myMacData->phyNumber])*10 - 1);
                    neighHtInfo->isOneSixtyMhzIntolerent
                     = vhtCapabilityElement.m_capabilitiesInfo.m_chWidth? 0:1;
                }
            }
        }
        //Update signal strength
        MacDot11StationUpdateAPMeasurement(node, dot11, apInfo, rxFrame);
    }
    mngmtVars->probeResponseRecieved = TRUE;

    return result;

}// MacDot11ManagementProcessProbeResponse


//--------------------------------------------------------------------------
/*!
 * \brief Process a reassociation request because of a AP change.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessReassociateRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    DOT11_FrameInfo* frameInfo = NULL;
    MacDot11Trace(node, dot11, rxFrame, "Receive");
    // DOT11e Now check if this is QSTA
    DOT11e_ReassocReqFrame* rxARqFrame =
        (DOT11e_ReassocReqFrame*)MESSAGE_ReturnPacket(rxFrame);

    BOOL isQSTA = FALSE;
    if (rxARqFrame->capInfo.QoS)
    {
        isQSTA = TRUE;
    }

    // Alloc Association response frame
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

//---------------------DOT11e--Updates------------------------------------//
    if (MacDot11IsQoSEnabled(node, dot11) && isQSTA)
    {

       // Get the request frame
       DOT11e_ReassocReqFrame * respFrame =
       (DOT11e_ReassocReqFrame *) MESSAGE_ReturnPacket(rxFrame);

        // Build response frame
        if (MacDot11eManagementBuildReassociateResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                respFrame,
                rxFrame) != DOT11_R_OK ) {

            ERROR_ReportWarning("MacDot11eManagementProcessAssociateRequest:"
                                "Build associate response failed. \n");
        }
//--------------------DOT11e-End-Updates---------------------------------//

    } else {

        // Get the request frame
        DOT11_ReassocReqFrame* respFrame =
            (DOT11_ReassocReqFrame*) MESSAGE_ReturnPacket(rxFrame);

        MESSAGE_PacketAlloc(
            node,
            msg,
            sizeof(DOT11_AssocRespFrame),
            TRACE_DOT11);

        // Build response frame
        if (MacDot11ManagementBuildReassociateResponseFrame(
                node,
                dot11,
                msg,
                &frameInfo,
                respFrame) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementProcessAssociateRequest:"
                                "Build associate response failed. \n");
        }
    }
    MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);
    result = DOT11_R_OK;
    return result;

}// MacDot11ManagementProcessReassociateRequest


//--------------------------------------------------------------------------
/*!
 * \brief Process a reassociation response because of a AP change.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessReassociateResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;

    DOT11_AssocRespFrame * respFrame =
        (DOT11_AssocRespFrame *) MESSAGE_ReturnPacket(rxFrame);

    if ((respFrame->statusCode == DOT11_SC_SUCCESSFUL) &&(respFrame->assocId > 0) )
    {

        MacDot11Trace(node, dot11, rxFrame, "Receive");

        // Set that station is authenticated
        dot11->stationAssociated = TRUE;
//---------------------------Power-Save-Mode-Updates---------------------//
// Receive the association ID value from AP and store this value at STAs
        dot11->assignAssociationId = respFrame->assocId;
//---------------------------Power-Save-Mode-End-Updates-----------------//
        // DOT11e Updates
        if (MacDot11IsQoSEnabled(node, dot11))
        {
            // Check for QAP
            DOT11e_AssocRespFrame* dot11erspFrame =
                (DOT11e_AssocRespFrame *) MESSAGE_ReturnPacket(rxFrame);

//--------------------HCCA-Updates Start---------------------------------//
            if (dot11erspFrame->capInfo.QoS)
            {
                dot11->associatedWithQAP = TRUE;
                if (dot11erspFrame->capInfo.CFPollRequest ||
                    dot11erspFrame->capInfo.CFPollable)
                {
                    dot11->associatedWithHC = TRUE;
                }
                else
                {
                    dot11->associatedWithHC = FALSE;
                }
            } else {
                dot11->associatedWithQAP = FALSE;
                dot11->associatedWithHC = FALSE;
            }
//--------------------HCCA-Updates End-----------------------------------//
        }
        MacDot11AssociationCompleted(node,dot11);

    } else {

        // TODO: Need some smarts here to handle a failed association

        MacDot11Trace(node, dot11, NULL,
            "WARNING reassociation failed.");

        char errString[MAX_STRING_LENGTH];
        char srcAddr[24],destAddr[24];
        MacDot11MacAddressToStr(srcAddr, &respFrame->hdr.sourceAddr);
        MacDot11MacAddressToStr(destAddr, &respFrame->hdr.destAddr);
        sprintf(errString,
                "MacDot11ManagementProcessReassociateResponse: "
                "STA %s failed to associate with AP %s with code %u ",
                destAddr,
                srcAddr,
                respFrame->statusCode);

        ERROR_ReportWarning(errString);
    }

    return result;

}// MacDot11ManagementProcessReassociateResponse


//--------------------------------------------------------------------------
/*!
 * \brief  Process a disassociation notification.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementProcessDisassociate(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementProcessDisassociate:"
                          " AP mode disassociation not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

    } else { // STATION
        // TODO: Not supported yet...not sure how to do it
        ERROR_ReportError("MacDot11ManagementProcessDisassociate:"
                          " Station mode disassociation not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;
    }

    return result;

}// MacDot11ManagementProcessDisassociate

//--------------------------HCCA-Updates------------------------------------
//--------------------------------------------------------------------------
/*!
 * \brief  Process if there are more ADDTS Request.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param tsid      unsigned int*    : pointer to unsigned int

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
BOOL MacDot11ManagementHaveMoreADDTSRequests(
    Node* node,
    MacDataDot11* dot11,
    unsigned int* tsid)
{
    for (int tsCounter = 0; tsCounter < DOT11e_HCCA_TOTAL_TSQUEUE_SUPPORTED; tsCounter++)
    {
        if (dot11->hcVars->staTSBufferItem[tsCounter]->size != 0 &&
            dot11->hcVars->staTSBufferItem[tsCounter]->tsStatus ==
                DOT11e_TS_None)
        {
            *tsid = dot11->hcVars->staTSBufferItem[tsCounter]->
                trafficSpec.info.tsid;
            return TRUE;
        }
    }
    return FALSE;
}
//------------------------------------------------------------------
// FUNCTION   :: Dot11_ParseCfsetIdElement
// LAYER      :: MAC
// PURPOSE    :: To parse CFSet ID element
// PARAMETERS ::
//   + node      : Node*         : pointer to node
//   + element   : DOT11_Ie*     : pointer to IE
//   + cfSet     :  DOT11_CFParameterSet*  : parsed mesh ID with
//                  null terminator
// RETURN     :: void
//-------------------------------------------------------------------------
static
void Dot11_ParseTSPECIdElement(
    Node* node,
    DOT11_Ie* element,
    TrafficSpec* TSPEC)
{
    int ieOffset = 0;
    int length = 0;
    DOT11_IeHdr ieHdr;
    Dot11_ReadFromElement(element, &ieOffset, &ieHdr,length);
    ERROR_Assert(
        ieHdr.id == DOT11_TSPEC_ID,
        "Dot11_ParseCfsetIdElement: "
        "IE ID does not match.\n");
    element->length = ieHdr.length + DOT11_IE_HDR_LENGTH;

    ERROR_Assert(
        ieHdr.length == sizeof(TrafficSpec),
        "Dot11_ParseCfsetIdElement: "
        "CFParameterSet length is not equal.\n");
    memset(TSPEC, 0, sizeof(TrafficSpec));
    memcpy(TSPEC, element->data + ieOffset, ieHdr.length);
    ieOffset += ieHdr.length;

    ERROR_Assert(ieOffset == element->length,
        "Dot11_ParseCfsetIdElement: all fields not parsed.\n") ;
}// Dot11_ParseCfsetIdElement

/// ---------------------------------------------------------
// FUNCTION   :: Dot11_GetTspecFromFrame
// LAYER      :: MAC
// PURPOSE    :: Receive and process a beacon frame if it has CfSet elements.
// PARAMETERS ::
//   + node      : Node*         : pointer to node
//   + dot11     : MacDataDot11* : pointer to Dot11 data structure
//   + msg       : Message*      : received beacon message
//   + cfSet     : DOT11_CFParameterSet* cfSet : wReturn the CfSet
// RETURN     ::BOOL
//-------------------------------------------------------------------

static
BOOL Dot11_GetTspecFromFrame(
    Node* node,
    MacDataDot11* dot11,
    Message* msg,
    TrafficSpec* TSPEC)
{
    BOOL isFound = FALSE;
    DOT11_FrameHdr* frameHdr = (DOT11_FrameHdr*) MESSAGE_ReturnPacket(msg);
    int offset = sizeof(DOT11e_ADDTS_Request_frame);
    int rxFrameSize = MESSAGE_ReturnPacketSize(msg);

    if (offset >= rxFrameSize)
    {
        return isFound;
    }

    DOT11_Ie element;
    DOT11_IeHdr ieHdr;
    unsigned char* rxFrame = (unsigned char *) frameHdr;

    ieHdr = rxFrame + offset;

    // The first expected IE is cfSet
    while (ieHdr.id != DOT11_TSPEC_ID)
    {
        offset += ieHdr.length + 2;
        ieHdr = rxFrame + offset;
        if (offset >= rxFrameSize)
        {
            return isFound;
        }
    }

    if (ieHdr.id == DOT11_TSPEC_ID)
    {
        Dot11s_AssignToElement(&element, rxFrame, &offset, rxFrameSize);
        Dot11_ParseTSPECIdElement(node, &element, TSPEC);

        isFound = TRUE;
    }
       return isFound;
}//Dot11_GetCfsetFromBeacon

//--------------------------------------------------------------------------
/*!
 * \brief Process an ADDTSRequest.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
int MacDot11ManagementHCProcessADDTSRequest(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;

    // DOT11e Now check if this is QSTA
    DOT11e_ADDTS_Request_frame* rxARqFrame =
        (DOT11e_ADDTS_Request_frame*)MESSAGE_ReturnPacket(rxFrame);

    DOT11e_ADDTS_Response_frame* resFrame;
    int ADDTSFrameSize = 0;
    DOT11_MacFrame ADDTS;
    DOT11_Ie* element = Dot11_Malloc(DOT11_Ie);
    //to do get TSPEC form req frame
    Dot11_GetTspecFromFrame(node,dot11,rxFrame,&dot11->TSPEC);

    ADDTSFrameSize = sizeof(DOT11e_ADDTS_Response_frame);
    MacDot11AddTrafficSpecToframe(node,dot11,element,
        &ADDTS, &ADDTSFrameSize,&dot11->TSPEC);

    Message* newFrame = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(
        node,
        newFrame,
        ADDTSFrameSize,
        TRACE_DOT11);

    // Allocate new Probe frame
    resFrame = (DOT11e_ADDTS_Response_frame*)MESSAGE_ReturnPacket(newFrame);
    memset(resFrame, 0, (size_t)ADDTSFrameSize);
    memcpy(resFrame, &ADDTS, (size_t)ADDTSFrameSize);
    MEM_free(element);

    resFrame->qosAction=DOT11e_ADDTS_Response;
    resFrame->TSDelay=0;
    resFrame->hdr.sourceAddr =rxARqFrame->hdr.destAddr;
    resFrame->hdr.destAddr = rxARqFrame->hdr.sourceAddr;
    resFrame->hdr.address3 = resFrame->hdr.sourceAddr;
    resFrame->category = rxARqFrame->category;
    resFrame->hdr.frameType = DOT11_ACTION;
    resFrame->statusCode = DOT11_SC_SUCCESSFUL;

     DOT11_FrameInfo* newFrameInfo =
    (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));

    newFrameInfo->msg = newFrame;
    newFrameInfo->frameType = DOT11_ACTION;
    newFrameInfo->RA = resFrame->hdr.destAddr;
    newFrameInfo->TA = dot11->selfAddr;
    newFrameInfo->DA = resFrame->hdr.destAddr;
    newFrameInfo->SA = INVALID_802ADDRESS;
    newFrameInfo->insertTime = node->getNodeTime();

    // Set state
    MacDot11StationSetState(
        node,
        dot11,
        DOT11_S_IDLE);
    if (DEBUG_HCCA)
    {
        printf("MacDot11ManagementHCProcessADDTSRequest:nodeID=%d,"
            " state=DOT11_S_IDLE\n",node->nodeId);
    }
    //Don't set the send frame. Response frame are
    //getting transmitted after SIFS time
    dot11->mgmtSendResponse = TRUE;

    MacDot11MgmtQueue_EnqueuePacket(node, dot11, newFrameInfo);

    result = DOT11_R_OK;

    return result;

}// MacDot11ManagementHCProcessADDTSRequest


//--------------------------------------------------------------------------
/*!
 * \brief Process an ADDTS Response.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
int MacDot11ManagementProcessADDTSResponse(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{
    int result = DOT11_R_FAILED;
    if (dot11->isHCCAEnable && !MacDot11IsHC(dot11)){

        DOT11e_ADDTS_Response_frame* respFrame =
            (DOT11e_ADDTS_Response_frame *) MESSAGE_ReturnPacket(rxFrame);

        if (respFrame->statusCode == DOT11_SC_SUCCESSFUL){

            // Set station state
            MacDot11StationSetState(
                node,
                dot11,
                DOT11_S_IDLE);
            if (DEBUG_HCCA)
            {
                printf("MacDot11ManagementProcessADDTSResponse: nodeID=%d, "
                    "state=DOT11_S_IDLE\n",node->nodeId);
            }

            // Set management state to join again
            MacDot11ManagementSetState(
                node,
                dot11,
                DOT11_S_M_JOINED);

            //transmit ACK
            MacDot11StationTransmitAck(node,dot11,respFrame->hdr.sourceAddr);
            //add to the list

            // Find if more ADD TS Requests required to be sent
            unsigned int tsid;
            if (MacDot11ManagementHaveMoreADDTSRequests(node,dot11,&tsid))
            {
                MacDot11ManagementAttemptToInitiateADDTS(node,dot11,tsid);
            }
            result = DOT11_R_OK;
        }
        else {

            // ADDTS Request failed. Not possible now

            MacDot11Trace(node, dot11, NULL,
                "ERROR ADDTS Failed failed.");

            char errString[MAX_STRING_LENGTH];
            char srcAddr[24],destAddr[24];
            MacDot11MacAddressToStr(srcAddr, &respFrame->hdr.sourceAddr);
            MacDot11MacAddressToStr(destAddr, &respFrame->hdr.destAddr);
            sprintf(errString,
                    "MacDot11ManagementProcessADDTSResponse: "
                    "STA %s failed to ADD TS at HC %s with code %u ",
                    destAddr,
                    srcAddr,
                    respFrame->statusCode);

            ERROR_ReportError(errString);
        }
    }
    else{
        /// Never supported on station or APs without HC capabilities
        ERROR_ReportError("MacDot11ManagementProcessADDTSResponse:"
            " HC and STAs which are not HCCA capable can't process"
            " ADDTS Response.\n");

        result = DOT11_R_NOT_SUPPORTED;
    }
    return result;
}//MacDot11ManagementProcessADDTSResponse


//-----------------------HCCA-UPDATES-END-----------------------------------

//--------------------------------------------------------------------------
/*!
 * \brief  Process a received management frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param rxFrame   DOT11_Frame*    : Received frame

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
void MacDot11ManagementProcessFrame(
    Node* node,
    MacDataDot11* dot11,
    DOT11_Frame* rxFrame)
{

    DOT11_FrameHdr* hdr =
        (DOT11_FrameHdr*) MESSAGE_ReturnPacket(rxFrame);

    // Stop retransmit timer
    if (hdr->destAddr != ANY_MAC802) {

        MacDot11StationCancelTimer(
        node,
        dot11);
    }

    if (MacDot11IsAp(dot11))
    {

        DOT11_ApStationListItem* stationItem =
        MacDot11ApStationListGetItemWithGivenAddress(
            node,
            dot11,
            hdr->sourceAddr);
        if (stationItem) {

            stationItem->data->LastFrameReceivedTime =
                                            node->getNodeTime();
        }
   }

    DOT11_ManagementVars * mngmtVars =
        (DOT11_ManagementVars*) dot11->mngmtVars;

    switch (hdr->frameType) {

        case DOT11_ASSOC_REQ:
        {
            dot11->mgmtSendResponse = TRUE;
            mngmtVars->assocReqReceived++;
            MacDot11ManagementProcessAssociateRequest(node, dot11, rxFrame);
            break;
        }
        case DOT11_ASSOC_RESP:
        {
            if (mngmtVars->state == DOT11_S_M_WFASSOC)
            {
                dot11->mgmtSendResponse = FALSE;
                mngmtVars->assocRespReceived++;
                MacDot11StationTransmitAck(node,dot11,hdr->sourceAddr);
                MacDot11ManagementProcessAssociateResponse(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_REASSOC_REQ:
        {
            dot11->mgmtSendResponse = TRUE;
            mngmtVars->reassocReqReceived++;
            MacDot11ManagementProcessReassociateRequest(node, dot11, rxFrame);
            break;
        }
        case DOT11_REASSOC_RESP:
        {
            if (mngmtVars->state == DOT11_S_M_WFREASSOC)
            {
                mngmtVars->reassocRespReceived++;
                MacDot11StationTransmitAck(node,dot11,hdr->sourceAddr);
                MacDot11ManagementProcessReassociateResponse(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_PROBE_REQ:
        {
            dot11->mgmtSendResponse = TRUE;
            if (MacDot11IsAp(dot11))
            {
                mngmtVars->ProbReqReceived++;
                MacDot11ManagementProcessProbeRequest(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_PROBE_RESP:
        {
            mngmtVars->ProbRespReceived++;
             MacDot11StationTransmitAck(node,dot11,hdr->sourceAddr);
            MacDot11ManagementProcessProbeResponse(node, dot11, rxFrame);
            break;
        }
        case DOT11_BEACON:
        {
            MacDot11ManagementProcessBeacon(node, dot11, rxFrame);
            break;
        }
//--------------------DOT11e-End-Updates---------------------------------//
//---------------------------Power-Save-Mode-Updates---------------------//
        case DOT11_ATIM:
        {
            if (!MacDot11IsATIMDurationActive(dot11)){
                break;
            }
            // Process ATIM frame here
            if (DEBUG_PS && DEBUG_PS_ATIM){
                MacDot11Trace(node, dot11, rxFrame, "Receive ATIM Frame");
            }
            dot11->atimFramesReceived++;
            MacDot11ManagementProcessATIMFrame(node, dot11, rxFrame);
            break;
        }
//---------------------------Power-Save-Mode-End-Updates-----------------//
        case DOT11_DISASSOCIATION:
        {
            ERROR_ReportError("MacDot11ManagementProcessFrame:"
                              " Disassociation not implemented.\n");
            break;
        }
        case DOT11_AUTHENTICATION:
        {
            if (((!MacDot11IsAp(dot11)&& mngmtVars->state == DOT11_S_M_WFAUTH))
                ||(MacDot11IsAp(dot11)))
            {
                if (MacDot11IsAp(dot11))
                {
                    mngmtVars->AuthReqReceived++;
                }
                else
                {
                    mngmtVars->AuthRespReceived++;
                }
                MacDot11ManagementProcessAuthenticate(node, dot11, rxFrame);
            }
            break;
        }
        case DOT11_DEAUTHENTICATION:
        {
            ERROR_ReportError("MacDot11ManagementProcessFrame:"
                              " Deauthentication not implemented.\n");
            break;
        }
        //---------------------------HCCA-Updates----------------------------
        case DOT11_ACTION:
        {
            // dot11s
            // This is parsed separately as the category field
            // of DOT11e_Action_frame is an int.
            // Note: This increments managementPacketsGot.
            unsigned char* frame = (unsigned char *) hdr;
            int offset = sizeof(DOT11_FrameHdr);
            unsigned char rxFrameCategory = *(frame + offset);
            if (rxFrameCategory == DOT11_ACTION_MESH)
            {
                break;
            }

            DOT11e_Action_frame* rxARqFrame =
                (DOT11e_Action_frame*)MESSAGE_ReturnPacket(rxFrame);
            if (rxARqFrame->qosAction == DOT11e_ADDTS_Request)
            {
                 dot11->mgmtSendResponse = TRUE;
                 MacDot11ManagementHCProcessADDTSRequest(node,
                    dot11, rxFrame);
            }
            else if (rxARqFrame->qosAction == DOT11e_ADDTS_Response &&
                mngmtVars->state == DOT11_S_M_WFADDTS_RSP)
            {
                MacDot11ManagementProcessADDTSResponse(node,
                    dot11, rxFrame);
            }
            else
            {
                MacDot11StationTransmitAck(node,dot11,hdr->sourceAddr);
            }
            break;
        }
//---------------------------HCCA-Updates-End------------------------
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

}// MacDot11ProcessManagemantFrame


//--------------------------------------------------------------------------
// Management Layer State Functions
//--------------------------------------------------------------------------

//---------------------DOT11e-HCCA-Updates--------------------------------//
//--------------------------------------------------------------------------
/*!
 * \brief  Attempt to initiate ADDTS Request.
 *
 * \param node      Node*                  : Pointer to node
 * \param dot11     MacDataDot11*          : Pointer to Dot11 structure
 * \param rxFrame   unsigned int tsid      : Traffic stream ID

 * \return          int                    : Error code
 */
//--------------------------------------------------------------------------

BOOL MacDot11ManagementAttemptToInitiateADDTS(
    Node* node,
    MacDataDot11* dot11,
    unsigned int tsid )
{
if (dot11->mgmtSendResponse == FALSE)
    {
        return (MacDot11ManagementADDTSRequest(node, dot11, tsid) ==
            DOT11_R_OK);
    }

    return FALSE;
}


//--------------------------------------------------------------------------
/*!
 * \brief  Action frame  to  an HC/AP.
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
    unsigned int tsid )
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) { // ACCESS POINT

        ERROR_ReportError("MacDot11ManagementADDTSRequest:"
                          " AP cannot initiate an ADDTSRequest Frame.\n");
        result = DOT11_R_NOT_SUPPORTED;


    } else { // STATION

        // Alloc ActionFrame
            DOT11_FrameInfo* frameInfo = NULL;

        // Build it
        if (MacDot11ManagementBuildADDTSRequestFrame(
            node,
            dot11,
            &frameInfo,
            tsid) != DOT11_R_OK ) {
                ERROR_ReportWarning("MacDot11ManagementADDTSRequest:"
                                    "Build ADDTSRequest Frame failed. \n");
        }

        // Set states

        MacDot11ManagementSetState(
            node,
            dot11,
            DOT11_S_M_WFADDTS_RSP);

        // Don't send. It will be sent from
        MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);

        result = DOT11_R_OK;
    }

    return result;

}// MacDot11ManagementADDTSRequest
//---------------------DOT11e-HCCA-End-Updates-----------------------------//
//--------------------------------------------------------------------------
/*!
 * \brief  Begin an authentication exchange.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementAuthenticate(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    // Alloc Authenticate frame
    Message* msg = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(
        node,
        msg,
        sizeof(DOT11_AuthFrame),
        TRACE_DOT11);

    DOT11_FrameInfo* frameInfo = NULL;


    // Build it
    if (MacDot11ManagementBuildAuthenticateFrame(
            node,
            dot11,
            msg,
            &frameInfo,
            NULL) != DOT11_R_OK )
        ERROR_ReportWarning("MacDot11ManagementAuthenticate:"
                            "Build authenticate frame failed. \n");


    // Set states
    MacDot11ManagementSetState (
        node,
        dot11,
        DOT11_S_M_WFAUTH);

    if (!MacDot11IsAp(dot11))
        {
        dot11->mgmtSendResponse = FALSE;
        }
    MacDot11MgmtQueue_EnqueuePacket(node, dot11, frameInfo);

    //Set backoff and attempt to transmit/
    result = DOT11_R_OK;

    return result;
} // MacDot11ManagementAuthenticate


//--------------------------------------------------------------------------
/*!
 * \brief  Send a deauthenticate notification.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementDeauthenticate(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementDeauthenticate:"
                          " AP mode deauthenticate not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;

    } else { // STATION
        ERROR_ReportError("MacDot11ManagementDeauthenticate:"
                          " Station mode deauthenticate not implimented.\n");;
        result = DOT11_R_NOT_IMPLEMENTED;

    }

    return result;

}// MacDot11ManagementDeauthenticate


//--------------------------------------------------------------------------
/*!
 * \brief  Associate with an AP.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementAssociateRequest(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    // Alloc Association Request frame
    Message* msg = NULL;

    DOT11_FrameInfo* newFrameInfo = NULL;
    // Build it
    if (MacDot11ManagementBuildAssociateRequestFrame(
        node,
        dot11,
        &msg,
        &newFrameInfo) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementAssociateRequest:"
                                "Build associate request failed. \n");
    }

    // Set states
    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_WFASSOC);

    if (newFrameInfo->frameType == DOT11_ASSOC_REQ)
    {
        dot11->mgmtSendResponse = FALSE;
    }

    MacDot11MgmtQueue_EnqueuePacket(node,dot11,newFrameInfo);

    //Set backoff and attempt to transmit
    result = DOT11_R_OK;

    return result;

}// MacDot11ManagementAssociateRequest


//--------------------------------------------------------------------------
/*!
 * \brief  Respond to an associate request.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementAssociateResponse(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        result = DOT11_R_NOT_IMPLEMENTED;

    } else {  // STATION
        ERROR_ReportError("MacDot11ManagementAssociateResponse:"
                          " Station mode cannot perform associate response.\n");
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementAssociateResponse

//--------------------------------------------------------------------------
/*!
 * \brief  Renew association because of an AP change.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementReassociateRequest(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    // dot11s. Reassociate in case of MAPs.
    // Alloc Association Request frame
    Message* msg = NULL;

    DOT11_FrameInfo* newFrameInfo = NULL;
    // Build it
    if (MacDot11ManagementBuildReassociateRequestFrame(
        node,
        dot11,
        &msg,
        &newFrameInfo) != DOT11_R_OK ) {
            ERROR_ReportWarning("MacDot11ManagementAssociateRequest:"
                                "Build reassociate request failed. \n");
    }

    // Set states
    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_WFREASSOC);

    MacDot11MgmtQueue_EnqueuePacket(node,dot11,newFrameInfo);

    result = DOT11_R_OK;

    return result;

}// MacDot11ManagementReassociate

//--------------------------------------------------------------------------
/*!
 * \brief  Send a disassociation notification.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementDisassociate(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT

        ERROR_ReportError("MacDot11ManagementDisassociate:"
                          " AP mode disassociates not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;
    } else {  // STATION

        // TODO: Not supported yet...not sure how to do it
        ERROR_ReportError("MacDot11ManagementDisassociate:"
                          " Station mode disassociates not implimented.\n");
        result = DOT11_R_NOT_IMPLEMENTED;
    }

    return result;

}// MacDot11ManagementDisassociate

DOT11_ApInfo* MacDot11ManagementFindBestAp(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_ApInfo* bestAp = NULL;
    DOT11_ApInfo* apInfo;
    DOT11_ManagementVars * mgmtVars =
                (DOT11_ManagementVars*) dot11->mngmtVars;

    apInfo = mgmtVars->channelInfo->headAPList;

    while (apInfo != NULL)
    {
        if ((apInfo->lastMeaTime >= mgmtVars->channelInfo->scanStartTime)
            && (apInfo->rssMean > dot11->thresholdSignalStrength))
        {
            if (bestAp == NULL)
            {
                bestAp = apInfo;
            }
            else if (apInfo->rssMean > bestAp->rssMean)
            {
                bestAp = apInfo;
            }
        }
        apInfo = apInfo->next;
    }

    return bestAp;
}
//--------------------Activity completed----------------------------------
//--------------------------------------------------------------------------
/*!
 * \brief  Signal the end of scan
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
void MacDot11ManagementScanCompleted(
    Node* node,
    MacDataDot11* dot11,
    int result)
{
    int scanStatus = result;
    DOT11_ApInfo* bestAp = MacDot11ManagementFindBestAp(node, dot11);
    dot11->ScanStarted = FALSE;

    if (bestAp == NULL)
    {
        scanStatus = DOT11_R_FAILED;
        dot11->stationAssociated = FALSE;
        dot11->associatedWithHC = FALSE;
         MacDot11ManagementStartTimerOfGivenType(
                                 node,
                                 dot11,
                                 (3*dot11->beaconInterval),
                                 MSG_MAC_DOT11_Scan_Start_Timer);
    }

    DOT11_ManagementVars * mgmtVars =
                (DOT11_ManagementVars*) dot11->mngmtVars;

    if ((scanStatus == DOT11_R_OK && !dot11->stationAssociated))
    {
        //Process scan results and select best AP.

        mgmtVars->PraposedAPInfo = bestAp;
        mgmtVars->apAddr = mgmtVars->PraposedAPInfo->bssAddr;

        MacDot11ManagementChangeToChannel(
                node,
                dot11,
                bestAp->channelId);

        //start management association start timer
        MacDot11ManagementStartTimerOfGivenType(
            node,
            dot11,
            (clocktype) (RANDOM_nrand(dot11->seed) %
            MacDot11TUsToClocktype(DOT11_MANAGEMENT_DELAY_TIME)),
            MSG_MAC_DOT11_Authentication_Start_Timer);

    }
    else if (scanStatus == DOT11_R_OK && dot11->stationAssociated)
    {
        if ((bestAp->bssAddr != dot11->associatedAP->bssAddr) &&
            ((bestAp->rssMean - dot11->associatedAP->rssMean) >=
              DOT11_HANDOVER_RSS_MARGIN))
        {
            dot11->stationAssociated = FALSE;
            dot11->associatedWithHC = FALSE;
            mgmtVars->PraposedAPInfo = bestAp;
            //TODO: send disassociation frame frame
            mgmtVars->prevApAddr = dot11->associatedAP->bssAddr;

            mgmtVars->apAddr = mgmtVars->PraposedAPInfo->bssAddr;

            MacDot11ManagementChangeToChannel(
                node,
                dot11,
                bestAp->channelId);

            //start management reassociation start timer
            MacDot11ManagementStartTimerOfGivenType(
                node,
                dot11,
                (clocktype) (RANDOM_nrand(dot11->seed) %
                MacDot11TUsToClocktype(DOT11_MANAGEMENT_DELAY_TIME)),
                MSG_MAC_DOT11_Reassociation_Start_Timer);
        }
        else
        {
            // Re-association not required. Changed the channel back
            MacDot11ManagementChangeToChannel(
                node,
                dot11,
                (unsigned)dot11->associatedAP->channelId);
            dot11->bssAddr = dot11->associatedAP->bssAddr;
        }
    }
} // MacDot11ManagementScanCompleted

//--------------------------------------------------------------------------
/*!
 * \brief  Signal the end of Association
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
void MacDot11AssociationCompleted(
    Node* node,
    MacDataDot11* dot11)
{
    // Set state to associate
    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_ASSOC_FINISHED);

     //cancel Authenticationtimer
     MacDot11StationCancelTimer(node, dot11);

    if (MacDot11ManagementJoinCompleted(node, dot11) != DOT11_R_OK){
                ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                  " Complete join failed.\n");
    }

} // MacDot11AssociationCompleted
//--------------------------------------------------------------------------
/*!
 * \brief  Signal the end of Authentication
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
void MacDot11AuthenticationCompleted(
    Node* node,
    MacDataDot11* dot11)
{
    dot11->stationAuthenticated = TRUE;
            // Set management state to associate next

     MacDot11ManagementSetState(
                node,
                dot11,
                DOT11_S_M_AUTH_FINISHED);

     //cancel Authenticationtimer
     MacDot11StationCancelTimer(node, dot11);

    if (dot11->stationAssociated)
    {
        //donot reassociate
    }
    else
    {
        if (MacDot11ManagementAssociateRequest(node, dot11) != DOT11_R_OK){
                    ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                      " Authenticate finished failed.\n");
        }
    }

} // MacDot11AuthenticationCompleted
//--------------------------------------------------------------------------
/*!
 * \brief  Start scanning for BSS
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementScanStart(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementScan:"
                          " AP mode cannot perform joins.\n");
        result = DOT11_R_NOT_SUPPORTED;

    } else {  // STATION

        // Scanning is disable, use pre-assigned channel and set state
        // to start authentication
        if (mngmtVars->scanType == DOT11_SCAN_DISABLED ) {

            mngmtVars->channelInfo->numChannels = 1;
            mngmtVars->channelInfo->numResults = 1;
            mngmtVars->channelInfo->scanStartTime =
                                       node->getNodeTime();

            // Set first channel
            MacDot11ManagementChangeToChannel(
                node,
                dot11,
                mngmtVars->currentChannel);

            // Set state that we're finished scanning
            MacDot11ManagementSetState(
                node,
                dot11,
                DOT11_S_M_WFSCAN);

            // Start timer
            //Randomize the wait
            MacDot11ManagementStartTimerOfGivenType(
                node,
                dot11,
                (clocktype) (RANDOM_nrand(dot11->seed) %
                 MacDot11TUsToClocktype(DOT11_MANAGEMENT_DELAY_TIME)),
                 MSG_MAC_DOT11_Beacon_Wait_Timer);


        } else if (mngmtVars->scanType ==DOT11_SCAN_PASSIVE){
            dot11->bssAddr = dot11->selfAddr;
            // Figure out our timeout
            mngmtVars->channelInfo->dwellTime =
                dot11->beaconInterval * 2;

            // Set first channel
            MacDot11ManagementChangeToChannel(
                node,
                dot11,
                mngmtVars->currentChannel);

            //To avoid scanning first channel twice.
            mngmtVars->channelInfo->numResults = 1;

            // Set state that we're finished scanning
            MacDot11ManagementSetState(
                node,
                dot11,
                DOT11_S_M_WFSCAN);

            mngmtVars->channelInfo->scanStartTime = node->getNodeTime();

            MacDot11ManagementStartTimerOfGivenType(
                node,
                dot11,
                              mngmtVars->channelInfo->dwellTime,
                              MSG_MAC_DOT11_Beacon_Wait_Timer);

        }
        else if (mngmtVars->scanType == DOT11_SCAN_ACTIVE)
        {
            
            
            dot11->bssAddr = dot11->selfAddr;

             // Set first channel
            MacDot11ManagementChangeToChannel(
                node,
                dot11,
                mngmtVars->currentChannel);

            //To avoid scanning first channel twice.
            mngmtVars->channelInfo->numResults = 1;

            MacDot11ManagementSetState(
                            node,
                            dot11,
                            DOT11_S_M_WFSCAN);
            mngmtVars->channelInfo->scanStartTime = node->getNodeTime();

            MacDot11TransmitProbeRequestFrame(node,dot11);


        }
        result = DOT11_R_OK;
    }

    return result;

} // MacDot11ManagementScan


//--------------------------------------------------------------------------
/*!
 * \brief  Scan next channel.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementScanNextChannel(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    if (mngmtVars->channelInfo->numChannels <=
                mngmtVars->channelInfo->numResults )
    {
        if (mngmtVars->channelInfo->headAPList == NULL)
        {
            result = DOT11_R_FAILED;
        }
        else
        {
            result = DOT11_R_OK;
        }
        MacDot11ManagementScanCompleted(node, dot11, result);

        mngmtVars->channelInfo->numResults = 1;

    }
    else
    {
        // update channel
        unsigned int newChannel = mngmtVars->currentChannel;

        // skip non-listenable channels
        unsigned int numSkippedChannels = 0;

        while (1)
        {
            // next channel
            newChannel = (newChannel + 1) %
                         mngmtVars->channelInfo->numChannels;

            if (!PHY_CanListenToChannel(
                   node,
                   dot11->myMacData->phyNumber,
                   newChannel))
            {
                numSkippedChannels ++;
                mngmtVars->channelInfo->numResults++;

                if (numSkippedChannels >=
                        mngmtVars->channelInfo->numChannels)
                {
                    ERROR_ReportError(
                        "An interface should be listenable on "
                        "at least one channel. Check your"
                        "PHY-LISTENABLE-CHANNEL-MASK.\n");
                }

                if (mngmtVars->channelInfo->numChannels <=
                        mngmtVars->channelInfo->numResults ) {

                    if (mngmtVars->channelInfo->headAPList == NULL)
                    {
                        result = DOT11_R_FAILED;
                    }
                    else
                    {
                        result = DOT11_R_OK;
                    }
                    MacDot11ManagementScanCompleted(node, dot11, result);

                    mngmtVars->channelInfo->numResults = 1;
                    break;
                }
            }
            else
            {
                MacDot11ManagementChangeToChannel(
                    node,
                    dot11,
                    newChannel);

                // Reset the NAV
                dot11->NAV = 0;
                mc(dot11)->updateStats("NavEnd", dot11->NAV);

                if (mngmtVars->scanType == DOT11_SCAN_ACTIVE)
                {
                    mngmtVars->probeResponseRecieved = FALSE;
                    MacDot11TransmitProbeRequestFrame(node,dot11);
                    dot11->ActiveScanShortTimerFunctional = FALSE;
                    dot11->MayReceiveProbeResponce = FALSE;
                }
                else{
                    // Wait for dwell time to hear a beacon
                    MacDot11ManagementStartTimerOfGivenType(
                                    node,
                                    dot11,
                                    mngmtVars->channelInfo->dwellTime,
                                    MSG_MAC_DOT11_Beacon_Wait_Timer);
                }
                mngmtVars->channelInfo->numResults ++;
                break;
            }
        }
    }
    result = DOT11_R_OK;

    return result;

} // MacDot11ManagementScanNextChannel


//--------------------------------------------------------------------------
/*!
 * \brief  Process the results of a scan. Start Authentication
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
static
int MacDot11ManagementScanProcessResults(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        ERROR_ReportError("MacDot11ManagementScanResults:"
                          " AP mode cannot perform scan results.\n");
        result = DOT11_R_NOT_SUPPORTED;

    } else {  // STATION

        // TODO: Process scan results and select best AP.
        result = DOT11_R_NOT_IMPLEMENTED;
    }

    return result;
} // MacDot11ManagementScanProcessResults


//--------------------------------------------------------------------------
/*!
 * \brief  Scan failed to find an AP.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementScanFailed(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    if (DEBUG_PS && DEBUG_MANAGEMENT){
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, "MacDot11ManagementScanFailed:"
                    " Staion %d failed to find an AP.\n",
                    node->nodeId);
        ERROR_ReportWarning(buf);
    }
    result = DOT11_R_OK;

    return result;
} // MacDot11ManagementScanFailed


//--------------------------------------------------------------------------
/*!
 * \brief  Join has completed. Assign AP
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementJoinCompleted(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    // Check if joined
    if (!MacDot11IsStationJoined(dot11)) {
        printf("Node: %d Authenticate: %d Associate: %d\n",
               node->nodeId,
               dot11->stationAuthenticated,
               dot11->stationAssociated);

        ERROR_ReportError("MacDot11ManagementJoinCompleted:"
                          " Station already joined.\n");
    }

    //Intialize the Associated AP info
    dot11->associatedAP = mngmtVars->PraposedAPInfo;
    dot11->associatedAP->cfSet = mngmtVars->PraposedAPInfo->cfSet;
    dot11->lastCfpBeaconTime =
          mngmtVars->PraposedAPInfo->lastCfpBeaconTime;
    // Set the BSS AP address for this station
    dot11->bssAddr = dot11->associatedAP->bssAddr;
    dot11->beaconInterval = MacDot11TUsToClocktype
        ((unsigned short) dot11->associatedAP->beaconInterval);
    dot11->cfpMaxDuration = MacDot11TUsToClocktype
        (dot11->associatedAP->cfSet.cfpMaxDuration);

    // Modifications
	if (DEBUG_MANAGEMENT /*true*/) {
		char timeStr[100];
		char macAdder[24];

		MacDot11MacAddressToStr(macAdder,&dot11->bssAddr);
		ctoa(node->getNodeTime(), timeStr);
		printf("MacDot11ManagementJoinCompleted: "
			"%s joined %s with AP %s at time %s\n",
				node->hostname /*node->nodeId*/,
				dot11->stationMIB->dot11DesiredSSID,
				macAdder,
				timeStr);
	}

	Message* msg;
	ActionData acnData;
	int infoSize = sizeof(dot11->bssAddr);
	int packetSize = sizeof(MacDataDot11);

	msg = MESSAGE_Alloc(node,
			APP_LAYER,
			APP_UP_CLIENT_DAEMON /*APP_UP_CLIENT*/,
			MSG_APP_UP_FromMacJoinCompleted);
	MESSAGE_InfoAlloc(node, msg, infoSize);
	memcpy(MESSAGE_ReturnInfo(msg), &dot11->bssAddr, infoSize);
	MESSAGE_PacketAlloc(node, msg, packetSize, TRACE_UP);
	memcpy(MESSAGE_ReturnPacket(msg), dot11, packetSize);

	//Trace Information
	acnData.actionType = SEND;
	acnData.actionComment = NO_COMMENT;
	TRACE_PrintTrace(node, msg, TRACE_MAC_LAYER,
			PACKET_OUT, &acnData);
	MESSAGE_Send(node, msg, (clocktype)0);

    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_JOINED);

    result = DOT11_R_OK;

    if (dot11->isHTEnable)
    {
        // Reset sequence numbers
        MacDot11nResetOutputBuffer(node, dot11, TRUE, FALSE);
        MacDot11nResetAmsduBuffer(node, dot11, TRUE, FALSE);
        MacDot11nResetCurrentMessageVariables(dot11);
    }
//    printf("Magic line at %s\n", node->hostname);

    return result;
} // MacDot11ManagementJoinCompleted


//--------------------------------------------------------------------------
/*!
 * \brief  Join a BSS that was previously obtained by a scan.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementJoin(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    // Check bsstype (needs to be infrastructure)
    if (!MacDot11IsBssStation(dot11))
        ERROR_ReportError("MacDot11ManagementJoin:"
                      " Cannot perform infrastructure joins.\n");

    // Check the ssid,
    if (strlen(MacDot11MibGetSSID(dot11)) != 0){
         //if present use it.
    } else {
        // if not, construct a bogus SSID and assign it to OwnSSID and
        // DesiredSSID
    }

    result = DOT11_R_OK;
    return result;
} // MacDot11ManagementJoin


//--------------------------------------------------------------------------
/*!
 * \brief  Auto associated with an AP. Skips the whole join process.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementAutoJoin(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;
    dot11->stationAuthenticated = TRUE;
    dot11->stationAssociated = TRUE;

//---------------------DOT11e--Updates------------------------------------//
    dot11->associatedWithQAP = FALSE;
//--------------------DOT11e-End-Updates---------------------------------//
    // Set states
    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_JOINED);

    MacDot11StationSetState(
        node,
        dot11,
        DOT11_S_WFBEACON);

    result = DOT11_R_OK;

    return result;


}// MacDot11ManagementAutoJoin


//--------------------------------------------------------------------------
/*!
 * \brief  Process states after receiving a timeout and pass to handler.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          NONE.
 */
//--------------------------------------------------------------------------
void MacDot11ManagementHandleTimeout(
    Node* node,
    MacDataDot11* dot11,
    Message* msg)
{
    ERROR_Assert((!MacDot11IsAp(dot11) ||
        msg->eventType == MSG_MAC_DOT11_Enable_Management_Timer),
        "MacDot11HandleTimeout: "
        "Wait For Join state for a non station.\n");
    switch(msg->eventType)
    {
        case MSG_MAC_DOT11_Active_Scan_Short_Timer:
        {
            unsigned timerSequenceNumber = (unsigned)*(int*)(MESSAGE_ReturnInfo(msg));
            if (timerSequenceNumber == dot11->managementSequenceNumber)
            {
                if (dot11->MayReceiveProbeResponce == TRUE)
                {
                    DOT11_ManagementVars* mngmtVars =
                        (DOT11_ManagementVars *) dot11->mngmtVars;
                    mngmtVars->channelInfo->dwellTime =
                                         mngmtVars->longScanTimer;
                    MacDot11ManagementStartTimerOfGivenType(
                        node,
                        dot11,
                        mngmtVars->channelInfo->dwellTime,
                        MSG_MAC_DOT11_Active_Scan_Long_Timer);

                }
                else
                {
                    if (MacDot11ManagementScanNextChannel(node, dot11) !=
                        DOT11_R_OK)
                    {
                        ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                " Scan next channel failed.\n");
                    }
                }
            }

            break;
        }

        case MSG_MAC_DOT11_Active_Scan_Long_Timer:
        case MSG_MAC_DOT11_Beacon_Wait_Timer:
        {
            unsigned timerSequenceNumber = (unsigned)*(int*)(MESSAGE_ReturnInfo(msg));
            if (timerSequenceNumber == dot11->managementSequenceNumber)
            {
                if (MacDot11ManagementScanNextChannel(node, dot11) !=
                    DOT11_R_OK){
                ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                " Scan next channel failed.\n");
                }
            }
            break;
           }

         case MSG_MAC_DOT11_Management_Authentication:
         {
            if (dot11->stationAuthenticated != TRUE)
                     MacDot11ManagementReset(node, dot11);
            break;
        }
         case MSG_MAC_DOT11_Management_Association:
         case MSG_MAC_DOT11_Management_Reassociation:
         {
            if (dot11->stationAssociated != TRUE)
                     MacDot11ManagementReset(node, dot11);
            break;
         }
         case MSG_MAC_DOT11_Scan_Start_Timer:
         {
                    // Reset station Management
                    MacDot11ManagementReset(node, dot11);
            break;
         }

         case MSG_MAC_DOT11_Authentication_Start_Timer:
         {
            dot11->ScanStarted = FALSE;
            if (MacDot11ManagementAuthenticate(node, dot11)
                != DOT11_R_OK)
            {
                ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                      " Scan finished failed.\n");
            }

            break;
        }
        case MSG_MAC_DOT11_Reassociation_Start_Timer:
        {
            if (MacDot11ManagementReassociateRequest(node,dot11) !=
            DOT11_R_OK)
            {
                ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                  " Scan finished failed.\n");
            }

            break;
        }
        case MSG_MAC_DOT11_Enable_Management_Timer:
        {
            MacDot11ManagementEnable(
                    node,
                    dot11);
            break;
        }
        default:
        {
            ERROR_ReportError("MacDot11Layer: "
                "Unknown message event type.\n");
            break;

        }
    }

} //MacDot11ManagementHandleTimeout


//--------------------------------------------------------------------------
/*!
 * \brief  Initaize MIB parameters.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementInitMib(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    dot11->stationMIB->dot11StationID = dot11->selfAddr;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        result = DOT11_R_OK;

    } else {  // STATION
        // Not worried about IBSS for now, assume infrastructure
        if (dot11->stationType != DOT11_STA_IBSS) {

            ERROR_Assert(dot11->stationType == DOT11_STA_IBSS,
                         "MacDot11ManagementEnable:"
                         " Station type is not set.");

            if (MacDot11IsAssociationDynamic(dot11)){
                // Start a join
                MacDot11ManagementJoin(
                    node,
                    dot11);
            } else {
                // Auto join
                MacDot11ManagementAutoJoin(
                    node,
                    dot11);
            }
        }

        result = DOT11_R_OK;
    }

    return result;

}// MacDot11ManagementInitMib


//--------------------------------------------------------------------------
/*!
 * \brief  Start a join.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          None
 */
//--------------------------------------------------------------------------
void MacDot11ManagementStartJoin(
    Node* node,
    MacDataDot11* dot11)
{

    // Start scanning first
    // Set state
    MacDot11ManagementSetState(
        node,
        dot11,
        DOT11_S_M_SCAN);

    dot11->ScanStarted = TRUE;

    //No need to wait for 1.5 * beaconInterval as
    //as scanning precess is not started yet.
    //Wait randomly between 0-2 TU
    if (MacDot11ManagementScanStart(node, dot11) != DOT11_R_OK){
                ERROR_ReportError("MacDot11ManagementAttemptingToJoin:"
                                " Start scan failed.\n");
    }
}// MacDot11ManagementStartJoin


//--------------------------------------------------------------------------
/*!
 * \brief  Gather channel info.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementChannelInfo(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) { // ACCESS POINT

        // setting default value for channellist = all channels
        // setting default value for channeldwelltime = 100 ms
        // Channel info is currently unsupported for AP
        result = DOT11_R_NOT_IMPLEMENTED;

    } else { // STATION
        // Not supported in STA
        result = DOT11_R_NOT_SUPPORTED;
    }

    return result;

}// MacDot11ManagementChannelInfo


//--------------------------------------------------------------------------
/*!
 * \brief  Returns channel usage info results.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementChannelInfoResults(
    Node* node,
    MacDataDot11* dot11)
{
    int result = DOT11_R_FAILED;

    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        // Channel info is currently unsupported for AP
        result = DOT11_R_NOT_IMPLEMENTED;

    } else {  // STATION
        // Not supported in a STATION
        result = DOT11_R_NOT_SUPPORTED;

    }

    return result;

}// MacDot11ManagementChannelInfoResults


//--------------------------------------------------------------------------
/*!
 * \brief  Set variables and states after the transmition of a management frame
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------

void MacDot11ManagementFrameTransmitted(
    Node* node,
    MacDataDot11* dot11)
{
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    mngmtVars->managementPacketsSent++;
//---------------------------Power-Save-Mode-Updates---------------------//
    if (MacDot11IsATIMDurationActive(dot11)){

        dot11->atimFramesSent++;

            if (dot11->lastTxATIMDestAddress == ANY_MAC802){

            if (DEBUG_PS && DEBUG_PS_ATIM)
                MacDot11Trace(node, dot11, NULL, "Broadcast ATIM Frame Send");

            dot11->transmitBroadcastATIMFrame = TRUE;
            MacDot11StationSetState(
                node,
                dot11,
                DOT11_S_IDLE);
        }
        else{
            if (DEBUG_PS && DEBUG_PS_ATIM)
                MacDot11Trace(node, dot11, NULL, "Unicast ATIM Frame Send");
            return;
        }
    }
}// MacDot11ManagementFrameTransmitted


//--------------------------------------------------------------------------
/*!
 * \brief  Attempt to send a management frame.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          BOOL            : TRUE if states permit or transmit occurs
 */
//--------------------------------------------------------------------------
BOOL MacDot11ManagementAttemptToTransmitFrame(
    Node* node,
    MacDataDot11* dot11)
{

    if (MacDot11StationWaitForNAV(node, dot11)) {

        clocktype currentTime = node->getNodeTime();

        char comment[MAX_STRING_LENGTH];
        sprintf(comment,
            "Node %d call to transmit beacon when NAV=%f and time=%f state %d",
            node->nodeId,
            dot11->NAV/(float)SECOND, currentTime/(float)SECOND,
            dot11->state);

        if (DEBUG_MANAGEMENT)
        {
            printf("%s\n", comment);
        }

        MacDot11Trace(
            node,
            dot11,
            NULL,
            comment);

        MacDot11StationSetState(
            node,
            dot11,
            DOT11_S_WFNAV);

        MacDot11StationStartTimer(
            node,
            dot11,
            dot11->NAV - currentTime);

        if (DEBUG_MANAGEMENT)
            return FALSE;
        else
            return TRUE;
    }
    return TRUE;

}// MacDot11ManagementAttemptToTransmitFrame
 //--------------------------------------------------------------------------
/*!
 * \brief  Print management statistics for packets dropped.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param interfaceIndex  int       :Interface index

 * \return          BOOL            : TRUE if states permit or transmit occurs
 */
//--------------------------------------------------------------------------

 void MacDot11ManagementFramesSendPrintStats(Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
 {
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

     char buf[MAX_STRING_LENGTH];
         if (dot11->isHTEnable && dot11->stationType == DOT11_STA_IBSS)
     {
             sprintf(buf, "Management probe request generated = %d",
                   mngmtVars->ProbReqGnrated);
            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
         }

        if ((mngmtVars->scanType == DOT11_SCAN_ACTIVE
                && dot11->stationType != DOT11_STA_AP)
            || (dot11->stationType == DOT11_STA_IBSS
                && dot11->isHTEnable))
         {
             sprintf(buf, "Management probe request send = %d",
                   mngmtVars->ProbReqSend);

             IO_PrintStat(
                 node,
                 "MAC",
                 DOT11_MANAGEMENT_STATS_LABEL,
                 ANY_DEST,
                 interfaceIndex,
                 buf);

         }
         
         if (dot11->stationType == DOT11_STA_IBSS && dot11->isHTEnable)
         {
             sprintf(buf, "Management probe request dropped = %d",
                    mngmtVars->ProbReqDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

         }
         
         if (mngmtVars->scanType == DOT11_SCAN_ACTIVE ||MacDot11IsAp(dot11)
             || (dot11->stationType == DOT11_STA_IBSS
                   && dot11->isHTEnable))
         {

            sprintf(buf, "Management probe request received = %d",
                   mngmtVars->ProbReqReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
         }

        if (mngmtVars->scanType == DOT11_SCAN_ACTIVE ||MacDot11IsAp(dot11)
            || (dot11->stationType == DOT11_STA_IBSS
                  && dot11->isHTEnable))
         {
            sprintf(buf, "Management probe response send = %d",
                   mngmtVars->ProbRespSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        
        if ((mngmtVars->scanType == DOT11_SCAN_ACTIVE
               && dot11->stationType != DOT11_STA_AP)
           || (dot11->stationType == DOT11_STA_IBSS
               && dot11->isHTEnable))
        {

            sprintf(buf, "Management probe response received = %d",
                   mngmtVars->ProbRespReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (mngmtVars->scanType == DOT11_SCAN_ACTIVE ||MacDot11IsAp(dot11)
            || (dot11->stationType == DOT11_STA_IBSS
                 && dot11->isHTEnable))
         {
            sprintf(buf, "Management probe response dropped = %d",
                   mngmtVars->ProbRespDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management authentication request send = %d",
                   mngmtVars->AuthReqSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (MacDot11IsAp(dot11))
        {

            sprintf(buf, "Management authentication request received = %d",
                   mngmtVars->AuthReqReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management authentication request dropped = %d",
                   mngmtVars->AuthReqDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (MacDot11IsAp(dot11))
        {

            sprintf(buf, "Management authentication response send = %d",
                   mngmtVars->AuthRespSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {


            sprintf(buf, "Management authentication response received = %d",
                   mngmtVars->AuthRespReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (MacDot11IsAp(dot11))
        {


            sprintf(buf, "Management authentication response dropped = %d",
                   mngmtVars->AuthRespDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management association requests send = %d",
                   mngmtVars->assocReqSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (MacDot11IsAp(dot11))
        {


             sprintf(buf, "Management association requests received = %d",
                   mngmtVars->assocReqReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {


            sprintf(buf, "Management association requests dropped = %d",
                   mngmtVars->assocReqDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (MacDot11IsAp(dot11))
        {


            sprintf(buf, "Management association response send = %d",
                   mngmtVars->assocRespSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);

        }
        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management association response received = %d",
                   mngmtVars->assocRespReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (MacDot11IsAp(dot11))
        {


            sprintf(buf, "Management association response dropped = %d",
                   mngmtVars->assocRespDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {


            sprintf(buf, "Management reassociation requests send = %d",
                   mngmtVars->reassocReqSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (MacDot11IsAp(dot11))
        {

             sprintf(buf, "Management reassociation requests received = %d",
                   mngmtVars->reassocReqReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }
        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {


            sprintf(buf, "Management reassociation requests dropped = %d",
                   mngmtVars->reassocReqDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (MacDot11IsAp(dot11))
        {

            sprintf(buf, "Management reassociation response send = %d",
                   mngmtVars->reassocRespSend);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management reassociation response received = %d",
                   mngmtVars->reassocRespReceived);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (!MacDot11IsAp(dot11) && dot11->stationType != DOT11_STA_IBSS)
        {

            sprintf(buf, "Management reassociation response dropped = %d",
                   mngmtVars->reassocRespDropped);

            IO_PrintStat(
                node,
                "MAC",
                DOT11_MANAGEMENT_STATS_LABEL,
                ANY_DEST,
                interfaceIndex,
                buf);
        }

        if (dot11->isHTEnable && !dot11->isVHTEnable)
        {
            sprintf(buf, "Management addba request sent = %d",
                       mngmtVars->ADDBSRequestSent);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management addba request received = %d",
                       mngmtVars->ADDBSRequestReceived);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management addba request dropped = %d",
                       mngmtVars->ADDBSRequestDropped);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                sprintf(buf, "Management addba response sent = %d",
                       mngmtVars->ADDBSResponseSent);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management addba response received = %d",
                       mngmtVars->ADDBSResponseReceived);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management addba response dropped = %d",
                       mngmtVars->ADDBSResponseDropped);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                sprintf(buf, "Management delba request sent = %d",
                       mngmtVars->DELBARequestSent);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management delba request received = %d",
                       mngmtVars->DELBARequestReceived);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);

                 sprintf(buf, "Management delba request dropped = %d",
                       mngmtVars->DELBARequestDropped);

                IO_PrintStat(
                    node,
                    "MAC",
                    DOT11_MANAGEMENT_STATS_LABEL,
                    ANY_DEST,
                    interfaceIndex,
                    buf);
    }
    //}
 }
//--------------------------------------------------------------------------
/*!
 * \brief  Print management statistics.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure
 * \param interfaceIndex  int       :Interface index

 * \return          BOOL            : TRUE if states permit or transmit occurs
 */
//--------------------------------------------------------------------------
void MacDot11ManagementPrintStats(Node* node,
    MacDataDot11* dot11,
    int interfaceIndex)
{
    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
    // Skip stats if not enabled
    if (mngmtVars == NULL ||
        !mngmtVars->printManagementStatistics) {
        return;
    }

    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Management packets sent to channel = %d",
           mngmtVars->managementPacketsSent);

    IO_PrintStat(
        node,
        "MAC",
        DOT11_MANAGEMENT_STATS_LABEL,
        ANY_DEST,
        interfaceIndex,
        buf);

    sprintf(buf, "Management packets received from channel= %d",
           mngmtVars->managementPacketsGot);

    IO_PrintStat(
        node,
        "MAC",
        DOT11_MANAGEMENT_STATS_LABEL,
        ANY_DEST,
        interfaceIndex,
        buf);
    MacDot11ManagementFramesSendPrintStats(node,
                                        dot11,
                                        interfaceIndex);

    sprintf(buf, "Beacons received = %d",
           dot11->beaconPacketsGot);

    IO_PrintStat(
        node,
        "MAC",
        DOT11_MANAGEMENT_STATS_LABEL,
        ANY_DEST,
        interfaceIndex,
        buf);

    sprintf(buf, "Beacons sent = %d",
           dot11->beaconPacketsSent);

    IO_PrintStat(
        node,
        "MAC",
        DOT11_MANAGEMENT_STATS_LABEL,
        ANY_DEST,
        interfaceIndex,
        buf);

}


//--------------------------------------------------------------------------
/*!
 * \brief  Reset management.
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          BOOL            : TRUE if states permit or transmit occurs
 */
//--------------------------------------------------------------------------
void MacDot11ManagementReset(
    Node* node,
    MacDataDot11* dot11)
{
    if (dot11->state != DOT11_S_IDLE)
    {
        //set state to pending
        dot11->ManagementResetPending = TRUE;
        return;
    }
    if (dot11->currentMessage != NULL)
    {
        //set state to pending
        dot11->ManagementResetPending = TRUE;
        return;
    }

    if (dot11->isHTEnable)
    {
            // Drop all packets from various input and output buffers.
            MacDot11nResetOutputBuffer(node, dot11, TRUE, FALSE);
            MacDot11nResetAmsduBuffer(node, dot11, TRUE, FALSE);
            MacDot11nResetCurrentMessageVariables(dot11);
    }




    // Deauthenticate and disassociate
    dot11->stationAuthenticated = FALSE;
    dot11->stationAssociated = FALSE;
    dot11->associatedWithHC = FALSE;
//---------------------------Power-Save-Mode-Updates---------------------//
    dot11->joinedAPSupportPSMode = TRUE;
//---------------------------Power-Save-Mode-End-Updates-----------------//

//---------------------DOT11e--Updates------------------------------------//

    dot11->associatedWithQAP = FALSE;

//--------------------DOT11e-End-Updates---------------------------------//

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
    while (dot11->mngmtQueue->first != NULL)
    {
        MacDot11MgmtQueue_DequeuePacket(node,dot11);
    }

    mngmtVars->beaconHeard = FALSE;
    mngmtVars->prevApAddr = mngmtVars->apAddr;

    mngmtVars->apAddr = INVALID_802ADDRESS;
    mngmtVars->isMap = FALSE;

    MacDot11StationSetState(
        node,
        dot11,
        DOT11_S_IDLE);

    MacDot11ManagementStartJoin(
        node,
        dot11);

    
}// MacDot11ManagementReset


//--------------------------------------------------------------------------
/*!
 * \brief  Enable management
 *
 * \param node      Node*           : Pointer to node
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          int             : Error code
 */
//--------------------------------------------------------------------------
int MacDot11ManagementEnable(
    Node* node,
    MacDataDot11* dot11)
{

    int result = DOT11_R_FAILED;

    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;

    MacDot11ManagementStartListeningToChannel(
            node,
            dot11->myMacData->phyNumber,
            (short)mngmtVars->currentChannel,
            dot11);


    PHY_SetTransmissionChannel(node, dot11->myMacData->phyNumber,
        mngmtVars->currentChannel);


    if (MacDot11IsAp(dot11)) {  // ACCESS POINT
        // Set the BSSID to the same as our MAC in MIB
        // like dot11->stationMIB->dot11StationID = dot11->selfAddr;

        // Ap should just start beaconing
        MacDot11ManagementSetState(
            node,
            dot11,
            DOT11_S_M_BEACON);

        result = DOT11_R_OK;

    } else {  // STATION
        // Not worried aboud IBSS for now, assume infrastructure
        if (dot11->stationType != DOT11_STA_IBSS) {

            ERROR_Assert(dot11->stationType != DOT11_STA_UNKNOWN,
                         "MacDot11ManagementEnable:"
                         "Station type is not set.");

            // Reset the BSSID to unknown address
            mngmtVars->prevApAddr = INVALID_802ADDRESS;
            mngmtVars->apAddr = INVALID_802ADDRESS;

            mngmtVars->isMap = FALSE;
            mngmtVars->currentApAddr = INVALID_802ADDRESS;

            if (MacDot11IsAssociationDynamic(dot11)){
                // Start a join
                MacDot11ManagementStartJoin(
                    node,
                    dot11);

            } else {
                // Auto join
                MacDot11ManagementAutoJoin(
                    node,
                    dot11);
            }
        }

        result = DOT11_R_OK;
    }

    return result;
}// MacDot11ManagementEnable
//--------------------------------------------------------------------------
// NAME         MacDot11ManagementCheckHeaderSizes
// PURPOSE      Check for valid header sizes.
// PARAMETERS   None
// RETURN       None
// NOTES        None
//-------------------------------------------------------------------------
static //inline
void MacDot11ManagementCheckHeaderSizes()
{
    int ProbReqSize = sizeof(DOT11_ProbeReqFrame);
    int AuthFrameSize = sizeof(DOT11_AuthFrame);
    int ProbRespSize = sizeof(DOT11_ProbeRespFrame);
    int dot11eProbRespSize = sizeof(DOT11e_ProbeRespFrame);
    int AssocReqSize = sizeof(DOT11_AssocReqFrame);
    int Dot11eAssocReqSize = sizeof(DOT11e_AssocReqFrame);
    int ReassocReqSize = sizeof(DOT11_ReassocReqFrame);
    int Dot11eReassocReqSize = sizeof(DOT11e_ReassocReqFrame);
    int AssocRespSize = sizeof(DOT11_AssocRespFrame);
    int DOT11eAssocRespSize = sizeof(DOT11e_AssocRespFrame);
    int DisassocFrameSize = sizeof(DOT11_DisassocFrame);


    assert(ProbReqSize == 60);
    assert(AuthFrameSize == 34);
    assert(ProbRespSize == 72);
    assert(dot11eProbRespSize == 72);
    assert(AssocReqSize == 64);
    assert(Dot11eAssocReqSize == 64);
    assert(ReassocReqSize == 70);
    assert(Dot11eReassocReqSize == 70);
    assert(AssocRespSize == 34);
    assert(DOT11eAssocRespSize == 34);
    assert(DisassocFrameSize == 30);

}//MacDot11ManagementCheckHeaderSizes


//--------------------------------------------------------------------------
/*!
 * \brief  Initalize dynamic link management.
 *
 * \param node      Node*           : Pointer to node
 * \param nodeInput NodeInput*      : Node input configuration
 * \param dot11     MacDataDot11*   : Pointer to Dot11 structure

 * \return          None.
 */
//--------------------------------------------------------------------------
void MacDot11ManagementInit(
    Node* node,
    const NodeInput* nodeInput,
    MacDataDot11* dot11,
    NetworkType networkType,
    BOOL printStatistics)
{
    char retString[MAX_STRING_LENGTH];
    char errString[MAX_STRING_LENGTH];
    int retInt = 0;
    BOOL wasFound = FALSE;
    Address address;
    double retDouble;


    DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars*)MEM_malloc(sizeof(DOT11_ManagementVars));

    memset(mngmtVars, 0, sizeof(DOT11_ManagementVars));

    MacDot11ManagementCheckHeaderSizes();

    mngmtVars->state = DOT11_S_M_IDLE;
    mngmtVars->prevState = DOT11_S_M_IDLE;
    mngmtVars->frameClass = DOT11_FRAME_CLASS1;

    mngmtVars->printManagementStatistics = printStatistics;
    mngmtVars->managementPacketsSent = 0;
    mngmtVars->managementPacketsGot = 0;

    mngmtVars->assocReqDropped = 0;
    mngmtVars->assocRespDropped = 0;
    mngmtVars->reassocReqDropped = 0;
    mngmtVars->reassocRespDropped = 0;
    mngmtVars->ProbRespDropped = 0;
    mngmtVars->AuthReqDropped = 0;
    mngmtVars->AuthReqDropped = 0;

    mngmtVars->assocReqSend = 0;
    mngmtVars->assocRespSend = 0;
    mngmtVars->reassocReqSend = 0;
    mngmtVars->reassocRespSend = 0;
    mngmtVars->ProbReqSend = 0;
    mngmtVars->ProbRespSend = 0;
    mngmtVars->AuthReqSend = 0;
    mngmtVars->AuthRespSend = 0;

    mngmtVars->assocReqReceived = 0;
    mngmtVars->assocRespReceived = 0;
    mngmtVars->reassocReqReceived = 0;
    mngmtVars->reassocRespReceived = 0;
    mngmtVars->ProbReqReceived = 0;
    mngmtVars->ProbRespReceived = 0;
    mngmtVars->AuthReqReceived = 0;
    mngmtVars->AuthRespReceived = 0;

    mngmtVars->beaconHeard = FALSE;
    mngmtVars->prevApAddr = INVALID_802ADDRESS;
    mngmtVars->apAddr = INVALID_802ADDRESS;
    mngmtVars->isMap = FALSE;

    NetworkGetInterfaceInfo(
                node,
                dot11->myMacData->interfaceIndex,
                &address,
                networkType);

    // The type of scan used for joining
    // MAC-DOT11-SCAN-TYPE DISABLED | PASSIVE | ACTIVE e.g.
    // [2] MAC-DOT11-SCAN-TYPE ACTIVE
    // Would indicate that node 2 used probing to find an AP.
    //
    IO_ReadString(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-SCAN-TYPE",
        &wasFound,
        retString);

    if ((!wasFound) || (strcmp(retString, "DISABLED") == 0))
    {
        mngmtVars->scanType = DOT11_SCAN_DISABLED;

    }
    else if (MacDot11IsAp(dot11) &&
             ((strcmp(retString, "PASSIVE") == 0) ||
              (strcmp(retString, "ACTIVE") == 0)))
    {
        ERROR_ReportWarning("The parameter MAC-DOT11-SCAN-TYPE is not "
                            "configurable at AP. Setting the scan type "
                            "to disabled.");
        mngmtVars->scanType = DOT11_SCAN_DISABLED;
    }
    else if (strcmp(retString, "PASSIVE") == 0)
    {
        mngmtVars->scanType = DOT11_SCAN_PASSIVE;

    }
    else if (strcmp(retString, "ACTIVE") == 0)
    {
        mngmtVars->scanType = DOT11_SCAN_ACTIVE;
        int longScanTimer = 0;
        Address address;
        NetworkGetInterfaceInfo(
                    node,
                    dot11->myMacData->interfaceIndex,
                    &address,
                    networkType);
        IO_ReadInt(
           node,
           node->nodeId,
           dot11->myMacData->interfaceIndex,
           nodeInput,
           "MAC-DOT11-SCAN-MAX-CHANNEL-TIME",
           &wasFound,
           &longScanTimer);

        if (!wasFound)
        {
            mngmtVars->longScanTimer = DOT11_LONG_SCAN_TIMER_DEFAULT;
        }
        else
        {
            mngmtVars->longScanTimer =
                    MacDot11TUsToClocktype((unsigned short)longScanTimer);
        }

    }
    else
    {
        ERROR_ReportWarningArgs("The parameter MAC-DOT11-SCAN-TYPE is "
                                "incorrectly configured as \"%s\". Setting "
                                "the scan type to disabled", retString);
        mngmtVars->scanType = DOT11_SCAN_DISABLED;

    }

    int firstListeningChannel = -1;
    int i = 0;
    BOOL isListeningChannelSet = FALSE;

    for (i = 0; i < PROP_NumberChannels(node); i++)
    {
        if (PHY_CanListenToChannel(
                        node,
                        dot11->myMacData->phyNumber,
                        i))
        {
            if (PHY_IsListeningToChannel(
                        node,
                        dot11->myMacData->phyNumber,
                        i))
            {
                if (!isListeningChannelSet)
                {
                    firstListeningChannel = i;
                    isListeningChannelSet = TRUE;
                }
            }
            else
            {
                if (firstListeningChannel == -1)
                {
                    firstListeningChannel = i;
                }
            }
        }

        MacDot11ManagementStopListeningToChannel(
            node,
            dot11->myMacData->phyNumber,
            (short)i);
    }

    sprintf(errString,
        "MacDot11Init: "
        "No listenable channel configured for node.interface %d %d\n",
        node->nodeId, dot11->myMacData->phyNumber);

    ERROR_Assert(firstListeningChannel >= 0, errString);


    // Read default station channel.
    // Format is :
    //      MAC-DOT11-STA-CHANNEL <value>
    // Default is MAC-DOT11-STA-CHANNEL (value 0)
    //
    IO_ReadString(
            node,
            node->nodeId,
            dot11->myMacData->interfaceIndex,
            nodeInput,
            "MAC-DOT11-STA-CHANNEL",
            &wasFound,
            retString);

    if (wasFound)
    {
        if (IO_IsStringNonNegativeInteger(retString))
        {
            mngmtVars->currentChannel = (Int32)strtoul(retString, NULL, 10);

            if (mngmtVars->currentChannel >= node->numberChannels)
            {
                ERROR_ReportWarningArgs(
                    "Value of MAC-DOT11-STA-CHANNEL is more than available"
                    " number of channels. Setting the channel to first"
                    " listening channel: %d",
                    firstListeningChannel);

                mngmtVars->currentChannel = firstListeningChannel;
            }
        }
        else if (isalpha(*retString) && PHY_ChannelNameExists(node, retString))
        {
            mngmtVars->currentChannel =
                            PHY_GetChannelIndexForChannelName(node,
                                                              retString);
        }
        else
        {
            ERROR_ReportWarningArgs("MAC-DOT11-STA-CHANNEL "
                                    "parameter should be an integer between "
                                    "0 and %d or a valid channel name. "
                                    "Setting the channel to first listening "
                                    "channel: %d",
                                    node->numberChannels - 1,
                                    firstListeningChannel);

            mngmtVars->currentChannel = firstListeningChannel;
        }
    }
    else
    {
        mngmtVars->currentChannel = firstListeningChannel;
    }

    // Init channel structures for scanning
    DOT11_ChannelInfo* channelInfo =
        (DOT11_ChannelInfo*) MEM_malloc(sizeof(DOT11_ChannelInfo));

    memset(channelInfo, 0, sizeof(DOT11_ChannelInfo));

    // load in the channels
    channelInfo->numChannels = (unsigned)PROP_NumberChannels(node);


    ERROR_Assert(channelInfo->numChannels < MAX_NUMBER_CHANNELLIST,
              "Number of Channels in scenario exceeds maximum limit of 96.");


    for (unsigned int i = 0; i < channelInfo->numChannels ;i++ )
    {
        channelInfo->channelList[i] = i;
    }

    mngmtVars->channelInfo = channelInfo;

    dot11->managementSequenceNumber = 0;

    dot11->mngmtVars = mngmtVars;


    IO_ReadDouble(
        node,
        node->nodeId,
        dot11->myMacData->interfaceIndex,
        nodeInput,
        "MAC-DOT11-STATION-HANDOVER-RSS-TRIGGER",
        &wasFound,
        &retDouble);

    if (wasFound) {
        dot11->thresholdSignalStrength = retDouble;
    }else {
        int phyIndex = dot11->myMacData->phyNumber;
        PhyData *thisPhy = node->phyData[phyIndex];
        switch(thisPhy->phyModel)
        {
            case PHY802_11b:
            case PHY802_11a:
            {
                PhyData802_11* phy802_11 = (PhyData802_11*)(thisPhy->phyVar);
                dot11->thresholdSignalStrength =
                        Phy802_11GetSignalStrength(node,phy802_11) +
                        DOT11_HANDOVER_RSS_MARGIN;
                break;
            }
            case PHY802_11n:
                {
                    PhyData802_11* phy802_11 = (PhyData802_11*)(thisPhy->phyVar);
                        dot11->thresholdSignalStrength =
                        Phy802_11nGetSignalStrength(node,phy802_11) +
                        DOT11_HANDOVER_RSS_MARGIN;

                    break;
                }
            case PHY802_11ac:
            {
                PhyData802_11* phy802_11 = (PhyData802_11*)(thisPhy->phyVar);
                dot11->thresholdSignalStrength =
                                Phy802_11acGetSignalStrength(node,phy802_11)
                                + DOT11_HANDOVER_RSS_MARGIN;
                break;
            }
            case PHY_ABSTRACT: {
                PhyDataAbstract* phy_abstract = (PhyDataAbstract *)thisPhy->phyVar;
                dot11->thresholdSignalStrength =
                    IN_DB(phy_abstract->rxSensitivity_mW);
                break;
            }
            default: {
                ERROR_ReportError("Unsupported or disabled PHY model");
            }
        }
    }

    // Enable station management
    MacDot11ManagementStartTimerOfGivenType(node, dot11, 0,
                                    MSG_MAC_DOT11_Enable_Management_Timer);
}// MacDot11ManagementInit
//---------------------------Power-Save-Mode-Updates---------------------//
//--------------------------------------------------------------------------
//  NAME:        MacDot11StationTransmitPSPollFrame
//  PURPOSE:     Transmit PSPoll to receive data.
//  PARAMETERS:  Node* node
//                  Pointer to node
//               MacDataDot11* dot11
//                  Pointer to Dot11 structure
//               Mac802Address detsAddr
//                  Node address of destination station
//  RETURN:      None.
//  ASSUMPTION:  None.
//--------------------------------------------------------------------------

 //inline//
void MacDot11StationTransmitPSPollFrame(
    Node* node,
    MacDataDot11* dot11,
    Mac802Address destAddr)
{

    Message* pktToPhy;
    DOT11_LongControlFrame* newHdr;

    pktToPhy = MESSAGE_Alloc(node, 0, 0, 0);

    MESSAGE_PacketAlloc(node,
                        pktToPhy,
                        DOT11_LONG_CTRL_FRAME_SIZE,
                        TRACE_DOT11);

    newHdr = (DOT11_LongControlFrame*)MESSAGE_ReturnPacket(pktToPhy);
    memset(newHdr, 0, sizeof(DOT11_LONG_CTRL_FRAME_SIZE));

    newHdr->frameType = DOT11_PS_POLL;
    newHdr->sourceAddr = dot11->selfAddr;
    newHdr->destAddr = destAddr;
    newHdr->duration = 0;
    DOT11_FrameInfo* newframeInfo =
        (DOT11_FrameInfo*) MEM_malloc(sizeof(DOT11_FrameInfo));

        newframeInfo->msg = pktToPhy;
        newframeInfo->frameType = DOT11_PS_POLL;
        newframeInfo->RA = newHdr->destAddr;
        newframeInfo->TA = dot11->selfAddr;
        newframeInfo->DA = newHdr->destAddr;
        newframeInfo->SA = dot11->selfAddr;
        newframeInfo->insertTime = node->getNodeTime();

    // Have something to send. Set Backoff if not already set and
    // start sending process
     dot11->mgmtSendResponse = FALSE;
     MacDot11MgmtQueue_EnqueuePacket(node,dot11,newframeInfo);
    return;
}// MacDot11StationTransmitPSPollFrame
//---------------------------Power-Save-Mode-End-Updates-----------------//
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
                                         DOT11_FrameInfo* dot11TxFrameInfo)
{
        DOT11_ManagementVars* mngmtVars =
        (DOT11_ManagementVars *) dot11->mngmtVars;
    switch(dot11TxFrameInfo->frameType)
    {
        case DOT11_ASSOC_REQ:
        {
            // Reset station Management
            mngmtVars->assocReqDropped++;
            MacDot11ManagementReset(node, dot11);

            break;
        }
        case DOT11_ASSOC_RESP:
        {
            mngmtVars->assocRespDropped++;
            break;
        }
        case DOT11_REASSOC_REQ:
        {
            // Reset station Management
            mngmtVars->reassocReqDropped++;
            MacDot11ManagementReset(node, dot11);

            break;
        }
        case DOT11_REASSOC_RESP:
        {
            mngmtVars->reassocRespDropped++;
            break;
        }
        case DOT11_PROBE_RESP:
        {
            mngmtVars->ProbRespDropped++;
            break;
        }
        case DOT11_BEACON:
        {
            break;
        }
        case DOT11_ATIM:
        {
            break;
        }
        case DOT11_DISASSOCIATION:
        {
             break;
        }
        case DOT11_AUTHENTICATION:
        {
            if (MacDot11IsBssStation(dot11))
            {
                // Reset station Management
                mngmtVars->AuthReqDropped++;
                MacDot11ManagementReset(node, dot11);
            }
            else
            {
                mngmtVars->AuthRespDropped++;
            }
            break;
        }
        case DOT11_DEAUTHENTICATION:
        {
            break;
        }
        case DOT11_ACTION:
        {
            if (dot11->isHTEnable)
            {
                MacDot11nUpdateBapForActionFrames(node,
                                                  dot11, 
                                                  dot11TxFrameInfo,
                                                  FALSE);
            }
            break;
        }
        case DOT11N_BLOCK_ACK_REQUEST:
        {
                MacDot11nUpdateBapForBar(node, 
                                         dot11, 
                                         dot11TxFrameInfo,
                                         FALSE);
            break;
        }
        case DOT11N_DELAYED_BLOCK_ACK:
        {
            // delayed block transmission failed
            // wait for another block ack request
            dot11->totalNoDelBlockAckDropped++;
            break;
        }
        case DOT11_PROBE_REQ:
        {
            // Ibss mode! Unicast poll request failed to be sent.
            // reset the probestatus so that more poll requests
            // can be sent when required.

            ERROR_Assert(dot11->isHTEnable,"HT disabled");
            mngmtVars->ProbReqDropped++;
            DOT11n_IBSS_Station_Info *stationInfo
                = MacDot11nGetIbssStationInfo(dot11, dot11TxFrameInfo->RA);
            ERROR_Assert(stationInfo->timerMessage == NULL,"timer ! = NULL");
            switch (stationInfo->probeStatus)
            {
            case IBSS_PROBE_NONE:
            case IBSS_PROBE_IN_PROGRESS:
            case IBSS_PROBE_COMPLETED:
                {
                    ERROR_Assert(FALSE,"invalid State");
                    break;
                }
            case IBSS_PROBE_REQUEST_QUEUED:
                {
                    stationInfo->probeStatus = IBSS_PROBE_NONE;
        break;
    }
        default:
        {
                    ERROR_Assert(FALSE,"invalid state");
            }
        }
        break;
    }
        default:
        {
            printf("\n%d, %d",dot11TxFrameInfo->frameType, node->nodeId);
            ERROR_Assert(FALSE,"invalid state");
            break;
        }
    }
}
//-------------------------------------------------------------------------
