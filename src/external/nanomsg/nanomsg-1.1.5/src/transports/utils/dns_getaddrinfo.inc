/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.

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

#include "literal.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/attr.h"

#include <string.h>

#ifndef NN_HAVE_WINDOWS
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define NN_DNS_STATE_IDLE 1
#define NN_DNS_STATE_DONE 2

/*  Private functions. */
static void nn_dns_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_dns_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

void nn_dns_init (struct nn_dns *self, int src, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_dns_handler, nn_dns_shutdown,
        src, self, owner);
    self->state = NN_DNS_STATE_IDLE;
    nn_fsm_event_init (&self->done);
}

void nn_dns_term (struct nn_dns *self)
{
    nn_assert_state (self, NN_DNS_STATE_IDLE);

    nn_fsm_event_term (&self->done);
    nn_fsm_term (&self->fsm);
}

int nn_dns_isidle (struct nn_dns *self)
{
    return nn_fsm_isidle (&self->fsm);
}

void nn_dns_start (struct nn_dns *self, const char *addr, size_t addrlen,
    int ipv4only, struct nn_dns_result *result)
{
    int rc;
    struct addrinfo query;
    struct addrinfo *reply;
    char hostname [NN_SOCKADDR_MAX];

    nn_assert_state (self, NN_DNS_STATE_IDLE);

    self->result = result;

    /*  Try to resolve the supplied string as a literal address. In this case,
        there's no DNS lookup involved. */
    rc = nn_literal_resolve (addr, addrlen, ipv4only, &self->result->addr,
        &self->result->addrlen);
    if (rc == 0) {
        self->result->error = 0;
        nn_fsm_start (&self->fsm);
        return;
    }
    errnum_assert (rc == -EINVAL, -rc);

    /*  The name is not a literal. Let's do an actual DNS lookup. */
    memset (&query, 0, sizeof (query));
    if (ipv4only)
        query.ai_family = AF_INET;
    else {
        query.ai_family = AF_INET6;
#ifdef AI_V4MAPPED
        query.ai_flags = AI_V4MAPPED;
#endif
    }
    nn_assert (sizeof (hostname) > addrlen);
    query.ai_socktype = SOCK_STREAM;
    memcpy (hostname, addr, addrlen);
    hostname [addrlen] = 0;

    /*  Perform the DNS lookup itself. */
    self->result->error = getaddrinfo (hostname, NULL, &query, &reply);
    if (self->result->error) {
        nn_fsm_start (&self->fsm);
        return;
    }

    /*  Take just the first address and store it.  (RFC recommends that we
        iterate through addresses until one works, but that doesn't match
        our state model.  This is the best we can do.) */
    self->result->error = 0;
    memcpy (&self->result->addr, reply->ai_addr, reply->ai_addrlen);
    self->result->addrlen = reply->ai_addrlen;
    freeaddrinfo (reply);

    nn_fsm_start (&self->fsm);
}

void nn_dns_stop (struct nn_dns *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_dns_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_dns *dns;

    dns = nn_cont (self, struct nn_dns, fsm);
    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        nn_fsm_stopped (&dns->fsm, NN_DNS_STOPPED);
        dns->state = NN_DNS_STATE_IDLE;
        return;
    }

    nn_fsm_bad_state(dns->state, src, type);
}

static void nn_dns_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_dns *dns;

    dns = nn_cont (self, struct nn_dns, fsm);

    switch (dns->state) {
/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_DNS_STATE_IDLE:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_fsm_raise (&dns->fsm, &dns->done, NN_DNS_DONE);
                dns->state = NN_DNS_STATE_DONE;
                return;
            default:
                nn_fsm_bad_action (dns->state, src, type);
            }
        default:
            nn_fsm_bad_source (dns->state, src, type);
        }

/******************************************************************************/
/*  DONE state.                                                               */
/******************************************************************************/
    case NN_DNS_STATE_DONE:
        nn_fsm_bad_source (dns->state, src, type);

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (dns->state, src, type);
    }
}

