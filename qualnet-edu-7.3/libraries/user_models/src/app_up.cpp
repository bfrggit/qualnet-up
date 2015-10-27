#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "api.h"
#include "app_util.h"
#include "transport_tcp.h"
#include "tcpapps.h"
#include "partition.h"
#include "ipv6.h"

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

	IO_ConvertIpAddressToString(&serverAddr, addrStr);
	printf("UP server: Initialized at node_%d (%s)\n",
		node->nodeId,
		addrStr);
}

void AppUpClientInit(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	char* appName,
	char* sourceString,
	AppUpNodeType nodeType) {
	AppDataUpClient *clientPtr;
	char addrStr[MAX_STRING_LENGTH];

	clientPtr = AppUpClientNewUpClient(
		node,
		clientAddr,
		serverAddr,
		appName,
		nodeType);
	if(!clientPtr) { // Failed initialization
		fprintf(stderr, "UP client: node_%d cannot allocate "
			"new client\n", node->nodeId);
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
	AppUpClientAddAddressInformation(node, clientPtr);

	IO_ConvertIpAddressToString(&clientAddr, addrStr);
	printf("UP client: Initialized at node_%d (%s)\n",
		node->nodeId,
		addrStr);

	// Open connection
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
		TIME_ConvertToClock("10"),
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
	upServer->itemSizeExpected = -1;
	upServer->itemSizeReceived = 0;
	upServer->sessionIsClosed = false;
	upServer->sessionStart = node->getNodeTime();
	upServer->sessionFinish = node->getNodeTime();
	upServer->stats = NULL;

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
	char* appName,
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
/*	printf("UP server: node_%d at time %s processed an event\n",
		node->nodeId,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransListenResult:
			TransportToAppListenResult *listenResult;

			listenResult = (TransportToAppListenResult*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at node_%d got listen result\n",
				buf, node->nodeId);*/

			if(listenResult->connectionId < 0) {
				printf("%s: UP server at node_%d listen failed\n",
					buf,
					node->nodeId);
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
/*			printf("%s: UP server at node_%d got open result\n",
				buf, node->nodeId);*/
			assert(openResult->type == TCP_CONN_PASSIVE_OPEN);

			if(openResult->connectionId < 0) {
				printf("%s: UP server at node_%d connection failed\n",
					buf,
					node->nodeId);
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
/*			printf("%s: UP server at node_%d sent data\n",
				buf, node->nodeId);*/
			printf("UP server: node_%d sent data, packetSize=%d\n",
					node->nodeId,
					dataSent->length);
			break;
		case MSG_APP_FromTransDataReceived:
			TransportToAppDataReceived* dataReceived;
			char *packet;
			int packetSize;

			dataReceived = (TransportToAppDataReceived*)MESSAGE_ReturnInfo(msg);
			packet = MESSAGE_ReturnPacket(msg);
			packetSize = MESSAGE_ReturnPacketSize(msg);

/*			printf("%s: UP server at node_%d received data\n",
				buf, node->nodeId);*/
			printf("UP server: node_%d received data, "
					"connectionId=%d packetSize=%d\n",
					node->nodeId,
					dataReceived->connectionId,
					packetSize);

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

				serverPtr->itemSizeExpected = header->itemSize;
				if(packet[packetSize - 1] == '$') {
					capSize = sizeof(AppUpMessageHeader) + 2;
				} else {
					capSize = sizeof(AppUpMessageHeader) + 1;
				}
				serverPtr->itemSizeReceived += packetSize - capSize;
			} else if(packet[packetSize - 1] == '$') {
				serverPtr->itemSizeReceived += packetSize - 1;
				node->appData.appTrafficSender->appTcpCloseConnection(
						node,
						serverPtr->connectionId);
				printf("UP server: node_%d disconnecting, connectionId=%d\n",
						node->nodeId,
						serverPtr->connectionId);
				if (node->appData.appStats) {
					if (!serverPtr->stats->IsSessionFinished()) {
						serverPtr->stats->SessionFinish(node);
					}
				}
			} else {
				serverPtr->itemSizeReceived += packetSize;
			}
			break;
		case MSG_APP_FromTransCloseResult:
			TransportToAppCloseResult *closeResult;

			closeResult = (TransportToAppCloseResult*)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP server at node_%d got close result\n",
					buf, node->nodeId);*/
			if(closeResult->type == TCP_CONN_PASSIVE_CLOSE) {
				printf("UP server: node_%d passively closed, "
						"connectionId=%d\n",
						node->nodeId,
						closeResult->connectionId);
			} else {
				printf("UP server: node_%d actively closed, "
						"connectionId=%d\n",
						node->nodeId,
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
			printf("%s: UP server at node_%d timer expired\n",
				buf, node->nodeId);
			break;
		default:
			printf("UP server: node_%d at time %s received "\
				"message of unknown type %d\n",
				node->nodeId,
				buf,
				msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppLayerUpClient(Node *node, Message *msg) {
	char buf[MAX_STRING_LENGTH];
	AppDataUpClient* clientPtr;

	ctoa(node->getNodeTime(), buf);
/*	printf("UP client: node_%d at time %s processed an event\n",
		node->nodeId,
		buf);*/

	switch(msg->eventType) {
		case MSG_APP_FromTransOpenResult:
			TransportToAppOpenResult *openResult;

			openResult = (TransportToAppOpenResult *)MESSAGE_ReturnInfo(msg);
/*			printf("%s: UP client at node_%d got open result\n",
				buf, node->nodeId);*/
			assert(openResult->type == TCP_CONN_ACTIVE_OPEN);

			if(openResult->connectionId < 0) {
				printf("%s: UP client at node_%d connection failed\n",
					buf,
					node->nodeId);
				node->appData.numAppTcpFailure ++;
			} else {
				AppDataUpClient* clientPtr;
				char* item;
				Int32 itemSize = 65536;
				Int32 fullSize;

				clientPtr = AppUpClientUpdateUpClient(node, openResult);
				assert(clientPtr != NULL);

				// XXX: Test only
				item = AppUpClientNewDataItem(itemSize, fullSize);
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
/*			printf("%s: UP client at node_%d sent data\n",
				buf, node->nodeId);*/
			printf("UP client: node_%d sent data, packetSize=%d\n",
					node->nodeId,
					dataSent->length);

			clientPtr = AppUpClientGetUpClient(node, dataSent->connectionId);
			assert(clientPtr != NULL);

			pthread_mutex_lock(&clientPtr->packetListMutex);
			if(clientPtr->packetList) {
				AppUpClientSendNextPacket(node, clientPtr);
			} else if(clientPtr->sessionIsClosed) {
				node->appData.appTrafficSender->appTcpCloseConnection(
						node,
						clientPtr->connectionId);
				printf("UP client: node_%d disconnecting, connectionId=%d\n",
						node->nodeId,
						clientPtr->connectionId);
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

/*			printf("%s: UP client at node_%d received data\n",
				buf, node->nodeId);*/
			printf("UP client: node_%d received data, "
					"connectionId=%d packetSize=%d\n",
					node->nodeId,
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
			printf("%s: UP client at node_%d got close result\n",
				buf, node->nodeId);

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
		default:
			printf("UP client: node_%d at time %s received "\
				"message of unknown type %d\n",
				node->nodeId,
				buf,
				msg->eventType);
	}
	MESSAGE_Free(node, msg);
}

void AppUpServerFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpServer *serverPtr = (AppDataUpServer*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

	// IO_ConvertIpAddressToString(&serverPtr->localAddr, addrStr);
	printf("UP server: Finalized at node_%d\n", node->nodeId);

//	TODO: Statistics
	if(node->appData.appStats) {
		;
	}
}

void AppUpClientFinalize(Node *node, AppInfo *appInfo) {
	AppDataUpClient *clientPtr = (AppDataUpClient*)appInfo->appDetail;
	char addrStr[MAX_STRING_LENGTH];

	// IO_ConvertIpAddressToString(&clientPtr->localAddr, addrStr);
	printf("UP client: Finalized at node_%d\n", node->nodeId);

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
		printf("%s: UP client at node_%d attempted invalid operation\n",
				buf,
				node->nodeId);
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

		//	TODO: Statistics
		if (node->appData.appStats)
		{
			;
		}
	}
}

char* AppUpClientNewDataItem(Int32 itemSize, Int32& fullSize) {
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
		printf("%s: UP client at node_%d attempted invalid operation\n",
				buf,
				node->nodeId);
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
