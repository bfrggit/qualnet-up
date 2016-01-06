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

typedef struct struct_app_up_client_daemon_data_chunk_str {
	int         identifier;
	int         size; // KB
	int         deadline;
	float       priority;
	bool        dirty;
	struct_app_up_client_daemon_data_chunk_str* next;
} AppUpClientDaemonDataChunkStr;

typedef struct struct_app_up_server_item_data {
	Int32       sizeExpected;
	Int32       sizeReceived;
	AppUpClientDaemonDataChunkStr dataChunk;
} AppUpServerItemData;

typedef struct struct_app_up_client_packet_list {
	char*       payload;
	Int32       packetSize;
	struct_app_up_client_packet_list* next;
} AppUpClientPacketList;

typedef struct struct_app_up_server_str {
	int         connectionId;
	Address     localAddr;
	Address     remoteAddr;
	short       localPort;
	short       remotePort;
	int         uniqueId;
	RandomSeed  seed;
	AppUpNodeType nodeType;
	AppUpServerItemData itemData;
	bool        sessionIsClosed;
	clocktype   sessionStart;
	clocktype   sessionFinish;
	STAT_AppStatistics* stats;
} AppDataUpServer;

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
	AppUpClientPacketList* packets;
	pthread_mutex_t packetsMutex;
	bool        sessionIsClosed;
	clocktype   sessionStart;
	clocktype   sessionFinish;
	STAT_AppStatistics* stats;
	std::string* applicationName;
	AppUpClientDaemonDataChunkStr* dataChunk;
} AppDataUpClient;

typedef enum enum_app_up_message_type {
	APP_UP_MSG_DATA = APP_UP_NODE_DATA_SITE
} AppUpMessageType;

typedef struct struct_app_up_message_header {
	AppUpMessageType type;
	Int32       itemSize;
	AppUpClientDaemonDataChunkStr dataChunk;
} AppUpMessageHeader;

typedef enum enum_app_up_plan_task_status {
	APP_UP_PLAN_TASK_INIT,
	APP_UP_PLAN_TASK_WAIT,
	APP_UP_PLAN_TASK_COMP
} AppUpPlanTaskStatus;

typedef struct struct_app_up_path_stop {
	double      t;
	Coordinates crds;
	map<int, int>* lsAId;
	map<int, int>* lsDId;
	struct_app_up_path_stop* next;
} AppUpPathStop;

typedef struct struct_app_up_client_daemon_str {
	Node*       firstNode;
	NodeAddress sourceNodeId;
	NodeAddress destNodeId;
	AppUpNodeType nodeType;
	std::string* inputString;
	std::string* applicationName;
	AppUpClientDaemonDataChunkStr* dataChunks;
	bool        test; // Initialize into test mode if plan not present
	int         joinedAId;
	map<int, int>* plan;
	int         connAttempted;
	AppUpPathStop* path;
	Coordinates initPos;
	int         timeoutId;
	int         sending; // Number of data chunks prepared for sending
	int (*getNextDataChunk)(struct_app_up_client_daemon_str*);
} AppDataUpClientDaemon;

typedef int (*AppUpClientDaemonGetNextDataChunkType)(AppDataUpClientDaemon*);

void AppUpServerInit(
	Node *node,
	Address serverAddr);

void AppUpClientInit(
	Node* node,
	Address clientAddr,
	Address serverAddr,
	const char* appName,
	char* sourceString,
	AppUpNodeType nodeType,
	int waitTime,
	AppUpClientDaemonDataChunkStr* chunk);

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
	const char* appName,
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

// Abandoned for virtual packets
/*
void AppUpClientSendNextPacket(
		Node *node,
		AppDataUpClient *clientPtr);*/

// Abandoned for virtual packets
/*
char* AppUpClientNewDataItem(
		Int32 itemSize,
		Int32& fullSize,
		int identifier,
		int deadline,
		float priority);*/

// Abandoned for virtual packets
/*
void AppUpClientSendItem(
		Node* node,
		AppDataUpClient *clientPtr,
		char* item,
		Int32 itemSize);*/

char* AppUpClientNewVirtualDataItem(
		Int32 itemSize,
		Int32& fullSize,
		int identifier,
		int deadline,
		float priority);

void AppUpClientSendVirtualItem(
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

void AppUpSendGeneralMessageToServer(Node* node, clocktype delay);
void AppUpSendGeneralMessageToClient(Node* node, clocktype delay);

void AppUpClientDaemonInit(
		Node* node,
		Node* firstNode,
		NodeAddress sourceNodeId,
		NodeAddress destNodeId,
		char* inputString,
		char* appName,
		AppUpNodeType nodeType);

AppDataUpClientDaemon* AppUpClientNewUpClientDaemon(
		Node* node,
		Node* firstNode,
		NodeAddress sourceNodeId,
		NodeAddress destNodeId,
		char* inputString,
		char* appName,
		AppUpNodeType nodeType);

void AppLayerUpClientDaemon(Node *node, Message *packet);
void AppUpClientDaemonFinalize(Node *node, AppInfo *appInfo);

AppDataUpClientDaemon* AppUpClientGetUpClientDaemon(Node *node);

const CoordinateType APP_UP_WIRELESS_CLOSE_RANGE = (CoordinateType)0;
const int APP_UP_WIRELESS_AP_WAIT_TIME = 5;
const int APP_UP_WIRELESS_MDC_WAIT_TIME = 5;
const int APP_UP_OPEN_CONN_ATTEMPT_MAX = 15;
const CoordinateType APP_UP_PATH_TOL = 1e-4;
const CoordinateType APP_UP_PATH_SIMU_DISTANCE = (CoordinateType)1;
//const clocktype APP_UP_PATH_SIMU_TIME = 100 * MILLI_SECOND;
const int APP_UP_PATH_STOP_TIMEOUT = 15;
const int APP_UP_PATH_STOP_TIMEOUT_2 = APP_UP_OPEN_CONN_ATTEMPT_MAX * 3;

int AppUpClientDaemonGNDCEverything(AppDataUpClientDaemon* clientDaemonPtr);
int AppUpClientDaemonGNDCPlanned(AppDataUpClientDaemon* clientDaemonPtr);

void AppUpClientDaemonSendNextDataChunk(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		AppUpClientDaemonGetNextDataChunkType getNextDataChunk,
		Address sourceAddr,
		Address destAddr,
		char* sourceString,
		int waitTime,
		char* daemonRecFileName);

void AppUpClientDaemonSendNextDataChunk(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		int waitTime);

void AppUpClientDaemonSetNextPathTimer(
		Node* node,
		clocktype interval,
		bool init);

void AppUpClientDaemonSetNextPathStopTimeout(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		clocktype interval);

void AppUpClientDaemonMobilityModelProcess(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		bool init);

bool AppUpClientDaemonCheckStop(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		bool timeoutFlag);

void AppUpClientDaemonCompAtA(
		Node* node,
		AppDataUpClientDaemon* clientDaemonPtr,
		int joinedAId);

#endif
