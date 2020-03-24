#ifndef AWS_COMMON_PACKAGE_H
#define AWS_COMMON_PACKAGE_H

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

/*
 * Preliminary cap on the number of possible aws-c-libraries participating in shared enum ranges for
 * errors, log subjects, and other cross-library enums. Expandable as needed
 */
#define AWS_PACKAGE_SLOTS 16

/*
 * Each aws-c-* and aws-crt-* library has a unique package id starting from zero.  These are used to macro-calculate
 * correct ranges for the cross-library enumerations.
 */
#define AWS_C_COMMON_PACKAGE_ID 0

#endif /* AWS_COMMON_PACKAGE_H */
