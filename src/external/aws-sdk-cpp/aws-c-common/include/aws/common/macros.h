#ifndef AWS_COMMON_MACROS_H
#define AWS_COMMON_MACROS_H
/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifdef __cplusplus
#    define AWS_EXTERN_C_BEGIN extern "C" {
#    define AWS_EXTERN_C_END }
#else
#    define AWS_EXTERN_C_BEGIN
#    define AWS_EXTERN_C_END
#endif /*  __cplusplus */

#define AWS_CONCAT(A, B) A##B
#define AWS_STATIC_ASSERT0(cond, msg) typedef char AWS_CONCAT(static_assertion_, msg)[(!!(cond)) * 2 - 1]
#define AWS_STATIC_ASSERT1(cond, line) AWS_STATIC_ASSERT0(cond, AWS_CONCAT(at_line_, line))
#define AWS_STATIC_ASSERT(cond) AWS_STATIC_ASSERT1(cond, __LINE__)

/* https://stackoverflow.com/questions/9183993/msvc-variadic-macro-expansion */
#define GLUE(x, y) x y

#define RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, count, ...) count
#define EXPAND_ARGS(args) RETURN_ARG_COUNT args
#define COUNT_ARGS_MAX5(...) EXPAND_ARGS((__VA_ARGS__, 5, 4, 3, 2, 1, 0))

#define OVERLOAD_MACRO2(name, count) name##count
#define OVERLOAD_MACRO1(name, count) OVERLOAD_MACRO2(name, count)
#define OVERLOAD_MACRO(name, count) OVERLOAD_MACRO1(name, count)

#define CALL_OVERLOAD(name, ...) GLUE(OVERLOAD_MACRO(name, COUNT_ARGS_MAX5(__VA_ARGS__)), (__VA_ARGS__))

#define CALL_OVERLOAD_TEST1(x) x
#define CALL_OVERLOAD_TEST2(x, y) y
#define CALL_OVERLOAD_TEST3(x, y, z) z
#define CALL_OVERLOAD_TEST(...) CALL_OVERLOAD(CALL_OVERLOAD_TEST, __VA_ARGS__)
AWS_STATIC_ASSERT(CALL_OVERLOAD_TEST(1) == 1);
AWS_STATIC_ASSERT(CALL_OVERLOAD_TEST(1, 2) == 2);
AWS_STATIC_ASSERT(CALL_OVERLOAD_TEST(1, 2, 3) == 3);

#define AWS_CACHE_LINE 64
/**
 * Format macro for strings of a specified length.
 * Allows non null-terminated strings to be used with the printf family of functions.
 * Ex: printf("scheme is " PRInSTR, 4, "http://example.org"); // ouputs: "scheme is http"
 */
#define PRInSTR "%.*s"

#if defined(_MSC_VER)
#    include <malloc.h>
#    define AWS_ALIGNED_TYPEDEF(from, to, alignment) typedef __declspec(align(alignment)) from to
#    define AWS_LIKELY(x) x
#    define AWS_UNLIKELY(x) x
#    define AWS_FORCE_INLINE __forceinline
#    define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type *name = _alloca(sizeof(type) * (length))
#    define AWS_DECLSPEC_NORETURN __declspec(noreturn)
#    define AWS_ATTRIBUTE_NORETURN
#else
#    if defined(__GNUC__) || defined(__clang__)
#        define AWS_ALIGNED_TYPEDEF(from, to, alignment) typedef from to __attribute__((aligned(alignment)))
#        define AWS_TYPE_OF(a) __typeof__(a)
#        define AWS_LIKELY(x) __builtin_expect(!!(x), 1)
#        define AWS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#        define AWS_FORCE_INLINE __attribute__((always_inline))
#        define AWS_DECLSPEC_NORETURN
#        define AWS_ATTRIBUTE_NORETURN __attribute__((noreturn))
#        if defined(__cplusplus)
#            define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type *name = alloca(sizeof(type) * (length))
#        else
#            define AWS_VARIABLE_LENGTH_ARRAY(type, name, length) type name[length];
#        endif /* defined(__cplusplus) */
#    endif     /*  defined(__GNUC__) || defined(__clang__) */
#endif         /*  defined(_MSC_VER) */

/* If this is C++, restrict isn't supported. If this is not at least C99 on gcc and clang, it isn't supported.
 * If visual C++ building in C mode, the restrict definition is __restrict.
 * This just figures all of that out based on who's including this header file. */
#if defined(__cplusplus)
#    define AWS_RESTRICT
#else
#    if defined(_MSC_VER)
#        define AWS_RESTRICT __restrict
#    else
#        if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#            define AWS_RESTRICT restrict
#        else
#            define AWS_RESTRICT
#        endif /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
#    endif     /* defined(_MSC_VER) */
#endif         /* defined(__cplusplus) */

#if defined(_MSC_VER)
#    define AWS_THREAD_LOCAL __declspec(thread)
#else
#    define AWS_THREAD_LOCAL __thread
#endif

#define AWS_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
/**
 * from a pointer and a type of the struct containing the node
 * this will get you back to the pointer of the object. member is the name of
 * the instance of struct aws_linked_list_node in your struct.
 */
#define AWS_CONTAINER_OF(ptr, type, member) ((type *)((uint8_t *)(ptr)-offsetof(type, member)))

#endif /* AWS_COMMON_MACROS_H */
