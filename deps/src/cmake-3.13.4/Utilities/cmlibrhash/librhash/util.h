/* util.h */
#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(__GNUC__) && __GNUC__ >= 4 && (__GNUC__ > 4 || __GNUC_MINOR__ >= 1) \
	&& defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)) \
	|| (defined(__INTEL_COMPILER) && !defined(_WIN32))
/* atomic operations are defined by ICC and GCC >= 4.1, but by the later one supposedly not for ARM */
/* note: ICC on ia64 platform possibly require ia64intrin.h, need testing */
# define atomic_compare_and_swap(ptr, oldval, newval) __sync_val_compare_and_swap(ptr, oldval, newval)
#elif defined(_MSC_VER)
# include <windows.h>
# define atomic_compare_and_swap(ptr, oldval, newval) InterlockedCompareExchange(ptr, newval, oldval)
#elif defined(__sun)
# include <atomic.h>
# define atomic_compare_and_swap(ptr, oldval, newval) atomic_cas_32(ptr, oldval, newval)
#else
/* pray that it will work */
# define atomic_compare_and_swap(ptr, oldval, newval) { if(*(ptr) == (oldval)) *(ptr) = (newval); }
# define NO_ATOMIC_BUILTINS
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* UTIL_H */
