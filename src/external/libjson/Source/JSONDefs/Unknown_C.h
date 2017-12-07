#ifndef JSON_UNKNOWN_C_HEADER
#define JSON_UNKNOWN_C_HEADER

#if !defined(__GNUC__) && !defined(_MSC_VER)

    #define json_deprecated(method, warning) method

    #define json_nothrow
    #define json_throws(x)
    #define json_pure json_nothrow
    #define json_read_priority
    #define json_write_priority
    #define json_malloc_attr json_nothrow
    #define json_hot
    #define json_cold
    #define json_likely(x) x
    #define json_unlikely(x) x

    #ifdef JSON_LESS_MEMORY
	   #define PACKED(x) :x
	   #define BITS(x) :x
    #endif

#endif

#endif
