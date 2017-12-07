/*
    Copyright (c) 2012 250bpm s.r.o.  All rights reserved.
    Copyright (c) 2014 Wirebird Labs LLC.  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>

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

#ifndef WS_H_INCLUDED
#define WS_H_INCLUDED

#include "nn.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NN_WS -4

/*  NN_WS level socket/cmsg options.  Note that only NN_WSMG_TYPE_TEXT and
    NN_WS_MSG_TYPE_BINARY messages are supported fully by this implementation.
    Attempting to set other message types is undefined.  */
#define NN_WS_MSG_TYPE 1

/*  WebSocket opcode constants as per RFC 6455 5.2  */
#define NN_WS_MSG_TYPE_TEXT 0x01
#define NN_WS_MSG_TYPE_BINARY 0x02

#ifdef __cplusplus
}
#endif

#endif
