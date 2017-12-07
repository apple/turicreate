#ifndef JSON_BASE64_H
#define JSON_BASE64_H

#include "JSONDebug.h"
#if defined(JSON_BINARY) || defined(JSON_EXPOSE_BASE64)  //if this is not needed, don't waste space compiling it

#include "../Dependencies/libbase64++/libbase64++.h"

class JSONBase64 {
public:
    inline static json_string json_encode64(const unsigned char * binary, size_t bytes) json_nothrow json_cold;
    inline static std::string json_decode64(const json_string & encoded) json_nothrow json_cold;
private: 
    JSONBase64(void);
};

json_string JSONBase64::json_encode64(const unsigned char * binary, size_t bytes) json_nothrow {
    #if defined JSON_DEBUG || defined JSON_SAFE
        return libbase64::encode<json_string, json_char, json_uchar, true>(binary, bytes);
    #else
	    return libbase64::encode<json_string, json_char, json_uchar, false>(binary, bytes);
    #endif
}

std::string JSONBase64::json_decode64(const json_string & encoded) json_nothrow {
    #if defined JSON_DEBUG || defined JSON_SAFE
        return libbase64::decode<json_string, json_char, json_uchar, true>(encoded);
    #else
    	return libbase64::decode<json_string, json_char, json_uchar, false>(encoded);
    #endif
}


#endif
#endif
