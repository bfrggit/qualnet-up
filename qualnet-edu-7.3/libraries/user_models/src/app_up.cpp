#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

#include "api.h"
#include "app_util.h"
#include "transport_tcp.h"
#include "tcpapps.h"
#include "mac.h"
#include "mac_dot11.h"
#include "partition.h"
#include "ipv6.h"
#include "mobility.h"
#include "coordinates.h"

#include "app_up.h"

// Pseudo traffic sender layer
#include "app_trafficSender.h"

void AppUpServerInit(
	Node* node,
	Address serverAddr) {
	char addrStr[MAX_STRING_LENGTH];

	// Listen
	node->appData.appTrafficSender->appTcpListen(
		node,
		APP_UP_SERVER, // Application type
		serverAddr,
		(UInt16) APP_UP_SERVER); // Server port

	// Send general messages
/*	AppUpSendGeneralMessageToServer(node, TIME_ConvertToClock("0"));
	AppUpSendGeneralMessageToClient(node, TIME_ConvertToClock("0"));
	printf("UP server: %s, partitionId=%d\n",
			node->hostname,
			node->partitionData->partitionId);*/

	char serverRecFileName[MAX_STRING_LENGTH];
	ofstream serverRecFile;

	sprintf(serverRecFileName, "server_%s.out", node->hostname);
	serverRecFile.open(serverRecFileName);
	serverRecFile.close();

	IO_ConvertIpAddressToString(&serverAddr, addrStr);
	printf("UP server: Initialized at %s (%s)\n",
		node->hostname,
		addrStr);
}

void AppUpClientInit(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	const char* appName,
	char* sourceString,
	AppUpNodeType nodeType,
	int waitTime,
	AppUpClientDaemonDataChunkStr* chunk) {
	AppDataUpClient *clientPtr;
	char addrStr[MAX_STRING_LENGTH];

	clientPtr = AppUpClientNewUpClient(
		node,
		clientAddr,
		serverAddr,
		appName,
		nodeType);
	if(!clientPtr) { // Failed initialization
		fprintf(stderr, "UP client: %s cannot allocate "
			"new client\n", node->hostname);
		assert(false);
	}
	if (node->appData.appStats)
	{
		std::string customName;

		if (clientPtr->applicationName->empty()) {
			customName = "UP Client";
		} else {
			customName = *clientPtr->applicationName;
		}
		clientPtr->stats = new STAT_AppStatistics(
				 node,
				 "up",
				 STAT_Unicast,
				 STAT_AppSenderReceiver,
				 customName.c_str());
		clientPtr->stats->EnableAutoRefragment();
		clientPtr->stats->Initialize(
				 node,
				 clientAddr,
				 serverAddr,
				 (STAT_SessionIdType)clientPtr->uniqueId,
				 clientPtr->uniqueId);
	}
	clientPtr->dataChunk = chunk;
	AppUpClientAddAddressInformation(node, clientPtr);

	IO_ConvertIpAddressToString(&clientAddr, addrStr);
	printf("UP client: Initialized at %s (%s), uniqueId=%d\n",
		node->hostname,
		addrStr,
		clientPtr->uniqueId);

	// Test
/*	AppUpSendGeneralMessageToServer(node, TIME_ConvertToClock("0"));
	AppUpSendGeneralMessageToClient(node, TIME_ConvertToClock("0"));
	printf("UP client: %s, partitionId=%d\n",
			node->hostname,
			node->partitionData->partitionId);*/

	// Open connection
	char waitTimeStr[MAX_STRING_LENGTH];

	sprintf(waitTimeStr, "%d", waitTime);
	node->appData.appTrafficSender->appTcpOpenConnection(
		node,
		APP_UP_CLIENT,
		clientPtr->localAddr,
		node->appData.nextPortNum++, // Client port
		clientPtr->remoteAddr,
		(UInt16) APP_UP_SERVER, // Server port
		clientPtr->uniqueId,
		APP_DEFAULT_TOS,
		ANY_INTERFACE,
		std::string(),
		TIME_ConvertToClock(waitTimeStr),
		clientPtr->destNodeId,
		clientPtr->clientInterfaceIndex,
		clientPtr->destInterfaceIndex);
}

/*
 * Called when a new connection is opened passively on a server node
 * Create a new server structure and register it
 */
AppDataUpServer* AppUpServerNewUpServer(
	Node* node,
	TransportToAppOpenResult *openResult) {
	AppDataUpServer *upServer;
	char localAddrStr[MAX_STRING_LENGTH];
	char remoteAddrStr[MAX_STRING_LENGTH];

	upServer = (AppDataUpServer*)MEM_malloc(sizeof(AppDataUpServer));

	// Fill in connection-specific application data
	upServer->connectionId = openResult->connectionId;
	upServer->localAddr = openResult->localAddr;
	upServer->remoteAddr = openResult->remoteAddr;
	upServer->localPort = openResult->localPort;
	upServer->remotePort = openResult->remotePort;
	upServer->uniqueId = node->appData.uniqueId++;
	upServer->itemData.sizeExpected = -1;
	upServer->itemData.sizeReceived = 0;
	upServer->sessionIsClosed = false;
	upServer->sessionStart = node->getNodeTime();
	upServer->sessionFinish = node->getNodeTime();
	upServer->stats = NULL;

	// Determine role of server
	if(AppUpClientGetUpClientDaemon(node) == NULL) {
		upServer->nodeType = APP_UP_NODE_CLOUD;
	} else {
		upServer->nodeType = APP_UP_NODE_MDC;
	}

	if (node->appData.appStats)
	{
		upServer->stats = new STAT_AppStatistics(
				node,
				"upServer",
				STAT_Unicast,
				STAT_AppSenderReceiver,
				"UP Server");
		upServer->stats->Initialize(
				node,
				openResult->remoteAddr,
				openResult->localAddr,
				(STAT_SessionIdType)openResult->clientUniqueId,
				upServer->uniqueId);
		upServer->stats->EnableAutoRefragment();
		upServer->stats->SessionStart(node);
	}
	RANDOM_SetSeed(upServer->seed,
			node->globalSeed,
			node->nodeId,
			APP_UP_SERVER,
			upServer->uniqueId);

	// Register
	APP_RegisterNewApp(node, APP_UP_SERVER, upServer);

	IO_ConvertIpAddressToString(&upServer->localAddr, localAddrStr);
	IO_ConvertIpAddressToString(&upServer->remoteAddr, remoteAddrStr);
	printf("UP server: %s:%d <- %s:%d, connectionId=%d\n",
			localAddrStr, upServer->localPort,
			remoteAddrStr, upServer->remotePort,
			upServer->connectionId);
	return upServer;
}

/*
 * Called when a node is initialized with client functionality
 * Create a new client structure and register it
 */
AppDataUpClient* AppUpClientNewUpClient(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	const char* appName,
	AppUpNodeType nodeType) {
	AppDataUpClient *upClient;
	const pthread_mutex_t mutexInit = PTHREAD_MUTEX_INITIALIZER;

	// Allocate memory for data structure
	upClient = (AppDataUpClient*)MEM_malloc(sizeof(AppDataUpClient));
	memset(upClient, 0, sizeof(AppDataUpClient));

	// Fill in node-specific application data
	upClient->localAddr = clientAddr;
	upClient->remoteAddr = serverAddr;
	upClient->localPort = -1;
	upClient->remotePort = -1;
	upClient->connectionId = -1;
	upClient->uniqueId = node->appData.uniqueId++;
	upClient->nodeType = nodeType;
	upClient->packetList = NULL;
//	upClient->packetListMutex = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&upClient->packetListMutex, &mutexInit, sizeof(mutexInit));

	upClient->sessionIsClosed = true;
	upClient->sessionStart = node->getNodeTime();
	upClient->sessionFinish = node->getNodeTime();
	upClient->stats = NULL;

	if (appName) {
		upClient->applicationName = new std::string(appName);
	} else {
		upClient->applicationName = new std::string();
	}
	RANDOM_SetSeed(upClient->seed,
			node->globalSeed,
			node->nodeId,
			APP_UP_CLIENT,
			upClient->uniqueId);

	// Register
	APP_RegisterNewApp(node, APP_UP_CLIENT, upClient);
	return upClient;
}

void AppLayerUpServer(Node *node, Message *msg) {
	char buf[MAX_STRING_LENGTH];
	AppDataUpServer *serverPtr;

	ctoa(node->getNodeTime(), buf);
/*	printf("UP server: %s at time %s processed an event\n",
		node->hostname,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransListenResult:
			TransportToAppListenResult *listenResult;

			listenResult = (TransportToAppListenResult*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at %s got listen result\n",
				buf, node->hostname);*/

			if(listenResult->connectionId < 0) {
				printf("%s: UP server at %s listen failed\n",
					buf,
					node->hostname);
				node->appData.numAppTcpFailure ++;
			} else {
				char addrStr[MAX_STRING_LENGTH];

				IO_ConvertIpAddressToString(&listenResult->localAddr, addrStr);
				printf("UP server: %s:%d, connectionId=%d\n",
						addrStr, listenResult->localPort,
						listenResult->connectionId);
			}
			break;
		case MSG_APP_FromTransOpenResult:
			TransportToAppOpenResult *openResult;

			openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at %s got open result\n",
				buf, node->hostname);*/
			assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

			if(openResult->connectionId < 0) {
				printf("%s: UP server at %s connection failed\n",
					buf,
					node->hostname);
				node->appData.numAppTcpFailure ++;
			} else {
				AppDataUpServer* serverPtr;

				serverPtr = AppUpServerNewUpServer(node, openResult);
				assert(serverPtr != NULL);
				assert(serverPtr->sessionIsClosed == false);
			}
			break;
		case MSG_APP_FromTransDataSent:
			TransportToAppDataSent* dataSent;

			dataSent = (TransportToAppDataSent*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at %s sent data\n",
				buf, node->hostname);*/
			printf("UP server: %s sent data, packetSize=%d\n",
					node->hostname,
					dataSent->length);
			break;
		case MSG_APP_FromTransDataReceived:
			TransportToAppDataReceived* dataReceived;
			char *packet;
			int packetSize;

			dataReceived = (TransportToAppDataReceived*)MESSAGE_ReturnInfo(msg);
			packet = MESSAGE_ReturnPacket(msg);
			packetSize = MESSAGE_ReturnPacketSize(msg);

/*			printf("%s: UP server at %s received data\n",
				buf, node->hostname);*/
/*			printf("UP server: %s received data, "
					"connectionId=%d packetSize=%d\n",
					node->hostname,
					dataReceived->connectionId,
					packetSize);*/

			serverPtr = AppUpServerGetUpServer(node,
					dataReceived->connectionId);
//			assert(serverPtr != NULL);
			if(serverPtr == NULL) break;
			assert(serverPtr->sessionIsClosed == false);

			// TODO: Statistics
			if (node->appData.appStats) {
				;
			}

			if(packet[0] == '^') {
				AppUpMessageHeader* header = (AppUpMessageHeader*)(packet + 1);
				Int32 capSize;

				serverPtr->itemData.sizeExpected = header->itemSize;
				serverPtr->itemData.dataChunk = header->dataChunk;
				if(packet[packetSize - 1] == '$') {
					capSize = sizeof(AppUpMessageHeader) + 2;
				} else {
					capSize = sizeof(AppUpMessageHeader) + 1;
				}
				serverPtr->itemData.sizeReceived += packetSize - capSize;
				printf("UP server: %s received data, "
						"id=%d itemSizeExpected=%d\n",
						node->hostname,
						serverPtr->itemData.dataChunk.identifier,
						serverPtr->itemData.sizeExpected);
			} else if(packet[packetSize - 1] == '$') {
				AppUpServerItemData* itemData;

				itemData = &serverPtr->itemData;
				itemData->sizeReceived += packetSize - 1;
				printf("UP server: %s received data, "
						"id=%d itemSizeReceived=%d\n",
						node->hostname,
						itemData->dataChunk.identifier,
						itemData->sizeReceived);
				node->appData.appTrafficSender->appTcpCloseConnection(
						node,
						serverPtr->connectionId);
				printf("UP server: %s disconnecting, connectionId=%d\n",
						node->hostname,
						serverPtr->connectionId);

				char serverRecFileName[MAX_STRING_LENGTH];
				ofstream serverRecFile;
				char clockInSecond[MAX_STRING_LENGTH];

				sprintf(serverRecFileName, "server_%s.out",
						node->hostname);
				serverRecFile.open(serverRecFileName, ios::app);
				TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
				if(serverPtr->nodeType == APP_UP_NODE_MDC) {
					serverRecFile << "MDC";
				} else if (serverPtr->nodeType == APP_UP_NODE_CLOUD) {
					serverRecFile << "CLOUD";
				} else assert(false);
				serverRecFile << " "
						<< node->hostname
						<< " " << "RECV DATA" << " "
						<< itemData->dataChunk.identifier
						<< " " << "AT TIME" << " "
						<< clockInSecond
						<< std::endl;
				serverRecFile.close();

				// Report data chunk information to daemon
				if(itemData->sizeReceived == itemData->sizeExpected
						&&serverPtr->nodeType == APP_UP_NODE_MDC) {
					Message* msg;
					ActionData acnData;
					int infoSize = sizeof(int);
					int packetSize = sizeof(AppUpClientDaemonDataChunkStr);
					int chunkIdentifier;

/*					if(serverPtr->itemData.dataChunk.identifier > 0) {
						chunkIdentifier =
								serverPtr->itemData.dataChunk.identifier;
					} else chunkIdentifier = 0;*/
					chunkIdentifier = itemData->dataChunk.identifier;

					msg = MESSAGE_Alloc(node,
							APP_LAYER,
							APP_UP_CLIENT_DAEMON /*APP_UP_CLIENT*/,
							MSG_APP_UP_DataChunkReceived);
					MESSAGE_InfoAlloc(node, msg, infoSize);
					memcpy(MESSAGE_ReturnInfo(msg), &chunkIdentifier, infoSize);
					if(chunkIdentifier > 0) {
						MESSAGE_PacketAlloc(node, msg, packetSize, TRACE_UP);
						memcpy(MESSAGE_ReturnPacket(msg),
								&itemData->dataChunk,
								packetSize);
					}

					//Trace Information
					acnData.actionType = SEND;
					acnData.actionComment = NO_COMMENT;
					TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
							PACKET_OUT, &acnData);
					MESSAGE_Send(node, msg, TIME_ConvertToClock("0"));
				}

				if (node->appData.appStats) {
					if (!serverPtr->stats->IsSessionFinished()) {
						serverPtr->stats->SessionFinish(node);
					}
				}
			} else {
				serverPtr->itemData.sizeReceived += packetSize;
			}
			break;
		case MSG_APP_FromTransCloseResult:
			TransportToAppCloseResult *closeResult;

			closeResult = (TransportToAppCloseResult*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at %s got close result\n",
					buf, node->hostname);*/
			if(closeResult->type == TCP_CONN_PASSIVE_CLOSE) {
				printf("UP server: %s passively closed, "
						"connectionId=%d\n",
						node->hostname,
						closeResult->connectionId);
			} else {
				printf("UP server: %s actively closed, "
						"connectionId=%d\n",
						node->hostname,
						closeResult->connectionId);
			}

			serverPtr = AppUpServerGetUpServer(node,
					closeResult->connectionId);
			assert(serverPtr != NULL);

			if (node->appData.appStats) {
				if (!serverPtr->stats->IsSessionFinished()) {
					serverPtr->stats->SessionFinish(node);
				}
			}
			if(serverPtr->sessionIsClosed == false) {
				serverPtr->sessionIsClosed = true;
				serverPtr->sessionFinish = node->getNodeTime();
			}
			break;
		case MSG_APP_TimerExpired:
			printf("%s: UP server at %s timer expired\n",
				buf, node->hostname);
			break;
		case MSG_APP_UP: // General message
			printf("UP server: %s at time %s received message\n",
					node->hostname,
					buf);
			break;
		default:
			printf("UP server: %s at time %s received "\
				"message of unknown type %d\n",
				node->hostname,
				buf,
				msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppLayerUpClient(Node *node, Message *msg) {
	char buf[MAX_STRING_LENGTH];
	AppDataUpClient* clientPtr;

	ctoa(node->getNodeTime(), buf);
/*	printf("UP client: %s at time %s processed an event\n",
		node->hostname,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransOpenResult:
			TransportToAppOpenResult *openResult;

			openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP client at %s got open result\n",
				buf, node->hostname);*/
			assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

			if(openResult->connectionId < 0) { // Connection failed
/*				printf("%s: UP client at %s connection failed\n",
					buf,
					node->hostname);*/
				node->appData.numAppTcpFailure ++;

				clientPtr = AppUpClientGetClientPtr(node, openResult->uniqueId);
				assert(clientPtr != NULL);

				// Report data chunk information to daemon
				Message* msg;
				ActionData acnData;
				int infoSize = sizeof(int);
				int packetSize = sizeof(AppUpClientDaemonDataChunkStr);
				int chunkIdentifier = 0;

				if(clientPtr->dataChunk) {
					chunkIdentifier = clientPtr->dataChunk->identifier;
				}

				msg = MESSAGE_Alloc(node,
						APP_LAYER,
						APP_UP_CLIENT_DAEMON,
						MSG_APP_UP_TransportConnectionFailed);
				MESSAGE_InfoAlloc(node, msg, infoSize);
				memcpy(MESSAGE_ReturnInfo(msg), &chunkIdentifier, infoSize);
				if(chunkIdentifier > 0) {
					MESSAGE_PacketAlloc(node, msg, packetSize, TRACE_UP);
					memcpy(MESSAGE_ReturnPacket(msg),
							clientPtr->dataChunk,
							packetSize);
				}

				//Trace Information
				acnData.actionType = SEND;
				acnData.actionComment = NO_COMMENT;
				TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
						PACKET_OUT, &acnData);
				MESSAGE_Send(node, msg, TIME_ConvertToClock("0"));
			} else { // Connection successful
				AppDataUpClient* clientPtr;
				char* item;
				Int32 itemSize = 1024 * 1024;
				Int32 fullSize;

				clientPtr = AppUpClientUpdateUpClient(node, openResult);
				assert(clientPtr != NULL);

				if(clientPtr->dataChunk == NULL) {
					item = AppUpClientNewDataItem(itemSize, fullSize, 0, 0, 0.);
				} else {
					item = AppUpClientNewDataItem(
							clientPtr->dataChunk->size * 1024,
							fullSize,
							clientPtr->dataChunk->identifier,
							clientPtr->dataChunk->deadline,
							clientPtr->dataChunk->priority);
				}
				AppUpClientSendItem(node, clientPtr, item, fullSize);

				pthread_mutex_lock(&clientPtr->packetListMutex);
				if(clientPtr->packetList) {
					AppUpClientSendNextPacket(node, clientPtr);
				} else {
					assert(false);
				}
				pthread_mutex_unlock(&clientPtr->packetListMutex);
				MEM_free(item);
			}
			break;
		case MSG_APP_FromTransDataSent:
			TransportToAppDataSent* dataSent;

			dataSent = (TransportToAppDataSent*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP client at %s sent data\n",
				buf, node->hostname);*/
/*			printf("UP client: %s at time %s sent data, packetSize=%d\n",
					node->hostname,
					buf,
					dataSent->length);*/

			clientPtr = AppUpClientGetUpClient(node, dataSent->connectionId);
			assert(clientPtr != NULL);

			pthread_mutex_lock(&clientPtr->packetListMutex);
			if(clientPtr->packetList) {
				AppUpClientSendNextPacket(node, clientPtr);
			} else if(clientPtr->sessionIsClosed) {
				node->appData.appTrafficSender->appTcpCloseConnection(
						node,
						clientPtr->connectionId);
				printf("UP client: %s disconnecting, connectionId=%d\n",
						node->hostname,
						clientPtr->connectionId);

				Message* msg;
				ActionData acnData;
				int infoSize = sizeof(int);
				int packetSize = sizeof(AppUpClientDaemonDataChunkStr);
				int chunkIdentifier;

				if(clientPtr->dataChunk) {
					chunkIdentifier = clientPtr->dataChunk->identifier;
				} else chunkIdentifier = 0;

				msg = MESSAGE_Alloc(node,
						APP_LAYER,
						APP_UP_CLIENT_DAEMON /*APP_UP_CLIENT*/,
						MSG_APP_UP_DataChunkDelivered);
				MESSAGE_InfoAlloc(node, msg, infoSize);
				memcpy(MESSAGE_ReturnInfo(msg), &chunkIdentifier, infoSize);
				if(clientPtr->dataChunk) {
					MESSAGE_PacketAlloc(node, msg, packetSize, TRACE_UP);
					memcpy(MESSAGE_ReturnPacket(msg),
							clientPtr->dataChunk,
							packetSize);
				}
				MEM_free(clientPtr->dataChunk);

				//Trace Information
				acnData.actionType = SEND;
				acnData.actionComment = NO_COMMENT;
				TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
						PACKET_OUT, &acnData);
				MESSAGE_Send(node, msg, TIME_ConvertToClock("0"));

				if (node->appData.appStats) {
					if (!clientPtr->stats->IsSessionFinished()) {
						clientPtr->stats->SessionFinish(node);
					}
				}
			}
			pthread_mutex_unlock(&clientPtr->packetListMutex);
			break;
		case MSG_APP_FromTransDataReceived:
			TransportToAppDataReceived* dataReceived;
			char *packet;
			int packetSize;

			dataReceived = (TransportToAppDataReceived*)MESSAGE_ReturnInfo(msg);
			packet = MESSAGE_ReturnPacket(msg);
			packetSize = MESSAGE_ReturnPacketSize(msg);

/*			printf("%s: UP client at %s received data\n",
				buf, node->hostname);*/
			printf("UP client: %s received data, "
					"connectionId=%d packetSize=%d\n",
					node->hostname,
					dataReceived->connectionId,
					packetSize);

			clientPtr = AppUpClientGetUpClient(node,
					dataReceived->connectionId);
			assert(clientPtr != NULL);

			// TODO: Statistics
			if (node->appData.appStats) {
				;
			}
			break;
		case MSG_APP_FromTransCloseResult:
			TransportToAppCloseResult *closeResult;

			closeResult = (TransportToAppCloseResult*)MESSAGE_ReturnInfo(msg);
/*			if(closeResult->type == TCP_CONN_PASSIVE_CLOSE) {
				printf("UP client: %s at time %s passively closed, "
						"connectionId=%d\n",
						node->hostname,
						buf,
						closeResult->connectionId);
			} else {
				printf("UP client: %s at time %s actively closed, "
						"connectionId=%d\n",
						node->hostname,
						buf,
						closeResult->connectionId);
			}*/

			clientPtr = AppUpClientGetUpClient(node,
					closeResult->connectionId);
			assert(clientPtr != NULL);

			if (node->appData.appStats) {
				if (!clientPtr->stats->IsSessionFinished()) {
					clientPtr->stats->SessionFinish(node);
				}
			}
			if(clientPtr->sessionIsClosed == false) {
				clientPtr->sessionIsClosed = true;
				clientPtr->sessionFinish = node->getNodeTime();
			}
			break;
		case MSG_APP_UP: // General message
			printf("UP client: %s at time %s received message\n",
					node->hostname,
					buf);
			break;
/*		case MSG_APP_UP_FromMacJoinCompleted:
			MacDataDot11* macData;
			char macAdder[24];

			macData = (MacDataDot11*)MESSAGE_ReturnPacket(msg);
			MacDot11MacAddressToStr(macAdder, &macData->bssAddr);
			printf("UP client: %s joined %s with AP %s\n",
					node->hostname,
					macData->stationMIB->dot11DesiredSSID,
					macAdder);
			break;*/
		default:
			printf("UP client: %s at time %s received "\
				"message of unknown type %d\n",
				node->hostname,
				buf,
				msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppUpServerFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpServer *serverPtr = (AppDataUpServer*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

	printf("UP server: Finalized at %s\n", node->hostname);

//	TODO: Statistics
	if(node->appData.appStats) {
		;
	}
}

void AppUpClientFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpClient *clientPtr = (AppDataUpClient*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

//	printf("UP client: Finalized at %s\n", node->hostname);

//	TODO: Statistics
	if(node->appData.appStats) {
		;
	}
}

void AppUpServerPrintStats(Node *node, AppDataUpServer *serverPtr) {
//	TODO: Statistics
	return;
}

void AppUpClientPrintStats(Node *node, AppDataUpClient *clientPtr) {
//	TODO: Statistics
	return;
}

void
AppUpClientAddAddressInformation(
	Node* node,
	AppDataUpClient* clientPtr)
{
	// Store the client and destination interface index such that we can get
	// the correct address when the application starts
	NodeId destNodeId = MAPPING_GetNodeIdFromInterfaceAddress(
		node,
		clientPtr->remoteAddr);
	
	if (destNodeId != INVALID_MAPPING)
	{
		clientPtr->destNodeId = destNodeId;
		clientPtr->destInterfaceIndex = 
			(Int16)MAPPING_GetInterfaceIdForDestAddress(
				node,
				destNodeId,
				clientPtr->remoteAddr);
	}
	// Handle loopback address in destination
	if (destNodeId == INVALID_MAPPING)
	{
		if (NetworkIpCheckIfAddressLoopBack(node, clientPtr->remoteAddr))
		{
			clientPtr->destNodeId = node->nodeId;
			clientPtr->destInterfaceIndex = DEFAULT_INTERFACE;
		}
	}
	clientPtr->clientInterfaceIndex = 
		(Int16)MAPPING_GetInterfaceIndexFromInterfaceAddress(
			node,
			clientPtr->localAddr);
}

/*
 * Match an existing server structure with connectionId
 */
AppDataUpServer*
AppUpServerGetUpServer(Node *node, int connId)
{
	AppInfo *appList = node->appData.appPtr;
	AppDataUpServer *upServer;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_UP_SERVER)
		{
			upServer = (AppDataUpServer *) appList->appDetail;

			if (upServer->connectionId == connId)
			{
				return upServer;
			}
		}
	}
	return NULL;
}

/*
 * Match an existing client structure with connectionId
 */
AppDataUpClient*
AppUpClientGetUpClient(Node *node, int connId)
{
	AppInfo *appList = node->appData.appPtr;
	AppDataUpClient *upClient;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_UP_CLIENT)
		{
			upClient = (AppDataUpClient *) appList->appDetail;

			if (upClient->connectionId == connId)
			{
				return upClient;
			}
		}
	}
	return NULL;
}

/*
 * Match an existing client structure with uniqueId
 */
AppDataUpClient*
AppUpClientGetClientPtr(
	Node* node,
	Int32 uniqueId)
{
	AppInfo* appList = node->appData.appPtr;
	AppDataUpClient* upClient = NULL;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_UP_CLIENT)
		{
			upClient = (AppDataUpClient*) appList->appDetail;

			if (upClient->uniqueId == uniqueId)
			{
				return (upClient);
			}
		}
	}
	return NULL;
}

void AppUpClientSendNextPacket(
		Node *node,
		AppDataUpClient *clientPtr) {
	char* payload;
	Int32 packetSize;
	AppUpClientPacketList* list = clientPtr->packetList;
	char buf[MAX_STRING_LENGTH];

	ctoa(node->getNodeTime(), buf);
	if(clientPtr->sessionIsClosed) {
		printf("%s: UP client at %s attempted invalid operation\n",
				buf,
				node->hostname);
		return;
	}

	/*
	 * Developer should remember to lock/unlock packetListMutex in caller
	 */
	if(!clientPtr->packetList) /*if(clientPtr->packetList == NULL)*/ {
		assert(false);
		return;
	}

	Message *msg = APP_TcpCreateMessage(
		node,
		clientPtr->connectionId,
		TRACE_UP);

	payload = list->payload;
	packetSize = list->packetSize;
	APP_AddPayload(node, msg, payload, packetSize);
	node->appData.appTrafficSender->appTcpSend(node, msg);

//	TODO: Statistics
	if (node->appData.appStats)
	{
		;
	}

	MEM_free(payload);
	clientPtr->packetList = list->next;
	MEM_free(list);

	if(clientPtr->packetList == NULL) {
		clientPtr->sessionIsClosed = true;
		clientPtr->sessionFinish = node->getNodeTime();

		// TODO: Statistics
		if (node->appData.appStats)
		{
			;
		}
	}
}

char* AppUpClientNewDataItem(
		Int32 itemSize,
		Int32& fullSize,
		int identifier,
		int deadline,
		float priority) {
	char* item;
	AppUpMessageHeader* header;

	fullSize = itemSize + sizeof(AppUpMessageHeader) + 2;
	item = (char*)MEM_malloc(fullSize);
	memset(item, 0, fullSize);
	item[0] = '^';
	item[fullSize - 1] = '$';
	header = (AppUpMessageHeader*)(item + 1);
	header->type = APP_UP_MSG_DATA;
	header->itemSize = itemSize;
	header->dataChunk.identifier = identifier;
	header->dataChunk.size = itemSize / 1024;
	header->dataChunk.deadline = deadline;
	header->dataChunk.priority = priority;
	header->dataChunk.next = NULL;
	return item;
}

void AppUpClientSendItem(
		Node* node,
		AppDataUpClient *clientPtr,
		char* item,
		Int32 itemSize) {
	char* payload;
	Int32 itemSizeLeft = itemSize;
	char* itemPtr = item;
	AppUpClientPacketList *list = NULL, *listItem;
	char buf[MAX_STRING_LENGTH];

	ctoa(node->getNodeTime(), buf);
	if(clientPtr->sessionIsClosed) {
		printf("%s: UP client at %s attempted invalid operation\n",
				buf,
				node->hostname);
		return;
	}

	while(itemSizeLeft > APP_MAX_DATA_SIZE) {
//		printf("UP Client: itemSizeLeft=%d\n", itemSizeLeft);
		payload = (char*)MEM_malloc(APP_MAX_DATA_SIZE);
		memcpy(payload, itemPtr, APP_MAX_DATA_SIZE);

		listItem = (AppUpClientPacketList*)MEM_malloc(
				sizeof(AppUpClientPacketList));
		listItem->payload = payload;
		listItem->packetSize = APP_MAX_DATA_SIZE;
		listItem->next = NULL;
		list = AppUpClientPacketListAppend(list, listItem);

		itemPtr += APP_MAX_DATA_SIZE;
		itemSizeLeft -= APP_MAX_DATA_SIZE;
	}

//	printf("UP Client: itemSizeLeft=%d\n", itemSizeLeft);
	payload = (char*)MEM_malloc(itemSizeLeft);
	memcpy(payload, itemPtr, itemSizeLeft);

	listItem = (AppUpClientPacketList*)MEM_malloc(
			sizeof(AppUpClientPacketList));
	listItem->payload = payload;
	listItem->packetSize = itemSizeLeft;
	listItem->next = NULL;
	list = AppUpClientPacketListAppend(list, listItem);

	itemPtr += itemSizeLeft;
	itemSizeLeft -= itemSizeLeft;

	pthread_mutex_lock(&clientPtr->packetListMutex);
	clientPtr->packetList = AppUpClientPacketListAppend(
			clientPtr->packetList,
			list);
	pthread_mutex_unlock(&clientPtr->packetListMutex);

//	TODO: Statistics
	if (node->appData.appStats)
	{
		;
	}

	/*
	 * Developer should remember to free memory of item in caller
	 */
}

/*
 * Called when a new connection is opened on a client node
 * Match an existing client structure with uniqueId
 * Update it with connectionId
 */
AppDataUpClient*
AppUpClientUpdateUpClient(Node *node,
	TransportToAppOpenResult *openResult)
{
	AppInfo *appList = node->appData.appPtr;
	AppDataUpClient *tmpUpClient = NULL;
	AppDataUpClient *upClient = NULL;
	char localAddrStr[MAX_STRING_LENGTH];
	char remoteAddrStr[MAX_STRING_LENGTH];

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_UP_CLIENT)
		{
			tmpUpClient = (AppDataUpClient *) appList->appDetail;

			if (tmpUpClient->uniqueId == openResult->uniqueId)
			{
				upClient = tmpUpClient;
				break;
			}
		}
	}
	assert (upClient != NULL);

	// Fill in
	upClient->connectionId = openResult->connectionId;
	upClient->localAddr = openResult->localAddr;
	upClient->remoteAddr = openResult->remoteAddr;
	upClient->localPort = openResult->localPort;
	upClient->remotePort = openResult->remotePort;
	upClient->sessionIsClosed = false;
	upClient->sessionStart = node->getNodeTime();
	upClient->sessionFinish = node->getNodeTime();

	if (node->appData.appStats) {
		if (!upClient->stats->IsSessionStarted()) {
			upClient->stats->SessionStart(node);
		}
	}

	IO_ConvertIpAddressToString(&upClient->localAddr, localAddrStr);
	IO_ConvertIpAddressToString(&upClient->remoteAddr, remoteAddrStr);
	printf("UP client: %s:%d -> %s:%d, connectionId=%d\n",
			localAddrStr, upClient->localPort,
			remoteAddrStr, upClient->remotePort,
			upClient->connectionId);
	return upClient;
}

AppUpClientPacketList* AppUpClientPacketListAppend(
		AppUpClientPacketList* list,
		AppUpClientPacketList* item) {
	if(list == NULL) {
		list = item;
	} else { // list != NULL
		AppUpClientPacketList* ptr = list;

//		assert(ptr != NULL);
		while(ptr->next != NULL) {
			ptr = ptr->next;
		}
		ptr->next = item;
	}
	return list;
}

void AppUpSendGeneralMessageToServer(Node* node, clocktype delay) {
	Message* msg;

	msg = MESSAGE_Alloc(node, APP_LAYER, APP_UP_SERVER, MSG_APP_UP);
	MESSAGE_Send(node, msg, delay);
}

void AppUpSendGeneralMessageToClient(Node* node, clocktype delay) {
	Message* msg;

	msg = MESSAGE_Alloc(node, APP_LAYER, APP_UP_CLIENT, MSG_APP_UP);
	MESSAGE_Send(node, msg, delay);
}

void AppUpClientDaemonInit(
		Node* node,
		Node* firstNode,
		NodeAddress sourceNodeId,
		NodeAddress destNodeId,
		char* inputString,
		char* appName,
		AppUpNodeType nodeType) {
	AppDataUpClientDaemon* clientDaemonPtr;

	clientDaemonPtr = AppUpClientNewUpClientDaemon(
			node,
			firstNode,
			sourceNodeId,
			destNodeId,
			inputString,
			appName,
			nodeType);
	if(!clientDaemonPtr) { // Failed initialization
		fprintf(stderr, "UP client: %s cannot allocate "
			"new client\n", node->hostname);
		assert(false);
	}

	char daemonRecFileName[MAX_STRING_LENGTH];
	ofstream daemonRecFile;

	sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
	daemonRecFile.open(daemonRecFileName);
	daemonRecFile.close();

	printf("UP client daemon: Initialized at %s\n",
			node->hostname);
}

AppDataUpClientDaemon* AppUpClientNewUpClientDaemon(
		Node* node,
		Node* firstNode,
		NodeAddress sourceNodeId,
		NodeAddress destNodeId,
		char* inputString,
		char* appName,
		AppUpNodeType nodeType) {
	AppDataUpClientDaemon* upClientDaemon;
	int dataChunkId = 0;
	int dataChunkSize = 0;
	int dataChunkDeadline = 0;
	float dataChunkPriority = 0.0;
	char planFileName[MAX_STRING_LENGTH];
	int numValues;

	// Allocate memory for data structure
	upClientDaemon = (AppDataUpClientDaemon*)
			MEM_malloc(sizeof(AppDataUpClientDaemon));
	memset(upClientDaemon, 0, sizeof(AppDataUpClientDaemon));

	// Fill in node-specific application data
	upClientDaemon->firstNode = firstNode;
	upClientDaemon->sourceNodeId = sourceNodeId;
	upClientDaemon->destNodeId = destNodeId;
	upClientDaemon->nodeType = nodeType;
	upClientDaemon->inputString = new std::string(inputString);
	upClientDaemon->joinedId = -1;
	upClientDaemon->plan = new std::map<int, int>;
	upClientDaemon->connAttempted = 0;

	if (appName) {
		upClientDaemon->applicationName = new std::string(appName);
	} else {
		upClientDaemon->applicationName = new std::string();
	}

	if(nodeType == APP_UP_NODE_MDC) {
		numValues = sscanf(inputString,
				"%*s %*s %*s %*s %s",
				planFileName);
		if(numValues != 1) assert(false);
		if(strcmp(planFileName, "-") == 0) {
			upClientDaemon->test = true;
		} else {
			upClientDaemon->test = false;

			ifstream planFile;
			int numDataChunks = 0;
			int linesRead = 0;
			int idData, idAP;

			planFile.open(planFileName);
			assert(planFile.is_open());
			planFile >> numDataChunks;
			printf("UP client daemon: %s will read %d data chunks\n",
					node->hostname,
					numDataChunks);
			while(planFile >> idData >> idAP) {
				assert(upClientDaemon->plan->count(idData) < 1);
				upClientDaemon->plan->insert(pair<int, int>(idData, idAP));
				++linesRead;
			}
			planFile.close();
			assert(linesRead == numDataChunks);

			printf("UP client daemon: %s read plan from file %s\n",
					node->hostname,
					planFileName);
			for(map<int, int>::iterator it = upClientDaemon->plan->begin();
					it != upClientDaemon->plan->end();
					++it) {
				printf("\t%d -> %d\n", it->first, it->second);
			}
		}

		upClientDaemon->dataChunks = NULL;
	} else if(nodeType == APP_UP_NODE_DATA_SITE) {
		numValues = sscanf(inputString,
				"%*s %*s %*s %*s %d %d %d %f",
				&dataChunkId,
				&dataChunkSize,
				&dataChunkDeadline,
				&dataChunkPriority);
		if(numValues != 4) assert(false);
		upClientDaemon->test = false;

		if(dataChunkId <= 0
				|| dataChunkSize <= 0
				|| dataChunkDeadline < 0
				|| dataChunkPriority <= 0
				|| dataChunkPriority > 1) {
			printf("UP client daemon: %s "
					"ignored invalid data chunk specifications\n",
					node->hostname);
			upClientDaemon->dataChunks = NULL;
		} else {
			upClientDaemon->dataChunks = (AppUpClientDaemonDataChunkStr*)
					MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
			upClientDaemon->dataChunks->identifier = dataChunkId;
			upClientDaemon->dataChunks->size = dataChunkSize;
			upClientDaemon->dataChunks->deadline = dataChunkDeadline;
			upClientDaemon->dataChunks->priority = dataChunkPriority;
			upClientDaemon->dataChunks->next = NULL;
		}
	} else assert(false);

	// Register
	APP_RegisterNewApp(node, APP_UP_CLIENT_DAEMON, upClientDaemon);
	return upClientDaemon;
}

void AppLayerUpClientDaemon(Node *node, Message *msg) {
	char buf[MAX_STRING_LENGTH];
	AppDataUpClientDaemon* clientDaemonPtr;
	char daemonRecFileName[MAX_STRING_LENGTH];
	ofstream daemonRecFile;
	char clockInSecond[MAX_STRING_LENGTH];

	sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
	ctoa(node->getNodeTime(), buf);
/*	printf("UP client: %s at time %s processed an event\n",
		node->hostname,
		buf);*/

	switch(msg->eventType) {
	case MSG_APP_UP_FromMacJoinCompleted: {
//		Mac802Address* bssAddr;
		MacDataDot11* macData;
		char macAdder[24];
		int bssAddrArray[6];

/*		bssAddr = (Mac802Address*)MESSAGE_ReturnInfo(msg);
		MacDot11MacAddressToStr(macAdder, bssAddr);*/
		macData = (MacDataDot11*)MESSAGE_ReturnPacket(msg);
		MacDot11MacAddressToStr(macAdder, &macData->bssAddr);
		sscanf(macAdder, "[%x-%x-%x-%x-%x-%x]",
				bssAddrArray + 0,
				bssAddrArray + 1,
				bssAddrArray + 2,
				bssAddrArray + 3,
				bssAddrArray + 4,
				bssAddrArray + 5);
/*		printf("UP client daemon: %s joined %s with AP %s\n",
				node->hostname,
				macData->stationMIB->dot11DesiredSSID,
				macAdder);*/
		for(int j = 0; j < 6; ++j) {
			assert(bssAddrArray[j] >= 0 && bssAddrArray[j] <= 0xff);
		}
		printf("UP client daemon: %s joined %s with AP, "
				"bssAddr=%02x:%02x:%02x:%02x:%02x:%02x\n",
				node->hostname,
				macData->stationMIB->dot11DesiredSSID,
				bssAddrArray[0],
				bssAddrArray[1],
				bssAddrArray[2],
				bssAddrArray[3],
				bssAddrArray[4],
				bssAddrArray[5]);

		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
//		assert(clientDaemonPtr != NULL);
		if(!clientDaemonPtr) break;

		clientDaemonPtr->connAttempted = 0;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			int bssAddrIdentifier = 0;

			// Calculation of identifier must be consistent with generator
			bssAddrIdentifier = bssAddrArray[4] * 256 + bssAddrArray[5];
			printf("UP client daemon: %s joined %s with AP, "
					"identifier=%d\n",
					node->hostname,
					macData->stationMIB->dot11DesiredSSID,
					bssAddrIdentifier);
			clientDaemonPtr->joinedId = bssAddrIdentifier;
		}

		// Print coordinates
		Coordinates crds;
		Orientation ornt;

		MOBILITY_ReturnCoordinates(node, &crds);
		MOBILITY_ReturnOrientation(node, &ornt);

		// Debug coordinates
/*		if(true) {
			printf("Cartesian coordinates: (%.1f, %.1f, %.1f)\n",
					crds.cartesian.x, crds.cartesian.y, crds.cartesian.z);
			printf("Latlonalt coordinates: (%.1f, %.1f, %.1f)\n",
					crds.latlonalt.latitude,
					crds.latlonalt.longitude,
					crds.latlonalt.altitude);
			printf("Common coordinates: (%.1f, %.1f, %.1f)\n",
					crds.common.c1, crds.common.c2, crds.common.c3);
		}*/

		// Debug coordinates
/*		switch(crds.type) {
		case INVALID_COORDINATE_TYPE:
			printf("UP client daemon: Invalid type of coordinates\n");
			break;
		case UNREFERENCED_CARTESIAN:
			printf("UP client daemon: Unreferenced Cartesian\n");
			break;
		case GEODETIC:
			printf("UP client daemon: Geodetic\n");
			break;
		case GEOCENTRIC_CARTESIAN:
			printf("UP client daemon: Geocentric Cartesian\n");
			break;
		case LTSE:
			printf("UP client daemon: LTSE\n");
			break;
		case JGIS:
			printf("UP client daemon: JGIS\n");
			break;
		case MGRS_CARTESIAN:
			printf("UP client daemon: MGRS\n");
			break;
		default:
			printf("UP client daemon: Unknown type of coordinates\n");
		}*/

		printf("UP client daemon: %s is at (%.1f, %.1f, %.1f)\n",
				node->hostname,
				crds.cartesian.x,
				crds.cartesian.y,
				crds.cartesian.z);

		// Print other mobility facts
		MobilityData* mobility = node->mobilityData;
		MobilityRemainder* remainder = &mobility->remainder;
		const Coordinates* dest = &mobility
				->destArray[remainder->destCounter].position;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			printf("UP client daemon: %s is moving at speed %.1f\n",
					node->hostname,
					mobility->current->speed);
			printf("UP client daemon: %s is heading to (%.1f, %.1f, %.1f)\n",
					node->hostname,
					dest->cartesian.x,
					dest->cartesian.y,
					dest->cartesian.z);
		}

		// Initialize client application
		char sourceString[MAX_STRING_LENGTH];
		char destString[MAX_STRING_LENGTH];
		NodeAddress sourceNodeId;
		Address sourceAddr;
		NodeAddress destNodeId;
		Address destAddr;
		char sourceAddrStr[MAX_STRING_LENGTH];
		char destAddrStr[MAX_STRING_LENGTH];

		sscanf(clientDaemonPtr->inputString->c_str(),
				"%*s %s %s",
				sourceString,
				destString);
		IO_AppParseSourceAndDestStrings(
				clientDaemonPtr->firstNode,
				clientDaemonPtr->inputString->c_str(),
				sourceString,
				&sourceNodeId,
				&sourceAddr,
				destString,
				&destNodeId,
				&destAddr);
/*		IO_ConvertIpAddressToString(&sourceAddr, sourceAddrStr);
		IO_ConvertIpAddressToString(&destAddr, destAddrStr);
		printf("Starting UP client with:\n");
		printf("  src: node_%u (%s)\n",
				sourceNodeId,
				sourceAddrStr);
		printf("  dst: node_%u (%s)\n",
				destNodeId,
				destAddrStr);*/

		CoordinateType distance;
		int waitTime;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			COORD_CalcDistance(CARTESIAN, &crds, dest, &distance);
			waitTime = (int)((distance - APP_UP_WIRELESS_CLOSE_RANGE)
						/ mobility->current->speed)
					+ APP_UP_WIRELESS_AP_WAIT_TIME;
			if(waitTime < 0) waitTime = 0;
		} else if(clientDaemonPtr->nodeType == APP_UP_NODE_DATA_SITE) {
			waitTime = APP_UP_WIRELESS_MDC_WAIT_TIME;
		} else assert(false);

/*		char daemonRecFileName[MAX_STRING_LENGTH];
		ofstream daemonRecFile;
		char clockInSecond[MAX_STRING_LENGTH];*/
		int chunkIdentifier = 0;

		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
//		sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			if(clientDaemonPtr->test) {
				printf("UP client daemon: %s will try to connect, "
						"waitTime=%d\n",
						node->hostname,
						waitTime);
				AppUpClientInit(
						node,
						sourceAddr,
						destAddr,
						clientDaemonPtr->applicationName->c_str(),
						sourceString,
						clientDaemonPtr->nodeType,
						waitTime,
						NULL);

				daemonRecFile.open(daemonRecFileName, ios::app);
				daemonRecFile << "MDC" << " "
						<< node->hostname
						<< " " << "PREP DATA" << " "
						<< chunkIdentifier
						<< " " << "AT TIME" << " "
						<< clockInSecond
						<< std::endl;
				daemonRecFile.close();
			} else {
				AppUpClientDaemonSendNextDataChunk(
						node,
						clientDaemonPtr,
						sourceAddr,
						destAddr,
						sourceString,
						waitTime,
						daemonRecFileName);
			}
		} else if(clientDaemonPtr->nodeType == APP_UP_NODE_DATA_SITE){
			if(clientDaemonPtr->dataChunks) {
				printf("UP client daemon: %s will try to connect, "
						"waitTime=%d\n",
						node->hostname,
						waitTime);
				AppUpClientInit(
						node,
						sourceAddr,
						destAddr,
						clientDaemonPtr->applicationName->c_str(),
						sourceString,
						clientDaemonPtr->nodeType,
						waitTime,
						clientDaemonPtr->dataChunks);
				chunkIdentifier = clientDaemonPtr->dataChunks->identifier;
				clientDaemonPtr->dataChunks =
						clientDaemonPtr->dataChunks->next;

				daemonRecFile.open(daemonRecFileName, ios::app);
				daemonRecFile << "DATA" << " "
						<< node->hostname
						<< " " << "PREP DATA" << " "
						<< chunkIdentifier
						<< " " << "AT TIME" << " "
						<< clockInSecond
						<< std::endl;
				daemonRecFile.close();
			}
		} else assert(false);
		break; }
	case MSG_APP_UP_DataChunkDelivered: {
		int* chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = (int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);

/*		char daemonRecFileName[MAX_STRING_LENGTH];
		ofstream daemonRecFile;
		char clockInSecond[MAX_STRING_LENGTH];*/

		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
//		sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
		daemonRecFile.open(daemonRecFileName, ios::app);

		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
		if(!clientDaemonPtr) break;

		clientDaemonPtr->connAttempted = 0;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			daemonRecFile << "MDC";
		} else if (clientDaemonPtr->nodeType == APP_UP_NODE_DATA_SITE) {
			daemonRecFile << "DATA";
		} else assert(false);
		daemonRecFile << " "
				<< node->hostname
				<< " " << "SENT DATA" << " "
				<< *chunkIdentifier
				<< " " << "AT TIME" << " "
				<< clockInSecond
				<< std::endl;
		daemonRecFile.close();

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC
				&& clientDaemonPtr->test == false) {
			// Initialize client application
			char sourceString[MAX_STRING_LENGTH];
			char destString[MAX_STRING_LENGTH];
			NodeAddress sourceNodeId;
			Address sourceAddr;
			NodeAddress destNodeId;
			Address destAddr;
			char sourceAddrStr[MAX_STRING_LENGTH];
			char destAddrStr[MAX_STRING_LENGTH];

			sscanf(clientDaemonPtr->inputString->c_str(),
					"%*s %s %s",
					sourceString,
					destString);
			IO_AppParseSourceAndDestStrings(
					clientDaemonPtr->firstNode,
					clientDaemonPtr->inputString->c_str(),
					sourceString,
					&sourceNodeId,
					&sourceAddr,
					destString,
					&destNodeId,
					&destAddr);
			AppUpClientDaemonSendNextDataChunk(
					node,
					clientDaemonPtr,
					sourceAddr,
					destAddr,
					sourceString,
					0,
					daemonRecFileName);
		}
		break; }
	case MSG_APP_UP_DataChunkReceived: {
		int* chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = (int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);
		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
		if(clientDaemonPtr->nodeType != APP_UP_NODE_MDC) {
			printf("UP client daemon: %s will not process data chunk\n",
					node->hostname);
			break;
		} // Should not happen
		// Because non-MDC nodes will not send MSG_APP_UP_DataChunkReceived
		if(chunkIdentifier <= 0) {
			printf("UP client daemon: %s will not process test chunk\n",
					node->hostname);
			break;
		} // Should not happen in normal operation
		// Because generation script will not make data chunks with id < 0

		AppUpClientDaemonDataChunkStr* chunkToAdd;

		chunkToAdd = (AppUpClientDaemonDataChunkStr*)
				MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
		memcpy(chunkToAdd, chunk, sizeof(AppUpClientDaemonDataChunkStr));

		chunkToAdd->next = clientDaemonPtr->dataChunks;
		clientDaemonPtr->dataChunks = chunkToAdd;

		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
		daemonRecFile.open(daemonRecFileName, ios::app);
		daemonRecFile << "MDC" << " "
				<< node->hostname
				<< " " << "COMP DATA" <<" "
				<< *chunkIdentifier
				<< " " << "AT TIME" << " "
				<< clockInSecond
				<< std::endl;
		daemonRecFile.close();
		break; }
	case MSG_APP_UP_TransportConnectionFailed: {
		int* chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = (int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);
		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);

		printf("UP client daemon: %s failed to connect for delivery, "
				"id=%d connAttempted=%d\n",
				node->hostname,
				*chunkIdentifier,
				clientDaemonPtr->connAttempted);

		if(clientDaemonPtr->connAttempted < APP_UP_OPEN_CONN_ATTEMPT_MAX) {
			// Application-layer reconnection handler
			// Will start a new client application instance

			// Initialize client application
			char sourceString[MAX_STRING_LENGTH];
			char destString[MAX_STRING_LENGTH];
			NodeAddress sourceNodeId;
			Address sourceAddr;
			NodeAddress destNodeId;
			Address destAddr;
			char sourceAddrStr[MAX_STRING_LENGTH];
			char destAddrStr[MAX_STRING_LENGTH];
			char clockInSecond[MAX_STRING_LENGTH];
			int waitTime = 0;

			sscanf(clientDaemonPtr->inputString->c_str(),
					"%*s %s %s",
					sourceString,
					destString);
			IO_AppParseSourceAndDestStrings(
					clientDaemonPtr->firstNode,
					clientDaemonPtr->inputString->c_str(),
					sourceString,
					&sourceNodeId,
					&sourceAddr,
					destString,
					&destNodeId,
					&destAddr);
			printf("UP client daemon: %s will try to connect, "
					"waitTime=%d\n",
					node->hostname,
					waitTime);
			TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
/*			if(clientDaemonPtr->nodeType == APP_UP_NODE_DATA_SITE
					|| (clientDaemonPtr->nodeType == APP_UP_NODE_MDC
							&& chunkIdentifier > 0)) {*/
			if(*chunkIdentifier > 0) {
				AppUpClientDaemonDataChunkStr* chunkToSend;

				chunkToSend = (AppUpClientDaemonDataChunkStr*)
						MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
				memcpy(chunkToSend, chunk,
						sizeof(AppUpClientDaemonDataChunkStr));
				AppUpClientInit(
						node,
						sourceAddr,
						destAddr,
						clientDaemonPtr->applicationName->c_str(),
						sourceString,
						clientDaemonPtr->nodeType,
						waitTime /*0*/,
						chunkToSend);
			} else if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
				AppUpClientInit(
						node,
						sourceAddr,
						destAddr,
						clientDaemonPtr->applicationName->c_str(),
						sourceString,
						clientDaemonPtr->nodeType,
						waitTime /*0*/,
						NULL);
			}
			daemonRecFile.open(daemonRecFileName, ios::app);
			daemonRecFile << "MDC" << " "
					<< node->hostname
					<< " " << "PREP DATA" << " "
					<< *chunkIdentifier
					<< " " << "AT TIME" << " "
					<< clockInSecond
					<< std::endl;
			daemonRecFile.close();
			++clientDaemonPtr->connAttempted;
		} else {
			if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
				printf("UP client daemon: %s will continue\n", node->hostname);
			}

			TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
			daemonRecFile.open(daemonRecFileName, ios::app);
			daemonRecFile << "MDC" << " "
					<< node->hostname
					<< " " << "COMP AP" <<" "
					<< clientDaemonPtr->joinedId
					<< " " << "AT TIME" << " "
					<< clockInSecond
					<< std::endl;
			daemonRecFile.close();
		}
		break; }
	default:
		printf("UP client daemon: %s at time %s received "\
			"message of unknown type %d\n",
			node->hostname,
			buf,
			msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppUpClientDaemonFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpClient *clientDaemonPtr = (AppDataUpClient*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

	printf("UP client daemon: Finalized at %s\n", node->hostname);

//	TODO: Statistics
	if(node->appData.appStats) {
		;
	}
}

AppDataUpClientDaemon*
AppUpClientGetUpClientDaemon(Node *node)
{
	AppInfo *appList = node->appData.appPtr;
	AppDataUpClientDaemon *upClientDaemon;

	for (; appList != NULL; appList = appList->appNext)
	{
		if (appList->appType == APP_UP_CLIENT_DAEMON)
		{
			upClientDaemon = (AppDataUpClientDaemon *) appList->appDetail;
			return upClientDaemon;
		}
	}
	return NULL;
}

int AppUpClientDaemonGetNextDataChunk(AppDataUpClientDaemon* clientDaemonPtr) {
//	std::list<pair<int, float> > chunks;
	map<int, int>* plan = clientDaemonPtr->plan;
	AppUpClientDaemonDataChunkStr* chunk;
	int joinedId = clientDaemonPtr->joinedId;
	int chunkId = -1;
	float chunkPriority = 0.0;

	if(joinedId < 1) return chunkId; // -1
	chunk = clientDaemonPtr->dataChunks;
	while(chunk) {
		if(plan->at(chunk->identifier) == joinedId) {
			if(chunkId < 0 || chunkPriority < chunk->priority) {
				chunkId = chunk->identifier;
				chunkPriority = chunk->priority;
			}
		}
		chunk = chunk->next;
	}
	return chunkId;
}

void AppUpClientDaemonSendNextDataChunk(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		Address sourceAddr,
		Address destAddr,
		char* sourceString,
		int waitTime,
		char* daemonRecFileName) {
	char clockInSecond[MAX_STRING_LENGTH];
	int chunkIdToSend;
//	int chunkIdentifier;
	ofstream daemonRecFile;

	chunkIdToSend = AppUpClientDaemonGetNextDataChunk(clientDaemonPtr);
	if(chunkIdToSend > 0) {
		AppUpClientDaemonDataChunkStr* chunkHeader;
		AppUpClientDaemonDataChunkStr* chunkBefore;
		AppUpClientDaemonDataChunkStr* chunkToSend;

		chunkHeader = (AppUpClientDaemonDataChunkStr*)
				MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
		chunkHeader->next = clientDaemonPtr->dataChunks;
		chunkBefore = chunkHeader;
		while(chunkBefore->next != NULL
				&& chunkBefore->next->identifier != chunkIdToSend) {
			chunkBefore = chunkBefore->next;
		}
		assert(chunkBefore->next != NULL);
		chunkToSend = chunkBefore->next;
		if(chunkToSend == clientDaemonPtr->dataChunks) {
			clientDaemonPtr->dataChunks = chunkToSend->next;
		} else {
			chunkBefore->next = chunkToSend->next;
		}
		MEM_free(chunkHeader);

		printf("UP client daemon: %s will try to connect, "
				"waitTime=%d\n",
				node->hostname,
				waitTime);
		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
		AppUpClientInit(
				node,
				sourceAddr,
				destAddr,
				clientDaemonPtr->applicationName->c_str(),
				sourceString,
				clientDaemonPtr->nodeType,
				waitTime,
				chunkToSend);
//		chunkIdentifier = chunkToSend->identifier;
		daemonRecFile.open(daemonRecFileName, ios::app);
		daemonRecFile << "MDC" << " "
				<< node->hostname
				<< " " << "PREP DATA" << " "
				<< chunkIdToSend
				<< " " << "AT TIME" << " "
				<< clockInSecond
				<< std::endl;
		daemonRecFile.close();
	} else {
		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
		printf("UP client daemon: %s will continue\n", node->hostname);

		daemonRecFile.open(daemonRecFileName, ios::app);
		daemonRecFile << "MDC" << " "
				<< node->hostname
				<< " " << "COMP AP" <<" "
				<< clientDaemonPtr->joinedId
				<< " " << "AT TIME" << " "
				<< clockInSecond
				<< std::endl;
		daemonRecFile.close();
		clientDaemonPtr->joinedId = -1;
	}
}
