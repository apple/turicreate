/*
    Copyright (c) 2013 Insollo Entertainment, LLC.  All rights reserved.

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

#ifndef NN_OPTIONS_HEADER
#define NN_OPTIONS_HEADER

#include <stdlib.h>

enum nn_option_type {
    NN_OPT_HELP,
    NN_OPT_INT,
    NN_OPT_INCREMENT,
    NN_OPT_DECREMENT,
    NN_OPT_ENUM,
    NN_OPT_SET_ENUM,
    NN_OPT_STRING,
    NN_OPT_BLOB,
    NN_OPT_FLOAT,
    NN_OPT_LIST_APPEND,
    NN_OPT_LIST_APPEND_FMT,
    NN_OPT_READ_FILE
};

struct nn_option {
    /*  Option names  */
    char *longname;
    char shortname;
    char *arg0name;

    /*  Parsing specification  */
    enum nn_option_type type;
    int offset;  /*  offsetof() where to store the value  */
    const void *pointer;  /*  type specific pointer  */

    /*  Conflict mask for options  */
    unsigned long mask_set;
    unsigned long conflicts_mask;
    unsigned long requires_mask;

    /*  Group and description for --help  */
    char *group;
    char *metavar;
    char *description;
};

struct nn_commandline {
    char *short_description;
    char *long_description;
    struct nn_option *options;
    int required_options;
};

struct nn_enum_item {
    char *name;
    int value;
};

struct nn_string_list {
    char **items;
    char **to_free;
    int num;
    int to_free_num;
};

struct nn_blob {
    char *data;
    size_t length;
    int need_free;
};


void nn_parse_options (struct nn_commandline *cline,
                      void *target, int argc, char **argv);
void nn_free_options (struct nn_commandline *cline, void *target);


#endif  /* NN_OPTIONS_HEADER */
