/*
    Copyright (c) 2014 Drew Crawford.  All rights reserved.

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

/*  Base class for device. */
struct nn_device_recipe {

    /*  NN_CHECK flags. */
    int required_checks;

    /*  The entry function.  This checks the inputs according to the
        required_checks flag, chooses the polling function, and starts
        the device.  You can override this function to implement
        additional checks.*/
    int(*nn_device_entry) (struct nn_device_recipe *device,
        int s1, int s2, int flags);

    /*  The two-way poll function. */
    int (*nn_device_twoway) (struct nn_device_recipe *device, int s1, int s2);

    /*  The one-way poll function. */
    int (*nn_device_oneway) (struct nn_device_recipe *device, int s1, int s2);

    int (*nn_device_loopback) (struct nn_device_recipe *device, int s);

    /*  The movemsg function. */
    int (*nn_device_mvmsg) (struct nn_device_recipe *device,
        int from, int to, int flags);

    /*  The message intercept function.  This function gives you an opportunity
        to modify or cancel an nn_msghdr as it passes from one socket
        to the other.

        from - the socket that the msghdr was received from
        to - the socket where it is going
        flags - the flags that are being used for send and receive functions
        msghdr - the nn_msghdr that was received from the from socket
        bytes - the actual received length of the msg.
                The nn_msghdr->msg_iov->iov_len is not valid because
                it contains NN_MSG

        return values:

        1  indicates that the msghdr should be forwarded.
        0  indicates that the msghdr should *not* be forwarded,
           e.g. the message is dropped in the device
        -1 indicates an error.  Set errno.
    */
    int (*nn_device_rewritemsg) (struct nn_device_recipe *device,
        int from, int to, int flags, struct nn_msghdr *msghdr, int bytes);
};

/*  Default implementations of the functions. */
int nn_device_loopback (struct nn_device_recipe *device, int s);
int nn_device_twoway (struct nn_device_recipe *device, int s1, int s2);
int nn_device_oneway (struct nn_device_recipe *device, int s1, int s2);
int nn_device_mvmsg (struct nn_device_recipe *device,
    int from, int to, int flags);
int nn_device_entry(struct nn_device_recipe *device,
    int s1, int s2, int flags);
int nn_device_rewritemsg(struct nn_device_recipe *device,
    int from, int to, int flags, struct nn_msghdr *msghdr, int bytes);


/*  At least one socket must be passed to the device. */
#define NN_CHECK_AT_LEAST_ONE_SOCKET (1 << 0)
/*  Loopback devices are allowed. */
#define NN_CHECK_ALLOW_LOOPBACK (1 << 1)
/*  Bidirectional devices are allowed. */
#define NN_CHECK_ALLOW_BIDIRECTIONAL (1 << 2)
/*  Unidirectional devices are allowed. */
#define NN_CHECK_ALLOW_UNIDIRECTIONAL (1<<3)
/*  Both sockets must be raw. */
#define NN_CHECK_REQUIRE_RAW_SOCKETS (1 << 4)
/*  Both sockets must be same protocol family. */
#define NN_CHECK_SAME_PROTOCOL_FAMILY (1 << 5)
/*  Check socket directionality. */
#define NN_CHECK_SOCKET_DIRECTIONALITY (1 << 6)

/*  Allows spawning a custom device from a recipe */
int nn_custom_device(struct nn_device_recipe *device,
    int s1, int s2, int flags);

static struct nn_device_recipe nn_ordinary_device = {
    NN_CHECK_AT_LEAST_ONE_SOCKET | NN_CHECK_ALLOW_LOOPBACK | NN_CHECK_ALLOW_BIDIRECTIONAL | NN_CHECK_REQUIRE_RAW_SOCKETS | NN_CHECK_SAME_PROTOCOL_FAMILY | NN_CHECK_SOCKET_DIRECTIONALITY | NN_CHECK_ALLOW_UNIDIRECTIONAL,
    nn_device_entry,
    nn_device_twoway,
    nn_device_oneway,
    nn_device_loopback,
    nn_device_mvmsg,
    nn_device_rewritemsg
};

