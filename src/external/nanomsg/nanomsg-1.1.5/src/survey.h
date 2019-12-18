/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
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

#ifndef SURVEY_H_INCLUDED
#define SURVEY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define NN_PROTO_SURVEY 6

/*  NB: Version 0 used 16 + 0/1.  That version lacked backtraces, and so
    is wire-incompatible with this version. */

#define NN_SURVEYOR (NN_PROTO_SURVEY * 16 + 2)
#define NN_RESPONDENT (NN_PROTO_SURVEY * 16 + 3)

#define NN_SURVEYOR_DEADLINE 1

#ifdef __cplusplus
}
#endif

#endif

