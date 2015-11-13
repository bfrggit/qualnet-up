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
// This file contains TCP timer management routines.
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
//  @(#)tcp_timer.c 8.2 (Berkeley) 5/24/95
//

#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "network_ip.h"
#include "transport_tcp.h"
#include "transport_tcp_seq.h"
#include "transport_tcp_timer.h"
#include "transport_tcp_var.h"
#include "transport_tcp_proto.h"


//-------------------------------------------------------------------------//
// TCP timer processing.
//-------------------------------------------------------------------------//
static
struct tcpcb * tcp_timers(
    Node *node,
    struct tcpcb *tp,
    int timer,
    UInt32 tcp_now,
    struct tcpstat *tcp_stat)
{
    int rexmt;
    TransportDataTcp *tcpLayer = (TransportDataTcp *)
                                 node->transportData.tcp;

    switch (timer) {

    //
    // 2 MSL timeout in shutdown went off.  If we're closed but
    // still waiting for peer to close and connection has been idle
    // too long, or if 2MSL time is up from TIME_WAIT, delete connection
    // control block.  Otherwise, check again in a bit.
    //
    case TCPT_2MSL:
        if (tp->t_state != TCPS_TIME_WAIT &&
            tp->t_idle <= TCPTV_MAXIDLE)
            tp->t_timer[TCPT_2MSL] = TCPTV_KEEPINTVL;
        else {
        	// printf("TCP: Connection closed by timer\n");
            tp = tcp_close(node, tp, tcp_stat);
        }
        break;

    //
    // Retransmission timer went off.  Message has not
    // been acked within retransmit interval.  Back off
    // to a longer retransmit interval and retransmit one segment.
    //
    case TCPT_REXMT:
        if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
            tp->t_rxtshift = TCP_MAXRXTSHIFT;
            //if (tcp_stat)
                //tcp_stat->tcps_timeoutdrop++;
            printf("TCP: Retransmission timer went off\n");
            tp = tcp_drop(node, tp, tcp_now, tcp_stat);
            break;
        }
        //if (tcp_stat)
            //tcp_stat->tcps_rexmttimeo++;
        rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];

        TCPT_RANGESET(tp->t_rxtcur, rexmt,
                      tp->t_rttmin, TCPTV_REXMTMAX);
        tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;

        //
        // If we backed off this far,
        // our srtt estimate is probably bogus.  Clobber it
        // so we'll take the next rtt measurement as our srtt;
        // move the current srtt into rttvar to keep the current
        // retransmit times until then.
        //
        if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {

            tp->t_rttvar +=
                (tp->t_srtt >> (TCP_RTT_SHIFT - TCP_DELTA_SHIFT));

            tp->t_srtt = 0;
        }
        tp->snd_nxt = tp->snd_una;

        if (TCP_VARIANT_IS_SACK(tp) && tp->isSackFastRextOn) {
            TransportTcpSackRextTimeoutInit(tp);
            TransportTcpTrace(node, 0, 0, "Faxt: timeout");
        }

        // Force a segment to be sent.
        tp->t_flags |= TF_ACKNOW;

        // If timing a segment in this window, stop the timer.
        // The retransmitted segment shouldn't be timed.
        tp->t_rtt = 0;

        //
        // Close the congestion window down to one segment
        // (we'll open it by one segment for each ack we get).
        // Since we probably have a window's worth of unacked
        // data accumulated, this "slow start" keeps us from
        // dumping all that data as back-to-back packets (which
        // might overwhelm an intermediate gateway).
        //
        // There are two phases to the opening: Initially we
        // open by one mss on each ack.  This makes the window
        // size increase exponentially with time.  If the
        // window is larger than the path can handle, this
        // exponential growth results in dropped packet(s)
        // almost immediately.  To get more time between
        // drops but still "push" the network to take advantage
        // of improving conditions, we switch from exponential
        // to linear window opening at some threshhold size.
        // For a threshhold, we use half the current window
        // size, truncated to a multiple of the mss.
        //
        // (the minimum cwnd that will give us exponential
        // growth is 2 mss.  We don't allow the threshhold
        // to go below this.)
        //
        {
            unsigned int win;
            win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
            if (win < 2)
                win = 2;
            tp->snd_cwnd = tp->t_maxseg;

            tp->snd_ssthresh = win * tp->t_maxseg;
            tp->t_partialacks = -1;
            tp->t_dupacks = 0;
        }
        tp->t_ecnFlags |= TF_CWND_REDUCED;
        TransportTcpTrace(node, 0, 0, "Rext: timeout");

        //
        // To eliminates the problem of multiple Fast Retransmits we uses this
        // new variable "send_high", whose initial value is the initial send
        // sequence number. After each retransmit timeout, the highest sequence
        // numbers transmitted so far is recorded in the variable "send_high".
        //
        if (TCP_VARIANT_IS_NEWRENO(tp)) {
            tp->send_high = tp->snd_max;
        }

        tcp_output(node, tp, tcp_now, tcp_stat);
        break;

    //
    // Persistance timer into zero window.
    // Force a byte to be output, if possible.
    //
    case TCPT_PERSIST:
        //if (tcp_stat)
            //tcp_stat->tcps_persisttimeo++;
        //
        // Hack: if the peer is dead/unreachable, we do not
        // time out if the window is closed.  After a full
        // backoff, drop the connection if the idle time
        // (no responses to probes) reaches the maximum
        // backoff that we would use if retransmitting.
        //
        if (tp->t_rxtshift == TCP_MAXRXTSHIFT) {
            UInt32 maxidle = TCP_REXMTVAL(tp);
            if (maxidle < tp->t_rttmin)
                maxidle = tp->t_rttmin;
            maxidle *= tcp_totbackoff;
            if (tp->t_idle >= TCPTV_KEEP_IDLE ||
                tp->t_idle >= maxidle) {
                //if (tcp_stat)
                    //tcp_stat->tcps_persistdrop++;
            	printf("TCP: Idle timer went off\n");
                tp = tcp_drop(node, tp, tcp_now, tcp_stat);
                break;
            }
        }
        tcp_setpersist(tp);
        tp->t_force = 1;
        tcp_output(node, tp, tcp_now, tcp_stat);
        tp->t_force = 0;
        break;

    //
    // Keep-alive timer went off; send something
    // or drop connection if idle for too long.
    //
    case TCPT_KEEP:
        //if (tcp_stat)
            //tcp_stat->tcps_keeptimeo++;
        if (tp->t_state < TCPS_ESTABLISHED)
        	printf("TCP: Keep-alive timer went off before established\n");
            goto dropit;
        if (tcpLayer->tcpUseKeepAliveProbes && tp->t_state <= TCPS_CLOSING) {

            //
            // If the connection has been idle for more than the sum of
            // TCPTV_KEEP_IDLE (set to 2 hours) and TCPTV_MAXIDLE
            // (set to the total time taken to send all the probes),
            // it's time to drop the connection.
            //
            if (tp->t_idle >= TCPTV_KEEP_IDLE + TCPTV_MAXIDLE)
            	printf("TCP: Keep-alive timer went off\n");
                goto dropit;

            //
            // Send a packet designed to force a response
            // if the peer is up and reachable:
            // either an ACK if the connection is still alive,
            // or an RST if the peer has closed the connection
            // due to timeout or reboot.
            // Using sequence number tp->snd_una-1
            // causes the transmitted zero-length segment
            // to lie outside the receive window;
            // by the protocol spec, this requires the
            // correspondent TCP to respond.
            //
            //if (tcp_stat)
                //tcp_stat->tcps_keepprobe++;
            tcp_respond(node, tp, tp->t_template,
                        0, tp->rcv_nxt, tp->snd_una - 1,
                        0, tcp_stat);

            tp->t_timer[TCPT_KEEP] = TCPTV_KEEPINTVL;
        } else {
            //
            // If the tcpUseKeepAliveProbes is FALSE
            // or the connection state is greater than TCPS_CLOSING,
            // reset the keepalive timer to TCPTV_KEEP_IDLE.
            //
            tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_IDLE;
        }
        break;
    dropit:
        //if (tcp_stat) {
            //
            // Note that this counter counts connection drops due to
            // failure in connection establishment and the keepalive
            // timer timeouts
            //
            //tcp_stat->tcps_keepdrops++;
        //}
	    // printf("TCP: Unknown timer went off\n");
        tp = tcp_drop(node, tp, tcp_now, tcp_stat);
        break;
    }
    return (tp);
}


//-------------------------------------------------------------------------//
// Fast timeout routine for processing delayed acks
//-------------------------------------------------------------------------//
void tcp_fasttimo(
    Node *node,
    struct inpcb *head,
    UInt32 tcp_now,
    struct tcpstat *tcp_stat)
{
    struct inpcb *inp;
    struct tcpcb *tp;

    inp = head->inp_next;
    if (inp) {
        for (; inp != head; inp = inp->inp_next) {
            tp = (struct tcpcb *) inp->inp_ppcb;
            if (tp &&(tp->t_flags & TF_DELACK)) {
                tp->t_flags &= ~TF_DELACK;
                tp->t_flags |= TF_ACKNOW;
                //if (tcp_stat)
                    //tcp_stat->tcps_delack++;
                tcp_output(node, tp, tcp_now, tcp_stat);
            }
        }
    }
}


//-------------------------------------------------------------------------//
// Tcp protocol timeout routine called every 500 ms.
// Updates the timers in all active tcb's and
// causes finite state machine actions if timers expire.
//-------------------------------------------------------------------------//
void tcp_slowtimo(
    Node *node,
    struct inpcb *head,
    tcp_seq *tcp_iss,
    UInt32 *tcp_now,
    struct tcpstat *tcp_stat)
{
    struct inpcb *ip, *ipnxt;
    struct tcpcb *tp;
    int i;

    ip = head->inp_next;
    if (ip == NULL) {
        return;
    }

    // Search through inpcb's and update active timers.
    for (; ip != head; ip = ipnxt) {
        ipnxt = ip->inp_next;
        tp = intotcpcb(ip);
        if (tp == 0 || tp->t_state == TCPS_LISTEN)
            continue;
        for (i = 0; i < TCPT_NTIMERS; i++) {
            if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
                (void) tcp_timers(node, tp, i,
                                  *tcp_now, tcp_stat);
                if (ipnxt->inp_prev != ip)
                    goto tpgone;
            }
        }
        tp->t_idle++;
        tp->t_duration++;
        if (tp->t_rtt)
            tp->t_rtt++;
tpgone:
        ;
    }
    *tcp_iss += TCP_ISSINCR/PR_SLOWHZ;      // increment iss for timestamps
    (*tcp_now) ++;
}


//-------------------------------------------------------------------------//
// Cancel all timers for TCP tp.
//-------------------------------------------------------------------------//
void tcp_canceltimers(struct tcpcb *tp)
{
    int i;

    for (i = 0; i < TCPT_NTIMERS; i++)
    {
        tp->t_timer[i] = 0;
    }
}
