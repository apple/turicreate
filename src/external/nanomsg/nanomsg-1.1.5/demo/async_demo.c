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

/*  This program serves as an example for how to write an async RPC service,
    using the RAW request/reply pattern and nn_poll.  The server receives
    messages and keeps them on a list, replying to them.

    Our demonstration application layer protocol is simple.  The client sends
    a number of milliseconds to wait before responding.  The server just gives
    back an empty reply after waiting that long.

    To run this program, start the server as async_demo <url> -s
    Then connect to it with the client as async_client <url> <msec>.

    For example:

    % ./async_demo tcp://127.0.0.1:5555 -s &
    % ./async_demo tcp://127.0.0.1:5555 323
    Request took 324 milliseconds.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

/*  MAXJOBS is a limit on the on the number of outstanding requests we
    can queue.  We will not accept new inbound jobs if we have more than
    this queued.  The reason for this limit is to prevent a bad client
    from consuming all server resources with new job requests. */

#define MAXJOBS 100

/*  The server keeps a list of work items, sorted by expiration time,
    so that we can use this to set the timeout to the correct value for
    use in poll.  */
struct work {
    struct work *next;
    struct nn_msghdr request;
    uint64_t expire;
    void *control;
};

/*  Return the UNIX time in milliseconds.  You'll need a working
    gettimeofday(), so this won't work on Windows.  */
uint64_t milliseconds (void)
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (((uint64_t)tv.tv_sec * 1000) + ((uint64_t)tv.tv_usec / 1000));
}
    
/*  The server runs forever. */
int server(const char *url)
{
    int fd; 
    struct work *worklist = NULL;
    int npending = 0;

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

    /*  Main processing loop. */

    for (;;) {
        uint32_t timer;
        int rc;
        int timeout;
        uint64_t now;
        struct work *work, **srch;
        uint8_t *body;
        void *control;
        struct nn_iovec iov;
        struct nn_msghdr hdr;
        struct nn_pollfd pfd[1];

        /*  Figure out if any work requests are finished, and can be
            responded to. */

        timeout = -1;
        while ((work = worklist) != NULL) {

            now = milliseconds ();
            if (work->expire > now) {
                timeout = (work->expire - now);
                break;
            }
            worklist = work->next;
            npending--;
            rc = nn_sendmsg (fd, &work->request, NN_DONTWAIT);
            if (rc < 0) {
                fprintf (stderr, "nn_sendmsg: %s\n",
                    nn_strerror (nn_errno ()));
                nn_freemsg (work->request.msg_control);
            }
            free (work);
        }

        /*  This check ensures that we don't allow more than a set limit
            of concurrent jobs to be queued.  This protects us from resource
            exhaustion by malicious or defective clients. */

        if (npending >= MAXJOBS) {
            nn_poll (pfd, 0, timeout);
            continue;
        }

        pfd[0].fd = fd;
        pfd[0].events = NN_POLLIN;
        pfd[0].revents = 0;
        nn_poll (pfd, 1, timeout);

        if ((pfd[0].revents & NN_POLLIN) == 0) {
            continue;
        }

        /*  So there should be a message waiting for us to receive.
            We handle it by parsing it, creating a work request for it,
            and adding the work request to the worklist. */

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

        work = malloc (sizeof (*work));
        if (work == NULL) {
            fprintf (stderr, "malloc: %s\n", strerror (errno));
            /*  Fatal error -- other programs can try to handle it better. */
            break;
        }
        memset (work, 0, sizeof (*work));
        work->expire = milliseconds () + ntohl (timer);
        work->control = control;
        work->request.msg_iovlen = 0;  /*  No payload data to send. */
        work->request.msg_iov = NULL;
        work->request.msg_control = &work->control;
        work->request.msg_controllen = NN_MSG;

        /*  Insert the work request into the list in order. */
        srch = &worklist;
        for (;;) {
            if ((*srch == NULL) || ((*srch)->expire > work->expire)) {
                work->next = *srch;
                *srch = work;
                npending++;
                break;
            }
            srch = &((*srch)->next);
        }
    }

    /*  This may wind up orphaning requests in the queue.   We are going
        to exit with an error anyway, so don't worry about it. */

    nn_close (fd);
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
