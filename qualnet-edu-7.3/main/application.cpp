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

/*
 * This file contains initialization function, message processing
 * function, and finalize function used by application layer.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <list>

#include "api.h"
#include "clock.h"
#include "coordinates.h"
#include "partition.h"
#include "external.h"
#include "external_util.h"

#include "network_ip.h"
#include "network_icmp.h"
#include "ipv6.h"
#include "app_util.h"
#include "if_loopback.h"

#include "app_ftp.h"
#include "app_gen_ftp.h"
#include "app_telnet.h"
#include "app_cbr.h"
#include "app_forward.h"
#include "app_http.h"
#include "app_traffic_gen.h"
#include "app_traffic_trace.h"
#include "app_mcbr.h"
#include "app_messenger.h"
#include "app_superapplication.h"
#include "app_timer.h" // timer manager
#include "app_vbr.h"
#include "app_lookup.h"

#include "routing_static.h"
#include "routing_bellmanford.h"
#include "routing_rip.h"
#include "routing_ripng.h"
#include "multicast_msdp.h"

#include "network_dualip.h"

#include "app_mdp.h"
#include "app_trafficSender.h"

#include "cellular.h"

#ifdef ENTERPRISE_LIB
#include "mpls.h"
#include "transport_rsvp.h"
#include "routing_hsrp.h"
#include "multimedia_h323.h"
#include "multimedia_h225_ras.h"
#include "multimedia_sip.h"
#include "app_voip.h"
#include "transport_rtp.h"
#include "routing_bgp.h"
#include "mpls_ldp.h"
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
#include "cellular_gsm.h"
#include "cellular_abstract.h"
#include "app_cellular_abstract.h"
#endif

#if defined(UMTS_LIB)
#include "app_umtscall.h"
#endif

#ifdef CYBER_CORE
#include "network_iahep.h"
#endif // CYBER_CORE

#ifdef ALE_ASAPS_LIB
#include "mac_ale.h"
#endif // ALE_ASAPS_LIB

#ifdef WIRELESS_LIB
#include "routing_fisheye.h"
#include "routing_olsr-inria.h"
#include "routing_olsrv2_niigata.h"
#include "network_neighbor_prot.h"

#include "cellular_layer3.h"
#endif // WIRELESS_LIB

#ifdef SENSOR_NETWORKS_LIB
#include "app_zigbeeApp.h"
#endif // SENSOR_NETWORKS_LIB

#ifdef CYBER_LIB
#include "app_jammer.h"
#include "app_dos.h"
#include "firewall_model.h"
#include "app_sigint.h"
#include "channel_scanner.h"
#include "app_eaves.h"
#include "app_portscan.h"
#endif // CYBER_LIB

#ifdef ADDON_MGEN4
#include "mgenApp.h"
#endif

#ifdef ADDON_MGEN3
#include "mgen3.h"
#include "drec.h"
#endif

//InsertPatch HEADER_FILES

#ifdef ADDON_HELLO
#include "hello.h"
#endif /* ADDON_HELLO */

#ifdef TUTORIAL_INTERFACE
#include "interfacetutorial_app.h"
#endif /* TUTORIAL_INTERFACE */

#ifdef MILITARY_RADIOS_LIB
#include "tadil_util.h"
#endif

#ifdef MILITARY_UNRESTRICTED_LIB
#include "app_threaded.h"
#define INPUT_LENGTH 1000
#endif

#ifdef EXATA
#include <sys/stat.h>
#include "app_eftp.h"
#include "app_etelnet.h"
#include "app_ehttp.h"
#include "record_replay.h"
#ifdef NETCONF_INTERFACE
#include "app_netconf.h"
#endif
#ifdef IPNE_INTERFACE
#include "ipnetworkemulator.h"
#endif
#ifdef NETSNMP_INTERFACE
#include "app_netsnmp.h"
#endif

#define PRODUCT_NAME "EXATA"
#endif  // EXATA



#ifdef ADDON_DB
#include "db.h"
#include "dbapi.h"
#endif //ADDON_DB


#ifdef LTE_LIB
#include "epc_lte_common.h"
#include "epc_lte_app.h"
#endif // LTE_LIB

#include "product_info.h"

// dns
#include "app_dns.h"

// DHCP
#include "app_dhcp.h"

// Pseudo traffic sender layer
#include "app_trafficSender.h"

// Modifications
#ifdef USER_MODELS_LIB
#include "app_hello.h"
#include "app_up.h"
#endif // USER_MODELS_LIB

void AppMap::pushDetail(
    const Address& sourceAddress,
    int id,
    AppType type,
    void* detail)
{
    AppIdentifier ident;
    std::map<AppIdentifier, AppInfo*>::iterator it;
    AppInfo* info = (AppInfo*) MEM_malloc(sizeof(AppInfo));

    info->appType = type;
    info->appDetail = detail;
    info->appNext = NULL;

    ident.sourceAddress = sourceAddress;
    ident.id = id;

    it = m_map.find(ident);
    if (it == m_map.end())
    {
        // Start new list
        m_map[ident] = info;
    }
    else
    {
        // Add to front of list
        info->appNext = m_map[ident];
        m_map[ident]->appNext = info;
    }
}

void AppMap::pushDetail(
    int id,
    AppType type,
    void* detail)
{
    std::map<int, AppInfo*>::iterator it;
    AppInfo* info = (AppInfo*) MEM_malloc(sizeof(AppInfo));

    info->appType = type;
    info->appDetail = detail;
    info->appNext = NULL;

    it = m_mapById.find(id);
    if (it == m_mapById.end())
    {
        // Start new list
        m_mapById[id] = info;
    }
    else
    {
        // Add to front of list
        info->appNext = m_mapById[id];
        m_mapById[id]->appNext = info;
    }
}

AppInfo* AppMap::getAppInfo(const Address& sourceAddress, int id)
{
    AppIdentifier ident;
    std::map<AppIdentifier, AppInfo*>::iterator it;

    ident.sourceAddress = sourceAddress;
    ident.id = id;

    it = m_map.find(ident);
    if (it == m_map.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}

AppInfo* AppMap::getAppInfo(int id)
{
    std::map<int, AppInfo*>::iterator it;

    it = m_mapById.find(id);
    if (it == m_mapById.end())
    {
        return NULL;
    }
    else
    {
        return it->second;
    }
}


void
AppInputHttpError(void)
{
    char errorbuf[MAX_STRING_LENGTH];
    sprintf(errorbuf,
            "Wrong HTTP configuration format!\n"
            "HTTP <client> <num-servers> <server1> <server2> ... "
            "<start time> <threshhold time>\n");
    ERROR_ReportError(errorbuf);
}


// Checks and Returns equivalent 8-bit TOS value
BOOL
APP_AssignTos(
    char tosString[],
    char tosValString[],
    unsigned* tosVal)
{
    unsigned value = 0;

    if (IO_IsStringNonNegativeInteger(tosValString))
    {
        if (!strcmp(tosString, "PRECEDENCE"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 7)
            {
                // Equivalent TOS [8-bit] value [[PRECEDENCE << 5]]
                *tosVal = value << 5;
                return TRUE;
            }
            else
            {
                // PRECEDENCE [range < 0 to 7 >]
                ERROR_ReportError("PRECEDENCE value range < 0 to 7 >");
            }
        }

        if (!strcmp(tosString, "DSCP"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 63)
            {
                // Equivalent TOS [8-bit] value [[DSCP << 2]]
                *tosVal = value << 2;
                return TRUE;
            }
            else
            {
                // DSCP [range <0 to 63 >]
                ERROR_ReportError("DSCP value range < 0 to 63 >");
            }
        }

        if (!strcmp(tosString, "TOS"))
        {
            value = (unsigned) atoi(tosValString);

            if (/*value >= 0 &&*/ value <= 255)
            {
                // Equivalent TOS [8-bit] value
                *tosVal = value;
                return TRUE;
            }
            else
            {
                // TOS [range <0 to 255 >]
                ERROR_ReportError("TOS value range < 0 to 255 >");
            }
        }
    }
    return FALSE;
}


/*
 * NAME:        APP_InitiateIcmpMessage.
 * PURPOSE:     Call NetworkIcmpCreateErrorMessage()to generate port
                unreachable message
 * PARAMETERS:  node - pointer to the node
                msg - pointer to the received message
                icmp - pointer to ICMP data structure
 * RETURN:      none.
 */
void
APP_InitiateIcmpMessage(Node *node, Message *msg,NetworkDataIcmp *icmp)
{

    UdpToAppRecv* udpToApp = (UdpToAppRecv *) MESSAGE_ReturnInfo(msg);
    // incoming interface and Destination address received from info Field

    int interfaceId = udpToApp->incomingInterfaceIndex;
    NodeAddress destinationAddress = GetIPv4Address(udpToApp-> sourceAddr);

    BOOL ICMPErrorMsgCreated = NetworkIcmpCreateErrorMessage(
                                   node,
                                   msg,
                                   destinationAddress,
                                   interfaceId,
                                   ICMP_DESTINATION_UNREACHABLE,
                                   ICMP_PORT_UNREACHABLE,
                                   0,
                                   0);
    if (ICMPErrorMsgCreated)
    {
#ifdef DEBUG_ICMP_ERROR_MESSAGES
        char srcAddr[MAX_STRING_LENGTH];
        IO_ConvertIpAddressToString(destinationAddress, srcAddr);
        printf("Node %d sending port unreachable message to %s\n",
                            node->nodeId, srcAddr);
#endif
        (icmp->icmpErrorStat.icmpPortUnreacableSent)++;
    }
}


/*
 * NAME:        APP_HandleLoopback.
 * PURPOSE:     Checks applications on nodes for loopback
 * PARAMETERS:  node - pointer to the node,
 * ASSUMPTION:  Network initialization is completed before
 *              Application initialization for a node.
 * RETURN:      BOOL.
 */
static BOOL
APP_SuccessfullyHandledLoopback(
    Node *node,
    const char *inputString,
    Address destAddr,
    NodeAddress destNodeId,
    Address sourceAddr,
    NodeAddress sourceNodeId)
{
    BOOL isLoopBack = FALSE;
    if ((sourceAddr.networkType == NETWORK_IPV6
        || destAddr.networkType == NETWORK_IPV6)
        && (Ipv6IsLoopbackAddress(node, GetIPv6Address(destAddr))
        || !CMP_ADDR6(GetIPv6Address(sourceAddr), GetIPv6Address(destAddr))
        || destNodeId == sourceNodeId))
    {
        isLoopBack = TRUE;
    }
    else if ((sourceAddr.networkType == NETWORK_IPV4
            || destAddr.networkType == NETWORK_IPV4)
            && (NetworkIpIsLoopbackInterfaceAddress(GetIPv4Address(destAddr))
            || (destNodeId == sourceNodeId)
            || (GetIPv4Address(sourceAddr) == GetIPv4Address(destAddr))))
    {
        isLoopBack = TRUE;
    }

    if (isLoopBack)
    {
        NetworkDataIp* ip = (NetworkDataIp *) node->networkData.networkVar;

        if (!ip->isLoopbackEnabled)
        {
            char errorStr[MAX_STRING_LENGTH] = {0};
            // Error
            sprintf(errorStr, "IP loopback is disabled for"
                " node %d!\n  %s\n", node->nodeId, inputString);

            ERROR_ReportError(errorStr);
        }
        // Node pointer already found, skip scanning mapping
        // for destNodeId

        destNodeId = sourceNodeId;

        return TRUE;
    }
    return FALSE;
}

/*
 * NAME:        APP_TraceInitialize.
 * PURPOSE:     Initialize trace routine of desired application
 *              layer protocols.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void APP_TraceInitialize(Node *node, const NodeInput *nodeInput)
{
    BOOL retVal;
    char apStr[MAX_STRING_LENGTH];
    NodeInput appInput;

    // return if trace is  disabled
    if (!node->partitionData->traceEnabled)
    {
        return;
    }

    IO_ReadCachedFile(ANY_NODEID,
                      ANY_ADDRESS,
                      nodeInput,
                      "APP-CONFIG-FILE",
                      &retVal,
                      &appInput);

    if (retVal == TRUE)
    {
       for (int index = 0; index < appInput.numLines; index++)
       {
           sscanf(appInput.inputStrings[index], "%s", apStr);

           // Init relevant app layer prtocol trace routine
           if (!strcmp(apStr, "CBR"))
           {
               AppLayerCbrInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "TRAFFIC-GEN"))
           {
               TrafficGenInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "FTP/GENERIC"))
           {
               AppGenFtpInitTrace(node, nodeInput);
           }
           else if (!strcmp(apStr, "SUPER-APPLICATION"))
           {
               SuperApplicationInitTrace(node, nodeInput);
           }
#ifdef SENSOR_NETWORKS_LIB
           else if (!strcmp(apStr,"ZIGBEEAPP"))
           {
               ZigbeeAppInitTrace(node, nodeInput);
           }
#endif
        }

    }
}

//fix to read signalling parameters by all nodes
/*
 * NAME:        Multimedia_Initialize.
 * PURPOSE:     initializes multimedia related parameters according to
 *               user's specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
static void
Multimedia_Initialize(Node *node, const NodeInput *nodeInput)
{
    char buf[MAX_STRING_LENGTH];
    BOOL retVal;
    BOOL gatekeeperFound = FALSE;
    H323CallModel callModel = Direct;
    if (node->appData.multimedia)
    {
        // Voip related intialization
        if (node->appData.multimedia->sigType == SIG_TYPE_H323)
        {
            // Gatekeeper of H323 should be created before starting
            // the application loop
            IO_ReadString(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "H323-CALL-MODEL",
                &retVal,
                buf);
            if (retVal == TRUE)
            {
                if (strcmp(buf, "DIRECT") == 0)
                {
                    callModel = Direct;
                }
                else if (strcmp(buf, "GATEKEEPER-ROUTED") == 0)
                {
                    callModel = GatekeeperRouted;
                }
                else
                {
                    /* invalid entry in config */
                    char errorBuf[MAX_STRING_LENGTH];
                    sprintf(errorBuf,
                        "Invalid call model (%s) for H323\n", buf);
                    ERROR_ReportError(errorBuf);
                }
            }
            gatekeeperFound = H323GatekeeperInit(node, nodeInput,
                                                 callModel);
        }
        else if (node->appData.multimedia->sigType == SIG_TYPE_SIP)
        {
            SipProxyInit(node, nodeInput);
        }
    }
}

#ifdef CYBER_LIB

void
APP_InitializeCyberApplication(
        Node* firstNode,
        const char* cyberInput,
        BOOL fromGUI)
{
    char appStr[MAX_STRING_LENGTH];
    char* appInput = NULL;
    Int32 len = 0;
    IdToNodePtrMap* nodeHash;
    std::vector<HITLInfo> apps;

    if (firstNode == NULL)
    {
        return; // this partition has no nodes.
    }

    len = strlen(cyberInput) + 1;
    appInput = (char*) MEM_malloc(len);
    memset(appInput, 0, len);
    strcpy(appInput, cyberInput);

    nodeHash = firstNode->partitionData->nodeIdHash;

    // Trim leading and trailing spaces in the entered command
    IO_TrimLeft(appInput);
    IO_TrimRight(appInput);

    // Truncate multiple spaces to single space
    TruncateSpaces(appInput);

    IO_ConvertStringToUpperCase(appInput);

    sscanf(appInput, "%s", appStr);

    if (!strcmp(appStr, "DOS"))
    {
        /*DOS <victim> <num-of-attackers> <attacker1> <attacker2>...<attackerN> <attack-type> <port>
            <num-of-items> <item-size> <interval> <start-time> <end-time>*/
        char victimNodeStr[MAX_STRING_LENGTH];
        NodeAddress victimNodeId;
        Address victimAddr;
        Node *victimNode;
        sscanf(appInput, "%*s %s", victimNodeStr);
        IO_AppParseSourceString(
            firstNode,
            appInput,
            victimNodeStr,
            &victimNodeId,
            &victimAddr);

        BOOL success = PARTITION_ReturnNodePointer(
                       firstNode->partitionData,
                       &victimNode,
                       victimNodeId,
                       TRUE);

        if (!success) {
            char errorString[MAX_STRING_LENGTH];
            sprintf(errorString,
                "Cannot find a node for the dos: %s", victimNodeStr);
            ERROR_SmartReportError(errorString, !fromGUI);
            return;
        }

        if ((!fromGUI)
            && (victimNode->partitionId != victimNode->partitionData->partitionId))
        {
            return;
        }
        //Send msg to the victim node so that the victim node
        //can update its data structure with the information
        //about the attackers, and send a msg to the attackers
        //to start/stop the dos attack
        Message *victimMsg;
        char *victimInfo;
        victimMsg = MESSAGE_Alloc(victimNode,
            APP_LAYER,
            APP_DOS_VICTIM,
            MSG_DOS_VictimInit);

        MESSAGE_SetInstanceId(victimMsg, fromGUI);

        victimInfo = MESSAGE_InfoAlloc(victimNode, victimMsg, strlen(appInput) + 1);
        memcpy (victimInfo, appInput, strlen(appInput) + 1);
        if (victimNode->partitionData->partitionId != victimNode->partitionId)
        {
            MESSAGE_RemoteSendSafely(victimNode,
                victimNode->nodeId,
                victimMsg,
                0);
        }
        else
        {
            MESSAGE_Send(victimNode, victimMsg, 0);
        }
    }
    /*else if (strcmp(appStr, "SIGINT") == 0)
    {
        char sigintString[MAX_STRING_LENGTH];
        NodeAddress sigintNodeId;
        Node *sigintNode;
        Address sigintNodeAddr;
        sscanf(cyberInput, "%*s %s", sigintString);
        IO_AppParseSourceString(
            firstNode,
            cyberInput,
            sigintString,
            &sigintNodeId,
            &sigintNodeAddr);
        sigintNode = MAPPING_GetNodePtrFromHash(nodeHash, sigintNodeId);
        Message *sigintMsg;
        char *sigInfo;
        sigintMsg = MESSAGE_Alloc(sigintNode,
            APP_LAYER,
            APP_SIGINT,
            //MSG_SIGINT_GuiInit);
            MSG_SIGINT_Start);
        sigintMsg->originatingNodeId = sigintNodeId;
        sigInfo = MESSAGE_InfoAlloc(sigintNode, sigintMsg, strlen(cyberInput));
        memcpy (sigInfo, cyberInput, strlen(cyberInput));
        MESSAGE_Send(sigintNode,sigintMsg,0);
    }*/

    else if (strcmp(appStr, "JAMMER") == 0)
    {
        bool reportError = true;
        std::string inputStr = appInput;
        std::string hId("");
        bool jammerInitialized =
            APP_InitializeJammer(
                firstNode, inputStr, fromGUI, reportError, apps, hId);

        if (!jammerInitialized && reportError)
        {
            std::string errorString (
                "Wrong Jammer configuration format!\n"
                 "JAMMER <src> <delay> <duration> <scanner-index>"
                 " [SILENT <min-rate>] [POWER <user-defined-power>]"
                 " [RAMP-UP-TIME <ramp-up-time>]");

            ERROR_SmartReportError(errorString.c_str(), !fromGUI);
            return;
        }
    }
    else if (strcmp(appStr, "PORTSCAN") == 0)
    {
        App_InitializePortScanner(
            firstNode, std::string(appInput), fromGUI);
    }
    else if (strcmp(appStr, "NETSCAN") == 0)
    {
        App_InitializeNetworkScanner(
            firstNode, std::string(appInput), fromGUI);
    }
    MEM_free(appInput);
}



/*
 * NAME:        APP_InitializeFirewall.
 * PURPOSE:     start firewalls on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */

void APP_InitializeFirewall(
        Node* firstNode,
        const NodeInput* nodeInput)
{
    NodeInput firewallInput;
    BOOL wasFound;

    IO_ReadCachedFile(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "FIREWALL-CONFIG-FILE",
            &wasFound,
            &firewallInput);

    if (!wasFound) {
        return;
    }

    for (int line = 0; line < firewallInput.numLines; line++)
    {
        int nodeId;
        Node* node;
        int numValues;

        numValues = sscanf(firewallInput.inputStrings[line], "%*s %d", &nodeId);

        if (numValues != 1) {
            continue;
        }

        // Get node on this partition only
        BOOL success = PARTITION_ReturnNodePointer(
                firstNode->partitionData,
                &node,
                nodeId,
                FALSE);

        if (!success) {
            continue;
        }

        if (!node->firewallModel) {
            node->firewallModel = new FirewallModel(node);
            node->firewallModel->turnFirewallOn();
        }

        node->firewallModel->processCommand(firewallInput.inputStrings[line]);
    }
}


#endif

void APP_InitializeGuiHitl(Node* node, const NodeInput* nodeInput)
{
    if (node->partitionData->partitionId != 0)
        return;
    NodeInput hitlInput;
    BOOL wasFound;

     IO_ReadCachedFile(
            ANY_NODEID,
            ANY_ADDRESS,
            nodeInput,
            "HITL-CONFIG-FILE",
            &wasFound,
            &hitlInput);

    if (!wasFound) {
        return;
    }

    for (int line = 0; line < hitlInput.numLines; line++)
    {
        char *timeStr = NULL;
        char *cmd = NULL;
        char iotoken[MAX_STRING_LENGTH];
        Message *timerMsg = NULL;
        timeStr = IO_GetDelimitedToken(iotoken, hitlInput.inputStrings[line], " ", &cmd);

        if (timeStr == NULL || cmd == NULL) {
            continue;
        }
        clocktype eventTime = TIME_ConvertToClock(timeStr);
        timerMsg = MESSAGE_Alloc(node,
            APP_LAYER,
            APP_GUI_HITL,
            MSG_Gui_Hitl_Event);

        char *command = MESSAGE_InfoAlloc(node, timerMsg, (int)(strlen(cmd+1) + 1));
        memcpy (command, cmd + 1, strlen(cmd+1) + 1);

        MESSAGE_Send(node,
                  timerMsg,
                  eventTime - node->getNodeTime());
    }
}

/*
 * NAME:        APP_Initialize.
 * PURPOSE:     start applications on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void
APP_Initialize(Node *node, const NodeInput *nodeInput)
{
    BOOL retVal = FALSE;
    BOOL readValue = FALSE;
    char buf[MAX_STRING_LENGTH];
    int i;
    AppMultimedia* multimedia = NULL;
    NetworkDataIp* ip = (NetworkDataIp*) node->networkData.networkVar;
#ifdef MILITARY_RADIOS_LIB
    APPLink16GatewayInit(node, nodeInput);
#endif

    /* Initialize application layer information.
       Note that the starting ephemeral port number is 1024. */

    node->appData.nextPortNum = 1024;
    node->appData.numAppTcpFailure = 0;

    // Initialize app map
    node->appData.appMap = new AppMap;
    // initialize finalize pointer in application

#ifdef ENTERPRISE_LIB
    // first check if the transport layer protocol for SIP is UDP
    IO_ReadString(node->nodeId,
              ANY_ADDRESS,
              nodeInput,
              "SIP-TRANSPORT-LAYER-PROTOCOL",
              &retVal,
              buf);

    if (retVal == TRUE)
    {
        if (!strcmp(buf, "UDP"))
        {

            // invalid entry in config
            char errBuf[MAX_STRING_LENGTH];
            sprintf(errBuf,
             " %s protocol is not supported in the current version!\n", buf);
            ERROR_ReportError(errBuf);
        }
    }
#endif // ENTERPRISE_LIB

    // Pseudo traffic sender layer
    node->appData.appTrafficSender = new AppTrafficSender();
    // DNS Initialization start
    // if DNS is enabled on any of the interface
    BOOL isDnsEnabled = FALSE;
    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadBool(
                node->nodeId,
                NetworkIpGetInterfaceAddress(node, i),
                nodeInput,
                "DNS-ENABLED",
                &retVal,
                &isDnsEnabled);
        if (retVal == TRUE && isDnsEnabled)
        {
            break;
        }
    }

    if (retVal == TRUE && isDnsEnabled)
    {
        AppDnsConfigurationParametersInit(node, nodeInput);
    }

    IO_ReadString(
                node->nodeId,
                ANY_ADDRESS,
                nodeInput,
                "MULTIMEDIA-SIGNALLING-PROTOCOL",
                &retVal,
                buf);

    if (retVal)
    {
#ifdef ENTERPRISE_LIB
        multimedia = (AppMultimedia*) MEM_malloc(sizeof(AppMultimedia));
        memset(multimedia, 0, sizeof(AppMultimedia));

        if (!strcmp(buf, "H323"))
        {
            multimedia->sigType = SIG_TYPE_H323;
        }
        else if (!strcmp(buf, "SIP"))
        {
            multimedia->sigType = SIG_TYPE_SIP;
        }
        else
        {
            ERROR_ReportError("Invalid Multimedia Signalling protocol\n");
        }
#else // ENTERPRISE_LIB
        ERROR_ReportMissingLibrary(buf, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
    }

    node->appData.multimedia = multimedia;

    #ifdef ENTERPRISE_LIB
        //fix to read signalling parameters by all nodes
        Multimedia_Initialize(node,nodeInput);
    #endif // ENTERPRISE_LIB


    node->appData.mdp = NULL;

#ifdef NETCONF_INTERFACE
    node->appData.ncAgent = NULL;
#endif
    node->appData.rtpData = NULL;

    // initialize and allocate memory to the port table
    node->appData.portTable = (PortInfo*) MEM_malloc(
        sizeof(PortInfo) * PORT_TABLE_HASH_SIZE);
    memset(node->appData.portTable,
       0,
       sizeof(PortInfo) * PORT_TABLE_HASH_SIZE);

    /* Check if statistics needs to be printed. */

    IO_ReadBool(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "APPLICATION-STATISTICS",
        &retVal,
        &readValue);

    if (retVal == FALSE || !readValue)
    {
        node->appData.appStats = FALSE;
    }
    else
    if (readValue)
    {
        node->appData.appStats = TRUE;
    }
    else
    {
        ERROR_ReportError("Expecting YES or NO for APPLICATION-STATISTICS "
                          "parameter\n");
    }

    APP_TraceInitialize(node, nodeInput);

#ifdef ADDON_DB
    if (!node->appData.appStats)
    {
        if (node->partitionData->statsDb)
        {
            if (node->partitionData->statsDb->statsAggregateTable->createAppAggregateTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB APPLICATION_Aggregate table requires\n"
                    " APPLICATION-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsSummaryTable->createAppSummaryTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB APPLICATION_Summary table requires\n"
                    " APPLICATION-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsSummaryTable->createMulticastAppSummaryTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB MULTICAST_APPLICATION_Summary table\n"
                    " requires APPLICATION-LAYER-STATISTICS to be set to YES\n");
            }
            if (node->partitionData->statsDb->statsEventsTable->createAppEventsTable)
            {
                ERROR_ReportError(
                    "Invalid Configuration settings: Use of StatsDB APPLICATION_Events table requires\n"
                    " APPLICATION-LAYER-STATISTICS to be set to YES\n");
            }
        }
    }
#endif

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ROUTING-STATISTICS",
        &retVal,
        buf);

    if (retVal == FALSE || strcmp(buf, "NO") == 0)
    {
        node->appData.routingStats = FALSE;
    }
    else
    if (strcmp(buf, "YES") == 0)
    {
        node->appData.routingStats = TRUE;
    }
    else
    {
        ERROR_ReportError("Expecting YES or NO for ROUTING-STATISTICS "
                          "parameter\n");
    }

#ifdef MILITARY_UNRESTRICTED_LIB
    node->appData.triggerThreadData = NULL;
    ThreadedAppTriggerInit(node);
#endif /* MILITARY_UNRESTRICTED_LIB */

    node->appData.triggerAppData = NULL;
    SuperAppTriggerInit(node);

    node->appData.clientPtrList = NULL;
    SuperAppClientListInit(node);

    // Process static routes
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "STATIC-ROUTE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RoutingStaticInit(
                node,
                nodeInput,
                ROUTING_PROTOCOL_STATIC);
        }
    }

    // Process default routes
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "DEFAULT-ROUTE",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RoutingStaticInit(
                node,
                nodeInput,
                ROUTING_PROTOCOL_DEFAULT);
        }
    }

    node->appData.hsrp = NULL;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadString(
                    node->nodeId,
                    NetworkIpGetInterfaceAddress(node, i),
                    nodeInput,
                    "HSRP-PROTOCOL",
                    &retVal,
                    buf);

        if (retVal == FALSE)
        {
            continue;
        }
        else
        {
            if (strcmp(buf, "YES") == 0)
            {
#ifdef ENTERPRISE_LIB
                RoutingHsrpInit(node, nodeInput, i);
#else // ENTERPRISE_LIB
                ERROR_ReportMissingLibrary("HSRP", "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
            }
        }
    }

#ifdef CYBER_CORE
    node->appData.isakmpEnabledInterface =
        (unsigned char*) MEM_malloc(node->numberInterfaces);
    memset(node->appData.isakmpEnabledInterface, 0, node->numberInterfaces);
    node->appData.isakmpData = NULL;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "ISAKMP-SERVER",
        &retVal,
        buf);

    if (retVal && !strcmp(buf, "YES"))
    {
        ISAKMPInit(node, nodeInput);
    }
    else if (retVal && strcmp(buf, "NO"))
    {
        ERROR_ReportError("ISAKMP-SERVER expects YES or NO");
    }

#endif // CYBER_CORE

    for (i = 0; i < node->numberInterfaces; i++)
    {
        NetworkType InterfaceType = NetworkIpGetInterfaceType(node, i);

        if (InterfaceType == NETWORK_IPV4 || InterfaceType == NETWORK_DUAL)
        {
            switch (ip->interfaceInfo[i]->routingProtocolType)
            {
                case ROUTING_PROTOCOL_BELLMANFORD:
                {
                    if (node->appData.bellmanford == NULL)
                    {
                        RoutingBellmanfordInit(node);
                        RoutingBellmanfordInitTrace(node, nodeInput);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_RIP:
                {
                    RipInit(node, nodeInput, i);
                    break;
                }
#ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                {
                    if (node->appData.routingVar == NULL)
                    {
                        RoutingFisheyeInit(node, nodeInput);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    if (node->appData.olsr == NULL)
                    {
                      RoutingOlsrInriaInit(node, nodeInput, i, NETWORK_IPV4);
                    }
                    else
                    {
                       if (((RoutingOlsr*)node->appData.olsr)->ip_version ==
                           NETWORK_IPV6)
                       {
                         ERROR_ReportError("Both IPV4 and IPv6 instance of"
                          "OLSR-INRIA can not run simultaneously on a node");
                       }

                     ((RoutingOlsr*)node->appData.olsr)->numOlsrInterfaces++;

                      NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction(
                                node,
                                ROUTING_PROTOCOL_OLSR_INRIA,
                                i,
                                NETWORK_IPV4);
                    }
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    if (node->appData.olsrv2 == NULL)
                    {
                        RoutingOLSRv2_Niigata_Init(node,
                                                   nodeInput,
                                                   i,
                                                   NETWORK_IPV4);
                    }
                    else
                    {
                        RoutingOLSRv2_Niigata_AddInterface(node,
                                                           nodeInput,
                                                           i,
                                                           NETWORK_IPV4);
                    }
                    break;
                }
#else // WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                case ROUTING_PROTOCOL_OLSR_INRIA:
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    ERROR_ReportMissingLibrary(buf, "Wireless");
                }
#endif // WIRELESS_LIB
                //InsertPatch APP_ROUTING_INIT_CODE

                default:
                {
                    break;
                }
            }
        }

        if (InterfaceType == NETWORK_IPV6 || InterfaceType == NETWORK_DUAL)
        {
            switch (ip->interfaceInfo[i]->ipv6InterfaceInfo->
                    routingProtocolType)
            {
                case ROUTING_PROTOCOL_RIPNG:
                {
                    RIPngInit(node, nodeInput, i);
                    break;
                }
#ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                   if (node->appData.olsr == NULL)
                   {
                      RoutingOlsrInriaInit(node, nodeInput, i, NETWORK_IPV6);
                   }
                   else
                   {
                       if (((RoutingOlsr*)node->appData.olsr)->ip_version ==
                            NETWORK_IPV4)
                       {
                         ERROR_ReportError("Both IPV4 and IPv6 instance of"
                          "OLSR-INRIA can not run simultaneously on a node");
                       }

                    ((RoutingOlsr* )node->appData.olsr)->numOlsrInterfaces++;

                      NetworkIpUpdateUnicastRoutingProtocolAndRouterFunction(
                                node,
                                ROUTING_PROTOCOL_OLSR_INRIA,
                                i,
                                NETWORK_IPV6);
                   }
                   break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    if (node->appData.olsrv2 == NULL)
                    {
                        RoutingOLSRv2_Niigata_Init(node,
                                                   nodeInput,
                                                   i,
                                                   NETWORK_IPV6);
                    }
                    else
                    {
                        RoutingOLSRv2_Niigata_AddInterface(node,
                                                           nodeInput,
                                                           i,
                                                           NETWORK_IPV6);
                    }
                    break;
                }
#else // WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    ERROR_ReportMissingLibrary(buf, "Wireless");
                }
#endif // WIRELESS_LIB
        //InsertPatch APP_ROUTING_INIT_CODE

                default:
                {
                    break;
                }
            }
        }
    }

    node->appData.uniqueId = 0;

    /* Setting up Border Gateway Protocol */
    node->appData.exteriorGatewayVar = NULL;

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "EXTERIOR-GATEWAY-PROTOCOL",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "BGPv4") == 0)
        {
#ifdef ENTERPRISE_LIB
            node->appData.exteriorGatewayProtocol =
                APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4;

            BgpInit(node, nodeInput);
#else // ENTERPRISE_LIB
            ERROR_ReportMissingLibrary("BGP", "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
        }
        else
        {
            // There is no other option now
            printf("The border gateway protocol should be BGPv4\n");
            assert(FALSE);
        }
    }

    /* Initialize Label Distribution Protocol, if necessary */
    for (i = 0; i < node->numberInterfaces; i++)
    {
        IO_ReadString(
            node->nodeId,
            NetworkIpGetInterfaceAddress(node, i),
            nodeInput,
            "MPLS-LABEL-DISTRIBUTION-PROTOCOL",
            &retVal,
            buf);

        if (retVal)
        {
#ifdef ENTERPRISE_LIB
            if (strcmp(buf, "LDP") == 0)
            {
                // This has been added for IP-MPLS Integration to support
                // Dual IP Nodes.Initialize LDP only for IPv4 interfaces.
                if ((NetworkIpGetInterfaceType(node, i) ==
                        NETWORK_IPV4) || (NetworkIpGetInterfaceType(node, i) ==
                        NETWORK_DUAL))
                {
                    MplsLdpInit(node, nodeInput,
                            NetworkIpGetInterfaceAddress(node, i));
                }
            }
            else if (strcmp(buf, "RSVP-TE") == 0)
            {
                if (node->transportData.rsvpProtocol)
                {
                    // initialize RSVP/RSVP-TE variable and
                    // go out of the for loop
                    RsvpInit(node, nodeInput);
                    break;
                }
                else
                {
                    // ERROR : some-one has tried to initialize
                    // RSVP-TE witout Setting TRANSPORT-PROTOCOL-RSVP
                    // to YES in the cofiguration file.
                    ERROR_Assert(FALSE, "RSVP PROTOCOL IS OFF");
                }
            }
            else
            {
                ERROR_ReportErrorArgs("Unknown "
                                      "MPLS-LABEL-DISTRIBUTION-PROTOCOL: "
                                      "%s\n", buf);
            }
#else // ENTERPRISE_LIB
            ERROR_ReportMissingLibrary(buf, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
        } // end if (retVal)
    }// end for (i = 0; i < node->numberInterfaces; i++)

    if (!node->transportData.rsvpVariable)
    {
        // If either transport layer or RSVP-TE had not initialized
        // RSVP protocol, then set node->transportData.rsvpProtocol
        // to FALSE. This patch is needed because RSVP-TE is
        // implemented without implemeting RSVP. When RSVP will be
        // fully implemeted this patch will be removed.
        node->transportData.rsvpProtocol = FALSE;
    }

#ifdef ENTERPRISE_LIB
    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "RTP-ENABLED",
        &retVal,
        buf);

    if (retVal)
    {
        if (strcmp(buf, "YES") == 0)
        {
            RtpInit(node, nodeInput);
        }
        else if (strcmp(buf, "NO") == 0){}
        else
        {
             ERROR_ReportError("RTP-ENABLED should be "
                               "set to \"YES\" or \"NO\".\n");
        }
    }
#endif // ENTERPRISE_LIB

    // Initialize multicast group membership if required.
    APP_InitMulticastGroupMembershipIfAny(node, nodeInput);

    IO_ReadString(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MDP-ENABLED",
        &retVal,
        buf);

    if (retVal == TRUE)
    {
        if (strcmp(buf, "YES") == 0)
        {
            MdpInit(node,nodeInput);
        }
        else if (strcmp(buf, "NO") == 0){}
        else
        {
             ERROR_ReportError("MDP should be "
                               "set to \"YES\" or \"NO\".\n");
        }
    }

    //MSDP Initialization

    IO_ReadBool(
        node->nodeId,
        ANY_ADDRESS,
        nodeInput,
        "MULTICAST-MSDP-ENABLED",
        &retVal,
        &node->isMsdpEnabled);

    if (retVal)
    {
        if (node->isMsdpEnabled)
        {
            MsdpInit(node, nodeInput);
        }
    }

    //AppLayerInitUserApplications(
    //   node, nodeInput, &node->appData.userApplicationData);
#ifdef ADDON_HELLO
    AppHelloInit(node, nodeInput);
#endif /* ADDON_HELLO */
}


/*
 * NAME:        APP_ForbidSameSourceAndDest
 * PURPOSE:     Checks whether source node address and destination
 *              node address is same or not and if same it
 *              prints some warning messages and returns.
 * PARAMETERS:
 * RETURN:      none.
 */
static
BOOL APP_ForbidSameSourceAndDest(
    const char* inputString,
    const char* appName,
    NodeAddress sourceNodeId,
    NodeAddress destNodeId)
{
    if (sourceNodeId == destNodeId)
    {
#ifdef MILITARY_UNRESTRICTED_LIB
        char errorStr[INPUT_LENGTH] = {0};
#else
        char errorStr[5*MAX_STRING_LENGTH] = {0};
#endif /* MILITARY_UNRESTRICTED_LIB */

        sprintf(errorStr, "%s : Application source and destination"
                " nodes are identical!\n %s\n",
                appName, inputString);

        ERROR_ReportWarning(errorStr);

        return TRUE;
    }
    return FALSE;
}

#ifdef EXATA
/*
 * NAME:        APP_InitializeEmulatedApplications.
 * PURPOSE:     start emulated applications on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void
APP_InitializeEmulatedApplications(
    Node *firstNode,
    const NodeInput *nodeInput)
{
    Node* node = NULL;
    IdToNodePtrMap* nodeHash;
    nodeHash = firstNode->partitionData->nodeIdHash;

    if (firstNode->partitionData->isEmulationMode ||
        firstNode->partitionData->rrInterface->GetReplayMode())
    {

        for (node = firstNode; node; node = node->nextNodeData)
        {
#ifdef NETSNMP_INTERFACE
            AppNetSnmpAgentInit(node, nodeInput);
#endif

            char baseDir[MAX_STRING_LENGTH] = {0};
            BOOL wasFound;
            int interfaceIndex;
            std::string fullPath;
            std::string home;
            BOOL rootDirSet = FALSE;

            /********************************************
             *   Emulated FTP Server Application        */
            IO_ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "FTP-ROOT-DIRECTORY",
                    &wasFound,
                    baseDir);

            if (wasFound) //if File Access is enabled
            {
                // determine base directory for FTP server

                std::string envVar = std::string("$") + Product::GetHomeVariable();

                // is it using $XXX_HOME environment variable?
                if (!envVar.compare(0, envVar.size(), baseDir, 0, envVar.size())
                    && Product::GetProductHome(home))
                {
                    fullPath = baseDir;
                    fullPath.replace(0, envVar.length(), home);
                }
                else
                {
                    fullPath = baseDir;
                }

                struct stat fileStat;
                rootDirSet = TRUE;
                if (stat(fullPath.c_str(), &fileStat) < 0)
                {
                    std::string errStr =
                        std::string("FTP root directory: ") + baseDir +
                        std::string(" does not exist. Using the default "
                                    "directory instead.");
                    ERROR_ReportWarning(errStr.c_str());
                    fullPath = "";
                    rootDirSet = FALSE;
                }
            }

            for (interfaceIndex = 0;
                interfaceIndex < node->numberInterfaces;
                interfaceIndex++)
            {
                NetworkType networkType
                    = MAPPING_GetNetworkTypeFromInterface(
                        node, interfaceIndex);
                if (networkType == NETWORK_IPV4
                    || networkType == NETWORK_IPV6)
                {
                    AppEFtpServerInit(
                            node,
                            MAPPING_GetInterfaceAddressForInterface(
                                node,
                                node->nodeId,
                                networkType,
                                interfaceIndex),
                            rootDirSet,
                            fullPath.c_str());
                }
                else if (networkType == NETWORK_DUAL)
                {
                    AppEFtpServerInit(
                            node,
                            MAPPING_GetInterfaceAddressForInterface(
                                node,
                                node->nodeId,
                                NETWORK_IPV4,
                                interfaceIndex),
                            rootDirSet,
                            fullPath.c_str());

                    AppEFtpServerInit(
                            node,
                            MAPPING_GetInterfaceAddressForInterface(
                                node,
                                node->nodeId,
                                NETWORK_IPV6,
                                interfaceIndex),
                            rootDirSet,
                            fullPath.c_str());
                }
            }
            /********************************************
             *   Emulated HTTP Server Application       */

            IO_ReadString(
                    ANY_NODEID,
                    ANY_ADDRESS,
                    nodeInput,
                    "HTTP-ROOT-DIRECTORY",
                    &wasFound,
                    baseDir);

            // determine base directory for HTTP server
            rootDirSet = FALSE;
            if (wasFound)
            {
                std::string envVar = std::string("$") + Product::GetHomeVariable();
                // is it using $XXX_HOME environment variable?
                if (!envVar.compare(0, envVar.size(), baseDir, 0, envVar.size())
                    && Product::GetProductHome(home))
                {
                    fullPath = baseDir;
                    fullPath.replace(0, envVar.length(), home);
                }

                struct stat fileStat;
                rootDirSet = TRUE;
                if (stat(fullPath.c_str(), &fileStat) < 0)
                {
                    std::string errStr =
                        std::string("Http Root Directory: ") + baseDir +
                        std::string(" doesn't exists, Hence setting default value.");
                    ERROR_ReportWarning(errStr.c_str());
                    rootDirSet = FALSE;
                }
            }
            if (!rootDirSet)
            {
                if (Product::GetProductHome(home))
                {
                    fullPath =  home + "/data/webfiles/";
                }
            }

            for (interfaceIndex = 0;
                interfaceIndex < node->numberInterfaces;
                interfaceIndex++)
            {
                NetworkType networkType
                    = MAPPING_GetNetworkTypeFromInterface(
                        node, interfaceIndex);
                if (networkType == NETWORK_IPV4
                    || networkType == NETWORK_IPV6)
                {
                    AppEHttpServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            networkType,
                            interfaceIndex),
                        fullPath.c_str());
                }
                else if (networkType == NETWORK_DUAL)
                {
                    AppEHttpServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            NETWORK_IPV4,
                            interfaceIndex),
                        fullPath.c_str());

                    AppEHttpServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            NETWORK_IPV6,
                            interfaceIndex),
                        fullPath.c_str());
                }
            }
            /********************************************
             *   Emulated Telnet Server Application     */

            for (interfaceIndex = 0;
                interfaceIndex < node->numberInterfaces;
                interfaceIndex++)
            {
                NetworkType networkType
                    = MAPPING_GetNetworkTypeFromInterface(
                        node, interfaceIndex);
                if (networkType == NETWORK_IPV4
                    || networkType == NETWORK_IPV6)
                {
                    AppETelnetServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            networkType,
                            interfaceIndex));
                }
                else if (networkType == NETWORK_DUAL)
                {
                    AppETelnetServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            NETWORK_IPV4,
                            interfaceIndex));

                    AppETelnetServerInit(
                        node,
                        MAPPING_GetInterfaceAddressForInterface(
                            node,
                            node->nodeId,
                            NETWORK_IPV6,
                            interfaceIndex));
                }
            }
#ifdef NETCONF_INTERFACE
            BOOL retVal = FALSE;
            BOOL readValue = FALSE;
            IO_ReadBool(node->nodeId,
                        ANY_ADDRESS,
                        nodeInput,
                        "NETCONF-ENABLED",
                        &retVal,
                        &readValue);

            if (retVal && readValue)
            {
                AppNetconfServerInit(node, nodeInput);
            }
#endif
        }
    }
}
#endif

/*
 * NAME:        APP_InitializeApplications.
 * PURPOSE:     start applications on nodes according to user's
 *              specification.
 * PARAMETERS:  node - pointer to the node,
 *              nodeInput - configuration information.
 * RETURN:      none.
 */
void
APP_InitializeApplications(
    Node *firstNode,
    const NodeInput *nodeInput)
{
    NodeInput appInput;
    char appStr[MAX_STRING_LENGTH];
    std::string name;

    BOOL retVal;
    int  i;
    int  numValues;
    Node* node = NULL;
    IdToNodePtrMap* nodeHash;
    // dns
    bool isUrl = FALSE;

    BOOL gatekeeperFound = FALSE;
#ifdef ENTERPRISE_LIB
    H323CallModel callModel = Direct;
#endif // ENTERPRISE_LIB

#ifdef CELLULAR_LIB
    BOOL cellularAbstractApp = FALSE;
    CellularAbstractApplicationLayerData *appCellular;

    // initilize all the node's application data info
    // This segment code is moved from inside to here
    // to handle the case when no CELLULAR-ABSTARCT-APP is defined
    // and USER model is  not used, we can still power on the MS
    if (cellularAbstractApp == FALSE)
    {
        Node* nextNodePt  = NULL;

        nextNodePt=firstNode->partitionData->firstNode;

        // network preinit already before application initilization
        while (nextNodePt != NULL)
        {
            //check to see if we've initialized
            //if not, then do so
            appCellular =
                (CellularAbstractApplicationLayerData *)
                    nextNodePt->appData.appCellularAbstractVar;

            if (nextNodePt->networkData.cellularLayer3Var &&
                nextNodePt->networkData.cellularLayer3Var->cellularAbstractLayer3Data &&
                appCellular == NULL)
            {
                // USER layer initialized before application layer
                // if network is cellular model and has not been
                // initialized by USER layer
                // then initialize the cellular application
                CellularAbstractAppInit(nextNodePt, nodeInput);
            }
            nextNodePt = nextNodePt->nextNodeData;
        }
        cellularAbstractApp = TRUE;
    }
#endif // CELLULAR_LIB

#if defined(UMTS_LIB)
    BOOL umtsCallApp = FALSE;
#endif

    if (firstNode == NULL)
        return; // this partition has no nodes.

    nodeHash = firstNode->partitionData->nodeIdHash;

    // Initialize zone list for DNS
    DnsZoneListInitialization(firstNode);

#ifdef EXATA
    APP_InitializeEmulatedApplications(firstNode, nodeInput);
#endif


    IO_ReadCachedFile(
        ANY_NODEID,
        ANY_ADDRESS,
        nodeInput,
        "APP-CONFIG-FILE",
        &retVal,
        &appInput);
    // initialize appinput.numMaxLines to 0
    appInput.maxNumLines = 0;

    // If there is no APP-CONFIG-FILE then proceed and look for
    // other application layer files.If APP-CONFIG-FILE present then parse
    // it and initialize the application and proceed further for other
    // application layer files.

    if (retVal == TRUE)
    {
        for (i = 0; i < appInput.numLines; i++)
        {
            sscanf(appInput.inputStrings[i], "%s", appStr);
            name = std::string(appStr);
            transform(name.begin(), name.end(), name.begin(), ::toupper);

            if (firstNode->networkData.networkProtocol == IPV6_ONLY)
            {
                if (strcmp(appStr, "CBR") != 0
                    && strcmp(appStr, "FTP/GENERIC") != 0
                    && strcmp(appStr, "FTP") != 0
                    && strcmp(appStr, "TELNET") != 0
                    && strcmp(appStr, "HTTP") != 0
                    && strcmp(appStr, "HTTPD") != 0
                    && strcmp(appStr, "MCBR") !=0
                    && strcmp(appStr, "TRAFFIC-GEN") !=0
                    && strcmp(appStr, "JAMMER") !=0)
                {
                    ERROR_ReportErrorArgs(
                        "%s is not supported for IPv6 based Network.\n"
                        "Only CBR, FTP/GENERIC, FTP, TELNET, HTTP, MCBR,"
                        "JAMMER and TRAFFIC-GEN \napplications "
                        "are currently supported.\n", appStr);
                }
            }

            // parse application name first
            char appName[MAX_STRING_LENGTH];
            char* appNamePtr = strstr(appInput.inputStrings[i],
                                    "APPLICATION-NAME");
            // saving pointer location
            char* appNamePtrLoc = appNamePtr;
            std::string appNameTempStore;
            size_t appNameLen = 0;

            if (appNamePtr)
            {
                appNameLen = strlen(appNamePtr);

                // saving the APPLICATION-NAME for restore
                appNameTempStore.assign(appNamePtr, appNameLen);
            }

            appName[0] = '\0';
            if (appNamePtr != NULL)
            {
                char* valuePtr1 = NULL;
                char* valuePtr2 = NULL;

                // skip the key "APPLICATION-NAME"
                valuePtr1 = appNamePtr + strlen("APPLICATION-NAME");

                // skip any spaces
                while (*valuePtr1 == ' ' && *valuePtr1 != '\0')
                {
                    valuePtr1 ++;
                }

                if (*valuePtr1 != '\0')
                {
                    // find the end of the value
                    valuePtr2 = valuePtr1;
                    while (*valuePtr2 != ' ' && *valuePtr2 != '\0')
                    {
                        valuePtr2 ++;
                    }

                    // copy the configured application name
                    memcpy(appName, valuePtr1, valuePtr2 - valuePtr1);
                    appName[valuePtr2 - valuePtr1] = '\0';

                    if ((strcmp(appStr, "SUPER-APPLICATION") != 0)
                        && (strcmp(appStr, "THREADED-APP") != 0))
                    {
                        while (appNamePtr != valuePtr2)
                        {
                            *appNamePtr = ' ';
                            appNamePtr ++;
                        }
                    }

                    // set appNamePtr to point the name
                    appNamePtr = &(appName[0]);
                }
                else
                {
                    // case when no value is provided with "APPLICATION-NAME"
                    if ((strcmp(appStr, "SUPER-APPLICATION") != 0)
                        && (strcmp(appStr, "THREADED-APP") != 0))
                    {
                        while (appNamePtr != valuePtr1)
                        {
                            *appNamePtr = ' ';
                            appNamePtr ++;
                        }
                    }

                    // set appNamePtr to NULL
                    appNamePtr = NULL;
                }
            }

#ifdef DEBUG
            printf("Parsing for application type %s\n", appStr);
#endif /* DEBUG */

            if (strcmp(appStr, "FTP") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                char destString[BIG_STRING_LENGTH];
                int itemsToSend;
                char startTimeStr[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %s",
                                sourceString,
                                destString,
                                &itemsToSend,
                                startTimeStr);

                if (numValues != 4)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong FTP configuration format!\n"
                            "FTP <src> <dest> <items to send> <start time>\n");
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];

                    printf("Starting FTP client with:\n");
                    printf("  src nodeId:    %u\n", sourceNodeId);
                    printf("  dst nodeId:    %u\n", destNodeId);
                    IO_ConvertIpAddressToString(&destAddr, addrStr);
                    printf("  dst address:   %s\n", addrStr);
                    printf("  items to send: %d\n", itemsToSend);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
#endif /* DEBUG */
                    // checks if destination is configured as url and find
                    // url destination address such that application server
                    // can be initialized
                    Address urlDestAddress;
                    isUrl = node->appData.appTrafficSender->
                            appCheckUrlAndGetUrlDestinationAddress(
                                        node,
                                        appInput.inputStrings[i],
                                        FALSE,
                                        &sourceAddr,
                                        &destAddr,
                                        &destNodeId,
                                        &urlDestAddress,
                                        &AppFtpUrlSessionStartCallBack);

                    AppFtpClientInit(node,
                                    sourceAddr,
                                    destAddr,
                                    itemsToSend,
                                    startTime,
                                    appNamePtr,
                                    destString);

                    // dns
                    if (isUrl)
                    {
                        memcpy(&destAddr, &urlDestAddress, sizeof(Address));
                    }
                    if (destAddr.networkType != NETWORK_INVALID &&
                        sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                            "FTP: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);
                    }
                }

                if (!isUrl && ADDR_IsUrl(destString) == TRUE)
                {
                    AppUpdateUrlServerNodeIdAndAddress(
                                                    firstNode,
                                                    destString,
                                                    &destAddr,
                                                    &destNodeId);
                }
                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr,
                                        destNodeId,
                                        sourceAddr,
                                        sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    // Dynamic address
                    // Delay FTP server listening by start time
                    // such that server may acquire an address by that time
                    // to ensure that server listens earlier than TCP
                    // open; therefore listen timer should be set at unit
                    // time (1 ns) before Open. If start time is less than
                    // 1 nano second(0S) then listenTime remains as 0S which
                    // is the least possible simulation time.
                    clocktype listenTime = 0;
                    if (TIME_ConvertToClock(startTimeStr) > 1 * NANO_SECOND)
                    {
                        listenTime =
                            TIME_ConvertToClock(startTimeStr) - 1 * NANO_SECOND;
                    }
                    AppStartTcpAppSessionServerListenTimer(
                                                        node,
                                                        APP_FTP_SERVER,
                                                        destAddr,
                                                        destString,
                                                        listenTime);
                }
            }
            else
            if (strcmp(appStr, "FTP/GENERIC") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                char destString[BIG_STRING_LENGTH];
                int itemsToSend;
                int itemSize;
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                char tosStr[MAX_STRING_LENGTH];
                char tosValueStr[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                unsigned tos = APP_DEFAULT_TOS;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %d %s %s %s %s",
                                sourceString,
                                destString,
                                &itemsToSend,
                                &itemSize,
                                startTimeStr,
                                endTimeStr,
                                tosStr,
                                tosValueStr);

                switch (numValues)
                {
                    case 6:
                        break;
                    case 8 :
                        if (APP_AssignTos(tosStr, tosValueStr, &tos))
                        {
                            break;
                        } // else fall through default
                    default:
                    {
                        char errorString[MAX_STRING_LENGTH];
                        sprintf(errorString,
                                "Wrong FTP/GENERIC configuration format!\n"
                                "FTP/GENERIC <src> <dest> <items to send> "
                                "<item size> <start time> <end time> "
                                "[TOS <tos-value> | DSCP <dscp-value>"
                                " | PRECEDENCE <precedence-value>]\n");
                        ERROR_ReportError(errorString);
                    }
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
                    clocktype endTime = TIME_ConvertToClock(endTimeStr);

#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];

                    printf("Starting FTP client with:\n");
                    printf("  src nodeId:    %u\n", sourceNodeId);
                    printf("  dst nodeId:    %u\n", destNodeId);
                    IO_ConvertIpAddressToString(&destAddr, addrStr);
                    printf("  dst address:   %s\n", addrStr);
                    printf("  items to send: %d\n", itemsToSend);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
                    ctoa(endTime, clockStr);
                    printf("  end time:      %s\n", clockStr);
                    printf("  tos:           %d\n", tos);
#endif /* DEBUG */

                    // checks if destination is configured as url and find
                    // url destination address such that application server
                    // can be initialized
                    Address urlDestAddress;
                    isUrl = node->appData.appTrafficSender->
                            appCheckUrlAndGetUrlDestinationAddress(
                                        node,
                                        appInput.inputStrings[i],
                                        FALSE,
                                        &sourceAddr,
                                        &destAddr,
                                        &destNodeId,
                                        &urlDestAddress,
                                        &AppGenFtpUrlSessionStartCallBack);

                    AppGenFtpClientInit(
                        node,
                        sourceAddr,
                        destAddr,
                        itemsToSend,
                        itemSize,
                        startTime,
                        endTime,
                        tos,
                        appNamePtr,
                        destString);
                    // dns
                    if (isUrl)
                    {
                        memcpy(&destAddr, &urlDestAddress, sizeof(Address));
                    }
                    if (destAddr.networkType != NETWORK_INVALID &&
                        sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                            "FTP: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);
                    }
                }

                if (!isUrl && ADDR_IsUrl(destString) == TRUE)
                {
                    AppUpdateUrlServerNodeIdAndAddress(
                                                    firstNode,
                                                    destString,
                                                    &destAddr,
                                                    &destNodeId);
                }

                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr,
                                        destNodeId,
                                        sourceAddr,
                                        sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    // Dynamic address
                    // Delay FTP-GEN server listening by start time
                    // such that server may acquire an address by that time
                    // to ensure that server listens earlier than TCP
                    // open; therefore listen timer should be set at unit
                    // time (1 ns) before Open.If start time is less than
                    // 1 nano second(0S) then listenTime remains as 0S which
                    // is the least possible simulation time.
                    clocktype listenTime = 0;
                    if (TIME_ConvertToClock(startTimeStr) > 1 * NANO_SECOND)
                    {
                        listenTime =
                            TIME_ConvertToClock(startTimeStr) - 1 * NANO_SECOND;
                    }
                    AppStartTcpAppSessionServerListenTimer(
                                                        node,
                                                        APP_GEN_FTP_SERVER,
                                                        destAddr,
                                                        destString,
                                                        listenTime);
                }
            }
            else if (strcmp(appStr, "TELNET") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                char destString[BIG_STRING_LENGTH];
                char sessDurationStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s",
                                sourceString,
                                destString,
                                sessDurationStr,
                                startTimeStr);

                if (numValues != 4)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong TELNET configuration format!\nTELNET"
                            " <src> <dest> <session duration> <start time>\n");
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime;
                    clocktype sessDuration;

#ifdef DEBUG
                    startTime = TIME_ConvertToClock(startTimeStr);
                    startTime += getSimStartTime(firstNode);
                    char startTimeStr2[MAX_STRING_LENGTH];
                    TIME_PrintClockInSecond(startTime, startTimeStr2);

                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting TELNET client with:\n");
                    printf("  src nodeId:       %u\n", sourceNodeId);
                    printf("  dst nodeId:       %u\n", destNodeId);
                    printf("  session duration: %s\n", sessDurationStr);
                    printf("  start time:       %s\n", startTimeStr2);
#endif /* DEBUG */

                    startTime    = TIME_ConvertToClock(startTimeStr);

#ifdef DEBUG
                    {
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];
                    printf("Starting TELNET client with:\n");
                    printf("  src nodeId:       %u\n", sourceNodeId);
                    printf("  dst nodeId:       %u\n", destNodeId);
                    IO_ConvertIpAddressToString(&destAddr, addrStr);
                    printf("  dst address:      %s\n", addrStr);
                    ctoa(sessDuration, clockStr);
                    printf("  session duration: %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:       %s\n", clockStr);
                    }
#endif /* DEBUG */

                    sessDuration = TIME_ConvertToClock(sessDurationStr);

#ifdef DEBUG
                    {
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    char addrStr[MAX_STRING_LENGTH];
                    printf("Starting TELNET client with:\n");
                    printf("  src nodeId:       %u\n", sourceNodeId);
                    printf("  dst nodeId:       %u\n", destNodeId);
                    IO_ConvertIpAddressToString(&destAddr, addrStr);
                    printf("  dst address:      %s\n", addrStr);
                    ctoa(sessDuration, clockStr);
                    printf("  session duration: %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:       %s\n", clockStr);
                    }
#endif /* DEBUG */

                    // checks if destination is configured as url and find
                    // url destination address such that application server
                    // can be initialized
                    Address urlDestAddress;
                    isUrl = node->appData.appTrafficSender->
                            appCheckUrlAndGetUrlDestinationAddress(
                                        node,
                                        appInput.inputStrings[i],
                                        FALSE,
                                        &sourceAddr,
                                        &destAddr,
                                        &destNodeId,
                                        &urlDestAddress,
                                        &AppTelnetUrlSessionStartCallBack);

                    AppTelnetClientInit(
                        node,
                        sourceAddr,
                        destAddr,
                        sessDuration,
                        startTime,
                        destString);

                    // dns
                    if (isUrl)
                    {
                        memcpy(&destAddr, &urlDestAddress, sizeof(Address));
                    }
                    if (destAddr.networkType != NETWORK_INVALID &&
                        sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                            "FTP: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);
                    }
                }

                if (!isUrl && ADDR_IsUrl(destString) == TRUE)
                {
                    AppUpdateUrlServerNodeIdAndAddress(
                                                    firstNode,
                                                    destString,
                                                    &destAddr,
                                                    &destNodeId);
                }

                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr,
                                        destNodeId,
                                        sourceAddr,
                                        sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    // Dynamic address
                    // Delay TELNET server listening by start time
                    // such that server may acquire an address by that time
                    // to ensure that server listens earlier than TCP
                    // open; therefore listen timer should be set at unit
                    // time (1 ns) before Open. If start time is less than
                    // 1 nano second(0S) then listenTime remains as 0S which
                    // is the least possible simulation time.
                    clocktype listenTime = 0;
                    if (TIME_ConvertToClock(startTimeStr) > 1 * NANO_SECOND)
                    {
                        listenTime =
                            TIME_ConvertToClock(startTimeStr) - 1 * NANO_SECOND;
                    }
                    AppStartTcpAppSessionServerListenTimer(
                                                        node,
                                                        APP_TELNET_SERVER,
                                                        destAddr,
                                                        destString,
                                                        listenTime);
                }
            }
            else
            if (strcmp(appStr, "CBR") == 0)
            {
                char profileName[MAX_STRING_LENGTH] = "/0";
                BOOL isProfileNameSet = FALSE;
                BOOL isMdpEnabled = FALSE;
                char sourceString[MAX_STRING_LENGTH];
                char destString[BIG_STRING_LENGTH];
                char intervalStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                int itemsToSend;
                int itemSize;
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                unsigned tos = APP_DEFAULT_TOS;
                BOOL isRsvpTeEnabled = FALSE;
                char optionToken1[MAX_STRING_LENGTH];
                char optionToken2[MAX_STRING_LENGTH];
                char optionToken3[MAX_STRING_LENGTH];
                char optionToken4[MAX_STRING_LENGTH];
                char optionToken5[MAX_STRING_LENGTH];
                char optionToken6[MAX_STRING_LENGTH];

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %d %s %s %s %s %s %s %s %s %s",
                                sourceString,
                                destString,
                                &itemsToSend,
                                &itemSize,
                                intervalStr,
                                startTimeStr,
                                endTimeStr,
                                optionToken1,
                                optionToken2,
                                optionToken3,
                                optionToken4,
                                optionToken5,
                                optionToken6);

                switch (numValues)
                {
                    case 7:
                        break;
                    case 8:
                        if (!strcmp(optionToken1, "RSVP-TE"))
                        {
                            isRsvpTeEnabled = TRUE;
                            break;
                        }
                        else if (!strcmp(optionToken1, "MDP-ENABLED"))
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                        }// else fall through default
                    case 9 :
                        if (APP_AssignTos(optionToken1, optionToken2, &tos))
                        {
                            break;
                        }
                        else if (!strcmp(optionToken1, "RSVP-TE")
                                && !strcmp(optionToken2, "MDP-ENABLED"))
                        {
                            isRsvpTeEnabled = TRUE;
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                        }// else fall through default
                    case 10 :
                        if (APP_AssignTos(optionToken1, optionToken2, &tos))
                        {
                            if (!strcmp(optionToken3, "RSVP-TE"))
                            {
                                isRsvpTeEnabled = TRUE;
                                break;
                            }
                            else if (!strcmp(optionToken3, "MDP-ENABLED"))
                            {
                                isMdpEnabled = TRUE;
                                isProfileNameSet = FALSE;
                                break;
                            }
                        }
                        else if (!strcmp(optionToken1, "RSVP-TE")
                                &&
                                APP_AssignTos(optionToken2, optionToken3, &tos))
                        {
                            isRsvpTeEnabled = TRUE;
                            break;
                        }
                        else if (!strcmp(optionToken1, "MDP-ENABLED")
                                && !strcmp(optionToken2, "MDP-PROFILE")
                                && strcmp(optionToken3, ""))
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = TRUE;
                            sprintf(profileName, "%s", optionToken3);
                                break;
                        }// else fall through default
                    case 11 :
                        if (APP_AssignTos(optionToken1, optionToken2, &tos)
                            && !strcmp(optionToken3, "RSVP-TE")
                            && !strcmp(optionToken4, "MDP-ENABLED"))
                        {
                            isRsvpTeEnabled = TRUE;
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                        }
                        else if (!strcmp(optionToken1, "RSVP-TE"))
                        {
                            if (APP_AssignTos(optionToken2,optionToken3,&tos)
                                && !strcmp(optionToken4, "MDP-ENABLED"))
                            {
                                isRsvpTeEnabled = TRUE;
                                isMdpEnabled = TRUE;
                                isProfileNameSet = FALSE;
                                break;
                            }
                            else if (!strcmp(optionToken2, "MDP-ENABLED")
                                    && !strcmp(optionToken3, "MDP-PROFILE")
                                    && strcmp(optionToken4, ""))
                            {
                                isRsvpTeEnabled = TRUE;
                                isMdpEnabled = TRUE;
                                isProfileNameSet = TRUE;
                                sprintf(profileName, "%s", optionToken4);
                                break;
                            }
                        }// else fall through default
                    case 12 :
                        if (APP_AssignTos(optionToken1, optionToken2, &tos)
                            && !strcmp(optionToken3, "MDP-ENABLED")
                            && !strcmp(optionToken4, "MDP-PROFILE")
                            && strcmp(optionToken5, ""))
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = TRUE;
                            sprintf(profileName, "%s", optionToken5);
                            break;
                        }// else fall through default
                    case 13 :
                        if ((APP_AssignTos(optionToken1, optionToken2, &tos)
                            && !strcmp(optionToken3, "RSVP-TE")
                            && !strcmp(optionToken4, "MDP-ENABLED")
                            && !strcmp(optionToken5, "MDP-PROFILE")
                            && strcmp(optionToken6, ""))
                            ||
                            (!strcmp(optionToken1, "RSVP-TE")
                            && APP_AssignTos(optionToken2, optionToken3, &tos)
                            && !strcmp(optionToken4, "MDP-ENABLED")
                            && !strcmp(optionToken5, "MDP-PROFILE")
                            && strcmp(optionToken6, "")))
                        {
                            isRsvpTeEnabled = TRUE;
                            isMdpEnabled = TRUE;
                            isProfileNameSet = TRUE;
                            sprintf(profileName, "%s", optionToken6);
                            break;
                        }// else fall through default
                    default:
                    {
                        char errorString[MAX_STRING_LENGTH + 100];
                        sprintf(errorString,
                            "Wrong CBR configuration format!\n"
                            "CBR <src> <dest> <items to send> "
                            "<item size> <interval> <start time> "
                            "<end time> [TOS <tos-value> | DSCP <dscp-value>"
                            " | PRECEDENCE <precedence-value>] [RSVP-TE] "
                            "[MDP-ENABLED [MDP-PROFILE <profile name>] ]\n");
                        ERROR_ReportError(errorString);
                    }
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
                    clocktype endTime = TIME_ConvertToClock(endTimeStr);
                    clocktype interval = TIME_ConvertToClock(intervalStr);

                    if ((node->adaptationData.adaptationProtocol
                        != ADAPTATION_PROTOCOL_NONE)
                        && (!node->adaptationData.endSystem))
                    {

                        char err[MAX_STRING_LENGTH];
                        sprintf(err,"Only end system can be a valid source\n");
                        ERROR_ReportWarning(err);

                        return;
                    }

#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting CBR client with:\n");
                    printf("  src nodeId:    %u\n", sourceNodeId);
                    printf("  dst nodeId:    %u\n", destNodeId);
                    printf("  dst address:   %u\n", destAddr);
                    printf("  items to send: %d\n", itemsToSend);
                    printf("  item size:     %d\n", itemSize);
                    ctoa(interval, clockStr);
                    printf("  interval:      %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
                    ctoa(endTime, clockStr);
                    printf("  end time:      %s\n", clockStr);
                    printf("  tos:           %u\n", tos);
#endif /* DEBUG */

                    // checks if destination is configured as url and find
                    // url destination address such that application server
                    // can be initialized
                    Address urlDestAddress;
                    isUrl = node->appData.appTrafficSender->
                            appCheckUrlAndGetUrlDestinationAddress(
                                        node,
                                        appInput.inputStrings[i],
                                        isMdpEnabled,
                                        &sourceAddr,
                                        &destAddr,
                                        &destNodeId,
                                        &urlDestAddress,
                                        &AppCbrUrlSessionStartCallback);

                    if (isMdpEnabled)
                    {
                        AppCbrClientInit(
                            node,
                            sourceAddr,
                            destAddr,
                            itemsToSend,
                            itemSize,
                            interval,
                            startTime,
                            endTime,
                            tos,
                            sourceString,
                            destString,
                            isRsvpTeEnabled,
                            appNamePtr,
                            isMdpEnabled,
                            isProfileNameSet,
                            profileName,
                            i+1,
                            nodeInput);
                    }
                    else
                    {
                        AppCbrClientInit(
                            node,
                            sourceAddr,
                            destAddr,
                            itemsToSend,
                            itemSize,
                            interval,
                            startTime,
                            endTime,
                            tos,
                            sourceString,
                            destString,
                            isRsvpTeEnabled,
                            appNamePtr);
                    }
                    // dns
                    if (isUrl)
                    {
                        memcpy(&destAddr, &urlDestAddress, sizeof(Address));
                    }
                    if (destAddr.networkType != NETWORK_INVALID &&
                        sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                            "CBR: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);
                    }
                }

                if (!isUrl && ADDR_IsUrl(destString) == TRUE)
                {
                    AppUpdateUrlServerNodeIdAndAddress(
                                                    firstNode,
                                                    destString,
                                                    &destAddr,
                                                    &destNodeId);
                }

                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr,
                                        destNodeId,
                                        sourceAddr,
                                        sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    AppCbrServerInit(node);
                }

                if (isMdpEnabled)
                {
                    //for MDP layer init on destination node
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                    if (node != NULL)
                    {
                        MdpLayerInitForOtherPartitionNodes(node,
                                                        nodeInput,
                                                        NULL,
                                                        TRUE);
                    }
                }
            }
            else
            if (strcmp(appStr, "LOOKUP") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char requestIntervalStr[MAX_STRING_LENGTH];
                char replyDelayStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                char tosStr[MAX_STRING_LENGTH];
                char tosValueStr[MAX_STRING_LENGTH];
                int requestsToSend;
                int requestSize;
                int replySize;
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                unsigned tos = APP_DEFAULT_TOS;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %d %d %s %s %s %s %s %s",
                                sourceString,
                                destString,
                                &requestsToSend,
                                &requestSize,
                                &replySize,
                                requestIntervalStr,
                                replyDelayStr,
                                startTimeStr,
                                endTimeStr,
                                tosStr,
                                tosValueStr);

                switch (numValues)
                {
                    case 9:
                        break;
                    case 11 :
                        if (APP_AssignTos(tosStr, tosValueStr, &tos))
                        {
                            break;
                        } // else fall through default
                    default:
                    {
                        char errorString[2* MAX_STRING_LENGTH];
                        sprintf(errorString,
                                "Wrong LOOKUP configuration format!\n"
                                "LOOKUP <src> <dest> <requests to send> "
                                "<request size> <reply size> <request interval> "
                                "<reply delay> <start time> <end time>"
                                " [TOS <tos-value> | DSCP <dscp-value>"
                                " | PRECEDENCE <precedence-value>]\n");
                        ERROR_ReportError(errorString);
                    }
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
                    clocktype endTime = TIME_ConvertToClock(endTimeStr);
                    clocktype requestInterval
                        = TIME_ConvertToClock(requestIntervalStr);
                    clocktype replyDelay = TIME_ConvertToClock(replyDelayStr);

#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting LOOKUP client with:\n");
                    printf("  src nodeId:       %u\n", sourceNodeId);
                    printf("  dst nodeId:       %u\n", destNodeId);
                    printf("  dst address:      %u\n", destAddr);
                    printf("  requests to send: %d\n", requestsToSend);
                    printf("  request size:     %d\n", requestSize);
                    printf("  reply size:       %d\n", replySize);
                    ctoa(requestInterval, clockStr);
                    printf("  request interval: %s\n", clockStr);
                    ctoa(replyDelay, clockStr);
                    printf("  reply delay:      %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:       %s\n", clockStr);
                    ctoa(endTime, clockStr);
                    printf("  end time:         %s\n", clockStr);
                    printf("  tos:              %u\n", tos);
#endif /* DEBUG */

                    AppLookupClientInit(
                        node,
                        GetIPv4Address(sourceAddr),
                        GetIPv4Address(destAddr),
                        requestsToSend,
                        requestSize,
                        replySize,
                        requestInterval,
                        replyDelay,
                        startTime,
                        endTime,
                        tos);
                }

                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr, destNodeId,
                                        sourceAddr, sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    AppLookupServerInit(node);
                }
            }
            else
            if (strcmp(appStr, "MCBR") == 0)
            {
                char profileName[MAX_STRING_LENGTH] = "/0";
                BOOL isProfileNameSet = FALSE;
                BOOL isMdpEnabled = FALSE;

                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char intervalStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                int itemsToSend;
                int itemSize;
                NodeAddress sourceNodeId;
                Address sourceAddr;
                Address destAddr;
                NodeId destNodeId;
                BOOL isNodeId;
                unsigned tos = APP_DEFAULT_TOS;
                char optionToken1[MAX_STRING_LENGTH];
                char optionToken2[MAX_STRING_LENGTH];

                char optionToken3[MAX_STRING_LENGTH];
                char optionToken4[MAX_STRING_LENGTH];
                char optionToken5[MAX_STRING_LENGTH];

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %d %s %s %s %s %s %s %s %s",
                                sourceString,
                                destString,
                                &itemsToSend,
                                &itemSize,
                                intervalStr,
                                startTimeStr,
                                endTimeStr,
                                optionToken1,
                                optionToken2,
                                optionToken3,
                                optionToken4,
                                optionToken5);

                switch (numValues)
                {
                    case 7:
                        break;
                    case 8:
                        if (strcmp(optionToken1, "MDP-ENABLED") == 0)
                        {
                        isMdpEnabled = TRUE;
                        isProfileNameSet = FALSE;
                        break;
                        }// else fall through default
                    case 9:
                        {
                            if (APP_AssignTos(optionToken1, optionToken2, &tos))
                            {
                                break;
                            }
                            else if (strcmp(optionToken1, "MDP-ENABLED") == 0)
                            {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                            }// else fall through default
                        }
                    case 10:
                        {
                            if (strcmp(optionToken1, "MDP-ENABLED") == 0
                                && strcmp(optionToken2, "MDP-PROFILE") == 0
                                && strcmp(optionToken3, "") != 0)
                            {
                                isMdpEnabled = TRUE;
                                isProfileNameSet = TRUE;
                                sprintf(profileName, "%s", optionToken3);
                                break;
                            }
                            else if (APP_AssignTos(optionToken1,optionToken2,&tos)
                                    && strcmp(optionToken3, "MDP-ENABLED") == 0)
                            {
                                isMdpEnabled = TRUE;
                                isProfileNameSet = FALSE;
                                break;
                            }// else fall through default
                        }
                    case 12:
                        {
                            if (APP_AssignTos(optionToken1, optionToken2, &tos))
                            {
                                if (strcmp(optionToken3, "MDP-ENABLED") == 0
                                    && strcmp(optionToken4, "MDP-PROFILE") == 0
                                    && strcmp(optionToken5, "") != 0)
                                {
                                    isMdpEnabled = TRUE;
                                    isProfileNameSet = TRUE;
                                    sprintf(profileName, "%s", optionToken5);
                                    break;
                                }
                            }// else fall through default
                        }
                    default:
                    {
                        char errorString[MAX_STRING_LENGTH + 100];
                        sprintf(errorString,
                            "Wrong MCBR configuration format!\n"
                            "MCBR <src> <dest> <items to send> <item size> "
                            "<interval> <start time> <end time> "
                            "[TOS <tos-value> | DSCP <dscp-value>"
                            " | PRECEDENCE <precedence-value>] "
                            "[MDP-ENABLED [MDP-PROFILE <profile name>] ]\n");
                        ERROR_ReportError(errorString);
                    }
                }


                if (MAPPING_GetNetworkType(destString) == NETWORK_IPV6)
                {
                    destAddr.networkType = NETWORK_IPV6;

                    IO_ParseNodeIdOrHostAddress(
                        destString,
                        &destAddr.interfaceAddr.ipv6,
                        &destNodeId);
                }
                else
                {
                    destAddr.networkType = NETWORK_IPV4;

                    IO_ParseNodeIdOrHostAddress(
                        destString,
                        &destAddr.interfaceAddr.ipv4,
                        &isNodeId);
                }

            if (destAddr.networkType == NETWORK_IPV4)
            {
                if (!(GetIPv4Address(destAddr) >= IP_MIN_MULTICAST_ADDRESS))
                {
                        ERROR_ReportWarning("MCBR: destination address is"
                                            " not a multicast address\n");
                }
            }
            else
            {
                    if (!IS_MULTIADDR6(GetIPv6Address(destAddr)))
                    {
                        ERROR_ReportWarning("MCBR: destination address is"
                                            " not a multicast address\n");
                    }
                    else if (!Ipv6IsValidGetMulticastScope(
                                node,
                                GetIPv6Address(destAddr)))
                    {
                        ERROR_ReportWarning("MCBR: destination address is not"
                                            " a valid multicast address\n");
                    }
                }

                IO_AppParseSourceString(
                        firstNode,
                        appInput.inputStrings[i],
                        sourceString,
                        &sourceNodeId,
                        &sourceAddr,
                        destAddr.networkType);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
                    clocktype endTime = TIME_ConvertToClock(endTimeStr);
                    clocktype interval = TIME_ConvertToClock(intervalStr);

#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting MCBR client with:\n");
                    printf("  src nodeId:    %u\n", sourceNodeId);
                    printf("  dst address:   %u\n", destAddr);
                    printf("  items to send: %d\n", itemsToSend);
                    printf("  item size:     %d\n", itemSize);
                    ctoa(interval, clockStr);
                    printf("  interval:      %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
                    ctoa(endTime, clockStr);
                    printf("  end time:      %s\n", clockStr);
                    printf("  tos:           %d\n", tos);
#endif /* DEBUG */

                    if (isMdpEnabled)
                    {
                        AppMCbrClientInit(
                            node,
                            sourceAddr,
                            destAddr,
                            itemsToSend,
                            itemSize,
                            interval,
                            startTime,
                            endTime,
                            sourceString,
                            destString,
                            tos,
                            appNamePtr,
                            isMdpEnabled,
                            isProfileNameSet,
                            profileName,
                            i+1,
                            nodeInput);
                    }
                    else
                    {
                        AppMCbrClientInit(
                            node,
                            sourceAddr,
                            destAddr,
                            itemsToSend,
                            itemSize,
                            interval,
                            startTime,
                            endTime,
                            sourceString,
                            destString,
                            tos,
                            appNamePtr);
                    }

                    if (sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                                "MCBR: At node %d, Source and Destination IP"
                                " version mismatch inside %s\n",
                            node->nodeId,
                            appInput.inputStrings[i]);
                    }
                }
                else
                {
                    if (isMdpEnabled)
                    {
                        // init the MDP layer for nodes lies in the
                        // "firstNode" partition
                        MdpLayerInitForOtherPartitionNodes(firstNode,
                                                        nodeInput,
                                                        &destAddr);
                    }
                }
            }

            else if (strcmp(appStr, "TIMER-TEST") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                //char destString[MAX_STRING_LENGTH];
                //char intervalStr[MAX_STRING_LENGTH];
                //char startTimeStr[MAX_STRING_LENGTH];
                //char endTimeStr[MAX_STRING_LENGTH];
                //int itemsToSend;
                //int itemSize;
                NodeAddress sourceNodeId;
                Address sourceAddr;
                //Address destAddr;
                //NodeId destNodeId;
                //BOOL isNodeId;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s",
                                sourceString);

                IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    AppTimerTestInit(node);
                }
            }

            else
            if (strcmp(appStr, "mgen") == 0)
            {
#ifdef ADDON_MGEN4
                int sourceNodeId = ANY_SOURCE_ADDR;
                int destNodeId   = ANY_DEST;
                Node* node1 = NULL;
                retVal = sscanf(appInput.inputStrings[i], "%*s %u %u",
                                &sourceNodeId, &destNodeId);
                node1 = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node1 != NULL)
                {
                    MgenInit(node1, appInput.inputStrings[i], appNamePtr);
                }
#else // ADDON_MGEN4
                ERROR_ReportError("MGEN4 addon required\n");
#endif // ADDON_MGEN4
            }
            else
                if (strcmp(appStr, "mgen3") == 0)
                {
#ifdef ADDON_MGEN3
                    int sourceNodeId;

                    retVal = sscanf(appInput.inputStrings[i],
                                        "%*s %u", &sourceNodeId);

                    node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);

                    if (node != NULL)
                    {
                        AppMgenInit(node, appInput.inputStrings[i]);
                    }
#else // ADDON_MGEN3
                ERROR_ReportError("MGEN3 addon required\n");
#endif // ADDON_MGEN3
                }
                else
                if (strcmp(appStr, "drec") == 0)
                {
#ifdef ADDON_MGEN3
                int sourceNodeId;

                retVal = sscanf(appInput.inputStrings[i],
                        "%*s %u", &sourceNodeId);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    AppDrecInit(node, appInput.inputStrings[i]);
                }
#else // ADDON_MGEN3
                ERROR_ReportError("MGEN3 addon required\n");
#endif // ADDON_MGEN3
            }

            // Modifications
#ifdef USER_MODELS_LIB
            else if (strcmp(appStr, "HELLO") == 0) {
                AppHelloInit(node, nodeInput);
            }

            else if (strcmp(appStr, "UP") == 0) {
                // AppUpInit(node, nodeInput);
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char nodeTypeString[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                AppUpNodeType nodeType;

                char nodePathFileName[MAX_STRING_LENGTH];
                char nodePlanFileName[MAX_STRING_LENGTH];
                char dataChunkIdString[MAX_STRING_LENGTH];
                char dataChunkSizeString[MAX_STRING_LENGTH];
                char dataChunkDeadlineString[MAX_STRING_LENGTH];
                char dataChunkPriorityString[MAX_STRING_LENGTH];

                numValues = sscanf(appInput.inputStrings[i],
						"%*s %s %s %s",
						sourceString,
						destString,
						nodeTypeString);

                // Check integraty before parsing parameters
				switch (numValues) {
					case 3: break;
					default: {
						char errorString[MAX_STRING_LENGTH];
						sprintf(errorString,
								"Wrong UP configuration format!\n"
								"UP <source> <dest> <type> ...\n");
						ERROR_ReportError(errorString);
					}
				}

                // Parse node type
				if (strcmp(nodeTypeString, "CLOUD") == 0) {
					nodeType = APP_UP_NODE_CLOUD;
				} else if (strcmp(nodeTypeString, "MDC") == 0) {
					nodeType = APP_UP_NODE_MDC;
					numValues += sscanf(appInput.inputStrings[i],
							"%*s %*s %*s %*s %s %s",
							nodePathFileName,
							nodePlanFileName);
					if (numValues != 5) {
						char errorString[MAX_STRING_LENGTH];
						sprintf(errorString,
								"Wrong UP configuration format!\n"
								"UP <source> <dest> MDC <path> <plan>\n");
						ERROR_ReportError(errorString);
					}
				} else if (strcmp(nodeTypeString, "DATA") == 0) {
					nodeType = APP_UP_NODE_DATA_SITE;
					numValues += sscanf(appInput.inputStrings[i],
							"%*s %*s %*s %*s %s %s %s %s",
							dataChunkIdString,
							dataChunkSizeString,
							dataChunkDeadlineString,
							dataChunkPriorityString);
					if (numValues != 7 && numValues != 4) { // Multiple chunks
						char errorString[MAX_STRING_LENGTH];
						sprintf(errorString,
								"Wrong UP configuration format!\n"
								"UP <source> <dest> DATA "
								"<id> <size> <deadline> <priority>\n"
								"UP <source> <dest> DATA <file>\n");
						ERROR_ReportError(errorString);
					}
				} else {
					char errorString[MAX_STRING_LENGTH];
					sprintf(errorString,
							"Wrong UP configuration format: "
							"Invalid node type\n");
					ERROR_ReportError(errorString);
				}

                // Parse addresses with respect to node type
                // Initialize client and/or server instances
				IO_AppParseSourceAndDestStrings(
					firstNode,
					appInput.inputStrings[i],
					sourceString,
					&sourceNodeId,
					&sourceAddr,
					destString,
					&destNodeId,
					&destAddr);
                switch (nodeType) {
                case APP_UP_NODE_CLOUD: {
/*                	IO_AppParseDestString(
                		firstNode,
						appInput.inputStrings[i],
						sourceString,
						&sourceNodeId,
						&sourceAddr);*/
                	node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
					if (node != NULL) {
#ifdef DEBUG
						char sourceAddrStr[MAX_STRING_LENGTH];

						IO_ConvertIpAddressToString(&sourceAddr, sourceAddrStr);
						printf("Starting UP server at:\n");
						printf("  node_%u (%s)\n",
								sourceNodeId,
								sourceAddrStr);
#endif // DEBUG
						AppUpServerInit(
							node,
							sourceAddr);
					}
                	break; }
                case APP_UP_NODE_MDC: {
                	node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
					if (node != NULL) {
#ifdef DEBUG
						char sourceAddrStr[MAX_STRING_LENGTH];

						IO_ConvertIpAddressToString(
								&sourceAddr,
								sourceAddrStr);
						printf("Starting UP client daemon with:\n");
						printf("  src: node_%u\n",
								sourceNodeId);
						printf("  dst: node_%u\n",
								destNodeId);
						printf("Starting UP server at:\n");
						printf("  node_%u (%s)\n",
								sourceNodeId,
								sourceAddrStr);
#endif // DEBUG
						AppUpClientDaemonInit(
							node,
							firstNode,
							sourceNodeId,
							destNodeId,
							appInput.inputStrings[i],
							appNamePtr,
							nodeType);
						AppUpServerInit(
							node,
							sourceAddr);
					}
                	break; }
                case APP_UP_NODE_DATA_SITE: {
                	node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
					if (node != NULL) {
#ifdef DEBUG
						printf("Starting UP client daemon with:\n");
						printf("  src: node_%u\n",
								sourceNodeId);
						printf("  dst: node_%u\n",
								destNodeId);
#endif // DEBUG
/*						AppUpClientInit(
							node,
							sourceAddr,
							destAddr,
							appNamePtr,
							sourceString,
							nodeType);*/
						AppUpClientDaemonInit(
							node,
							firstNode,
							sourceNodeId,
							destNodeId,
							appInput.inputStrings[i],
							appNamePtr,
							nodeType);
					}
                	break; }
                default:
                	assert(false);
                }
            }
#endif // USER_MODELS_LIB

#ifndef STANDALONE_MESSENGER_APPLICATION
            else
            if (strcmp(appStr, "MESSENGER-APP") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s",
                                sourceString);
                assert(numValues == 1);

                IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    MessengerInit(node);
                }
            }

#else /* STANDALONE_MESSENGER_APPLICATION */
            else
            if (strcmp(appStr, "MESSENGER-APP") == 0)
            {
                char srcStr[MAX_STRING_LENGTH];
                char destStr[MAX_STRING_LENGTH];
                char tTypeStr[MAX_STRING_LENGTH];
                char appTypeStr[MAX_STRING_LENGTH];
                char lifeTimeStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char intervalStr[MAX_STRING_LENGTH];
                char filenameStr[MAX_STRING_LENGTH];
                char errorString[2*MAX_STRING_LENGTH];
#ifdef MILITARY_RADIOS_LIB
                char destNPGStr[MAX_STRING_LENGTH];
                int destNPGId = 0;
#endif
                int fragSize = 0;
                int fragNum = 0;
                NodeAddress srcNodeId;
                NodeAddress srcAddr;
                NodeAddress destNodeId = 0;
                NodeAddress destAddr;
                TransportType t_type = (TransportType)0;
                MessengerAppType app_type = GENERAL;
                clocktype lifeTime = 0;
                clocktype startTime = 0;
                clocktype interval = 0;
                char* filename = NULL;
#ifdef MILITARY_RADIOS_LIB
                int tempnumValues = sscanf(appInput.inputStrings[i],
                                    "%*s %s", srcStr);

                IO_AppParseSourceString(firstNode,
                    appInput.inputStrings[i],
                    srcStr,
                    &srcNodeId,
                    &srcAddr);
                node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);

                if (node == NULL){
                    continue;
                }
                if (node != NULL)
                {
                    int numValues = sscanf(appInput.inputStrings[i],
                                        "%*s %s %s %s %s %s %s %s %d %d %s %d %s",
                                        srcStr,
                                        destStr,
                                        tTypeStr,
                                        appTypeStr,
                                        lifeTimeStr,
                                        startTimeStr,
                                        intervalStr,
                                        &fragSize,
                                        &fragNum,
                                        destNPGStr,
                                        &destNPGId,
                                        filenameStr);

                    if (numValues == 11)
                    {
                        filename = NULL;
                    }
                    if (numValues == 12)
                    {
                        filename = (char*) MEM_malloc(strlen(filenameStr) + 1);
                        strcpy(filename, filenameStr);
                    }
                    if (numValues == 10)
                    {
                        filename = (char*) MEM_malloc(strlen(destNPGStr) + 1);
                        strcpy(filename, destNPGStr);
                    }

                    if ((numValues > 9) && (strcmp(destNPGStr, "NPG") == 0))
                    {
                        if (node->macData[MAC_DEFAULT_INTERFACE]->macProtocol
                            == MAC_PROTOCOL_TADIL_LINK16)
                        {
                            if (numValues != 11 && numValues != 12)
                            {
                                sprintf(errorString,
                                    "MESSENGER:Wrong MESSENGER configuration format!\n"
                                    "\tMESSENGER-APP <src> <dest> <transport_type> "
                                    "<life_time>\n\t\t<start_time> <interval> "
                                    "<fragment_size> <fragment_num> "
                                    "NPG <desttination NPGId> "
                                    "[additional_data_file_name]\n");
                                ERROR_ReportError(errorString);
                            }
                        }
                        else
                        {
                            // Ignore the Destination NPG id information
                            destNPGId = 0;
                            if (numValues == 10)
                            {
                                filename = NULL;
                            }
                        }
                    }
                    else
                    {
                        if (node->macData[MAC_DEFAULT_INTERFACE]->macProtocol
                            == MAC_PROTOCOL_TADIL_LINK16)
                        {
                            sprintf(errorString,
                                "MESSENGER:Wrong MESSENGER configuration format!\n"
                                "\tMESSENGER-APP <src> <dest> <transport_type> "
                                "<life_time>\n\t\t<start_time> <interval> "
                                "<fragment_size> <fragment_num> "
                                "NPG <desttination NPGId> "
                                "[additional_data_file_name]\n");
                            ERROR_ReportError(errorString);
                        }
                        else
                        {
                            destNPGId = 0;
                            if (numValues != 9 && numValues != 10 && numValues != 12)
                            {
                                sprintf(errorString,
                                    "MESSENGER:Wrong MESSENGER configuration format!\n"
                                    "\tMESSENGER-APP <src> <dest> <transport_type> "
                                    "<life_time>\n\t\t<start_time> <interval> "
                                    "<fragment_size> <fragment_num> "
                                    "[additional_data_file_name]\n");
                                ERROR_ReportError(errorString);
                            }
                            if (numValues == 9)
                            {
                                filename = NULL;
                            }

                        }
                    }
                }
#else

                int numValues = sscanf(appInput.inputStrings[i],
                                    "%*s %s %s %s %s %s %s %s %d %d %s",
                                    srcStr,
                                    destStr,
                                    tTypeStr,
                                    appTypeStr,
                                    lifeTimeStr,
                                    startTimeStr,
                                    intervalStr,
                                    &fragSize,
                                    &fragNum,
                                    filenameStr);

                if (numValues != 9 && numValues != 10)
                {
                    sprintf(errorString,
                        "MESSENGER:Wrong MESSENGER configuration format!\n"
                        "\tMESSENGER-APP <src> <dest> <transport_type> "
                        "<life_time>\n\t\t<start_time> <interval> "
                        "<fragment_size> <fragment_num> "
                        "[additional_data_file_name]\n");
                    ERROR_ReportError(errorString);
                }

                if (numValues == 9)
                {
                    filename = NULL;
                }
                else
                {
                    filename = (char*) MEM_malloc(strlen(filenameStr) + 1);
                    strcpy(filename, filenameStr);
                }
#endif
                IO_AppParseSourceString(firstNode,
                                    appInput.inputStrings[i],
                                    srcStr,
                                    &srcNodeId,
                                    &srcAddr);

                if (strcmp(destStr, "255.255.255.255") == 0)
                {
                    destAddr = ANY_DEST;
                }
                else
                {
                    IO_AppParseDestString(firstNode,
                                        appInput.inputStrings[i],
                                        destStr,
                                        &destNodeId,
                                        &destAddr);
                }

                if (strcmp(tTypeStr, "UNRELIABLE") == 0)
                {
                    t_type = TRANSPORT_TYPE_UNRELIABLE;
                }
                else if (strcmp(tTypeStr, "RELIABLE") == 0)
                {
                    if (destAddr == ANY_DEST)
                    {
                        sprintf(errorString, "MESSENGER:BROADCAST "
                                "is not assigned for RELIABLE service.\n");
                        ERROR_ReportError(errorString);
                    }
                    t_type = TRANSPORT_TYPE_RELIABLE;
                }
                else if (strcmp(tTypeStr, "MAC") == 0)
                {
                    t_type = TRANSPORT_TYPE_MAC;
                }
                else
                {
                    sprintf(errorString,
                            "MESSENGER: %s is not supported by TRANSPORT-TYPE.\n"
                                "Only UNRELIABLE / RELIABLE / TADIL"
                                " are currently supported.\n", tTypeStr);
                    ERROR_ReportError(errorString);
                }

                if (strcmp(appTypeStr, "GENERAL") == 0)
                {
                    app_type = GENERAL;
                }
                else if (strcmp(appTypeStr, "VOICE") == 0)
                {
                    if (t_type == TRANSPORT_TYPE_RELIABLE ||
                        t_type == TRANSPORT_TYPE_MAC ||
                        filename != NULL)
                    {
                        sprintf(errorString, "MESSENGER:VOICE application "
                                "only support UNRELIABLE transport type "
                                "and not support external file data.\n");
                        ERROR_ReportError(errorString);
                    }
                    app_type = VOICE_PACKET;
                }
                else
                {
                    sprintf(errorString, "MESSENGER:%s is not supported by "
                            "APPLICATION-TYPE.\nOnly GENERAL/VOICE are "
                            "currently supported.\n", appTypeStr);
                    ERROR_ReportError(errorString);
                }
                lifeTime = TIME_ConvertToClock(lifeTimeStr);
                startTime = TIME_ConvertToClock(startTimeStr);
                interval = TIME_ConvertToClock(intervalStr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);

                if (node == NULL){
                    continue;
                }
                if (node != NULL)
                {
#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting MESSENGER client with:\n");
                    printf("  src nodeId:    %u\n", srcNodeId);

                    if (destAddr == ANY_DEST)
                    {
                        printf("  dst address:   255.255.255.255\n");
                    }
                    else
                    {
                        printf("  dst address:   %u\n", destAddr);
                    }

                    printf("  transport type: %s\n", tTypeStr);
                    ctoa(lifeTime, clockStr);
                    printf("  life time:      %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
                    ctoa(interval, clockStr);
                    printf("  interval:      %s\n", clockStr);
                    printf("  fragment size:      %d\n", fragSize);
                    printf("  fragment number:      %d\n", fragNum);

                    if (filename)
                    {
                        printf("  file name:      %s\n", filename);
                    }
#endif // DEBUG
                    MessengerClientInit(
                        node,
                        srcAddr,
                        destAddr,
                        t_type,
                        app_type,
                        lifeTime,
                        startTime,
                        interval,
                        fragSize,
                        fragNum,
#ifdef MILITARY_RADIOS_LIB
                        (unsigned short)destNPGId,
#endif // MILITARY_RADIOS_LIB
                        filename);
            }
                if (destAddr != ANY_DEST)
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);

                    if (node != NULL)
                    {
                        MessengerServerListen(node);
                    }
                }
            }
#endif // STANDALONE_MESSENGER_APPLICATION

            else
            if (strcmp(appStr, "INTERFACETUTORIAL") == 0)
            {
#ifdef TUTORIAL_INTERFACE
                char nodeString[MAX_STRING_LENGTH];
                char dataString[MAX_STRING_LENGTH];
                NodeAddress nodeId;
                NodeAddress nodeAddr;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                nodeString,
                                dataString);

                if (numValues != 2)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong INTERFACETUTORIAL configuration format!\n"
                            "INTERFACETUTORIAL <node> <data>");
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceString(
                        firstNode,
                        appInput.inputStrings[i],
                        nodeString,
                        &nodeId,
                        &nodeAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, nodeId);
                if (node != NULL)
                {
                    AppInterfaceTutorialInit(node, dataString);
                }
#else /* TUTORIAL_INTERFACE */
                ERROR_ReportMissingInterface(appStr, "Interface Tutorial\n");
#endif /* TUTORIAL_INTERFACE */
            }

            else if (strcmp(appStr, "ALE") == 0)
            {
#ifdef ALE_ASAPS_LIB
                BOOL    errorOccured = FALSE;

                int lineLen = NI_GetMaxLen(&appInput);
                char *inputString;
                char* next;
                char *token;
                char iotoken[MAX_STRING_LENGTH];
                const char *delims = " \t\n";

                char    sourceString[MAX_STRING_LENGTH];
                char    destNumString[MAX_STRING_LENGTH];
                char    *destString[MAX_STRING_LENGTH];

                char    intervalStr[MAX_STRING_LENGTH];
                char    startTimeStr[MAX_STRING_LENGTH];
                char    endTimeStr[MAX_STRING_LENGTH];
                char    errorString[MAX_STRING_LENGTH];
                char    stdErrorString[MAX_STRING_LENGTH];

                int destNum = 1;    // Group calls are treated as Net calls
                int count;
                int sourceInterfaceIndex;
                int interfaceIndex;
                int requestedCallChannel;

                Node        *sourceNode;
                NodeAddress sourceNodeId;
                NodeAddress *destNodeId = NULL;

                clocktype   startTime;
                clocktype   endTime;

                sprintf(stdErrorString,
                    "Incorrect ALE configuration format\n'%s'\n"
                "ALE <SourceNodeName> <DestNodeName(s)> "
                "<StartTime> <EndTime> [CH<OptionalChannelNum>]\n"
                "'ALE ET HOME 10S 50S CH3' or 'ALE 1 3 10S 50S'\n",
                appInput.inputStrings[i]);
                /*
                Group calls will have multiple destinations, and
                slots will be assigned from left-to-right,
                starting with slot 0 for source/calling node.
                A NET address can only appear as a destination and
                the slots are pre-assigned, based on the node's
                appearance order in ALE-CONFIG-FILE.
                */
                inputString = (char*) MEM_malloc(lineLen);
                memset(inputString,0,lineLen);
                strcpy(inputString, appInput.inputStrings[i]);
                // ALE
                token = IO_GetDelimitedToken(
                            iotoken, inputString, delims, &next);
                // Retrieve next token.
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token)
                {
                    ERROR_ReportWarning(stdErrorString);
                    MEM_free(inputString);
                    continue;
                }

                strcpy(sourceString, token);
                if (MacAleGetNodeNameMapping(
                        token,
                        &sourceNodeId,
                        &sourceInterfaceIndex) == FALSE)
                {
                    sprintf(errorString,
                            "%s\n Non-existent ALE nodeId/name: %s",
                            appInput.inputStrings[i], token);
                    MEM_free(inputString);
                    ERROR_ReportError(errorString);
                }

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL) {

                    // Group calls are modeled as Net calls, so only 1 dest, always.
                    /*
                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token) {
                        ERROR_ReportError(stdErrorString);
                    }

                    destNum = strtoul(token, NULL, 10);
                    */

                    destString[0] = (char *) MEM_malloc(
                            sizeof(char) * MAX_STRING_LENGTH * destNum);

                    destNodeId = (NodeAddress *) MEM_malloc(
                            sizeof(NodeAddress) * destNum);

                    if (!token) {
                        ERROR_ReportError(stdErrorString);
                    }

                    for (count = 0; count < destNum; count++)
                    {
                        token = IO_GetDelimitedToken(iotoken, next, delims, &next);
                        if (MacAleGetNodeNameMapping(
                                token,
                                &(destNodeId[count]),
                                &interfaceIndex) == FALSE)
                        {
                            sprintf(errorString,
                                    "'%s'\nNon-existent ALE nodeId/name: %s",
                                    appInput.inputStrings[i], token);
                            MEM_free(inputString);
                            ERROR_ReportError(errorString);
                        }

                        strcpy(destString[count], token);
                    }

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token) {
                        ERROR_ReportError(stdErrorString);
                    }

                    strcpy(startTimeStr, token);
                    startTime = TIME_ConvertToClock(startTimeStr);

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token) {
                        ERROR_ReportError(stdErrorString);
                    }

                    strcpy(endTimeStr, token);
                    endTime = TIME_ConvertToClock(endTimeStr);

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (token != NULL &&
                        IO_IsStringNonNegativeInteger(token))
                    {
                        requestedCallChannel = strtoul(token, NULL, 10);
                    }
                    else
                    {
                        requestedCallChannel = -1;
                    }
                    /*
                    // Optional keyword CH is used to get optional channel info.
                    if (token != NULL && (token[0] == 'c' || token[0] == 'C')
                        && (token[1] == 'h' || token[1] == 'H') &&
                        strlen(token) >= 3 &&
                        IO_IsStringNonNegativeInteger(&(token[2])))
                    {
                        requestedCallChannel = strtoul(&(token[2]), NULL, 10);
                    } else {
                        requestedCallChannel = -1;
                    }
                    */

                    MacAleSetupCall(
                        node,
                        sourceInterfaceIndex,
                        startTime,
                        endTime,
                        sourceString,
                        destNum,
                        destString,             // destination(s)
                        requestedCallChannel,   // -1 if not specified
                        NULL);                  // initial message to send
#ifdef DEBUG
                    {
                        printf("--------\n");
                        if (destNum > 1)
                        {
                            printf("ALE Group Call:\n");
                        }
                        else {
                            printf("ALE Call:\n");
                        }

                        printf("start time: %s\n", startTimeStr);
                        printf("end time:   %s\n", endTimeStr);
                        printf("src[%ld]: %s\n", sourceNodeId, sourceString);
                        printf("dst[%ld]: %s\n", destNodeId[0], destString[0]);
                        printf("--------\n");
                    }
#endif // DEBUG
                MEM_free(inputString);
                }
#else // ALE_ASAPS_LIB
                ERROR_ReportMissingLibrary(appStr, "ALE/ASAPS");
#endif // ALE_ASAPS_LIB
            }
            else
            if (strcmp(appStr, "HTTP") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                int lineLen = NI_GetMaxLen(&appInput);
                char *inputString = (char*) MEM_malloc(lineLen);
                // For IO_GetDelimitedToken
                char* next;
                char *token;
                const char *delims = " \t\n";
                char iotoken[BIG_STRING_LENGTH];
                NodeAddress clientNodeId;
                Address clientAddr;
                int isNum = 1;

                memset(inputString,0,lineLen);
                strcpy(inputString, appInput.inputStrings[i]);
                // HTTP
                token = IO_GetDelimitedToken(
                            iotoken, inputString, delims, &next);
                // Retrieve next token.
                token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                if (!token)
                {
                    AppInputHttpError();
                }

                IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    token,
                    &clientNodeId,
                    &clientAddr);

                strcpy(sourceString, token);
                node = MAPPING_GetNodePtrFromHash(nodeHash, clientNodeId);
                if (node != NULL)
                {
                    char threshTimeStr[MAX_STRING_LENGTH];
                    char startTimeStr[MAX_STRING_LENGTH];
                    int numServerAddrs;
                    NodeAddress *serverNodeIds;
                    Address *serverAddrs;
                    int count;
                    clocktype threshTime;
                    clocktype startTime;

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token)
                    {
                        AppInputHttpError();
                    }

                    for (UInt32 j = 0; j < strlen(token); j++)
                    {
                        if (!isdigit(token[j]))
                        {
                            isNum = 0;
                            break;
                        }
                    }

                    if (isNum)
                    {
                        numServerAddrs = atoi(token);
                    }
                    else
                    {
                        numServerAddrs = 0;
                    }

                    if ((numServerAddrs == 0) && !strcmp(token, "0"))
                    {
                        AppInputHttpError();
                    }

                    serverNodeIds =
                        (NodeAddress *)
                        MEM_malloc(sizeof(NodeAddress) * numServerAddrs);
                    serverAddrs =
                        (Address *)
                        MEM_malloc(sizeof(Address) * numServerAddrs);

                    if (numServerAddrs == 0)
                    {
                        std::string buf;

                        buf = "No HTTP server configured for HTTP application: ";
                        buf += inputString;
                        buf += "\nconfigured in file: ";
                        buf += appInput.ourName;
                        ERROR_ReportError(buf.c_str());
                    }

                    // DNS Changes
                    std::vector<std::string> serverUrls;
                    for (count = 0; count < numServerAddrs; count++)
                    {
                        token = IO_GetDelimitedToken(
                                    iotoken, next, delims, &next);

                        if (!token)
                        {
                            AppInputHttpError();
                        }

                        IO_AppParseDestString(
                            firstNode,
                            appInput.inputStrings[i],
                            token,
                            &serverNodeIds[count],
                            &serverAddrs[count]);

                        // DNS Changes
                        // if server url is specified then
                        // server nodeIds would not be found out
                        std::string destinationString(token);
                        if (node->appData.appTrafficSender->checkIfValidUrl(
                                            node,
                                            destinationString,
                                            FALSE,
                                            appStr,
                                            &serverAddrs[count]) == TRUE)
                        {
                            isUrl = TRUE;
                            std::string urlConfigured(token);
                            serverUrls.push_back(urlConfigured);
                            node->appData.appTrafficSender->
                                        httpDnsInit(
                                            node,
                                            sourceString,
                                            token,
                                            &serverAddrs[count],
                                            &clientAddr,
                                            &AppHttpUrlSessionStartCallBack);
                        }
                        else if (!serverUrls.empty())
                        {
                            serverUrls[count].clear();
                        }
                    }

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token)
                    {
                        AppInputHttpError();
                    }
                    strcpy(startTimeStr, token);
                    startTime = TIME_ConvertToClock(startTimeStr);

                    token = IO_GetDelimitedToken(iotoken, next, delims, &next);

                    if (!token)
                    {
                        AppInputHttpError();
                    }
                    strcpy(threshTimeStr, token);
                    threshTime = TIME_ConvertToClock(threshTimeStr);

#ifdef DEBUG
                    {
                        char clockStr[MAX_CLOCK_STRING_LENGTH];
                        char addrStr[MAX_STRING_LENGTH];
                        printf("Starting HTTP client with:\n");
                        printf("  client nodeId:  %u\n", clientNodeId);
                        for (count = 0; count < numServerAddrs; count++)
                        {
                            printf("  server nodeId:  %u\n",
                                serverNodeIds[count]);
                            IO_ConvertIpAddressToString(&serverAddrs[count],
                                addrStr);
                            printf("  server address: %s\n", addrStr);
                        }
                        printf("  num servers:    %d\n", numServerAddrs);
                        ctoa(startTime, clockStr);
                        printf("  start time:     %s\n", clockStr);
                        ctoa(threshTime, clockStr);
                        printf("  threshold time: %s\n", clockStr);
                    }
#endif /* DEBUG */

                    MEM_free(serverNodeIds);

                    AppHttpClientInit(
                        node,
                        clientAddr,
                        serverAddrs,
                        numServerAddrs,
                        startTime,
                        threshTime,
                        appNamePtr,
                        serverUrls);


                    MEM_free(inputString);
                }
            }
            else
            if (strcmp(appStr, "HTTPD") == 0)
            {
                char sourceString[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s",
                                sourceString);
                assert(numValues == 1);

                IO_AppParseSourceString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    AppHttpServerInit(node, sourceAddr);
                }
            }
            else
            if (strcmp(appStr, "TRAFFIC-GEN") == 0)
            {
                char profileName[MAX_STRING_LENGTH];
                BOOL isProfileNameSet = FALSE;
                BOOL isMdpEnabled = FALSE;

                char sourceString[MAX_STRING_LENGTH];
                char destString[BIG_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                DestinationType destType = DEST_TYPE_UNICAST;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                sourceString,
                                destString);

                if (numValues != 2)
                {
                    char errorString[3* MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong TRAFFIC-GEN configuration format!\n"
                            TRAFFIC_GEN_USAGE
                            "\n");
                    ERROR_ReportError(errorString);
                }

                // Parse and check given destination address.
                if (strcmp(destString, "255.255.255.255") == 0)
                {
                    IO_AppParseSourceString(
                        firstNode,
                        appInput.inputStrings[i],
                        sourceString,
                        &sourceNodeId,
                        &sourceAddr,
                        NETWORK_IPV4);

                    SetIPv4AddressInfo(&destAddr, ANY_DEST);
                    destType = DEST_TYPE_BROADCAST;
                    destNodeId = ANY_NODEID;
                }
                else
                {
                    //Get source and destination nodeId and address
                    IO_AppParseSourceAndDestStrings(
                        firstNode,
                        appInput.inputStrings[i],
                        sourceString,
                        &sourceNodeId,
                        &sourceAddr,
                        destString,
                        &destNodeId,
                        &destAddr);

                    if (destAddr.networkType == NETWORK_IPV6)
                    {
                        if (IS_MULTIADDR6(destAddr.interfaceAddr.ipv6) ||
                        Ipv6IsReservedMulticastAddress(
                                node, destAddr.interfaceAddr.ipv6) == TRUE)
                        {
                            destType= DEST_TYPE_MULTICAST;
                        }
                    }
                    else if (destAddr.networkType == NETWORK_IPV4)
                    {
                        if (NetworkIpIsMulticastAddress(
                            firstNode, destAddr.interfaceAddr.ipv4))
                        {
                            destType= DEST_TYPE_MULTICAST;
                        }
                    }
                }

                if (destType == DEST_TYPE_MULTICAST
                    || destType == DEST_TYPE_UNICAST)
                {
                    char* token = NULL;
                    char* next = NULL;
                    char delims[] = {" \t"};
                    char iotoken[MAX_STRING_LENGTH];
                    char errorString[MAX_STRING_LENGTH];

                    token = IO_GetDelimitedToken(iotoken,
                                            appInput.inputStrings[i],
                                            delims,
                                            &next);
                    while (token)
                    {
                        if (strcmp(token,"MDP-ENABLED") == 0)
                        {
                            isMdpEnabled = TRUE;
                        }
                        if (strcmp(token,"MDP-PROFILE") == 0)
                        {
                            if (!isMdpEnabled)
                            {
                                sprintf(errorString,
                                    "MDP-PROFILE can not be set "
                                    "without MDP-ENABLED. \n");
                                ERROR_ReportError(errorString);
                            }
                            isProfileNameSet = TRUE;
                            //getting profile name
                            token = IO_GetDelimitedToken(iotoken, next,
                                                        delims, &next);
                            if (token)
                            {
                                sprintf(profileName, "%s", token);
                            }
                            else
                            {
                                sprintf(errorString,
                                    "Please specify MDP-PROFILE name\n");
                                ERROR_ReportError(errorString);
                            }
                        }
                        token = IO_GetDelimitedToken(iotoken,next,delims,&next);
                    }
                }

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    // checks if destination is configured as url and find
                    // url destination address such that application server
                    // can be initialized
                    Address urlDestAddress;
                    isUrl = node->appData.appTrafficSender->
                            appCheckUrlAndGetUrlDestinationAddress(
                                    node,
                                    appInput.inputStrings[i],
                                    isMdpEnabled,
                                    &sourceAddr,
                                    &destAddr,
                                    &destNodeId,
                                    &urlDestAddress,
                                    &AppTrafficGenUrlSessionStartCallback);
                    //Call Traffic-Gen client initialization function
                    if (!isMdpEnabled)
                    {
                        TrafficGenClientInit(node,
                                            appInput.inputStrings[i],
                                            sourceAddr,
                                            destAddr,
                                            destType,
                                            appNamePtr,
                                            destString);
                    }
                    else
                    {
                        TrafficGenClientInit(node,
                                            appInput.inputStrings[i],
                                            sourceAddr,
                                            destAddr,
                                            destType,
                                            appNamePtr,
                                            destString,
                                            isMdpEnabled,
                                            isProfileNameSet,
                                            profileName,
                                            i+1,
                                            nodeInput);
                    }

                    // dns
                    if (isUrl)
                    {
                        memcpy(&destAddr, &urlDestAddress, sizeof(Address));
                    }
                }
                else
                {
                    if (isMdpEnabled)
                    {
                        if (destType == DEST_TYPE_MULTICAST)
                        {
                            // init the MDP layer for nodes lies in the
                            // "firstNode" partition
                            MdpLayerInitForOtherPartitionNodes(firstNode,
                                                            nodeInput,
                                                            &destAddr);
                        }
                    }
                    if (destAddr.networkType != NETWORK_INVALID &&
                        sourceAddr.networkType != destAddr.networkType)
                    {
                        ERROR_ReportErrorArgs(
                                "Traffic-Gen: At node %d, Source and Destination"
                                " IP version mismatch inside %s\n",
                            node->nodeId,
                            appInput.inputStrings[i]);
                    }
                }

                if (!isUrl && ADDR_IsUrl(destString) == TRUE)
                {
                    AppUpdateUrlServerNodeIdAndAddress(
                                                    firstNode,
                                                    destString,
                                                    &destAddr,
                                                    &destNodeId);
                }

                if (destType == DEST_TYPE_UNICAST)
                {
                    // Handle Loopback Address
                    if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                            node,
                                            appInput.inputStrings[i],
                                            destAddr, destNodeId,
                                            sourceAddr, sourceNodeId))
                    {
                        node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                    }

                    if (node != NULL)
                    {
                        TrafficGenServerInit(node);
                    }
                }
                if (isMdpEnabled && destType == DEST_TYPE_UNICAST)
                {
                    // for MDP layer init on destination node
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                    if (node != NULL)
                    {
                        MdpLayerInitForOtherPartitionNodes(node,
                                                        nodeInput,
                                                        NULL,
                                                        TRUE);
                    }
                }
            }
            else
            if (strcmp(appStr, "TRAFFIC-TRACE") == 0)
            {
                char profileName[MAX_STRING_LENGTH];
                BOOL isProfileNameSet = FALSE;
                BOOL isMdpEnabled = FALSE;

                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                memset(&destAddr, 0, sizeof(Address));
                memset(&sourceAddr, 0, sizeof(Address));
                BOOL isDestMulticast;

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                sourceString,
                                destString);

                if (numValues != 2)
                {
                    char errorString[3* MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Wrong TRAFFIC-TRACE configuration format!\n"
                            TRAFFIC_TRACE_USAGE
                            "\n");
                    ERROR_ReportError(errorString);
                }

                APP_CheckMulticastByParsingSourceAndDestString(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr.interfaceAddr.ipv4,
                    destString,
                    &destNodeId,
                    &destAddr.interfaceAddr.ipv4,
                    &isDestMulticast);

                if (strcmp(destString, "255.255.255.255") != 0)//(isDestMulticast)
                {
                    char* token = NULL;
                    char* next = NULL;
                    char delims[] = {" \t"};
                    char iotoken[MAX_STRING_LENGTH];
                    char errorString[MAX_STRING_LENGTH];

                    token = IO_GetDelimitedToken(iotoken,
                                            appInput.inputStrings[i],
                                            delims,
                                            &next);
                    while (token)
                    {
                        if (strcmp(token,"MDP-ENABLED") == 0)
                        {
                            isMdpEnabled = TRUE;
                        }
                        if (strcmp(token,"MDP-PROFILE") == 0)
                        {
                            if (!isMdpEnabled)
                            {
                                sprintf(errorString,
                                    "MDP-PROFILE can not be set "
                                    "without MDP-ENABLED. \n");
                                ERROR_ReportError(errorString);
                            }
                            isProfileNameSet = TRUE;
                            //getting profile name
                            token = IO_GetDelimitedToken(iotoken, next,
                                                        delims, &next);
                            if (token)
                            {
                                sprintf(profileName, "%s", token);
                            }
                            else
                            {
                                sprintf(errorString,
                                    "Please specify MDP-PROFILE name\n");
                                ERROR_ReportError(errorString);
                            }
                        }
                        token = IO_GetDelimitedToken(iotoken,next,delims,&next);
                    }
                }

                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    if (!isMdpEnabled)
                    {
                        TrafficTraceClientInit(node,
                                            appInput.inputStrings[i],
                                            GetIPv4Address(sourceAddr),
                                            GetIPv4Address(destAddr),
                                            isDestMulticast);
                    }
                    else
                    {
                        TrafficTraceClientInit(node,
                                            appInput.inputStrings[i],
                                            GetIPv4Address(sourceAddr),
                                            GetIPv4Address(destAddr),
                                            isDestMulticast,
                                            isMdpEnabled,
                                            isProfileNameSet,
                                            profileName,
                                            i+1,
                                            nodeInput);
                    }
                }
                else
                {
                    if (isMdpEnabled)
                    {
                        if (isDestMulticast)
                        {
                            // init the MDP layer for nodes lies in the
                            // "firstNode" partition
                            MdpLayerInitForOtherPartitionNodes(firstNode,
                                                            nodeInput,
                                                            &destAddr);
                        }
                    }
                }

                if (!isDestMulticast)
                {
                    // Handle Loopback Address
                    if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                            node,
                                            appInput.inputStrings[i],
                                            destAddr, destNodeId,
                                            sourceAddr, sourceNodeId))
                    {
                        node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                    }

                    if (node != NULL)
                    {
                        TrafficTraceServerInit(node);
                    }
                }

                if (isMdpEnabled && !isDestMulticast)
                {
                    // for MDP layer init on destination node
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                    if (node != NULL)
                    {
                        MdpLayerInitForOtherPartitionNodes(node,
                                                        nodeInput,
                                                        NULL,
                                                        TRUE);
                    }
                }
            }
            else if (!strcmp(appStr, "VOIP"))
            {
#ifdef ENTERPRISE_LIB
                char srcStr[MAX_STRING_LENGTH];
                char destStr[MAX_STRING_LENGTH];
                char intervalStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                char option1Str[MAX_STRING_LENGTH];
                char option2Str[MAX_STRING_LENGTH];
                char option3Str[MAX_STRING_LENGTH];

                // For IO_GetDelimitedToken
                char iotoken[MAX_STRING_LENGTH];
                char viopAppString[MAX_STRING_LENGTH];
                char* next;
                char* token;
                char* p;
                const char* delims = " ";

                NodeAddress srcNodeId;
                NodeAddress srcAddr;
                NodeAddress destNodeId;
                NodeAddress destAddr;

                BOOL callAccept = TRUE;
                clocktype packetizationInterval = 0;
                unsigned int bitRatePerSecond = 0;
                clocktype startTime;
                clocktype endTime;
                clocktype interval;
                unsigned tos = APP_DEFAULT_TOS;
                BOOL newSyntax = FALSE;

                unsigned short srcPort;
                unsigned short callSignalPort;
                // Each line for VOIP describes a call, so we can
                // use it to assign ports
                srcPort = (unsigned short) (VOIP_PORT_NUMBER_BASE - (i * 2));
                callSignalPort = srcPort - 1;

                char sourceAliasAddr[MAX_ALIAS_ADDR_SIZE];
                char destAliasAddr[MAX_ALIAS_ADDR_SIZE];
                Node* destNode = NULL;

                char codecScheme[MAX_STRING_LENGTH] = {"G.711"};

                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s %s %s %s %s",
                                srcStr, destStr, intervalStr,
                                startTimeStr, endTimeStr,
                                option1Str, option2Str, option3Str);

                strcpy(viopAppString, appInput.inputStrings[i]);
                p = viopAppString;

                token = IO_GetDelimitedToken(iotoken, p, delims, &next);

                while (token)
                {
                    if (!strcmp(token, "CALL-STATUS") ||
                        !strcmp(token, "ENCODING") ||
                        !strcmp(token, "PACKETIZATION-INTERVAL") ||
                        !strcmp(token, "TOS") ||
                        !strcmp(token, "DSCP") ||
                        !strcmp(token, "PRECEDENCE"))
                    {
                        newSyntax = TRUE;
                        break;
                    }

                    token = IO_GetDelimitedToken(
                                iotoken, next, delims, &next);
                }

                if (!newSyntax)
                {
                    if (retVal < 5 || retVal > 7)
                    {
                        char errStr[MAX_STRING_LENGTH];
                        sprintf(errStr, "Wrong VOIP configuration format!\n");
                        ERROR_ReportError(errStr);
                    }
                    else if ((retVal == 6) || (retVal == 7))
                    {
                        // Process optional argument
                        if (!isdigit(*option1Str))
                        {
                            callAccept = VOIPsetCallStatus(option1Str);
                            if (retVal == 7)
                            {
                                packetizationInterval =
                                    TIME_ConvertToClock(option2Str);
                            }
                            else
                            {
                                packetizationInterval = 0;
                            }
                        }
                        else
                        {
                            packetizationInterval =
                                TIME_ConvertToClock(option1Str);
                            if (retVal == 7)
                            {
                                callAccept = VOIPsetCallStatus(option2Str);
                            }
                        }
                    }
                }
                else // New Syntax
                {
                    int index;
                    packetizationInterval = 0;
                    bitRatePerSecond = 0;
                    char tosType[MAX_STRING_LENGTH] = {0};
                    token = IO_GetDelimitedToken(iotoken, p, delims, &next);

                    for (index = 0; index < 6; index++)
                    {
                        token = IO_GetDelimitedToken(
                                    iotoken, next, delims, &next);
                    }

                    while (token)
                    {
                        int optionValue = VOIPgetOption(token);
                        token = IO_GetDelimitedToken(
                                        iotoken, next, delims, &next);
                        switch (optionValue)
                        {
                            case VOIP_CALL_STATUS:
                            {
                                callAccept = VOIPsetCallStatus(token);
                                break;
                            }
                            case VOIP_ENCODING_SCHEME:
                            {
                                strcpy(codecScheme, token);
                                break;
                            }
                            case VOIP_PACKETIZATION_INTERVAL:
                            {
                                if (!strlen(codecScheme))
                                {
                                    packetizationInterval = 0;
                                }
                                else
                                {
                                    packetizationInterval =
                                            TIME_ConvertToClock(token);
                                }
                                break;
                            }
                            case VOIP_TOS:
                            {
                                strcpy(tosType, "TOS");
                                break;
                            }
                            case VOIP_DSCP:
                            {
                                strcpy(tosType, "DSCP");
                                break;
                            }
                            case VOIP_PRECEDENCE:
                            {
                                strcpy(tosType, "PRECEDENCE");
                                break;
                            }
                            default:
                            {
                                char errStr[MAX_STRING_LENGTH];
                                sprintf(errStr,"Invalid option: %s\n", token);
                                ERROR_ReportError(errStr);
                            }
                        }
                        if ((optionValue == VOIP_TOS) ||
                                        (optionValue == VOIP_DSCP)
                            || (optionValue == VOIP_PRECEDENCE))
                        {
                            if (!(APP_AssignTos(tosType, token, &tos)))
                            {
                                char errStr[MAX_STRING_LENGTH];
                                sprintf(errStr, "Invalid tos option: %s\n",
                                        token);
                                ERROR_ReportError(errStr);
                            }
                        }
                        if (token)
                        {
                            token = IO_GetDelimitedToken(
                                iotoken, next, delims, &next);
                        }
                    }
                }

                IO_AppParseSourceAndDestStrings(
                                        firstNode,
                                        appInput.inputStrings[i],
                                        srcStr,
                                        &srcNodeId,
                                        &srcAddr,
                                        destStr,
                                        &destNodeId,
                                        &destAddr);

                if (APP_ForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        "VOIP",
                        srcNodeId,
                        destNodeId))
                {
                    return;
                }

                // Handle Loopback Address
                if (NetworkIpIsLoopbackInterfaceAddress(destAddr))
                {
                    char errorStr[MAX_STRING_LENGTH] = {0};
                    sprintf(errorStr, "VOIP: Application doesn't support "
                        "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                    ERROR_ReportWarning(errorStr);

                    return;
                }

                node = MAPPING_GetNodePtrFromHash(nodeHash, srcNodeId);
                destNode = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);

                if (node != NULL || destNode != NULL) {
                    VoipReadConfiguration(node, destNode, nodeInput, codecScheme,
                                        packetizationInterval, bitRatePerSecond);
                }

                startTime = TIME_ConvertToClock(startTimeStr);
                endTime = TIME_ConvertToClock(endTimeStr);
                interval = TIME_ConvertToClock(intervalStr);

                if (endTime == 0)
                {
                    BOOL retVal;

                    IO_ReadString(ANY_NODEID, ANY_ADDRESS, nodeInput,
                                    "SIMULATION-TIME", &retVal, endTimeStr);

                    if (retVal == TRUE)
                    {
                        endTime = TIME_ConvertToClock(endTimeStr);
                    }
                    else
                    {
                        ERROR_Assert(FALSE,
                            "SIMULATION-TIME not found in config file.\n");
                    }
                }

                if (interval < MINIMUM_TALKING_TIME)
                {
                    interval = MINIMUM_TALKING_TIME;

                    ERROR_ReportWarning("Minimum value of average talking time "
                        "is one second. default value used here.\n");
                }
                if (destNode != NULL)
                {

                    AppMultimedia* multimedia = destNode->appData.multimedia;

                    if (multimedia)
                    {
                        if (multimedia->sigType == SIG_TYPE_H323)
                        {
                            H323Data* h323 = (H323Data*) multimedia->sigPtr;

                            if (h323 && h323->endpointType == Gatekeeper)
                            {
                                char err[MAX_STRING_LENGTH];
                                sprintf(err,"\nVOIP receiving Node Id %d"
                                    " should not be Gatekeeper. "
                                    "Connection canceled.\n", destNode->nodeId);
                                ERROR_ReportWarning(err);
                                continue;
                            }

                            //Find gatekeeper present or not
                            for (Int32 index = 0; index < nodeInput->numLines; index++)
                            {
                                if (!strcmp(nodeInput->variableNames[index],
                                    "H323-GATEKEEPER") &&
                                    !strcmp(nodeInput->values[index],
                                    "YES"))
                                {
                                    gatekeeperFound = TRUE;
                                    break;
                                }
                            }
                            if (!h323)
                            {
                                H323Init(destNode,
                                    nodeInput,
                                    gatekeeperFound,
                                    callModel,
                                    callSignalPort,
                                    srcPort,
                                    startTime,
                                    destNodeId);

                                h323 = (H323Data*) multimedia->sigPtr;
                            }
                            strcpy(
                                destAliasAddr,
                                h323->h225Terminal->terminalAliasAddr);
                        }
                        else if (multimedia->sigType == SIG_TYPE_SIP)
                        {
                            SipData* sip;
                            if (!multimedia->sigPtr)
                            {
                                SipInit(destNode,
                                        nodeInput,
                                        SIP_UA,
                                        startTime);
                            }

                            sip = (SipData*) multimedia->sigPtr;
                            if (!sip->SipGetOwnAliasAddr())
                            {
                                char err[MAX_STRING_LENGTH];
                                sprintf(err, "No data available in default.sip"
                                    " for node: %d\n", destNode->nodeId);
                                ERROR_Assert(false, err);
                            }
                            strcpy(destAliasAddr, sip->SipGetOwnAliasAddr());
                        }
                    }
                    else
                    {
                        char errorBuf[MAX_STRING_LENGTH] = {0};
                        sprintf(errorBuf,"No Multimedia Signalling Protocol Set At"
                                " Node %d\n Presently"
                                " H323 and SIP are Supported", destNode->nodeId);
                        ERROR_Assert(destNode->appData.multimedia, errorBuf);
                    }
                }

                if (node != NULL)
                {
                    // Initialize the source node's SIP application.
                    // (the source node is on this partition)
                    AppMultimedia* multimedia = node->appData.multimedia;

                    if (multimedia)
                    {
                        if (multimedia->sigType == SIG_TYPE_H323)
                        {
                            H323Data* h323 = (H323Data*) multimedia->sigPtr;
                            gatekeeperFound = FALSE;

                            if (h323 && h323->endpointType == Gatekeeper)
                            {
                                char err[MAX_STRING_LENGTH];
                                sprintf(err,"\nVOIP sending Node Id %d"
                                    " should not be Gatekeeper. "
                                    "Connection canceled.\n", node->nodeId);
                                ERROR_ReportWarning(err);
                                continue;
                            }
                            //Find gatekeeper present or not
                            for (Int32 index = 0; index < nodeInput->numLines; index++)
                            {
                                if (!strcmp(nodeInput->variableNames[index],
                                    "H323-GATEKEEPER") &&
                                    !strcmp(nodeInput->values[index],
                                    "YES"))
                                {
                                    gatekeeperFound = TRUE;
                                    break;
                                }
                            }
                            if (!h323)
                            {
                                H323Init(node,
                                    nodeInput,
                                    gatekeeperFound,
                                    callModel,
                                    callSignalPort,
                                    srcPort,
                                    startTime,
                                    destNodeId);

                                h323 = (H323Data*) multimedia->sigPtr;
                            }
                            else
                            {
                                h323->h225Terminal->terminalCallSignalTransAddr.port =
                                    callSignalPort;
                            }
                            strcpy(sourceAliasAddr,
                                h323->h225Terminal->terminalAliasAddr);
                        }
                        else if (multimedia->sigType == SIG_TYPE_SIP)
                        {
                            SipData* sip;

                            if (!multimedia->sigPtr)
                            {
                                SipInit(node, nodeInput, SIP_UA, startTime);
                            }
                            sip = (SipData*) multimedia->sigPtr;
                            strcpy(sourceAliasAddr, sip->SipGetOwnAliasAddr());
                        }
                    }
                    else
                    {
                        char errorBuf[MAX_STRING_LENGTH] = {0};
                        sprintf(errorBuf,"No Multimedia Signalling Protocol Set At"
                                " Node %d\n Presently"
                                " H323 and SIP are Supported", node->nodeId);
                        ERROR_Assert(node->appData.multimedia, errorBuf);

                    }

                    if (multimedia)
                    {
                        // src and dest node's signaling protocols have to match
                        // Determine the alias for the destination (because
                        // destNode may be NULL -- on a different partition)
                        if (multimedia->sigType == SIG_TYPE_SIP)
                        {
                            SIPDATA_fillAliasAddrForNodeId (destNodeId, nodeInput,
                                destAliasAddr);
                        }
                        else if (multimedia->sigType == SIG_TYPE_H323)
                        {
                            // Determine the alias for our destination.
                            H323ReadEndpointList(node->partitionData,
                                destNodeId, nodeInput, destAliasAddr);
                        }
                    }

                    VoipCallInitiatorInit(
                        node,
                        srcAddr,
                        destAddr,
                        callSignalPort,
                        srcPort,
                        sourceAliasAddr,
                        destAliasAddr,
                        appNamePtr,
                        interval,
                        startTime,
                        endTime,
                        packetizationInterval,
                        bitRatePerSecond,
                        tos);
                }

                if (callAccept)
                {
                    if (destNode)
                    {
                        VoipAddNewCallToList(
                                destNode,
                                srcAddr,
                                srcPort,
                                interval,
                                startTime,
                                endTime,
                                packetizationInterval);

                        VoipCallReceiverInit(destNode,
                                            destAddr,
                                            srcAddr,
                                            srcPort,
                                            destAliasAddr,
                                            appNamePtr,
                                            callSignalPort,
                                            INVALID_ID,
                                            packetizationInterval,
                                            bitRatePerSecond,
                                            tos);
                    }
                }
#else // ENTERPRISE_LIB
                ERROR_ReportMissingLibrary(appStr, "Multimedia & Enterprise");
#endif // ENTERPRISE_LIB
            }
            else if (strcmp(appStr, "GSM") == 0)
            {
#ifdef CELLULAR_LIB
                char startTimeStr[MAX_STRING_LENGTH];
                char callDurationTimeStr[MAX_STRING_LENGTH];
                char simEndTimeStr[MAX_STRING_LENGTH];
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char errorString[MAX_STRING_LENGTH];

                unsigned int *info;

                clocktype callStartTime;
                clocktype callEndTime;
                clocktype simEndTime;
                clocktype *endTime;

                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;
                NodeAddress destNodeId;
                NodeAddress destAddr;

                Message *callStartTimerMsg;
                Message *callEndTimerMsg;

                // For Mobile Station nodes only

                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s",
                                sourceString,
                                destString,
                                startTimeStr,
                                callDurationTimeStr);

                if (retVal != 4)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(errorString,
                            "Incorrect GSM call setup: %s\n"
                            "'GSM <src> <dest> <call start time> <call duration>'",
                            appInput.inputStrings[i]);
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                // TBD : for loopBack

                IO_AppForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        sourceNodeId,
                        destNodeId);

                // Should check if the source & dest nodes are
                // configured for GSM (check interfaces) ???

                // Call Terminating(MT) node
                if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                    == FALSE)
                {
                sprintf(errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        destNodeId);
                    ERROR_ReportError(errorString);
                }

                // Call Originating(MO) node
                if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                    == FALSE)
                {
                sprintf(errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        sourceNodeId);
                    ERROR_ReportError(errorString);
                }
                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node == NULL)
                {
                    // not on this partition
                    continue;
                }

                callStartTime = TIME_ConvertToClock(startTimeStr);
                callEndTime = callStartTime +
                    TIME_ConvertToClock(callDurationTimeStr);
                callStartTime -= getSimStartTime(firstNode);
                callEndTime -= getSimStartTime(firstNode);

                simEndTime = node->partitionData->maxSimClock;

                // printf("GSM_MS[%ld] -> %d, Call start %s, duration %s\n",
                //     node->nodeId, destNodeId,
                //     startTimeStr, callDurationTimeStr);

                if (callStartTime <= 0 || callStartTime >= simEndTime ||
                    callEndTime <= 0 || callEndTime >= simEndTime)
                {
                    sprintf(errorString,
                        "Illegal Call Time:\n'%s'"
                        "\nTotal Simulation Time = %s",
                        appInput.inputStrings[i], simEndTimeStr);
                    ERROR_ReportError(errorString);
                }

                if ((callEndTime - callStartTime) < GSM_MIN_CALL_DURATION)
                {
                    char clockStr2[MAX_STRING_LENGTH];
                    char clockStr3[MAX_STRING_LENGTH];


                    ctoa(GSM_MIN_CALL_DURATION / SECOND, clockStr2);
                    ctoa(GSM_AVG_CALL_DURATION / SECOND, clockStr3);

                    callEndTime = callStartTime + GSM_AVG_CALL_DURATION;

                    sprintf(errorString,
                        "Mininum call duration is %s second(s).\n"
                        "Default value %sS will be used.",
                        clockStr2, clockStr3);

                    ERROR_ReportWarning(errorString);
                }

                // Setup Call Start Timer with info on dest & call end time
                callStartTimerMsg = MESSAGE_Alloc(node,
                                        NETWORK_LAYER,
                                        0,
                                        MSG_NETWORK_GsmCallStartTimer);

                MESSAGE_InfoAlloc(node,
                    callStartTimerMsg,
                    sizeof(unsigned int) + sizeof(clocktype));
                info = (unsigned int*) MESSAGE_ReturnInfo(callStartTimerMsg);
                *info = (unsigned int) destNodeId;
                endTime = (clocktype *)(info + sizeof(unsigned int));
                *endTime = callEndTime;
                callStartTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

                MESSAGE_Send(node,
                        callStartTimerMsg,
                        callStartTime);

                // Set a timer to initiate call disconnect
                callEndTimerMsg = MESSAGE_Alloc(node,
                                    NETWORK_LAYER,
                                    0,
                                    MSG_NETWORK_GsmCallEndTimer);

                callEndTimerMsg->protocolType = NETWORK_PROTOCOL_GSM;

                MESSAGE_Send(node,
                        callEndTimerMsg,
                        callEndTime);
#else // CELLULAR_LIB
                ERROR_ReportMissingLibrary(appStr, "Cellular");
#endif // CELLULAR_LIB

            } // End of GSM //

    //StartVBR
            else if (strcmp(appStr, "VBR") == 0) {
                char profileName[MAX_STRING_LENGTH] = "/0";
                BOOL isProfileNameSet = FALSE;
                BOOL isMdpEnabled = FALSE;
                NodeAddress clientAddr;
                NodeAddress serverAddr;
                int itemSize;
                clocktype meanInterval;
                clocktype startTime;
                clocktype endTime;
                NodeAddress clientId;
                NodeAddress serverId;
                char clientString[MAX_STRING_LENGTH];
                char serverString[MAX_STRING_LENGTH];
                char meanIntervalString[MAX_STRING_LENGTH];
                char startTimeString[MAX_STRING_LENGTH];
                char endTimeString[MAX_STRING_LENGTH];
                unsigned tos = APP_DEFAULT_TOS;
                char optionToken1[MAX_STRING_LENGTH];
                char optionToken2[MAX_STRING_LENGTH];
                char optionToken3[MAX_STRING_LENGTH];
                char optionToken4[MAX_STRING_LENGTH];
                char optionToken5[MAX_STRING_LENGTH];

                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %s %s %s %s %s %s %s %s",
                                clientString,
                                serverString,
                                &itemSize,
                                meanIntervalString,
                                startTimeString,
                                endTimeString,
                                optionToken1,
                                optionToken2,
                                optionToken3,
                                optionToken4,
                                optionToken5);

                switch (retVal)
                {
                    case 6:
                        break;
                    case 7:
                        if (strcmp(optionToken1, "MDP-ENABLED") == 0)
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                        }// else fall through default
                    case 8:
                        if (APP_AssignTos(optionToken1, optionToken2, &tos))
                        {
                            break;
                        }
                        else if (strcmp(optionToken1, "MDP-ENABLED") == 0)
                        {
                        isMdpEnabled = TRUE;
                        isProfileNameSet = FALSE;
                            break;
                        }// else fall through default
                    case 9:
                        if (strcmp(optionToken1, "MDP-ENABLED") == 0
                            && strcmp(optionToken2, "MDP-PROFILE") == 0
                            && strcmp(optionToken3, "") != 0)
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = TRUE;
                            sprintf(profileName, "%s", optionToken3);
                            break;
                        }
                        else if (APP_AssignTos(optionToken1,optionToken2,&tos)
                                && strcmp(optionToken3, "MDP-ENABLED") == 0)
                        {
                            isMdpEnabled = TRUE;
                            isProfileNameSet = FALSE;
                            break;
                        }// else fall through default
                    case 11:
                        if (APP_AssignTos(optionToken1, optionToken2, &tos))
                        {
                            if (strcmp(optionToken3, "MDP-ENABLED") == 0
                                && strcmp(optionToken4, "MDP-PROFILE") == 0
                                && strcmp(optionToken5, "") != 0)
                            {
                                isMdpEnabled = TRUE;
                                isProfileNameSet = TRUE;
                                sprintf(profileName, "%s", optionToken5);
                                break;
                            }
                        }// else fall through default
                    default:
                    {
                        ERROR_ReportError(
                                "Wrong VBR configuration format!\n"
                                "VBR <src> <dest> <item size> "
                                "<interval> <start time> <end time> "
                                "[TOS <tos-value> | DSCP <dscp-value>"
                                " | PRECEDENCE <precedence-value>] "
                                "[MDP-ENABLED [MDP-PROFILE <profile name>] ]\n"
                                "Illegal transition");
                    }
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    clientString,
                    &clientId,
                    &clientAddr,
                    serverString,
                    &serverId,
                    &serverAddr);

                node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);

                meanInterval = TIME_ConvertToClock(meanIntervalString);
                startTime = TIME_ConvertToClock(startTimeString);
                endTime = TIME_ConvertToClock(endTimeString);

                if (node != NULL) {
                    if (isMdpEnabled)
                    {
                        VbrInit(node,
                            clientAddr,
                            serverAddr,
                            itemSize,
                            meanInterval,
                            startTime,
                            endTime,
                            tos,
                            appNamePtr,
                            isMdpEnabled,
                            isProfileNameSet,
                            profileName,
                            i+1,
                            nodeInput);
                    }
                    else
                    {
                        VbrInit(node,
                            clientAddr,
                            serverAddr,
                            itemSize,
                            meanInterval,
                            startTime,
                            endTime,
                            tos,
                            appNamePtr);
                    }
                }

                node = MAPPING_GetNodePtrFromHash(nodeHash, serverId);
                if (node != NULL) {
                    if (isMdpEnabled)
                    {
                        VbrInit(node,
                                clientAddr,
                                serverAddr,
                                itemSize,
                                meanInterval,
                                startTime,
                                endTime,
                                tos,
                                appNamePtr,
                                isMdpEnabled,
                                isProfileNameSet,
                                profileName,
                                i+1,
                                nodeInput);

                        // for MDP layer init on destination node
                        MdpLayerInitForOtherPartitionNodes(node,
                                                        nodeInput,
                                                        NULL,
                                                        TRUE);
                    }
                    else
                    {
                        VbrInit(node,
                                clientAddr,
                                serverAddr,
                                itemSize,
                                meanInterval,
                                startTime,
                                endTime,
                                tos,
                                appNamePtr);
                    }
                }
            }
    //EndVBR
            else if (strcmp(appStr, "CELLULAR-ABSTRACT-APP") == 0)
            {
#ifdef CELLULAR_LIB
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char appDurationTimeStr[MAX_STRING_LENGTH];
                char simEndTimeStr[MAX_STRING_LENGTH];
                char appTypeString[MAX_STRING_LENGTH];
                char errorString[MAX_STRING_LENGTH];

                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;
                NodeAddress destNodeId;
                NodeAddress destAddr;
                clocktype appStartTime;
                clocktype appEndTime;
                clocktype appDuration;
                clocktype simEndTime;
                CellularAbstractApplicationType appType =
                                                CELLULAR_ABSTRACT_VOICE_PHONE;
                short numChannelReq;
                float bandwidthReq;

                CellularAbstractApplicationLayerData *appCellularVar;

                // For Mobile Station nodes only
                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s %s %f",
                                sourceString,
                                destString,
                                startTimeStr,
                                appDurationTimeStr,
                                appTypeString,
                                &bandwidthReq);

                //this one is used when application is specified by  channel #
                numChannelReq = 1;

                if (retVal != 6)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(
                        errorString,
                        "Incorrect CELLULAR ABSTRACT APP setup: %s\n"
                        "'CELLULAR-ABSTRACT-APP <src> <dest> <app start time>"
                        "<appcall duration> <app type> <bandwidth required>'",
                        appInput.inputStrings[i]);
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                IO_AppForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        sourceNodeId,
                        destNodeId);

                // Should check if the source & dest nodes are
                // configured for CELLULAR MS or aggregated node???

                // APP Terminating(MT) node
                if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                    == FALSE)
                {
                sprintf(
                        errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        destNodeId);
                    ERROR_ReportError(errorString);
                }

                // APP Originating(MO) node
                if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                    == FALSE)
                {
                sprintf(
                    errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        sourceNodeId);
                    ERROR_ReportError(errorString);
                }

                appStartTime = TIME_ConvertToClock(startTimeStr);
                appEndTime = appStartTime +
                    TIME_ConvertToClock(appDurationTimeStr);

                simEndTime = firstNode->partitionData->maxSimClock;
#if 0
                printf(
                    "CELLULAR_MS[%ld] -> %d, %s start %s, "
                    "duration %s numchannel %d bandwidth %f\n",
                    node->nodeId, destNodeId, appTypeString,
                    startTimeStr, appDurationTimeStr,
                    numChannelReq,bandwidthReq);
#endif /* 0 */


                if (appStartTime <= 0 || appStartTime >= simEndTime ||
                    appEndTime <= 0 || appEndTime >= simEndTime)
                {
                    TIME_PrintClockInSecond(simEndTime, simEndTimeStr);
                    sprintf(
                        errorString,
                        "Illegal Call Time:\n'%s'"
                        "\nTotal Simulation Time = %s",
                        appInput.inputStrings[i], simEndTimeStr);
                    ERROR_ReportError(errorString);
                }

                appDuration = TIME_ConvertToClock(appDurationTimeStr);

                if (strcmp(appTypeString,"VOICE") == 0)
                {
                    appType = CELLULAR_ABSTRACT_VOICE_PHONE;
                }
                else if (strcmp(appTypeString,"VIDEO-PHONE") == 0)
                {
                    appType=CELLULAR_ABSTRACT_VIDEO_PHONE;
                }
                else if (strcmp(appTypeString,"TEXT-MAIL") == 0)
                {
                    appType = CELLULAR_ABSTRACT_TEXT_MAIL;
                }
                else if (strcmp(appTypeString,"PICTURE-MAIL") == 0)
                {
                    appType = CELLULAR_ABSTRACT_PICTURE_MAIL;
                }
                else if (strcmp(appTypeString,"ANIMATION-MAIL") == 0)
                {
                    appType = CELLULAR_ABSTRACT_ANIMATION_MAIL;
                }
                else if (strcmp(appTypeString,"WEB") == 0)
                {
                    appType = CELLULAR_ABSTRACT_WEB;
                }
                else
                {
                    sprintf(
                        errorString,
                        "Illegal App type:\n'%s'",
                        appTypeString);
                    ERROR_ReportError(errorString);
                }

                node=MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL) {
                    //update the app list
                    //CellularAbstractApplicationLayerData *appCellularVar;
                    appCellularVar =
                        (CellularAbstractApplicationLayerData *)
                        node->appData.appCellularAbstractVar;
                    numChannelReq = 1;//used when bandwidth is used

                    CellularAbstractAppInsertList(
                        node,
                        &(appCellularVar->appInfoOriginList),
                        appCellularVar->numOriginApp,
                        sourceNodeId, destNodeId,
                        appStartTime, appDuration,
                        appType, numChannelReq,
                        bandwidthReq, TRUE);
                }
#else // CELLULAR_LIB
                ERROR_ReportMissingLibrary(appStr, "Cellular");
#endif // CELLULAR_LIB
            } // end of abstrct cellular app
            else if (strcmp(appStr, "UMTS-CALL") == 0
                     || strcmp(appStr, "PHONE-CALL") == 0) // for backward compatibility
            {
#if defined(UMTS_LIB)
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char avgTalkTimeStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char appDurationTimeStr[MAX_STRING_LENGTH];
                char simEndTimeStr[MAX_STRING_LENGTH];
                char errorString[MAX_STRING_LENGTH];

                NodeAddress sourceNodeId;
                NodeAddress sourceAddr;
                NodeAddress destNodeId;
                NodeAddress destAddr;
                clocktype appStartTime;
                clocktype appEndTime;
                clocktype appDuration;
                clocktype simEndTime;
                clocktype avgTalkTime;

                AppUmtsCallLayerData* appUmtsCallLayerData;

                //initilize all the node's application data info
                if (umtsCallApp == FALSE)
                {
                    Node* nextNode  = NULL;
                    nextNode=firstNode->partitionData->firstNode;
                    while (nextNode != NULL)
                    {
                        //check to see if we've initialized
                        //if not, then do so
                        appUmtsCallLayerData =
                            (AppUmtsCallLayerData*)
                                nextNode->appData.umtscallVar;

                        if (appUmtsCallLayerData == NULL)
                        {
                            // need to initialize
                            AppUmtsCallInit(nextNode, nodeInput);
                        }

                        // printf("node %d:app init ing\n", nextNode->nodeId);
                        nextNode = nextNode->nextNodeData;
                    }
                    umtsCallApp = TRUE;
                }

                // For Mobile Station nodes only
                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %s %s %s",
                                sourceString,
                                destString,
                                avgTalkTimeStr,
                                startTimeStr,
                                appDurationTimeStr);

                if (retVal != 5)
                {
                    char errorString[MAX_STRING_LENGTH];
                    sprintf(
                        errorString,
                        "Incorrect UMTS-CALL setup: %s\n"
                        "'UMTS-CALL <src> <dest> <avg talk time>"
                        "<app start time> <appcall duration>'",
                        appInput.inputStrings[i]);
                    ERROR_ReportError(errorString);
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);

                IO_AppForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        sourceNodeId,
                        destNodeId);

                // APP Terminating(MT) node
                if (PARTITION_NodeExists(firstNode->partitionData, destNodeId)
                    == FALSE)
                {
                    sprintf(
                        errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        destNodeId);
                    ERROR_ReportError(errorString);
                }

                // APP Originating(MO) node
                if (PARTITION_NodeExists(firstNode->partitionData, sourceNodeId)
                    == FALSE)
                {
                    sprintf(
                        errorString,
                        "%s: Node %d does not exist",
                        appInput.inputStrings[i],
                        sourceNodeId);
                    ERROR_ReportError(errorString);
                }

                appStartTime = TIME_ConvertToClock(startTimeStr);
                appEndTime = appStartTime +
                    TIME_ConvertToClock(appDurationTimeStr);
                avgTalkTime = TIME_ConvertToClock(avgTalkTimeStr);
                simEndTime = firstNode->partitionData->maxSimClock;


                if (appStartTime <= 0 || appStartTime >= simEndTime ||
                    appEndTime <= 0 || appEndTime >= simEndTime)
                {
                    TIME_PrintClockInSecond(simEndTime, simEndTimeStr);
                    sprintf(
                        errorString,
                        "Illegal Call Time:\n'%s'"
                        "\nTotal Simulation Time (s) = %s",
                        appInput.inputStrings[i], simEndTimeStr);
                    ERROR_ReportError(errorString);
                }

                appDuration = TIME_ConvertToClock(appDurationTimeStr);

                node=MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL) {
                    // update the app list

                    appUmtsCallLayerData =
                        (AppUmtsCallLayerData *)
                        node->appData.umtscallVar;

                    AppUmtsCallInsertList(
                        node,
                        &(appUmtsCallLayerData->appInfoOriginList),
                        appUmtsCallLayerData->numOriginApp,
                        sourceNodeId, destNodeId,
                        avgTalkTime,
                        appStartTime, appDuration,
                        TRUE);
                }
#else // UMTS_LIB
                 ERROR_ReportMissingLibrary(appStr, "UMTS Libraries");
#endif // UMTS_LIB
            } // end of Umts Call
    //StartSuperApplication
            else if (strcmp(appStr, "SUPER-APPLICATION") == 0) {
                Address clientAddr;
                Address serverAddr;
                NodeAddress clientId;
                NodeAddress serverId;
                char clientString[MAX_STRING_LENGTH];
                char serverString[BIG_STRING_LENGTH];

                BOOL wasFound;
                char simulationTimeStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                int numberOfReturnValue = 0;
                clocktype simulationTime = 0;
                double simTime = 0.0;
                clocktype sTime = 0;

                // read in the simulation time from config file
                IO_ReadString( ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "SIMULATION-TIME",
                            &wasFound,
                            simulationTimeStr);

                // read-in the simulation-time, convert into second, and store in simTime variable.
                numberOfReturnValue = sscanf(simulationTimeStr, "%s %s", startTimeStr, endTimeStr);
                if (numberOfReturnValue == 1) {
                    simulationTime = (clocktype) TIME_ConvertToClock(startTimeStr);
                } else {
                    sTime = (clocktype) TIME_ConvertToClock(startTimeStr);
                    simulationTime = (clocktype) TIME_ConvertToClock(endTimeStr);
                    simulationTime -= sTime;
                }

                TIME_PrintClockInSecond(simulationTime, simulationTimeStr);
                simTime = atof(simulationTimeStr);

                BOOL isStartArray = FALSE;
                BOOL isUNI = FALSE;
                BOOL isRepeat = FALSE;
                BOOL isSources = FALSE;
                BOOL isDestinations = FALSE;
                BOOL isConditions = FALSE;
                BOOL isSame = FALSE;
                BOOL isLoopBack   = FALSE;

                // check if the inputStrings has enable startArray, repeat, source or denstination.
                SuperApplicationCheckInputString(&appInput,
                                                i,
                                                &isStartArray,
                                                &isUNI,
                                                &isRepeat,
                                                &isSources,
                                                &isDestinations,
                                                &isConditions);
                if (isConditions){
                    SuperApplicationHandleConditionsInputString(&appInput, i);
                }
                if (isStartArray){
                    SuperApplicationHandleStartTimeInputString(nodeInput, &appInput, i, isUNI);
                }
                if (isSources){
                    SuperApplicationHandleSourceInputString(nodeInput, &appInput, i);
                }
                if (isDestinations){
                    SuperApplicationHandleDestinationInputString(nodeInput, &appInput, i);
                }
                if (isRepeat){
                    retVal = sscanf(appInput.inputStrings[i],
                                    "%*s %s %s",
                                    clientString, serverString);

                    IO_AppParseSourceAndDestStrings(
                        firstNode,
                        appInput.inputStrings[i],
                        clientString,
                        &clientId,
                        &clientAddr,
                        serverString,
                        &serverId,
                        &serverAddr);

                    node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
                    SuperApplicationHandleRepeatInputString(node, nodeInput, &appInput, i, isUNI, simTime);
                }

                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                clientString, serverString);

                if (retVal != 2) {
                    ERROR_ReportError(
                            "Wrong SUPER-APPLICATION configuration format!\n"
                            "SUPER-APPLICATION Client Server\n"
                            "Illegal transition");
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    clientString,
                    &clientId,
                    &clientAddr,
                    serverString,
                    &serverId,
                    &serverAddr);

                if (clientAddr.networkType == NETWORK_IPV6
                    || serverAddr.networkType == NETWORK_IPV6)
                {
                    ERROR_ReportError("SUPER-APPLICATION is not Supported for "
                        "IPV6");
                }

                if (APP_ForbidSameSourceAndDest(
                        appInput.inputStrings[i],
                        "SUPER-APPLICATION",
                        clientId,
                        serverId))
                {
                        isSame = TRUE;
                }

                if (isSame == FALSE){
                    // Handle Loopback Address
                    // TBD : must be handled by designner
                    if (NetworkIpIsLoopbackInterfaceAddress(serverAddr.interfaceAddr.ipv4))
                    {
                        char errorStr[5 * MAX_STRING_LENGTH] = {0};
                        sprintf(errorStr, "SUPER-APPLICATION : Application doesn't support "
                                "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                        ERROR_ReportWarning(errorStr);
                        isLoopBack = TRUE;

                    }

                    if (isLoopBack == FALSE){
                        node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);

                        if (node != NULL) {

                            // dns check and set destAddr to 0 if server url
                            // present
                            std::string destinationString(serverString);
                            if (node->appData.appTrafficSender->
                                                checkIfValidUrl(
                                                        node,
                                                        destinationString,
                                                        FALSE,
                                                        appStr,
                                                        &serverAddr) == TRUE)
                            {
                                // MDP is sent false above as MDP check is
                                // done later; currently destAddr will be set
                                // to 0 if serverUrl present
                                isUrl = TRUE;
                            }
                            SuperApplicationInit(
                                                node,
                                                clientAddr.interfaceAddr.ipv4,
                                                serverAddr.interfaceAddr.ipv4,
                                                appInput.inputStrings[i],
                                                i,
                                                nodeInput,
                                                i+1); //for mdp unique id
                        }

                        if (NetworkIpIsMulticastAddress(node,
                                            serverAddr.interfaceAddr.ipv4))
                        {
                        SuperAppConfigurationData* multicastServ =
                                            new SuperAppConfigurationData;

                        // Read the Multicast SuperApp configuration param
                        // and save it for later use in server init.
                        SuperAppReadMulticastConfiguration(
                                            serverAddr.interfaceAddr.ipv4,
                                            clientAddr.interfaceAddr.ipv4,
                                            appInput.inputStrings[i],
                                            multicastServ,
                                            i+1);

                        multicastServ->appFileIndex = i;

                        if (!node && multicastServ->isMdpEnabled)
                        {
                                // init the MDP layer for nodes lies in the
                                // "firstNode" partition
                                MdpLayerInitForOtherPartitionNodes(
                                                            firstNode,
                                                            nodeInput,
                                                            &serverAddr);
                        }

                        if (firstNode->appData.superAppconfigData == NULL)
                        {
                            ListInit(firstNode,
                                    &firstNode->appData.superAppconfigData);
                        }
                        // Insert SuperAppConfigurationData into the list
                        ListInsert(
                            firstNode,
                            firstNode->appData.superAppconfigData,
                            firstNode->getNodeTime(),
                            (void *)multicastServ);

                        Node* nextNode = firstNode->nextNodeData;
                        while (nextNode != NULL)
                        {
                            if (nextNode->appData.superAppconfigData ==
                                                                        NULL)
                            {
                                ListInit(nextNode,
                                    &nextNode->appData.superAppconfigData);
                            }
                            // Insert SuperAppConfigurationData into the list
                            ListInsert(
                                nextNode,
                                nextNode->appData.superAppconfigData,
                                nextNode->getNodeTime(),
                                (void *)multicastServ);
                            nextNode = nextNode->nextNodeData;
                        }
                        }
                        // dns if serverurl present then server already
                        // initialized in dynamic address initialization
                        // for super-app
                        else if (!isUrl)
                        {
                            // for parallel case
                            if (ADDR_IsUrl(serverString) == TRUE)
                            {
                                AppUpdateUrlServerNodeIdAndAddress(
                                                        firstNode,
                                                        serverString,
                                                        &serverAddr,
                                                        &serverId);
                            }
                            node =
                            MAPPING_GetNodePtrFromHash(nodeHash, serverId);
                            if (node != NULL) {
                                SuperApplicationInit(
                                            node,
                                            clientAddr.interfaceAddr.ipv4,
                                            serverAddr.interfaceAddr.ipv4,
                                            appInput.inputStrings[i],
                                            i,
                                            nodeInput,
                                            i+1);
                            }
                        }
                    }
                }
            }
    //EndSuperApplication
            //Bug fix 5959
            else if (strcmp(appStr, "THREADED-APP") == 0)
            {
#ifdef MILITARY_UNRESTRICTED_LIB
                NodeAddress clientAddr;
                NodeAddress serverAddr;
                NodeAddress clientId;
                NodeAddress serverId;
                char clientString[MAX_STRING_LENGTH];
                char serverString[MAX_STRING_LENGTH];
                BOOL isDestMulticast = FALSE;
                BOOL isSimulation = FALSE;
                BOOL wasFound;
                unsigned long sessionId = 1;
                char simulationTimeStr[MAX_STRING_LENGTH];
                clocktype simulationTime = 0;
                double simTime = 0.0;
                char sessionIdStr[MAX_STRING_LENGTH];

                sprintf(sessionIdStr, " %s %ld ", "SESSION-ID",sessionId);

                // read in the simulation time from config file
                IO_ReadString( ANY_NODEID,
                            ANY_ADDRESS,
                            nodeInput,
                            "SIMULATION-TIME",
                            &wasFound,
                            simulationTimeStr);

                // read-in the simulation-time, convert into second, and store in simTime variable.
                simulationTime = (clocktype) TIME_ConvertToClock(simulationTimeStr);
                TIME_PrintClockInSecond(simulationTime, simulationTimeStr);
                simTime = atof(simulationTimeStr);

                retVal = sscanf(appInput.inputStrings[i],
                                "%*s %s %s",
                                clientString, serverString);

                if (retVal != 2) {
                    printf("configuration is \n%s\n", appInput.inputStrings[i]);
                    fprintf(stderr,
                            "Wrong THREADED-APP configuration format!\n"
                            "THREADED-APP Client Server                        \n");
                    ERROR_ReportError("Illegal transition");
                }

                APP_CheckMulticastByParsingSourceAndDestString(
                    firstNode,
                    appInput.inputStrings[i],
                    clientString,
                    &clientId,
                    &clientAddr,
                    serverString,
                    &serverId,
                    &serverAddr,
                    &isDestMulticast);

                if (!isDestMulticast &&
                    APP_ForbidSameSourceAndDest(
                    appInput.inputStrings[i],
                    "THREADED-APP",
                    clientId,
                    serverId))
                {
                    return;
                }
                else
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, clientId);
                    if (node != NULL)
                    {
                        ThreadedAppHandleRepeatInputString(node, nodeInput, &appInput, i, simTime);

                        // Handle Loopback Address
                        // TBD : must be handled by designner
                        if (NetworkIpIsLoopbackInterfaceAddress(serverAddr))
                        {
                            char errorStr[INPUT_LENGTH] = {0};
                            sprintf(errorStr, "THREADED-APP : Application doesn't support "
                                "Loopback Address!\n  %s\n", appInput.inputStrings[i]);

                            ERROR_ReportWarning(errorStr);

                            return;
                        }
                        ThreadedAppDynamicInit(node, appInput.inputStrings[i], i, sessionId);
                        sessionId++;
                    }
                    else
                    {
                        // Check for server
                        node = MAPPING_GetNodePtrFromHash(nodeHash, serverId);
                        if (node != NULL)
                        {
                            ThreadedAppHandleRepeatInputString(node, nodeInput, &appInput, i, simTime);
                            ThreadedAppDynamicInit(node, appInput.inputStrings[i], i, sessionId);
                            sessionId++;
                        }
                    }
                }
#else /* MILITARY_UNRESTRICTED_LIB */
                ERROR_ReportMissingLibrary(appStr, "MILITARY");
#endif /* MILITARY_UNRESTRICTED_LIB */
            }
    //EndSuperApplication
    //InsertPatch APP_INIT_CODE
#ifdef CYBER_LIB
            else if (!strcmp(name.c_str(), "DOS"))
            {
                APP_InitializeCyberApplication(
                    firstNode,
                    appInput.inputStrings[i],
                    FALSE);
            }
            else if (!strcmp(name.c_str(), "JAMMER"))
            {
                APP_InitializeCyberApplication(
                    firstNode,
                    appInput.inputStrings[i],
                    FALSE);
            }
            else if (!strcmp(name.c_str(), "PORTSCAN"))
            {
                APP_InitializeCyberApplication(
                    firstNode,
                    appInput.inputStrings[i],
                    FALSE);
            }
            else if (!strcmp(name.c_str(), "NETSCAN"))
            {
                APP_InitializeCyberApplication(
                    firstNode,
                    appInput.inputStrings[i],
                    FALSE);
            }
            /*else if (!strcmp(appStr, "SIGINT"))
            {
                APP_InitializeCyberApplication(firstNode,
                        appInput.inputStrings[i], FALSE);
            }*/
#endif
            else
            if (strcmp(appStr, "ZIGBEEAPP") == 0)
            {
#ifdef SENSOR_NETWORKS_LIB
                char sourceString[MAX_STRING_LENGTH];
                char destString[MAX_STRING_LENGTH];
                char intervalStr[MAX_STRING_LENGTH];
                char startTimeStr[MAX_STRING_LENGTH];
                char endTimeStr[MAX_STRING_LENGTH];
                int itemsToSend;
                int itemSize;
                NodeAddress sourceNodeId;
                Address sourceAddr;
                NodeAddress destNodeId;
                Address destAddr;
                unsigned tos = APP_DEFAULT_TOS;
                char optionToken1[MAX_STRING_LENGTH];
                char optionToken2[MAX_STRING_LENGTH];
                char optionToken3[MAX_STRING_LENGTH];

                numValues = sscanf(appInput.inputStrings[i],
                                "%*s %s %s %d %d %s %s %s %s %s %s ",
                                sourceString,
                                destString,
                                &itemsToSend,
                                &itemSize,
                                intervalStr,
                                startTimeStr,
                                endTimeStr,
                                optionToken1,
                                optionToken2,
                                optionToken3);

                switch (numValues)
                {
                    case 7:
                        break;
                    case 9:
                        if (!strcmp(optionToken1, "PRECEDENCE"))
                        {
                            if (APP_AssignTos(optionToken1, optionToken2, &tos))
                            {
                                break;
                            } // else fall through default
                        } // else fall through default
                    case 8:
                        // follow through default
                    case 10:
                        // follow through default
                    default:
                    {
                        char errorString[MAX_STRING_LENGTH];
                        sprintf(errorString,
                            "Wrong ZIGBEEAPP configuration format!\n"
                            "ZIGBEEAPP <src> <dest> <items to send> "
                            "<item size> <interval> <start time> "
                            "<end time> PRECEDENCE <precedence-value> \n");
                        ERROR_ReportError(errorString);
                    }
                }

                IO_AppParseSourceAndDestStrings(
                    firstNode,
                    appInput.inputStrings[i],
                    sourceString,
                    &sourceNodeId,
                    &sourceAddr,
                    destString,
                    &destNodeId,
                    &destAddr);


                node = MAPPING_GetNodePtrFromHash(nodeHash, sourceNodeId);
                if (node != NULL)
                {
                    clocktype startTime = TIME_ConvertToClock(startTimeStr);
                    clocktype endTime = TIME_ConvertToClock(endTimeStr);
                    clocktype interval = TIME_ConvertToClock(intervalStr);

                    if ((node->adaptationData.adaptationProtocol
                            != ADAPTATION_PROTOCOL_NONE)
                        && (!node->adaptationData.endSystem))
                    {
                        char err[MAX_STRING_LENGTH];
                        sprintf(err,"Only end system can be a valid source\n");
                        ERROR_ReportWarning(err);
                        return;
                    }

#ifdef DEBUG
                    char clockStr[MAX_CLOCK_STRING_LENGTH];
                    printf("Starting ZIGBEEAPP client with:\n");
                    printf("  src nodeId:    %u\n", sourceNodeId);
                    printf("  dst nodeId:    %u\n", destNodeId);
                    printf("  dst address:   %u\n", destAddr);
                    printf("  items to send: %d\n", itemsToSend);
                    printf("  item size:     %d\n", itemSize);
                    ctoa(interval, clockStr);
                    printf("  interval:      %s\n", clockStr);
                    ctoa(startTime, clockStr);
                    printf("  start time:    %s\n", clockStr);
                    ctoa(endTime, clockStr);
                    printf("  end time:      %s\n", clockStr);
                    printf("  precedence:    %u\n", tos);
#endif /* DEBUG */

                    ZigbeeAppClientInit(
                            node,
                            sourceAddr,
                            destAddr,
                            itemsToSend,
                            itemSize,
                            interval,
                            startTime,
                            endTime,
                            tos);
                }

                if (sourceAddr.networkType != destAddr.networkType)
                {
                    char err[MAX_STRING_LENGTH];

                    sprintf(err,
                            "ZIGBEEAPP: At node %d, Source and Destination IP"
                            " version mismatch inside %s\n",
                        node->nodeId,
                        appInput.inputStrings[i]);

                    ERROR_Assert(FALSE, err);
                }

                // Handle Loopback Address
                if (node == NULL || !APP_SuccessfullyHandledLoopback(
                                        node,
                                        appInput.inputStrings[i],
                                        destAddr,
                                        destNodeId,
                                        sourceAddr,
                                        sourceNodeId))
                {
                    node = MAPPING_GetNodePtrFromHash(nodeHash, destNodeId);
                }

                if (node != NULL)
                {
                    ZigbeeAppServerInit(node);
                }
#else
                ERROR_ReportMissingLibrary(appStr, "SENSOR_NETWORKS");
#endif //SENSOR_NETWORKS_LIB
            }
            else
            {
                std::string errorString;
                errorString += "Unknown application type ";
                errorString += appStr;
                errorString += " ";
                errorString += appInput.inputStrings[i];
                ERROR_ReportError(errorString.c_str());
            }

            // restore the APPLICATION-NAME configuration, if exist
            if (appNamePtrLoc
                && (strcmp(appStr, "SUPER-APPLICATION") != 0)
                && (strcmp(appStr, "THREADED-APP") != 0))
            {
                memcpy(appNamePtrLoc, appNameTempStore.c_str(), appNameLen);
            }
        }
    }
#ifdef CYBER_LIB
    APP_InitializeFirewall(firstNode, nodeInput);
#endif
    APP_InitializeGuiHitl(firstNode, nodeInput);
}

/*
 * NAME:        APP_ProcessEvent.
 * PURPOSE:     call proper protocol to process messages.
 * PARAMETERS:  node - pointer to the node,
 *              msg - pointer to the received message.
 * RETURN:      none.
 */
void APP_ProcessEvent(Node *node, Message *msg)
{
    unsigned short protocolType;
    protocolType = APP_GetProtocolType(node,msg);
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    NetworkDataIcmp *icmp = (NetworkDataIcmp*) ip->icmpStruct;


#ifdef EXATA
    if (SocketLayerProcessEvent(node, msg) != FALSE)
    {
        return;
    }
#endif

#ifdef CYBER_LIB
    if (node->resourceManager)
    {
        switch (msg->eventType)
        {
        case MSG_APP_FromTransOpenResult:
        {
            node->resourceManager->connectionEstablished();
            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            node->resourceManager->connectionTeardown();
            break;
        }
        }
    }

    if (DOS_Drop(node, msg))
    {
        MESSAGE_Free(node, msg);
        return;
    }
#endif
    switch(protocolType)
    {
        case APP_TIMER:
        {
            AppLayerTimerTest(node, msg);
            break;
        }
        case APP_ROUTING_BELLMANFORD:
        {
            RoutingBellmanfordLayer(node, msg);
            break;
        }
#ifdef WIRELESS_LIB
        case APP_ROUTING_FISHEYE:
        {
            RoutingFisheyeLayer(node,msg);
            break;
        }
        case APP_ROUTING_OLSR_INRIA:
        {
            RoutingOlsrInriaLayer(node, msg);
            break;
        }
        case APP_ROUTING_OLSRv2_NIIGATA:
        {
            RoutingOLSRv2_Niigata_Layer(node, msg);
            break;
        }
        case APP_NEIGHBOR_PROTOCOL:
        {
            NeighborProtocolProcessMessage(node, msg);
            break;
        }
#endif // WIRELESS_LIB
        case APP_ROUTING_STATIC:
        {
            RoutingStaticLayer(node, msg);
            break;
        }
        case APP_TELNET_CLIENT:
        {
            AppLayerTelnetClient(node, msg);
            break;
        }
        case APP_TELNET_SERVER:
        {
            AppLayerTelnetServer(node, msg);
            break;
        }
        case APP_FTP_CLIENT:
        {
            AppLayerFtpClient(node, msg);
            break;
        }
        case APP_FTP_SERVER:
        {
            AppLayerFtpServer(node, msg);
            break;
        }
#ifdef EXATA
        case APP_EFTP_SERVER:
        {
            AppLayerEFtpServer(node, msg);
            break;
        }
        case APP_EFTP_SERVER_DATA:
        {
            AppLayerEFtpServerData(node, msg);
            break;
        }
        case APP_ETELNET_SERVER:
        {
            AppLayerETelnetServer(node, msg);
            break;
        }
        case APP_EHTTP_SERVER:
        {
            AppLayerEHttpServer(node, msg);
            break;
        }
#ifdef NETCONF_INTERFACE
        case APP_NETCONF_AGENT:
        {
            if (node->appData.ncAgent != NULL) {
                AppLayerNetconfServer (node, msg);
            }
            break;
        }
#endif
#endif //EXATA
        case APP_GEN_FTP_CLIENT:
        {
            AppLayerGenFtpClient(node, msg);
            break;
        }
        case APP_GEN_FTP_SERVER:
        {
            AppLayerGenFtpServer(node, msg);
            break;
        }
        case APP_CBR_CLIENT:
        {
            AppLayerCbrClient(node, msg);
            break;
        }
        case APP_CBR_SERVER:
        {
            AppLayerCbrServer(node, msg);
            break;
        }
        case APP_DNS_SERVER:
        {
            AppLayerDnsServer(node, msg);
            break;
        }
        case APP_DNS_CLIENT:
        {
            AppLayerDnsClient(node, msg);
                    break;
        }
#ifdef SENSOR_NETWORKS_LIB
        case APP_ZIGBEEAPP_CLIENT:
        {
            ZigbeeAppClient(node,msg);
            break;
        }
        case APP_ZIGBEEAPP_SERVER:
        {
            ZigbeeAppServer(node,msg);
            break;
        }
#endif //SENSOR_NETWORKS_LIB
#ifdef EXATA
#if defined(NETSNMP_INTERFACE)
        case APP_SNMP_TRAP:
        {
            AppLayerNetSnmpAgent(node, msg);
            break;
        }
        case APP_SNMP_AGENT:
        {
            AppLayerNetSnmpAgent(node, msg);
            break;
        }
#endif
#endif

#ifdef CYBER_CORE
        case APP_PROTOCOL_ISAKMP:
        {
            ISAKMPHandleProtocolEvent(node, msg);
            break;
        }
#endif // CYBER_CORE

#ifdef CYBER_LIB
        case APP_DOS_VICTIM:
        case APP_DOS_ATTACKER:
        {
            DOS_ProcessEvent(node, msg);
            break;
        }
        case APP_JAMMER:
        {
            AppJammerProcessEvent(node, msg);
            break;
        }
        case APP_FIREWALL:
        {
            FirewallModel::processEvent(node, msg);
            break;
        }
        case APP_SIGINT:
        {
            Sigint_ProcessEvent(node, msg);
            break;
        }
        case APP_CHANNEL_SCANNER:
        {
            ChannelScannerProcessEvent(node, msg);
            break;
        }
        case APP_EAVESDROP:
        {
            Eaves_ProcessEvent(node, msg);
            break;
        }
        case APP_PORTSCANNER:
        {
            AppPortScannerProcessEvent(node, msg);
            break;
        }
#endif // CYBER_LIB
        case APP_GUI_HITL:
        {
            char *cmd;
            cmd = (char *)MESSAGE_ReturnInfo(msg);
            GUI_HandleHITLInput(cmd, node->partitionData);
            MESSAGE_Free(node, msg);
            break;
        }
        case APP_FORWARD:
        {
            AppLayerForward(node, msg);
            break;
        }
        case APP_MCBR_CLIENT:
        {
            AppLayerMCbrClient(node, msg);
            break;
        }
        case APP_MCBR_SERVER:
        {
            AppLayerMCbrServer(node, msg);
            break;
        }
        case APP_HTTP_CLIENT:
        {
            AppLayerHttpClient(node, msg);
            break;
        }
        case APP_HTTP_SERVER:
        {
            AppLayerHttpServer(node, msg);
            break;
        }
        case APP_TRAFFIC_GEN_CLIENT:
        {
            TrafficGenClientLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_GEN_SERVER:
        {
            TrafficGenServerLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_TRACE_CLIENT:
        {
            TrafficTraceClientLayerRoutine(node, msg);
            break;
        }
        case APP_TRAFFIC_TRACE_SERVER:
        {
            TrafficTraceServerLayerRoutine(node, msg);
            break;
        }
        case APP_MESSENGER:
        {
            MessengerLayer(node, msg);
            break;
        }
        case APP_LOOKUP_CLIENT:
        {
            AppLayerLookupClient(node, msg);
            break;
        }
        case APP_LOOKUP_SERVER:
        {
            AppLayerLookupServer(node, msg);
            break;
        }
        case APP_VBR_CLIENT:
        case APP_VBR_SERVER:
        {
            VbrProcessEvent(node, msg);
            break;
        }
        case APP_SUPERAPPLICATION_CLIENT:
        case APP_SUPERAPPLICATION_SERVER:
        {
            SuperApplicationProcessEvent(node, msg);
            break;
        }
        case APP_MDP:
        {
            MdpProcessEvent(node, msg);
            break;
        }
        case APP_ROUTING_RIP:
        {
            RipProcessEvent(node,msg);
            break;
        }
        case APP_ROUTING_RIPNG:
        {
            RIPngProcessEvent(node,msg);
            break;
        }
#ifdef MILITARY_UNRESTRICTED_LIB
        case APP_THREADEDAPP_CLIENT:
        case APP_THREADEDAPP_SERVER:
        {
            ThreadedAppProcessEvent(node, msg);
            break;
        }
        case APP_DYNAMIC_THREADEDAPP:
        {
            ThreadedAppDynamicInitProcessEvent(node, msg);
            break;
        }
#endif //MILITARY_UNRESTRICTED_LIB
#ifdef ENTERPRISE_LIB
        case APP_ROUTING_HSRP:
        {
            RoutingHsrpLayer(node, msg);
            break;
        }
        case APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4:
        {
            BgpLayer(node, msg);
            break;
        }
        case APP_MPLS_LDP:
        {
            MplsLdpLayer(node, msg);
            break;
        }
        case APP_H323:
        {
            H323Layer(node, msg);
            break;
        }
        case APP_SIP:
        {
            SipProcessEvent(node, msg);
            break;
        }
        case APP_VOIP_CALL_INITIATOR:
        {
            VoipCallInitiatorHandleEvent(node, msg);
            break;
        }
        case APP_VOIP_CALL_RECEIVER:
        {
            VoipCallReceiverHandleEvent(node, msg);
            break;
        }
        case APP_H225_RAS_MULTICAST:
        case APP_H225_RAS_UNICAST:
        {
            // multicast and unicast both ras packets are handle here
            H225RasLayer(node, msg);
            break;
        }
        case APP_RTP:
        {
            RtpLayer(node, msg);
            break;
        }
        case APP_RTCP:
        {
            RtcpLayer(node, msg);
            break;
        }
#endif // ENTERPRISE_LIB

#ifdef ADDON_MGEN4
        case APP_MGEN:
        {
            MgenLayer(node, msg);
            break;
        }
#endif // ADDON_MGEN4

#ifdef ADDON_MGEN3
        case APP_MGEN3_SENDER:
        {
            AppMgenLayer(node, msg);
            break;
        }
        case APP_DREC3_RECEIVER:
        {
            AppDrecLayer(node, msg);
            break;
        }
#endif // ADDON_MGEN3

#ifdef ADDON_HELLO
        case APP_HELLO:
        {
            AppHelloProcessEvent(node,msg);
            break;
        }
#endif /* ADDON_HELLO */

#ifdef TUTORIAL_INTERFACE
        case APP_INTERFACETUTORIAL:
        {
            AppLayerInterfaceTutorial(node, msg);
            break;
        }
#endif /* TUTORIAL_INTERFACE */
#ifdef CELLULAR_LIB
        case APP_CELLULAR_ABSTRACT:
        {
            CellularAbstractAppLayer(node, msg);
            break;
        }
#endif // CELLULAR_LIB
#ifdef UMTS_LIB
        case APP_UMTS_CALL:
        {
            AppUmtsCallLayer(node, msg);
            break;
        }
#endif // UMTS_LIB


        case APP_MSDP:
        {
            MsdpLayer(node, msg);
            break;
        }

//InsertPatch LAYER_FUNCTION

#ifdef LTE_LIB
        case APP_EPC_LTE:
        {
            EpcLteAppProcessEvent(node, msg);
            break;
        }
#endif // LTE_LIB
        // DHCP
        case APP_DHCP_CLIENT:
        {
            AppLayerDhcpClient(node, msg);
            break;
        }
        case APP_DHCP_SERVER:
        {
            AppLayerDhcpRelay(node, msg);
            AppLayerDhcpServer(node, msg);
            break;
        }
        case APP_TRAFFIC_SENDER:
        {
            node->appData.appTrafficSender->processEvent(
                            node, msg);
            break;
        }

        // Modifications
#ifdef USER_MODELS_LIB
        case APP_HELLO: {
            AppHelloProcessEvent(node, msg);
            break;
        }

        case APP_UP_SERVER: {
            AppLayerUpServer(node, msg);
            break;
        }
        case APP_UP_CLIENT: {
            AppLayerUpClient(node, msg);
            break;
        }
        case APP_UP_CLIENT_DAEMON: {
        	AppLayerUpClientDaemon(node, msg);
        	break;
        }
#endif // USER_MODELS_LIB

        default:
        {
            int i;
            BOOL udp = FALSE;
            for (i=0; i < MAX_HEADERS; i++)
            {
                if (msg->headerProtocols[i] == TRACE_UDP)
                {
                    udp = TRUE;
                    break;
                }
            }
            if (ip->isIcmpEnable && icmp->portUnreachableEnable && udp)
            {

                APP_InitiateIcmpMessage(node, msg, icmp);
#ifdef ADDON_DB
               HandleStatsDBAppDropEventsInsertion(
                   node,
                   msg,
                   "AppDrop",
                   -1,
                   MESSAGE_ReturnPacketSize(msg),
                   (char*)"Unknown Application");
#endif

                MESSAGE_Free(node, msg);
            }
#ifdef EXATA
            else if (udp)
            {
#ifdef ADDON_DB
                HandleStatsDBAppDropEventsInsertion(
                    node,
                    msg,
                    "AppDrop",
                    -1,
                    MESSAGE_ReturnPacketSize(msg),
                    (char*)"Unknown Application");
#endif

            MESSAGE_Free(node, msg);
            }
#endif
            else
            {
                char errMsg[MAX_STRING_LENGTH];
                sprintf(errMsg,
                    "Application layer receives a message with unidentified "
                    "Protocol = %d\n", MESSAGE_GetProtocol(msg));
                ERROR_ReportError(errMsg);
            }
            break;
        }
    }//switch//
}

/*
 * NAME: APP_RunTimeStat.
 * PURPOSE:      display the run-time statistics for theapplication layer
 * PARAMETERS:  node - pointer to the node
 * RETURN:       none.
 */
void
APP_RunTimeStat(Node *node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    int i;
    AppInfo *appList = NULL;
    NetworkRoutingProtocolType routingProtocolType;

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV4)
        {
            routingProtocolType = ip->interfaceInfo[i]->routingProtocolType;
        }
        else if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV6)
        {
            routingProtocolType = ip->interfaceInfo[i]->ipv6InterfaceInfo->
                                      routingProtocolType;
        }
        else
        {
            routingProtocolType = ROUTING_PROTOCOL_NONE;
        }

        // Select application-layer routing protocol model and generate
        // run-time stat
        switch (routingProtocolType)
        {
            case ROUTING_PROTOCOL_BELLMANFORD:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
            case ROUTING_PROTOCOL_OLSR_INRIA:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }

            case ROUTING_PROTOCOL_FISHEYE:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
            case ROUTING_PROTOCOL_STATIC:
            {
                //STAT FUNCTION TO BE ADDED
                break;
            }
//StartRIP
            case ROUTING_PROTOCOL_RIP:
            {
                RipRunTimeStat(node, (RipData *) node->appData.routingVar);
                break;
            }
//EndRIP
//StartRIPng
            case ROUTING_PROTOCOL_RIPNG:
            {
                RIPngRunTimeStat(node, (RIPngData *) node->appData.routingVar);
                break;
            }
//EndRIPng
//InsertPatch STATS_ROUTING_FUNCTION

            default:
                break;
        }//switch//
    }


    for (appList = node->appData.appPtr;
         appList != NULL;
         appList = appList->appNext)
    {
        /*
         * Get application specific data structure and call
         * the corresponding protocol to print the stats.
         */

        switch (appList->appType)
        {
            case APP_TELNET_CLIENT:
            {
                // APP_TELNET_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_TELNET_SERVER:
            {
                // APP_TELNET_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_FTP_CLIENT:
            {
                // APP_FTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_FTP_SERVER:
            {
                // APP_FTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_GEN_FTP_CLIENT:
            {
                // APP_GEN_FTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_GEN_FTP_SERVER:
            {
                // APP_GEN_FTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_CBR_CLIENT:
            {
                // APP_CBR_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_CBR_SERVER:
            {
                // APP_CBR_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_MCBR_CLIENT:
            {
                // APP_MCBR_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_MCBR_SERVER:
            {
                // APP_MCBR_SERVER STATS UNDER CONSTRUCTION
                break;
            }
            case APP_HTTP_CLIENT:
            {
                // APP_HTTP_CLIENT STATS UNDER CONSTRUCTION
                break;
            }
            case APP_HTTP_SERVER:
            {
                // APP_HTTP_SERVER STATS UNDER CONSTRUCTION
                break;
            }

#ifdef ADDON_HELLO
            case APP_HELLO:
            {
            break;
            }
#endif /* ADDON_HELLO */

#ifdef MILITARY_UNRESTRICTED_LIB
            case APP_THREADEDAPP_CLIENT:
            case APP_THREADEDAPP_SERVER:
            {
                ThreadedAppRunTimeStat(node, (ThreadedAppData*) appList->appDetail);
                break;
            }
#endif /* MILITARY_UNRESTRICTED_LIB */
//StartVBR
            case APP_VBR_CLIENT:
            case APP_VBR_SERVER:
            {
                VbrRunTimeStat(node, (VbrData *) appList->appDetail);
            }
                break;
//EndVBR
//StartSuperApplication
            case APP_SUPERAPPLICATION_CLIENT:
            case APP_SUPERAPPLICATION_SERVER:
            {
                SuperApplicationRunTimeStat(node, (SuperApplicationData *) appList->appDetail);
            }
                break;
//EndSuperApplication
//InsertPatch STATS_FUNCTION
            case APP_FORWARD:
                ForwardRunTimeStat(node,
                                   (AppDataForward*) appList->appDetail);
                break;

            default:
                break;
        }
    }
}

/*
 * NAME:        APP_Finalize.
 * PURPOSE:     call protocols to print statistics.
 * PARAMETERS:  node - pointer to the node,
 * RETURN:      none.
 */
void
APP_Finalize(Node *node)
{
    NetworkDataIp *ip = (NetworkDataIp *) node->networkData.networkVar;
    AppInfo *appList = NULL;
    AppInfo *nextApp = NULL;
    int i;
    NetworkRoutingProtocolType routingProtocolType;


#ifdef MILITARY_RADIOS_LIB
    APPLink16GatewayFinalize(node);
#endif

    for (i = 0; i < node->numberInterfaces; i++)
    {
        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV4
            || ip->interfaceInfo[i]->interfaceType == NETWORK_DUAL)
        {
            routingProtocolType = ip->interfaceInfo[i]->routingProtocolType;

            // Select application-layer routing protocol model and finalize.
            switch (routingProtocolType)
            {
                case ROUTING_PROTOCOL_BELLMANFORD:
                {
                    RoutingBellmanfordFinalize(node, i);
                    break;
                }
    #ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_FISHEYE:
                {
                    RoutingFisheyeFinalize(node);
                    break;
                }
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    RoutingOlsrInriaFinalize(node, i);
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    RoutingOLSRv2_Niigata_Finalize(node, i);
                }
    #endif // WIRELESS_LIB
                case ROUTING_PROTOCOL_STATIC:
                {
                    RoutingStaticFinalize(node);
                    break;
                }
    //StartRIP
                case ROUTING_PROTOCOL_RIP:
                {
                    RipFinalize(node, i);
                    break;
                }
    //EndRIP
    //InsertPatch FINALIZE_ROUTING_FUNCTION

                default:
                    break;
            }//switch//
        }

        if (ip->interfaceInfo[i]->interfaceType == NETWORK_IPV6
            || ip->interfaceInfo[i]->interfaceType == NETWORK_DUAL)
        {
            routingProtocolType = ip->interfaceInfo[i]->ipv6InterfaceInfo->
                                      routingProtocolType;

            // Select application-layer routing protocol model and finalize.
            switch (routingProtocolType)
            {

    #ifdef WIRELESS_LIB
                case ROUTING_PROTOCOL_OLSR_INRIA:
                {
                    RoutingOlsrInriaFinalize(node, i);
                    break;
                }
                case ROUTING_PROTOCOL_OLSRv2_NIIGATA:
                {
                    RoutingOLSRv2_Niigata_Finalize(node, i);
                }
    #endif // WIRELESS_LIB
                case ROUTING_PROTOCOL_STATIC:
                {
                    RoutingStaticFinalize(node);
                    break;
                }
    //StartRIPng
                case ROUTING_PROTOCOL_RIPNG:
                {
                    RIPngFinalize(node, i);
                    break;
                }
    //EndRIPng
    //InsertPatch FINALIZE_ROUTING_FUNCTION
                default:
                    break;
            }//switch//
        }
    }

#ifdef CYBER_CORE
    if (node->appData.isakmpData != NULL)
    {
        ISAKMPFinalize(node);
    }
#endif // CYBER_CORE

#ifdef ENTERPRISE_LIB
    // Finalize BGP, if appropriate.
    if (node->appData.exteriorGatewayProtocol ==
        APP_EXTERIOR_GATEWAY_PROTOCOL_BGPv4)
    {
        BgpFinalize(node);
    }

    if (node->appData.hsrp != NULL)
    {
        for (i = 0; i < node->numberInterfaces; i++)
        {
            RoutingHsrpFinalize(node, i);
        }
    }
#endif // ENTERPRISE_LIB

    if (node->appData.dnsData != NULL)
    {
        DnsFinalize(node);
    }
    if (node->isMsdpEnabled == TRUE)
    {
        MsdpFinalize(node);
    }
    /* Select application model and finalize. */
   // DHCP
    BOOL dhcpServerFlag = FALSE;
    BOOL dhcpClientFlag = FALSE;
    BOOL dhcpRelayFlag = FALSE;
    for (appList = node->appData.appPtr; appList != NULL; appList = nextApp)
    {
        switch (appList->appType)
        {
            case APP_TELNET_CLIENT:
            {
                AppTelnetClientFinalize(node, appList);
                break;
            }
            case APP_TELNET_SERVER:
            {
                AppTelnetServerFinalize(node, appList);
                break;
            }
            case APP_FTP_CLIENT:
            {
                AppFtpClientFinalize(node, appList);
                break;
            }
            case APP_FTP_SERVER:
            {
                AppFtpServerFinalize(node, appList);
                break;
            }
#ifdef EXATA
            case APP_EFTP_SERVER_DATA:
            {
                break;
            }
            case APP_EFTP_SERVER:
            {
                AppEFtpServerFinalize(node, appList);
                break;
            }
            case APP_ETELNET_SERVER:
            {
                AppETelnetServerFinalize(node, appList);
                break;
            }
            case APP_EHTTP_SERVER:
            {
                AppEHttpServerFinalize(node, appList);
                break;
            }
#ifdef NETCONF_INTERFACE
            case APP_NETCONF_AGENT:
            {
                AppNetconfServerFinalize(node);
                break;
            }
#endif
#endif
            case APP_GEN_FTP_CLIENT:
            {
                AppGenFtpClientFinalize(node, appList);
                break;
            }
            case APP_GEN_FTP_SERVER:
            {
                AppGenFtpServerFinalize(node, appList);
                break;
            }
            case APP_CBR_CLIENT:
            {
                AppCbrClientFinalize(node, appList);
                break;
            }
            case APP_CBR_SERVER:
            {
                AppCbrServerFinalize(node, appList);
                break;
            }
            case APP_FORWARD:
            {
                AppForwardFinalize(node, appList);
                break;
            }
#ifdef CYBER_LIB
            case APP_DOS_VICTIM:
            {
                DOS_Finalize(node, appList);
                break;
            }
            case APP_DOS_ATTACKER:
            {
                DOS_Finalize(node, appList);
                break;
            }
            case APP_JAMMER:
            {
                AppJammerFinalize(node);
                break;
            }
            case APP_CHANNEL_SCANNER:
            {
                break;
            }
            case APP_SIGINT:
            {
                AppSigintFinalize(node, appList);
                break;
            }
            case APP_PORTSCANNER:
            {
                AppPortScannerFinalize(node, appList);
                break;
            }
#endif // CYBER_LIB
#ifdef ADDON_MGEN4
            case APP_MGEN:
            {
               MgenFinalize(node);
               break;
            }
#endif // ADDON_MGEN4

#ifdef ADDON_MGEN3
            case APP_MGEN3_SENDER:
            {
                AppMgenSenderFinalize(node, appList);

                break;
            }
            case APP_DREC3_RECEIVER:
            {
                AppDrecReceiverFinalize(node, appList);
                break;
            }
#endif // ADDON_MGEN3
#ifdef TUTORIAL_INTERFACE
            case APP_INTERFACETUTORIAL:
            {
                AppInterfaceTutorialFinalize(node, appList);
                break;
            }
#endif // TUTORIAL_INTERFACE
            case APP_MESSENGER:
            {
                MessengerFinalize(node, appList);
                break;
            }
            case APP_MCBR_CLIENT:
            {
                AppMCbrClientFinalize(node, appList);
                break;
            }
            case APP_MCBR_SERVER:
            {
                AppMCbrServerFinalize(node, appList);
                break;
            }
            case APP_HTTP_CLIENT:
            {
                AppHttpClientFinalize(node, appList);
                break;
            }
            case APP_HTTP_SERVER:
            {
                AppHttpServerFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_GEN_CLIENT:
            {
                TrafficGenClientFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_GEN_SERVER:
            {
                TrafficGenServerFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_TRACE_CLIENT:
            {
                TrafficTraceClientFinalize(node, appList);
                break;
            }
            case APP_TRAFFIC_TRACE_SERVER:
            {
                TrafficTraceServerFinalize(node, appList);
                break;
            }
            case APP_VBR_CLIENT:
            case APP_VBR_SERVER:
            {
                VbrFinalize(node, (VbrData *) appList->appDetail);
                break;
            }

            case APP_SUPERAPPLICATION_CLIENT:
            {
                    SuperApplicationClientFinalize(node);
                    SuperApplicationFinalize(node, (SuperApplicationData *) appList->appDetail);
                    break;
            }
            case APP_SUPERAPPLICATION_SERVER:
            {
                SuperApplicationFinalize(node, (SuperApplicationData *) appList->appDetail);
                break;
            }

#ifdef MILITARY_UNRESTRICTED_LIB
            case APP_THREADEDAPP_CLIENT:
            case APP_THREADEDAPP_SERVER:
            {
                ThreadedAppFinalize(node, (ThreadedAppData*) appList->appDetail);
                break;
            }
            case APP_DYNAMIC_THREADEDAPP:
            {
                break;
            }
#endif /* MILITARY_UNRESTRICTED_LIB */

            case APP_LOOKUP_CLIENT:
            {
                AppLookupClientFinalize(node, appList);
                break;
            }
            case APP_LOOKUP_SERVER:
            {
                AppLookupServerFinalize(node, appList);
                break;
            }

#ifdef WIRELESS_LIB
            case APP_NEIGHBOR_PROTOCOL:
            {
                NeighborProtocolFinalize(node, appList);
                break;
            }
#endif // WIRELESS_LIB

#ifdef ENTERPRISE_LIB
            case APP_MPLS_LDP:
            {
                MplsLdpFinalize(node, appList);
                break;
            }
            case APP_VOIP_CALL_INITIATOR:
            {
                AppVoipInitiatorFinalize(node, appList);
                break;
            }
            case APP_VOIP_CALL_RECEIVER:
            {
                AppVoipReceiverFinalize(node, appList);
                break;
            }
#endif // ENTERPRISE_LIB

#ifdef ADDON_HELLO
            case APP_HELLO:
            {
                AppHelloFinalize(node);
                break;
            }
#endif /* ADDON_HELLO */

#ifdef EXATA
#if defined(NETSNMP_INTERFACE)
            case APP_SNMP_AGENT:
            {
                AppNetSnmpAgentFinalize(node);
                break;
            }
#endif
#endif

#ifdef SENSOR_NETWORKS_LIB
            case APP_ZIGBEEAPP_CLIENT:
            {
                ZigbeeAppClientFinalize(node, appList);
                break;
            }
            case APP_ZIGBEEAPP_SERVER:
            {
                ZigbeeAppServerFinalize(node, appList);
                break;
            }
#endif //SENSOR_NETWORKS_LIB
           // DHCP
            case APP_DHCP_CLIENT:
            {
                if (dhcpClientFlag == FALSE)
                {
                    AppDhcpClientFinalize(node);
                    dhcpClientFlag = TRUE;
                }
                break;
            }
            case APP_DHCP_SERVER:
            {
                if (dhcpServerFlag == FALSE)
                {
                    AppDhcpServerFinalize(node);
                    dhcpServerFlag = TRUE;
                }
                break;
            }
            case APP_DHCP_RELAY:
            {
                if (dhcpRelayFlag == FALSE)
                {
                    AppDhcpRelayFinalize(node);
                    dhcpRelayFlag = TRUE;
                }
                break;
            }

            // Modifications
#ifdef USER_MODELS_LIB
            case APP_HELLO: {
                AppHelloFinalize(node);
                break;
            }

            case APP_UP_SERVER: {
                AppUpServerFinalize(node, appList);
                break;
            }
            case APP_UP_CLIENT: {
                AppUpClientFinalize(node, appList);
                break;
            }
            case APP_UP_CLIENT_DAEMON: {
            	AppUpClientDaemonFinalize(node, appList);
            	break;
            }
#endif // USER_MODELS_LIB

//InsertPatch FINALIZE_FUNCTION
            default:
#ifndef EXATA
                ERROR_ReportError("Unknown or disabled application");
#endif
                break;
        }//switch//

        nextApp = appList->appNext;
    }

    if (node->appData.mdp != NULL)
    {
        MdpData* mdpData = (MdpData*) node->appData.mdp;
        if (mdpData->mdpStats)
        {
            MdpFinalize(node);
        }
    }
#ifdef ENTERPRISE_LIB
    if (((RtpData*)(node->appData.rtpData)) &&
        ((RtpData*)(node->appData.rtpData))->rtpStats)
    {
        RtpFinalize(node);
        RtcpFinalize(node);
    }

    if (node->appData.multimedia && node->appData.multimedia->sigPtr)
    {
        if (node->appData.multimedia->sigType == SIG_TYPE_H323)
        {
            H323Finalize(node);
        }
        else if (node->appData.multimedia->sigType == SIG_TYPE_SIP)
        {
            SipFinalize(node);
        }
    }
#endif // ENTERPRISE_LIB

#ifdef MILITARY_UNRESTRICTED_LIB
    if (node->appData.triggerThreadData != NULL)
    {
        ThreadedAppTriggerFinalize(node);
    }
#endif /* MILITARY_UNRESTRICTED_LIB */

#ifdef CELLULAR_LIB
    if (node->appData.appCellularAbstractVar)
    {
        CellularAbstractAppFinalize(node);
    }
#endif // CELLULAR_LIB
#ifdef UMTS_LIB
    if (node->appData.umtscallVar)
    {
        AppUmtsCallFinalize(node);
    }
#endif // UMTS_LIB
#ifdef CYBER_LIB
    if (node->firewallModel)
    {
        node->firewallModel->finalize();
    }
#endif

    // Pseudo traffic sender layer
    if (node->appData.appTrafficSender)
    {
        node->appData.appTrafficSender->finalize(node);
        delete (node->appData.appTrafficSender);
        node->appData.appTrafficSender = NULL;
    }
}
// Dynamic Address

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
                            AppToTcpOpen* appOpen)
{   // variable to determine the server current address state
    Int32 serverAddresState = 0;
    // variable to determine the client current address state
    Int32 clientAddressState = 0;

    if (((destNodeId == ANY_NODEID) && (destInterfaceIndex == ANY_INTERFACE))
        || (clientInterfaceIndex == ANY_INTERFACE))
    {
        return ADDRESS_FOUND;
    }
    // Get the current client and destination address
    clientAddressState =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                node->nodeId,
                                clientInterfaceIndex,
                                appOpen->localAddr.networkType,
                                &(appOpen->localAddr));

    if (destNodeId != ANY_NODEID &&
        destInterfaceIndex != ANY_INTERFACE &&
        !NetworkIpCheckIfAddressLoopBack(node, appOpen->remoteAddr))
    {
        serverAddresState =
            MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                                node,
                                destNodeId,
                                destInterfaceIndex,
                                appOpen->remoteAddr.networkType,
                                &(appOpen->remoteAddr));
    }
    if (NetworkIpCheckIfAddressLoopBack(node , appOpen->remoteAddr))
    {
        serverAddresState = clientAddressState;
    }
    // if either client or server are not in valid address state
    // then mapping will return INVALID_MAPPING
    if (clientAddressState == INVALID_MAPPING ||
        serverAddresState == INVALID_MAPPING)
    {
        return ADDRESS_INVALID;
    }
    return ADDRESS_FOUND;
}

//--------------------------------------------------------------------------
// FUNCTION    : AppTcpValidServerAddressState
// PURPOSE     : To check if TCP application server is in valid address
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
                            Address* retServerAddress)
{
    AppTcpListenTimer* timerInfo = (AppTcpListenTimer*)
                        MESSAGE_ReturnInfo(msg, INFO_TYPE_AppServerListen);
    // variable to determine the server current address state
    Int32 serverAddresState = 0;
    Address serverAddress;
    memcpy(&serverAddress, &timerInfo->address, sizeof(Address));
    // Get the current address
    if (timerInfo->serverInterfaceIndex != ANY_INTERFACE)
    {
        serverAddresState =
        MAPPING_GetInterfaceAddrForNodeIdAndIntfId(
                            node,
                            node->nodeId,
                            timerInfo->serverInterfaceIndex,
                            timerInfo->address.networkType,
                            &serverAddress);
    }
    // now start listening at the updated address if
    // in valid address state
    if (serverAddresState != INVALID_MAPPING)
    {
        if (NetworkIpCheckIfAddressLoopBack(node, timerInfo->address))
        {
            memcpy(retServerAddress, &timerInfo->address, sizeof(Address));
        }
        else
        {

            memcpy(retServerAddress, &serverAddress, sizeof(Address));
        }
        return (true);

    }
    return (false);
}

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
// RETURN                : NONE
//--------------------------------------------------------------------------
void
AppStartTcpAppSessionServerListenTimer(
                    Node* node,
                    AppType appType,
                    Address destAddr,
                    char* destString,
                    clocktype startTime)
{

    Address localDestAddress;
    DnsClientData serverDnsClientData;
    memcpy(&localDestAddress, &destAddr, sizeof(Address));
    Message* timerMsg = MESSAGE_Alloc(
                                    node,
                                    APP_LAYER,
                                    appType,
                                    MSG_APP_TimerExpired);

    MESSAGE_AddInfo(
                node,
                timerMsg,
                sizeof(AppTcpListenTimer),
                INFO_TYPE_AppServerListen);
    AppTcpListenTimer* timerInfo =(AppTcpListenTimer*)
                    MESSAGE_ReturnInfo(timerMsg,INFO_TYPE_AppServerListen);
    timerInfo->serverInterfaceIndex = ANY_INTERFACE;

    // if loopback address
    if ((localDestAddress.networkType == NETWORK_IPV4) &&
        NetworkIpIsLoopbackInterfaceAddress
            (localDestAddress.interfaceAddr.ipv4))
    {
            localDestAddress.interfaceAddr.ipv4 =
                MAPPING_GetDefaultInterfaceAddressFromNodeId(
                                                        node,
                                                        node->nodeId);
    }
    NodeId destinationNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                                node,
                                                localDestAddress);
    if (destinationNodeId > 0)
    {
        timerInfo->serverInterfaceIndex =
                (Int16)MAPPING_GetInterfaceIdForDestAddress(
                                        node,
                                        destinationNodeId,
                                        localDestAddress);
        memcpy(&timerInfo->address, &destAddr, sizeof(Address));
        MESSAGE_Send(
                node,
                timerMsg,
                startTime);
    }


    // In case of serverurl and server node is dual ip
    // start listening on ipv6 address also
    if (destString && ADDR_IsUrl(destString) &&
        strcmp(destString, "localhost.") &&
        strcmp(destString, "localhost"))
    {
        // get the DnsClientData serverDnsClientData
        Address urlDestAddress;
        node->appData.appTrafficSender->getAppServerAddressAndNodeFromUrl(
                                                    node,
                                                    destString,
                                                    &urlDestAddress,
                                                    &serverDnsClientData);
        if (serverDnsClientData.interfaceNo != ANY_INTERFACE &&
           (serverDnsClientData.networkType == DUAL_IP &&
            serverDnsClientData.ipv6Address.networkType != NETWORK_INVALID))
        {
            Message* timerMsg = MESSAGE_Alloc(
                                    node,
                                    APP_LAYER,
                                    appType,
                                    MSG_APP_TimerExpired);
            MESSAGE_AddInfo(
                        node,
                        timerMsg,
                        sizeof(AppTcpListenTimer),
                        INFO_TYPE_AppServerListen);

            AppTcpListenTimer* timerInfo = (AppTcpListenTimer*)
                    MESSAGE_ReturnInfo(timerMsg, INFO_TYPE_AppServerListen);
            timerInfo->serverInterfaceIndex = ANY_INTERFACE;
            NodeId destinationNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
                                            node,
                                            serverDnsClientData.ipv6Address);
            if (destinationNodeId > 0)
            {
                timerInfo->serverInterfaceIndex =
                    (Int16)MAPPING_GetInterfaceIdForDestAddress(
                                        node,
                                        destinationNodeId,
                                        serverDnsClientData.ipv6Address);
                memcpy(
                    &timerInfo->address,
                    &serverDnsClientData.ipv6Address,
                    sizeof(Address));
                MESSAGE_Send(
                    node,
                    timerMsg,
                    startTime);
            }
        }
    }
}

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
                    NodeId* destNodeId)
{
    // for parallel case when client and server are not
    // initialized sequentially. If client and server
    // are in different partition then isUrl will be false
    // for server partition but destString will be URL
    // get the server node and destAddress of
    // application server
    DnsClientData serverDnsClientData;
    Node* destNode = node->appData.appTrafficSender->
                                        getAppServerAddressAndNodeFromUrl(
                                            node,
                                            destString,
                                            destAddr,
                                            &serverDnsClientData);

    if (destNode != NULL)
    {
        *destNodeId = destNode->nodeId;
    }
}

#ifdef CYBER_LIB
void 
APP_InitializeCyberApplication(Node* firstNode,
                               std::string& cyberInput,
                               BOOL fromGUI,
                               std::vector<HITLInfo>& apps,
                               const std::string& hid)
{
    if (firstNode == NULL)
    {
        // This partition has no nodes.
        return;
    }

    std::string appStr = cyberInput.substr(0, cyberInput.find_first_of(" "));

    if (appStr.compare("DOS") == 0)
    {
        App_InitializeDOSVictim(firstNode, cyberInput, fromGUI, apps);
    }
    else if (appStr.compare("JAMMER") == 0)
    {   
        bool reportError = true;

        bool jammerInitialized =
            APP_InitializeJammer(
                firstNode, cyberInput, fromGUI, reportError, apps, hid);

        if (!jammerInitialized && reportError)
        {
            std::string errorString (
                "Wrong Jammer configuration format!\n"
                 "JAMMER <src> <delay> <duration> <scanner-index>"
                 " [SILENT <min-rate>] [POWER <user-defined-power>]"
                 " [RAMP-UP-TIME <ramp-up-time>]");

            ERROR_SmartReportError(errorString.c_str(), !fromGUI);
            return;
        }
    }
    else if (appStr.compare("PORTSCAN") == 0)
    {
        App_InitializePortScanner(firstNode, cyberInput, fromGUI);
    }
    else if (appStr.compare("NETSCAN") == 0)
    {
        App_InitializeNetworkScanner(firstNode, cyberInput, fromGUI);
    }
}
#endif // CYBER_LIB
