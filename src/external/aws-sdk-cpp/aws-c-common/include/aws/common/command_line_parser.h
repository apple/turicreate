#ifndef AWS_COMMON_COMMAND_LINE_PARSER_H
#define AWS_COMMON_COMMAND_LINE_PARSER_H
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
#include <aws/common/common.h>

enum aws_cli_options_has_arg {
    AWS_CLI_OPTIONS_NO_ARGUMENT = 0,
    AWS_CLI_OPTIONS_REQUIRED_ARGUMENT = 1,
    AWS_CLI_OPTIONS_OPTIONAL_ARGUMENT = 2,
};

/* Ignoring padding since we're trying to maintain getopt.h compatibility */
/* NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding) */
struct aws_cli_option {
    const char *name;
    enum aws_cli_options_has_arg has_arg;
    int *flag;
    int val;
};

/**
 * Initialized to 1 (for where the first argument would be). As arguments are parsed, this number is the index
 * of the next argument to parse. Reset this to 1 to parse another set of arguments, or to rerun the parser.
 */
AWS_COMMON_API extern int aws_cli_optind;

/**
 * If an option has an argument, when the option is encountered, this will be set to the argument portion.
 */
AWS_COMMON_API extern const char *aws_cli_optarg;

AWS_EXTERN_C_BEGIN
/**
 * A mostly compliant implementation of posix getopt_long(). Parses command-line arguments. argc is the number of
 * command line arguments passed in argv. optstring contains the legitimate option characters. The option characters
 * coorespond to aws_cli_option::val. If the character is followed by a :, the option requires an argument. If it is
 * followed by '::', the argument is optional (not implemented yet).
 *
 *  longopts, is an array of struct aws_cli_option. These are the allowed options for the program.
 *  The last member of the array must be zero initialized.
 *
 *  If longindex is non-null, it will be set to the index in longopts, for the found option.
 *
 *  Returns option val if it was found, '?' if an option was encountered that was not specified in the option string,
 * returns -1 when all arguments that can be parsed have been parsed.
 */
AWS_COMMON_API int aws_cli_getopt_long(
    int argc,
    char *const argv[],
    const char *optstring,
    const struct aws_cli_option *longopts,
    int *longindex);
AWS_EXTERN_C_END

#endif /* AWS_COMMON_COMMAND_LINE_PARSER_H */
