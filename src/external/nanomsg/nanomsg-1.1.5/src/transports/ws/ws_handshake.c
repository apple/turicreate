/*
    Copyright (c) 2013 250bpm s.r.o.  All rights reserved.
    Copyright (c) 2014-2016 Jack R. Dunaway.  All rights reserved.
    Copyright 2017 Garrett D'Amore <garrett@damore.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "ws_handshake.h"
#include "sha1.h"

#include "../../aio/timer.h"

#include "../../core/sock.h"

#include "../utils/base64.h"

#include "../../utils/alloc.h"
#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/wire.h"
#include "../../utils/attr.h"
#include "../../utils/random.h"
#include "../../utils/strcasestr.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>

#define	CRLF	"\r\n"

/*****************************************************************************/
/***  BEGIN undesirable dependency *******************************************/
/*****************************************************************************/
/*  TODO: A transport should be SP agnostic; alas, these includes are        */
/*        required for the map. Ideally, this map would live in another      */
/*        abstraction layer; perhaps a "registry" of Scalability Protocols?  */
/*****************************************************************************/
#include "../../pair.h"
#include "../../reqrep.h"
#include "../../pubsub.h"
#include "../../survey.h"
#include "../../pipeline.h"
#include "../../bus.h"

static const struct nn_ws_sp_map NN_WS_HANDSHAKE_SP_MAP[] = {
    { NN_PAIR,       NN_PAIR,       "pair.sp.nanomsg.org" },
    { NN_REQ,        NN_REP,        "req.sp.nanomsg.org" },
    { NN_REP,        NN_REQ,        "rep.sp.nanomsg.org" },
    { NN_PUB,        NN_SUB,        "pub.sp.nanomsg.org" },
    { NN_SUB,        NN_PUB,        "sub.sp.nanomsg.org" },
    { NN_SURVEYOR,   NN_RESPONDENT, "surveyor.sp.nanomsg.org" },
    { NN_RESPONDENT, NN_SURVEYOR,   "respondent.sp.nanomsg.org" },
    { NN_PUSH,       NN_PULL,       "push.sp.nanomsg.org" },
    { NN_PULL,       NN_PUSH,       "pull.sp.nanomsg.org" },
    { NN_BUS,        NN_BUS,        "bus.sp.nanomsg.org" }
};

const size_t NN_WS_HANDSHAKE_SP_MAP_LEN = sizeof (NN_WS_HANDSHAKE_SP_MAP) /
    sizeof (NN_WS_HANDSHAKE_SP_MAP [0]);
/*****************************************************************************/
/***  END undesirable dependency *********************************************/
/*****************************************************************************/

/*  State machine finite states. */
#define NN_WS_HANDSHAKE_STATE_IDLE 1
#define NN_WS_HANDSHAKE_STATE_SERVER_RECV 2
#define NN_WS_HANDSHAKE_STATE_SERVER_REPLY 3
#define NN_WS_HANDSHAKE_STATE_CLIENT_SEND 4
#define NN_WS_HANDSHAKE_STATE_CLIENT_RECV 5
#define NN_WS_HANDSHAKE_STATE_HANDSHAKE_SENT 6
#define NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR 7
#define NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_DONE 8
#define NN_WS_HANDSHAKE_STATE_DONE 9
#define NN_WS_HANDSHAKE_STATE_STOPPING 10

/*  Subordinate srcptr objects. */
#define NN_WS_HANDSHAKE_SRC_USOCK 1
#define NN_WS_HANDSHAKE_SRC_TIMER 2

/*  Time allowed to complete handshake. */
#define NN_WS_HANDSHAKE_TIMEOUT 5000

/*  Possible return codes internal to the parsing operations. */
#define NN_WS_HANDSHAKE_NOMATCH 0
#define NN_WS_HANDSHAKE_MATCH 1

/*  Possible return codes from parsing opening handshake from peer. */
#define NN_WS_HANDSHAKE_VALID 0
#define NN_WS_HANDSHAKE_RECV_MORE 1
#define NN_WS_HANDSHAKE_INVALID -1

/*  Possible handshake responses to send to client when acting as server. */
#define NN_WS_HANDSHAKE_RESPONSE_NULL -1
#define NN_WS_HANDSHAKE_RESPONSE_OK 0
#define NN_WS_HANDSHAKE_RESPONSE_TOO_BIG 1
#define NN_WS_HANDSHAKE_RESPONSE_UNUSED2 2
#define NN_WS_HANDSHAKE_RESPONSE_WSPROTO 3
#define NN_WS_HANDSHAKE_RESPONSE_WSVERSION 4
#define NN_WS_HANDSHAKE_RESPONSE_NNPROTO 5
#define NN_WS_HANDSHAKE_RESPONSE_NOTPEER 6
#define NN_WS_HANDSHAKE_RESPONSE_UNKNOWNTYPE 7

/*  Private functions. */
static void nn_ws_handshake_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_ws_handshake_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_ws_handshake_leave (struct nn_ws_handshake *self, int rc);

/*  WebSocket protocol support functions. */
static int nn_ws_handshake_parse_client_opening (struct nn_ws_handshake *self);
static void nn_ws_handshake_server_reply (struct nn_ws_handshake *self);
static void nn_ws_handshake_client_request (struct nn_ws_handshake *self);
static int nn_ws_handshake_parse_server_response (struct nn_ws_handshake *self);
static int nn_ws_handshake_hash_key (const char *key, size_t key_len,
    char *hashed, size_t hashed_len);

/*  String parsing support functions. */

/*  Scans for reference token against subject string, optionally ignoring
    case sensitivity and/or leading spaces in subject. On match, advances
    the subject pointer to the next non-ignored character past match. Both
    strings must be NULL terminated to avoid undefined behavior. Returns
    NN_WS_HANDSHAKE_MATCH on match; else, NN_WS_HANDSHAKE_NOMATCH. */
static int nn_ws_match_token (const char* token, const char **subj,
    int case_insensitive, int ignore_leading_sp);

/*  Scans subject string for termination sequence, optionally ignoring
    leading and/or trailing spaces in subject. On match, advances
    the subject pointer to the next character past match. Both
    strings must be NULL terminated to avoid undefined behavior. If the
    match succeeds, values are stored into *addr and *len. */
static int nn_ws_match_value (const char* termseq, const char **subj,
    int ignore_leading_sp, int ignore_trailing_sp, const char **addr,
    size_t* const len);

/*  Compares subject octet stream to expected value, optionally ignoring
    case sensitivity. Returns non-zero on success, zero on failure. */
static int nn_ws_validate_value (const char* expected, const char *subj,
    size_t subj_len, int case_insensitive);

void nn_ws_handshake_init (struct nn_ws_handshake *self, int src,
    struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_ws_handshake_handler, nn_ws_handshake_shutdown,
        src, self, owner);
    self->state = NN_WS_HANDSHAKE_STATE_IDLE;
    nn_timer_init (&self->timer, NN_WS_HANDSHAKE_SRC_TIMER, &self->fsm);
    nn_fsm_event_init (&self->done);
    self->timeout = NN_WS_HANDSHAKE_TIMEOUT;
    self->usock = NULL;
    self->usock_owner.src = -1;
    self->usock_owner.fsm = NULL;
    self->pipebase = NULL;
}

void nn_ws_handshake_term (struct nn_ws_handshake *self)
{
    nn_assert_state (self, NN_WS_HANDSHAKE_STATE_IDLE);

    nn_fsm_event_term (&self->done);
    nn_timer_term (&self->timer);
    nn_fsm_term (&self->fsm);
}

int nn_ws_handshake_isidle (struct nn_ws_handshake *self)
{
    return nn_fsm_isidle (&self->fsm);
}

void nn_ws_handshake_start (struct nn_ws_handshake *self,
    struct nn_usock *usock, struct nn_pipebase *pipebase,
    int mode, const char *resource, const char *host)
{
    /*  It's expected this resource has been allocated during intial connect. */
    if (mode == NN_WS_CLIENT)
        nn_assert (strlen (resource) >= 1);

    /*  Take ownership of the underlying socket. */
    nn_assert (self->usock == NULL && self->usock_owner.fsm == NULL);
    self->usock_owner.src = NN_WS_HANDSHAKE_SRC_USOCK;
    self->usock_owner.fsm = &self->fsm;
    nn_usock_swap_owner (usock, &self->usock_owner);
    self->usock = usock;
    self->pipebase = pipebase;
    self->mode = mode;
    self->resource = resource;
    self->remote_host = host;

    memset (self->opening_hs, 0, sizeof (self->opening_hs));
    memset (self->response, 0, sizeof (self->response));

    self->recv_pos = 0;
    self->retries = 0;

    /*  Calculate the absolute minimum length possible for a valid opening
        handshake. This is an optimization since we must poll for the
        remainder of the opening handshake in small byte chunks. */
    switch (self->mode) {
    case NN_WS_SERVER:
        self->recv_len = strlen (
            "GET x HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Host: x\r\n"
            "Origin: x\r\n"
            "Sec-WebSocket-Key: xxxxxxxxxxxxxxxxxxxxxxxx\r\n"
            "Sec-WebSocket-Version: xx\r\n\r\n");
        break;
    case NN_WS_CLIENT:
        /*  Shortest conceiveable response from server is a terse status. */
        self->recv_len = strlen ("HTTP/1.1 xxx\r\n\r\n");
        break;
    default:
        /*  Developer error; unexpected mode. */
        nn_assert (0);
        break;
    }

    /*  Launch the state machine. */
    nn_fsm_start (&self->fsm);
}

void nn_ws_handshake_stop (struct nn_ws_handshake *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_ws_handshake_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_ws_handshake *handshaker;

    handshaker = nn_cont (self, struct nn_ws_handshake, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        nn_timer_stop (&handshaker->timer);
        handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING;
    }
    if (nn_slow (handshaker->state == NN_WS_HANDSHAKE_STATE_STOPPING)) {
        if (!nn_timer_isidle (&handshaker->timer))
            return;
        handshaker->state = NN_WS_HANDSHAKE_STATE_IDLE;
        nn_fsm_stopped (&handshaker->fsm, NN_WS_HANDSHAKE_STOPPED);
        return;
    }

    nn_fsm_bad_state (handshaker->state, src, type);
}

static int nn_ws_match_token (const char* token, const char **subj,
    int case_insensitive, int ignore_leading_sp)
{
    const char *pos;

    nn_assert (token && *subj);

    pos = *subj;

    if (ignore_leading_sp) {
        while (*pos == '\x20' && *pos) {
            pos++;
        }
    }

    if (case_insensitive) {
        while (*token && *pos) {
            if (tolower (*token) != tolower (*pos))
                return NN_WS_HANDSHAKE_NOMATCH;
            token++;
            pos++;
        }
    }
    else {
        while (*token && *pos) {
            if (*token != *pos)
                return NN_WS_HANDSHAKE_NOMATCH;
            token++;
            pos++;
        }
    }

    /*  Encountered end of subject before matching completed. */
    if (!*pos && *token)
        return NN_WS_HANDSHAKE_NOMATCH;

    /*  Entire token has been matched. */
    nn_assert (!*token);

    /*  On success, advance subject position. */
    *subj = pos;

    return NN_WS_HANDSHAKE_MATCH;
}

static int nn_ws_match_value (const char* termseq, const char **subj,
    int ignore_leading_sp, int ignore_trailing_sp, const char **addr,
    size_t* const len)
{
    const char *start;
    const char *end;

    nn_assert (termseq && *subj);

    start = *subj;
    if (addr)
        *addr = NULL;
    if (len)
        *len = 0;

    /*  Find first occurence of termination sequence. */
    end = strstr (start, termseq);

    /*  Was a termination sequence found? */
    if (end) {
        *subj = end + strlen (termseq);
    }
    else {
        return NN_WS_HANDSHAKE_NOMATCH;
    }
        
    if (ignore_leading_sp) {
        while (*start == '\x20' && start < end) {
            start++;
        }
    }

    if (addr)
        *addr = start;

    /*  In this special case, the value was "found", but is just empty or
        ignored space. */
    if (start == end)
        return NN_WS_HANDSHAKE_MATCH;

    if (ignore_trailing_sp) {
        while (*(end - 1) == '\x20' && start < end) {
            end--;
        }
    }

    if (len)
        *len = end - start;

    return NN_WS_HANDSHAKE_MATCH;
}

static int nn_ws_validate_value (const char* expected, const char *subj,
    size_t subj_len, int case_insensitive)
{
    if (strlen (expected) != subj_len)
        return NN_WS_HANDSHAKE_NOMATCH;

    if (case_insensitive) {
        while (*expected && *subj) {
            if (tolower (*expected) != tolower (*subj))
                return NN_WS_HANDSHAKE_NOMATCH;
            expected++;
            subj++;
        }
    }
    else {
        while (*expected && *subj) {
            if (*expected != *subj)
                return NN_WS_HANDSHAKE_NOMATCH;
            expected++;
            subj++;
        }
    }

    return NN_WS_HANDSHAKE_MATCH;
}

static void nn_ws_handshake_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_ws_handshake *handshaker;

    size_t i;

    handshaker = nn_cont (self, struct nn_ws_handshake, fsm);

    switch (handshaker->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_assert (handshaker->recv_pos == 0);
                nn_assert (handshaker->recv_len >= NN_WS_HANDSHAKE_TERMSEQ_LEN);

                nn_timer_start (&handshaker->timer, handshaker->timeout);

                switch (handshaker->mode) {
                case NN_WS_CLIENT:
                    /*  Send opening handshake to server. */
                    nn_assert (handshaker->recv_len <=
                        sizeof (handshaker->response));
                    handshaker->state = NN_WS_HANDSHAKE_STATE_CLIENT_SEND;
                    nn_ws_handshake_client_request (handshaker);
                    return;
                case NN_WS_SERVER:
                    /*  Begin receiving opening handshake from client. */
                    nn_assert (handshaker->recv_len <=
                        sizeof (handshaker->opening_hs));
                    handshaker->state = NN_WS_HANDSHAKE_STATE_SERVER_RECV;
                    nn_usock_recv (handshaker->usock, handshaker->opening_hs,
                        handshaker->recv_len, NULL);
                    return;
                default:
                    /*  Unexpected mode. */
                    nn_assert (0);
                    return;
                }

            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  SERVER_RECV state.                                                        */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_SERVER_RECV:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            switch (type) {
            case NN_USOCK_RECEIVED:
                /*  Parse bytes received thus far. */
                switch (nn_ws_handshake_parse_client_opening (handshaker)) {
                case NN_WS_HANDSHAKE_INVALID:
                    /*  Opening handshake parsed successfully but does not
                        contain valid values. Respond failure to client. */
                    handshaker->state = NN_WS_HANDSHAKE_STATE_SERVER_REPLY;
                    nn_ws_handshake_server_reply (handshaker);
                    return;
                case NN_WS_HANDSHAKE_VALID:
                    /*  Opening handshake parsed successfully, and is valid.
                        Respond success to client. */
                    handshaker->state = NN_WS_HANDSHAKE_STATE_SERVER_REPLY;
                    nn_ws_handshake_server_reply (handshaker);
                    return;
                case NN_WS_HANDSHAKE_RECV_MORE:
                    /*  Not enough bytes have been received to determine
                        validity; remain in the receive state, and retrieve
                        more bytes from client. */
                    handshaker->recv_pos += handshaker->recv_len;

                    /*  Validate the previous recv operation. */
                    nn_assert (handshaker->recv_pos <
                        sizeof (handshaker->opening_hs));

                    /*  Ensure we can back-track at least the length of the
                        termination sequence to determine how many bytes to
                        receive on the next retry. This is an assertion, not
                        a conditional, since under no condition is it
                        necessary to initially receive so few bytes. */
                    nn_assert (handshaker->recv_pos >=
                        (int) NN_WS_HANDSHAKE_TERMSEQ_LEN);

                    /*  We only compare if we have at least one byte to
                        compare against.  When i drops to zero, it means
                        we don't have any bytes to match against, and it is
                        automatically true. */
                    for (i = NN_WS_HANDSHAKE_TERMSEQ_LEN; i > 0; i--) {
                        if (memcmp (NN_WS_HANDSHAKE_TERMSEQ,
                            handshaker->opening_hs + handshaker->recv_pos - i,
                            i) == 0) {
                            break;
                        }
                    }

                    nn_assert (i < NN_WS_HANDSHAKE_TERMSEQ_LEN);

                    handshaker->recv_len = NN_WS_HANDSHAKE_TERMSEQ_LEN - i;

                    /*  In the unlikely case the client would overflow what we
                        assumed was a sufficiently-large buffer to receive the
                        handshake, we fail the client. */
                    if (handshaker->recv_len + handshaker->recv_pos >
                        sizeof (handshaker->opening_hs)) {
                        handshaker->response_code =
                            NN_WS_HANDSHAKE_RESPONSE_TOO_BIG;
                        handshaker->state =
                            NN_WS_HANDSHAKE_STATE_SERVER_REPLY;
                        nn_ws_handshake_server_reply (handshaker);
                    }
                    else {
                        handshaker->retries++;
                        nn_usock_recv (handshaker->usock,
                            handshaker->opening_hs + handshaker->recv_pos,
                            handshaker->recv_len, NULL);
                    }
                    return;
                default:
                    nn_fsm_error ("Unexpected handshake result",
                        handshaker->state, src, type);
                }
                return;
            case NN_USOCK_SHUTDOWN:
                /*  Ignore it and wait for ERROR event. */
                return;
            case NN_USOCK_ERROR:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  SERVER_REPLY state.                                                       */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_SERVER_REPLY:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SENT:
                /*  As per RFC 6455 4.2.2, the handshake is now complete
                    and the connection is immediately ready for send/recv. */
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_DONE;
            case NN_USOCK_SHUTDOWN:
                /*  Ignore it and wait for ERROR event. */
                return;
            case NN_USOCK_ERROR:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  CLIENT_SEND state.                                                        */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_CLIENT_SEND:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SENT:
                handshaker->state = NN_WS_HANDSHAKE_STATE_CLIENT_RECV;
                nn_usock_recv (handshaker->usock, handshaker->response,
                    handshaker->recv_len, NULL);
                return;
            case NN_USOCK_SHUTDOWN:
                /*  Ignore it and wait for ERROR event. */
                return;
            case NN_USOCK_ERROR:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  CLIENT_RECV state.                                                        */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_CLIENT_RECV:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            switch (type) {
            case NN_USOCK_RECEIVED:
                /*  Parse bytes received thus far. */
                switch (nn_ws_handshake_parse_server_response (handshaker)) {
                case NN_WS_HANDSHAKE_INVALID:
                    /*  Opening handshake parsed successfully but does not
                        contain valid values. Fail connection. */
                        nn_timer_stop (&handshaker->timer);
                        handshaker->state =
                            NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                    return;
                case NN_WS_HANDSHAKE_VALID:
                    /*  As per RFC 6455 4.2.2, the handshake is now complete
                        and the connection is immediately ready for send/recv. */
                    nn_timer_stop (&handshaker->timer);
                    handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_DONE;
                    return;
                case NN_WS_HANDSHAKE_RECV_MORE:
                    /*  Not enough bytes have been received to determine
                        validity; remain in the receive state, and retrieve
                        more bytes from client. */
                    handshaker->recv_pos += handshaker->recv_len;

                    /*  Validate the previous recv operation. */
                    nn_assert (handshaker->recv_pos <
                        sizeof (handshaker->response));

                    /*  Ensure we can back-track at least the length of the
                        termination sequence to determine how many bytes to
                        receive on the next retry. This is an assertion, not
                        a conditional, since under no condition is it
                        necessary to initially receive so few bytes. */
                    nn_assert (handshaker->recv_pos >=
                        (int) NN_WS_HANDSHAKE_TERMSEQ_LEN);

                    /*  If i goes to 0, it no need to compare. */
                    for (i = NN_WS_HANDSHAKE_TERMSEQ_LEN; i > 0; i--) {
                        if (memcmp (NN_WS_HANDSHAKE_TERMSEQ,
                            handshaker->response + handshaker->recv_pos - i,
                            i) == 0) {
                            break;
                        }
                    }

                    nn_assert (i < NN_WS_HANDSHAKE_TERMSEQ_LEN);

                    handshaker->recv_len = NN_WS_HANDSHAKE_TERMSEQ_LEN - i;

                    /*  In the unlikely case the client would overflow what we
                        assumed was a sufficiently-large buffer to receive the
                        handshake, we fail the connection. */
                    if (handshaker->recv_len + handshaker->recv_pos >
                        sizeof (handshaker->response)) {
                        nn_timer_stop (&handshaker->timer);
                        handshaker->state =
                            NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                    }
                    else {
                        handshaker->retries++;
                        nn_usock_recv (handshaker->usock,
                            handshaker->response + handshaker->recv_pos,
                            handshaker->recv_len, NULL);
                    }
                    return;
                default:
                    nn_fsm_error ("Unexpected handshake result",
                        handshaker->state, src, type);
                }
                return;
            case NN_USOCK_SHUTDOWN:
                /*  Ignore it and wait for ERROR event. */
                return;
            case NN_USOCK_ERROR:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  HANDSHAKE_SENT state.                                                     */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_HANDSHAKE_SENT:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SENT:
                /*  As per RFC 6455 4.2.2, the handshake is now complete
                    and the connection is immediately ready for send/recv. */
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_DONE;
                return;
            case NN_USOCK_SHUTDOWN:
                /*  Ignore it and wait for ERROR event. */
                return;
            case NN_USOCK_ERROR:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&handshaker->timer);
                handshaker->state = NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR;
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_TIMER_ERROR state.                                               */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_ERROR:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            /*  Ignore. The only circumstance the client would send bytes is
                to notify the server it is closing the connection. Wait for the
                socket to eventually error. */
            return;

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_STOPPED:
                nn_ws_handshake_leave (handshaker, NN_WS_HANDSHAKE_ERROR);
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_TIMER_DONE state.                                                */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_STOPPING_TIMER_DONE:
        switch (src) {

        case NN_WS_HANDSHAKE_SRC_USOCK:
            /*  Ignore. The only circumstance the client would send bytes is
                to notify the server it is closing the connection. Wait for the
                socket to eventually error. */
            return;

        case NN_WS_HANDSHAKE_SRC_TIMER:
            switch (type) {
            case NN_TIMER_STOPPED:
                nn_ws_handshake_leave (handshaker, NN_WS_HANDSHAKE_OK);
                return;
            default:
                nn_fsm_bad_action (handshaker->state, src, type);
            }

        default:
            nn_fsm_bad_source (handshaker->state, src, type);
        }

/******************************************************************************/
/*  DONE state.                                                               */
/*  The header exchange was either done successfully of failed. There's       */
/*  nothing that can be done in this state except stopping the object.        */
/******************************************************************************/
    case NN_WS_HANDSHAKE_STATE_DONE:
        nn_fsm_bad_source (handshaker->state, src, type);

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (handshaker->state, src, type);
    }
}

/******************************************************************************/
/*  State machine actions.                                                    */
/******************************************************************************/

static void nn_ws_handshake_leave (struct nn_ws_handshake *self, int rc)
{
    nn_usock_swap_owner (self->usock, &self->usock_owner);
    self->usock = NULL;
    self->usock_owner.src = -1;
    self->usock_owner.fsm = NULL;
    self->state = NN_WS_HANDSHAKE_STATE_DONE;
    nn_fsm_raise (&self->fsm, &self->done, rc);
}

static int nn_ws_handshake_parse_client_opening (struct nn_ws_handshake *self)
{
    /*  As per RFC 6455 section 1.7, this parser is not intended to be a
        general-purpose parser for arbitrary HTTP headers. As with the design
        philosophy of nanomsg, application-specific exchanges are better
        reserved for accepted connections, not as fields within these
        headers. */

    int rc;
    const char *pos;
    unsigned i;

    /*  Guarantee that a NULL terminator exists to enable treating this
        recv buffer like a string. */
    nn_assert (memchr (self->opening_hs, '\0', sizeof (self->opening_hs)));

    /*  Having found the NULL terminator, from this point forward string
        functions may be used. */
    nn_assert (strlen (self->opening_hs) < sizeof (self->opening_hs));

    pos = self->opening_hs;

    /*  Is the opening handshake from the client fully received? */
    if (!strstr (pos, NN_WS_HANDSHAKE_TERMSEQ))
        return NN_WS_HANDSHAKE_RECV_MORE;

    self->host = NULL;
    self->origin = NULL;
    self->key = NULL;
    self->upgrade = NULL;
    self->conn = NULL;
    self->version = NULL;
    self->protocol = NULL;
    self->uri = NULL;

    self->host_len = 0;
    self->origin_len = 0;
    self->key_len = 0;
    self->upgrade_len = 0;
    self->conn_len = 0;
    self->version_len = 0;
    self->protocol_len = 0;
    self->uri_len = 0;

    /*  NB: If we got here, we already have a fully received set of
        HTTP headers.  So there is no point in asking for more if the
        headers lack what we need. */

    /*  This function, if generating a return value that triggers
        a response to the client, should replace this sentinel value
        with a proper response code. */
    self->response_code = NN_WS_HANDSHAKE_RESPONSE_NULL;

    /*  RFC 7230 3.1.1 Request Line: HTTP Method
        Note requirement of one space and case sensitivity. */
    if (!nn_ws_match_token ("GET\x20", &pos, 0, 0))
        return NN_WS_HANDSHAKE_INVALID;

    /*  RFC 7230 3.1.1 Request Line: Requested Resource. */
    if (!nn_ws_match_value ("\x20", &pos, 0, 0, &self->uri, &self->uri_len))
        return NN_WS_HANDSHAKE_INVALID;

    /*  RFC 7230 3.1.1 Request Line: HTTP version. Note case sensitivity. */
    if (!nn_ws_match_token ("HTTP/1.1", &pos, 0, 0))
        return NN_WS_HANDSHAKE_INVALID;

    if (!nn_ws_match_token (CRLF, &pos, 0, 0))
        return NN_WS_HANDSHAKE_INVALID;

    /*  It's expected the current position is now at the first
        header field. Match them one by one. */
    while (strlen (pos))
    {
        const char *conn = NULL;
        size_t conn_len = 0;
        if (nn_ws_match_token ("Host:", &pos, 1, 0)) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->host, &self->host_len);
        }
        else if (nn_ws_match_token ("Origin:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->origin, &self->origin_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Key:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->key, &self->key_len);
        }
        else if (nn_ws_match_token ("Upgrade:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->upgrade, &self->upgrade_len);
        }
        else if (nn_ws_match_token ("Connection:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {

            rc = nn_ws_match_value (CRLF, &pos, 1, 1, &conn, &conn_len);

            /*  The values here can be comma delimited, or they can be
                listed as separate Connection headers.  We only care about
                the presence of the Upgrade header, and we're willing to
                assume well-formedness.  This crummy parse may let clients
                send us a malformed header that we ought to reject, but
                we'll just cite Postel's law here if anyone asks. */
            self->conn = nn_strcasestr (conn, "upgrade");
            if (self->conn != NULL) {
                self->conn_len = strlen ("upgrade");
            }
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Version:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->version, &self->version_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Protocol:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->protocol, &self->protocol_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Extensions:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->extensions, &self->extensions_len);
        }
        else if (nn_ws_match_token (CRLF,
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            /*  Exit loop since all headers are parsed. */
            break;
        }
        else {
            /*  Skip unknown headers. */
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                NULL, NULL);
        }

        if (rc != NN_WS_HANDSHAKE_MATCH)
            return NN_WS_HANDSHAKE_INVALID;
    }

    /*  Validate the opening handshake is now fully parsed. Additionally,
        as per RFC 6455 section 4.1, the client should not send additional data
        after the opening handshake, so this assertion validates upstream recv
        logic prevented this case. */
    nn_assert (strlen (pos) == 0);

    /*  TODO: protocol expectations below this point are hard-coded here as
        an initial design decision. Perhaps in the future these values should
        be settable via compile time (or run-time socket) options? */

    /*  These header fields are required as per RFC 6455 section 4.1. */
    if (!self->host || !self->upgrade || !self->conn ||
        !self->key || !self->version) {
        self->response_code = NN_WS_HANDSHAKE_RESPONSE_WSPROTO;
        return NN_WS_HANDSHAKE_INVALID;
    }

    /*  RFC 6455 section 4.2.1.6 (version December 2011). */
    if (nn_ws_validate_value ("13", self->version,
        self->version_len, 1) != NN_WS_HANDSHAKE_MATCH) {
        self->response_code = NN_WS_HANDSHAKE_RESPONSE_WSVERSION;
        return NN_WS_HANDSHAKE_INVALID;
    }

    /*  RFC 6455 section 4.2.1.3 (version December 2011). */
    if (nn_ws_validate_value ("websocket", self->upgrade,
        self->upgrade_len, 1) != NN_WS_HANDSHAKE_MATCH) {
        self->response_code = NN_WS_HANDSHAKE_RESPONSE_WSPROTO;
        return NN_WS_HANDSHAKE_INVALID;
    }

    /*  RFC 6455 section 4.2.1.4 (version December 2011). */
    if (nn_ws_validate_value ("Upgrade", self->conn,
        self->conn_len, 1) != NN_WS_HANDSHAKE_MATCH) {
        self->response_code = NN_WS_HANDSHAKE_RESPONSE_WSPROTO;
        return NN_WS_HANDSHAKE_INVALID;
    }

    /*  At this point, client meets RFC 6455 compliance for opening handshake.
        Now it's time to check nanomsg-imposed required handshake values. */
    if (self->protocol) {
        /*  Ensure the client SP is a compatible socket type. */
        for (i = 0; i < NN_WS_HANDSHAKE_SP_MAP_LEN; i++) {
            if (nn_ws_validate_value (NN_WS_HANDSHAKE_SP_MAP [i].ws_sp,
                self->protocol, self->protocol_len, 1)) {

                if (self->pipebase->sock->socktype->protocol ==
                    NN_WS_HANDSHAKE_SP_MAP [i].server) {
                    self->response_code = NN_WS_HANDSHAKE_RESPONSE_OK;
                    return NN_WS_HANDSHAKE_VALID;
                }
                else {
                    self->response_code = NN_WS_HANDSHAKE_RESPONSE_NOTPEER;
                    return NN_WS_HANDSHAKE_INVALID;
                }
                break;
            }
        }

        self->response_code = NN_WS_HANDSHAKE_RESPONSE_UNKNOWNTYPE;
        return NN_WS_HANDSHAKE_INVALID;
    }
    else {
        /*  Be permissive and generous here, assuming that if a protocol is
            not explicitly declared, PAIR is presumed. This enables
            interoperability with non-nanomsg remote peers, nominally by
            making the local socket PAIR type. For any other local
            socket type, we expect connection to be rejected as
            incompatible if the header is not specified. */

        if (nn_pipebase_ispeer (self->pipebase, NN_PAIR)) {
            self->response_code = NN_WS_HANDSHAKE_RESPONSE_OK;
            return NN_WS_HANDSHAKE_VALID;
        }
        else {
            self->response_code = NN_WS_HANDSHAKE_RESPONSE_NOTPEER;
            return NN_WS_HANDSHAKE_INVALID;
        }
    }
}

static int nn_ws_handshake_parse_server_response (struct nn_ws_handshake *self)
{
    /*  As per RFC 6455 section 1.7, this parser is not intended to be a
        general-purpose parser for arbitrary HTTP headers. As with the design
        philosophy of nanomsg, application-specific exchanges are better
        reserved for accepted connections, not as fields within these
        headers. */

    int rc;
    const char *pos;

    /*  Guarantee that a NULL terminator exists to enable treating this
        recv buffer like a string. The lack of such would indicate a failure
        upstream to catch a buffer overflow. */
    nn_assert (memchr (self->response, '\0', sizeof (self->response)));

    /*  Having found the NULL terminator, from this point forward string
        functions may be used. */
    nn_assert (strlen (self->response) < sizeof (self->response));

    pos = self->response;

    /*  Is the response from the server fully received? */
    if (!strstr (pos, NN_WS_HANDSHAKE_TERMSEQ))
        return NN_WS_HANDSHAKE_RECV_MORE;

    self->status_code = NULL;
    self->reason_phrase = NULL;
    self->server = NULL;
    self->accept_key = NULL;
    self->upgrade = NULL;
    self->conn = NULL;
    self->version = NULL;
    self->protocol = NULL;

    self->status_code_len = 0;
    self->reason_phrase_len = 0;
    self->server_len = 0;
    self->accept_key_len = 0;
    self->upgrade_len = 0;
    self->conn_len = 0;
    self->version_len = 0;
    self->protocol_len = 0;

    /*  RFC 7230 3.1.2 Status Line: HTTP Version. */
    if (!nn_ws_match_token ("HTTP/1.1\x20", &pos, 0, 0))
        return NN_WS_HANDSHAKE_RECV_MORE;

    /*  RFC 7230 3.1.2 Status Line: Status Code. */
    if (!nn_ws_match_value ("\x20", &pos, 0, 0, &self->status_code,
        &self->status_code_len))
        return NN_WS_HANDSHAKE_RECV_MORE;

    /*  RFC 7230 3.1.2 Status Line: Reason Phrase. */
    if (!nn_ws_match_value (CRLF, &pos, 0, 0,
        &self->reason_phrase, &self->reason_phrase_len))
        return NN_WS_HANDSHAKE_RECV_MORE;

    /*  It's expected the current position is now at the first
        header field. Match them one by one. */
    while (strlen (pos))
    {
        if (nn_ws_match_token ("Server:", &pos, 1, 0)) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->server, &self->server_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Accept:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->accept_key, &self->accept_key_len);
        }
        else if (nn_ws_match_token ("Upgrade:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->upgrade, &self->upgrade_len);
        }
        else if (nn_ws_match_token ("Connection:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->conn, &self->conn_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Version-Server:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->version, &self->version_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Protocol-Server:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->protocol, &self->protocol_len);
        }
        else if (nn_ws_match_token ("Sec-WebSocket-Extensions:",
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                &self->extensions, &self->extensions_len);
        }
        else if (nn_ws_match_token (CRLF,
            &pos, 1, 0) == NN_WS_HANDSHAKE_MATCH) {
            /*  Exit loop since all headers are parsed. */
            break;
        }
        else {
            /*  Skip unknown headers. */
            rc = nn_ws_match_value (CRLF, &pos, 1, 1,
                NULL, NULL);
        }

        if (rc != NN_WS_HANDSHAKE_MATCH)
            return NN_WS_HANDSHAKE_RECV_MORE;
    }

    /*  Validate the opening handshake is now fully parsed. Additionally,
        as per RFC 6455 section 4.1, the client should not send additional data
        after the opening handshake, so this assertion validates upstream recv
        logic prevented this case. */
    nn_assert (strlen (pos) == 0);

    /*  TODO: protocol expectations below this point are hard-coded here as
        an initial design decision. Perhaps in the future these values should
        be settable via compile time (or run-time socket) options? */

    /*  These header fields are required as per RFC 6455 4.2.2. */
    if (!self->status_code || !self->upgrade || !self->conn ||
        !self->accept_key)
        return NN_WS_HANDSHAKE_INVALID;

    /*  TODO: Currently, we only handle a successful connection upgrade.
        Anything else is treated as a failed connection.
        Consider handling other scenarios like 3xx redirects. */
    if (nn_ws_validate_value ("101", self->status_code,
        self->status_code_len, 1) != NN_WS_HANDSHAKE_MATCH)
        return NN_WS_HANDSHAKE_INVALID;

    /*  RFC 6455 section 4.2.2.5.2 (version December 2011). */
    if (nn_ws_validate_value ("websocket", self->upgrade,
        self->upgrade_len, 1) != NN_WS_HANDSHAKE_MATCH)
        return NN_WS_HANDSHAKE_INVALID;

    /*  RFC 6455 section 4.2.2.5.3 (version December 2011). */
    if (nn_ws_validate_value ("Upgrade", self->conn,
        self->conn_len, 1) != NN_WS_HANDSHAKE_MATCH)
        return NN_WS_HANDSHAKE_INVALID;

    /*  RFC 6455 section 4.2.2.5.4 (version December 2011). */
    if (nn_ws_validate_value (self->expected_accept_key, self->accept_key,
        self->accept_key_len, 1) != NN_WS_HANDSHAKE_MATCH)
        return NN_WS_HANDSHAKE_INVALID;

    /*  Server response meets RFC 6455 compliance for opening handshake. */
    return NN_WS_HANDSHAKE_VALID;
}

static void nn_ws_handshake_client_request (struct nn_ws_handshake *self)
{
    struct nn_iovec open_request;
    size_t encoded_key_len;
    int rc;
    unsigned i;

    /*  Generate random 16-byte key as per RFC 6455 4.1 */
    uint8_t rand_key [16];

    /*  Known length required to base64 encode above random key plus
        string NULL terminator. */
    char encoded_key [24 + 1];

    nn_random_generate (rand_key, sizeof (rand_key));

    rc = nn_base64_encode (rand_key, sizeof (rand_key),
        encoded_key, sizeof (encoded_key));
    nn_assert (rc >=0);

    encoded_key_len = strlen (encoded_key);

    nn_assert (encoded_key_len == sizeof (encoded_key) - 1);

    /*  Pre-calculated expected Accept Key value as per
        RFC 6455 section 4.2.2.5.4 (version December 2011). */
    rc = nn_ws_handshake_hash_key (encoded_key, encoded_key_len,
        self->expected_accept_key, sizeof (self->expected_accept_key));

    nn_assert (rc == NN_WS_HANDSHAKE_ACCEPT_KEY_LEN);

    /*  Lookup SP header value. */
    for (i = 0; i < NN_WS_HANDSHAKE_SP_MAP_LEN; i++) {
        if (NN_WS_HANDSHAKE_SP_MAP [i].client ==
            self->pipebase->sock->socktype->protocol) {
            break;
        }
    }

    /*  Guarantee that the socket type was found in the map. */
    nn_assert (i < NN_WS_HANDSHAKE_SP_MAP_LEN);

    sprintf (self->opening_hs,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: %s\r\n\r\n",
        self->resource, self->remote_host, encoded_key,
        NN_WS_HANDSHAKE_SP_MAP[i].ws_sp);

    open_request.iov_len = strlen (self->opening_hs);
    open_request.iov_base = self->opening_hs;

    nn_usock_send (self->usock, &open_request, 1);
}

static void nn_ws_handshake_server_reply (struct nn_ws_handshake *self)
{
    struct nn_iovec response;
    char *code;
    char *version;
    char *protocol;
    int rc;

    /*  Allow room for NULL terminator. */
    char accept_key [NN_WS_HANDSHAKE_ACCEPT_KEY_LEN + 1];

    memset (self->response, 0, sizeof (self->response));

    if (self->response_code == NN_WS_HANDSHAKE_RESPONSE_OK) {
        /*  Upgrade connection as per RFC 6455 section 4.2.2. */
        
        rc = nn_ws_handshake_hash_key (self->key, self->key_len,
            accept_key, sizeof (accept_key));
        nn_assert (rc >= 0);

        nn_assert (strlen (accept_key) == NN_WS_HANDSHAKE_ACCEPT_KEY_LEN);

        protocol = nn_alloc (self->protocol_len + 1, "WebSocket protocol");
        alloc_assert (protocol);
        strncpy (protocol, self->protocol, self->protocol_len);
        protocol [self->protocol_len] = '\0';

        sprintf (self->response,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "Sec-WebSocket-Protocol: %s\r\n\r\n",
            accept_key, protocol);

        nn_free (protocol);
    }
    else {
        /*  Fail the connection with a helpful hint. */
        switch (self->response_code) {
        case NN_WS_HANDSHAKE_RESPONSE_TOO_BIG:
            code = "400 Opening Handshake Too Long";
            break;
        case NN_WS_HANDSHAKE_RESPONSE_WSPROTO:
            code = "400 Cannot Have Body";
            break;
        case NN_WS_HANDSHAKE_RESPONSE_WSVERSION:
            code = "400 Unsupported WebSocket Version";
            break;
        case NN_WS_HANDSHAKE_RESPONSE_NNPROTO:
            code = "400 Missing nanomsg Required Headers";
            break;
        case NN_WS_HANDSHAKE_RESPONSE_NOTPEER:
            code = "400 Incompatible Socket Type";
            break;
        case NN_WS_HANDSHAKE_RESPONSE_UNKNOWNTYPE:
            code = "400 Unrecognized Socket Type";
            break;
        default:
            /*  Unexpected failure response. */
            nn_assert (0);
            break;
        }

        version = nn_alloc (self->version_len + 1, "WebSocket version");
        alloc_assert (version);
        strncpy (version, self->version, self->version_len);
        version [self->version_len] = '\0';

        /*  Fail connection as per RFC 6455 4.4. */
        sprintf (self->response,
            "HTTP/1.1 %s\r\n"
            "Sec-WebSocket-Version: %s\r\n",
            code, version);

        nn_free (version);
    }

    response.iov_len = strlen (self->response);
    response.iov_base = &self->response;

    nn_usock_send (self->usock, &response, 1);

    return;
}

static int nn_ws_handshake_hash_key (const char *key, size_t key_len,
    char *hashed, size_t hashed_len)
{
    int rc;
    unsigned i;
    struct nn_sha1 hash;

    nn_sha1_init (&hash);

    for (i = 0; i < key_len; i++)
        nn_sha1_hashbyte (&hash, key [i]);

    for (i = 0; i < strlen (NN_WS_HANDSHAKE_MAGIC_GUID); i++)
        nn_sha1_hashbyte (&hash, NN_WS_HANDSHAKE_MAGIC_GUID [i]);

    rc = nn_base64_encode (nn_sha1_result (&hash),
        sizeof (hash.state), hashed, hashed_len);

    return rc;
}

