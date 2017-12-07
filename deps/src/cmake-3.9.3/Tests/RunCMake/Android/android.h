#ifndef __ANDROID__
#error "__ANDROID__ not defined"
#endif

#include <android/api-level.h>

#if API_LEVEL != __ANDROID_API__
#error "API levels do not match"
#endif

#ifdef COMPILER_IS_CLANG
#ifndef __clang__
#error "COMPILER_IS_CLANG but __clang__ is not defined"
#endif
#else
#ifdef __clang__
#error "!COMPILER_IS_CLANG but __clang__ is defined"
#endif
#endif

#ifdef ARM_MODE
#if ARM_MODE == 1 && defined(__thumb__)
#error "ARM_MODE==1 but __thumb__ is defined"
#elif ARM_MODE == 0 && !defined(__thumb__)
#error "ARM_MODE==0 but __thumb__ is not defined"
#endif
#endif

#ifdef ARM_NEON
#if ARM_NEON == 0 && defined(__ARM_NEON__)
#error "ARM_NEON==0 but __ARM_NEON__ is defined"
#elif ARM_NEON == 1 && !defined(__ARM_NEON__)
#error "ARM_NEON==1 but __ARM_NEON__ is not defined"
#endif
#endif

#ifdef ABI_armeabi
#ifndef __ARM_EABI__
#error "ABI_armeabi: __ARM_EABI__ not defined"
#endif
#if __ARM_ARCH != 5
#error "ABI_armeabi: __ARM_ARCH is not 5"
#endif
#endif

#ifdef ABI_armeabi_v6
#ifndef __ARM_EABI__
#error "ABI_armeabi_v6: __ARM_EABI__ not defined"
#endif
#if __ARM_ARCH != 6
#error "ABI_armeabi_v6: __ARM_ARCH is not 6"
#endif
#endif

#ifdef ABI_armeabi_v7a
#ifndef __ARM_EABI__
#error "ABI_armeabi_v7a: __ARM_EABI__ not defined"
#endif
#if __ARM_ARCH != 7
#error "ABI_armeabi_v7a: __ARM_ARCH is not 7"
#endif
#endif

#ifdef ABI_arm64_v8a
#ifdef __ARM_EABI__
#error "ABI_arm64_v8a: __ARM_EABI__ defined"
#endif
#ifndef __aarch64__
#error "ABI_arm64_v8a: __aarch64__ not defined"
#endif
#endif

#ifdef ABI_mips
#if __mips != 32
#error "ABI_mips: __mips != 32"
#endif
#ifndef _ABIO32
#error "ABI_mips: _ABIO32 not defined"
#endif
#endif

#ifdef ABI_mips64
#if __mips != 64
#error "ABI_mips64: __mips != 64"
#endif
#ifndef _ABI64
#error "ABI_mips: _ABI64 not defined"
#endif
#endif

#ifdef ABI_x86
#ifndef __i686__
#error "ABI_x86: __i686__ not defined"
#endif
#endif

#ifdef ABI_x86_64
#ifndef __x86_64__
#error "ABI_x86_64: __x86_64__ not defined"
#endif
#endif

#include <stddef.h>
