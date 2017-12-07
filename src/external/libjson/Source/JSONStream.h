#ifndef JSONSTREAM_H
#define JSONSTREAM_H

#include "JSONDebug.h"

#ifdef JSON_STREAM

#ifdef JSON_LESS_MEMORY
	#ifdef __GNUC__
		#pragma pack(push, 1)
	#elif _MSC_VER
		#pragma pack(push, JSONStream_pack, 1)
	#endif
#endif

#ifdef JSON_MEMORY_CALLBACKS
#include "JSONMemory.h"
#endif

#ifndef JSON_LIBRARY
class JSONNode; //foreward declaration
typedef void (*json_stream_callback_t)(JSONNode &, void *);
#endif

class JSONStream {
public:
    JSONStream(json_stream_callback_t call_p, json_stream_e_callback_t call_e = NULL, void * callbackIdentifier = JSONSTREAM_SELF) json_nothrow;
    JSONStream(const JSONStream & orig) json_nothrow;
    JSONStream & operator =(const JSONStream & orig) json_nothrow;
#ifdef JSON_LIBRARY
	JSONStream & operator << (const json_char * str) json_nothrow;
#else
	JSONStream & operator << (const json_string & str) json_nothrow;
#endif
	
    static void deleteJSONStream(JSONStream * stream) json_nothrow {
#ifdef JSON_MEMORY_CALLBACKS
		stream -> ~JSONStream();
		libjson_free<JSONStream>(stream);
#else
		delete stream;
#endif
    }
	
    static JSONStream * newJSONStream(json_stream_callback_t callback, json_stream_e_callback_t call_e, void * callbackIdentifier) json_nothrow {
#ifdef JSON_MEMORY_CALLBACKS
		return new(json_malloc<JSONStream>(1)) JSONStream(callback, call_e, callbackIdentifier);
#else
		return new JSONStream(callback, call_e, callbackIdentifier);
#endif
    }
	
	inline void reset() json_nothrow {
		state = true;
		buffer.clear();
	}
JSON_PRIVATE
	inline void * getIdentifier(void) json_nothrow {
		if (callback_identifier == JSONSTREAM_SELF){
			return (void*)this;
		}
		return callback_identifier;
	}
	
	#if (JSON_READ_PRIORITY == HIGH) && (!(defined(JSON_LESS_MEMORY)))
		template<json_char ch>
		static size_t FindNextRelevant(const json_string & value_t, const size_t pos) json_nothrow json_read_priority;
	#else
		static size_t FindNextRelevant(json_char ch, const json_string & value_t, const size_t pos) json_nothrow json_read_priority;
	#endif
	
    void parse(void) json_nothrow;
    json_string buffer;
    json_stream_callback_t call;
	json_stream_e_callback_t err_call;
	void * callback_identifier;
	bool state BITS(1);
};

#ifdef JSON_LESS_MEMORY
	#ifdef __GNUC__
		#pragma pack(pop)
	#elif _MSC_VER
		#pragma pack(pop, JSONStream_pack)
	#endif
#endif

#endif

#endif

