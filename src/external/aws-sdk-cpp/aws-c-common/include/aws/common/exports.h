#ifndef AWS_COMMON_EXPORTS_H
#define AWS_COMMON_EXPORTS_H
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
#if defined(AWS_C_RT_USE_WINDOWS_DLL_SEMANTICS) || defined(WIN32)
#    ifdef AWS_COMMON_USE_IMPORT_EXPORT
#        ifdef AWS_COMMON_EXPORTS
#            define AWS_COMMON_API __declspec(dllexport)
#        else
#            define AWS_COMMON_API __declspec(dllimport)
#        endif /* AWS_COMMON_EXPORTS */
#    else
#        define AWS_COMMON_API
#    endif /* AWS_COMMON_USE_IMPORT_EXPORT */

#else /* defined (AWS_C_RT_USE_WINDOWS_DLL_SEMANTICS) || defined (WIN32) */

#    if ((__GNUC__ >= 4) || defined(__clang__)) && defined(AWS_COMMON_USE_IMPORT_EXPORT) && defined(AWS_COMMON_EXPORTS)
#        define AWS_COMMON_API __attribute__((visibility("default")))
#    else
#        define AWS_COMMON_API
#    endif /* __GNUC__ >= 4 || defined(__clang__) */

#endif /* defined (AWS_C_RT_USE_WINDOWS_DLL_SEMANTICS) || defined (WIN32) */

#ifdef AWS_NO_STATIC_IMPL
#    define AWS_STATIC_IMPL AWS_COMMON_API
#endif

#ifndef AWS_STATIC_IMPL
/*
 * In order to allow us to export our inlinable methods in a DLL/.so, we have a designated .c
 * file where this AWS_STATIC_IMPL macro will be redefined to be non-static.
 */
#    define AWS_STATIC_IMPL static inline
#endif

#endif /* AWS_COMMON_EXPORTS_H */
