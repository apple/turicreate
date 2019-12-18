/*
    Copyright (c) 2013 Insollo Entertainment, LLC.  All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>

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

#include "../src/nn.h"
#include "../src/pubsub.h"
#include "../src/pipeline.h"
#include "../src/bus.h"
#include "../src/pair.h"
#include "../src/survey.h"
#include "../src/reqrep.h"

#include "options.h"
#include "../src/utils/sleep.c"
#include "../src/utils/clock.c"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#if !defined NN_HAVE_WINDOWS
#include <unistd.h>
#endif

enum echo_format {
    NN_NO_ECHO,
    NN_ECHO_RAW,
    NN_ECHO_ASCII,
    NN_ECHO_QUOTED,
    NN_ECHO_MSGPACK,
    NN_ECHO_HEX
};

typedef struct nn_options {
    /* Global options */
    int verbose;

    /* Socket options */
    int socket_type;
    struct nn_string_list bind_addresses;
    struct nn_string_list connect_addresses;
    float send_timeout;
    float recv_timeout;
    struct nn_string_list subscriptions;
    char *socket_name;

    /* Output options */
    float send_delay;
    float send_interval;
    struct nn_blob data_to_send;

    /* Input options */
    enum echo_format echo_format;
} nn_options_t;

/*  Constants to get address of in option declaration  */
static const int nn_push = NN_PUSH;
static const int nn_pull = NN_PULL;
static const int nn_pub = NN_PUB;
static const int nn_sub = NN_SUB;
static const int nn_req = NN_REQ;
static const int nn_rep = NN_REP;
static const int nn_bus = NN_BUS;
static const int nn_pair = NN_PAIR;
static const int nn_surveyor = NN_SURVEYOR;
static const int nn_respondent = NN_RESPONDENT;


struct nn_enum_item socket_types[] = {
    {"PUSH", NN_PUSH},
    {"PULL", NN_PULL},
    {"PUB", NN_PUB},
    {"SUB", NN_SUB},
    {"REQ", NN_REQ},
    {"REP", NN_REP},
    {"BUS", NN_BUS},
    {"PAIR", NN_PAIR},
    {"SURVEYOR", NN_SURVEYOR},
    {"RESPONDENT", NN_RESPONDENT},
    {NULL, 0},
};


/*  Constants to get address of in option declaration  */
static const int nn_echo_raw = NN_ECHO_RAW;
static const int nn_echo_ascii = NN_ECHO_ASCII;
static const int nn_echo_quoted = NN_ECHO_QUOTED;
static const int nn_echo_msgpack = NN_ECHO_MSGPACK;
static const int nn_echo_hex = NN_ECHO_HEX;

struct nn_enum_item echo_formats[] = {
    {"no", NN_NO_ECHO},
    {"raw", NN_ECHO_RAW},
    {"ascii", NN_ECHO_ASCII},
    {"quoted", NN_ECHO_QUOTED},
    {"msgpack", NN_ECHO_MSGPACK},
    {"hex", NN_ECHO_HEX},
    {NULL, 0},
};

/*  Constants for conflict masks  */
#define NN_MASK_SOCK 1
#define NN_MASK_WRITEABLE 2
#define NN_MASK_READABLE 4
#define NN_MASK_SOCK_SUB 8
#define NN_MASK_DATA 16
#define NN_MASK_ENDPOINT 32
#define NN_NO_PROVIDES 0
#define NN_NO_CONFLICTS 0
#define NN_NO_REQUIRES 0
#define NN_MASK_SOCK_WRITEABLE (NN_MASK_SOCK | NN_MASK_WRITEABLE)
#define NN_MASK_SOCK_READABLE (NN_MASK_SOCK | NN_MASK_READABLE)
#define NN_MASK_SOCK_READWRITE  (NN_MASK_SOCK_WRITEABLE|NN_MASK_SOCK_READABLE)

struct nn_option nn_options[] = {
    /* Generic options */
    {"verbose", 'v', NULL,
     NN_OPT_INCREMENT, offsetof (nn_options_t, verbose), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Generic", NULL, "Increase verbosity of the nanocat"},
    {"silent", 'q', NULL,
     NN_OPT_DECREMENT, offsetof (nn_options_t, verbose), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Generic", NULL, "Decrease verbosity of the nanocat"},
    {"help", 'h', NULL,
     NN_OPT_HELP, 0, NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Generic", NULL, "This help text"},

    /* Socket types */
    {"push", 0, "nn_push",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_push,
     NN_MASK_SOCK_WRITEABLE, NN_MASK_SOCK, NN_MASK_DATA,
     "Socket Types", NULL, "Use NN_PUSH socket type"},
    {"pull", 0, "nn_pull",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pull,
     NN_MASK_SOCK_READABLE, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PULL socket type"},
    {"pub", 0, "nn_pub",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pub,
     NN_MASK_SOCK_WRITEABLE, NN_MASK_SOCK, NN_MASK_DATA,
     "Socket Types", NULL, "Use NN_PUB socket type"},
    {"sub", 0, "nn_sub",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_sub,
     NN_MASK_SOCK_READABLE|NN_MASK_SOCK_SUB, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_SUB socket type"},
    {"req", 0, "nn_req",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_req,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_MASK_DATA,
     "Socket Types", NULL, "Use NN_REQ socket type"},
    {"rep", 0, "nn_rep",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_rep,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_REP socket type"},
    {"surveyor", 0, "nn_surveyor",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_surveyor,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_MASK_DATA,
     "Socket Types", NULL, "Use NN_SURVEYOR socket type"},
    {"respondent", 0, "nn_respondent",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_respondent,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_RESPONDENT socket type"},
    {"bus", 0, "nn_bus",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_bus,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_BUS socket type"},
    {"pair", 0, "nn_pair",
     NN_OPT_SET_ENUM, offsetof (nn_options_t, socket_type), &nn_pair,
     NN_MASK_SOCK_READWRITE, NN_MASK_SOCK, NN_NO_REQUIRES,
     "Socket Types", NULL, "Use NN_PAIR socket type"},

    /* Socket Options */
    {"bind", 0, NULL,
     NN_OPT_LIST_APPEND, offsetof (nn_options_t, bind_addresses), NULL,
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "ADDR", "Bind socket to the address ADDR"},
    {"connect", 0, NULL,
     NN_OPT_LIST_APPEND, offsetof (nn_options_t, connect_addresses), NULL,
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "ADDR", "Connect socket to the address ADDR"},
    {"bind-ipc", 'X' , NULL, NN_OPT_LIST_APPEND_FMT,
     offsetof (nn_options_t, bind_addresses), "ipc://%s",
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "PATH", "Bind socket to the ipc address "
                               "\"ipc://PATH\"."},
    {"connect-ipc", 'x' , NULL, NN_OPT_LIST_APPEND_FMT,
     offsetof (nn_options_t, connect_addresses), "ipc://%s",
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "PATH", "Connect socket to the ipc address "
                               "\"ipc://PATH\"."},
    {"bind-local", 'L' , NULL, NN_OPT_LIST_APPEND_FMT,
     offsetof (nn_options_t, bind_addresses), "tcp://127.0.0.1:%s",
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "PORT", "Bind socket to the tcp address "
                               "\"tcp://127.0.0.1:PORT\"."},
    {"connect-local", 'l' , NULL, NN_OPT_LIST_APPEND_FMT,
     offsetof (nn_options_t, connect_addresses), "tcp://127.0.0.1:%s",
     NN_MASK_ENDPOINT, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "PORT", "Connect socket to the tcp address "
                               "\"tcp://127.0.0.1:PORT\"."},
    {"recv-timeout", 0, NULL,
     NN_OPT_FLOAT, offsetof (nn_options_t, recv_timeout), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Socket Options", "SEC", "Set timeout for receiving a message"},
    {"send-timeout", 0, NULL,
     NN_OPT_FLOAT, offsetof (nn_options_t, send_timeout), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_WRITEABLE,
     "Socket Options", "SEC", "Set timeout for sending a message"},
    {"socket-name", 0, NULL,
     NN_OPT_STRING, offsetof (nn_options_t, socket_name), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Socket Options", "NAME", "Name of the socket for statistics"},

    /* Pattern-specific options */
    {"subscribe", 0, NULL,
     NN_OPT_LIST_APPEND, offsetof (nn_options_t, subscriptions), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_SOCK_SUB,
     "SUB Socket Options", "PREFIX", "Subscribe to the prefix PREFIX. "
        "Note: socket will be subscribed to everything (empty prefix) if "
        "no prefixes are specified on the command-line."},

    /* Input Options */
    {"format", 0, NULL,
     NN_OPT_ENUM, offsetof (nn_options_t, echo_format), &echo_formats,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", "FORMAT", "Use echo format FORMAT "
                               "(same as the options below)"},
    {"raw", 0, NULL,
     NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_raw,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", NULL, "Dump message as is "
                           "(Note: no delimiters are printed)"},
    {"ascii", 'A', NULL,
     NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_ascii,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", NULL, "Print ASCII part of message delimited by newline. "
                           "All non-ascii characters replaced by dot."},
    {"quoted", 'Q', NULL,
     NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_quoted,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", NULL, "Print each message on separate line in double "
                           "quotes with C-like character escaping"},
    {"msgpack", 0, NULL,
     NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_msgpack,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", NULL, "Print each message as msgpacked string (raw type)."
                           " This is useful for programmatic parsing."},

    {"hex", 0, NULL,
     NN_OPT_SET_ENUM, offsetof (nn_options_t, echo_format), &nn_echo_hex,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_READABLE,
     "Input Options", NULL, "Print each message on separate line in double "
                           "quotes with hex values"},
    /* Output Options */
    {"interval", 'i', NULL,
     NN_OPT_FLOAT, offsetof (nn_options_t, send_interval), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_MASK_WRITEABLE,
     "Output Options", "SEC", "Send message (or request) every SEC seconds"},
    {"delay", 'd', NULL,
     NN_OPT_FLOAT, offsetof (nn_options_t, send_delay), NULL,
     NN_NO_PROVIDES, NN_NO_CONFLICTS, NN_NO_REQUIRES,
     "Output Options", "SEC", "Wait for SEC seconds before sending message"
                              " (useful for one-shot PUB sockets)"},
    {"data", 'D', NULL,
     NN_OPT_BLOB, offsetof (nn_options_t, data_to_send), &echo_formats,
     NN_MASK_DATA, NN_MASK_DATA, NN_MASK_WRITEABLE,
     "Output Options", "DATA", "Send DATA to the socket and quit for "
     "PUB, PUSH, PAIR, BUS socket. Use DATA to reply for REP or "
     " RESPONDENT socket. Send DATA as request for REQ or SURVEYOR socket."},
    {"file", 'F', NULL,
     NN_OPT_READ_FILE, offsetof (nn_options_t, data_to_send), &echo_formats,
     NN_MASK_DATA, NN_MASK_DATA, NN_MASK_WRITEABLE,
     "Output Options", "PATH", "Same as --data but get data from file PATH"},

    /* Sentinel */
    {NULL, 0, NULL,
     0, 0, NULL,
     0, 0, 0,
     NULL, NULL, NULL},
    };


struct nn_commandline nn_cli = {
    "A command-line interface to nanomsg",
    "",
    nn_options,
    NN_MASK_SOCK | NN_MASK_ENDPOINT,
};


void nn_assert_errno (int flag, char *description)
{
    int err;

    if (!flag) {
        err = errno;
        fprintf (stderr, "%s: %s\n", description, nn_strerror (err));
        exit (3);
    }
}

void nn_sub_init (nn_options_t *options, int sock)
{
    int i;
    int rc;

    if (options->subscriptions.num) {
        for (i = 0; i < options->subscriptions.num; ++i) {
            rc = nn_setsockopt (sock, NN_SUB, NN_SUB_SUBSCRIBE,
                options->subscriptions.items[i],
                strlen (options->subscriptions.items[i]));
            nn_assert_errno (rc == 0, "Can't subscribe");
        }
    } else {
        rc = nn_setsockopt (sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
        nn_assert_errno (rc == 0, "Can't subscribe");
    }
}

void nn_set_recv_timeout (int sock, int millis)
{
    int rc;
    rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_RCVTIMEO,
                       &millis, sizeof (millis));
    nn_assert_errno (rc == 0, "Can't set recv timeout");
}

int nn_create_socket (nn_options_t *options)
{
    int sock;
    int rc;
    int millis;

    sock = nn_socket (AF_SP, options->socket_type);
    nn_assert_errno (sock >= 0, "Can't create socket");

    /* Generic initialization */
    if (options->send_timeout >= 0) {
        millis = (int)(options->send_timeout * 1000);
        rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_SNDTIMEO,
                           &millis, sizeof (millis));
        nn_assert_errno (rc == 0, "Can't set send timeout");
    }
    if (options->recv_timeout >= 0) {
        nn_set_recv_timeout (sock, (int) options->recv_timeout * 1000);
    }
    if (options->socket_name) {
        rc = nn_setsockopt (sock, NN_SOL_SOCKET, NN_SOCKET_NAME,
                           options->socket_name, strlen(options->socket_name));
        nn_assert_errno (rc == 0, "Can't set socket name");
    }

    /* Specific initialization */
    switch (options->socket_type) {
    case NN_SUB:
        nn_sub_init (options, sock);
        break;
    }

    return sock;
}

void nn_print_message (nn_options_t *options, char *buf, int buflen)
{
    switch (options->echo_format) {
    case NN_NO_ECHO:
        return;
    case NN_ECHO_RAW:
        fwrite (buf, 1, buflen, stdout);
        break;
    case NN_ECHO_ASCII:
        for (; buflen > 0; --buflen, ++buf) {
            if (isprint (*buf)) {
                fputc (*buf, stdout);
            } else {
                fputc ('.', stdout);
            }
        }
        fputc ('\n', stdout);
        break;
    case NN_ECHO_QUOTED:
        fputc ('"', stdout);
        for (; buflen > 0; --buflen, ++buf) {
            switch (*buf) {
            case '\n':
                fprintf (stdout, "\\n");
                break;
            case '\r':
                fprintf (stdout, "\\r");
                break;
            case '\\':
            case '\"':
                fprintf (stdout, "\\%c", *buf);
                break;
            default:
                if (isprint (*buf)) {
                    fputc (*buf, stdout);
                } else {
                    fprintf (stdout, "\\x%02x", (unsigned char)*buf);
                }
            }
        }
        fprintf (stdout, "\"\n");
        break;
    case NN_ECHO_MSGPACK:
        if (buflen < 256) {
            fputc ('\xc4', stdout);
            fputc (buflen, stdout);
            fwrite (buf, 1, buflen, stdout);
        } else if (buflen < 65536) {
            fputc ('\xc5', stdout);
            fputc (buflen >> 8, stdout);
            fputc (buflen & 0xff, stdout);
            fwrite (buf, 1, buflen, stdout);
        } else {
            fputc ('\xc6', stdout);
            fputc (buflen >> 24, stdout);
            fputc ((buflen >> 16) & 0xff, stdout);
            fputc ((buflen >> 8) & 0xff, stdout);
            fputc (buflen & 0xff, stdout);
            fwrite (buf, 1, buflen, stdout);
        }
        break;
    case NN_ECHO_HEX:
        fputc ('"', stdout);
        for (; buflen > 0; --buflen, ++buf) {
             fprintf (stdout, "\\x%02x", (unsigned char)*buf);
        }
        fprintf (stdout, "\"\n");
        break;
    
    }
    fflush (stdout);
}

void nn_connect_socket (nn_options_t *options, int sock)
{
    int i;
    int rc;

    for (i = 0; i < options->bind_addresses.num; ++i) {
        rc = nn_bind (sock, options->bind_addresses.items[i]);
        nn_assert_errno (rc >= 0, "Can't bind");
    }
    for (i = 0; i < options->connect_addresses.num; ++i) {
        rc = nn_connect (sock, options->connect_addresses.items[i]);
        nn_assert_errno (rc >= 0, "Can't connect");
    }
}

void nn_send_loop (nn_options_t *options, int sock)
{
    int rc;
    uint64_t start_time;
    int64_t time_to_sleep, interval;

    interval = (int)(options->send_interval*1000);

    for (;;) {
        start_time = nn_clock_ms();
        rc = nn_send (sock,
            options->data_to_send.data, options->data_to_send.length,
            0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
        if (interval >= 0) {
            time_to_sleep = (start_time + interval) - nn_clock_ms();
            if (time_to_sleep > 0) {
                nn_sleep ((int) time_to_sleep);
            }
        } else {
            break;
        }
    }
}

void nn_recv_loop (nn_options_t *options, int sock)
{
    int rc;
    void *buf;

    for (;;) {
        rc = nn_recv (sock, &buf, NN_MSG, 0);
        if (rc < 0 && errno == EAGAIN) {
            continue;
        } else if (rc < 0 && (errno == ETIMEDOUT || errno == EFSM)) {
            return;  /*  No more messages possible  */
        } else {
            nn_assert_errno (rc >= 0, "Can't recv");
        }
        nn_print_message (options, buf, rc);
        nn_freemsg (buf);
    }
}

void nn_rw_loop (nn_options_t *options, int sock)
{
    int rc;
    void *buf;
    uint64_t start_time;
    int64_t time_to_sleep, interval, recv_timeout;

    interval = (int)(options->send_interval*1000);
    recv_timeout = (int)(options->recv_timeout*1000);

    for (;;) {
        start_time = nn_clock_ms();
        rc = nn_send (sock,
            options->data_to_send.data, options->data_to_send.length,
            0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
        if (options->send_interval < 0) {  /*  Never send any more  */
            nn_recv_loop (options, sock);
            return;
        }

        for (;;) {
            time_to_sleep = (start_time + interval) - nn_clock_ms();
            if (time_to_sleep <= 0) {
                break;
            }
            if (recv_timeout >= 0 && time_to_sleep > recv_timeout)
            {
                time_to_sleep = recv_timeout;
            }
            nn_set_recv_timeout (sock, (int) time_to_sleep);
            rc = nn_recv (sock, &buf, NN_MSG, 0);
            if (rc < 0) {
                if (errno == EAGAIN) {
                    continue;
                } else if (errno == ETIMEDOUT || errno == EFSM) {
                    time_to_sleep = (start_time + interval) - nn_clock_ms();
                    if (time_to_sleep > 0)
                        nn_sleep ((int) time_to_sleep);
                    continue;
                }
            }
            nn_assert_errno (rc >= 0, "Can't recv");
            nn_print_message (options, buf, rc);
            nn_freemsg (buf);
        }
    }
}

void nn_resp_loop (nn_options_t *options, int sock)
{
    int rc;
    void *buf;

    for (;;) {
        rc = nn_recv (sock, &buf, NN_MSG, 0);
        if (rc < 0 && errno == EAGAIN) {
                continue;
        } else {
            nn_assert_errno (rc >= 0, "Can't recv");
        }
        nn_print_message (options, buf, rc);
        nn_freemsg (buf);
        rc = nn_send (sock,
            options->data_to_send.data, options->data_to_send.length,
            0);
        if (rc < 0 && errno == EAGAIN) {
            fprintf (stderr, "Message not sent (EAGAIN)\n");
        } else {
            nn_assert_errno (rc >= 0, "Can't send");
        }
    }
}

int main (int argc, char **argv)
{
    int sock;
    nn_options_t options = {
        /* verbose           */ 0,
        /* socket_type       */ 0,
        /* bind_addresses    */ {NULL, NULL, 0, 0},
        /* connect_addresses */ {NULL, NULL, 0, 0},
        /* send_timeout      */ -1.f,
        /* recv_timeout      */ -1.f,
        /* subscriptions     */ {NULL, NULL, 0, 0},
        /* socket_name       */ NULL,
        /* send_delay        */ 0.f,
        /* send_interval     */ -1.f,
        /* data_to_send      */ {NULL, 0, 0},
        /* echo_format       */ NN_NO_ECHO
    };

    nn_parse_options (&nn_cli, &options, argc, argv);
    sock = nn_create_socket (&options);
    nn_connect_socket (&options, sock);
    nn_sleep((int)(options.send_delay*1000));
    switch (options.socket_type) {
    case NN_PUB:
    case NN_PUSH:
        nn_send_loop (&options, sock);
        break;
    case NN_SUB:
    case NN_PULL:
        nn_recv_loop (&options, sock);
        break;
    case NN_BUS:
    case NN_PAIR:
        if (options.data_to_send.data) {
            nn_rw_loop (&options, sock);
        } else {
            nn_recv_loop (&options, sock);
        }
        break;
    case NN_SURVEYOR:
    case NN_REQ:
        nn_rw_loop (&options, sock);
        break;
    case NN_REP:
    case NN_RESPONDENT:
        if (options.data_to_send.data) {
            nn_resp_loop (&options, sock);
        } else {
            nn_recv_loop (&options, sock);
        }
        break;
    }

    nn_close (sock);
    nn_free_options(&nn_cli, &options);
    return 0;
}
