#ifndef JSON_GNU_C_HEADER
#define JSON_GUN_C_HEADER

#ifdef __GNUC__

    #if defined(JSON_INT_TYPE) 
        #if (JSON_INT_TYPE == long long) && defined(JSON_ISO_STRICT)
	        #error, JSON_INT_TYPE cant be a long long unless JSON_ISO_STRICT is off
        #endif
    #endif

    #define json_deprecated(method, warning) method __attribute__((deprecated))

    #if (__GNUC__ >= 3)
	   #define JSON_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #else
	   #define JSON_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
    #endif

    #if (JSON_GCC_VERSION >= 40300)
	   #define json_hot __attribute__ ((hot))
	   #define json_cold __attribute__ ((cold))
	   #define json_pure json_nothrow __attribute__ ((pure, hot))
	   #define json_malloc_attr json_nothrow __attribute__ ((malloc, hot))

	   /* Can do priorities */
	   #if (JSON_WRITE_PRIORITY == HIGH)
		  #define json_write_priority __attribute__ ((hot))
	   #elif (JSON_WRITE_PRIORITY == LOW)
		  #define json_write_priority __attribute__ ((cold))
	   #else
		  #define json_write_priority
	   #endif

	   #if (JSON_READ_PRIORITY == HIGH)
		  #define json_read_priority __attribute__ ((hot))
	   #elif (JSON_READ_PRIORITY == LOW)
		  #define json_read_priority __attribute__ ((cold))
	   #else
		  #define json_read_priority
	   #endif

	   #define json_likely(x) __builtin_expect((long)((bool)(x)),1)
	   #define json_unlikely(x) __builtin_expect((long)((bool)(x)),0)
    #else
	   #if (JSON_GCC_VERSION >= 29600)
		  #define json_pure json_nothrow __attribute__ ((pure))
		  #define json_likely(x) __builtin_expect((long)((bool)(x)),1)
		  #define json_unlikely(x) __builtin_expect((long)((bool)(x)),0)
	   #else
		  #define json_pure json_nothrow
		  #define json_likely(x) x
		  #define json_unlikely(x) x
	   #endif

	   #define json_malloc_attr json_nothrow __attribute__ ((malloc))
	   #define json_write_priority
	   #define json_read_priority
	   #define json_hot
	   #define json_cold
    #endif

    #define json_nothrow throw()
    #define json_throws(x) throw(x)

    #ifdef JSON_LESS_MEMORY
	   #define PACKED(x) :x __attribute__ ((packed))
	   #define BITS(x) :x
    #endif

#endif

#endif
