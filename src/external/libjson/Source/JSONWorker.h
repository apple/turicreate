#ifndef JSON_WORKER_H
#define JSON_WORKER_H

#include "JSONNode.h"
#include "JSONSharedString.h"

class JSONWorker {
public:
    static json_string RemoveWhiteSpaceAndComments(const json_string & value_t, bool escapeQuotes) json_nothrow json_read_priority;
	static json_char * RemoveWhiteSpaceAndCommentsC(const json_string & value_t, bool escapeQuotes) json_nothrow json_read_priority;

    #ifdef JSON_READ_PRIORITY
	   static JSONNode parse(const json_string & json) json_throws(std::invalid_argument) json_read_priority;
	   static JSONNode parse_unformatted(const json_string & json) json_throws(std::invalid_argument) json_read_priority;

		static JSONNode _parse_unformatted(const json_char * json, const json_char * const end) json_throws(std::invalid_argument) json_read_priority;

		static json_char * RemoveWhiteSpace(const json_string & value_t, size_t & len, bool escapeQuotes) json_nothrow json_read_priority;

	   static void DoArray(const internalJSONNode * parent, const json_string & value_t) json_nothrow json_read_priority;
	   static void DoNode(const internalJSONNode * parent, const json_string & value_t) json_nothrow json_read_priority;

	   #ifdef JSON_LESS_MEMORY
		  #define NAME_ENCODED this, true
		  #define STRING_ENCODED this, false
		  static json_string FixString(const json_string & value_t, const internalJSONNode * flag, bool which) json_nothrow json_read_priority;
	   #else
		  #define NAME_ENCODED _name_encoded
		  #define STRING_ENCODED _string_encoded
		  static json_string FixString(const json_string & value_t, bool & flag) json_nothrow json_read_priority;
	   #endif
    #endif

    #if defined(JSON_READ_PRIORITY) || defined(JSON_STREAM)
		#if (JSON_READ_PRIORITY == HIGH) && (!(defined(JSON_LESS_MEMORY)))
			template<json_char ch>
			static size_t FindNextRelevant(const json_string & value_t, const size_t pos) json_nothrow json_read_priority;
		#else
			static size_t FindNextRelevant(json_char ch, const json_string & value_t, const size_t pos) json_nothrow json_read_priority;
		#endif
    #endif
    static void UnfixString(const json_string & value_t, bool flag, json_string & res) json_nothrow;
JSON_PRIVATE
    #ifdef JSON_READ_PRIORITY
	   static json_char Hex(const json_char * & pos) json_nothrow;
	   static json_uchar UTF8(const json_char * & pos, const json_char * const end) json_nothrow;
    #endif
    #ifdef JSON_ESCAPE_WRITES
	   static json_string toUTF8(json_uchar p) json_nothrow;
    #endif
    #ifdef JSON_UNICODE
	   static void UTF(const json_char * & pos, json_string & result, const json_char * const end) json_nothrow;
	   #ifdef JSON_ESCAPE_WRITES
		  static json_string toSurrogatePair(json_uchar pos) json_nothrow;
	   #endif
    #endif
    #ifdef JSON_READ_PRIORITY
	   static void SpecialChar(const json_char * & pos, const json_char * const end, json_string & res) json_nothrow;
	   static void NewNode(const internalJSONNode * parent, const json_string & name, const json_string & value, bool array) json_nothrow;
    #endif
private:
    JSONWorker(void);
};

#endif
