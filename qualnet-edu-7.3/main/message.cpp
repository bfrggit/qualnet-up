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
 * Functions used for message sending and retrieval
 * in the simulation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include <time.h>

#include "api.h"
#include "memory.h"
#include "node.h"
#include "partition.h"
#include "scheduler.h"
#include "qualnet_mutex.h"
#include "context.h"
#include "pthread.h"

#ifdef CELLULAR_LIB
#include "mac_cellular_abstract.h"
#endif // CELLULAR_LIB

#ifdef ADDON_DB
#include "dbapi.h"
#endif // ADDON_DB

//#define MEMDEBUG
#ifdef MEMDEBUG
// silly counters to help find mismatched allocs/frees
static int allocs = 0;
static int deallocs = 0;
#endif

#define DEBUG 0
#define noMESSAGE_NO_RECYCLE
//#define MESSAGE_NO_RECYCLE
/* Maximum amount of messages that can be kept in the free list. */
#define MSG_LIST_MAX 10000
//#define MSG_LIST_MAX 0

// #define DEBUG_EVENT_TRACE TRUE

// DO not recycle messages.  Useful for memory debugging.
//#define MESSAGE_NO_RECYCLE

// TRACK_UNFREED_MESSAGES - After every message free show which messages
// have not yet been freed.  Turns on MESSAGE_NO_RECYCLE.
//
//#define TRACK_UNFREED_MESSAGES

#ifdef TRACK_UNFREED_MESSAGES

#ifndef MESSAGE_NO_RECYCLE
#define MESSAGE_NO_RECYCLE
#endif

std::set<Message*> gMessageSet;
int gMessageSetPrintCount = 0;
#endif /* TRACK_UNFREED_MESSAGES */

// Message graph outputs a dependency graph that is useful for debugging.
// purposes.  works for -np 1.  Outputs a file in the form:
//
// (originatingNodeId, naturalOrder, eventType) -> (o2, n2, e2)
//
// If a problem exists with a message the dependency graph can be used to
// determine which message type caused that message.  Best run with output
// redirected to a file.  The amount of output will be very large.
//
//#define MESSAGE_GRAPH

#ifdef MESSAGE_GRAPH
static Message gGraphMessage;
#endif

Message::Message() : m_radioId(-1), d_stamped(false)
{
#ifdef TRACK_UNFREED_MESSAGES
    assert(gMessageSet.find(this) == gMessageSet.end());
    gMessageSet.insert(this);
#endif
}

Message::Message(const Message& m) : m_radioId(-1), d_stamped(false)
{
#ifdef TRACK_UNFREED_MESSAGES
    assert(gMessageSet.find(this) == gMessageSet.end());
    gMessageSet.insert(this);
#endif

    *this = m;

    // Not set using the = operator
    mtWasMT = false;
}

Message::Message(
    PartitionData *partition,
    int  layerType,
    int  protocol,
    int  eventType,
    bool isMT) : m_radioId(-1), d_stamped(false)
{
#ifdef TRACK_UNFREED_MESSAGES
    assert(gMessageSet.find(this) == gMessageSet.end());
    gMessageSet.insert(this);
#endif
}

/*
 * FUNCTION     MESSAGE_FreeContents
 * PURPOSE      Free memory for the message contents: Info fields, payload,
 *              and so on.
 *
 * Parameters:
 *    partition:  partition which is freeing the message
 *    msg:        message which has to be freed
 */
void MESSAGE_FreeContents(PartitionData *partition, Message *msg)
{
    bool wasMT = (msg->mtWasMT == TRUE);

    if (msg->payloadSize > 0)
    {
        MESSAGE_PayloadFree(partition, msg->payload, msg->payloadSize, wasMT);
        msg->payload = 0;
        msg->payloadSize = 0;
    }

    for (size_t i = 0; i < msg->infoArray.size(); i ++)
    {
        MessageInfoHeader* hdr = &(msg->infoArray[i]);
        if (hdr->infoSize > 0)
        {
            MESSAGE_InfoFieldFree(partition, hdr, wasMT);
            hdr->infoSize = 0;
            hdr->info = NULL;
            hdr->infoType = INFO_TYPE_UNDEFINED;
        }
    }

    delete msg->m_band;
    msg->m_band = NULL;
    msg->infoArray.clear();
    msg->infoBookKeeping.clear();
}

Message::~Message()
{
    PartitionData* partitionData = SimContext::getPartition();

    ERROR_Assert(!getFreed(), "Message already freed");
    ERROR_Assert(!getDeleted(), "Message already deleted");
    ERROR_Assert(!getSent(), "Freeing a sent message");
    setDeleted(true);

    MESSAGE_FreeContents(partitionData, this);

#ifdef TRACK_UNFREED_MESSAGES
    assert(gMessageSet.find(this) != gMessageSet.end());
    gMessageSet.erase(this);
    if (gMessageSet.size() > 0 && (gMessageSetPrintCount++ == 1000))
    {
        std::map<int, int> count;
        for (std::set<Message*>::iterator it = gMessageSet.begin(); it != gMessageSet.end(); it++)
        {
            count[(*it)->eventType]++;
        }
        printf("\n");
        for (std::map<int, int>::iterator it2 = count.begin(); it2 != count.end(); it2++)
        {
            printf("count[%d] = %d\n", it2->first, it2->second);
        }
        gMessageSetPrintCount = 0;
    }
#endif
}

void Message::initialize(PartitionData* partition)
{
    next                  = NULL;
    m_partitionData       = partition;
    layerType             = 0;
    protocolType          = 0;
    instanceId            = 0;
    m_radioId             = 0;
    eventType             = 0;
    naturalOrder          = 0;
    error                 = FALSE;
    allowLoose            = false;
    mtWasMT               = false;
    nodeId                = 0;
    eventTime             = 0;
    eot                   = 0;
    sourcePartitionId     = 0;
    memset(smallInfoSpace, 0, SMALL_INFO_SPACE_SIZE);
    packetSize            = 0;
    packet                = NULL;
    payload               = NULL;
    payloadSize           = 0;
    virtualPayloadSize    = 0;
    packetCreationTime    = 0;
    pktNetworkSendTime    = 0;
    cancelled             = 0;
    originatingNodeId     = 0;
    sequenceNumber        = 0;
    originatingProtocol   = 0;
    numberOfHeaders       = 0;
    memset(headerProtocols, 0, sizeof(int) * MAX_HEADERS);
    memset(headerSizes, 0, sizeof(int) * MAX_HEADERS);
    relayNodeAddr         = 0;
    originatingProtocol   = -1;
    packetCreationTime    = partition->getGlobalTime();
    subChannelIndex       = -1;
    isPacked              = FALSE;
    actualPktSize         = 0;
    isEmulationPacket     = false;
    timerManager          = NULL;
    isScheduledOnMainHeap = false;
    timerExpiresAt        = 0;
    m_flags               = 0;
    m_band                = NULL;

    MESSAGE_SetLayer(this, DEFAULT_LAYER, TRACE_ANY_PROTOCOL);
    MESSAGE_SetEvent(this, MSG_DEFAULT);
}

void Message::operator = (const Message &msg)
{
    // copied the contents of MESSAGE_Duplicate to create this.

    unsigned int i;

    next                = NULL;
    // this is important.  m_partitionData is used for allocating
    // info, payload, etc. so must be the current partition, not
    // a remote partition.
    // If this is called from MESSAGE_Duplicate, the following is
    // not necessary since MESSAGE_Alloc will have already set the
    // partition.
    m_partitionData     = SimContext::getPartition();

    layerType           = msg.layerType;
    protocolType        = msg.protocolType;
    instanceId          = msg.instanceId;
    m_radioId           = msg.m_radioId;
    eventType           = msg.eventType;
    naturalOrder        = msg.naturalOrder;
    error               = msg.error;
    allowLoose          = msg.allowLoose;
    // mtWasMT is set in MESSAGE_Alloc and must not be overwritten
    nodeId              = msg.nodeId;
    eventTime           = msg.eventTime;
    eot                 = msg.eot;
    sourcePartitionId   = msg.sourcePartitionId;
    packetSize          = msg.packetSize;
    packet              = NULL;
    payload             = NULL;
    payloadSize         = msg.payloadSize;
    virtualPayloadSize  = msg.virtualPayloadSize;
    packetCreationTime  = msg.packetCreationTime;
    pktNetworkSendTime  = msg.pktNetworkSendTime;
    cancelled           = msg.cancelled;
    originatingNodeId   = msg.originatingNodeId;
    sequenceNumber      = msg.sequenceNumber;
    originatingProtocol = msg.originatingProtocol;
    numberOfHeaders     = msg.numberOfHeaders;
    relayNodeAddr       = msg.relayNodeAddr;
    subChannelIndex     = msg.subChannelIndex;
    isPacked            = msg.isPacked;
    actualPktSize       = msg.actualPktSize;
    isEmulationPacket   = msg.isEmulationPacket;
    timerManager        = NULL;   // timers not serialized
    isScheduledOnMainHeap = true; // timers not serialized
    timerExpiresAt      = 0;      // timers not serialized

    d_stamped = msg.d_stamped;
    d_realStampTime = msg.d_realStampTime;
    d_simStampTime = msg.d_simStampTime;
    d_stampId = msg.d_stampId;

    m_flags = 0;

    if (msg.m_band == NULL)
    {
      m_band = NULL;
    }
    else
    {
      m_band = msg.m_band->clone();
    }

    memcpy(smallInfoSpace, msg.smallInfoSpace, SMALL_INFO_SPACE_SIZE);
    memcpy(headerProtocols, msg.headerProtocols, sizeof(int) * MAX_HEADERS);
    memcpy(headerSizes, msg.headerSizes, sizeof(int) * MAX_HEADERS);

    if (payloadSize == 0) {
        payload = NULL;
    }
    else
    {
        payload = MESSAGE_PayloadAlloc(m_partitionData, msg.payloadSize, false);
        assert(payload != NULL);
        memcpy(payload, msg.payload, msg.payloadSize);
    }

    if (msg.packet != NULL)
    {
        ERROR_Assert(payload != NULL, "invalid payload");
        packet = payload + (msg.packet - msg.payload);
    }

    MessageInfoHeader* hdrPtr = NULL;
    MessageInfoHeader* srcHdrPtr = NULL;
    if (infoArray.size() > 0)
    {
        // clean up the vector. Should never happen.
        for (i = 0; i < infoArray.size(); i++)
        {
            MessageInfoHeader* hdr = &(infoArray[i]);
            if (hdr->infoSize > 0)
            {
                MESSAGE_InfoFieldFree(m_partitionData, hdr, mtWasMT == TRUE);
            }
        }
        infoArray.clear();
    }
    if (infoBookKeeping.size() > 0)
    {
        infoBookKeeping.clear();
    }

    infoArray = msg.infoArray;
    for (i = 0; i < msg.infoArray.size(); i++)
    {
        hdrPtr = &(infoArray[i]);
        srcHdrPtr = (MessageInfoHeader*) &(msg.infoArray[i]);
        if (srcHdrPtr->infoSize > 0)
        {
            if (srcHdrPtr->infoType != INFO_TYPE_DEFAULT
                || srcHdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
            {
                hdrPtr->info = MESSAGE_InfoFieldAlloc(
                                   m_partitionData,
                                   srcHdrPtr->infoSize,
                                   false);
                memset(hdrPtr->info, 0, srcHdrPtr->infoSize);
                ERROR_Assert(hdrPtr->info != NULL,
                             "Out of memory");
                memcpy(hdrPtr->info,
                       srcHdrPtr->info,
                       srcHdrPtr->infoSize);
            }
            else
            {
                hdrPtr->info = (char*)&(smallInfoSpace[0]);
            }
        }
    }

    // Copy the book keeping info.
    infoBookKeeping = msg.infoBookKeeping;
}

void Message::setSent(bool val)
{
    if (val)
    {
        m_flags |= SENT;
    }
    else
    {
        m_flags &= ~SENT;
    }
}

void Message::setFreed(bool val)
{
    if (val)
    {
        m_flags |= FREED;
    }
    else
    {
        m_flags &= ~FREED;
    }
}

void Message::setDeleted(bool val)
{
    if (val)
    {
        m_flags |= DELETED;
    }
    else
    {
        m_flags &= ~DELETED;
    }
}

MessageQueue::MessageQueue()
{
    m_head = NULL;
    m_tail = NULL;
    m_size = 0;
    m_earliestEvent = CLOCKTYPE_MAX;
}

MessageQueue::~MessageQueue()
{
}

// Enqueue one or more messages
void MessageQueue::enqueue(Message* msg)
{
    // Find last message we are enqueuing
    Message* last = msg;
    m_earliestEvent = MIN(m_earliestEvent, last->eventTime);
    while (last->next != NULL)
    {
        last = last->next;
        m_earliestEvent = MIN(m_earliestEvent, last->eventTime);
    }

    // Insert
    if (m_head == NULL)
    {
        m_head = msg;
    }
    else
    {
        m_tail->next = msg;
    }
    m_tail = last;
}

Message* MessageQueue::dequeue()
{
    if (m_head == NULL)
    {
        return NULL;
    }

    Message* tmp = m_head;

    m_head = m_head->next;

    // Check for empty
    if (m_head == NULL)
    {
        m_tail = NULL;
        m_earliestEvent = CLOCKTYPE_MAX;
    }
    else if (tmp->eventTime == m_earliestEvent)
    {
        // Check for updating earliest event time
        m_earliestEvent = CLOCKTYPE_MAX;
        for (Message* tmp2 = m_head; tmp2 != NULL; tmp2 = tmp2->next)
        {
            m_earliestEvent = MIN(m_earliestEvent, tmp2->eventTime);
        }
    }

    tmp->next = NULL;
    return tmp;
}

Message* MessageQueue::dequeueAll()
{
    if (m_head == NULL)
    {
        return NULL;
    }

    Message* tmp = m_head;

    m_head = NULL;
    m_tail = NULL;
    m_earliestEvent = CLOCKTYPE_MAX;

    return tmp;
}

ThreadSafeMessageQueue::ThreadSafeMessageQueue()
{
    m_mutex = new pthread_mutex_t;
    pthread_mutex_init(m_mutex, NULL);
}

ThreadSafeMessageQueue::~ThreadSafeMessageQueue()
{
    delete m_mutex;
}

void ThreadSafeMessageQueue::enqueue(Message* msg)
{
    pthread_mutex_lock(m_mutex);
    MessageQueue::enqueue(msg);
    pthread_mutex_unlock(m_mutex);
}

Message* ThreadSafeMessageQueue::dequeue()
{
    pthread_mutex_lock(m_mutex);
    Message* msg = MessageQueue::dequeue();
    pthread_mutex_unlock(m_mutex);

    return msg;
}

Message* ThreadSafeMessageQueue::dequeueAll()
{
    pthread_mutex_lock(m_mutex);
    Message* msg = MessageQueue::dequeueAll();
    pthread_mutex_unlock(m_mutex);

    return msg;
}

void MESSAGE_PrintMessage(Message* msg) {
    unsigned int j;
    int i;
    char timeBuffer[MAX_STRING_LENGTH];

    int numInfoFields = (int)msg->infoArray.size();

    printf("Printing message: layer %d protocol %d instance %d event %d\n",
           msg->layerType,
           msg->protocolType,
           msg->instanceId,
           msg->eventType);
    printf("m_radioId: \t\t%d\n", (int) msg->m_radioId);
    printf("naturalOrder: \t\t%u\n", msg->naturalOrder);
    printf("mtWasMT: \t\t%d\n", (int) msg->mtWasMT);
    printf("error: \t\t\t%d\n", (int) msg->error);
    printf("allowLoose: \t\t%d\n", (int) msg->allowLoose);
    printf("nodeId: \t\t%d\n", (int) msg->nodeId);
    printf("sourcePartitionId: \t%d\n", (int) msg->sourcePartitionId);
    ctoa(msg->eventTime, timeBuffer);
    printf("eventTime: \t\t%s\n", timeBuffer);
    ctoa(msg->eot, timeBuffer);
    printf("eot: \t\t\t%s\n", timeBuffer);
    printf("info fields = %d\n", numInfoFields);
    for (i = 0; i < numInfoFields; i ++)
    {
        printf("infoArray[%d].infoType: \t%d\n",
               i, msg->infoArray[i].infoType);
        printf("infoArray[%d].infoSize: \t%d\n",
               i, msg->infoArray[i].infoSize);
        printf("infoArray[%d].info: \t", i);
        for (j = 0; j < msg->infoArray[i].infoSize; j++)
        {
            printf("%02X ", (0xFF & (unsigned)msg->infoArray[i].info[j]));
        }
        printf("\n");
    }
    printf("smallInfoSpace:\n");
    printf("packet*: \t\t%p\n", msg->packet);
    printf("payload*: \t\t%p\n", msg->payload);
    printf("packetSize: \t\t%d\n", msg->packetSize);
    printf("payloadSize: \t\t%d\n", msg->payloadSize);
    {
        printf("packet: \t\t");
        for (i = 0; i < msg->packetSize; i++)
            printf("%02X ", (0xFF & (unsigned) msg->packet[i]));
        printf("\n");
    }
    printf("virtualPayloadSize: \t%d\n", msg->virtualPayloadSize);
    ctoa(msg->packetCreationTime, timeBuffer);
    printf("packetCreationTime: \t%s\n", timeBuffer);
    printf("cancelled: \t\t%d\n", (int) msg->cancelled);
    printf("originatingNodeId: \t%u\n", msg->originatingNodeId);
    printf("sequenceNumber: \t%d\n", msg->sequenceNumber);
    printf("originatingProtocol: \t%d\n", msg->originatingProtocol);
    printf("numberOfHeaders: \t%d\n", msg->numberOfHeaders);
    for (i = 0; i < msg->numberOfHeaders; i++)
        printf("headerProtocols[%d]: \t%d\n", i, msg->headerProtocols[i]);
    for (i = 0; i < msg->numberOfHeaders; i++)
        printf("headerSizes[%d]: \t%d\n", i, msg->headerSizes[i]);

    printf("relayNodeAddr: \t\t%d\n", (int) msg->relayNodeAddr);
    printf("subChannelIndex: \t%d\n", msg->subChannelIndex);
    printf("isPacked: \t\t%d\n", (int) msg->isPacked);
    printf("actualPktSize: \t\t%d\n", msg->actualPktSize);
    printf("isEmulationPacket: \t%d\n", (int) msg->isEmulationPacket);
    printf("isScheduledOnMainHeap: \t%d\n", (int) msg->isScheduledOnMainHeap);
    ctoa(msg->timerExpiresAt, timeBuffer);
    printf("timerExpiresAt: \t%s\n", timeBuffer);
}

/*
 * FUNCTION     MESSAGE_Alloc
 * PURPOSE      Allocate a new Message structure. This is called when a new
 *              message has to be sent through the system. The last three
 *              parameters indicate the nodeId, layerType, and eventType
 *              that will be set for this message.
 *
 * Parameters:
 *    node:       node which is allocating message
 *    layerType:  layer type to be set for this message
 *    protocol:   Protocol to be set for this message
 *    eventType:  event type to be set for this message
 */
Message* MESSAGE_Alloc(Node *node,
                       int layerType,
                       int protocol,
                       int eventType,
                       bool isMT)
{
    return MESSAGE_Alloc(node->partitionData,
                         layerType,
                         protocol,
                         eventType, isMT);
}

// Allocate a new Message structure. This is called when a
// new message has to be sent through the system. The last
// three parameters indicate the layerType, protocol and the
// eventType that will be set for this message.
//
// \param partition  partition that is allocating message
// \param layerType  Layer type to be set for this message
// \param protocol  Protocol to be set for this message
// \param eventType  event type to be set for this message
//
// \return Pointer to allocated message structure
Message* MESSAGE_Alloc(PartitionData *partition,
                       int layerType,
                       int protocol,
                       int eventType,
                       bool isMT)
{
    Message *newMsg = NULL;

#ifdef MEMDEBUG
    ++allocs;
    if (!(allocs % 100))
    printf("partition %d, allocs = %d, deallocs = %d\n",
           partition->partitionId, allocs, deallocs);
#endif

#ifdef MESSAGE_NO_RECYCLE
    newMsg = new Message();
#else
    // When called from a worker context, we can't use the partition
    // data FreeList (it isn't locked)
    if ((partition->msgFreeList == NULL) || (isMT))
    {
        newMsg = new Message();
        newMsg->infoArray.reserve(10);
    }
    else
    {
        newMsg = partition->msgFreeList;
        partition->msgFreeList =
            partition->msgFreeList->next;
        (partition->msgFreeListNum)--;
    }
#endif
    assert(newMsg != NULL);

    newMsg->initialize(partition);

    // Set layer, protocol, event type
    MESSAGE_SetLayer(newMsg, (short) layerType, (short) protocol);
    MESSAGE_SetEvent(newMsg, (short) eventType);

    // Set multi threaded
    if (isMT)
    {
        newMsg->mtWasMT = TRUE;
    }

    return newMsg;
}


// Allocate space for one "info" field
//
// \param node  node which is allocating the space.
// \param infoSize  size of the space to be allocated
//
// \return pointer to the allocated space.
char* MESSAGE_InfoFieldAlloc(Node *node, int infoSize, bool isMT)
{
    return MESSAGE_InfoFieldAlloc(node->partitionData, infoSize, isMT);
}

// Allocate space for one "info" field
//
// \param partition  partition which is allocating the space.
// \param infoSize  size of the space to be allocated
//
// \return pointer to the allocated space.
char* MESSAGE_InfoFieldAlloc(PartitionData *partition, int infoSize, bool isMT)
{
    char * newInfo;

#ifdef MESSAGE_NO_RECYCLE
    const bool recycle = false;
#else
    const bool recycle = true;
#endif

    bool useRecycled = (recycle && !isMT && partition != NULL);

    if (infoSize > SMALL_INFO_SPACE_SIZE || !useRecycled)
    {
        newInfo = (char *) MEM_malloc(infoSize);
        memset(newInfo, 0, infoSize);
        return newInfo;
    }
    else
    {
        if ((partition->msgInfoFreeList == NULL) || ((isMT)) )
        {
            MessageInfoListCell* NewCell = (MessageInfoListCell*)
                MEM_malloc(sizeof(MessageInfoListCell));
            memset((char*)&(NewCell->infoMemory[0]), 0, SMALL_INFO_SPACE_SIZE);
            return (char*) &(NewCell->infoMemory[0]);
        }
        else
        {
            newInfo = (char*)
                &(partition->msgInfoFreeList->infoMemory[0]);
            partition->msgInfoFreeList =
                partition->msgInfoFreeList->next;
            (partition->msgInfoFreeListNum) --;
            memset(newInfo, 0, SMALL_INFO_SPACE_SIZE);
            return newInfo;
        }
    }
}

// Free space for one "info" field
//
// \param node  node which is allocating the space.
// \param hdrPtr  pointer to the "info" field
//
void MESSAGE_InfoFieldFree(Node *node, MessageInfoHeader* hdrPtr, bool wasMT)
{
    MESSAGE_InfoFieldFree(node->partitionData, hdrPtr, wasMT);
}

// API       :: MESSAGE_InfoFieldFree
// LAYER     :: ANY LAYER
// PURPOSE   :: Free space for one "info" field
// PARAMETERS ::
// + partition:  PartitionData* : partition which is allocating the space.
// + hdrPtr  :  MessageInfoHeader* : pointer to the "info" field
// RETURN    :: void : NULL
void MESSAGE_InfoFieldFree(
    PartitionData *partition,
    MessageInfoHeader* hdrPtr,
    bool wasMT)
{
#ifdef MESSAGE_NO_RECYCLE
    if (hdrPtr->infoType != INFO_TYPE_DEFAULT||
        hdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
    {
        MEM_free(hdrPtr->info);
    }
#else
    if ((partition != NULL &&
        hdrPtr->infoType != INFO_TYPE_DEFAULT &&
        hdrPtr->infoSize <= SMALL_INFO_SPACE_SIZE &&
        (wasMT == false) &&
        partition->msgInfoFreeListNum < MSG_INFO_LIST_MAX))
    {
        MessageInfoListCell* cellPtr =
            (MessageInfoListCell*)hdrPtr->info;
        cellPtr->next = partition->msgInfoFreeList;
        partition->msgInfoFreeList = cellPtr;
        (partition->msgInfoFreeListNum)++;
    }
    else if (hdrPtr->infoType != INFO_TYPE_DEFAULT ||
             hdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
    {
        MEM_free(hdrPtr->info);
    }
    else
    {
        assert(hdrPtr->infoType == INFO_TYPE_DEFAULT && hdrPtr->infoSize <= SMALL_INFO_SPACE_SIZE);
    }
#endif
}

// Allocate one "info" field with given info type for the
// message. This function is used for the delivery of data
// for messages which are NOT packets as well as the delivery
// of extra information for messages which are packets. If a
// "info" field with the same info type has previously been
// allocated for the message, it will be replaced by a new
// "info" field with the specified size. Once this function
// has been called, MESSAGE_ReturnInfo function can be used
// to get a pointer to the allocated space for the info field
// in the message structure.
//
// \param node  node which is allocating the info field.
// \param msg  message for which "info" field
//    has to be allocated
// \param infoSize  size of the "info" field to be allocated
// \param infoType  type of the "info" field to be allocated.
//
// \return Pointer to the added info field.
char* MESSAGE_AddInfo(Node *node,
                      Message *msg,
                      int infoSize,
                      unsigned short infoType)
{
    return MESSAGE_AddInfo(node->partitionData,
                           msg,
                           infoSize,
                           infoType);
}


// Allocate one "info" field with given info type for the
// message. This function is used for the delivery of data
// for messages which are NOT packets as well as the delivery
// of extra information for messages which are packets. If a
// "info" field with the same info type has previously been
// allocated for the message, it will be replaced by a new
// "info" field with the specified size. Once this function
// has been called, MESSAGE_ReturnInfo function can be used
// to get a pointer to the allocated space for the info field
// in the message structure.
//
// \param partition  partition which is allocating the info field.
// \param msg  message for which "info" field
//    has to be allocated
// \param infoSize  size of the "info" field to be allocated
// \param infoType  type of the "info" field to be allocated.
//
// \return Pointer to the added info field.
char* MESSAGE_AddInfo(PartitionData *partition,
                      Message *msg,
                      int infoSize,
                      unsigned short infoType)
{
    MessageInfoHeader infoHdr;
    MessageInfoHeader* hdrPtrInfo = NULL;
    bool insertInfo = false;
    unsigned int i;

    ERROR_Assert(infoSize != 0, "Cannot add empty info");

    if (infoType == INFO_TYPE_DEFAULT)
    {
        // First check if there is already a default type
        if (msg->infoArray.size() > 0)
        {
            for (i = 0; i < msg->infoArray.size(); i++)
            {
                MessageInfoHeader* info;
                info = &(msg->infoArray[i]);
                if (info->infoType == infoType)
                {
                    hdrPtrInfo = &(msg->infoArray[i]);
                    break;
                }
            }
        }

        if (hdrPtrInfo == NULL)
        {
            // There is no default info so we will insert it
            insertInfo = true;

            // Use small info space if info is small enough.
            if (infoSize <= SMALL_INFO_SPACE_SIZE)
            {
                infoHdr.info = (char*) msg->smallInfoSpace;
            }
            else
            {
                // TODO: Check that this is mem_freed
                infoHdr.info = (char*) MEM_malloc(infoSize);
                memset(infoHdr.info, 0, infoSize);
            }

            infoHdr.infoSize = infoSize;
            infoHdr.infoType = INFO_TYPE_DEFAULT;
        }
        else
        {
            // There is already a default info in the list.  Reuse it.
            if (infoSize <= SMALL_INFO_SPACE_SIZE)
            {
                // Free old memory if it was not using small info space
                if (hdrPtrInfo->infoSize > SMALL_INFO_SPACE_SIZE)
                {
                    MEM_free(hdrPtrInfo->info);
                }

                hdrPtrInfo->info = (char*) msg->smallInfoSpace;
                memset(hdrPtrInfo->info, 0, SMALL_INFO_SPACE_SIZE);
            }
            else
            {
                // Free old memory if new info is bigger, otherwise reuse it.
                if ((unsigned int)infoSize > hdrPtrInfo->infoSize
                    && hdrPtrInfo->infoSize > SMALL_INFO_SPACE_SIZE)
                {
                    // Old one was not small info, free then allocate
                    MEM_free(hdrPtrInfo->info);
                    hdrPtrInfo->info = (char*) MEM_malloc(infoSize);
                }
                else if ((unsigned int)infoSize > hdrPtrInfo->infoSize)
                {
                    // Old one was small info, allocate new memory
                    hdrPtrInfo->info = (char*) MEM_malloc(infoSize);
                }

                memset(hdrPtrInfo->info, 0, infoSize);
            }

            hdrPtrInfo->infoSize = infoSize;
        }
    }
    else
    {
        // Info type is not default

        // Loop over all infos.  Use INFO_TYPE_UNDEFINED unless we find
        // an exact infoType match.
        for (i = 0; i < msg->infoArray.size(); i++)
        {
            if (msg->infoArray[i].infoType == infoType)
            {
                hdrPtrInfo = &(msg->infoArray[i]);
                memset(hdrPtrInfo->info, 0, hdrPtrInfo->infoSize);
                break;
            }
            else if (hdrPtrInfo == NULL &&
                     msg->infoArray[i].infoType == INFO_TYPE_UNDEFINED)
            {
                hdrPtrInfo = &(msg->infoArray[i]);
                memset(hdrPtrInfo->info, 0, hdrPtrInfo->infoSize);
            }
        }

        if (hdrPtrInfo == NULL)
        {
            // No info was found.  We will insert infoHdr.
            insertInfo = true;

            infoHdr.info = MESSAGE_InfoFieldAlloc(partition,
                                                  infoSize,
                                                  msg->mtWasMT);
            memset(infoHdr.info, 0, infoSize);
            infoHdr.infoSize = infoSize;
            infoHdr.infoType = infoType;
        }
        else
        {
            // Info was found.  Enlarge if either new or old infos are bigger than
            // small info space.  Then set the type.
            if (hdrPtrInfo->infoSize < (unsigned int)infoSize &&
                (hdrPtrInfo->infoSize > SMALL_INFO_SPACE_SIZE ||
                infoSize > SMALL_INFO_SPACE_SIZE))
            {
                if (hdrPtrInfo->infoSize > 0)
                {
                    MESSAGE_InfoFieldFree(partition,
                                          hdrPtrInfo,
                                          msg->mtWasMT);
                }

                hdrPtrInfo->info = MESSAGE_InfoFieldAlloc(partition,
                                                          infoSize,
                                                          msg->mtWasMT);
                hdrPtrInfo->infoSize = infoSize;
                memset(hdrPtrInfo->info, 0, infoSize);
            }

            hdrPtrInfo->infoType = infoType;
        }
    }

    // If we are adding new info push it onto the array
    if (insertInfo)
    {
        msg->infoArray.push_back(infoHdr);
        return infoHdr.info;
    }

    // We reused old info
    return hdrPtrInfo->info;
}

// Remove one "info" field with given info type from the
// info array of the message.
//
// \param node  node which is removing info field.
// \param msg  message for which "info" field
//    has to be removed
// \param infoType  type of the "info" field to be removed.
//
void MESSAGE_RemoveInfo(Node *node, Message *msg, unsigned short infoType)
{
    unsigned int i;
    MessageInfoHeader* hdrPtrInfo = NULL;

    // Remove the info field from the vector info Array.
    for (i = 0; i < msg->infoArray.size(); i ++)
    {
        hdrPtrInfo = &(msg->infoArray[i]);
        if (infoType == hdrPtrInfo->infoType)
        {
            break;
        }
    }

    if (i < msg->infoArray.size())
    {
        if (hdrPtrInfo->infoSize > 0)
        {
            MESSAGE_InfoFieldFree(node, hdrPtrInfo);
        }

        hdrPtrInfo->infoType = INFO_TYPE_UNDEFINED;
        hdrPtrInfo->infoSize = 0;
        hdrPtrInfo->info = NULL;

        // Remove the entry from the vector.
        msg->infoArray.erase(msg->infoArray.begin()+i);
    }
}


// Remove one "info" field with given info type from the
// info array of the message.
//
// \param node  node which is removing info field.
// \param msg  message for which "info" field
//    has to be removed
// \param infoType  type of the "info" field to be removed.
// \param fragmentNumber  Location of the fragment in the TCP packet.
//
void MESSAGE_RemoveInfo(Node *node,
                        Message *msg,
                        unsigned short infoType,
                        int fragmentNumber)
{
    MessageInfoHeader* hdrPtr = NULL;
    int i;
    int infoLowerLimit  =
        msg->infoBookKeeping.at(fragmentNumber).infoLowerLimit;
    int infoUpperLimit =
        msg->infoBookKeeping.at(fragmentNumber).infoUpperLimit;

    for (i = infoLowerLimit; i < infoUpperLimit; i ++)
    {
        hdrPtr = &(msg->infoArray[i]);
        if (infoType == hdrPtr->infoType)
        {
            break;
        }
    }

    if (i < infoUpperLimit)
    {
        if (hdrPtr->infoSize > 0)
        {
            MESSAGE_InfoFieldFree(node->partitionData, hdrPtr,
                                  (BOOL)msg->mtWasMT);
        }

        hdrPtr->infoType = INFO_TYPE_UNDEFINED;
        hdrPtr->infoSize = 0;
        hdrPtr->info = NULL;

        // Remove the entry from the vector.
        msg->infoArray.erase(msg->infoArray.begin()+i);
    }
}


// Copy the "info" fields of the source message to
// the destination message.
//
// \param node  Node which is copying the info fields
// \param dsgMsg  Destination message
// \param srcMsg  Source message
//
void MESSAGE_CopyInfo(Node *node, Message *dstMsg, Message *srcMsg)
{
    unsigned int i;

    MessageInfoHeader* hdrPtr = NULL;
    MessageInfoHeader infoHdr;
    memset(&infoHdr, 0, sizeof(MessageInfoHeader));
    bool insertInfo = false;

    for (i = 0; i < srcMsg->infoArray.size(); i++)
    {
        MessageInfoHeader* srcHdrPtr = &(srcMsg->infoArray[i]);
        if (i < dstMsg->infoArray.size())
        {
            hdrPtr = &(dstMsg->infoArray[i]);
            insertInfo = FALSE;
        }
        else
        {
            hdrPtr = NULL;
            infoHdr.infoSize = 0;
            infoHdr.infoType = INFO_TYPE_UNDEFINED;
            infoHdr.info = NULL;
            insertInfo = true;
        }

        // Free the old memory
        if (hdrPtr != NULL && hdrPtr->infoSize > 0)
        {
            MESSAGE_InfoFieldFree(node, hdrPtr);
        }
        if (srcHdrPtr->infoType != INFO_TYPE_DEFAULT ||
            srcHdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
        {
            if (!insertInfo)
            {
                hdrPtr->info = MESSAGE_InfoFieldAlloc(
                    node,
                    srcHdrPtr->infoSize,
                    dstMsg->mtWasMT);
                ERROR_Assert(hdrPtr->info != NULL,
                    "Out of memory");

            }
            else
            {
                infoHdr.info = MESSAGE_InfoFieldAlloc(
                    node,
                    srcHdrPtr->infoSize,
                    dstMsg->mtWasMT);
                ERROR_Assert(infoHdr.info != NULL,
                    "Out of memory");
            }
        }
        else
        {
            if (!insertInfo)
            {
                hdrPtr->info = (char*)&(dstMsg->smallInfoSpace[0]);
            }
            else
            {
                infoHdr.info = (char*)&(dstMsg->smallInfoSpace[0]);
            }
        }

        if (!insertInfo)
        {
            memcpy(hdrPtr->info,
                srcHdrPtr->info,
                srcHdrPtr->infoSize);
            hdrPtr->infoType = srcHdrPtr->infoType;
            hdrPtr->infoSize = srcHdrPtr->infoSize;
        }
        else
        {
            memcpy(infoHdr.info,
                srcHdrPtr->info,
                srcHdrPtr->infoSize);
            infoHdr.infoType = srcHdrPtr->infoType;
            infoHdr.infoSize = srcHdrPtr->infoSize;
        }
        if (insertInfo)
        {
            dstMsg->infoArray.push_back(infoHdr);
        }
        hdrPtr = NULL;
        insertInfo = false;
    }

    // Now to handle the book Keeping vector.
    if (dstMsg->infoBookKeeping.capacity() < srcMsg->infoBookKeeping.size())
    {
        dstMsg->infoBookKeeping.reserve(srcMsg->infoBookKeeping.size() + 1);
    }
    dstMsg->infoBookKeeping = srcMsg->infoBookKeeping;
}

// Appends the "info" fields of the source message to
// the destination message.
//
// \param partitionData  Partition which is copying the info fields
// \param msg  Destination message
// \param infosize  size of the info field
// \param infoType  type of info field.
//
char* MESSAGE_AppendInfo(PartitionData *partitionData,
                         Message *msg,
                         int infoSize,
                         unsigned short infoType)
{
    MessageInfoHeader infoHdr;
    if (infoType == INFO_TYPE_DEFAULT)
    {
        // Check if we already have a default info in the destination message
        if (msg->infoArray.size() > 0)
        {
            char* tempBuf = MESSAGE_ReturnInfo(msg);
            if (tempBuf != NULL)
            {
                return tempBuf;
            }
        }
    }
    infoHdr.infoSize = infoSize;
    infoHdr.infoType = infoType;
    infoHdr.info = NULL;

    if (infoType != INFO_TYPE_DEFAULT ||
        infoSize > SMALL_INFO_SPACE_SIZE)
    {
        infoHdr.info = MESSAGE_InfoFieldAlloc(partitionData,
                                              infoHdr.infoSize,
                                              msg->mtWasMT);
        ERROR_Assert(infoHdr.info != NULL, "Out of memory!");
    }
    else
    {
        infoHdr.info = (char*)&(msg->smallInfoSpace[0]);
    }

    msg->infoArray.push_back(infoHdr);
    return infoHdr.info;
}
// Appends the "info" fields of the source message to
// the destination message.
//
// \param node  Node which is copying the info fields
// \param msg  Destination message
// \param infosize  size of the info field
// \param infoType  type of info field.
//
char* MESSAGE_AppendInfo(Node *node,
                        Message *msg,
                        int infoSize,
                        unsigned short infoType)
{
    return MESSAGE_AppendInfo(node->partitionData, msg, infoSize, infoType);
}

// Appends the "info" fields of the source message to
// the destination message.
//
// \param node  Pointer to partition data
// \param dsgMsg  Destination message
// \param srcMsg  Source message
//
void MESSAGE_AppendInfo(Node *node, Message *dstMsg, Message *srcMsg)
{
    unsigned int i;
    MessageInfoHeader infoHdr;
    MessageInfoHeader* hdrPtr = NULL;
    bool isDefaultInfo = false;
    // copy the header fields

    // Check if we already have a default info in the destination message
    // If so do not copy the source default info field over
    if (dstMsg->infoArray.size() > 0)
    {
        char* tempBuf = MESSAGE_ReturnInfo(dstMsg);
        if (tempBuf != NULL)
        {
            isDefaultInfo = true;
        }
    }
    // set the "info" pointer of each info field
    for (i = 0; i < srcMsg->infoArray.size(); i ++)
    {
        hdrPtr = &(srcMsg->infoArray[i]);
        if (hdrPtr->infoType == INFO_TYPE_DEFAULT && isDefaultInfo)
        {
            continue;
        }
        if (hdrPtr->infoSize > 0)
        {
            // Allocate space for infoHeader
            if (hdrPtr->infoType != INFO_TYPE_DEFAULT ||
                hdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
            {
                infoHdr.info = MESSAGE_InfoFieldAlloc(node,
                                                      hdrPtr->infoSize,
                                                      dstMsg->mtWasMT);
                ERROR_Assert(infoHdr.info != NULL, "Out of memory!");
            }
            else
            {
                infoHdr.info = (char*)&(dstMsg->smallInfoSpace[0]);
            }

            memcpy(infoHdr.info,
                   hdrPtr->info,
                   hdrPtr->infoSize);
            infoHdr.infoSize = hdrPtr->infoSize;
            infoHdr.infoType = hdrPtr->infoType;

            dstMsg->infoArray.push_back(infoHdr);
        }
    }
}

// Appends the "info" fields of the source message to
// the destination message.
//
// \param node  Node which is copying the info fields
// \param dsgMsg  Destination message
// \param srcMsg  Source message
//
void MESSAGE_AppendInfo(
    Node *node,
    Message *dstMsg,
    std::vector<MessageInfoHeader> srcInfo)
{
    unsigned int i;
    MessageInfoHeader* hdrPtr = NULL;
    MessageInfoHeader infoHdr;
    bool isDefaultInfo = false;
    // copy the header fields

    // Check if we already have a default info in the destination message
    // If so do not append default info
    if (dstMsg->infoArray.size() > 0)
    {
        char* tempBuf = MESSAGE_ReturnInfo(dstMsg);
        if (tempBuf != NULL)
        {
            isDefaultInfo = true;
        }
    }
    // set the "info" pointer of each info field
    for (i = 0; i < srcInfo.size(); i ++)
    {
        hdrPtr = &(srcInfo.at(i));
        if (hdrPtr->infoType == INFO_TYPE_DEFAULT && isDefaultInfo)
        {
            continue;
        }
        if (hdrPtr->infoSize > 0)
        {
            // Allocate space for infoHeader
            if (hdrPtr->infoType != INFO_TYPE_DEFAULT ||
                hdrPtr->infoSize > SMALL_INFO_SPACE_SIZE)
            {
                infoHdr.info = MESSAGE_InfoFieldAlloc(node,
                                                      hdrPtr->infoSize,
                                                      dstMsg->mtWasMT);
                ERROR_Assert(infoHdr.info != NULL, "Out of memory!");
            }
            else
            {
                infoHdr.info = (char*)&(dstMsg->smallInfoSpace[0]);
            }

            memcpy(infoHdr.info,
                   hdrPtr->info,
                   hdrPtr->infoSize);
            infoHdr.infoSize = hdrPtr->infoSize;
            infoHdr.infoType = hdrPtr->infoType;
            dstMsg->infoArray.push_back(infoHdr);
        }
    }
}


/*
 * FUNCTION     MESSAGE_PacketAlloc
 * PURPOSE      Allocate the "payload" field for the packet to be delivered.
 *              Add additional free space in front of the packet for header
 *              that might be added to the packet.
 *              This function can be called from the application layer or
 *              anywhere else (e.g TCP, IP) that a packet may originiate from.
 *              The "packetSize" variable will be set to the "packetSize"
 *              parameter specified in the function call.
 *              Once this function has been called the "packet" variable in
 *              the message structure can be used to access this space.
 *
 * Parameters:
 *    node:         node which is allocating message
 *    msg:          message for which data has to be allocated
 *    payloadSize:  size of the payload to be allocated
 *    originatingProtocol:  Protocol allocating this packet.
 */

void MESSAGE_PacketAlloc(Node *node,
                         Message *msg,
                         int packetSize,
                         TraceProtocolType originatingProtocol)
{
    MESSAGE_PacketAlloc(node->partitionData,
                        msg,
                        packetSize,
                        originatingProtocol, false);

    msg->originatingNodeId = node->nodeId;
    msg->sequenceNumber = node->packetTraceSeqno++;

#ifdef ADDON_DB
    StatsDBAddMessageMsgIdIfNone(node, msg) ;
#endif
}

// Allocate the "payload" field for the packet to be delivered.
// Add additional free space in front of the packet for
// headers that might be added to the packet. This function
// can be called from the application layer or anywhere else
// (e.g TCP, IP) that a packet may originiate from. The
// "packetSize" variable will be set to the "packetSize"
// parameter specified in the function call. Once this function
// has been called the "packet" variable in the message
// structure can be used to access this space.
//
// \param partition  artition which is allocating the packet
// \param msg  message for which packet has to be allocated
// \param packetSize  size of the packet to be allocated
// \param originatingProtocol  Protocol allocating this packet
// \param isMT  Is this packet being created from a worker thread
//
void MESSAGE_PacketAlloc(PartitionData *partition,
                         Message *msg,
                         int packetSize,
                         TraceProtocolType originatingProtocol,
                         bool isMT)
{
    assert(msg->payload == NULL);
    assert(msg->payloadSize == 0);
    assert(msg->packetSize == 0);
    assert(msg->packet == NULL);

    msg->payload = MESSAGE_PayloadAlloc(
                       partition,
                       packetSize + MSG_MAX_HDR_SIZE, isMT);

    assert(msg->payload != NULL);

    msg->payloadSize = packetSize + MSG_MAX_HDR_SIZE;
    msg->packet = msg->payload + MSG_MAX_HDR_SIZE;
    msg->packetSize = packetSize;
    msg->packetCreationTime = partition->getGlobalTime();

    msg->originatingProtocol = originatingProtocol;
    msg->numberOfHeaders = 0;

    //Add a patch to allow MESSAGE_PacketAlloc to be called solely for
    //allocating real payload space purpose. In this case, packetSize
    //is considered as payload size, not a packet header size.
    //Use case
    //Tcp: submit_data(): to create a new packet to submit data to
    //application.
    if (originatingProtocol != TRACE_UNDEFINED)
    {
        msg->headerProtocols[msg->numberOfHeaders] = originatingProtocol;
        msg->headerSizes[msg->numberOfHeaders] = packetSize;
        msg->numberOfHeaders++;
    }
}

/*
 * FUNCTION     MESSAGE_AddHeader
 * PURPOSE      This function is called to reserve additional space for a
 *              header of size "hdrSize" for the packet enclosed in the
 *              message. The "packetSize" variable in the message structure
 *              will be increased by "hdrSize".
 *              After this function is called the "packet" variable in the
 *              message structure will point the space occupied by this new
 *              header.
 *
 * Parameters:
 *    node:          node which is adding header
 *    msg:           message for which header has to be added
 *    hdrSize:       size of the header to be added
 *    traceProtocol: protocol that is adding this header.
 */

void MESSAGE_AddHeader(Node *node,
                       Message *msg,
                       int hdrSize,
                       TraceProtocolType traceProtocol)
{
    msg->packet -= hdrSize;
    msg->packetSize += hdrSize;

    if (msg->isPacked)
    {
        msg->actualPktSize += hdrSize;
    }
    msg->headerProtocols[msg->numberOfHeaders] = traceProtocol;
    msg->headerSizes[msg->numberOfHeaders] = hdrSize;
    msg->numberOfHeaders++;

    if (msg->packet < msg->payload) {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Not enough space for headers.\n"
               "Increase the value of MSG_MAX_HDR_SIZE and try again.\n");
        ERROR_ReportError(errorStr);
    }
}


/*
 * FUNCTION     MESSAGE_RemoveHeader
 * PURPOSE      This function is called to remove a header from the packet
 *              enclosed in the message. The "packetSize" variable in the
 *              message will be decreased by "hdrSize".
 *
 * Parameters:
 *    node:         node which is removing the header
 *    msg:          message for which header is being removed
 *    hdrSize:      size of the header being removed
 *    traceProtocol: protocol that is adding this header.
 */
void MESSAGE_RemoveHeader(Node *node,
                          Message *msg,
                          int hdrSize,
                          TraceProtocolType traceProtocol)
{
	if (msg->headerProtocols[msg->numberOfHeaders-1] != traceProtocol) {
		printf("TRACE: headerProtocol=%d traceProtocol=%d\n",
				msg->headerProtocols[msg->numberOfHeaders-1],
				traceProtocol);
	}
    ERROR_Assert(msg->headerProtocols[msg->numberOfHeaders-1] == traceProtocol,
                 "TRACE: Removing trace header that doesn't match!\n");

    msg->packet += hdrSize;
    msg->packetSize -= hdrSize;

    if (msg->isPacked)
    {
        msg->actualPktSize -= hdrSize;
    }

    msg->numberOfHeaders--;

    if (msg->packet > (msg->payload + msg->payloadSize)) {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Packet pointer going beyond allocated memory.\n");
        ERROR_ReportError(errorStr);
    }
}

// This is kind of a hack so that MAC protocols (dot11) that
// need to peak at a packet that still has the PHY header can
// return the contents after the first (N) headers without
// first removing those headers.
//
// \param msg  message containing a packet with headers
// \param header  number of the header to return.
//
// \return the packet starting at the header'th header
char* MESSAGE_ReturnHeader(const Message* msg,
                           int            header)
{
    int h;
    int sizeOfHeadersToSkip = 0;
    assert(header <= msg->numberOfHeaders);

    for (h = 0; h < header; h++) {
        sizeOfHeadersToSkip += msg->headerSizes[h];
    }
    return (char*)(msg->packet + sizeOfHeadersToSkip);
}

/*
 * FUNCTION     MESSAGE_ExpandPacket
 * PURPOSE      Expand packet by a specified size
 *
 * Parameters:
 *    node:          node which is expanding the packet
 *    msg:           message which is to be expanded
 *    hdrSize:       size to expand
 */

void MESSAGE_ExpandPacket(Node *node,
                          Message *msg,
                          int size)
{
    msg->packet -= size;
    msg->packetSize += size;

    if (msg->packet < msg->payload) {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Not enough space for headers.\n"
               "Increase the value of MSG_MAX_HDR_SIZE and try again.\n");
        ERROR_ReportError(errorStr);
    }
}


/*
 * FUNCTION     MESSAGE_ShrinkPacket
 * PURPOSE      This function is called to shrink packet by a specified size.
 *
 * Parameters:
 *    node:         node which is shrinking packet
 *    msg:          message whose packet is be shrinked
 *    hdrSize:      size to shrink
 */
void MESSAGE_ShrinkPacket(Node *node,
                          Message *msg,
                          int size)
{
    msg->packet += size;
    msg->packetSize -= size;

    if (msg->packet > (msg->payload + msg->payloadSize)) {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr, "Packet pointer going beyond allocated memory.\n");
        ERROR_ReportError(errorStr);
    }
 }

/*
 * FUNCTION     MESSAGE_Free
 * PURPOSE      When the message is no longer needed it can be freed. Firstly,
 *              the data portion of the message is freed. Than the message
 *              itself is freed. It is important to remember to free the
 *              message. Otherwise there will nasty memory leaks in the
 *              program.
 *
 * Parameters:
 *    node:       node which is freeing the message
 *    msg:        message which has to be freed
 */
void MESSAGE_Free(Node *node, Message *msg)
{
    MESSAGE_Free(node->partitionData, msg);
}

/*
 * FUNCTION     MESSAGE_Free
 * PURPOSE      When the message is no longer needed it can be freed. Firstly,
 *              the data portion of the message is freed. Than the message
 *              itself is freed. It is important to remember to free the
 *              message. Otherwise there will nasty memory leaks in the
 *              program.
 *
 * Parameters:
 *    partition:  partition which is freeing the message
 *    msg:        message which has to be freed
 */
void MESSAGE_Free(PartitionData *partition, Message *msg)
{
#ifdef MESSAGE_NO_RECYCLE
    // Make sure we always go through the delete operator when not
    // recycling messages (delete does not recycle messages)
    if (!msg->getDeleted())
    {
        delete msg;
        return;
    }
#endif

#ifdef MEMDEBUG
    deallocs++;
    //printf("partition %d, allocs = %d, deallocs = %d\n", partition->partitionId, allocs, deallocs);
#endif

    bool wasMT = msg->mtWasMT;

    ERROR_Assert(!msg->getFreed(), "Message already freed");
    ERROR_Assert(!msg->getDeleted(), "Message already deleted");
    ERROR_Assert(!msg->getSent(), "Freeing a sent message");

#ifndef MESSAGE_NO_RECYCLE
    // Message recycling is enabled
    if ((partition != NULL) &&
        (wasMT == false) &&
        (partition->msgFreeListNum < MSG_LIST_MAX))
    {
        // Free contents and save to list
        MESSAGE_FreeContents(partition, msg);
        msg->setFreed(true);

        msg->next = partition->msgFreeList;
        partition->msgFreeList = msg;
        (partition->msgFreeListNum)++;
    }
    else
    {
        // Delete message completely
        delete msg;
    }
#endif
}

/*
 * FUNCTION    MESSAGE_FreeList
 * PURPOSE     Free a list of messages until the next pointer of the message is NULL
 *
 * Parameters:
 *    node:      node which is freeing the message
 *    msg:       pointer to the first message of the list
 */

void MESSAGE_FreeList(Node *node, Message *msg)
{
    Message* nextMsg;
    bool        freeMsg = true;
    while (msg != NULL)
    {
        nextMsg = msg->next;
        if (freeMsg)
        {
            MESSAGE_Free(node,msg);
        }
        msg = nextMsg;
    }
}


/*
 * FUNCTION     MESSAGE_Duplicate
 * PURPOSE      Create a new message which is an exact duplicate of the message
 *              supplied as the parameter to the function and return the new
 *              message.
 *
 * Parameters:
 *    node:       node which is caling message copy
 *    msg:        message for which duplicate has to be made
 */
Message* MESSAGE_Duplicate(Node *node, const Message *msg, bool isMT) {
    return MESSAGE_Duplicate(node->partitionData, msg, isMT);
}

/* FUNCTION     MESSAGE_Duplicate
 * PURPOSE      Create a new message which is an exact duplicate of the message
 *              supplied as the parameter to the function and return the new
 *              message.
 *
 * Parameters:
 *    partition:  partition which is caling message copy
 *    msg:        message for which duplicate has to be made
 *    isMT:       Is this packet being created from a worker thread
 */
Message* MESSAGE_Duplicate(
    PartitionData *partition,
    const Message *msg,
    bool isMT)
{
    Message* newMsg = MESSAGE_Alloc(partition,
                                    msg->layerType,
                                    msg->protocolType,
                                    msg->eventType,
                                    isMT);

    assert(newMsg != NULL);
    *newMsg = *msg;

    return newMsg;
}


/*
 * FUNCTION     MESSAGE_PayloadAlloc
 * PURPOSE      Allocate a character payload out of the free list, if possible
 *              otherwise via malloc.
 *
 * Parameters:
 *    node:         node which is allocating payload
 *    payloadSize:  size of the "info" field to be allocated
+     isMT    : bool   : Is this packet being created from a worker thread
 */
char* MESSAGE_PayloadAlloc(Node *node, int payloadSize, bool isMT) {
    return MESSAGE_PayloadAlloc(node->partitionData, payloadSize, isMT);
}

/*
 * FUNCTION     MESSAGE_PayloadAlloc
 * PURPOSE      Allocate a character payload out of the free list, if possible
 *              otherwise via malloc.
 *
 * Parameters:
 *    partition  :  partition which is allocating payload
 *    payloadSize:  size of the "info" field to be allocated
 */
char* MESSAGE_PayloadAlloc(PartitionData *partition,
                           int payloadSize,
                           bool isMT)
{
    if ((payloadSize/* + MSG_MAX_HDR_SIZE*/) > MAX_CACHED_PAYLOAD_SIZE)
    {
        return (char *) MEM_malloc(payloadSize/* + MSG_MAX_HDR_SIZE*/);
    }
    else
    {
#ifdef MESSAGE_NO_RECYCLE
        char *ptr = (char *) MEM_malloc(payloadSize/* + MSG_MAX_HDR_SIZE*/);
        memset(ptr, 0, payloadSize);
        return ptr;
#else
        if ((partition->msgPayloadFreeList == NULL) || (isMT))
        {
            MessagePayloadListCell* NewCell = (MessagePayloadListCell*)
                MEM_malloc(sizeof(MessagePayloadListCell));
            return (char *) &(NewCell->payloadMemory[0]);
        }
        else
        {
            char *payload = (char *)
                &(partition->msgPayloadFreeList->payloadMemory[0]);
            partition->msgPayloadFreeList =
                partition->msgPayloadFreeList->next;
            (partition->msgPayloadFreeListNum)--;

            return payload;
        }
#endif
    }
}

/*
 * FUNCTION     MESSAGE_PayloadFree
 * PURPOSE      Return a character payload to the free list, if possible
 *              otherwise free it.
 *
 * Parameters:
 *    node:         node which is allocating payload
 *    payloadSize:  size of the "info" field to be allocated
 */
void MESSAGE_PayloadFree(Node *node,
                         char *payload,
                         int payloadSize,
                         bool wasMT)
{
    MESSAGE_PayloadFree(node->partitionData, payload, payloadSize, wasMT);
}

/*
 * FUNCTION     MESSAGE_PayloadFree
 * PURPOSE      Return a character payload to the free list, if possible
 *              otherwise free it.
 *
 * Parameters:
 *    partition:    partition which is allocating payload
 *    payloadSize:  size of the "info" field to be allocated
 */
void MESSAGE_PayloadFree(PartitionData *partition,
                         char *payload,
                         int payloadSize,
                         bool wasMT)
{
#ifdef MESSAGE_NO_RECYCLE
    MEM_free(payload);
#else
    if ((partition != NULL) &&
        (payloadSize <= MAX_CACHED_PAYLOAD_SIZE) &&
        (wasMT == false) &&
        (partition->msgPayloadFreeListNum < MSG_PAYLOAD_LIST_MAX))
    {
        MessagePayloadListCell* cellPtr =
            (MessagePayloadListCell*)payload;
        cellPtr->next = partition->msgPayloadFreeList;
        partition->msgPayloadFreeList = cellPtr;
        (partition->msgPayloadFreeListNum)++;
    }
    else
    {
        MEM_free(payload);
    }
#endif
}

// Fragment one packet into multiple fragments
// Note: The original packet will be freed in this function.
// The array for storing pointers to fragments will be
// dynamically allocated. The caller of this function
// will need to free the memory.
//
// \param node  node which is fragmenting the packet
// \param msg  The packet to be fragmented
// \param fragUnit  The unit size for fragmenting the packet
// \param fragList  A list of fragments created.
// \param numFrags  Number of fragments in the fragment list.
// \param protocolType  Protocol type for packet tracing.
//
void MESSAGE_FragmentPacket(
         Node* node,
         Message* msg,
         int fragUnit,
         Message*** fragList,
         int* numFrags,
         TraceProtocolType protocolType)
{
    int pktSize = MESSAGE_ReturnPacketSize(msg);

    if (pktSize <= fragUnit)
    {
        // no need to fragment
        *numFrags = 1;
        *fragList = (Message**) MEM_malloc(sizeof(Message*));
        ERROR_Assert((*fragList) != NULL, "Out of memory!");
        (*fragList)[0] = msg;

        return;
    }
    else
    {
        int realPayloadSize = MESSAGE_ReturnActualPacketSize(msg);
        int virtualPayloadSize = MESSAGE_ReturnVirtualPacketSize(msg);
        Message* fragMsg;
        int realIndex = 0;
        int virtualIndex = 0;
        int fragRealSize;
        int fragVirtualSize;
        int i;

        // calculate number of fragments to be created
        if (pktSize % fragUnit == 0)
        {
            *numFrags = pktSize / fragUnit;
        }
        else
        {
            *numFrags = pktSize / fragUnit + 1;
        }

        *fragList = (Message**) MEM_malloc(sizeof(Message*) * (*numFrags));
        ERROR_Assert((*fragList) != NULL, "Out of memory!");

        for (i = 0; i < *numFrags; i ++)
        {
            fragMsg = MESSAGE_Alloc(node, 0, 0, 0);

            // copy info fields
            MESSAGE_CopyInfo(node, fragMsg, msg);

            fragRealSize = realPayloadSize - realIndex;
            if (fragRealSize >= fragUnit)
            {
                fragRealSize = fragUnit;
                fragVirtualSize = 0;
            }
            else
            {
                fragVirtualSize = virtualPayloadSize - virtualIndex;
                if (fragVirtualSize + fragRealSize > fragUnit)
                {
                    fragVirtualSize = fragUnit - fragRealSize;
                }
            }

            if (fragRealSize > 0)
            {
                MESSAGE_PacketAlloc(node,
                                    fragMsg,
                                    fragRealSize,
                                    protocolType);
                memcpy(MESSAGE_ReturnPacket(fragMsg),
                       MESSAGE_ReturnPacket(msg) + realIndex,
                       fragRealSize);
            }
            else
            {
                MESSAGE_PacketAlloc(node, fragMsg, 0, protocolType);
            }

            if (fragVirtualSize > 0)
            {
                MESSAGE_AddVirtualPayload(node, fragMsg, fragVirtualSize);
            }

            (*fragList)[i] = fragMsg;

            realIndex += fragRealSize;
            virtualIndex += fragVirtualSize;
        }

        // copy other simulation specific information to the first fragment
        (*fragList)[0]->sequenceNumber = msg->sequenceNumber;
        (*fragList)[0]->originatingProtocol = msg->originatingProtocol;
        (*fragList)[0]->numberOfHeaders = msg->numberOfHeaders;

        for (i = 0; i < msg->numberOfHeaders; i ++)
        {
            (*fragList)[0]->headerProtocols[i] = msg->headerProtocols[i];
            (*fragList)[0]->headerSizes[i] = msg->headerSizes[i];
        }

        MESSAGE_Free(node, msg);
    }
}

// Reassemble multiple fragments into one packet
// Note: All the fragments will be freed in this function.
//
// \param node  node which is assembling the packet
// \param fragList  A list of fragments.
// \param numFrags  Number of fragments in the fragment list.
// \param protocolType  Protocol type for packet tracing.
//
// \return The reassembled packet.
Message* MESSAGE_ReassemblePacket(
             Node* node,
             Message** fragList,
             int numFrags,
             TraceProtocolType protocolType)
{
    Message* msg;
    int realPayloadSize = 0;
    int virtualPayloadSize = 0;
    char* payload;
    int i;

    if (numFrags <= 0)
    {
        return NULL;
    }
    else if (numFrags == 1)
    {
        // only one fragment, just return it.
        msg = fragList[0];
        fragList[0] = NULL;

        return msg;
    }

    // more than 1 fragments, reassemble it now

    for (i = 0; i < numFrags; i ++)
    {
        realPayloadSize += MESSAGE_ReturnActualPacketSize(fragList[i]);
        virtualPayloadSize += MESSAGE_ReturnVirtualPacketSize(fragList[i]);
    }

    msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        realPayloadSize,
                        protocolType);

    // copy info fields from the first fragment
    MESSAGE_CopyInfo(node, msg, fragList[0]);

    // copy other simulation specific information from the first fragment
    msg->sequenceNumber = fragList[0]->sequenceNumber;
    msg->originatingProtocol = fragList[0]->originatingProtocol;
    msg->numberOfHeaders = fragList[0]->numberOfHeaders;
    for (i = 0; i < msg->numberOfHeaders; i ++)
    {
        msg->headerProtocols[i] = fragList[0]->headerProtocols[i];
        msg->headerSizes[i] = fragList[0]->headerSizes[i];
    }

    // reassemble the payload
    payload = MESSAGE_ReturnPacket(msg);
    for (i = 0; i < numFrags; i ++)
    {
        realPayloadSize = MESSAGE_ReturnActualPacketSize(fragList[i]);

        // copy the real payload
        memcpy(payload,
               MESSAGE_ReturnPacket(fragList[i]),
               realPayloadSize);
        payload += realPayloadSize;

        MESSAGE_Free(node, fragList[i]);
        fragList[i] = NULL;
    }

    // add the virtual payload
    MESSAGE_AddVirtualPayload(node, msg, virtualPayloadSize);

    return msg;
}

// FUNCTION   :: MESSAGE_Serialize
// LAYER      :: ANY
// PURPOSE    :: Serialize a single message into a buffer so that the orignal
//               message can be recovered from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to a messages
// + buffer    : string   : The string buffer the message will be serialzed into (append to the end)
// RETURN     :: void     : NULL
void MESSAGE_Serialize(Node* node,
                       Message* msg,
                       std::string& buffer)
{
    buffer.append((const char*)msg, MESSAGE_SizeOf());

    // handle the payload field
    buffer.append(msg->packet, msg->packetSize);

    // Append the size of the infoArray.
    int infoArraySize = (int)msg->infoArray.size();
    buffer.append((char*)(&infoArraySize), sizeof(infoArraySize));

    // handle the info fields
    for (unsigned int i = 0; i < msg->infoArray.size(); i++)
    {
        MessageInfoHeader* hdrPtr = &(msg->infoArray[i]);
        buffer.append((char*)&(hdrPtr->infoType),
                        sizeof(hdrPtr->infoType));
        buffer.append((char*)&(hdrPtr->infoSize),
                        sizeof(hdrPtr->infoSize));

        if ((hdrPtr->infoType == INFO_TYPE_DEFAULT && hdrPtr->infoSize >
            SMALL_INFO_SPACE_SIZE) ||
            (hdrPtr->infoType != INFO_TYPE_DEFAULT && hdrPtr->infoSize > 0))
        {
            // Add the infoType and infoSize.
            buffer.append(hdrPtr->info,
                          hdrPtr->infoSize);
        }
    }

    int bookkeepSize = (int)msg->infoBookKeeping.size();
    buffer.append((char*)&bookkeepSize, sizeof(bookkeepSize));

    for (unsigned int i = 0; i < msg->infoBookKeeping.size(); i++)
    {
        MessageInfoBookKeeping& bookKeep = msg->infoBookKeeping[i];
        buffer.append((char*)&bookKeep, sizeof(bookKeep));
    }

    double band[2];
    if (msg->m_band == 0)
    {
      band[0] = 0.0;
      band[1] = 0.0;
    }
    else
    {
      band[0] = msg->m_band->getFrequency();
      band[1] = msg->m_band->getBandwidth();
    }
    buffer.append((char*)band, sizeof(band));
}

// FUNCTION   :: MESSAGE_Unserialize
// LAYER      :: MAC
// PURPOSE    :: recover the orignal message from the buffer
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + buffer    : string   : The string buffer containing the message was serialzed into
// + bufIndex  : size_t&  : the start position in the buffer pointing to the message
//                          updated to the end of the message after the unserialization.
// RETURN     :: Message* : Message pointer to be recovered
Message* MESSAGE_Unserialize(PartitionData* partitionData,
                             const char* buffer,
                             int& bufIndex,
                             bool mt)
{
    Message* tmpMsg;
    Message* newMsg;

    tmpMsg = (Message*) (buffer + bufIndex);
    bufIndex += sizeof(Message);

    // build the message structure first
    newMsg = MESSAGE_Alloc(partitionData,
                           tmpMsg->layerType,
                           tmpMsg->protocolType,
                           tmpMsg->eventType,
                           mt);
    // handle packet field
    newMsg->payloadSize = tmpMsg->payloadSize;
    newMsg->packetSize = tmpMsg->packetSize;

    if (newMsg->payloadSize > 0)
    {
        newMsg->payload = MESSAGE_PayloadAlloc(
                              partitionData,
                              tmpMsg->payloadSize);
        newMsg->packet = newMsg->payload +
                         (tmpMsg->packet - tmpMsg->payload);
        memcpy(newMsg->packet,
               &buffer[bufIndex],
               tmpMsg->packetSize);
        bufIndex += tmpMsg->packetSize;
    }
    else
    {
        newMsg->payload = NULL;
        newMsg->packet = NULL;
    }

    // handle info field
    int infoArraySize;
    memcpy(&infoArraySize, &buffer[bufIndex], sizeof(infoArraySize));
    bufIndex += sizeof(infoArraySize);
    for (int i = 0; i < infoArraySize; i ++)
    {
        char* info;
        MessageInfoHeader hdrPtr;
        memcpy(&(hdrPtr.infoType), &buffer[bufIndex], sizeof(hdrPtr.infoType));
        bufIndex += sizeof(hdrPtr.infoType);
        memcpy(&(hdrPtr.infoSize), &buffer[bufIndex], sizeof(hdrPtr.infoSize));
        bufIndex += sizeof(hdrPtr.infoSize);

        if (hdrPtr.infoSize > 0)
        {
            info = MESSAGE_AppendInfo(
                               partitionData,
                               newMsg,
                               hdrPtr.infoSize,
                               hdrPtr.infoType);
            if (hdrPtr.infoType == INFO_TYPE_DEFAULT &&
                hdrPtr.infoSize <= SMALL_INFO_SPACE_SIZE)
            {
                memcpy(MESSAGE_ReturnInfo(newMsg),
                       tmpMsg->smallInfoSpace,
                       hdrPtr.infoSize);
            }
            else
            {
                // bufIndex points to end of message, including
                // appended packet, if any
                memcpy(info, //MESSAGE_ReturnInfo(newMsg, hdrPtr.infoType),
                       &buffer[bufIndex],
                       hdrPtr.infoSize);
                bufIndex += hdrPtr.infoSize;
            }
        }
    }

    int bookKeepSize;
    MessageInfoBookKeeping bookInfo;
    memcpy(&bookKeepSize, &buffer[bufIndex], sizeof(bookKeepSize));
    bufIndex += sizeof(bookKeepSize);
    for (int i = 0; i < bookKeepSize; i ++)
    {
        memcpy(&bookInfo, &buffer[bufIndex], sizeof(MessageInfoBookKeeping));
        bufIndex += sizeof(newMsg->infoBookKeeping[i]);
        newMsg->infoBookKeeping.push_back(bookInfo);
    }

    double band[2];
    memcpy(band, &buffer[bufIndex], sizeof(band));
    bufIndex += sizeof(band);
    if (band[0] != 0.0)
    {
      spectralBand_Square sb(band[0], band[1], "");
      MESSAGE_SetSpectralBand(0, newMsg, &sb);
    }

    { // set the rest of the fields
        int i;
        newMsg->instanceId          = tmpMsg->instanceId;
        newMsg->m_radioId           = tmpMsg->m_radioId;
        newMsg->naturalOrder        = tmpMsg->naturalOrder;
        newMsg->error               = tmpMsg->error;
        newMsg->allowLoose          = tmpMsg->allowLoose;
        newMsg->nodeId              = tmpMsg->nodeId;
        newMsg->eventTime           = tmpMsg->eventTime;
        newMsg->eot                 = tmpMsg->eot;
        newMsg->sourcePartitionId   = tmpMsg->sourcePartitionId;
        //packetSize, packet, payload, payloadSize all copied elsewhere
        newMsg->virtualPayloadSize  = tmpMsg->virtualPayloadSize;
        newMsg->packetCreationTime  = tmpMsg->packetCreationTime;
        newMsg->pktNetworkSendTime  = tmpMsg->pktNetworkSendTime;
        newMsg->cancelled           = tmpMsg->cancelled;
        newMsg->originatingNodeId   = tmpMsg->originatingNodeId;
        newMsg->sequenceNumber      = tmpMsg->sequenceNumber;
        newMsg->originatingProtocol = tmpMsg->originatingProtocol;
        newMsg->numberOfHeaders     = tmpMsg->numberOfHeaders;
        newMsg->relayNodeAddr       = tmpMsg->relayNodeAddr;
        newMsg->subChannelIndex     = tmpMsg->subChannelIndex;
        newMsg->isPacked            = tmpMsg->isPacked;

        newMsg->isEmulationPacket   = tmpMsg->isEmulationPacket;
        newMsg->timerManager        = NULL;   // timers not serialized
        newMsg->isScheduledOnMainHeap = true; // timers not serialized
        newMsg->timerExpiresAt      = 0;      // timers not serialized

        newMsg->d_stamped = tmpMsg->d_stamped;
        newMsg->d_realStampTime = tmpMsg->d_realStampTime;
        newMsg->d_simStampTime = tmpMsg->d_simStampTime;
        newMsg->d_stampId = tmpMsg->d_stampId;

        if (newMsg->isPacked)
        {
            newMsg->actualPktSize   = tmpMsg->actualPktSize;
        }

        for (i = 0; i < newMsg->numberOfHeaders; i++)
        {
            newMsg->headerProtocols[i] = tmpMsg->headerProtocols[i];
            newMsg->headerSizes[i]     = tmpMsg->headerSizes[i];
        }
    }

    return newMsg;
}

// FUNCTION   :: MESSAGE_SerializeMsgList
// LAYER      :: MAC
// PURPOSE    :: Store a list of messages into a buffer so that the orignal
//               messages can be recovered from the buffer
// PARAMETERS ::
// + node      : Node*    : Pointer to node.
// + msg       : Message* : Pointer to a message list
// + buffer    : string&  : The string buffer the messages will be serialzed into (append to the end)
// RETURN     :: int      : number of messages in the list
int MESSAGE_SerializeMsgList(Node* node,
                             Message* msgList,
                             std::string& buffer)
{
    Message* nextMsg = msgList;
    Message* lastMsg;

    int  numMsgs = 0;
    while (nextMsg != NULL)
    {
        MESSAGE_Serialize(node, nextMsg, buffer);

        lastMsg = nextMsg;
        nextMsg = nextMsg->next;

        MESSAGE_Free(node, lastMsg);
        numMsgs++;
    }
    return numMsgs;
}

// FUNCTION   :: MESSAGE_UnserializeMsgList
// LAYER      :: MAC
// PURPOSE    :: recover the orignal message list from the buffer
// PARAMETERS ::
// + partitionData : PartitionData* : Pointer to partition data.
// + buffer    : string   : The string buffer containing the message list serialzed into
// + bufIndex  : size_t&  : the start position in the buffer pointing to the message list
//                          updated to the end of the message list after the unserialization.
// + numMsgs   : size_t   : Number of messages in the list
// RETURN     :: Message* : Pointer to the message list to be recovered
Message* MESSAGE_UnserializeMsgList(PartitionData* partitionData,
                                    const char* buffer,
                                    int& bufIndex,
                                    unsigned int numMsgs)
{
    Message* firstMsg = NULL;
    Message* lastMsg = NULL;

    size_t numMsgsDone = 0;
    while (numMsgsDone < numMsgs)
    {
        Message* newMsg = MESSAGE_Unserialize(
                            partitionData,
                            buffer,
                            bufIndex);

        if (firstMsg == NULL)
        {
            firstMsg = newMsg;
            lastMsg = newMsg;
        }
        else
        {
            lastMsg->next = newMsg;
            lastMsg = newMsg;
        }
        numMsgsDone++;
    }
    // set the next field of the last msg to NULL
    lastMsg->next = NULL;

    // return the list of messages
    return firstMsg;
}

// Pack a list of messages to be one message structure
// Whole contents of the list messages will be put as
// payload of the new message. So the packet size of
// the new message cannot be directly used now.
// The original lis of msgs will be freed.
//
// \param node  Pointer to node.
// \param msgList  Pointer to a list of messages
// \param origProtocol  Protocol allocating this packet
// \param actualPktSize  For return sum of packet size of msgs in list
//
// \return The super msg contains a list of msgs as payload
Message* MESSAGE_PackMessage(Node* node,
                             Message* msgList,
                             TraceProtocolType origProtocol,
                             int* actualPktSize)
{
    Message* nextMsg = msgList;
    Message* lastMsg;
    Message* newMsg;

    std::string buffer;
    int pktSize =0;

    ERROR_Assert(msgList != NULL, "Corrupted message list!");

    newMsg = MESSAGE_Alloc(node, 0, 0, 0);
    buffer.reserve(5000);
    while (nextMsg != NULL)
    {
        if (!nextMsg->isPacked)
        {
            // if pack a non-packed msg
            pktSize += MESSAGE_ReturnPacketSize(nextMsg);
        }
        else
        {
            // if pack a packed msg
            pktSize += MESSAGE_ReturnActualPacketSize(nextMsg);
        }

        MESSAGE_Serialize(node, nextMsg, buffer);

        lastMsg = nextMsg;
        nextMsg = nextMsg->next;

#ifdef ADDON_DB
        // Copy the address info field to the packed new message
        StatsDBCopyMessageAddrInfo(node, newMsg, lastMsg);
#endif // ADDON_DB
        // free the orignal message
        MESSAGE_Free(node, lastMsg);
    }

    MESSAGE_PacketAlloc(node, newMsg,(int) buffer.size(), origProtocol);
    memcpy(MESSAGE_ReturnPacket(newMsg),
           buffer.data(),
           buffer.size());

    newMsg->isPacked = TRUE;
    newMsg->actualPktSize = pktSize;

    if (actualPktSize != NULL)
    {
        *actualPktSize = pktSize;
    }

    return newMsg;
}

// Unpack a super message to the original list of messages
// The list of messages were stored as payload of this super
// message.
//
// \param node  Pointer to node.
// \param msg  Pointer to the supper msg contains list of msgs
// \param copyInfo  Whether copy info from old msg to first msg
// \param freeOld  Whether the original message should be freed
//
// \return A list of messages unpacked from original msg
Message* MESSAGE_UnpackMessage(Node* node,
                               Message* msg,
                               bool copyInfo,
                               bool freeOld)
{
    Message* firstMsg = NULL;
    Message* lastMsg = NULL;
    Message* newMsg;

    char* payload;
    int payloadSize;
    int msgSize;

    payload = MESSAGE_ReturnPacket(msg);
    payloadSize = MESSAGE_ReturnPacketSize(msg);
    msgSize = 0;

    while (msgSize < payloadSize)
    {
        newMsg = MESSAGE_Unserialize(node->partitionData,
                                     payload,
                                     msgSize);

        // add new msg to msgList
        if (firstMsg == NULL)
        {
            firstMsg = newMsg;
            lastMsg = newMsg;
        }
        else
        {
            lastMsg->next = newMsg;
            lastMsg = newMsg;
        }
    }

    if (copyInfo && MESSAGE_ReturnInfoSize(msg) > 0)
    {
        // copy over the info field of original msg to first one of list
        MESSAGE_InfoAlloc(node, firstMsg, MESSAGE_ReturnInfoSize(msg));
        memcpy(MESSAGE_ReturnInfo(firstMsg),
               MESSAGE_ReturnInfo(msg),
               MESSAGE_ReturnInfoSize(msg));
    }

    // free original message
    if (freeOld)
    {
        MESSAGE_Free(node, msg);
    }

    // set the next field of the last msg to NULL
    lastMsg->next = NULL;

    // return the list of messages
    return firstMsg;
}

/*
 * FUNCTION     MESSAGE_Send
 * PURPOSE      Function call used to send a message within QualNet. When
 *              a message is sent using this mechanism, only the pointer to
 *              the message is actually sent through the system. So the user
 *              has to be careful not to do anything with the pointer to the
 *              message once MESSAGE_Send has been called.
 *
 * Parameters:
 *    node:       node which is sending message
 *    msg:        message to be delivered
 *    delay:      delay suffered by this message.
 *    isMT:       is the function being called from a thread?
 */
void MESSAGE_Send(Node *node, Message *msg, clocktype delay, bool isMT) {
    if (DEBUG)
    {
        printf("at %" TYPES_64BITFMT "d: partition %d node %3d scheduling event %3d at "
               "time %" TYPES_64BITFMT "d on interface %2d\n",
               node->getNodeTime(),
               node->partitionData->partitionId,
               node->nodeId,
               msg->eventType,
               node->getNodeTime() + delay,
               msg->instanceId);
        //MESSAGE_PrintMessage(msg);
        fflush(stdout);
    }

    if (delay < 0)
    {
        char errorStr[MAX_STRING_LENGTH];
        char delayStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (delay, delayStr);
        sprintf(errorStr,
            "Node %d sending message with invalid negative delay of %s\n",
            node->nodeId,
            delayStr);
        ERROR_ReportError(errorStr);
    }
    else if (delay == CLOCKTYPE_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "Node %d sending message with invalid delay of CLOCKTYPE_MAX\n",
            node->nodeId);
        ERROR_ReportError(errorStr);
    }

#ifdef USE_MPI //Parallel
    if (node->partitionId != node->partitionData->partitionId) {
        // don't schedule this because it's a remote node.
        MESSAGE_Free(node, msg);
        return;
    }
#endif //endParallel

#ifdef DEBUG_EVENT_TRACE
    printf("at %lld, p%d, node %d scheduling %d on intf %d, delay is %lld\n",
           node->getNodeTime(), node->partitionData->partitionId, node->nodeId,
           msg->eventType, msg->instanceId, delay);
#endif

    ERROR_Assert(!msg->getFreed(), "Sending a freed message");
    ERROR_Assert(!msg->getDeleted(), "Sending a deleted message");
    ERROR_Assert(!msg->getSent(), "Sending an already scheduled message");
    msg->setSent(true);

    msg->naturalOrder = node->partitionData->eventSequence++;
    if (isMT)
    {
        msg->nodeId = node->nodeId;
        // calculate the requested event time
        msg->eventTime = node->getNodeTime() + delay;
#ifdef USE_LOCK_FREE_QUEUE
        node->partitionData->externalLFMsgQ->pushBack(msg);
#else
        {
            if (DEBUG)
            {
                printf("at %" TYPES_64BITFMT "d: partition %d node %3d sending"
                       " thread event %3d with delay %" TYPES_64BITFMT "d on"
                       " interface %2d\n",
                       node->getNodeTime(),
                       node->partitionData->partitionId,
                       node->nodeId,
                       msg->eventType,
                       delay,
                       msg->instanceId);
                fflush(stdout);
            }
            // lock the list of pending messages, hopefully the lock disappears
            // outside this little block.
            QNThreadLock messageListLock(node->partitionData->sendMTListMutex);
            // add
            node->partitionData->sendMTList->push_back(msg);
        }
        // list is now unlocked
        // Now, partition private (or some other code that is executing as
        // part of simulation thread needs to call PARTTIONT_SendMTPrcoess ()
#endif // USE_LOCK_FREE_QUEUE
    }
    else
    {
        SCHED_InsertMessage(node, msg, delay);
    }


    MESSAGE_DebugSend(node->partitionData, node, msg);
}


/*
 * FUNCTION     MESSAGE_RemoteSend
 * PURPOSE      Function call used to send a message within QualNet. When
 *              a message is sent using this mechanism, only the pointer to
 *              the message is actually sent through the system. So the user
 *              has to be careful not to do anything with the pointer to the
 *              message once MESSAGE_Send has been called.
 *
 * Parameters:
 *    node:       node which is sending message
 *    destNodeId: nodeId of receiving node
 *    msg:        message to be delivered
 *    delay:      delay suffered by this message.
 */
void MESSAGE_RemoteSend(Node*     node,
                        NodeId    destNodeId,
                        Message*  msg,
                        clocktype delay) {
    if (delay < 0)
    {
        char errorStr[MAX_STRING_LENGTH];
        char delayStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (delay, delayStr);
        sprintf(errorStr,
            "Node %d sending message with invalid negative delay of %s\n",
            node->nodeId,
            delayStr);
        ERROR_ReportError(errorStr);
    }
    else if (delay == CLOCKTYPE_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
            "Node %d sending message with invalid delay of CLOCKTYPE_MAX\n",
            node->nodeId);
        ERROR_ReportError(errorStr);
    }

    Node* destNode = MAPPING_GetNodePtrFromHash(node->partitionData->nodeIdHash,
                                                destNodeId);

#ifdef PARALLEL //Parallel

    if (destNode == NULL) {

        destNode =  MAPPING_GetNodePtrFromHash(node->partitionData->remoteNodeIdHash, destNodeId);
        assert(destNode != NULL);

        msg->nodeId    = destNodeId;
        msg->eventTime = node->getNodeTime() + delay;

        if (DEBUG) {
            printf("at %015" TYPES_64BITFMT "d, partition %d sending "
                   "remote message to %d for node %d\n",
                   node->getNodeTime(), node->partitionData->partitionId,
                   destNode->partitionId, destNode->nodeId);
            fflush(stdout);
        }

        PARALLEL_SendRemoteMessages(msg,
                                    node->partitionData,
                                    destNode->partitionId);
#ifdef USE_MPI
        MESSAGE_Free(node, msg);
#endif
        return;
    }

#endif //endParallel

    assert(destNode != NULL);

#ifdef DEBUG_EVENT_TRACE
    printf("at %lld, p%d, node %d scheduling %d on intf %d, delay is %lld\n",
        node->getNodeTime(), node->partitionData->partitionId, node->nodeId,
        msg->eventType, msg->instanceId, delay);
#endif

    msg->naturalOrder = node->partitionData->eventSequence++;
    SCHED_InsertMessage(destNode, msg, delay);
}


/*
 * FUNCTION     MESSAGE_RouteReceivedRemoteEvent
 * PURPOSE      Allow models to handle messages received from a remote
 *              partition.
 *
 *              The parallel code will set node to partitionData->firstNode
 *              if no other node can be determined.  It is otherwise up
 *              to the model to determine for which node to schedule the
 *              event.
 *              The user may assume that msg->eventTime is set properly.
 *
 * Parameters:
 *    node:       node which is receiving message
 *    msg:        message to be delivered
 */
void MESSAGE_RouteReceivedRemoteEvent(Node*    node,
                                      Message* msg) {

    PartitionData* partitionData = node->partitionData;

    // User models that use MESSAGE_RemoteSend or PARALLEL_Broadcast
    // can place handlers here to process those messages when they're
    // received.

    if (DEBUG) {
        printf("at %015" TYPES_64BITFMT "d, partition %d routing remote"
               "event %d to node %d\n",
               node->getNodeTime(),
               node->partitionData->partitionId,
               msg->eventType,
               node->nodeId);
        printf("MSG_MAC_CELLULAR_FromTch is %d\n", MSG_MAC_CELLULAR_FromTch);
        fflush(stdout);
    }

#ifdef CELLULAR_LIB
    // Abstract Cellular model TCH handling.
    if (msg->eventType == MSG_MAC_CELLULAR_FromTch) {
        MacCellularAbstractReceiveTCH(node, msg);
        if (DEBUG) {
            printf("at %015" TYPES_64BITFMT "d, partition %d finished"
                   "routing remote event to node %d, interface %d at time %"
                   TYPES_64BITFMT "d\n",
                   node->getNodeTime(), node->partitionData->partitionId,
                   node->nodeId, msg->instanceId,
                   msg->eventTime);
            fflush(stdout);
        }
        return;
    } // end of abstract cellular TCH handling
#endif // CELLULAR_LIB

    // Default handling.  If no model provides a handler for this
    // remote message, then it gets scheduled as normal.
    msg->naturalOrder = node->partitionData->eventSequence++;
    SCHED_InsertMessage(node,
                        msg,
                        msg->eventTime - node->getNodeTime());
}

size_t MESSAGE_SizeOf()
{
  //printf("message_sizeof is %d\n", sizeof(Message));
    return sizeof(Message);
}

// Fragment one packet into TWO fragments
// Note:(i) This API treats the original packet as raw packet
// and does not take account of fragmentation related
// information like fragment id. The caller of this API
// will have to itself put in logic for distinguishing
// the fragmented packets
// (ii) Overloaded MESSAGE_FragmentPacket
//
// \param node  node which is fragmenting the packet
// \param msg  The packet to be fragmented
// \param fragmentedMsg  First fragment
// \param remainingMsg  Remaining packet
// \param fragUnit  The unit size for fragmenting the packet
// \param protocolType  Protocol type for packet tracing.
// \param freeOriginalMsg  If TRUE, then original msg is set to NULL
//
// \return TRUE if any fragment is created, FALSE otherwise

BOOL
MESSAGE_FragmentPacket(
    Node* node,
    Message*& msg,
    Message*& fragmentedMsg,
    Message*& remainingMsg,
    int fragUnit,
    TraceProtocolType protocolType,
    bool freeOriginalMsg)
{
    const int NUM_FRAG = 2;
    int pktSize = MESSAGE_ReturnPacketSize(msg);

    fragmentedMsg = NULL;
    remainingMsg = NULL;

    if (pktSize <= fragUnit)
    {
        // no need to fragment
        return FALSE;
    }
    else
    {
        const int realPayloadSize = MESSAGE_ReturnActualPacketSize(msg);
        const int virtualPayloadSize = MESSAGE_ReturnVirtualPacketSize(msg);

        int realIndex = 0;
        int virtualIndex = 0;
        int fragRealSize;
        int fragVirtualSize;
        int i;

        Message* fragMsg;
        Message* fragList[2] = {NULL, NULL};

        for (i = 0; i < NUM_FRAG; ++i)
        {
            //determine fragment real and virtual payload size
            fragRealSize = realPayloadSize - realIndex;
            if ((i != NUM_FRAG -1) && fragRealSize >= fragUnit)
            {
                fragRealSize = fragUnit;
                fragVirtualSize = 0;
            }
            else
            {
                fragVirtualSize = virtualPayloadSize - virtualIndex;
                if ((i != NUM_FRAG -1) &&
                    fragVirtualSize + fragRealSize > fragUnit)
                {
                    fragVirtualSize = fragUnit - fragRealSize;
                }
            }


            if (i == NUM_FRAG - 1 && freeOriginalMsg)
            {
                //if this is last packet and if original msg is to be freed,
                //then original msg is set as fragment.
                //so no need to allocate memory for new fragment

                msg->virtualPayloadSize = fragVirtualSize;

                MESSAGE_ShrinkPacket(
                    node, msg, realPayloadSize - fragRealSize);

                fragList[i] = msg;
            }
            else
            {
                fragMsg = MESSAGE_Alloc(node, 0, 0, 0);

                // copy info fields
                MESSAGE_CopyInfo(node, fragMsg, msg);

                if (fragRealSize > 0)
                {
                    MESSAGE_PacketAlloc(node,
                                        fragMsg,
                                        fragRealSize,
                                        protocolType);
                    memcpy(MESSAGE_ReturnPacket(fragMsg),
                           MESSAGE_ReturnPacket(msg) + realIndex,
                           fragRealSize);
                }
                else
                {
                    MESSAGE_PacketAlloc(node, fragMsg, 0, protocolType);
                }

                if (fragVirtualSize > 0)
                {
                    MESSAGE_AddVirtualPayload(
                            node, fragMsg, fragVirtualSize);
                }

                fragList[i] = fragMsg;

                realIndex += fragRealSize;
                virtualIndex += fragVirtualSize;

                fragList[i]->sequenceNumber = msg->sequenceNumber;
                fragList[i]->originatingProtocol = msg->originatingProtocol;
                fragList[i]->protocolType = (short)protocolType;
                fragList[i]->layerType = 0;
                    fragList[i]->numberOfHeaders = msg->numberOfHeaders;
                fragList[i]->packetCreationTime = node->getNodeTime();
                fragList[i]->originatingNodeId   = msg->originatingNodeId;
                fragList[i]->naturalOrder = msg->naturalOrder;
                fragList[i]->instanceId = msg->instanceId;
                //copy headers
                for (int j = 0; j < msg->numberOfHeaders; j++)
                {
                    fragList[i]->headerProtocols[j] =
                                     msg->headerProtocols[j];
                    fragList[i]->headerSizes[j] = msg->headerSizes[j];
                }
            }
        } //end for loop

        fragmentedMsg = fragList[0];
        remainingMsg = fragList[1];

        if (freeOriginalMsg)
        {
            msg = NULL;
        }
    }
    return TRUE;
}



// Reassemble TWO fragments into one packet
// Note: (i) None of the fragments will be freed in this API.
// The caller of this API will itself have to free
// the fragments
// (ii) Overloaded MESSAGE_ReassemblePacket
//
// \param node  node which is assembling the packet
// \param fragMsg1  First fragment
// \param fragMsg2  Second fragment
// \param protocolType  Protocol type for packet tracing.
//
// \return The reassembled packet.
Message*
MESSAGE_ReassemblePacket(
             Node* node,
             Message* fragMsg1,
             Message* fragMsg2,
             TraceProtocolType protocolType)
{
    Message* msg = NULL;
    int realPayloadSize = 0;
    int virtualPayloadSize = 0;
    char* payload;
    int i;
    Message* fragList[2] = {fragMsg1, fragMsg2};

    if (fragMsg1 == NULL || fragMsg2 == NULL)
    {
        msg = NULL;
        if (fragMsg1)
        {
            msg = fragMsg1;
        }
        else if (fragMsg2)
        {
            //First Packet is NULL but the second is not
            //so one fragment has been lost and thus
            //msg can't be reassemled
            //return NULL
            //The caller of this function should handle fragMsg2 (possibly
            //drop it)
            return (Message*)NULL;
        }
        return msg;
    }

    // more than 1 fragments, reassemble it now

    for (i = 0; i < 2; i++)
    {
        realPayloadSize += MESSAGE_ReturnActualPacketSize(fragList[i]);
        virtualPayloadSize += MESSAGE_ReturnVirtualPacketSize(fragList[i]);
    }

    msg = MESSAGE_Alloc(node, 0, 0, 0);
    MESSAGE_PacketAlloc(node,
                        msg,
                        realPayloadSize,
                        protocolType);

    // copy info fields from the first fragment
    MESSAGE_CopyInfo(node, msg, fragList[0]);

    //copy headers from the first fragment
    for (i = 0; i < fragList[0]->numberOfHeaders; i ++)
    {
        msg->headerProtocols[i] = fragList[0]->headerProtocols[i];
        msg->headerSizes[i] = fragList[0]->headerSizes[i];
    }

    // copy other simulation specific information from the first fragment
    msg->sequenceNumber = fragList[0]->sequenceNumber;
    msg->originatingProtocol = fragList[0]->originatingProtocol;
    msg->protocolType = (short)protocolType;
    msg->numberOfHeaders = fragList[0]->numberOfHeaders;
    msg->originatingNodeId = fragList[0]->originatingNodeId;
    msg->naturalOrder =  fragList[0]->naturalOrder;
    msg->instanceId = fragList[0]->instanceId;

    msg->d_stamped = fragList[0]->d_stamped;
    msg->d_realStampTime = fragList[0]->d_realStampTime;
    msg->d_simStampTime = fragList[0]->d_simStampTime;
    msg->d_stampId = fragList[0]->d_stampId;

    // reassemble the payload
    payload = MESSAGE_ReturnPacket(msg);
    for (i = 0; i < 2; i ++)
    {
        realPayloadSize = MESSAGE_ReturnActualPacketSize(fragList[i]);

        // copy the real payload
        memcpy(payload,
               MESSAGE_ReturnPacket(fragList[i]),
               realPayloadSize);
        payload += realPayloadSize;
    }

    // add the virtual payload
    MESSAGE_AddVirtualPayload(node, msg, virtualPayloadSize);

    return msg;
}

// PARAMETERS ::
// + node    :  Node*     : node which is sending message
// + msg     :  Message*  : message to be delivered
void MESSAGE_SendAsEarlyAsPossible(Node *node, Message *msg)
{
    clocktype safeDelay = 0;

    if (node->partitionData->isRunningInParallel())
    {
        if (node->getNodeTime() < node->partitionData->safeTime + 1)
        {
            safeDelay = node->partitionData->safeTime + 1 - node->getNodeTime();
        }
    }

    MESSAGE_Send(node, msg, safeDelay);
}

void MESSAGE_RemoteSendSafely(Node*     node,
                        NodeId    destNodeId,
                        Message*  msg,
                        clocktype delay)
{
    if (delay < 0)
    {
        char errorStr[MAX_STRING_LENGTH];
        char delayStr[MAX_STRING_LENGTH];
        TIME_PrintClockInSecond (delay, delayStr);
        sprintf(errorStr,
                "Node %d sending message with invalid negative delay of %s\n",
                node->nodeId,
                delayStr);
        ERROR_ReportError(errorStr);
    }
    else if (delay == CLOCKTYPE_MAX)
    {
        char errorStr[MAX_STRING_LENGTH];
        sprintf(errorStr,
                "Node %d sending message with invalid delay of CLOCKTYPE_MAX\n",
                node->nodeId);
        ERROR_ReportError(errorStr);
    }

    if (node->getNodeTime() + delay < node->partitionData->safeTime + 1)
    {
        delay = node->partitionData->safeTime + 1 - node->getNodeTime();
    }
    //printf(" delay value is %.9f\n",(double)delay/MILLI_SECOND);
    MESSAGE_RemoteSend(node, destNodeId, msg, delay);
}

/*
 * FUNCTION     MESSAGE_DebugSend
 * PURPOSE      Perform debugging and tracing activities for this message
 *              as it is sent
 *
 * Parameters:
 *    partition:  partition which is debugging
 *    node:       node which is debugging.  May be null.
 *    msg:        message to debug and trace
 */
void MESSAGE_DebugSend(PartitionData* partition, Node* node, Message* msg)
{
#ifdef MESSAGE_GRAPH
    // Set naturalOrder for partition level events
    if (node == NULL)
    {
        msg->naturalOrder = partition->eventSequence++;
    }

    // Print message graph if time is greater than 0
    if (partition->getGlobalTime() > 0)
    {
        printf("(%d, %d, %d) -> (%d, %d, %d)\n",
            gGraphMessage.originatingNodeId,
            gGraphMessage.naturalOrder,
            gGraphMessage.eventType,
            msg->originatingNodeId,
            msg->naturalOrder,
            msg->eventType);
        fflush(stdout);
    }
#endif
}

/*
 * FUNCTION     MESSAGE_DebuProcess
 * PURPOSE      Perform debugging and tracing activities for this message
 *              as it is processed
 *
 * Parameters:
 *    partition:  partition which is debugging
 *    node:       node which is debugging.  May be null.
 *    msg:        message to debug and trace
 */
void MESSAGE_DebugProcess(PartitionData* partition, Node* node, Message* msg)
{
    ERROR_Assert(partition != NULL, "Invalid partition data");
    ERROR_Assert(msg != NULL, "Invalid message");

#define EVENT_TRACE_SIMPLE 0
    if (node && EVENT_TRACE_SIMPLE)
    {
        char clockStr[MAX_CLOCK_STRING_LENGTH];
        ctoa(node->getNodeTime(), clockStr);
        printf("at %s: partition %d Node %3d executing event %3d on interface %2d\n",
               clockStr,
               partition->partitionId,
               node->nodeId,
               msg->eventType,
               msg->instanceId);
        fflush(stdout);
    }

#define EVENT_TRACE_FILE 0
    if (node && EVENT_TRACE_FILE)
    {
        static FILE * traceFH [32] = { NULL, NULL };
        char clockStr[MAX_CLOCK_STRING_LENGTH];
        char extraEventMsg [256];
        bool subtractedEvent = false;

        if (traceFH [node->partitionData->partitionId] == NULL)
        {
            char traceFName [256];
            sprintf (traceFName, "event_trace_partition%d.txt",
                node->partitionData->partitionId);
            traceFH [node->partitionData->partitionId] = fopen (traceFName, "w");
            printf ("opened file %s for event logging\n", traceFName);
        }

        ctoa(node->getNodeTime(), clockStr);
        if (msg->eventType == MSG_PROP_SignalReleased)
        {
            PropTxInfo* propTxInfo = (PropTxInfo*)MESSAGE_ReturnInfo(msg);
            sprintf (extraEventMsg, "sigRelease from tx nd %03d", propTxInfo->txNodeId);
            // SignalReleased isn't counted because they get replicated to all partitons
            subtractedEvent = true;
        }
        else
        {
            extraEventMsg [0] = 'n';
            extraEventMsg [1] = 'i';
            extraEventMsg [2] = 'l';
            extraEventMsg [3] = '\0';
        }
        if (!subtractedEvent)
        {
            char clockStr[MAX_CLOCK_STRING_LENGTH];
            ctoa(node->getNodeTime(), clockStr);
            //  "at %015lld: Node %03d executing event %03d (layer %03d, protcol %03d) on interface %02d %s\n",
            fprintf(traceFH [node->partitionData->partitionId],
                    "at %s: Node %03d executing event %03d (layer %03d, protcol %03d) on interface %02d %s",
                    clockStr,
                    (int) node->nodeId,
                    (int) msg->eventType,
                    (int) msg->layerType,
                    (int) msg->protocolType,
                    (int) msg->instanceId,
                    extraEventMsg);
        }
        else
        {
            fprintf(traceFH [node->partitionData->partitionId], "at %015"
                    TYPES_64BITFMT "d: parallel EXTRA Ev - Node %03d"
                    "executing event %03d on interface %02d %s",
                    node->getNodeTime(),
                    node->nodeIndex,
                    msg->eventType,
                    msg->instanceId,
                    extraEventMsg);
        }
        if (msg->cancelled) {
            fprintf(traceFH [node->partitionData->partitionId],
                    "    (msg cancelled)\n");
        }
        else {
            fprintf(traceFH [node->partitionData->partitionId],
                    "\n");
        }
        fflush(traceFH [node->partitionData->partitionId]);
    }

#ifdef MESSAGE_GRAPH
    // Save this message as the current processed message
    gGraphMessage.originatingNodeId = msg->originatingNodeId;
    gGraphMessage.naturalOrder = msg->naturalOrder;
    gGraphMessage.eventType = msg->eventType;
#endif
}

double Message::now()
{
#ifdef JEFF
    struct timespec tsnow;
    clock_gettime(CLOCK_REALTIME, &tsnow);
    double t = (double)tsnow.tv_sec + (double)tsnow.tv_nsec * 1.0e-9;

    return t;
#else
    return 0.0;
#endif
}

void Message::stamp(Node* node)
{
    static long long s_stamp_counter = 0;

    d_stampId = ++s_stamp_counter;

    d_simStampTime = (double)node->getNodeTime() / (double)SECOND;
    d_realStampTime = now();
    d_stamped = true;

/*
    printf("Stamped as id=%lld real=%0.6f sim=%0.6f\n",
           d_stampId, d_realStampTime, d_simStampTime);
*/
}

double Message::deltaSimTime(Node* node)
{
  return (d_stamped) ? ((double)node->getNodeTime() / (double)SECOND)
     - d_simStampTime : 0.0;
}
