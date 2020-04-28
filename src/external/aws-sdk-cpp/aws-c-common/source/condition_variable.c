/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/common/condition_variable.h>

int aws_condition_variable_wait_pred(
    struct aws_condition_variable *condition_variable,
    struct aws_mutex *mutex,
    aws_condition_predicate_fn *pred,
    void *pred_ctx) {

    int err_code = 0;
    while (!err_code && !pred(pred_ctx)) {
        err_code = aws_condition_variable_wait(condition_variable, mutex);
    }

    return err_code;
}

int aws_condition_variable_wait_for_pred(
    struct aws_condition_variable *condition_variable,
    struct aws_mutex *mutex,
    int64_t time_to_wait,
    aws_condition_predicate_fn *pred,
    void *pred_ctx) {

    int err_code = 0;
    while (!err_code && !pred(pred_ctx)) {
        err_code = aws_condition_variable_wait_for(condition_variable, mutex, time_to_wait);
    }

    return err_code;
}
