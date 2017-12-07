#include "JSONValidator.h"

#ifdef JSON_VALIDATE

inline bool isHex(json_char c) json_pure;
inline bool isHex(json_char c) json_nothrow {
    return (((c >= JSON_TEXT('0')) && (c <= JSON_TEXT('9'))) ||
		  ((c >= JSON_TEXT('A')) && (c <= JSON_TEXT('F'))) ||
		  ((c >= JSON_TEXT('a')) && (c <= JSON_TEXT('f'))));
}

bool JSONValidator::isValidNumber(const json_char * & ptr) json_nothrow {
    //ptr points at the first character in the number
    //ptr will end up past the last character
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
		#ifdef JSON_STRICT
			switch(*(ptr + 1)){
				case '.':
				case 'e':
				case 'E':
				case '\0':
					return false;
			}
			break;
		#endif
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
				switch(*ptr){
				    case JSON_TEXT('\0'):
					   return false;
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
					   return false;
				}
				break;
			 #ifndef JSON_STRICT
				case JSON_TEXT('x'):
				    while(isHex(*++ptr)){};
				    return true;
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
					   return ((*ptr != JSON_TEXT('8')) && (*ptr != JSON_TEXT('9')));
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
				return true;
		  }
		  break;
	   default:
		  return false;
    }
    ++ptr;

    //next digits
    while (true){
	   switch(*ptr){
		  case JSON_TEXT('.'):
			 if (json_unlikely(decimal)) return false; //multiple decimals
			 if (json_unlikely(scientific)) return false;
			 decimal = true;
			 break;
		  case JSON_TEXT('e'):
		  case JSON_TEXT('E'):
			 if (json_likely(scientific)) return false;
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
				    return false;
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
			 return true;
	   }
	   ++ptr;
    }
    return false;
}

#ifndef JSON_STRICT
    #define LETTERCASE(x, y)\
	   case JSON_TEXT(x):\
	   case JSON_TEXT(y)
    #define LETTERCHECK(x, y)\
	   if (json_unlikely((*++ptr != JSON_TEXT(x)) && (*ptr != JSON_TEXT(y)))) return false
#else
    #define LETTERCASE(x, y)\
	   case JSON_TEXT(x)
    #define LETTERCHECK(x, y)\
	   if (json_unlikely(*++ptr != JSON_TEXT(x))) return false
#endif
bool JSONValidator::isValidMember(const json_char * & ptr  DEPTH_PARAM) json_nothrow {
    //ptr is on the first character of the member
    //ptr will end up immediately after the last character in the member
    switch(*ptr){
	   case JSON_TEXT('\"'):
		  return isValidString(++ptr);
	   case JSON_TEXT('{'):
		  INC_DEPTH();
		  return isValidObject(++ptr  DEPTH_ARG(depth_param));
	   case JSON_TEXT('['):
		  INC_DEPTH();
		  return isValidArray(++ptr  DEPTH_ARG(depth_param));
	   LETTERCASE('t', 'T'):
		  LETTERCHECK('r', 'R');
		  LETTERCHECK('u', 'U');
		  LETTERCHECK('e', 'E');
		  ++ptr;
		  return true;
	   LETTERCASE('f', 'F'):
		  LETTERCHECK('a', 'A');
		  LETTERCHECK('l', 'L');
		  LETTERCHECK('s', 'S');
		  LETTERCHECK('e', 'E');
		  ++ptr;
		  return true;
	   LETTERCASE('n', 'N'):
		  LETTERCHECK('u', 'U');
		  LETTERCHECK('l', 'L');
		  LETTERCHECK('l', 'L');
		  ++ptr;
		  return true;
	   #ifndef JSON_STRICT
		  case JSON_TEXT('}'):  //null in libjson
		  case JSON_TEXT(']'):  //null in libjson
		  case JSON_TEXT(','):  //null in libjson
			 return true;
	   #endif
	   case JSON_TEXT('\0'):
		  return false;
    }
    //a number
    return isValidNumber(ptr);
}

bool JSONValidator::isValidString(const json_char * & ptr) json_nothrow {
    //ptr is pointing to the first character after the quote
    //ptr will end up behind the closing "
    while(true){
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
				    if (json_unlikely(!isHex(*++ptr))) return false;
				    if (json_unlikely(!isHex(*++ptr))) return false;
				    //fallthrough to \x
				#ifndef JSON_STRICT
				case JSON_TEXT('x'):  //hex
				#endif
				    if (json_unlikely(!isHex(*++ptr))) return false;
				    if (json_unlikely(!isHex(*++ptr))) return false;
				    break;
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
					   if (json_unlikely((*++ptr < JSON_TEXT('0')) || (*ptr > JSON_TEXT('7')))) return false;
					   if (json_unlikely((*++ptr < JSON_TEXT('0')) || (*ptr > JSON_TEXT('7')))) return false;
					   break;
				#endif
				default:
				    return false;
			 }
			 break;
		  case JSON_TEXT('\"'):
			 ++ptr;
			 return true;
		  case JSON_TEXT('\0'):
			 return false;
	   }
	   ++ptr;
    }
    return false;
}

bool JSONValidator::isValidNamedObject(const json_char * &ptr  DEPTH_PARAM) json_nothrow {
    if (json_unlikely(!isValidString(++ptr))) return false;
    if (json_unlikely(*ptr++ != JSON_TEXT(':'))) return false;
    if (json_unlikely(!isValidMember(ptr  DEPTH_ARG(depth_param)))) return false;
    switch(*ptr){
	   case JSON_TEXT(','):
		  return isValidNamedObject(++ptr  DEPTH_ARG(depth_param));
	   case JSON_TEXT('}'):
		  ++ptr;
		  return true;
	   default:
		  return false;
    }
}

bool JSONValidator::isValidObject(const json_char * & ptr  DEPTH_PARAM) json_nothrow {
    //ptr should currently be pointing past the {, so this must be the start of a name, or the closing }
    //ptr will end up past the last }
    do{
	   switch(*ptr){
		  case JSON_TEXT('\"'):
			 return isValidNamedObject(ptr  DEPTH_ARG(depth_param));
		  case JSON_TEXT('}'):
			 ++ptr;
			 return true;
		  default:
			 return false;
	   }
    } while (*++ptr);
    return false;
}

bool JSONValidator::isValidArray(const json_char * & ptr  DEPTH_PARAM) json_nothrow {
    //ptr should currently be pointing past the [, so this must be the start of a member, or the closing ]
    //ptr will end up past the last ]
    do{
	   switch(*ptr){
		  case JSON_TEXT(']'):
			 ++ptr;
			 return true;
		  default:
			 if (json_unlikely(!isValidMember(ptr  DEPTH_ARG(depth_param)))) return false;
			 switch(*ptr){
				case JSON_TEXT(','):
				    break;
				case JSON_TEXT(']'):
				    ++ptr;
				    return true;
				default:
				    return false;
			 }
			 break;
	   }
    } while (*++ptr);
    return false;
}

bool JSONValidator::isValidRoot(const json_char * json) json_nothrow {
    const json_char * ptr = json;
    switch(*ptr){
	   case JSON_TEXT('{'):
		  if (json_likely(isValidObject(++ptr  DEPTH_ARG(1)))){
			 return *ptr == JSON_TEXT('\0');
		  }
		  return false;
	   case JSON_TEXT('['):
		  if (json_likely(isValidArray(++ptr  DEPTH_ARG(1)))){
			 return *ptr == JSON_TEXT('\0');
		  }
		  return false;
    }
    return false;
}

#ifdef JSON_STREAM
//It has already been checked for a complete structure, so we know it's not complete
bool JSONValidator::isValidPartialRoot(const json_char * json) json_nothrow {
	const json_char * ptr = json;
    switch(*ptr){
		case JSON_TEXT('{'):
			JSON_ASSERT_SAFE(!isValidObject(++ptr  DEPTH_ARG(1)), JSON_TEXT("Partial Object seems to be valid"), );
			return *ptr == JSON_TEXT('\0');
		case JSON_TEXT('['):
			JSON_ASSERT_SAFE(!isValidArray(++ptr  DEPTH_ARG(1)), JSON_TEXT("Partial Object seems to be valid"), );
			return *ptr == JSON_TEXT('\0');
    }
    return false;
}
#endif

#endif
