/* byte_order.h */
#ifndef BYTE_ORDER_H
#define BYTE_ORDER_H
#include "ustd.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* if x86 compatible cpu */
#if defined(i386) || defined(__i386__) || defined(__i486__) || \
	defined(__i586__) || defined(__i686__) || defined(__pentium__) || \
	defined(__pentiumpro__) || defined(__pentium4__) || \
	defined(__nocona__) || defined(prescott) || defined(__core2__) || \
	defined(__k6__) || defined(__k8__) || defined(__athlon__) || \
	defined(__amd64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IX86) || \
	defined(_M_AMD64) || defined(_M_IA64) || defined(_M_X64)
/* detect if x86-64 instruction set is supported */
# if defined(_LP64) || defined(__LP64__) || defined(__x86_64) || \
	defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
#  define CPU_X64
# else
#  define CPU_IA32
# endif
#endif


/* detect CPU endianness */
#include <cm_kwiml.h>
#if KWIML_ABI_ENDIAN_ID == KWIML_ABI_ENDIAN_ID_LITTLE
# define CPU_LITTLE_ENDIAN
# define IS_BIG_ENDIAN 0
# define IS_LITTLE_ENDIAN 1
#elif KWIML_ABI_ENDIAN_ID == KWIML_ABI_ENDIAN_ID_BIG
# define CPU_BIG_ENDIAN
# define IS_BIG_ENDIAN 1
# define IS_LITTLE_ENDIAN 0
#else
# error "Can't detect CPU architechture"
#endif

#define IS_ALIGNED_32(p) (0 == (3 & ((const char*)(p) - (const char*)0)))
#define IS_ALIGNED_64(p) (0 == (7 & ((const char*)(p) - (const char*)0)))

#if defined(_MSC_VER)
#define ALIGN_ATTR(n) __declspec(align(n))
#elif defined(__GNUC__)
#define ALIGN_ATTR(n) __attribute__((aligned (n)))
#else
#define ALIGN_ATTR(n) /* nothing */
#endif


#if defined(_MSC_VER) || defined(__BORLANDC__)
#define I64(x) x##ui64
#else
#define I64(x) x##LL
#endif

/* convert a hash flag to index */
#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) /* GCC < 3.4 */
# define rhash_ctz(x) __builtin_ctz(x)
#else
unsigned rhash_ctz(unsigned); /* define as function */
#endif

void rhash_swap_copy_str_to_u32(void* to, int index, const void* from, size_t length);
void rhash_swap_copy_str_to_u64(void* to, int index, const void* from, size_t length);
void rhash_swap_copy_u64_to_str(void* to, const void* from, size_t length);
void rhash_u32_mem_swap(unsigned *p, int length_in_u32);

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

/* define bswap_32 */
#if defined(__GNUC__) && defined(CPU_IA32) && !defined(__i386__)
/* for intel x86 CPU */
static inline uint32_t bswap_32(uint32_t x) {
	__asm("bswap\t%0" : "=r" (x) : "0" (x));
	return x;
}
#elif defined(__GNUC__)  && (__GNUC__ >= 4) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)
/* for GCC >= 4.3 */
# define bswap_32(x) __builtin_bswap32(x)
#elif defined(__clang__) && __has_builtin(__builtin_bswap32)
# define bswap_32(x) __builtin_bswap32(x)
#elif (_MSC_VER > 1300) && (defined(CPU_IA32) || defined(CPU_X64)) /* MS VC */
# define bswap_32(x) _byteswap_ulong((unsigned long)x)
#else
/* general bswap_32 definition */
static uint32_t bswap_32(uint32_t x) {
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
	return (x >> 16) | (x << 16);
}
#endif /* bswap_32 */

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)
# define bswap_64(x) __builtin_bswap64(x)
#elif defined(__clang__) && __has_builtin(__builtin_bswap64)
# define bswap_64(x) __builtin_bswap64(x)
#elif (_MSC_VER > 1300) && (defined(CPU_IA32) || defined(CPU_X64)) /* MS VC */
# define bswap_64(x) _byteswap_uint64((__int64)x)
#else
static uint64_t bswap_64(uint64_t x) {
	union {
		uint64_t ll;
		uint32_t l[2];
	} w, r;
	w.ll = x;
	r.l[0] = bswap_32(w.l[1]);
	r.l[1] = bswap_32(w.l[0]);
	return r.ll;
}
#endif

#ifdef CPU_BIG_ENDIAN
# define be2me_32(x) (x)
# define be2me_64(x) (x)
# define le2me_32(x) bswap_32(x)
# define le2me_64(x) bswap_64(x)

# define be32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define le32_copy(to, index, from, length) rhash_swap_copy_str_to_u32((to), (index), (from), (length))
# define be64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define le64_copy(to, index, from, length) rhash_swap_copy_str_to_u64((to), (index), (from), (length))
# define me64_to_be_str(to, from, length) memcpy((to), (from), (length))
# define me64_to_le_str(to, from, length) rhash_swap_copy_u64_to_str((to), (from), (length))

#else /* CPU_BIG_ENDIAN */
# define be2me_32(x) bswap_32(x)
# define be2me_64(x) bswap_64(x)
# define le2me_32(x) (x)
# define le2me_64(x) (x)

# define be32_copy(to, index, from, length) rhash_swap_copy_str_to_u32((to), (index), (from), (length))
# define le32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define be64_copy(to, index, from, length) rhash_swap_copy_str_to_u64((to), (index), (from), (length))
# define le64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define me64_to_be_str(to, from, length) rhash_swap_copy_u64_to_str((to), (from), (length))
# define me64_to_le_str(to, from, length) memcpy((to), (from), (length))
#endif /* CPU_BIG_ENDIAN */

/* ROTL/ROTR macros rotate a 32/64-bit word left/right by n bits */
#define ROTL32(dword, n) ((dword) << (n) ^ ((dword) >> (32 - (n))))
#define ROTR32(dword, n) ((dword) >> (n) ^ ((dword) << (32 - (n))))
#define ROTL64(qword, n) ((qword) << (n) ^ ((qword) >> (64 - (n))))
#define ROTR64(qword, n) ((qword) >> (n) ^ ((qword) << (64 - (n))))

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* BYTE_ORDER_H */
