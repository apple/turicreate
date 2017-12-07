#ifndef JSON_VALIDATOR_H
#define JSON_VALIDATOR_H

#include "JSONDebug.h"

#ifdef JSON_VALIDATE

#ifdef JSON_SECURITY_MAX_NEST_LEVEL
    #define DEPTH_PARAM ,size_t depth_param
    #define DEPTH_ARG(arg) ,arg
    #define INC_DEPTH()\
    if (++depth_param > JSON_SECURITY_MAX_NEST_LEVEL){\
	   JSON_FAIL(JSON_TEXT("Exceeded JSON_SECURITY_MAX_NEST_LEVEL"));\
	   return false;\
    }
#else
    #define DEPTH_PARAM
    #define DEPTH_ARG(arg)
    #define INC_DEPTH() (void)0
#endif

class JSONValidator {
    public:
	   static bool isValidNumber(const json_char * & ptr) json_nothrow json_read_priority;
	   static bool isValidMember(const json_char * & ptr  DEPTH_PARAM) json_nothrow json_read_priority;
	   static bool isValidString(const json_char * & ptr) json_nothrow json_read_priority;
	   static bool isValidNamedObject(const json_char * & ptr  DEPTH_PARAM) json_nothrow json_read_priority;
	   static bool isValidObject(const json_char * & ptr  DEPTH_PARAM) json_nothrow json_read_priority;
	   static bool isValidArray(const json_char * & ptr  DEPTH_PARAM) json_nothrow json_read_priority;
	   static bool isValidRoot(const json_char * json) json_nothrow json_read_priority;
		#ifdef JSON_STREAM
			static bool isValidPartialRoot(const json_char * json) json_nothrow json_read_priority;
		#endif
};

#endif

#endif
