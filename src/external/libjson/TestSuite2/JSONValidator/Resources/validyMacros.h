#ifndef JSON_TESTSUITE_JSON_VALIDATOR_RESOURCES_VALIDITY_MACROS_H
#define JSON_TESTSUITE_JSON_VALIDATOR_RESOURCES_VALIDITY_MACROS_H

#include "../../../JSONOptions.h"

#ifdef JSON_VALIDATE

	#define assertValid(x, method, nextchar)\
		{\
			json_string temp(JSON_TEXT(x));\
			const json_char * ptr = temp.c_str();\
			assertTrue(JSONValidator::method(ptr) && ((*ptr)==JSON_TEXT(nextchar)));\
		}

	#define assertNotValid(x, method, nextchar)\
		{\
			json_string temp(JSON_TEXT(x));\
			const json_char * ptr = temp.c_str();\
			assertTrue(!JSONValidator::method(ptr)  || ((*ptr)!=JSON_TEXT(nextchar)));\
		}

	#ifdef JSON_SECURITY_MAX_NEST_LEVEL
		#define assertValid_Depth(x, method, nextchar)\
			{\
				json_string temp(JSON_TEXT(x));\
				const json_char * ptr = temp.c_str();\
				assertTrue(JSONValidator::method(ptr, 1) && ((*ptr)==JSON_TEXT(nextchar)));\
			}

		#define assertNotValid_Depth(x, method, nextchar)\
			{\
				json_string temp(JSON_TEXT(x));\
				const json_char * ptr = temp.c_str();\
				assertTrue(!JSONValidator::method(ptr, 1) || ((*ptr)!=JSON_TEXT(nextchar)));\
			}
	#else
		#define assertValid_Depth(x, method, nextchar) assertValid(x, method, nextchar)
		#define assertNotValid_Depth(x, method, nextchar) assertNotValid(x, method, nextchar)
	#endif

#else
	#define assertValid(x, method, nextchar)
	#define assertNotValid(x, method, nextchar)
	#define assertValid_Depth(x, method, nextchar)
	#define assertNotValid_Depth(x, method, nextchar)
#endif

#endif
