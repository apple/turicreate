/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright (c) 2015-2016 Jack R. Dunaway.  All rights reserved.
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

#ifndef NN_H_INCLUDED
#define NN_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

/*  Handle DSO symbol visibility. */
#if !defined(NN_EXPORT)
#    if defined(_WIN32) && !defined(NN_STATIC_LIB)
#        if defined NN_SHARED_LIB
#            define NN_EXPORT __declspec(dllexport)
#        else
#            define NN_EXPORT __declspec(dllimport)
#        endif
#    else
#        define NN_EXPORT extern
#    endif
#endif

/******************************************************************************/
/*  ABI versioning support.                                                   */
/******************************************************************************/

/*  Don't change this unless you know exactly what you're doing and have      */
/*  read and understand the following documents:                              */
/*  www.gnu.org/software/libtool/manual/html_node/Libtool-versioning.html     */
/*  www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html  */

/*  The current interface version. */
#define NN_VERSION_CURRENT 5

/*  The latest revision of the current interface. */
#define NN_VERSION_REVISION 1

/*  How many past interface versions are still supported. */
#define NN_VERSION_AGE 0

/******************************************************************************/
/*  Errors.                                                                   */
/******************************************************************************/

/*  A number random enough not to collide with different errno ranges on      */
/*  different OSes. The assumption is that error_t is at least 32-bit type.   */
#define NN_HAUSNUMERO 156384712

/*  On some platforms some standard POSIX errnos are not defined.    */
#ifndef ENOTSUP
#define ENOTSUP (NN_HAUSNUMERO + 1)
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT (NN_HAUSNUMERO + 2)
#endif
#ifndef ENOBUFS
#define ENOBUFS (NN_HAUSNUMERO + 3)
#endif
#ifndef ENETDOWN
#define ENETDOWN (NN_HAUSNUMERO + 4)
#endif
#ifndef EADDRINUSE
#define EADDRINUSE (NN_HAUSNUMERO + 5)
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL (NN_HAUSNUMERO + 6)
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED (NN_HAUSNUMERO + 7)
#endif
#ifndef EINPROGRESS
#define EINPROGRESS (NN_HAUSNUMERO + 8)
#endif
#ifndef ENOTSOCK
#define ENOTSOCK (NN_HAUSNUMERO + 9)
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT (NN_HAUSNUMERO + 10)
#endif
#ifndef EPROTO
#define EPROTO (NN_HAUSNUMERO + 11)
#endif
#ifndef EAGAIN
#define EAGAIN (NN_HAUSNUMERO + 12)
#endif
#ifndef EBADF
#define EBADF (NN_HAUSNUMERO + 13)
#endif
#ifndef EINVAL
#define EINVAL (NN_HAUSNUMERO + 14)
#endif
#ifndef EMFILE
#define EMFILE (NN_HAUSNUMERO + 15)
#endif
#ifndef EFAULT
#define EFAULT (NN_HAUSNUMERO + 16)
#endif
#ifndef EACCES
#define EACCES (NN_HAUSNUMERO + 17)
#endif
#ifndef EACCESS
#define EACCESS (EACCES)
#endif
#ifndef ENETRESET
#define ENETRESET (NN_HAUSNUMERO + 18)
#endif
#ifndef ENETUNREACH
#define ENETUNREACH (NN_HAUSNUMERO + 19)
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH (NN_HAUSNUMERO + 20)
#endif
#ifndef ENOTCONN
#define ENOTCONN (NN_HAUSNUMERO + 21)
#endif
#ifndef EMSGSIZE
#define EMSGSIZE (NN_HAUSNUMERO + 22)
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT (NN_HAUSNUMERO + 23)
#endif
#ifndef ECONNABORTED
#define ECONNABORTED (NN_HAUSNUMERO + 24)
#endif
#ifndef ECONNRESET
#define ECONNRESET (NN_HAUSNUMERO + 25)
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT (NN_HAUSNUMERO + 26)
#endif
#ifndef EISCONN
#define EISCONN (NN_HAUSNUMERO + 27)
#define NN_EISCONN_DEFINED
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT (NN_HAUSNUMERO + 28)
#endif

/*  Native nanomsg error codes.                                               */
#ifndef ETERM
#define ETERM (NN_HAUSNUMERO + 53)
#endif
#ifndef EFSM
#define EFSM (NN_HAUSNUMERO + 54)
#endif

/*  This function retrieves the errno as it is known to the library.          */
/*  The goal of this function is to make the code 100% portable, including    */
/*  where the library is compiled with certain CRT library (on Windows) and   */
/*  linked to an application that uses different CRT library.                 */
NN_EXPORT int nn_errno (void);

/*  Resolves system errors and native errors to human-readable string.        */
NN_EXPORT const char *nn_strerror (int errnum);


/*  Returns the symbol name (e.g. "NN_REQ") and value at a specified index.   */
/*  If the index is out-of-range, returns NULL and sets errno to EINVAL       */
/*  General usage is to start at i=0 and iterate until NULL is returned.      */
NN_EXPORT const char *nn_symbol (int i, int *value);

/*  Constants that are returned in `ns` member of nn_symbol_properties        */
#define NN_NS_NAMESPACE 0
#define NN_NS_VERSION 1
#define NN_NS_DOMAIN 2
#define NN_NS_TRANSPORT 3
#define NN_NS_PROTOCOL 4
#define NN_NS_OPTION_LEVEL 5
#define NN_NS_SOCKET_OPTION 6
#define NN_NS_TRANSPORT_OPTION 7
#define NN_NS_OPTION_TYPE 8
#define NN_NS_OPTION_UNIT 9
#define NN_NS_FLAG 10
#define NN_NS_ERROR 11
#define NN_NS_LIMIT 12
#define NN_NS_EVENT 13
#define NN_NS_STATISTIC 14

/*  Constants that are returned in `type` member of nn_symbol_properties      */
#define NN_TYPE_NONE 0
#define NN_TYPE_INT 1
#define NN_TYPE_STR 2

/*  Constants that are returned in the `unit` member of nn_symbol_properties  */
#define NN_UNIT_NONE 0
#define NN_UNIT_BYTES 1
#define NN_UNIT_MILLISECONDS 2
#define NN_UNIT_PRIORITY 3
#define NN_UNIT_BOOLEAN 4
#define NN_UNIT_MESSAGES 5
#define NN_UNIT_COUNTER 6

/*  Structure that is returned from nn_symbol  */
struct nn_symbol_properties {

    /*  The constant value  */
    int value;

    /*  The constant name  */
    const char* name;

    /*  The constant namespace, or zero for namespaces themselves */
    int ns;

    /*  The option type for socket option constants  */
    int type;

    /*  The unit for the option value for socket option constants  */
    int unit;
};

/*  Fills in nn_symbol_properties structure and returns it's length           */
/*  If the index is out-of-range, returns 0                                   */
/*  General usage is to start at i=0 and iterate until zero is returned.      */
NN_EXPORT int nn_symbol_info (int i,
    struct nn_symbol_properties *buf, int buflen);

/******************************************************************************/
/*  Helper function for shutting down multi-threaded applications.            */
/******************************************************************************/

NN_EXPORT void nn_term (void);

/******************************************************************************/
/*  Zero-copy support.                                                        */
/******************************************************************************/

#define NN_MSG ((size_t) -1)

NN_EXPORT void *nn_allocmsg (size_t size, int type);
NN_EXPORT void *nn_reallocmsg (void *msg, size_t size);
NN_EXPORT int nn_freemsg (void *msg);

/******************************************************************************/
/*  Socket definition.                                                        */
/******************************************************************************/

struct nn_iovec {
    void *iov_base;
    size_t iov_len;
};

struct nn_msghdr {
    struct nn_iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
};

struct nn_cmsghdr {
    size_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
};

/*  Internal stuff. Not to be used directly.                                  */
NN_EXPORT  struct nn_cmsghdr *nn_cmsg_nxthdr_ (
    const struct nn_msghdr *mhdr,
    const struct nn_cmsghdr *cmsg);
#define NN_CMSG_ALIGN_(len) \
    (((len) + sizeof (size_t) - 1) & (size_t) ~(sizeof (size_t) - 1))

/* POSIX-defined msghdr manipulation. */

#define NN_CMSG_FIRSTHDR(mhdr) \
    nn_cmsg_nxthdr_ ((struct nn_msghdr*) (mhdr), NULL)

#define NN_CMSG_NXTHDR(mhdr, cmsg) \
    nn_cmsg_nxthdr_ ((struct nn_msghdr*) (mhdr), (struct nn_cmsghdr*) (cmsg))

#define NN_CMSG_DATA(cmsg) \
    ((unsigned char*) (((struct nn_cmsghdr*) (cmsg)) + 1))

/* Extensions to POSIX defined by RFC 3542.                                   */

#define NN_CMSG_SPACE(len) \
    (NN_CMSG_ALIGN_ (len) + NN_CMSG_ALIGN_ (sizeof (struct nn_cmsghdr)))

#define NN_CMSG_LEN(len) \
    (NN_CMSG_ALIGN_ (sizeof (struct nn_cmsghdr)) + (len))

/*  SP address families.                                                      */
#define AF_SP 1
#define AF_SP_RAW 2

/*  Max size of an SP address.                                                */
#define NN_SOCKADDR_MAX 128

/*  Socket option levels: Negative numbers are reserved for transports,
    positive for socket types. */
#define NN_SOL_SOCKET 0

/*  Generic socket options (NN_SOL_SOCKET level).                             */
#define NN_LINGER 1
#define NN_SNDBUF 2
#define NN_RCVBUF 3
#define NN_SNDTIMEO 4
#define NN_RCVTIMEO 5
#define NN_RECONNECT_IVL 6
#define NN_RECONNECT_IVL_MAX 7
#define NN_SNDPRIO 8
#define NN_RCVPRIO 9
#define NN_SNDFD 10
#define NN_RCVFD 11
#define NN_DOMAIN 12
#define NN_PROTOCOL 13
#define NN_IPV4ONLY 14
#define NN_SOCKET_NAME 15
#define NN_RCVMAXSIZE 16
#define NN_MAXTTL 17

/*  Send/recv options.                                                        */
#define NN_DONTWAIT 1

/*  Ancillary data.                                                           */
#define PROTO_SP 1
#define SP_HDR 1

NN_EXPORT int nn_socket (int domain, int protocol);
NN_EXPORT int nn_close (int s);
NN_EXPORT int nn_setsockopt (int s, int level, int option, const void *optval,
    size_t optvallen);
NN_EXPORT int nn_getsockopt (int s, int level, int option, void *optval,
    size_t *optvallen);
NN_EXPORT int nn_bind (int s, const char *addr);
NN_EXPORT int nn_connect (int s, const char *addr);
NN_EXPORT int nn_shutdown (int s, int how);
NN_EXPORT int nn_send (int s, const void *buf, size_t len, int flags);
NN_EXPORT int nn_recv (int s, void *buf, size_t len, int flags);
NN_EXPORT int nn_sendmsg (int s, const struct nn_msghdr *msghdr, int flags);
NN_EXPORT int nn_recvmsg (int s, struct nn_msghdr *msghdr, int flags);

/******************************************************************************/
/*  Socket mutliplexing support.                                              */
/******************************************************************************/

#define NN_POLLIN 1
#define NN_POLLOUT 2

struct nn_pollfd {
    int fd;
    short events;
    short revents;
};

NN_EXPORT int nn_poll (struct nn_pollfd *fds, int nfds, int timeout);

/******************************************************************************/
/*  Built-in support for devices.                                             */
/******************************************************************************/

NN_EXPORT int nn_device (int s1, int s2);

/******************************************************************************/
/*  Statistics.                                                               */
/******************************************************************************/

/*  Transport statistics  */
#define NN_STAT_ESTABLISHED_CONNECTIONS 101
#define NN_STAT_ACCEPTED_CONNECTIONS    102
#define NN_STAT_DROPPED_CONNECTIONS     103
#define NN_STAT_BROKEN_CONNECTIONS      104
#define NN_STAT_CONNECT_ERRORS          105
#define NN_STAT_BIND_ERRORS             106
#define NN_STAT_ACCEPT_ERRORS           107

#define NN_STAT_CURRENT_CONNECTIONS     201
#define NN_STAT_INPROGRESS_CONNECTIONS  202
#define NN_STAT_CURRENT_EP_ERRORS       203

/*  The socket-internal statistics  */
#define NN_STAT_MESSAGES_SENT           301
#define NN_STAT_MESSAGES_RECEIVED       302
#define NN_STAT_BYTES_SENT              303
#define NN_STAT_BYTES_RECEIVED          304
/*  Protocol statistics  */
#define	NN_STAT_CURRENT_SND_PRIORITY    401

NN_EXPORT uint64_t nn_get_statistic (int s, int stat);

#ifdef __cplusplus
}
#endif

#endif
