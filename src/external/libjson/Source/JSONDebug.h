#ifndef JSON_DEBUG_H
#define JSON_DEBUG_H

#include "JSONDefs.h"

#ifdef JSON_DEBUG
    #ifdef JSON_SAFE
	   #define JSON_ASSERT_SAFE(condition, msg, code)\
		  {\
			 if (json_unlikely(!(condition))){\
				JSON_FAIL(msg);\
				code\
			 }\
		  }
	   #define JSON_FAIL_SAFE(msg, code)\
		  {\
			 JSON_FAIL(msg);\
			 code\
		  }
    #else
	   #define JSON_ASSERT_SAFE(condition, msg, code) JSON_ASSERT(condition, msg)
	   #define JSON_FAIL_SAFE(msg, code) JSON_FAIL(msg)
    #endif

    #define JSON_FAIL(msg) JSONDebug::_JSON_FAIL(msg)
    #define JSON_ASSERT(bo, msg) JSONDebug::_JSON_ASSERT(bo, msg)

    class JSONDebug {
    public:
	   #ifndef JSON_STDERROR
		  static json_error_callback_t register_callback(json_error_callback_t callback) json_nothrow json_cold;
	   #endif
	   static void _JSON_FAIL(const json_string & msg) json_nothrow json_cold;
	   static void _JSON_ASSERT(bool condition, const json_string & msg) json_nothrow json_cold;
    };
#else
    #ifdef JSON_SAFE
	   #define JSON_ASSERT_SAFE(condition, msg, code)\
		  {\
			 if (json_unlikely(!(condition))){\
				code\
			 }\
		  }
	   #define JSON_FAIL_SAFE(msg, code)\
		  {\
			 code\
		  }
    #else
	   #define JSON_ASSERT_SAFE(condition, msg, code)
	   #define JSON_FAIL_SAFE(msg, code)
    #endif

    #define JSON_ASSERT(condition, msg)
    #define JSON_FAIL(msg)
#endif

#endif

