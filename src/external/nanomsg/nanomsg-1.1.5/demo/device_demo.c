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

/*  This program serves as an example for how to write a simple device service, using
    a rendezvous.  This works by having the program support three modes.  The protocol is
    is REQ/REP, where the REQ is a name, and the REP is a greeting based on the name, and
    an instance number (we use the process ID) and time of day.

    We provide a rendezvous server running the device code, where servers and clients can
    connect.  Both sides of the device are in bind mode, and both servers and clients run
    in connect mode.  This lets us support many servers and clients simultaneously.

    For example, if I want to have servers rendezvous at port 5554 and clients at port 5555:

    % ./device_demo -d tcp://127.0.0.1:5554 tcp://127.0.0.1:5555 &
    % ./device_demo -s tcp://127.0.0.1:5554 &
    % ./device_demo -c tcp://127.0.0.1:5555 Garrett
    Good morning, Garrett.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

/*  The server runs forever. */
int server(const char *url)
{
    int fd; 

    /*  Create the socket. */
    fd = nn_socket (AF_SP, NN_REP);
    if (fd < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror (nn_errno ()));
        return (-1);
    }

    /*  Connect to the URL.  This will connect to the address and listen
        synchronously; new clients will be accepted asynchronously
        without further action from the calling program. */

    if (nn_connect (fd, url) < 0) {
        fprintf (stderr, "nn_connect: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    /*  Now we can just process results.  Note that there is no explicit
        accept required.  We just receive a request, and reply to it.
        Its important to note that we must not issue two receives in a
        row without replying first, or the following receive(s) will
        cancel any unreplied requests. */

    for (;;) {
        char username[128];
        char greeting[128];
        time_t secs;
        struct tm *now;
        char *daytime;
        int rc;
        char *fmt;

        rc = nn_recv (fd, username, sizeof (username), 0);
        if (rc < 0) {
            /*  Any error here is unexpected. */
            fprintf (stderr, "nn_recv: %s\n", nn_strerror (nn_errno ()));
            break;
        }

        secs = time (NULL);
        now = localtime (&secs);
        if (now->tm_hour < 12) {
            daytime = "morning";

        } else if (now->tm_hour < 17) {
            daytime = "afternoon";

        } else if (now->tm_hour < 20) {
            daytime = "evening";

        } else {
            daytime = "night";
        }

        /*  Ensure ASCIIZ terminated string. */
        if (rc < sizeof (username)) {
            username[rc] = '\0';
        } else {
            username[sizeof (username) - 1] = '\0';
        }

        fmt = "Good %s, %s (from %d).";

        /*  Technically this might be overly pessimistic about size. */
        if ((strlen (username) + strlen (daytime) + strlen (fmt)) >=
            sizeof (greeting)) {

            fmt = "I'm sorry, your name is too long.  But good %s anyway.";
        }

        /*  snprintf would be safer, but the above check protects us. */
        sprintf (greeting, fmt, daytime, username, (int)getpid());

        rc = nn_send (fd, greeting, strlen (greeting), 0);
        if (rc < 0) {
            /*  There are several legitimate reasons this can fail.
                We note them for debugging purposes, but then ignore
                otherwise.  If the socket is closed or failing, we will
                notice in recv above, and exit then. */
            fprintf (stderr, "nn_send: %s (ignoring)\n",
                nn_strerror (nn_errno ()));
        }
    }

    nn_close (fd);
    return (-1);
}

/*  The client runs just once, and then returns. */
int client (const char *url, const char *username)
{
    int fd;
    int rc;
    char *greeting;
    char *msg;

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

    usleep(1000);

    if (nn_send (fd, username, strlen (username), 0) < 0) {
        fprintf (stderr, "nn_send: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    /*  Here we ask the library to allocate response buffer for us (NN_MSG). */
    rc = nn_recv (fd, &msg, NN_MSG, 0);
    if (rc < 0) {
        fprintf (stderr, "nn_recv: %s\n", nn_strerror (nn_errno ()));
        nn_close (fd);
        return (-1);
    }

    nn_close (fd);

    /*  Response is not ASCIIZ terminated. */
    greeting = calloc (rc + 1, 1);
    if (greeting == NULL) {
        fprintf (stderr, "calloc: %s\n", strerror (errno));
        return (-1);
    }
    memcpy(greeting, msg, rc);

    nn_freemsg (msg);
    printf ("%s\n", greeting); 
    free (greeting);
    return (0);
}

int device (const char *url1, const char *url2)
{
    int s1, s2, rv;
    s1 = nn_socket (AF_SP_RAW, NN_REQ);
    if (s1 < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror (nn_errno ()));
        return (-1);
    }
    if (nn_bind (s1, url1) < 0) {
        fprintf (stderr, "nn_bind1(%s): %s\n", url1, nn_strerror (nn_errno ()));
        return (-1);
    }
    s2 = nn_socket (AF_SP_RAW, NN_REP);
    if (s2 < 0) {
        fprintf (stderr, "nn_socket: %s\n", nn_strerror(nn_errno ()));
        return (-1);
    }
    if (nn_bind (s2, url2) < 0) {
        fprintf (stderr, "nn_bind2(%s): %s\n", url2, nn_strerror (nn_errno ()));
        return (-1);
    }

    if (nn_device (s1, s2) != 0) {
        fprintf (stderr, "nn_device: %s\n", nn_strerror (nn_errno ()));
        return (-1);
    }
    return (0);
}

int main (int argc, char **argv)
{
    int rc;

    if ((argc == 3) && (strcmp (argv[1], "-s") == 0)) {
        rc = server (argv[2]);
    } else if ((argc == 4) && (strcmp (argv[1], "-d") == 0)) {
        rc = device (argv[2], argv[3]);
    } else if ((argc == 4) && (strcmp (argv[1], "-c") == 0)) {
        rc = client (argv[2], argv[3]);
    } else {
        fprintf (stderr, "Usage: %s -s <serverurl>\n", argv[0]);
        fprintf (stderr, "Usage: %s -d <serverurl> <clienturl>\n", argv[0]);
        fprintf (stderr, "Usage: %s -c <clienturl> <name>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    
    exit (rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
