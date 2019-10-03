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

#ifndef NN_REQ_INCLUDED
#define NN_REQ_INCLUDED

#include "xreq.h"
#include "task.h"

#include "../../protocol.h"
#include "../../aio/fsm.h"

struct nn_req {

    /*  The base class. Raw REQ socket. */
    struct nn_xreq xreq;

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Last request ID assigned. */
    uint32_t lastid;

    /*  Protocol-specific socket options. */
    int resend_ivl;

    /*  The request being processed. */
    struct nn_task task;
};

extern struct nn_socktype *nn_req_socktype;

/*  Some users may want to extend the REQ protocol similar to how REQ extends XREQ.
    Expose these methods to improve extensibility. */
void nn_req_init (struct nn_req *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
void nn_req_term (struct nn_req *self);
int nn_req_inprogress (struct nn_req *self);
void nn_req_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
void nn_req_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
void nn_req_action_send (struct nn_req *self, int allow_delay);

/*  Implementation of nn_sockbase's virtual functions. */
void nn_req_stop (struct nn_sockbase *self);
void nn_req_destroy (struct nn_sockbase *self);
void nn_req_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_req_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_req_events (struct nn_sockbase *self);
int nn_req_csend (struct nn_sockbase *self, struct nn_msg *msg);
void nn_req_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_req_crecv (struct nn_sockbase *self, struct nn_msg *msg);
int nn_req_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
int nn_req_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen);
int nn_req_csend (struct nn_sockbase *self, struct nn_msg *msg);
int nn_req_crecv (struct nn_sockbase *self, struct nn_msg *msg);

#endif
