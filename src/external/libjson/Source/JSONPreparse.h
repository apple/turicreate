#ifndef JSON_PREPARSE_H
#define JSON_PREPARSE_H

#include "JSONDebug.h"
#include "JSONNode.h"

#if (defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY))

#ifdef JSON_COMMENTS
	#define COMMENT_PARAM(name) ,const json_string & name
#else
	#define COMMENT_PARAM(name)
#endif

class JSONPreparse {
public:
    static JSONNode isValidNumber(json_string::const_iterator & ptr, json_string::const_iterator & end) json_read_priority;
    static JSONNode isValidMember(json_string::const_iterator & ptr, json_string::const_iterator & end) json_read_priority;
    static json_string isValidString(json_string::const_iterator & ptr, json_string::const_iterator & end) json_read_priority;
    static void isValidNamedObject(json_string::const_iterator & ptr, json_string::const_iterator & end, JSONNode & parent COMMENT_PARAM(comment)) json_read_priority;
    static JSONNode isValidObject(json_string::const_iterator & ptr, json_string::const_iterator & end) json_read_priority;
    static JSONNode isValidArray(json_string::const_iterator & ptr, json_string::const_iterator & end) json_read_priority;
    static JSONNode isValidRoot(const json_string & json) json_throws(std::invalid_argument) json_read_priority;
};

#endif

#endif
