/** @file rhash.h LibRHash interface */
#ifndef RHASH_H
#define RHASH_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RHASH_API
/* modifier for LibRHash functions */
# define RHASH_API
#endif

/**
 * Identifiers of supported hash functions.
 * The rhash_init() function allows mixing several ids using
 * binary OR, to calculate several hash functions for one message.
 */
enum rhash_ids
{
#if 0
	RHASH_CRC32 = 0x01,
	RHASH_MD4   = 0x02,
	RHASH_MD5   = 0x04,
	RHASH_SHA1  = 0x08,
	RHASH_TIGER = 0x10,
	RHASH_TTH   = 0x20,
	RHASH_BTIH  = 0x40,
	RHASH_ED2K  = 0x80,
	RHASH_AICH  = 0x100,
	RHASH_WHIRLPOOL = 0x200,
	RHASH_RIPEMD160 = 0x400,
	RHASH_GOST      = 0x800,
	RHASH_GOST_CRYPTOPRO = 0x1000,
	RHASH_HAS160    = 0x2000,
	RHASH_SNEFRU128 = 0x4000,
	RHASH_SNEFRU256 = 0x8000,
	RHASH_SHA224    = 0x10000,
	RHASH_SHA256    = 0x20000,
	RHASH_SHA384    = 0x40000,
	RHASH_SHA512    = 0x80000,
	RHASH_EDONR256  = 0x0100000,
	RHASH_EDONR512  = 0x0200000,
	RHASH_SHA3_224  = 0x0400000,
	RHASH_SHA3_256  = 0x0800000,
	RHASH_SHA3_384  = 0x1000000,
	RHASH_SHA3_512  = 0x2000000,

	/** The bit-mask containing all supported hashe functions */
	RHASH_ALL_HASHES = RHASH_CRC32 | RHASH_MD4 | RHASH_MD5 | RHASH_ED2K | RHASH_SHA1 |
		RHASH_TIGER | RHASH_TTH | RHASH_GOST | RHASH_GOST_CRYPTOPRO |
		RHASH_BTIH | RHASH_AICH | RHASH_WHIRLPOOL | RHASH_RIPEMD160 |
		RHASH_HAS160 | RHASH_SNEFRU128 | RHASH_SNEFRU256 |
		RHASH_SHA224 | RHASH_SHA256 | RHASH_SHA384 | RHASH_SHA512 |
		RHASH_SHA3_224 | RHASH_SHA3_256 | RHASH_SHA3_384 | RHASH_SHA3_512 |
		RHASH_EDONR256 | RHASH_EDONR512,

	/** The number of supported hash functions */
	RHASH_HASH_COUNT = 26
#else
	RHASH_MD5        = 0x01,
	RHASH_SHA1       = 0x02,
	RHASH_SHA224     = 0x04,
	RHASH_SHA256     = 0x08,
	RHASH_SHA384     = 0x10,
	RHASH_SHA512     = 0x20,
	RHASH_SHA3_224   = 0x40,
	RHASH_SHA3_256   = 0x80,
	RHASH_SHA3_384   = 0x100,
	RHASH_SHA3_512   = 0x200,
	RHASH_ALL_HASHES =
		RHASH_MD5 |
		RHASH_SHA1 |
		RHASH_SHA224 |
		RHASH_SHA256 |
		RHASH_SHA384 |
		RHASH_SHA512 |
		RHASH_SHA3_224 |
		RHASH_SHA3_256 |
		RHASH_SHA3_384 |
		RHASH_SHA3_512,
	RHASH_HASH_COUNT = 10
#endif
};

/**
 * The rhash context structure contains contexts for several hash functions
 */
typedef struct rhash_context
{
	/** The size of the hashed message */
	unsigned long long msg_size;

	/**
	 * The bit-mask containing identifiers of the hashes being calculated
	 */
	unsigned hash_id;
} rhash_context;

#ifndef LIBRHASH_RHASH_CTX_DEFINED
#define LIBRHASH_RHASH_CTX_DEFINED
/**
 * Hashing context.
 */
typedef struct rhash_context* rhash;
#endif /* LIBRHASH_RHASH_CTX_DEFINED */

/** type of a callback to be called periodically while hashing a file */
typedef void (*rhash_callback_t)(void* data, unsigned long long offset);

RHASH_API void rhash_library_init(void); /* initialize static data */

/* hi-level hashing functions */
RHASH_API int rhash_msg(unsigned hash_id, const void* message, size_t length, unsigned char* result);
RHASH_API int rhash_file(unsigned hash_id, const char* filepath, unsigned char* result);
RHASH_API int rhash_file_update(rhash ctx, FILE* fd);

#ifdef _WIN32 /* windows only function */
RHASH_API int rhash_wfile(unsigned hash_id, const wchar_t* filepath, unsigned char* result);
#endif

/* lo-level interface */
RHASH_API rhash rhash_init(unsigned hash_id);
/*RHASH_API rhash rhash_init_by_ids(unsigned hash_ids[], unsigned count);*/
RHASH_API int  rhash_update(rhash ctx, const void* message, size_t length);
RHASH_API int  rhash_final(rhash ctx, unsigned char* first_result);
RHASH_API void rhash_reset(rhash ctx); /* reinitialize the context */
RHASH_API void rhash_free(rhash ctx);

/* additional lo-level functions */
RHASH_API void  rhash_set_callback(rhash ctx, rhash_callback_t callback, void* callback_data);

/** bit-flag: default hash output format is base32 */
#define RHASH_INFO_BASE32 1

/**
 * Information about a hash function.
 */
typedef struct rhash_info
{
	/** hash function indentifier */
	unsigned hash_id;
	/** flags bit-mask, including RHASH_INFO_BASE32 bit */
	unsigned flags;
	/** size of binary message digest in bytes */
	size_t digest_size;
	const char* name;
	const char* magnet_name;
} rhash_info;

/* information functions */
RHASH_API int  rhash_count(void); /* number of supported hashes */
RHASH_API int  rhash_get_digest_size(unsigned hash_id); /* size of binary message digest */
RHASH_API int  rhash_get_hash_length(unsigned hash_id); /* length of formatted hash string */
RHASH_API int  rhash_is_base32(unsigned hash_id); /* default digest output format */
RHASH_API const char* rhash_get_name(unsigned hash_id); /* get hash function name */
RHASH_API const char* rhash_get_magnet_name(unsigned hash_id); /* get name part of magnet urn */

/* note, that rhash_info_by_id() is not exported to a shared library or DLL */
const rhash_info* rhash_info_by_id(unsigned hash_id); /* get hash sum info by hash id */

#if 0
/**
 * Flags for printing a hash sum
 */
enum rhash_print_sum_flags
{
	/** print in a default format */
	RHPR_DEFAULT   = 0x0,
	/** output as binary message digest */
	RHPR_RAW       = 0x1,
	/** print as a hexadecimal string */
	RHPR_HEX       = 0x2,
	/** print as a base32-encoded string */
	RHPR_BASE32    = 0x3,
	/** print as a base64-encoded string */
	RHPR_BASE64    = 0x4,

	/**
	 * Print as an uppercase string. Can be used
	 * for base32 or hexadecimal format only.
	 */
	RHPR_UPPERCASE = 0x8,

	/**
	 * Reverse hash bytes. Can be used for GOST hash.
	 */
	RHPR_REVERSE   = 0x10,

	/** don't print 'magnet:?' prefix in rhash_print_magnet */
	RHPR_NO_MAGNET  = 0x20,
	/** print file size in rhash_print_magnet */
	RHPR_FILESIZE  = 0x40,
};
#endif

/* output hash into the given buffer */
RHASH_API size_t rhash_print_bytes(char* output,
	const unsigned char* bytes, size_t size, int flags);

RHASH_API size_t rhash_print(char* output, rhash ctx, unsigned hash_id,
	int flags);

/* output magnet URL into the given buffer */
RHASH_API size_t rhash_print_magnet(char* output, const char* filepath,
	rhash context, unsigned hash_mask, int flags);

/* macros for message API */

/** The type of an unsigned integer large enough to hold a pointer */
#if defined(UINTPTR_MAX)
typedef uintptr_t rhash_uptr_t;
#elif defined(_LP64) || defined(__LP64__) || defined(__x86_64) || \
	defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
typedef unsigned long long rhash_uptr_t;
#else
typedef unsigned long rhash_uptr_t;
#endif

/** The value returned by rhash_transmit on error */
#define RHASH_ERROR ((rhash_uptr_t)-1)
/** Convert a pointer to rhash_uptr_t */
#define RHASH_STR2UPTR(str) ((rhash_uptr_t)(char*)(str))
/** Convert a rhash_uptr_t to a void* pointer */
#define RHASH_UPTR2PVOID(u) ((void*)((char*)0 + (u)))

/* rhash API to set/get data via messages */
RHASH_API rhash_uptr_t rhash_transmit(
	unsigned msg_id, void* dst, rhash_uptr_t ldata, rhash_uptr_t rdata);

/* rhash message constants */

#define RMSG_GET_CONTEXT 1
#define RMSG_CANCEL      2
#define RMSG_IS_CANCELED 3
#define RMSG_GET_FINALIZED 4
#define RMSG_SET_AUTOFINAL 5
#define RMSG_SET_OPENSSL_MASK 10
#define RMSG_GET_OPENSSL_MASK 11

/* helper macros */

/** Get a pointer to context of the specified hash function */
#define rhash_get_context_ptr(ctx, hash_id) RHASH_UPTR2PVOID(rhash_transmit(RMSG_GET_CONTEXT, ctx, hash_id, 0))
/** Cancel hash calculation of a file */
#define rhash_cancel(ctx) rhash_transmit(RMSG_CANCEL, ctx, 0, 0)
/** Return non-zero if hash calculation was canceled, zero otherwise */
#define rhash_is_canceled(ctx) rhash_transmit(RMSG_IS_CANCELED, ctx, 0, 0)
/** Return non-zero if rhash_final was called for rhash_context */
#define rhash_get_finalized(ctx) rhash_transmit(RMSG_GET_FINALIZED, ctx, 0, 0)

/**
 * Turn on/off the auto-final flag for the given rhash_context. By default
 * auto-final is on, which means rhash_final is called automatically, if
 * needed when a hash value is retrived by rhash_print call.
 */
#define rhash_set_autofinal(ctx, on) rhash_transmit(RMSG_SET_AUTOFINAL, ctx, on, 0)

/**
 * Set the bit-mask of hash algorithms to be calculated by OpenSSL library.
 * The call rhash_set_openssl_mask(0) made before rhash_library_init(),
 * turns off loading of the OpenSSL dynamic library.
 * This call works if the LibRHash was compiled with OpenSSL support.
 */
#define rhash_set_openssl_mask(mask) rhash_transmit(RMSG_SET_OPENSSL_MASK, NULL, mask, 0)

/**
 * Return current bit-mask of hash algorithms selected to be calculated
 * by OpenSSL library.
 */
#define rhash_get_openssl_mask() rhash_transmit(RMSG_GET_OPENSSL_MASK, NULL, 0, 0)

/** The bit mask of hash algorithms implemented by OpenSSL */
#if defined(USE_OPENSSL) || defined(OPENSSL_RUNTIME)
# define RHASH_OPENSSL_SUPPORTED_HASHES (RHASH_MD4 | RHASH_MD5 | \
	RHASH_SHA1 | RHASH_SHA224 | RHASH_SHA256 | RHASH_SHA384 | \
	RHASH_SHA512 | RHASH_RIPEMD160 | RHASH_WHIRLPOOL)
#else
# define RHASH_OPENSSL_SUPPORTED_HASHES 0
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* RHASH_H */
