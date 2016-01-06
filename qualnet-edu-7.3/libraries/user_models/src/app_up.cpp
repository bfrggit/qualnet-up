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
/*	char waitTimeStr[MAX_STRING_LENGTH];

	sprintf(waitTimeStr, "%d", waitTime);*/
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
		waitTime * SECOND,
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
	upClient->packets = NULL;
//	upClient->packetsMutex = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&upClient->packetsMutex, &mutexInit, sizeof(mutexInit));

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
//	char buf[MAX_STRING_LENGTH];
	char clockInSecond[MAX_STRING_LENGTH];
	AppDataUpServer *serverPtr;

//	ctoa(node->getNodeTime(), buf);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
/*	printf("UP server: %s at time %s processed an event\n",
		node->hostname,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransListenResult:
			TransportToAppListenResult *listenResult;

			listenResult = (TransportToAppListenResult*)MESSAGE_ReturnInfo(msg);
			if(listenResult->connectionId < 0) {
				printf("%s: UP server at %s listen failed\n",
					clockInSecond,
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
			assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

			if(openResult->connectionId < 0) {
				printf("%s: UP server at %s connection failed\n",
					clockInSecond,
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
			printf("UP server: %s sent data, packetSize=%d\n",
					node->hostname,
					dataSent->length);
			break;
		case MSG_APP_FromTransDataReceived: {
			TransportToAppDataReceived* dataReceived;
			char *packet;
			int packetSize;
			int packetSizeVirtual;

			dataReceived = (TransportToAppDataReceived*)MESSAGE_ReturnInfo(msg);
			packet = MESSAGE_ReturnPacket(msg);
			packetSize = MESSAGE_ReturnPacketSize(msg);
			packetSizeVirtual = MESSAGE_ReturnVirtualPacketSize(msg);

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

			// Statistics
			if (node->appData.appStats) {
				;
			}

			if(packetSize - packetSizeVirtual > 0 && packet[0] == '^') {
				AppUpMessageHeader* header = (AppUpMessageHeader*)(packet + 1);
				Int32 capSize;

				serverPtr->itemData.sizeExpected = header->itemSize;
				serverPtr->itemData.dataChunk = header->dataChunk;

				// Changed for virtual packets
/*				if(packet[packetSize - 1] == '$') {
					capSize = sizeof(AppUpMessageHeader) + 2;
				} else {
					capSize = sizeof(AppUpMessageHeader) + 1;
				}*/

				capSize = sizeof(AppUpMessageHeader) + 2;
				serverPtr->itemData.sizeReceived += packetSize - capSize;
				printf("UP server: %s received data, "
						"identifier=%d itemSizeExpected=%d\n",
						node->hostname,
						serverPtr->itemData.dataChunk.identifier,
						serverPtr->itemData.sizeExpected);

				if(serverPtr->nodeType == APP_UP_NODE_MDC) {
					Message* msg;
					ActionData acnData;
					int chunkIdentifier;

					chunkIdentifier = serverPtr->itemData.dataChunk.identifier;

					msg = MESSAGE_Alloc(node,
							APP_LAYER,
							APP_UP_CLIENT_DAEMON /*APP_UP_CLIENT*/,
							MSG_APP_UP_DataChunkHeaderReceived);
					MESSAGE_InfoAlloc(node, msg, sizeof(int));
					memcpy(MESSAGE_ReturnInfo(msg),
							&chunkIdentifier,
							sizeof(int));

					//Trace Information
					acnData.actionType = SEND;
					acnData.actionComment = NO_COMMENT;
					TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
							PACKET_OUT, &acnData);
					MESSAGE_Send(node, msg, (clocktype)0);
				}
			} /*else if(packet[packetSize - 1] == '$') {
				// Removed for virtual packets
			}*/ else {
				serverPtr->itemData.sizeReceived += packetSize;
			}
			break; }
		case MSG_APP_FromTransCloseResult: {
			TransportToAppCloseResult *closeResult;

			closeResult = (TransportToAppCloseResult*)MESSAGE_ReturnInfo(msg);
			serverPtr = AppUpServerGetUpServer(node,
					closeResult->connectionId);
			assert(serverPtr != NULL);
			if(serverPtr->sessionIsClosed) break;

			if(closeResult->type == TCP_CONN_PASSIVE_CLOSE) {
				printf("UP server: %s passively closed, "
						"connectionId=%d\n",
						node->hostname,
						closeResult->connectionId);

				AppUpServerItemData* itemData;

				itemData = &serverPtr->itemData;
				printf("UP server: %s received data, "
						"identifier=%d itemSizeReceived=%d\n",
						node->hostname,
						itemData->dataChunk.identifier,
						itemData->sizeReceived);

				char serverRecFileName[MAX_STRING_LENGTH];
				ofstream serverRecFile;

				sprintf(serverRecFileName, "server_%s.out",
						node->hostname);
				serverRecFile.open(serverRecFileName, ios::app);
//				TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
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
					MESSAGE_Send(node, msg, (clocktype)0);
				}

				if (node->appData.appStats) {
					if (!serverPtr->stats->IsSessionFinished()) {
						serverPtr->stats->SessionFinish(node);
					}
				}
			} else {
				printf("UP server: %s actively closed, "
						"connectionId=%d\n",
						node->hostname,
						closeResult->connectionId);
			}

			// Original handler
			if (node->appData.appStats) {
				if (!serverPtr->stats->IsSessionFinished()) {
					serverPtr->stats->SessionFinish(node);
				}
			}
			if(serverPtr->sessionIsClosed == false) {
				serverPtr->sessionIsClosed = true;
				serverPtr->sessionFinish = node->getNodeTime();
			}
			break; }
		case MSG_APP_TimerExpired:
			printf("UP server: %s at time %s timer expired\n",
					node->hostname, clockInSecond);
			break;
		case MSG_APP_UP: // General message
			printf("UP server: %s at time %s received message\n",
					node->hostname, clockInSecond);
			break;
		default: {
			printf("UP server: %s at time %s received "\
				"message of unknown type %d\n",
				node->hostname, clockInSecond, msg->eventType);
		}
	}
	MESSAGE_Free(node, msg);
}

void AppLayerUpClient(Node *node, Message *msg) {
//	char buf[MAX_STRING_LENGTH];
	char clockInSecond[MAX_STRING_LENGTH];
	AppDataUpClient* clientPtr;

//	ctoa(node->getNodeTime(), buf);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
/*	printf("UP client: %s at time %s processed an event\n",
		node->hostname,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransOpenResult: {
			TransportToAppOpenResult *openResult;

			openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);
			assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

			if(openResult->connectionId < 0) { // Connection failed
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
				MESSAGE_Send(node, msg, (clocktype)0);
			} else { // Connection successful
				AppDataUpClient* clientPtr;
				char* item;
				Int32 itemSize = 1024 * 1024;
				Int32 fullSize;

				clientPtr = AppUpClientUpdateUpClient(node, openResult);
				assert(clientPtr != NULL);

				if(clientPtr->dataChunk == NULL) {
					// Changed for virtual packets
//					item = AppUpClientNewDataItem(
					item = AppUpClientNewVirtualDataItem(
							itemSize, fullSize, 0, 0, 0.);
				} else {
					// Changed for virtual packets
//					item = AppUpClientNewDataItem(
					item = AppUpClientNewVirtualDataItem(
							clientPtr->dataChunk->size * 1024,
							fullSize,
							clientPtr->dataChunk->identifier,
							clientPtr->dataChunk->deadline,
							clientPtr->dataChunk->priority);
				}
				// Changed for virtual packets
//				AppUpClientSendItem(
				AppUpClientSendVirtualItem(
						node, clientPtr, item, fullSize);

				// Removed for virtual packets
/*				pthread_mutex_lock(&clientPtr->packetsMutex);
				if(clientPtr->packets) {
					AppUpClientSendNextPacket(node, clientPtr);
				} else {
					assert(false);
				}
				pthread_mutex_unlock(&clientPtr->packetsMutex);*/

				MEM_free(item);
			}
			break; }
		case MSG_APP_FromTransDataSent: {
			TransportToAppDataSent* dataSent;

			dataSent = (TransportToAppDataSent*)MESSAGE_ReturnInfo(msg);
/*			printf("UP client: %s at time %s sent data, packetSize=%d\n",
					node->hostname,
					buf,
					dataSent->length);*/

			clientPtr = AppUpClientGetUpClient(node, dataSent->connectionId);
			assert(clientPtr != NULL);

			pthread_mutex_lock(&clientPtr->packetsMutex);
			// Removed for virtual packets
/*			if(clientPtr->packets) {
				AppUpClientSendNextPacket(node, clientPtr);
			} else*/ if(clientPtr->sessionIsClosed) {
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
//				MEM_free(clientPtr->dataChunk);

				//Trace Information
				acnData.actionType = SEND;
				acnData.actionComment = NO_COMMENT;
				TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
						PACKET_OUT, &acnData);
				MESSAGE_Send(node, msg, (clocktype)0);

				if (node->appData.appStats) {
					if (!clientPtr->stats->IsSessionFinished()) {
						clientPtr->stats->SessionFinish(node);
					}
				}
			}
			pthread_mutex_unlock(&clientPtr->packetsMutex);
			break; }
		case MSG_APP_FromTransDataReceived:
			TransportToAppDataReceived* dataReceived;
			char *packet;
			int packetSize;

			dataReceived = (TransportToAppDataReceived*)MESSAGE_ReturnInfo(msg);
			packet = MESSAGE_ReturnPacket(msg);
			packetSize = MESSAGE_ReturnPacketSize(msg);

			printf("UP client: %s received data, "
					"connectionId=%d packetSize=%d\n",
					node->hostname,
					dataReceived->connectionId,
					packetSize);

			clientPtr = AppUpClientGetUpClient(node,
					dataReceived->connectionId);
			assert(clientPtr != NULL);

			// Statistics
			if (node->appData.appStats) {
				;
			}
			break;
		case MSG_APP_FromTransCloseResult: {
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
			break; }
		case MSG_APP_UP: // General message
			printf("UP client: %s at time %s received message\n",
					node->hostname,
					clockInSecond);
			break;
		default: {
			printf("UP client: %s at time %s received "\
				"message of unknown type %d\n",
				node->hostname,
				clockInSecond,
				msg->eventType);
		}
	}
	MESSAGE_Free(node, msg);
}

void AppUpServerFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpServer *serverPtr = (AppDataUpServer*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

	printf("UP server: Finalized at %s\n", node->hostname);

	// Statistics
	if(node->appData.appStats) {
		;
	}
}

void AppUpClientFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpClient *clientPtr = (AppDataUpClient*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

//	printf("UP client: Finalized at %s\n", node->hostname);

	// Statistics
	if(node->appData.appStats) {
		;
	}
}

void AppUpServerPrintStats(Node *node, AppDataUpServer *serverPtr) {
//	TODO
	return;
}

void AppUpClientPrintStats(Node *node, AppDataUpClient *clientPtr) {
//	TODO
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

// Abandoned for virtual packets
/*
void AppUpClientSendNextPacket(
		Node *node,
		AppDataUpClient *clientPtr) {
//	char buf[MAX_STRING_LENGTH];
	char clockInSecond[MAX_STRING_LENGTH];
	char* payload;
	Int32 packetSize;
	AppUpClientPacketList* list = clientPtr->packets;

//	ctoa(node->getNodeTime(), buf);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
	if(clientPtr->sessionIsClosed) {
		printf("UP client: %s at time %s attempted invalid operation\n",
				node->hostname, clockInSecond);
		return;
	}

	if(!clientPtr->packets) if(clientPtr->packets == NULL) {
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

	// Statistics
	if (node->appData.appStats)
	{
		;
	}

	MEM_free(payload);
	clientPtr->packets = list->next;
	MEM_free(list);

	if(clientPtr->packets == NULL) {
		clientPtr->sessionIsClosed = true;
		clientPtr->sessionFinish = node->getNodeTime();

		// Statistics
		if (node->appData.appStats)
		{
			;
		}
	}
}*/

// Abandoned for virtual packets
/*
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
}*/

// Abandoned for virtual packets
/*
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

	pthread_mutex_lock(&clientPtr->packetsMutex);
	clientPtr->packets = AppUpClientPacketListAppend(
			clientPtr->packets,
			list);
	pthread_mutex_unlock(&clientPtr->packetsMutex);

	// Statistics
	if (node->appData.appStats)
	{
		;
	}
}*/

char* AppUpClientNewVirtualDataItem(
		Int32 itemSize,
		Int32& fullSize,
		int identifier,
		int deadline,
		float priority) {
	char* item;
	AppUpMessageHeader* header;

	fullSize = itemSize + sizeof(AppUpMessageHeader) + 2;
	item = (char*)MEM_malloc(sizeof(AppUpMessageHeader) + 2);
	memset(item, 0, sizeof(AppUpMessageHeader) + 2);
	item[0] = '^';
	item[sizeof(AppUpMessageHeader) + 2 - 1] = '$';
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

void AppUpClientSendVirtualItem(
		Node* node,
		AppDataUpClient *clientPtr,
		char* item,
		Int32 itemSize) {
//	char buf[MAX_STRING_LENGTH];
	char clockInSecond[MAX_STRING_LENGTH];
	Int32 packetSize = sizeof(AppUpMessageHeader) + 2;

//	ctoa(node->getNodeTime(), buf);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
	if(clientPtr->sessionIsClosed) {
		printf("UP client: %s at time %s attempted invalid operation\n",
				node->hostname, clockInSecond);
		return;
	}

	Message *msg = APP_TcpCreateMessage(
		node,
		clientPtr->connectionId,
		TRACE_UP);

	APP_AddPayload(node, msg, item, packetSize);
	APP_AddVirtualPayload(node, msg, itemSize - packetSize);
	node->appData.appTrafficSender->appTcpSend(node, msg);

	// Statistics
	if (node->appData.appStats)
	{
		;
	}

	if(clientPtr->packets == NULL) {
		clientPtr->sessionIsClosed = true;
		clientPtr->sessionFinish = node->getNodeTime();

		// Statistics
		if (node->appData.appStats)
		{
			;
		}
	}
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
	if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
		AppUpClientDaemonSetNextPathTimer(node, (clocktype)0, true);
		char daemonRecFileName[MAX_STRING_LENGTH];
		ofstream daemonRecFile;

		sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
		daemonRecFile.open(daemonRecFileName);
		daemonRecFile.close();
	}

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
	int numValues;
	CoordinateRepresentationType coordinateSystemType =
			(CoordinateRepresentationType)
			node->partitionData->terrainData->getCoordinateSystem();

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
	upClientDaemon->joinedAId = -1;
	upClientDaemon->plan = new std::map<int, int>;
	upClientDaemon->connAttempted = 0;
	upClientDaemon->path = NULL;
	upClientDaemon->initPos.cartesian.x = (CoordinateType)0;
	upClientDaemon->initPos.cartesian.y = (CoordinateType)0;
	upClientDaemon->initPos.cartesian.z = (CoordinateType)0;
	upClientDaemon->initPos.type = coordinateSystemType;
	upClientDaemon->timeoutId = 0;
	upClientDaemon->sending = 0;
	upClientDaemon->test = false;
	upClientDaemon->dataChunks = NULL;
	upClientDaemon->getNextDataChunk = NULL;

	if (appName) {
		upClientDaemon->applicationName = new std::string(appName);
	} else {
		upClientDaemon->applicationName = new std::string();
	}

	if(nodeType == APP_UP_NODE_MDC) {
		char pathFileName[MAX_STRING_LENGTH];
		char planFileName[MAX_STRING_LENGTH];

		numValues = sscanf(inputString,
				"%*s %*s %*s %*s %s %s",
				pathFileName,
				planFileName);
		assert(numValues == 2);
		upClientDaemon->getNextDataChunk = AppUpClientDaemonGNDCPlanned;

		/* Read path */ {
			ifstream pathFile;
			int numStops = 0;
			int linesRead = 0;
			double tStop, xStop, yStop;
			AppUpPathStop* ptrStop;
			AppUpPathStop* tmpStop;

			pathFile.open(pathFileName);
			assert(pathFile.is_open());
			pathFile >> numStops;
			assert(numStops >= 0);
			printf("UP client daemon: %s will read %d stops\n",
					node->hostname,
					numStops);
			while(pathFile >> tStop >> xStop >> yStop) {
				tmpStop = (AppUpPathStop*)MEM_malloc(sizeof(AppUpPathStop));
				tmpStop->t = tStop;
				tmpStop->crds.cartesian.x = (CoordinateType)xStop;
				tmpStop->crds.cartesian.y = (CoordinateType)yStop;
				tmpStop->crds.cartesian.z = (CoordinateType)0;
				tmpStop->crds.type = coordinateSystemType;
				tmpStop->lsAId = new map<int, int>;
				tmpStop->lsDId = new map<int, int>;
				tmpStop->next = NULL;

				/* Read AP list (at this stop) */{
					int numA = 0;
					int idA;
					int j;

					pathFile >> numA;
					assert(numA >= 0);
					for(j = 0; j < numA && pathFile >> idA; ++j) {
						assert(tmpStop->lsAId->count(idA) < 1);
						tmpStop->lsAId->insert(
								pair<int, int>(idA, APP_UP_PLAN_TASK_INIT));
					}
					assert(j == numA);
				}

				/* Read DS list (at this stop) */ {
					int numD = 0;
					int idD;
					int j;

					pathFile >> numD;
					assert(numD >= 0);
					for(j = 0; j < numD && pathFile >> idD; ++j) {
						assert(tmpStop->lsDId->count(idD) < 1);
						tmpStop->lsDId->insert(
								pair<int, int>(idD, APP_UP_PLAN_TASK_INIT));
					}
					assert(j == numD);
				}

				/* XXX
				 * Current implementation does not allow one stop
				 * 		to have both AP and DS.
				 * This is made guaranteed in scenario generation script
				 */
				assert(tmpStop->lsAId->size() < 1
						|| tmpStop->lsDId->size() < 1);

				if(upClientDaemon->path) {
					ptrStop->next = tmpStop;
				} else {
					upClientDaemon->path = tmpStop;
				}
				ptrStop = tmpStop;
				++linesRead;
			}
			pathFile.close();
			assert(linesRead == numStops);

			printf("UP client daemon: %s read path from file: %s\n",
					node->hostname,
					pathFileName);
			for(ptrStop = upClientDaemon->path;
					ptrStop;
					ptrStop = ptrStop->next) {
				printf("%+12.2f @ %.1f, %.1f >>",
						ptrStop->t,
						ptrStop->crds.cartesian.x,
						ptrStop->crds.cartesian.y);
				for(map<int, int>::iterator it = ptrStop->lsAId->begin();
						it != ptrStop->lsAId->end();
						++it) {
					printf(" %d", it->first);
				}
				if(ptrStop->lsAId->size() < 1) printf(" *");
				printf(" <<");
				for(map<int, int>::iterator it = ptrStop->lsDId->begin();
						it != ptrStop->lsDId->end();
						++it) {
					printf(" %d", it->first);
				}
				if(ptrStop->lsDId->size() < 1) printf(" *");
				printf("\n");
			}
		}

		if(strcmp(planFileName, "-") == 0) {
			upClientDaemon->test = true;
		} else { // Read plan
//			upClientDaemon->test = false;

			ifstream planFile;
			int numDataChunks = 0;
			int linesRead = 0;
			int idD, idA;

			planFile.open(planFileName);
			assert(planFile.is_open());
			planFile >> numDataChunks;
			assert(numDataChunks >= 0);
			printf("UP client daemon: %s will read %d data chunks\n",
					node->hostname,
					numDataChunks);
			while(planFile >> idD >> idA) {
				assert(upClientDaemon->plan->count(idD) < 1);
				upClientDaemon->plan->insert(pair<int, int>(idD, idA));
				++linesRead;
			}
			planFile.close();
			assert(linesRead == numDataChunks);

			printf("UP client daemon: %s read plan from file: %s\n",
					node->hostname,
					planFileName);
			for(map<int, int>::iterator it = upClientDaemon->plan->begin();
					it != upClientDaemon->plan->end();
					++it) {
				printf("%12d -> %d\n", it->first, it->second);
			}
		}
	} else if(nodeType == APP_UP_NODE_DATA_SITE) {
		int dataChunkId = 0;
		int dataChunkSize = 0;
		int dataChunkDeadline = 0;
		float dataChunkPriority = 0.0;
		char dataFileName[MAX_STRING_LENGTH];
		bool dataFileFlag = false;

		numValues = sscanf(inputString,
				"%*s %*s %*s %*s %d %d %d %f",
				&dataChunkId,
				&dataChunkSize,
				&dataChunkDeadline,
				&dataChunkPriority);
//		if(numValues != 4) assert(false);
		if(numValues != 4) {
			numValues = sscanf(inputString,
					"%*s %*s %*s %*s %s",
					dataFileName);
			if(numValues != 1) assert(false);
			dataFileFlag = true;
		}
//		upClientDaemon->test = false;
//		upClientDaemon->dataChunks = NULL;
		upClientDaemon->getNextDataChunk = AppUpClientDaemonGNDCEverything;

		if(!dataFileFlag) { // Legacy data chunk specification
			if(dataChunkId <= 0
					|| dataChunkSize <= 0
					|| dataChunkDeadline < 0
					|| dataChunkPriority <= 0
					|| dataChunkPriority > 1) {
				printf("UP client daemon: %s "
						"ignored invalid data chunk specifications\n",
						node->hostname);
//				upClientDaemon->dataChunks = NULL;
			} else {
				upClientDaemon->dataChunks = (AppUpClientDaemonDataChunkStr*)
						MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
				upClientDaemon->dataChunks->identifier = dataChunkId;
				upClientDaemon->dataChunks->size = dataChunkSize;
				upClientDaemon->dataChunks->deadline = dataChunkDeadline;
				upClientDaemon->dataChunks->priority = dataChunkPriority;
				upClientDaemon->dataChunks->dirty = false;
				upClientDaemon->dataChunks->next = NULL;
			}
		} else { // Multiple data chunks at same data site
			ifstream dataFile;
			int numDataChunks = 0;
			int linesRead = 0;

			dataFile.open(dataFileName);
			assert(dataFile.is_open());
			dataFile >> numDataChunks;
			assert(numDataChunks >= 0);
			printf("UP client daemon: %s will read %d data chunks\n",
					node->hostname,
					numDataChunks);
//			upClientDaemon->dataChunks = NULL;
			while(dataFile
					>> dataChunkId
					>> dataChunkSize
					>> dataChunkDeadline
					>> dataChunkPriority) {
				// Append to list of data chunks
				if(dataChunkId <= 0
						|| dataChunkSize <= 0
						|| dataChunkDeadline < 0
						|| dataChunkPriority <= 0
						|| dataChunkPriority > 1) {
					printf("UP client daemon: %s "
							"ignored invalid data chunk specifications\n",
							node->hostname);
				} else {
					AppUpClientDaemonDataChunkStr* lastChunkPtr;

					lastChunkPtr = upClientDaemon->dataChunks;
					upClientDaemon->dataChunks =
							(AppUpClientDaemonDataChunkStr*)
							MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
					upClientDaemon->dataChunks->identifier = dataChunkId;
					upClientDaemon->dataChunks->size = dataChunkSize;
					upClientDaemon->dataChunks->deadline = dataChunkDeadline;
					upClientDaemon->dataChunks->priority = dataChunkPriority;
					upClientDaemon->dataChunks->dirty = false;
					upClientDaemon->dataChunks->next = lastChunkPtr;
				}
				++linesRead;
			}
			dataFile.close();
			assert(linesRead == numDataChunks);
			printf("UP client daemon: %s read data chunks from file: %s\n",
					node->hostname,
					dataFileName);
		}
	} else assert(false);

	// Register
	APP_RegisterNewApp(node, APP_UP_CLIENT_DAEMON, upClientDaemon);
	return upClientDaemon;
}

void AppLayerUpClientDaemon(Node *node, Message *msg) {
//	char buf[MAX_STRING_LENGTH];
	char clockInSecond[MAX_STRING_LENGTH];
	AppDataUpClientDaemon* clientDaemonPtr;
	char daemonRecFileName[MAX_STRING_LENGTH];
	ofstream daemonRecFile;

	clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
	sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
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

//		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
//		assert(clientDaemonPtr != NULL);
		if(!clientDaemonPtr) break;

		clientDaemonPtr->connAttempted = 0;
//		clientDaemonPtr->sending = 0;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			int bssAddrIdentifier = 0;

			// Calculation of identifier must be consistent with generator
			bssAddrIdentifier = bssAddrArray[4] * 256 + bssAddrArray[5];
			printf("\033[1;36m"
					"UP client daemon: %s joined %s with AP, "
					"identifier=%d\n"
					"\033[0m",
					node->hostname,
					macData->stationMIB->dot11DesiredSSID,
					bssAddrIdentifier);
			clientDaemonPtr->joinedAId = bssAddrIdentifier;

			AppUpPathStop* nextStop = clientDaemonPtr->path;
			int joinedAId = clientDaemonPtr->joinedAId;

			// Mark corresponding task as going
			assert(nextStop);
			if(nextStop->lsAId->count(joinedAId) > 0) {
				nextStop->lsAId->at(joinedAId) =
						APP_UP_PLAN_TASK_WAIT;
			} else { // Connected to another AP
				for(AppUpPathStop* ptrStop = nextStop;
						ptrStop;
						ptrStop = ptrStop->next) {
					if(ptrStop->lsAId->count(joinedAId) > 0) {
						ptrStop->lsAId->at(joinedAId) =
								APP_UP_PLAN_TASK_WAIT;
						break;
					}
				}
			}
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

		// Print other mobility facts
		MobilityData* mobility = node->mobilityData;
		MobilityRemainder* remainder = &mobility->remainder;
		Coordinates dest;

		if(mobility->current->speed > 0) {
			dest = clientDaemonPtr->path->crds;
		} else {
			dest = mobility->current->position;
		}

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			printf("UP client daemon: %s is now at (%.1f, %.1f, %.1f)\n",
					node->hostname,
					crds.cartesian.x,
					crds.cartesian.y,
					crds.cartesian.z);
			printf("UP client daemon: %s is moving at speed %.1f\n",
					node->hostname,
					mobility->current->speed);
			printf("UP client daemon: %s is heading to (%.1f, %.1f, %.1f)\n",
					node->hostname,
					dest.cartesian.x,
					dest.cartesian.y,
					dest.cartesian.z);
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
			COORD_CalcDistance(CARTESIAN, &crds, &dest, &distance);
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

//		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
//		sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);

		if(clientDaemonPtr->sending > 0) {
			printf("UP client daemon: %s has ongoing task(s), "
					"sending=%d\n",
					node->hostname,
					clientDaemonPtr->sending);
			break;
		}

		if(clientDaemonPtr->test) {
			printf("UP client daemon: %s will try to connect, "
					"waitTime=%d\n",
					node->hostname,
					waitTime);
			clientDaemonPtr->sending += 1;
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
					clientDaemonPtr->getNextDataChunk,
					sourceAddr,
					destAddr,
					sourceString,
					waitTime,
					daemonRecFileName);
		}
		break; }
	case MSG_APP_UP_DataChunkDelivered: {
		int chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = *(int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);

/*		char daemonRecFileName[MAX_STRING_LENGTH];
		ofstream daemonRecFile;
		char clockInSecond[MAX_STRING_LENGTH];*/

//		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
//		sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);

//		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);
		if(!clientDaemonPtr) break;

		clientDaemonPtr->connAttempted = 0;
		clientDaemonPtr->sending -= 1;

		printf("UP client daemon: %s delivered data chunk, "
				"identifier=%d sending=%d\n",
				node->hostname,
				chunkIdentifier,
				clientDaemonPtr->sending);

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			daemonRecFile.open(daemonRecFileName, ios::app);
			daemonRecFile << "MDC" << " "
					<< node->hostname
					<< " " << "SENT DATA" << " "
					<< chunkIdentifier
					<< " " << "AT TIME" << " "
					<< clockInSecond
					<< std::endl;
			daemonRecFile.close();
		}

		if(clientDaemonPtr->test) {
			AppUpClientDaemonCompAtA(
					node,
					clientDaemonPtr,
					clientDaemonPtr->joinedAId);
		} else {
			AppUpClientDaemonSendNextDataChunk(
					node,
					clientDaemonPtr,
					(clocktype)0);
		}
		break; }
	case MSG_APP_UP_DataChunkHeaderReceived: {
		int chunkIdentifier;
		AppUpPathStop* nextStop = clientDaemonPtr->path;

		chunkIdentifier = *(int*)MESSAGE_ReturnInfo(msg);

		if(clientDaemonPtr->nodeType != APP_UP_NODE_MDC) {
			printf("UP client daemon: %s will not process data chunk\n",
					node->hostname);
			break;
		} // Should not happen
		// Because non-MDC nodes will not send MSG_APP_UP_DataChunkReceived
		assert(chunkIdentifier > 0);
//		assert(nextStop);
		if(!nextStop) {
			printf("UP client daemon: %s disregarded past data chunk, "
					"identifier=%d\n",
					node->hostname,
					chunkIdentifier);
			break;
		}
		if(nextStop->lsDId->count(chunkIdentifier) > 0) {
			nextStop->lsDId->at(chunkIdentifier) =
					APP_UP_PLAN_TASK_WAIT;
		} else { // Received data chunk from another DS
			AppUpPathStop* ptrStop;

			for(ptrStop = nextStop; ptrStop; ptrStop = ptrStop->next) {
				if(ptrStop->lsDId->count(chunkIdentifier) > 0) {
					ptrStop->lsDId->at(chunkIdentifier) =
							APP_UP_PLAN_TASK_WAIT;
					break;
				}
			}
			if(!ptrStop) {
				printf("UP client daemon: %s disregarded past data chunk, "
						"identifier=%d\n",
						node->hostname,
						chunkIdentifier);
			}
		}
		break; }
	case MSG_APP_UP_DataChunkReceived: {
		int chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = *(int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);

//		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);

		if(clientDaemonPtr->nodeType != APP_UP_NODE_MDC) {
			printf("UP client daemon: %s will not process data chunk\n",
					node->hostname);
			break;
		} // Should not happen
		// Because non-MDC nodes will not send MSG_APP_UP_DataChunkReceived
/*		if(chunkIdentifier <= 0) {
			printf("UP client daemon: %s will not process test chunk\n",
					node->hostname);
			break;
		}*/ // Should not happen in normal operation
		// Because generation script will not make data chunks with id < 0
		assert(chunkIdentifier > 0);

		AppUpClientDaemonDataChunkStr* chunkToAdd;
		AppUpPathStop* nextStop = clientDaemonPtr->path;

		chunkToAdd = (AppUpClientDaemonDataChunkStr*)
				MEM_malloc(sizeof(AppUpClientDaemonDataChunkStr));
		memcpy(chunkToAdd, chunk, sizeof(AppUpClientDaemonDataChunkStr));

		chunkToAdd->next = clientDaemonPtr->dataChunks;
		chunkToAdd->dirty = false;
		clientDaemonPtr->dataChunks = chunkToAdd;
//		assert(nextStop);
		if(!nextStop) {
			printf("UP client daemon: %s disregarded past data chunk, "
					"identifier=%d",
					node->hostname,
					chunkIdentifier);
			break;
		}
		if(nextStop->lsDId->count(chunkIdentifier) > 0) {
			printf("UP client daemon: %s <- %d (%.1f, %.1f, %.1f)\n",
					node->hostname,
					chunkIdentifier,
					nextStop->crds.cartesian.x,
					nextStop->crds.cartesian.y,
					nextStop->crds.cartesian.z);
			nextStop->lsDId->at(chunkIdentifier) =
					APP_UP_PLAN_TASK_COMP;
		} else { // Received data chunk from another DS
			AppUpPathStop* ptrStop;

			printf("UP client daemon: %s <- %d\n",
					node->hostname,
					chunkIdentifier);
			for(ptrStop = nextStop; ptrStop; ptrStop = ptrStop->next) {
				if(ptrStop->lsDId->count(chunkIdentifier) > 0) {
					ptrStop->lsDId->at(chunkIdentifier) =
							APP_UP_PLAN_TASK_COMP;
					break;
				}
			}
			if(!ptrStop) {
				printf("UP client daemon: %s disregarded past data chunk, "
						"identifier=%d",
						node->hostname,
						chunkIdentifier);
				break;
			}
		}
		clientDaemonPtr->timeoutId += 1;

		AppUpClientDaemonSetNextPathStopTimeout(
				node,
				clientDaemonPtr,
				APP_UP_PATH_STOP_TIMEOUT * SECOND);

		printf("\033[1;32m"
				"UP client daemon: %s completed with DS, identifier=%d\n"
				"\033[0m",
				node->hostname,
				chunkIdentifier);

//		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
		daemonRecFile.open(daemonRecFileName, ios::app);
		daemonRecFile << "MDC" << " "
				<< node->hostname
				<< " " << "COMP DATA" <<" "
				<< chunkIdentifier
				<< " " << "AT TIME" << " "
				<< clockInSecond
				<< std::endl;
		daemonRecFile.close();

		if(nextStop->lsAId->size() < 1) {
			AppUpClientDaemonCheckStop(node, clientDaemonPtr, false);
		} /*else if(clientDaemonPtr->joinedAId < 0) {
			AppUpClientDaemonCheckStop(node, clientDaemonPtr, false);
		} else {
			for(map<int, int>::iterator it = nextStop->lsAId->begin();
					it != nextStop->lsAId->end();
					++it){
				if(it->first == clientDaemonPtr->joinedAId) {
					if(it->second == APP_UP_PLAN_TASK_COMP) {
						it->second = APP_UP_PLAN_TASK_WAIT;
					}
					break;
				}
			}
			AppUpClientDaemonSendNextDataChunk(node, clientDaemonPtr, 0);
		}*/
		break; }
	case MSG_APP_UP_TransportConnectionFailed: {
		int chunkIdentifier;
		AppUpClientDaemonDataChunkStr* chunk;

		chunkIdentifier = *(int*)MESSAGE_ReturnInfo(msg);
		chunk = (AppUpClientDaemonDataChunkStr*)MESSAGE_ReturnPacket(msg);

//		clientDaemonPtr = AppUpClientGetUpClientDaemon(node);

		clientDaemonPtr->sending -= 1;
		printf("UP client daemon: %s failed to connect for delivery, "
				"id=%d connAttempted=%d\n",
				node->hostname,
				chunkIdentifier,
				clientDaemonPtr->connAttempted);

		map<int, int>* plan = clientDaemonPtr->plan;
		int joinedAId = clientDaemonPtr->joinedAId;

		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC
				&& (joinedAId < 0
						|| (chunkIdentifier > 0
								&& plan->count(chunkIdentifier) > 0
								&& plan->at(chunkIdentifier) != joinedAId))) {
			// No longer at the correct AP
			;
		} else
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
/*			printf("UP client daemon: %s will try to connect, "
					"waitTime=%d\n",
					node->hostname,
					waitTime);*/

			if(chunkIdentifier > 0) {
				AppUpClientDaemonDataChunkStr* chunkPtr;

				for(chunkPtr = clientDaemonPtr->dataChunks;
						chunkPtr;
						chunkPtr = chunkPtr->next) {
					if(chunkPtr->identifier == chunkIdentifier) {
						chunkPtr->dirty = false;
						break;
					}
				}
				AppUpClientDaemonSendNextDataChunk(
						node,
						clientDaemonPtr,
						(clocktype)0);
			} else if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
				clientDaemonPtr->sending += 1;
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

/*			if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
				daemonRecFile.open(daemonRecFileName, ios::app);
				daemonRecFile << "MDC" << " "
					<< node->hostname
					<< " " << "REPR DATA" << " "
					<< chunkIdentifier
					<< " " << "AT TIME" << " "
					<< clockInSecond
					<< std::endl;
				daemonRecFile.close();
			}*/
			++clientDaemonPtr->connAttempted;
		} else {
			if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
				AppUpClientDaemonCompAtA(
						node,
						clientDaemonPtr,
						joinedAId);
			}
		}
		break; }
	case MSG_APP_UP_PathTimer:
		bool initPath;

		initPath = *(bool*)MESSAGE_ReturnInfo(msg);
		AppUpClientDaemonMobilityModelProcess(
				node,
				clientDaemonPtr,
				initPath);
		break;
	case MSG_APP_UP_PathStopTimeout: {
		int timeoutId;
		Coordinates crds;

		timeoutId = *(int*)MESSAGE_ReturnInfo(msg);
		crds = node->mobilityData->current->position;
		if(timeoutId == clientDaemonPtr->timeoutId) {
			if(AppUpClientDaemonCheckStop(node, clientDaemonPtr, true)) {
				printf("\033[1;33m"
						"UP client daemon: %s timed out, timeoutId=%d\n"
						"\033[0m",
						node->hostname,
						timeoutId);
			}
		} else {
			printf("UP client daemon: %s disregarded timeout checker, "
					"timeoutId=%d\n",
					node->hostname,
					timeoutId);
		}
		break; }
	default:
		printf("UP client daemon: %s at time %s received "\
			"message of unknown type %d\n",
			node->hostname,
			clockInSecond,
			msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppUpClientDaemonFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpClient *clientDaemonPtr = (AppDataUpClient*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

//	printf("UP client daemon: Finalized at %s\n", node->hostname);

	// Statistics
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

int AppUpClientDaemonGNDCEverything(AppDataUpClientDaemon* clientDaemonPtr) {
	AppUpClientDaemonDataChunkStr* chunkPtr;
	int chunkId = -1;
	int chunkDeadline = -1;
	float chunkPriority = 0.0;

	for(chunkPtr = clientDaemonPtr->dataChunks;
			chunkPtr;
			chunkPtr = chunkPtr->next) {
		if(chunkPtr->dirty == false) {
			if(chunkId < 0 || chunkPriority < chunkPtr->priority
					|| (chunkPriority == chunkPtr->priority
							&& chunkDeadline > chunkPtr->deadline)) {
				chunkId = chunkPtr->identifier;
				chunkDeadline = chunkPtr->deadline;
				chunkPriority = chunkPtr->priority;
			}
		}
	}
	return chunkId;
}

int AppUpClientDaemonGNDCPlanned(AppDataUpClientDaemon* clientDaemonPtr) {
	map<int, int>* plan = clientDaemonPtr->plan;
	AppUpClientDaemonDataChunkStr* chunkPtr;
	int joinedAId = clientDaemonPtr->joinedAId;
	int chunkId = -1;
	int chunkDeadline = -1;
	float chunkPriority = 0.0;

	if(joinedAId < 1) return chunkId; // -1
	for(chunkPtr = clientDaemonPtr->dataChunks;
			chunkPtr;
			chunkPtr = chunkPtr->next) {
		if(plan->count(chunkPtr->identifier) > 0
		&& plan->at(chunkPtr->identifier) == joinedAId
		&& chunkPtr->dirty == false) {
			if(chunkId < 0 || chunkPriority < chunkPtr->priority
					|| (chunkPriority == chunkPtr->priority
							&& chunkDeadline > chunkPtr->deadline)) {
				chunkId = chunkPtr->identifier;
				chunkDeadline = chunkPtr->deadline;
				chunkPriority = chunkPtr->priority;
			}
		}
	}
	return chunkId;
}

void AppUpClientDaemonSendNextDataChunk(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		AppUpClientDaemonGetNextDataChunkType getNextDataChunk,
		Address sourceAddr,
		Address destAddr,
		char* sourceString,
		int waitTime,
		char* daemonRecFileName) {
	char clockInSecond[MAX_STRING_LENGTH];
	int chunkIdToSend;
	ofstream daemonRecFile;

	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
	chunkIdToSend = getNextDataChunk(clientDaemonPtr);
	if(chunkIdToSend > 0) {
		AppUpClientDaemonDataChunkStr* chunkPtr;

		for(chunkPtr = clientDaemonPtr->dataChunks;
				chunkPtr;
				chunkPtr = chunkPtr->next) {
			if(chunkPtr->identifier == chunkIdToSend) {
				break;
			}
		}
		assert(chunkPtr);
		chunkPtr->dirty = true;
//		MEM_free(chunkHeader);

		clientDaemonPtr->sending += 1;
		printf("UP client daemon: %s will try to connect, "
				"waitTime=%d sending=%d\n",
				node->hostname,
				waitTime,
				clientDaemonPtr->sending);
//		TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);
		AppUpClientInit(
				node,
				sourceAddr,
				destAddr,
				clientDaemonPtr->applicationName->c_str(),
				sourceString,
				clientDaemonPtr->nodeType,
				waitTime,
				chunkPtr);
		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			daemonRecFile.open(daemonRecFileName, ios::app);
			daemonRecFile << "MDC" << " "
					<< node->hostname
					<< " " << "PREP DATA" << " "
					<< chunkIdToSend
					<< " " << "AT TIME" << " "
					<< clockInSecond
					<< std::endl;
			daemonRecFile.close();
		}
	} else {
		printf("UP client daemon: %s has nothing to send\n",
				node->hostname);
		if(clientDaemonPtr->nodeType == APP_UP_NODE_MDC) {
			AppUpClientDaemonCompAtA(
					node,
					clientDaemonPtr,
					clientDaemonPtr->joinedAId);
		}
	}
}

void AppUpClientDaemonSendNextDataChunk(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		int waitTime) {
	char sourceString[MAX_STRING_LENGTH];
	char destString[MAX_STRING_LENGTH];
	NodeAddress sourceNodeId;
	Address sourceAddr;
	NodeAddress destNodeId;
	Address destAddr;
	char sourceAddrStr[MAX_STRING_LENGTH];
	char destAddrStr[MAX_STRING_LENGTH];
	char daemonRecFileName[MAX_STRING_LENGTH];

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
	sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
	AppUpClientDaemonSendNextDataChunk(
			node,
			clientDaemonPtr,
			clientDaemonPtr->getNextDataChunk,
			sourceAddr,
			destAddr,
			sourceString,
			waitTime,
			daemonRecFileName);
}

void AppUpClientDaemonSetNextPathTimer(
		Node* node,
		clocktype interval,
		bool init) {
	Message* msg;
	ActionData acnData;

	msg = MESSAGE_Alloc(node,
			APP_LAYER,
			APP_UP_CLIENT_DAEMON,
			MSG_APP_UP_PathTimer);
	MESSAGE_InfoAlloc(node, msg, sizeof(bool));
	memcpy(MESSAGE_ReturnInfo(msg), &init, sizeof(bool));

	//Trace Information
	acnData.actionType = SEND;
	acnData.actionComment = NO_COMMENT;
	TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
			PACKET_OUT, &acnData);
	MESSAGE_Send(node, msg, interval);
}

void AppUpClientDaemonSetNextPathStopTimeout(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		clocktype interval) {
	Message* msg;
	ActionData acnData;

	msg = MESSAGE_Alloc(node,
			APP_LAYER,
			APP_UP_CLIENT_DAEMON,
			MSG_APP_UP_PathStopTimeout);
	MESSAGE_InfoAlloc(node, msg, sizeof(int));
	memcpy(MESSAGE_ReturnInfo(msg),
			&clientDaemonPtr->timeoutId,
			sizeof(int));

	printf("UP client daemon: %s set this stop's timeout, timeoutId=%d\n",
			node->hostname,
			clientDaemonPtr->timeoutId);

	//Trace Information
	acnData.actionType = SEND;
	acnData.actionComment = NO_COMMENT;
	TRACE_PrintTrace(node, msg, TRACE_APPLICATION_LAYER,
			PACKET_OUT, &acnData);
	MESSAGE_Send(node, msg, interval);
}

void AppUpClientDaemonMobilityModelProcess(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		bool init) {
	Coordinates crds;
	Orientation ornt;
	Coordinates crdsStop;
	Orientation orntPrep;
	CoordinateType distance = (CoordinateType)0;
	clocktype timeNow = node->getNodeTime();
	CoordinateRepresentationType coordinateSystemType =
			(CoordinateRepresentationType)
			node->partitionData->terrainData->getCoordinateSystem();

	MOBILITY_ReturnCoordinates(node, &crds);
	MOBILITY_ReturnOrientation(node, &ornt);
	orntPrep.azimuth = 0;
	orntPrep.elevation = 0;

/*	printf("UP client daemon: %s path timer triggered\n",
			node->hostname);*/
	if(init) { // Initialize my mobility model
		clientDaemonPtr->initPos = crds;
		printf("\033[1;33m"
				"UP client daemon: %s is initialized at (%.1f, %.1f, %.1f)\n"
				"\033[0m",
				node->hostname,
				clientDaemonPtr->initPos.cartesian.x,
				clientDaemonPtr->initPos.cartesian.y,
				clientDaemonPtr->initPos.cartesian.z);
	}
	if(clientDaemonPtr->path == NULL) return;

	Coordinates crdsNext;
	AppUpPathStop* stopNext = clientDaemonPtr->path;

	crdsStop = stopNext->crds;
	COORD_CalcDistance(coordinateSystemType, &crds, &crdsStop, &distance);
	if(distance < APP_UP_PATH_TOL) {
		clocktype timeStay = (clocktype)0;

		if(init) { // Stay
			if(stopNext->t * SECOND > timeNow) {
				timeStay = stopNext->t * SECOND - timeNow;
			}
			MOBILITY_InsertANewEvent(node,
					timeNow + timeStay,
					crdsStop, orntPrep, 0.);
			AppUpClientDaemonSetNextPathTimer(node, timeStay, false);
		} else { // Arrived
			printf("\033[1;33m"
					"UP client daemon: %s arrived at (%.1f, %.1f, %.1f)\n"
					"\033[0m",
					node->hostname,
					crds.cartesian.x,
					crds.cartesian.y,
					crds.cartesian.z);
			if(stopNext->lsAId->size() < 1 && stopNext->lsDId->size() < 1) {
				clientDaemonPtr->path = stopNext->next;
				delete(stopNext->lsAId);
				delete(stopNext->lsDId);
				MEM_free(stopNext);
				AppUpClientDaemonSetNextPathTimer(node, (clocktype)0, false);
			} else {
				if(!AppUpClientDaemonCheckStop(node, clientDaemonPtr, false)) {
					if(clientDaemonPtr->sending < 1
							&&clientDaemonPtr->joinedAId > 0
							&&stopNext->lsAId->size() > 0) {
						AppUpClientDaemonSendNextDataChunk(
								node,
								clientDaemonPtr,
								0);
					} else {
						int timeout = APP_UP_PATH_STOP_TIMEOUT;

						if(stopNext->lsDId->size() > 0) {
							timeout = APP_UP_PATH_STOP_TIMEOUT_2;
						}
						AppUpClientDaemonSetNextPathStopTimeout(
								node,
								clientDaemonPtr,
								timeout * SECOND);
					}
				}
			}
		}
	} else { // Moving
		double speed;
		double cosAng = (crdsStop.cartesian.x - crds.cartesian.x) / distance;
		double sinAng = (crdsStop.cartesian.y - crds.cartesian.y) / distance;
		clocktype timeNext;

		if(node->mobilityData->current->speed == 0) {
			speed = distance / stopNext->t;
			printf("UP client daemon: %s started to move at speed %.1f\n",
					node->hostname,
					speed);
			printf("UP client daemon: %s is heading to (%.1f, %.1f, %.1f)\n",
					node->hostname,
					crdsStop.cartesian.x,
					crdsStop.cartesian.y,
					crdsStop.cartesian.z);
		} else {
			speed = node->mobilityData->current->speed;
		}
		if(distance > APP_UP_PATH_SIMU_DISTANCE + APP_UP_PATH_TOL) {
			crdsNext = crds;
			crdsNext.cartesian.x += APP_UP_PATH_SIMU_DISTANCE * cosAng;
			crdsNext.cartesian.y += APP_UP_PATH_SIMU_DISTANCE * sinAng;
			timeNext = APP_UP_PATH_SIMU_DISTANCE / speed * SECOND;
		} else {
			crdsNext = crdsStop;
			timeNext = distance / speed * SECOND;
			speed = 0;
		}
		MOBILITY_InsertANewEvent(node,
				timeNow + timeNext,
				crdsNext, orntPrep, speed);
		AppUpClientDaemonSetNextPathTimer(node, timeNext, false);
	}
}

bool AppUpClientDaemonCheckStop(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		bool timeoutFlag) {
	assert(clientDaemonPtr->path);

	bool completed = true;
	map<int, int>* tasks;
	AppUpPathStop* stopNext = clientDaemonPtr->path;

	tasks = stopNext->lsAId;
	for(map<int, int>::iterator it = tasks->begin();
			it != tasks->end();
			++it) {
		if((!timeoutFlag && it->second == APP_UP_PLAN_TASK_INIT)
				|| it->second == APP_UP_PLAN_TASK_WAIT) {
			completed = false;
		}
	}
	tasks = stopNext->lsDId;
	for(map<int, int>::iterator it = tasks->begin();
			it != tasks->end();
			++it) {
		if((!timeoutFlag && it->second == APP_UP_PLAN_TASK_INIT)
				|| it->second == APP_UP_PLAN_TASK_WAIT) {
			completed = false;
		}
	}

	if(completed) {
		if(node->mobilityData->current->speed > 0) {
			printf("\033[1;33m"
					"UP client daemon: %s signaled completion before arrival\n",
					node->hostname);
		} else {
			clientDaemonPtr->timeoutId += 1;

			printf("\033[1;33m"
					"UP client daemon: %s completed at (%.1f, %.1f, %.1f)\n"
					"\033[0m",
					node->hostname,
					stopNext->crds.cartesian.x,
					stopNext->crds.cartesian.y,
					stopNext->crds.cartesian.z);

			clientDaemonPtr->path = stopNext->next;
			delete(stopNext->lsAId);
			delete(stopNext->lsDId);
			MEM_free(stopNext);
			AppUpClientDaemonSetNextPathTimer(node, (clocktype)0, false);
		}
	} else {
		;
	}
	return completed;
}

void AppUpClientDaemonCompAtA(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		int joinedAId) {
	char clockInSecond[MAX_STRING_LENGTH];
	char daemonRecFileName[MAX_STRING_LENGTH];
	ofstream daemonRecFile;
	AppUpPathStop* nextStop = clientDaemonPtr->path;

	sprintf(daemonRecFileName, "daemon_%s.out", node->hostname);
	TIME_PrintClockInSecond(node->getNodeTime(), clockInSecond);

	assert(nextStop);
	if(nextStop->lsAId->count(joinedAId) > 0) {
		printf("UP client daemon: %s -> %d (%.1f, %.1f, %.1f)\n",
				node->hostname,
				joinedAId,
				nextStop->crds.cartesian.x,
				nextStop->crds.cartesian.y,
				nextStop->crds.cartesian.z);
		nextStop->lsAId->at(joinedAId) =
				APP_UP_PLAN_TASK_COMP;
	} else {
		for(AppUpPathStop* ptrStop = nextStop;
				ptrStop;
				ptrStop = ptrStop->next) {
			if(ptrStop->lsAId->count(joinedAId) > 0) {
				ptrStop->lsAId->at(joinedAId) =
						APP_UP_PLAN_TASK_WAIT;
				break;
			}
		}
	}
	clientDaemonPtr->timeoutId += 1;

	AppUpClientDaemonSetNextPathStopTimeout(
			node,
			clientDaemonPtr,
			APP_UP_PATH_STOP_TIMEOUT * SECOND);

	printf("\033[1;32m"
			"UP client daemon: %s completed with AP, "
			"identifier=%d\n"
			"\033[0m",
			node->hostname,
			clientDaemonPtr->joinedAId);

	daemonRecFile.open(daemonRecFileName, ios::app);
	daemonRecFile << "MDC" << " "
			<< node->hostname
			<< " " << "COMP AP" <<" "
			<< clientDaemonPtr->joinedAId
			<< " " << "AT TIME" << " "
			<< clockInSecond
			<< std::endl;
	daemonRecFile.close();

	AppUpClientDaemonCheckStop(node, clientDaemonPtr, false);
}

