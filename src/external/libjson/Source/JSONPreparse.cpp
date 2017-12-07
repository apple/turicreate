/*
 *  JSONPreparse.cpp
 *  TestSuite
 *
 *  Created by Wallace on 4/13/11.
 *  Copyright 2011 Streamwide. All rights reserved.
 *
 */

#include "JSONPreparse.h"

#if (defined(JSON_PREPARSE) && defined(JSON_READ_PRIORITY))

#ifdef JSON_COMMENTS
	json_string extractComment(json_string::const_iterator & ptr, json_string::const_iterator & end);
	json_string extractComment(json_string::const_iterator & ptr, json_string::const_iterator & end){
		json_string::const_iterator start;
		json_string result;
looplabel:
		if (json_unlikely(((ptr != end) && (*ptr == JSON_TEMP_COMMENT_IDENTIFIER)))){
			start = ++ptr;
			for(; (ptr != end) && (*(ptr) != JSON_TEMP_COMMENT_IDENTIFIER); ++ptr){}
			result += json_string(start, ptr);
			if (json_unlikely(ptr == end)) return result;
			++ptr;
			if (json_unlikely(((ptr != end) && (*ptr == JSON_TEMP_COMMENT_IDENTIFIER)))){
				result += JSON_TEXT('\n');
				goto looplabel;
			}
		}
		return result;
	}
	#define GET_COMMENT(x, y, name) json_string name = extractComment(x, y)
	#define RETURN_NODE(node, name){\
		JSONNode res = node;\
		res.set_comment(name);\
		return res;\
	}
	#define RETURN_NODE_NOCOPY(node, name){\
		node.set_comment(name);\
		return node;\
	}
	#define SET_COMMENT(node, name) node.set_comment(name)
	#define COMMENT_ARG(name) ,name
#else
	#define GET_COMMENT(x, y, name) (void)0
	#define RETURN_NODE(node, name) return node
	#define RETURN_NODE_NOCOPY(node, name) return node
	#define SET_COMMENT(node, name) (void)0
	#define COMMENT_ARG(name)
#endif

inline bool isHex(json_char c) json_pure;
inline bool isHex(json_char c) json_nothrow {
    return (((c >= JSON_TEXT('0')) && (c <= JSON_TEXT('9'))) ||
		  ((c >= JSON_TEXT('A')) && (c <= JSON_TEXT('F'))) ||
		  ((c >= JSON_TEXT('a')) && (c <= JSON_TEXT('f'))));
}

#ifdef JSON_STRICT
	#include "NumberToString.h"
#endif

json_number FetchNumber(const json_string & _string) json_nothrow;
json_number FetchNumber(const json_string & _string) json_nothrow {
    #ifdef JSON_STRICT
	   return NumberToString::_atof(_string.c_str());
    #else
	   #ifdef JSON_UNICODE
		  const size_t len = _string.length();
		  #if defined(_MSC_VER) && defined(JSON_SAFE)
			 const size_t bytes = (len * (sizeof(json_char) / sizeof(char))) + 1;
			 json_auto<char> temp(bytes);
			 size_t res;
			 errno_t err = std::wcstombs_s(&res, temp.ptr, bytes, _string.c_str(), len);
			 if (err != 0){
				return (json_number)0.0;
			 }
		  #elif defined(JSON_SAFE)
			 const size_t bytes = (len * (sizeof(json_char) / sizeof(char))) + 1;
			 json_auto<char> temp(bytes);
			 size_t res = std::wcstombs(temp.ptr, _string.c_str(), len);
			 if (res == (size_t)-1){  //-1 is error code for this function
				return (json_number)0.0;
			 }
		  #else
			 json_auto<char> temp(len + 1);
			 size_t res = std::wcstombs(temp.ptr, _string.c_str(), len);
		  #endif
		  temp.ptr[res] = JSON_TEXT('\0');
		  return (json_number)std::atof(temp.ptr);
	   #else
		  return (json_number)std::atof(_string.c_str());
	   #endif
    #endif
}

JSONNode JSONPreparse::isValidNumber(json_string::const_iterator & ptr, json_string::const_iterator & end){
    //ptr points at the first character in the number
    //ptr will end up past the last character
    json_string::const_iterator start = ptr;
    bool decimal = false;
    bool scientific = false;
    
    //first letter is weird
    switch(*ptr){
	   #ifndef JSON_STRICT
	   case JSON_TEXT('.'):
		  decimal = true;
		  break;
	   case JSON_TEXT('+'):
	   #endif
		  
	   case JSON_TEXT('-'):
	   case JSON_TEXT('1'):
	   case JSON_TEXT('2'):
	   case JSON_TEXT('3'):
	   case JSON_TEXT('4'):
	   case JSON_TEXT('5'):
	   case JSON_TEXT('6'):
	   case JSON_TEXT('7'):
	   case JSON_TEXT('8'):
	   case JSON_TEXT('9'):
		  break;
	   case JSON_TEXT('0'):
		  ++ptr;
		  switch(*ptr){
			 case JSON_TEXT('.'):
				decimal = true;
				break;
			 case JSON_TEXT('e'):
			 case JSON_TEXT('E'):
				scientific = true;
				++ptr;
				if (ptr == end) throw false;
				switch(*ptr){
				    case JSON_TEXT('-'):
				    case JSON_TEXT('+'):
				    case JSON_TEXT('0'):
				    case JSON_TEXT('1'):
				    case JSON_TEXT('2'):
				    case JSON_TEXT('3'):
				    case JSON_TEXT('4'):
				    case JSON_TEXT('5'):
				    case JSON_TEXT('6'):
				    case JSON_TEXT('7'):
				    case JSON_TEXT('8'):
				    case JSON_TEXT('9'):
					   break;
				    default:
					   throw false;
				}
				break;
				
			 #ifndef JSON_STRICT
			 case JSON_TEXT('x'):
				while(isHex(*++ptr)){};
				return JSONNode(json_global(EMPTY_JSON_STRING), FetchNumber(json_string(start, end - 1)));
			 #ifdef JSON_OCTAL
			 #ifdef __GNUC__
				case JSON_TEXT('0') ... JSON_TEXT('7'):  //octal
			 #else
				case JSON_TEXT('0'):
				case JSON_TEXT('1'):
				case JSON_TEXT('2'):
				case JSON_TEXT('3'):
				case JSON_TEXT('4'):
				case JSON_TEXT('5'):
				case JSON_TEXT('6'):
				case JSON_TEXT('7'):
			 #endif
				while((*++ptr >= JSON_TEXT('0')) && (*ptr <= JSON_TEXT('7'))){};
				if ((*ptr != JSON_TEXT('8')) && (*ptr != JSON_TEXT('9'))){
				    return JSONNode(json_global(EMPTY_JSON_STRING), FetchNumber(json_string(start, ptr - 1)));
				} 
				throw false;
			 case JSON_TEXT('8'):
			 case JSON_TEXT('9'):
				break;
			 #else
			 #ifdef __GNUC__
				case JSON_TEXT('0') ... JSON_TEXT('9'):
			 #else
				case JSON_TEXT('0'):
				case JSON_TEXT('1'):
				case JSON_TEXT('2'):
				case JSON_TEXT('3'):
				case JSON_TEXT('4'):
				case JSON_TEXT('5'):
				case JSON_TEXT('6'):
				case JSON_TEXT('7'):
				case JSON_TEXT('8'):
				case JSON_TEXT('9'):
			 #endif
				break;
			 #endif
			 #else
			 #ifdef __GNUC__
				case JSON_TEXT('0') ... JSON_TEXT('9'):
			 #else
				case JSON_TEXT('0'):
				case JSON_TEXT('1'):
				case JSON_TEXT('2'):
				case JSON_TEXT('3'):
				case JSON_TEXT('4'):
				case JSON_TEXT('5'):
				case JSON_TEXT('6'):
				case JSON_TEXT('7'):
				case JSON_TEXT('8'):
				case JSON_TEXT('9'):
			 #endif
				break;
			 #endif
			 default:  //just a 0
				return JSONNode(json_global(EMPTY_JSON_STRING), FetchNumber(json_string(start, ptr - 1)));;
		  }
		  break;
	   default:
		  throw false;
    }
    ++ptr;
    
    //next digits
    while (true){
	   switch(*ptr){
		  case JSON_TEXT('.'):
			 if (json_unlikely(decimal)) throw false; //multiple decimals
			 if (json_unlikely(scientific)) throw false;
			 decimal = true;
			 break;
		  case JSON_TEXT('e'):
		  case JSON_TEXT('E'):
			 if (json_likely(scientific)) throw false;
			 scientific = true;
			 ++ptr;
			 switch(*ptr){
				case JSON_TEXT('-'):
				case JSON_TEXT('+'):
				#ifdef __GNUC__
				case JSON_TEXT('0') ... JSON_TEXT('9'):
				#else
				case JSON_TEXT('0'):
				case JSON_TEXT('1'):
				case JSON_TEXT('2'):
				case JSON_TEXT('3'):
				case JSON_TEXT('4'):
				case JSON_TEXT('5'):
				case JSON_TEXT('6'):
				case JSON_TEXT('7'):
				case JSON_TEXT('8'):
				case JSON_TEXT('9'):
				#endif
				    break;
				default:
				    throw false;
			 }
			 break;
		  #ifdef __GNUC__
		  case JSON_TEXT('0') ... JSON_TEXT('9'):
		  #else
		  case JSON_TEXT('0'):
		  case JSON_TEXT('1'):
		  case JSON_TEXT('2'):
		  case JSON_TEXT('3'):
		  case JSON_TEXT('4'):
		  case JSON_TEXT('5'):
		  case JSON_TEXT('6'):
		  case JSON_TEXT('7'):
		  case JSON_TEXT('8'):
		  case JSON_TEXT('9'):
		  #endif
			 break;
		  default:
			 return JSONNode(json_global(EMPTY_JSON_STRING), FetchNumber(json_string(start, ptr)));;
	   }
	   ++ptr;
    }
    throw false;
}

#ifndef JSON_STRICT
    #define LETTERCASE(x, y)\
	   case JSON_TEXT(x):\
	   case JSON_TEXT(y)
    #define LETTERCHECK(x, y)\
	   if (json_unlikely((*++ptr != JSON_TEXT(x)) && (*ptr != JSON_TEXT(y)))) throw false
#else
    #define LETTERCASE(x, y)\
	   case JSON_TEXT(x)
    #define LETTERCHECK(x, y)\
	   if (json_unlikely(*++ptr != JSON_TEXT(x))) throw false
#endif
JSONNode JSONPreparse::isValidMember(json_string::const_iterator & ptr, json_string::const_iterator & end){
    //ptr is on the first character of the member
    //ptr will end up immediately after the last character in the member
    if (ptr == end) throw false;
    
    switch(*ptr){
	   case JSON_TEXT('\"'):{
           return JSONNode::stringType(isValidString(++ptr, end));
	   }
	   case JSON_TEXT('{'):
		  return isValidObject(++ptr, end);
	   case JSON_TEXT('['):
		  return isValidArray(++ptr, end);
	   LETTERCASE('t', 'T'):
	   LETTERCHECK('r', 'R');
	   LETTERCHECK('u', 'U');
	   LETTERCHECK('e', 'E');
		  ++ptr;
		  return JSONNode(json_global(EMPTY_JSON_STRING), true);
	   LETTERCASE('f', 'F'):
	   LETTERCHECK('a', 'A');
	   LETTERCHECK('l', 'L');
	   LETTERCHECK('s', 'S');
	   LETTERCHECK('e', 'E');
		  ++ptr;
		  return JSONNode(json_global(EMPTY_JSON_STRING), false);
	   LETTERCASE('n', 'N'):
	   LETTERCHECK('u', 'U');
	   LETTERCHECK('l', 'L');
	   LETTERCHECK('l', 'L');
		  ++ptr;
		  return JSONNode(JSON_NULL);
	   #ifndef JSON_STRICT
		  case JSON_TEXT('}'):  //null in libjson
		  case JSON_TEXT(']'):  //null in libjson
		  case JSON_TEXT(','):  //null in libjson
			 return JSONNode(JSON_NULL);
	   #endif
    }
    //a number
    return isValidNumber(ptr, end);
}

json_string JSONPreparse::isValidString(json_string::const_iterator & ptr, json_string::const_iterator & end){
    //ptr is pointing to the first character after the quote
    //ptr will end up behind the closing "
    json_string::const_iterator start = ptr;
    
    while(ptr != end){
	   switch(*ptr){
		  case JSON_TEXT('\\'):
			 switch(*(++ptr)){
				case JSON_TEXT('\"'):
				case JSON_TEXT('\\'):
				case JSON_TEXT('/'):
				case JSON_TEXT('b'):
				case JSON_TEXT('f'):
				case JSON_TEXT('n'):
				case JSON_TEXT('r'):
				case JSON_TEXT('t'):
				    break;
				case JSON_TEXT('u'):
				    if (json_unlikely(!isHex(*++ptr))) throw false;
				    if (json_unlikely(!isHex(*++ptr))) throw false;
				    //fallthrough to \x
				#ifndef JSON_STRICT
				    case JSON_TEXT('x'):  //hex
				#endif
				    if (json_unlikely(!isHex(*++ptr))) throw false;
				    if (json_unlikely(!isHex(*++ptr))) throw false;
				    break;
				#ifndef JSON_OCTAL
				    #ifdef __GNUC__
					   case JSON_TEXT('0') ... JSON_TEXT('7'):  //octal
				    #else
					   case JSON_TEXT('0'):
					   case JSON_TEXT('1'):
					   case JSON_TEXT('2'):
					   case JSON_TEXT('3'):
					   case JSON_TEXT('4'):
					   case JSON_TEXT('5'):
					   case JSON_TEXT('6'):
					   case JSON_TEXT('7'):
				    #endif
				    if (json_unlikely((*++ptr < JSON_TEXT('0')) || (*ptr > JSON_TEXT('7')))) throw false;
				    if (json_unlikely((*++ptr < JSON_TEXT('0')) || (*ptr > JSON_TEXT('7')))) throw false;
				    break;
				#endif
				default:
				    throw false;
			 }
			 break;
		  case JSON_TEXT('\"'):
			 return json_string(start, ptr++);
	   }
	   ++ptr;
    }
    throw false;
}

void JSONPreparse::isValidNamedObject(json_string::const_iterator & ptr, json_string::const_iterator & end, JSONNode & parent COMMENT_PARAM(comment)) {
	//ptr should be right before the string name
    {
	   json_string _name = isValidString(++ptr, end);
	   if (json_unlikely(*ptr++ != JSON_TEXT(':'))) throw false;
	   JSONNode res = isValidMember(ptr, end);
	   res.set_name_(_name);
		SET_COMMENT(res, comment);
		#ifdef JSON_LIBRARY
			parent.push_back(&res);
		#else
			parent.push_back(res);
		#endif
    }
    if (ptr == end) throw false;
    switch(*ptr){
	   case JSON_TEXT(','):
			++ptr;
			{
				GET_COMMENT(ptr, end, nextcomment);
				isValidNamedObject(ptr, end, parent COMMENT_ARG(nextcomment));  //will handle all of them
			}
			return;
	   case JSON_TEXT('}'):
		  ++ptr;
		  return;
	   default:
		  throw false;
    }
}

JSONNode JSONPreparse::isValidObject(json_string::const_iterator & ptr, json_string::const_iterator & end) {
    //ptr should currently be pointing past the {, so this must be the start of a name, or the closing }
    //ptr will end up past the last }
    JSONNode res(JSON_NODE);
	GET_COMMENT(ptr, end, comment);
	switch(*ptr){
	  case JSON_TEXT('\"'):
		 isValidNamedObject(ptr, end, res COMMENT_ARG(comment));
		 return res;
	  case JSON_TEXT('}'):
		 ++ptr;
		 return res;
	  default:
		 throw false;
	}
}

void pushArrayMember(JSONNode & res, json_string::const_iterator & ptr, json_string::const_iterator & end);
void pushArrayMember(JSONNode & res, json_string::const_iterator & ptr, json_string::const_iterator & end){
	GET_COMMENT(ptr, end, comment);
	JSONNode temp = JSONPreparse::isValidMember(ptr, end);
	SET_COMMENT(temp, comment);
	#ifdef JSON_LIBRARY
		res.push_back(&temp);
	#else
		res.push_back(temp);
	#endif
}

JSONNode JSONPreparse::isValidArray(json_string::const_iterator & ptr, json_string::const_iterator & end) {
    //ptr should currently be pointing past the [, so this must be the start of a member, or the closing ]
    //ptr will end up past the last ]
    JSONNode res(JSON_ARRAY);
    do{
	   switch(*ptr){
		  case JSON_TEXT(']'):
			 ++ptr;
			   return res;
		  default:
			 pushArrayMember(res, ptr, end);
			 switch(*ptr){
				case JSON_TEXT(','):
				    break;
				case JSON_TEXT(']'):
				    ++ptr;
					return res;
				default:
				    throw false;
			 }
			 break;
	   }
    } while (++ptr != end);
    throw false;
}

JSONNode JSONPreparse::isValidRoot(const json_string & json) json_throws(std::invalid_argument) {
    json_string::const_iterator it = json.begin();
    json_string::const_iterator end = json.end();
    try {
		GET_COMMENT(it, end, comment);
	   switch(*it){
		  case JSON_TEXT('{'):
			   RETURN_NODE(isValidObject(++it, end), comment);
		   case JSON_TEXT('['):
			   RETURN_NODE(isValidArray(++it, end), comment);
	   }
    } catch (...){}
    
    #ifndef JSON_NO_EXCEPTIONS
	   throw std::invalid_argument(json_global(EMPTY_STD_STRING));
    #else
	   return JSONNode(JSON_NULL);
    #endif
}

#endif
