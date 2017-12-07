#include "JSONNode.h"
#ifdef JSON_WRITE_PRIORITY
#include "JSONWorker.h"
#include "JSONGlobals.h"

extern bool used_ascii_one;

#ifdef JSON_INDENT
    inline json_string makeIndent(unsigned int amount) json_nothrow json_write_priority;
    inline json_string makeIndent(unsigned int amount) json_nothrow {
	   if (amount == 0xFFFFFFFF) return json_global(EMPTY_JSON_STRING);
	   json_string result;
	   result.reserve(amount * json_global(INDENT).length());
	   for(unsigned int i = 0; i < amount; ++i){
		  result += json_global(INDENT);
	   }
	   JSON_ASSERT(result.capacity == amount * json_global(INDENT).length(), JSON_TEXT("makeIndent made a string too big"));
	   return result;
    }
#else
    inline json_string makeIndent(unsigned int amount) json_nothrow {
		if (amount == 0xFFFFFFFF) return json_global(EMPTY_JSON_STRING);
		if (json_likely(amount < 8)){
			static const json_string cache[] = {
				json_string(),
				json_string(JSON_TEXT("\t")),
				json_string(JSON_TEXT("\t\t")),
				json_string(JSON_TEXT("\t\t\t")),
				json_string(JSON_TEXT("\t\t\t\t")),
				json_string(JSON_TEXT("\t\t\t\t\t")),
				json_string(JSON_TEXT("\t\t\t\t\t\t")),
				json_string(JSON_TEXT("\t\t\t\t\t\t\t"))
			};
			return cache[amount];
		}
		#ifndef JSON_LESS_MEMORY
			if (json_likely(amount < 16)){
				static const json_string cache[] = {
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
					json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"))
				};
				return cache[amount - 8];
			}
			#if JSON_WRITE_PRIORITY == HIGH
				if (json_likely(amount < 24)){
					static const json_string cache[] = {
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t")),
						json_string(JSON_TEXT("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"))
					};
					return cache[amount - 16];
				}
			#endif
		#endif
		return json_string(amount, JSON_TEXT('\t'));
    }
#endif

void internalJSONNode::WriteName(bool formatted, bool arrayChild, json_string & output) const json_nothrow {
    if (!arrayChild){
		output += JSON_TEXT("\"");
		JSONWorker::UnfixString(_name, _name_encoded, output);
		output += ((formatted) ? JSON_TEXT("\" : ") : JSON_TEXT("\":"));
    }
}

void internalJSONNode::WriteChildren(unsigned int indent, json_string & output) const json_nothrow {
    //Iterate through the children and write them
    if (json_likely(CHILDREN -> empty())) return;

    json_string indent_plus_one;
    //handle whether or not it's formatted JSON
    if (indent != 0xFFFFFFFF){  //it's formatted, make the indentation strings
	   indent_plus_one = json_global(NEW_LINE) + makeIndent(++indent);
    }

    //else it's not formatted, leave the indentation strings empty
    const size_t size_minus_one = CHILDREN -> size() - 1;
    size_t i = 0;
    JSONNode ** it = CHILDREN -> begin();
    for(JSONNode ** it_end = CHILDREN -> end(); it != it_end; ++it, ++i){

		output += indent_plus_one;
		(*it) -> internal -> Write(indent, type() == JSON_ARRAY, output);
	    if (json_likely(i < size_minus_one)) output += JSON_TEXT(',');  //the last one does not get a comma, but all of the others do
    }
    if (indent != 0xFFFFFFFF){
		output += json_global(NEW_LINE);
		output += makeIndent(indent - 1);
    }
}

#ifdef JSON_ARRAY_SIZE_ON_ONE_LINE
    void internalJSONNode::WriteChildrenOneLine(unsigned int indent, json_string & output) const json_nothrow {
	   //Iterate through the children and write them
	   if (json_likely(CHILDREN -> empty())) return json_global(EMPTY_JSON_STRING);
	   if ((*CHILDREN -> begin()) -> internal -> isContainer()) return WriteChildren(indent);

	   json_string comma(JSON_TEXT(","));
	   if (indent != 0xFFFFFFFF){
		  comma += JSON_TEXT(' ');
	   }

	   //else it's not formatted, leave the indentation strings empty
	   const size_t size_minus_one = CHILDREN -> size() - 1;
	   size_t i = 0;
	   JSONNode ** it = CHILDREN -> begin();
	   for(JSONNode ** it_end = CHILDREN -> end(); it != it_end; ++it, ++i){
		  (*it) -> internal -> Write(indent, type() == JSON_ARRAY, output);
		  if (json_likely(i < size_minus_one)) output += comma;  //the last one does not get a comma, but all of the others do
	   }
    }
#endif

#ifdef JSON_COMMENTS
    void internalJSONNode::WriteComment(unsigned int indent, json_string & output) const json_nothrow {
	   if (indent == 0xFFFFFFFF) return;
	   if (json_likely(_comment.empty())) return;
	   size_t pos = _comment.find(JSON_TEXT('\n'));
		
	   const json_string current_indent(json_global(NEW_LINE) + makeIndent(indent));
		
	   if (json_likely(pos == json_string::npos)){  //Single line comment
		   output += current_indent;
		   output += json_global(SINGLELINE_COMMENT);
		   output.append(_comment.begin(), _comment.end());
		   output += current_indent;
		   return;
	   }

	   /*
	    Multiline comments
	    */
		output += current_indent;
	   #if !(defined(JSON_WRITE_BASH_COMMENTS) || defined(JSON_WRITE_SINGLE_LINE_COMMENTS))
		  const json_string current_indent_plus_one(json_global(NEW_LINE) + makeIndent(indent + 1));
		  output += JSON_TEXT("/*");
		  output += current_indent_plus_one;
	   #endif
	   size_t old = 0;
	   while(pos != json_string::npos){
		  if (json_unlikely(pos && _comment[pos - 1] == JSON_TEXT('\r'))) --pos;
		  #if defined(JSON_WRITE_BASH_COMMENTS) || defined(JSON_WRITE_SINGLE_LINE_COMMENTS)
			 output += json_global(SINGLELINE_COMMENT);
		  #endif
		  output.append(_comment.begin() + old, _comment.begin() + pos);
		  
		  #if defined(JSON_WRITE_BASH_COMMENTS) || defined(JSON_WRITE_SINGLE_LINE_COMMENTS)
			 output += current_indent;
		  #else
			 output += current_indent_plus_one;
		  #endif
		  old = (_comment[pos] == JSON_TEXT('\r')) ? pos + 2 : pos + 1;
		  pos = _comment.find(JSON_TEXT('\n'), old);
	   }
	   #if defined(JSON_WRITE_BASH_COMMENTS) || defined(JSON_WRITE_SINGLE_LINE_COMMENTS)
		  output += json_global(SINGLELINE_COMMENT);
	   #endif
	   output.append(_comment.begin() + old, _comment.end());
	   output += current_indent;
	   #if !(defined(JSON_WRITE_BASH_COMMENTS) || defined(JSON_WRITE_SINGLE_LINE_COMMENTS))
		  output += JSON_TEXT("*/");
		  output += current_indent;
	   #endif
    }
#else
    inline void internalJSONNode::WriteComment(unsigned int, json_string &) const json_nothrow {}
#endif

void internalJSONNode::DumpRawString(json_string & output) const json_nothrow {
	//first remove the \1 characters
	if (used_ascii_one){  //if it hasn't been used yet, don't bother checking
		json_string result(_string.begin(), _string.end());
		for(json_string::iterator beg = result.begin(), en = result.end(); beg != en; ++beg){
			if (*beg == JSON_TEXT('\1')) *beg = JSON_TEXT('\"');
		}
		output += result;
		return;
	} else {
		output.append(_string.begin(), _string.end());
	}
}

void internalJSONNode::Write(unsigned int indent, bool arrayChild, json_string & output) const json_nothrow {
    const bool formatted = indent != 0xFFFFFFFF;
	WriteComment(indent, output);
	
    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
	   if (!(formatted || fetched)){  //It's not formatted or fetched, just do a raw dump
		   WriteName(false, arrayChild, output);
           //first remove the \1 characters
		   DumpRawString(output);
		   return;
	   }
    #endif

	WriteName(formatted, arrayChild, output);
    //It's either formatted or fetched
    switch (_type){
	   case JSON_NODE:   //got members, write the members
		  Fetch();
            output += JSON_TEXT("{");
			WriteChildren(indent, output);
			output += JSON_TEXT("}");
			return;
	   case JSON_ARRAY:	   //write out the child nodes int he array
		  Fetch();
		  output += JSON_TEXT("[");
		  #ifdef JSON_ARRAY_SIZE_ON_ONE_LINE
			 if (size() <= JSON_ARRAY_SIZE_ON_ONE_LINE){
				 WriteChildrenOneLine(indent, output);
			 } else {
		  #endif
				 WriteChildren(indent, output);
		  #ifdef JSON_ARRAY_SIZE_ON_ONE_LINE
			 }
		  #endif
            output += JSON_TEXT("]");
			return;
	   case JSON_NUMBER:   //write out a literal, without quotes
	   case JSON_NULL:
	   case JSON_BOOL:
            output.append(_string.begin(), _string.end());
			return;
    }

	JSON_ASSERT(_type == JSON_STRING, JSON_TEXT("Unknown json node type"));
    //If it go here, then it's a json_string
    #if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
		if (json_likely(fetched)){
	#endif
			output += JSON_TEXT("\"");
			JSONWorker::UnfixString(_string, _string_encoded, output);  //It's already been fetched, meaning that it's unescaped
			output += JSON_TEXT("\"");  
	#if !defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY)
		} else {
			DumpRawString(output);  //it hasn't yet been fetched, so it's already unescaped, just do a dump
		}
    #endif
}
#endif
