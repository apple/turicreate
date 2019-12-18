/*
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

    "nanomsg" is a trademark of Martin Sustrik
*/

/*  This program serves as an example for how to write a threaded RPC service,
    using the RAW request/reply pattern and pthreads.  Multiple worker threads
    are spawned on a single socket, and each worker processes jobs in order.

    Our demonstration application layer protocol is simple.  The client sends
    a number of milliseconds to wait before responding.  The server just gives
    back an empty reply after waiting that long.

    To run this program, start the server as pthread_demo <url> -s
    Then connect to it with the client as pthread_client <url> <msec>.

    For example:

    % ./pthread_demo tcp://127.0.0.1:5555 -s &
    % ./pthread_demo tcp://127.0.0.1:5555 323
    Request took 324 milliseconds.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <sys/time.h>
#include <pthread.h>

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

/*  MAXWORKERS is a limit on the on the number of workers we will fire
    off.  Since each worker processes jobs sequentially, this is a limit
    on the concurrency of the server.  New inbound messages will queue up
    waiting for a worker to receive them. */

#define MAXWORKERS 100

/*  Return the UNIX time in milliseconds.  You'll need a working
    gettimeofday(), so this won't work on Windows.  */
uint64_t milliseconds (void)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000));
}

void *worker (void *arg)
{
    int fd = (intptr_t)arg; 

    /*  Main processing loop. */

    for (;;) {
        uint32_t timer;
        int rc;
        int timeout;
        uint8_t *body;
        void *control;
        struct nn_iovec iov;
        struct nn_msghdr hdr;

        memset (&hdr, 0, sizeof (hdr));
        control = NULL;
        iov.iov_base = &body;
        iov.iov_len = NN_MSG;
        hdr.msg_iov = &iov;
        hdr.msg_iovlen = 1;
        hdr.msg_control = &control;
        hdr.msg_controllen = NN_MSG;

        rc = nn_recvmsg (fd, &hdr, 0);
        if (rc < 0) {
            if (nn_errno () == EBADF) {
                return (NULL);   /* Socket closed by another thread. */
            }
            /*  Any error here is unexpected. */
            fprintf (stderr, "nn_recv: %s\n", nn_strerror (nn_errno ()));
            break;
        }

        if (rc != sizeof (uint32_t)) {
            fprintf (stderr, "nn_recv: wanted %d, but got %d\n",
                (int) sizeof (uint32_t), rc);
            nn_freemsg (body);
            nn_freemsg (control);
            continue;
        }

        memcpy (&timer, body, sizeof (timer));
        nn_freemsg (body);

        /*  Poor mans usleep but in msec. */
        poll (NULL, 0, ntohl (timer));

        hdr.msg_iov = NULL;
        hdr.msg_iovlen = 0;

        rc = nn_sendmsg (fd, &hdr, 0);
        if (rc < 0) {
            fprintf (stderr, "nn_send: %s\n", nn_strerror (nn_errno ()));
            nn_freemsg (control);
        }
    }

    /*  We got here, so close the file.  That will cause the other threads
        to shut down too. */

    nn_close (fd);
    return (NULL);
}
/*  The server runs forever. */
int server(const char *url)
{
    int fd; 
    int i;
    pthread_t tids [MAXWORKERS];
    int rc;

    /*  Create the socket. */
    fd = nn_socket(AF_SP_RAW, NN_REP);
    if (fd < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror (nn_errno ()));
        return (-1);
    }

    /*  Bind to the URL.  This will bind to the address and listen
        synchronously; new clients will be accepted asynchronously
        without further action from the calling program. */

    if (nn_bind (fd, url) < 0) {
        fprintf (stderr, "nn_bind: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    memset (tids, 0, sizeof (tids));

    /*  Start up the threads. */
    for (i = 0; i < MAXWORKERS; i++) {
        rc = pthread_create (&tids[i], NULL, worker, (void *)(intptr_t)fd);
        if (rc < 0) {
            fprintf (stderr, "pthread_create: %s\n", strerror (rc));
            nn_close (fd);
            break;
        }
    }

    /*  Now wait on them to finish. */
    for (i = 0; i < MAXWORKERS; i++) {
        if (tids[i] != NULL) {
            pthread_join (tids[i], NULL);
        }
    }
    return (-1);
}

/*  The client runs just once, and then returns. */
int client (const char *url, const char *msecstr)
{
    int fd;
    int rc;
    char *greeting;
    uint8_t msg[sizeof (uint32_t)];
    uint64_t start;
    uint64_t end;
    unsigned msec;

    msec = atoi(msecstr);

    fd = nn_socket (AF_SP, NN_REQ);
    if (fd < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror (nn_errno ()));
        return (-1);
    }

    if (nn_connect (fd, url) < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);        
    }

    msec = htonl(msec);
    memcpy (msg, &msec, sizeof (msec));

    start = milliseconds ();

    if (nn_send (fd, msg, sizeof (msg), 0) < 0) {
        fprintf (stderr, "nn_send: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    rc = nn_recv (fd, &msg, sizeof (msg), 0);
    if (rc < 0) {
        fprintf (stderr, "nn_recv: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    nn_close (fd);

    end = milliseconds ();

    printf ("Request took %u milliseconds.\n", (uint32_t)(end - start));
    return (0);
}

int main (int argc, char **argv)
{
    int rc;
    if (argc < 3) {
        fprintf (stderr, "Usage: %s <url> [-s|name]\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    if (strcmp (argv[2], "-s") == 0) {
        rc = server (argv[1]);
    } else {
        rc = client (argv[1], argv[2]);
    }
    exit (rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
