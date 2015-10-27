#ifndef _UP_APP_H
#define _UP_APP_H

// typedef struct struct_app_up_data {
// 	char type;
// } UpData;

typedef enum enum_app_up_node_type {
	APP_UP_NODE_CLOUD = 10,
	APP_UP_NODE_MDC,
	APP_UP_NODE_DATA_SITE
} AppUpNodeType;


typedef struct struct_app_up_server_str {
	int         connectionId;
	Address     localAddr;
	Address     remoteAddr;
	short       localPort;
	short       remotePort;
	int         uniqueId;
	RandomSeed  seed;
	Int32       itemSizeExpected;
	Int32       itemSizeReceived;
	bool        sessionIsClosed;
	clocktype   sessionStart;
	clocktype   sessionFinish;
	STAT_AppStatistics* stats;
} AppDataUpServer;

typedef struct struct_app_up_client_packet_list {
	char*       payload;
	Int32       packetSize;
	struct_app_up_client_packet_list* next;
} AppUpClientPacketList;

typedef struct struct_app_up_client_str {
	int         connectionId;
	Address     localAddr;
	Address     remoteAddr;
	short       localPort;
	short       remotePort;
	int         uniqueId;
	RandomSeed  seed;
	NodeId      destNodeId; // destination node id for this app session
	Int16       clientInterfaceIndex;
	Int16       destInterfaceIndex;
	AppUpNodeType nodeType;
	AppUpClientPacketList* packetList;
	pthread_mutex_t packetListMutex;
	bool        sessionIsClosed;
	clocktype   sessionStart;
	clocktype   sessionFinish;
	STAT_AppStatistics* stats;
	std::string* applicationName;
} AppDataUpClient;

typedef enum enum_app_up_message_type {
	APP_UP_MSG_DATA = 10
} AppUpMessageType;

typedef struct struct_app_up_message_header {
	AppUpMessageType type;
	Int32       itemSize;
} AppUpMessageHeader;

void AppUpServerInit(
	Node *node,
	Address serverAddr);

void AppUpClientInit(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	char* appName,
	char* sourceString,
	AppUpNodeType nodeType);

void AppLayerUpServer(Node *node, Message *packet);
void AppLayerUpClient(Node *node, Message *packet);
void AppUpServerFinalize(Node *node, AppInfo *appInfo);
void AppUpClientFinalize(Node *node, AppInfo *appInfo);

AppDataUpServer* AppUpServerNewUpServer(
	Node* node,
	TransportToAppOpenResult *openResult);

AppDataUpClient* AppUpClientNewUpClient(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	char* appName,
	AppUpNodeType nodeType);

void AppUpServerPrintStats(Node *node, AppDataUpServer *serverPtr);
void AppUpClientPrintStats(Node *node, AppDataUpClient *clientPtr);

void AppUpClientAddAddressInformation(
	Node* node,
	AppDataUpClient* clientPtr);

AppDataUpServer* AppUpServerGetUpServer(Node *node, int connId);
AppDataUpClient* AppUpClientGetUpClient(Node *node, int connId);
AppDataUpClient* AppUpClientGetClientPtr(
	Node* node,
	Int32 uniqueId);

void AppUpClientSendNextPacket(
		Node *node,
		AppDataUpClient *clientPtr);

char* AppUpClientNewDataItem(Int32 itemSize, Int32& fullSize);

void AppUpClientSendItem(
		Node* node,
		AppDataUpClient *clientPtr,
		char* item,
		Int32 itemSize);

AppDataUpClient*
AppUpClientUpdateUpClient(
		Node *node,
		TransportToAppOpenResult *openResult);

AppUpClientPacketList* AppUpClientPacketListAppend(
		AppUpClientPacketList* list,
		AppUpClientPacketList* ptr);

#endif
