/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <aws/common/command_line_parser.h>

int aws_cli_optind = 1;
int aws_cli_opterr = -1;
int aws_cli_optopt = 0;

const char *aws_cli_optarg = NULL;

static const struct aws_cli_option *s_find_option_from_char(
    const struct aws_cli_option *longopts,
    char search_for,
    int *longindex) {
    int index = 0;
    const struct aws_cli_option *option = &longopts[index];

    while (option->val != 0 || option->name) {
        if (option->val == search_for) {
            if (longindex) {
                *longindex = index;
            }
            return option;
        }

        option = &longopts[++index];
    }

    return NULL;
}

static const struct aws_cli_option *s_find_option_from_c_str(
    const struct aws_cli_option *longopts,
    const char *search_for,
    int *longindex) {
    int index = 0;
    const struct aws_cli_option *option = &longopts[index];

    while (option->name || option->val != 0) {
        if (option->name) {
            if (option->name && !strcmp(search_for, option->name)) {
                if (longindex) {
                    *longindex = index;
                }
                return option;
            }
        }

        option = &longopts[++index];
    }

    return NULL;
}

int aws_cli_getopt_long(
    int argc,
    char *const argv[],
    const char *optstring,
    const struct aws_cli_option *longopts,
    int *longindex) {
    aws_cli_optarg = NULL;

    if (aws_cli_optind >= argc) {
        return -1;
    }

    char first_char = argv[aws_cli_optind][0];
    char second_char = argv[aws_cli_optind][1];
    char *option_start = NULL;
    const struct aws_cli_option *option = NULL;

    if (first_char == '-' && second_char != '-') {
        option_start = &argv[aws_cli_optind][1];
        option = s_find_option_from_char(longopts, *option_start, longindex);
    } else if (first_char == '-' && second_char == '-') {
        option_start = &argv[aws_cli_optind][2];
        option = s_find_option_from_c_str(longopts, option_start, longindex);
    } else {
        return -1;
    }

    aws_cli_optind++;
    if (option) {
        bool has_arg = false;
        if (option) {
            char *opt_value = memchr(optstring, option->val, strlen(optstring));
            if (!opt_value) {
                return '?';
            }

            if (opt_value[1] == ':') {
                has_arg = true;
            }
        }

        if (has_arg) {
            if (aws_cli_optind >= argc - 1) {
                return '?';
            }

            aws_cli_optarg = argv[aws_cli_optind++];
        }

        return option->val;
    }

    return '?';
}
