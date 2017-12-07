#ifndef JSONDEFS_H
#define JSONDEFS_H

/*
    Defines all of the types of functions and various other definitions
    that are used in C applications, this is very useful if dynamically loading
    the library instead of linking.
*/

#include "../JSONOptions.h"
#include "JSONDefs/Unknown_C.h"
#include "JSONDefs/GNU_C.h"
#include "JSONDefs/Visual_C.h"
#include "JSONDefs/Strings_Defs.h"

#define __LIBJSON_MAJOR__ 7
#define __LIBJSON_MINOR__ 6
#define __LIBJSON_PATCH__ 0
#define __LIBJSON_VERSION__ (__LIBJSON_MAJOR__ * 10000 + __LIBJSON_MINOR__ * 100 + __LIBJSON_PATCH__)

#define JSON_NULL '\0'
#define JSON_STRING '\1'
#define JSON_NUMBER '\2'
#define JSON_BOOL '\3'
#define JSON_ARRAY '\4'
#define JSON_NODE '\5'

#ifdef __cplusplus
	#if defined(JSON_MEMORY_CALLBACKS) || defined(JSON_MEMORY_POOL)
		#include "JSONAllocator.h"
	#else
		#define json_allocator std::allocator
	#endif

	#ifdef JSON_STRING_HEADER
		#include JSON_STRING_HEADER
	#else
		typedef std::basic_string<json_char, std::char_traits<json_char>, json_allocator<json_char> > json_string;
	#endif
#endif
#define JSON_MAP(x, y) std::map<x, y, std::less<x>, json_allocator<std::pair<const x, y> > >

#ifdef JSON_NO_EXCEPTIONS
    #define json_throw(x)
    #define json_try
    #define json_catch(exception, code)
	#ifdef JSON_SAFE
		#error, JSON_NO_EXCEPTIONS is not permitted with JSON_SAFE
	#endif
#else
    #define json_throw(x) throw(x)
    #define json_try try
    #define json_catch(exception, code) catch(exception){ code }
#endif

#ifdef JSON_STRICT
    #ifndef JSON_UNICODE
	   #error, JSON_UNICODE is required for JSON_STRICT
    #endif
    #ifdef JSON_COMMENTS
	   #error, JSON_COMMENTS is required to be off for JSON_STRICT
    #endif
#endif

#ifdef JSON_ISO_STRICT
    #ifdef JSON_UNICODE
	   #error, You can not use unicode under ANSI Strict C++
    #endif
#else
    #ifdef __GNUC__
	   #ifdef __STRICT_ANSI__
		  #warning, Using -ansi GCC option, but JSON_ISO_STRICT not on, turning it on for you
		  #define JSON_ISO_STRICT
	   #endif
    #endif
#endif


#ifdef JSON_NUMBER_TYPE
	typedef JSON_NUMBER_TYPE json_number
	#define JSON_FLOAT_THRESHHOLD 0.00001
#else
	#ifdef JSON_LESS_MEMORY
		typedef float json_number;
		#define JSON_FLOAT_THRESHHOLD 0.00001f
	#else
		typedef double json_number;
		#define JSON_FLOAT_THRESHHOLD 0.00001
	#endif
#endif


#ifdef JSON_LESS_MEMORY
    /* PACKED and BITS stored in compiler specific headers */
    #define START_MEM_SCOPE {
    #define END_MEM_SCOPE }
#else
    #define PACKED(x)
    #define BITS(x)
    #define START_MEM_SCOPE
    #define END_MEM_SCOPE
#endif

#if defined JSON_DEBUG || defined JSON_SAFE
    #ifdef JSON_LIBRARY
	   typedef void (*json_error_callback_t)(const json_char *);
    #else
	   typedef void (*json_error_callback_t)(const json_string &);
    #endif
#endif

#ifdef JSON_INDEX_TYPE
    typedef JSON_INDEX_TYPE json_index_t;
#else
    typedef unsigned int json_index_t;
#endif

#ifdef JSON_BOOL_TYPE
    typedef JSON_BOOL_TYPE json_bool_t;
#else
    typedef int json_bool_t;
#endif

#ifdef JSON_INT_TYPE
    typedef JSON_INT_TYPE json_int_t;
#else
    typedef long json_int_t;
#endif

#define JSONSTREAM_SELF (void*)-1
typedef void (*json_stream_e_callback_t)(void * identifier);

typedef void (*json_mutex_callback_t)(void *);
typedef void (*json_free_t)(void *);
#ifndef JSON_LIBRARY
    typedef void * (*json_malloc_t)(size_t);
    typedef void * (*json_realloc_t)(void *, size_t);
#else
    #define JSONNODE void  /* so that JSONNODE* is void* */
    typedef JSONNODE** JSONNODE_ITERATOR;
    #ifdef JSON_STREAM
	   #define JSONSTREAM void
	    typedef void (*json_stream_callback_t)(JSONNODE *, void * identifier);
    #endif
    typedef void * (*json_malloc_t)(unsigned long);
    typedef void * (*json_realloc_t)(void *, unsigned long);
#endif


#ifdef JSON_UNIT_TEST
    #define JSON_PRIVATE public:
    #define JSON_PROTECTED public:
#else
    #define JSON_PRIVATE private:
    #define JSON_PROTECTED protected:
#endif
#ifdef JSON_STREAM
    #ifndef JSON_READ_PRIORITY
	   #error, JSON_STREAM also requires JSON_READ_PRIORITY
    #endif
#endif
#ifdef JSON_VALIDATE
    #ifndef JSON_READ_PRIORITY
	   #error, JSON_VALIDATE also requires JSON_READ_PRIORITY
    #endif
#endif

#define JSON_TEMP_COMMENT_IDENTIFIER JSON_TEXT('#')

#endif
