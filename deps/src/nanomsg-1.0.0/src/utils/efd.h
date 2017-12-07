/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>
    Copyright (c) 2015-2016 Jack R. Dunaway.  All rights reserved.

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

#ifndef NN_EFD_INCLUDED
#define NN_EFD_INCLUDED

/*  Provides a way to send signals via file descriptors. The important part
    is that nn_efd_getfd() returns an actual OS-level file descriptor that
    you can poll on to wait for the event. */

#include "fd.h"

#if defined NN_USE_EVENTFD
    #include "efd_eventfd.h"
#elif defined NN_USE_PIPE
    #include "efd_pipe.h"
#elif defined NN_USE_SOCKETPAIR
    #include "efd_socketpair.h"
#elif defined NN_USE_WINSOCK
    #include "efd_win.h"
#else
    #error
#endif

/*  Initialise the efd object. */
int nn_efd_init (struct nn_efd *self);

/*  Uninitialise the efd object. */
void nn_efd_term (struct nn_efd *self);

/*  Get the OS file descriptor that is readable when the efd object
    is signaled. */
nn_fd nn_efd_getfd (struct nn_efd *self);

/*  Stop the efd object. */
void nn_efd_stop (struct nn_efd *self);

/*  Switch the object into signaled state. */
void nn_efd_signal (struct nn_efd *self);

/*  Switch the object into unsignaled state. */
void nn_efd_unsignal (struct nn_efd *self);

/*  Wait till efd object becomes signaled or when timeout (in milliseconds,
    nagative value meaning 'infinite') expires. In the former case 0 is
    returened. In the latter, -ETIMEDOUT. */
int nn_efd_wait (struct nn_efd *self, int timeout);

#endif
