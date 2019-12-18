/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore

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
#include "../src/survey.h"

#include "testutil.h"

#define SOCKET_ADDRESS "inproc://test"

int main ()
{
    int rc;
    int surveyor;
    int respondent1;
    int respondent2;
    int respondent3;
    int deadline;
    char buf [7];

    /*  Test a simple survey with three respondents. */
    surveyor = test_socket (AF_SP, NN_SURVEYOR);
    deadline = 500;
    rc = nn_setsockopt (surveyor, NN_SURVEYOR, NN_SURVEYOR_DEADLINE,
        &deadline, sizeof (deadline));
    errno_assert (rc == 0);
    test_bind (surveyor, SOCKET_ADDRESS);
    respondent1 = test_socket (AF_SP, NN_RESPONDENT);
    test_connect (respondent1, SOCKET_ADDRESS);
    respondent2 = test_socket (AF_SP, NN_RESPONDENT);
    test_connect (respondent2, SOCKET_ADDRESS);
    respondent3 = test_socket (AF_SP, NN_RESPONDENT);
    test_connect (respondent3, SOCKET_ADDRESS);

    /* Check that attempt to recv with no survey pending is EFSM. */
    rc = nn_recv (surveyor, buf, sizeof (buf), 0);
    errno_assert (rc == -1 && nn_errno () == EFSM);

    /*  Send the survey. */
    test_send (surveyor, "ABC");

    /*  First respondent answers. */
    test_recv (respondent1, "ABC");
    test_send (respondent1, "DEF");

    /*  Second respondent answers. */
    test_recv (respondent2, "ABC");
    test_send (respondent2, "DEF");

    /*  Surveyor gets the responses. */
    test_recv (surveyor, "DEF");
    test_recv (surveyor, "DEF");

    /*  There are no more responses. Surveyor hits the deadline. */
    rc = nn_recv (surveyor, buf, sizeof (buf), 0);
    errno_assert (rc == -1 && nn_errno () == ETIMEDOUT);

    /*  Third respondent answers (it have already missed the deadline). */
    test_recv (respondent3, "ABC");
    test_send (respondent3, "GHI");

    /*  Surveyor initiates new survey. */
    test_send (surveyor, "ABC");

    /*  Check that stale response from third respondent is not delivered. */
    rc = nn_recv (surveyor, buf, sizeof (buf), 0);
    errno_assert (rc == -1 && nn_errno () == ETIMEDOUT);

    /* Check that subsequent attempt to recv with no survey pending is EFSM. */
    rc = nn_recv (surveyor, buf, sizeof (buf), 0);
    errno_assert (rc == -1 && nn_errno () == EFSM);

    test_close (surveyor);
    test_close (respondent1);
    test_close (respondent2);
    test_close (respondent3);

    return 0;
}

