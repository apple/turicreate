#ifndef AWS_COMMON_PRIVATE_ARRAY_LIST_H
#define AWS_COMMON_PRIVATE_ARRAY_LIST_H

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

AWS_EXTERN_C_BEGIN

/**
 * Helper function that calculates the number of bytes needed by an array_list, where "index" is the last valid
 * index.
 */
int aws_array_list_calc_necessary_size(struct aws_array_list *AWS_RESTRICT list, size_t index, size_t *necessary_size);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_PRIVATE_ARRAY_LIST_H */
