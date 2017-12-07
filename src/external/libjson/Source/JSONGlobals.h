#ifndef JSON_GLOBALS_H
#define JSON_GLOBALS_H

#include "JSONDefs.h"

/*
 *	The use of singletons for globals makes globals not
 *	actually be initialized until it is first needed, this
 *	makes the library faster to load, and have a smaller
 *	memory footprint
 */

#define json_global_decl(TYPE, NAME, VALUE)						\
class jsonSingleton ## NAME {									\
public:															\
	inline static TYPE & getValue() json_nothrow {				\
		static jsonSingleton ## NAME single;					\
		return single.val;										\
	}															\
protected:														\
	inline jsonSingleton ## NAME() json_nothrow : val(VALUE) {}	\
	TYPE val;													\
}


#define json_global(NAME) jsonSingleton ## NAME::getValue()

#include <string>
json_global_decl(json_string, EMPTY_JSON_STRING, );
json_global_decl(std::string, EMPTY_STD_STRING, );

json_global_decl(json_string, CONST_TRUE, JSON_TEXT("true"));
json_global_decl(json_string, CONST_FALSE, JSON_TEXT("false"));
json_global_decl(json_string, CONST_NULL, JSON_TEXT("null"));

#ifndef JSON_NEWLINE
	json_global_decl(json_string, NEW_LINE, JSON_TEXT("\n"));
#else
	json_global_decl(json_string, NEW_LINE, JSON_TEXT(JSON_NEWLINE));
#endif

#ifdef JSON_WRITE_BASH_COMMENTS
	json_global_decl(json_string, SINGLELINE_COMMENT, JSON_TEXT("#"));
#else
	json_global_decl(json_string, SINGLELINE_COMMENT, JSON_TEXT("//"));
#endif

#ifdef JSON_INDENT
	json_global_decl(json_string, INDENT, JSON_TEXT(JSON_INDENT));
#endif

#ifdef JSON_MUTEX_CALLBACKS
	#include <map>
	json_global_decl(JSON_MAP(void *, unsigned int), MUTEX_MANAGER, );
	json_global_decl(JSON_MAP(int, JSON_MAP(void *, unsigned int) ), THREAD_LOCKS, );
#endif

#ifdef JSON_LIBRARY
	#ifdef JSON_MEMORY_MANAGE
		#include "JSONMemory.h"
		json_global_decl(auto_expand, STRING_HANDLER, );
		json_global_decl(auto_expand_node, NODE_HANDLER, );
		#ifdef JSON_STREAM
			json_global_decl(auto_expand_stream, STREAM_HANDLER, );
		#endif
	#endif
#endif

//These are common error responses
json_global_decl(json_string, ERROR_TOO_LONG, JSON_TEXT("Exceeding JSON_SECURITY_MAX_STRING_LENGTH"));
json_global_decl(json_string, ERROR_UNKNOWN_LITERAL, JSON_TEXT("Unknown JSON literal: "));
json_global_decl(json_string, ERROR_NON_CONTAINER, JSON_TEXT("Calling container method on non-container: "));
json_global_decl(json_string, ERROR_NON_ITERATABLE, JSON_TEXT("Calling iterator method on non-iteratable: "));
json_global_decl(json_string, ERROR_NULL_IN_CHILDREN, JSON_TEXT("a null pointer within the children"));
json_global_decl(json_string, ERROR_UNDEFINED, JSON_TEXT("Undefined results: "));
json_global_decl(json_string, ERROR_LOWER_RANGE, JSON_TEXT(" is outside the lower range of "));
json_global_decl(json_string, ERROR_UPPER_RANGE, JSON_TEXT(" is outside the upper range of "));
json_global_decl(json_string, ERROR_NOT_BASE64, JSON_TEXT("Not base64"));
json_global_decl(json_string, ERROR_OUT_OF_MEMORY, JSON_TEXT("Out of memory"));

#endif
