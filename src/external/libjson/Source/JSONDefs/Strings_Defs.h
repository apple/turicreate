#ifndef STRINGS_DEFS_HEADER
#define STRINGS_DEFS_HEADER

#include "../../JSONOptions.h"

#ifdef JSON_UNICODE
    #define json_char wchar_t
    #define json_uchar wchar_t
    #ifdef __cplusplus
	   #include <cwchar>  /* need wide characters */
	   #ifndef JSON_STRING_HEADER
		  #include <string>
	   #endif
    #else
	   #include <wchar.h>  /* need wide characters */
    #endif
    #define JSON_TEXT(s) L ## s
    #define json_strlen wcslen
    #define json_strcmp wcscmp
#else
    #define json_char char
    #define json_uchar unsigned char
    #ifdef __cplusplus
	   #ifndef JSON_STRING_HEADER
		  #include <string>
	   #endif
    #else
	   #include <string.h> /* still need it for strlen and such */
    #endif
    #define JSON_TEXT(s) s
    #define json_strlen strlen
    #define json_strcmp strcmp
#endif


#endif
