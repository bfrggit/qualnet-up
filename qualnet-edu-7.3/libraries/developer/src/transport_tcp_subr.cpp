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

//
//
// Ported from FreeBSD 2.2.2.
// This file contains miscellaneous TCP subroutines, eg. initialization.
//

//
// Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
//  The Regents of the University of California.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this software
//    must display the following acknowledgement:
//  This product includes software developed by the University of
//  California, Berkeley and its contributors.
// 4. Neither the name of the University nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
//  @(#)tcp_subr.c  8.2 (Berkeley) 5/24/95
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"
#include "network_ip.h"
#include "transport_tcp.h"
#include "transport_tcp_seq.h"
#include "transport_tcp_timer.h"
#include "transport_tcp_var.h"
#include "transport_tcpip.h"
#include "transport_tcp_proto.h"

#ifdef ADDON_DB
#include "db.h"
#endif

#define NoDEBUG


//-------------------------------------------------------------------------//
// Remove a node from reassembly queue.
//-------------------------------------------------------------------------//
void remque_ti(struct tcpiphdr *node)
{
    struct tcpiphdr *prev;
    struct tcpiphdr *next;

    next = (struct tcpiphdr *)node->ti_next;
    prev = (struct tcpiphdr *)node->ti_prev;
    prev->ti_next = (char *) next;
    next->ti_prev = (char *) prev;
}

//-------------------------------------------------------------------------//
// Insert a node just after prev.
//-------------------------------------------------------------------------//
void insque_ti(
    struct tcpiphdr *node,
    struct tcpiphdr *prev)
{
    struct tcpiphdr *next;

    next = (struct tcpiphdr *)prev->ti_next;
    node->ti_prev = (char *) prev;
    node->ti_next = (char *) next;
    prev->ti_next = (char *) node;
    next->ti_prev = (char *) node;
}


//-------------------------------------------------------------------------//
// Get a new connection id.
//-------------------------------------------------------------------------//
int get_conid(struct inpcb *head)
{
    return ++head->con_id;
}

//-------------------------------------------------------------------------//
// in_cksum --
//      Checksum routine for Internet Protocol family headers (C Version)
//      Needs modification for IPv6.
//-------------------------------------------------------------------------//
int in_cksum(
    short unsigned int *addr,
    int len)
{
    int nleft = len;
    unsigned short *w = addr;
    int sum = 0;
    unsigned short answer = 0;

    // Our algorithm is simple, using a 32 bit accumulator (sum), we add
    // sequential 16 bit words to it, and at the end, fold back all the
    // carry bits from the top 16 bits into the lower 16 bits.

    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    // mop up an odd byte, if necessary
    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w ;
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff);     // add hi 16 to low 16
    sum += (sum >> 16);                     // add carry
    answer = (unsigned short) ~sum;         // truncate to 16 bits
    return(answer);
}


//-------------------------------------------------------------------------//
// Create template to be used to send tcp packets on a connection.
// Call after host entry created, allocates an mbuf and fills
// in a skeletal tcp/ip header, minimizing the amount of work
// necessary when the connection is used.
//-------------------------------------------------------------------------//
struct tcpiphdr * tcp_template(struct tcpcb *tp)
{
    struct inpcb *inp = tp->t_inpcb;
    struct tcpiphdr *n;

    if ((n = tp->t_template) == 0) {
        n = (struct tcpiphdr *)MEM_malloc(sizeof(struct tcpiphdr));
    }
    assert (n != NULL);

    n->ti_next = n->ti_prev = 0;
    n->ti_x1 = 0;
    n->ti_pr = IPPROTO_TCP;
    n->ti_len = sizeof (struct tcphdr);
    n->ti_src = inp->inp_local_addr;
    n->ti_dst = inp->inp_remote_addr;
    n->ti_sport = inp->inp_local_port;
    n->ti_dport = inp->inp_remote_port;
    n->ti_seq = 0;
    n->ti_ack = 0;
    tcphdrSetTh_x2(&(n->ti_t.tcpHdr_x_off), 0);
    tcphdrSetDataOffset(&(n->ti_t.tcpHdr_x_off), 5);
    n->ti_flags = 0;
    n->ti_win = 0;
    n->ti_sum = 0;
    n->ti_urp = 0;
    return (n);
}

//-------------------------------------------------------------------------//
// Send a single message to the TCP at address specified by
// the given TCP/IP header.  If m == 0, then we make a copy
// of the tcpiphdr at ti and send directly to the addressed host.
// This is used to force keep alive messages out using the TCP
// template for a connection tp->t_template.  If flags are given
// then we send a message back to the TCP which originated the
// segment ti, and discard the mbuf containing it and any other
// attached mbufs.
//
// In any case the ack and sequence number of the transmitted
// segment are as specified by the parameters.
//-------------------------------------------------------------------------//
void tcp_respond(
    Node *node,
    struct tcpcb *tp,
    struct tcpiphdr *old_ti,
    int m,
    tcp_seq ack,
    tcp_seq seq,
    int flags,
    struct tcpstat *tcp_stat)
{
    int win = 0;
    struct tcpiphdr *ti;
    struct inpcb *inp;// = tp->t_inpcb;
    TosType priority; // = inp->priority;

    Message *msg;
    int outgoingInterface;

    if (tp != NULL)
    {
        inp = tp->t_inpcb;
        ERROR_Assert(inp != NULL, "tcp control block w/o inpcb");
        outgoingInterface = tp->outgoingInterface;
        priority = inp->priority;
    }
    else
    {
        inp = NULL;
        outgoingInterface = ANY_INTERFACE;
        priority = IPTOS_PREC_INTERNETCONTROL;
    }
    if (tcp_stat != NULL) {
        tcp_stat->tcps_sndtotal++;
        if (flags & (TH_SYN|TH_FIN|TH_RST))
        {
            tcp_stat->tcps_sndctrl++;
            if (flags & TH_RST)
                tcp_stat->tcps_sndrst++;
        }
        else if (tp != NULL && tp->t_flags & TF_ACKNOW)
            tcp_stat->tcps_sndacks++;
        else if (tp != NULL && SEQ_GT(tp->snd_up, tp->snd_una))
            ;//tcp_stat->tcps_sndurg++;
        else
            tcp_stat->tcps_sndwinup++;
    }

    if (tp) {
        win = sbspace(tp->t_inpcb);
    }

    msg = MESSAGE_Alloc(node, 0, 0, 0);

    //Allocated buffer memory rather than allocating memory to a packet
    //MESSAGE_PacketAlloc(node, msg, sizeof(struct tcpiphdr), TRACE_TCP);
    //ti = (struct tcpiphdr *) msg->packet;
    ti = (struct tcpiphdr *)MEM_malloc(sizeof(struct tcpiphdr));

    assert (ti != NULL);

    memcpy(ti, old_ti, sizeof(struct tcpiphdr));

    if (m == 0) {
        flags = TH_ACK;
    }
    else {
        Address tempAddr = ti->ti_dst;
        ti->ti_dst = ti->ti_src;
        ti->ti_src = tempAddr;

        unsigned short tempPort = ti->ti_dport;
        ti->ti_dport = ti->ti_sport;
        ti->ti_sport = tempPort;
    }
    ti->ti_len = (unsigned short)(sizeof (struct tcphdr));
    ti->ti_next = ti->ti_prev = 0;
    ti->ti_x1 = 0;
    ti->ti_seq = seq;
    ti->ti_ack = ack;
    tcphdrSetTh_x2(&(ti->ti_t.tcpHdr_x_off), 0) ;
    tcphdrSetDataOffset(&(ti->ti_t.tcpHdr_x_off),
        sizeof (struct tcphdr) >> 2);
    ti->ti_flags = (unsigned char)flags;
    if (tp && !TCP_VARIANT_IS_RENO(tp))
        ti->ti_win = (unsigned short) (win >> tp->rcv_scale);
    else
        ti->ti_win = (unsigned short)win;
    ti->ti_urp = 0;
    ti->ti_sum = 0;

    /* Checksum commented out because QualNet currently does not
        model bit errors
    ti->ti_sum = in_cksum((unsigned short *)ti, sizeof (struct tcpiphdr));
    */

    IpHeaderSetIpLength(&(((IpHeaderType *)ti)->ip_v_hl_tos_len),
        sizeof (struct tcpiphdr));
    ((IpHeaderType *)ti)->ip_ttl = IPDEFTTL;

    // Send the packet to the IP model.
    // Remove the ipovly pseudo-header beforehand.
    {
        Address destAddr = ti->ti_dst;

#ifdef DEBUG
        {
            char clockStr[100];
            char anAddrStr[MAX_STRING_LENGTH];

            ctoa(node->getNodeTime(), clockStr);
            IO_ConvertIpAddressToString(&destAddr, anAddrStr);

            printf("Node %u sending segment to ip_dst %s at time %s\n",
                   node->nodeId, anAddrStr, clockStr);


            //printf("    size = %d\n", msg->packetSize);
            printf("    seqno = %d\n", ti->ti_seq);
            printf("    ack = %d\n", ti->ti_ack);
            printf("    type =");

            if (flags & TH_FIN)
            {
                printf(" FIN |");
            }
            if (flags & TH_SYN)
            {
                printf(" SYN |");
            }
            if (flags & TH_RST)
            {
                printf(" RST |");
            }
            if (flags & TH_PUSH)
            {
                printf(" PUSH |");
            }
            if (flags & TH_ACK)
            {
                printf(" ACK |");
            }
            if (flags & TH_URG)
            {
                printf(" URG |");
            }

            printf("\n");
        }
#endif // DEBUG

        //Bcz the Packet is not created till so no need to remove pseudo
        //ipovly header.
        // MESSAGE_RemoveHeader(node, msg, sizeof(struct ipovly), TRACE_TCP);

        // Hack solution for TCP trace.
        //msg->headerSizes[msg->numberOfHeaders] -= sizeof(struct ipovly);
        //msg->numberOfHeaders++;


        //Create the Packet of size tcphdr which is same in both 32bit and
        //64bit compilation modes.


        MESSAGE_PacketAlloc(node, msg, sizeof(struct tcphdr),TRACE_TCP);
        struct tcphdr *tcph;
        tcph = (struct tcphdr *)msg->packet;
        memcpy(tcph, &(ti->ti_t), sizeof(struct tcphdr));


        // Trace sending packet. We don't trace the IP pseudo header
        ActionData acnData;
        acnData.actionType = SEND;
        acnData.actionComment = NO_COMMENT;
        TRACE_PrintTrace(node, msg, TRACE_TRANSPORT_LAYER, PACKET_OUT, &acnData);

        TransportTcpTrace(node, tp, ti, "output");
#ifdef ADDON_DB

    // Here we add the packet to the Network database.
    // Gather as much information we can for the database.

    HandleTransportDBEvents(
        node,
        msg,
        outgoingInterface,
        "TCPSendToLower",        
        ti->ti_sport,
        ti->ti_dport,
        sizeof (struct tcphdr),
        ti->ti_len);

    HandleTransportDBSummary(
        node,
        msg,
        outgoingInterface,
        "TCPSend",
        ti->ti_src,
        ti->ti_dst,
        ti->ti_sport,
        ti->ti_dport,
        ti->ti_len);

    HandleTransportDBAggregate(
        node,
        msg,
        outgoingInterface,
        "TCPSend", ti->ti_len);

#endif
        TransportDataTcp *tcpLayer = (TransportDataTcp *) node->transportData.tcp;
        if (tcpLayer->tcpStatsEnabled)
        {
            int headerSize = ti->ti_t.tcpHdr_x_off * 4;
            int type = STAT_AddressToDestAddressType(node, ti->ti_dst);
            if (headerSize < ti->ti_len)
            {
                // Data packet
                tcpLayer->newStats->AddSegmentSentDataPoints(
                    node,
                    msg,
                    0,
                    ti->ti_len - headerSize,
                    headerSize,
                    ti->ti_src,
                    ti->ti_dst,
                    ti->ti_sport,
                    ti->ti_dport);
            }
            else
            {
                // Control packet
                tcpLayer->newStats->AddSegmentSentDataPoints(
                    node,
                    msg,
                    ti->ti_len,
                    0,
                    0,
                    ti->ti_src,
                    ti->ti_dst,
                    ti->ti_sport,
                    ti->ti_dport);
            }
        }
        NetworkIpReceivePacketFromTransportLayer(
            node,
            msg,
            ti->ti_src,
            destAddr,
            outgoingInterface,
            priority,
            IPPROTO_TCP,
            FALSE);
    }
    MEM_free(ti);
}

//-------------------------------------------------------------------------//
// Create a new TCP control block, making an
// empty reassembly queue and hooking it to the argument
// protocol control block.
//-------------------------------------------------------------------------//
struct tcpcb * tcp_newtcpcb(
    Node *node,
    struct inpcb *inp)
{
    struct tcpcb *tp;
    TransportDataTcp *tcpLayer = (TransportDataTcp *)
                                 node->transportData.tcp;

    tp = (struct tcpcb *)MEM_malloc(sizeof(struct tcpcb));
    assert(tp != NULL);

    memset ((char *) tp, 0, sizeof(struct tcpcb));
    tp->seg_next = tp->seg_prev = (struct tcpiphdr *)tp;
    tp->mss = tp->t_maxseg = tp->t_maxopd = tcpLayer->tcpMss;
    tp->tcpVariant = tcpLayer->tcpVariant;

    /*
     * Set flags
     */
    if (tcpLayer->tcpUseRfc1323) {
        tp->t_flags = (TF_REQ_SCALE|TF_REQ_TSTMP);
    }

    if (!tcpLayer->tcpUseNagleAlgorithm) {
        tp->t_flags |= TF_NODELAY;
    }

    TransportTcpSackInit(tp);

    if (!tcpLayer->tcpUseOptions)
        tp->t_flags |= TF_NOOPT;

    if (TCP_NOPUSH)
        tp->t_flags |= TF_NOPUSH;

    tp->t_inpcb = inp;

    // Init srtt to TCPTV_SRTTBASE (0), so we can tell that we have no
    // rtt estimate.  Set rttvar so that srtt + 4 * rttvar gives
    // reasonable initial retransmit time.

    tp->t_srtt = TCPTV_SRTTBASE;
    tp->t_rttvar = ((TCPTV_RTOBASE - TCPTV_SRTTBASE) << TCP_RTTVAR_SHIFT);
    tp->t_rttmin = TCPTV_MIN;
    tp->t_rxtcur = TCPTV_RTOBASE * 2;

    tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
    tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
    tp->t_partialacks = -1;
    inp->inp_ppcb = (char *)tp;

    // To eliminates the problem of multiple Fast Retransmits we uses this
    // new variable "send_high", whose initial value is the initial send
    // sequence number. After each retransmit timeout, the highest sequence
    // numbers transmitted so far is recorded in the variable "send_high".

    if (TCP_VARIANT_IS_NEWRENO(tp)) {
        tp->send_high = tp->iss;
    }

    // ECN related variables
    tp->t_ecnFlags = 0;
    tp->ecnMaxSeq = 0;

    return (tp);
}


//-------------------------------------------------------------------------//
// Drop a TCP connection.
// If connection is synchronized,
// then send a RST to peer.
//-------------------------------------------------------------------------//
struct tcpcb * tcp_drop(
    Node *node,
    struct tcpcb *tp,
    UInt32 tcp_now,
    struct tcpstat *tcp_stat)
{

    if (TCPS_HAVERCVDSYN(tp->t_state)){
        tp->t_state = TCPS_CLOSED;
        tcp_output(node, tp, tcp_now, tcp_stat);
        if (tcp_stat)
            tcp_stat->tcps_drops++;
    } else {
        if (tcp_stat)
            tcp_stat->tcps_conndrops++;
    }
    printf("TCP: Connection dropped\n");
    return (tcp_close(node, tp, tcp_stat));
}


//-------------------------------------------------------------------------//
// Close a TCP control block:
//      discard all space held by the tcp
//      discard internet protocol block
//      wake up any sleepers
//-------------------------------------------------------------------------//
struct tcpcb * tcp_close(
    Node *node,
    struct tcpcb *tp,
    struct tcpstat *tcp_stat)
{
    struct tcpiphdr *t, *t1;
    struct inpcb *inp = tp->t_inpcb;
    Message *msg;
    TransportToAppOpenResult *tcpOpenResult;
    TransportToAppCloseResult *tcpCloseResult;

    // free the reassembly queue, if any
    t = tp->seg_next;
    while (t != (struct tcpiphdr *)tp) {
        t1 = t;
        t = (struct tcpiphdr *)t->ti_next;
        remque_ti(t1);
        MEM_free(t1);
    }

    if (tp->t_template)
        MEM_free(tp->t_template);

    inp->inp_ppcb = 0;

    if (TCP_VARIANT_IS_SACK(tp)) {
        TransportTcpSackFreeLists(tp);
    }

    MEM_free(tp);

    switch (inp->usrreq){
    case INPCB_USRREQ_OPEN:

        msg = MESSAGE_Alloc(node, APP_LAYER,
                inp->app_proto_type, MSG_APP_FromTransOpenResult);
        MESSAGE_InfoAlloc(node, msg, sizeof(TransportToAppOpenResult));
        tcpOpenResult = (TransportToAppOpenResult *) MESSAGE_ReturnInfo(msg);
        tcpOpenResult->type = TCP_CONN_ACTIVE_OPEN;
        tcpOpenResult->localAddr = inp->inp_local_addr;
        tcpOpenResult->localPort = inp->inp_local_port;
        tcpOpenResult->remoteAddr = inp->inp_remote_addr;
        tcpOpenResult->remotePort = inp->inp_remote_port;
        tcpOpenResult->connectionId = -1;

        tcpOpenResult->uniqueId = inp->unique_id;

        printf("TCP: Connection failed\n");
        MESSAGE_Send(node, msg, TRANSPORT_DELAY);

        break;

    case INPCB_USRREQ_CONNECTED:
    case INPCB_USRREQ_CLOSE:

        msg = MESSAGE_Alloc(node, APP_LAYER,
                inp->app_proto_type, MSG_APP_FromTransCloseResult);
        MESSAGE_InfoAlloc(node, msg, sizeof(TransportToAppCloseResult));
        tcpCloseResult = (TransportToAppCloseResult*)MESSAGE_ReturnInfo(msg);
        if (inp->usrreq == INPCB_USRREQ_CONNECTED) {
            tcpCloseResult->type = TCP_CONN_PASSIVE_CLOSE;
        } else {
            tcpCloseResult->type = TCP_CONN_ACTIVE_CLOSE;
        }
        tcpCloseResult->connectionId = inp->con_id;

        MESSAGE_Send(node, msg, TRANSPORT_DELAY);
        break;

    case INPCB_USRREQ_NONE: break;
    default:
        ERROR_ReportErrorArgs("TCP: unknown user request %d\n", inp->usrreq);
    }
    std::list<infofield_array*>::iterator iter;
    for (iter = inp->info_buf->info.begin();
        iter != inp->info_buf->info.end();
        )
    {
        infofield_array* hdrPtr = *iter;
        for (unsigned int j = 0; j < hdrPtr->infoArray.size(); j++)
        {
            MessageInfoHeader* infoHdr = &(hdrPtr->infoArray[j]);
            if (infoHdr->infoSize > 0)
            {
                MESSAGE_InfoFieldFree(node, infoHdr);
                infoHdr->infoSize = 0;
                infoHdr->info = NULL;
                infoHdr->infoType = INFO_TYPE_UNDEFINED;
            }
        }
        hdrPtr->infoArray.clear();
        delete hdrPtr;
        inp->info_buf->info.erase(iter);
        iter = inp->info_buf->info.begin();
    }
    in_pcbdetach(inp);

    if (tcp_stat)
        tcp_stat->tcps_closed++;
    return ((struct tcpcb *)0);
}

