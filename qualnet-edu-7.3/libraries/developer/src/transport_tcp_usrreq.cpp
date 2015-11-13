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
// This file contains TCP functions that process application requests.
//

//
// Copyright (c) 1982, 1986, 1988, 1993
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
//  From: @(#)tcp_usrreq.c  8.2 (Berkeley) 1/3/94
//

#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "transport_tcp.h"
#include "transport_tcp_seq.h"
#include "transport_tcp_timer.h"
#include "transport_tcp_var.h"
#include "transport_tcp_proto.h"

#ifdef ADDON_DB
#include "db.h"
#endif

//-------------------------------------------------------------------------//
// Prepare to accept connections.
//-------------------------------------------------------------------------//
void tcp_listen(
    Node *node,
    struct inpcb *phead,
    AppType app_type,
    Address* local_addr,
    short local_port,
    TosType priority)
{
    struct inpcb *inp;
    int result = 0;
    struct tcpcb *tp;
    Message *msg;
    TransportToAppListenResult *tcpListenResult;

    // check whether this server already exists
    inp = (struct inpcb *) in_pcblookup(phead,
                                        local_addr,
                                        local_port,
                                        0,
                                        0,
                                        INPCB_WILDCARD);

    if (inp != phead) {
        result = -2;      // listen failed, server already set up
    } else {
        Address anAddress;

        Address_SetToAnyAddress(&anAddress, local_addr);

        inp =   tcp_attach(node,
                           phead,
                           app_type,
                           local_addr,
                           local_port,
                           &anAddress,
                           -1,
                           -1,
                           priority);

        if (inp == NULL){
            result = -1;  // listen failed
        } else {
            result = inp->con_id; // listen succeeded
            tp = intotcpcb(inp);
            tp->t_state = TCPS_LISTEN;
        }
    }

    //
    // report status to application
    // result = -2, server port already set up
    // result = -1, failed to set up the server port
    // result >= 0, succeeded
    //
    msg = MESSAGE_Alloc(node, APP_LAYER,
                         app_type, MSG_APP_FromTransListenResult);
    MESSAGE_InfoAlloc(node, msg, sizeof(TransportToAppListenResult));
    tcpListenResult = (TransportToAppListenResult *) MESSAGE_ReturnInfo(msg);
    tcpListenResult->localAddr = *local_addr;
    tcpListenResult->localPort = local_port;
    tcpListenResult->connectionId = result;

    MESSAGE_Send(node, msg, TRANSPORT_DELAY);
}


//-------------------------------------------------------------------------//
// Do a send by putting data in output queue and
// possibly send more data.
//-------------------------------------------------------------------------//
void tcp_send(
    Node *node,
    struct inpcb *phead,
    int conn_id,
    unsigned char *payload,
    int actualLength,
    int virtualLength,
    UInt32 tcp_now,
    struct tcpstat *tcp_stat,
    Message* msg)
{
    struct inpcb *inp;
    struct tcpcb *tp;
    int length;

    inp  = in_pcbsearch(phead, conn_id);
    inp->ttl = phead->ttl;
    if (inp == phead) {

        // can't find the in_pcb of the connection
        ERROR_ReportWarningArgs("TCP: can't find (id=%u,conn=%d)\n",
                                node->nodeId,
                                conn_id);
        return;
    }

    // put data in the send buffer of the connection
    // and call tcp_output to send data
    tp = intotcpcb(inp);
#ifdef ADDON_DB
       HandleTransportDBEvents(
        node,
        msg,
        tp->outgoingInterface,
        "TCPReceiveFromUpper",
        inp->inp_local_port,
        inp->inp_remote_port,
        0,
        actualLength + virtualLength);
#endif
    length = append_buf(node, inp, payload, actualLength, virtualLength, msg);

    if (length > 0){
        tcp_output(node, tp, tcp_now, tcp_stat);
    }
}


//-------------------------------------------------------------------------//
// Common subroutine to open a TCP connection to remote host specified
// by remote_addr.
// Initialize connection parameters and enter SYN-SENT state.
//-------------------------------------------------------------------------//

#define TCPTV_KEEP_INIT_SHORT (4*PR_SLOWHZ)

void tcp_connect(
    Node *node,
    struct inpcb *phead,
    AppType app_type,
    Address* local_addr,
    short local_port,
    Address* remote_addr,
    short remote_port,
    UInt32 tcp_now,
    tcp_seq *tcp_iss,
    struct tcpstat *tcp_stat,
    Int32 unique_id,
    TosType priority,
    int outgoingInterface)
{

    struct inpcb *inp;
    int result = 0;
    struct tcpcb *tp = 0;
    Message *msg;
    TransportToAppOpenResult *tcpOpenResult;

    // check whether this connection already exists
    inp = (struct inpcb *) in_pcblookup(phead,
                                        local_addr,
                                        local_port,
                                        remote_addr,
                                        remote_port,
                                        INPCB_NO_WILDCARD);

    if (inp != phead) {
        // connection already exists
        result = -1;
        // printf("TCP: Connection already exists\n");
    } else {

        // create inpcb and tcpcb for this connection
        inp  = tcp_attach(node,
                          phead,
                          app_type,
                          local_addr,
                          local_port,
                          remote_addr,
                          remote_port,
                          unique_id,
                          priority);

         if (inp == NULL) {
             // can't create inpcb and tcpcb
             result = -1;
             // printf("TCP: Cannot attach\n");
         } else {
             tp = intotcpcb(inp);

             tp->t_template = (struct tcpiphdr *) tcp_template(tp);
             if (tp->t_template == 0) {
                 // can't allocate template
                 MEM_free(tp);
                 inp->inp_ppcb = 0;
                 in_pcbdetach(inp);
                 result = -1;
                 // printf("TCP: Cannot create template\n");
             }
         }
    }

    if (result == -1) {

        // report failure to application and stop processing this message
        msg = MESSAGE_Alloc(node, APP_LAYER,
                             app_type, MSG_APP_FromTransOpenResult);
        MESSAGE_InfoAlloc(node, msg, sizeof(TransportToAppOpenResult));
        tcpOpenResult = (TransportToAppOpenResult *) MESSAGE_ReturnInfo(msg);
        tcpOpenResult->type = TCP_CONN_ACTIVE_OPEN;
        tcpOpenResult->localAddr = inp->inp_local_addr;
        tcpOpenResult->localPort = inp->inp_local_port;
        tcpOpenResult->remoteAddr = inp->inp_remote_addr;
        tcpOpenResult->remotePort = inp->inp_remote_port;
        tcpOpenResult->connectionId = result;

        tcpOpenResult->uniqueId = inp->unique_id;

        MESSAGE_Send(node, msg, TRANSPORT_DELAY);

        return;
    }

    //
    // Set receiving window scale.
    // Enter SYN_SENT state.
    // Start keepalive timer and seed output sequence space.
    // Send initial segment on connection.
    //
    if (!TCP_VARIANT_IS_RENO(tp)) {
        while ((tp->request_r_scale < TCP_MAX_WINSHIFT) &&
            ((UInt32) (TCP_MAXWIN << tp->request_r_scale) <
            inp->inp_rcv_hiwat))
                tp->request_r_scale++;
    }

    tp->outgoingInterface = outgoingInterface;

    tp->t_state = TCPS_SYN_SENT;
    // tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
    tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT_SHORT;

    if (tcp_stat)
        tcp_stat->tcps_connattempt++;

    tp->iss = *tcp_iss;
    *tcp_iss += TCP_ISSINCR/2;
    tcp_sendseqinit(tp);
    tcp_output(node, tp, tcp_now, tcp_stat);

    // If the variant is Sack, reset the value to the default LITE.
    // If the peer responds with a Sack permitted option,
    //      the Sack variant will be set correctly

    if (TCP_VARIANT_IS_SACK(tp)) {
        tp->tcpVariant = TCP_VARIANT_LITE;
    }

    inp->usrreq = INPCB_USRREQ_OPEN;
}

#undef TCPTV_KEEP_INIT_SHORT

//-------------------------------------------------------------------------//
// Attach TCP protocol, allocating
// internet protocol control block, tcp control block,
// buffer space, initialize connection state to CLOSED.
//-------------------------------------------------------------------------//
struct inpcb *tcp_attach(
    Node *node,
    struct inpcb *head,
    AppType app_type,
    Address* local_addr,
    short local_port,
    Address* remote_addr,
    short remote_port,
    Int32 unique_id,
    TosType priority)
{
    struct inpcb *inp;
    struct tcpcb *tp;
    TransportDataTcp *tcpLayer = (TransportDataTcp *)
                                 node->transportData.tcp;

    // Create inpcb and fill in connection information
    if (Address_IsAnyAddress(remote_addr) && (remote_port == -1)) {

        // pcb for listening port
        inp = in_pcballoc(head, 0, 0);
    } else {

        // pcb for connections
        inp = in_pcballoc(head, tcpLayer->tcpSendBuffer,
                                tcpLayer->tcpRecvBuffer);
    }

    if (inp == NULL)
        return (inp);

    inp->app_proto_type = app_type;
    inp->inp_local_addr = *local_addr;
    inp->inp_local_port = local_port;
    inp->inp_remote_addr = *remote_addr;
    inp->inp_remote_port = remote_port;
    inp->con_id = get_conid(head);

    inp->unique_id = unique_id;
    inp->priority = priority;

    // create a tcpcb
    tp = tcp_newtcpcb(node, inp);

    if (tp == 0){
        in_pcbdetach(inp);
        return (NULL);
    }
    tp->t_state = TCPS_CLOSED;
    return (inp);
}


//-------------------------------------------------------------------------//
// User issued close, and wish to trail through shutdown states:
// if never received SYN, just forget it.  If got a SYN from peer,
// but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
// If already got a FIN from peer, then almost done; go to LAST_ACK
// state.  In all other cases, have already sent FIN to peer,
// and just have to play tedious game waiting
// for peer to send FIN or not respond to keep-alives, etc.
// We can let the user exit from the close as soon as the FIN is acked.
//-------------------------------------------------------------------------//
static
struct tcpcb * tcp_usrclosed(
    Node *node,
    struct tcpcb *tp,
    struct tcpstat *tcp_stat)
{
    switch (tp->t_state) {

    case TCPS_CLOSED:
    case TCPS_LISTEN:
    case TCPS_SYN_SENT:
            tp->t_state = TCPS_CLOSED;
            // printf("TCP: Connection closed by user\n");
            tp = tcp_close(node, tp, tcp_stat);
            break;

    case TCPS_SYN_RECEIVED:
            tp->t_flags |= TF_NEEDFIN;

    case TCPS_ESTABLISHED:
            tp->t_state = TCPS_FIN_WAIT_1;
            break;

    case TCPS_CLOSE_WAIT:
            tp->t_state = TCPS_LAST_ACK;
            break;
    }
    if (tp && tp->t_state >= TCPS_FIN_WAIT_2) {
        // To prevent the connection hanging in FIN_WAIT_2 forever.
        if (tp->t_state == TCPS_FIN_WAIT_2)
            tp->t_timer[TCPT_2MSL] = TCPTV_MAXIDLE;
    }
    return (tp);
}


//-------------------------------------------------------------------------//
// Initiate (or continue) disconnect.
// If embryonic state, just send reset (once).
// Otherwise, switch states based on user close, and
// send segment to peer (with FIN).
//-------------------------------------------------------------------------//
void tcp_disconnect(
    Node *node,
    struct inpcb *phead,
    int conn_id,
    UInt32 tcp_now,
    struct tcpstat *tcp_stat)
{
    struct inpcb *inp;
    struct tcpcb *tp;

    inp  = in_pcbsearch(phead, conn_id);
    if (inp == phead) {

        // can't find the in_pcb of the connection
        ERROR_ReportWarningArgs("TCP: can't find (id=%d,conn=%d)\n",
                                node->nodeId,
                                conn_id);
        return;
    }

    inp->usrreq = INPCB_USRREQ_CLOSE;

    tp = intotcpcb(inp);
    if (tp->t_state < TCPS_ESTABLISHED)
        tp = tcp_close(node, tp, tcp_stat);
    else {
        tp = tcp_usrclosed(node, tp, tcp_stat);
        if (tp)
            tcp_output(node, tp, tcp_now, tcp_stat);

    }
}
