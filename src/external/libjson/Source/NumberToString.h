#ifndef NUMBERTOSTRING_H
#define NUMBERTOSTRING_H

#include <limits>
#include "JSONDebug.h"
#ifdef JSON_LESS_MEMORY
    #include "JSONMemory.h"
#endif
#include "JSONSharedString.h"
#include <cstdio>
#ifdef JSON_STRICT
    #include <cmath>
#endif
template <unsigned int GETLENSIZE>
struct getLenSize{
    char tmp[GETLENSIZE == 16];  // compile time assertion
    enum {GETLEN = 41};
};

template<>
struct getLenSize<1>{
    enum {GETLEN = 5};
};

template <>
struct getLenSize<2>{
    enum {GETLEN = 7};
};

template <>
struct getLenSize<4>{
    enum {GETLEN = 12};
};

template <>
struct getLenSize<8>{
    enum {GETLEN = 22};
};

static inline bool _floatsAreEqual(const json_number & one, const json_number & two) json_pure;
static inline bool _floatsAreEqual(const json_number & one, const json_number & two) json_nothrow {
    return (one > two) ? (one - two) < JSON_FLOAT_THRESHHOLD : (one - two) > -JSON_FLOAT_THRESHHOLD;
}

#ifdef JSON_LESS_MEMORY
    #define num_str_result s.ptr
#endif

class NumberToString {
public:
    template<typename T>
    static json_string _itoa(T val) json_nothrow {		
	   #ifdef JSON_LESS_MEMORY
		  json_auto<json_char> s(getLenSize<sizeof(T)>::GETLEN);
	   #else
		  json_char num_str_result[getLenSize<sizeof(T)>::GETLEN];
	   #endif
	   num_str_result[getLenSize<sizeof(T)>::GETLEN - 1] = JSON_TEXT('\0'); //null terminator
	   json_char * runner = &num_str_result[getLenSize<sizeof(T)>::GETLEN - 2];
	   bool negative;

	   START_MEM_SCOPE
	   long value = (long)val;
	   //first thing, check if it's negative, if so, make it positive
	   if (value < 0){
		  value = -value;
		  negative = true;
	   } else {
		  negative = false;
	   }

	   //create the string
	   do {
		  *runner-- = (json_char)(value % 10) + JSON_TEXT('0');
	   } while(value /= 10);
	   END_MEM_SCOPE

	   //if it's negative, add the negation
	   if (negative){
		  *runner = JSON_TEXT('-');
		  return json_string(runner);
	   }
	   return json_string(runner + 1);
    }

    #ifndef JSON_LIBRARY
	   template<typename T>
	   static json_string _uitoa(T val) json_nothrow {
		  #ifdef JSON_LESS_MEMORY
			 json_auto<json_char> s(getLenSize<sizeof(T)>::GETLEN);
		  #else
			 json_char num_str_result[getLenSize<sizeof(T)>::GETLEN];
		  #endif
		  num_str_result[getLenSize<sizeof(T)>::GETLEN - 1] = JSON_TEXT('\0'); //null terminator
		  json_char * runner = &num_str_result[getLenSize<sizeof(T)>::GETLEN - 2];

		  //create the string
		  START_MEM_SCOPE
			 unsigned long value = (unsigned long)val;
			 do {
				*runner-- = (json_char)(value % 10) + JSON_TEXT('0');
			 } while(value /= 10);
		  END_MEM_SCOPE

		  return json_string(runner + 1);
	   }
    #endif

    #ifdef JSON_ISO_STRICT
	   #define EXTRA_LONG
	   #define FLOAT_STRING "%f"
	   #define LFLOAT_STRING L"%f"
    #else
	   #define EXTRA_LONG long
	   #define FLOAT_STRING "%Lf"
	   #define LFLOAT_STRING L"%Lf"
    #endif

    static json_string _ftoa(json_number value) json_nothrow {
	   #ifndef JSON_LIBRARY
			//ScopeCoverage(_ftoa_coverage, 6);
		  if (json_unlikely(value >= 0.0 && _floatsAreEqual(value, (json_number)((unsigned EXTRA_LONG long)value)))){
			 return _uitoa<unsigned EXTRA_LONG long>((unsigned EXTRA_LONG long)value);
		  } else
		#else
			  //ScopeCoverage(_ftoa_coverage, 5);
	   #endif
		  if (json_unlikely(_floatsAreEqual(value, (json_number)((long EXTRA_LONG)value)))){
			 return _itoa<long EXTRA_LONG>((long EXTRA_LONG)value);
		  }

	   #ifdef JSON_LESS_MEMORY
		  json_auto<json_char> s(64);
	   #else
		  json_char num_str_result[64];
	   #endif
	   #ifdef JSON_UNICODE
		  std::swprintf(num_str_result, 63, LFLOAT_STRING, (EXTRA_LONG double)value);
	   #else
		  //Thanks to Salvor Hardin for this Visual C++ fix
		  #ifdef _MSC_VER
			 _snprintf_s(num_str_result, 63, 63, FLOAT_STRING, (EXTRA_LONG double)value); //yes, 63 appears twice using _snprintf_s()
		  #else
			 std::snprintf(num_str_result, 63, FLOAT_STRING, (EXTRA_LONG double)value);
		  #endif
	   #endif
	   //strip the trailing zeros
	   for(json_char * pos = &num_str_result[0]; *pos; ++pos){
		  if (json_unlikely(*pos == '.')){  //only care about after the decimal
			 for(json_char * runner = pos + 1; *runner; ++runner){
				 if (json_likely(*runner != JSON_TEXT('0'))){
					 pos = runner + 1;  //have to go to the end 1.0001
				 }
			 }
			 *pos = JSON_TEXT('\0');
			 break;
		  }
	   }
	   return json_string(num_str_result);
    }

    #if defined(JSON_SAFE) || defined(JSON_DEBUG)
	   static bool isNumeric(const json_string & str) json_nothrow {
		  const json_char * p = str.c_str();
		  bool decimal = false;
		  bool scientific = false;
		   
			#ifdef JSON_STRICT
		   bool leadingzero = false;	   
			#endif

		  //first letter is weird
		  switch(*p){
			 case JSON_TEXT('\0'):
				return false;
		  #ifndef JSON_STRICT
			 case JSON_TEXT('.'):
				decimal = true;
				break;
			 case JSON_TEXT('+'):
		  #endif
			 case JSON_TEXT('-'):
				  switch (*(p + 1)){
					  case JSON_TEXT('.'):
					  case JSON_TEXT('e'):
					  case JSON_TEXT('E'):
					  case JSON_TEXT('\0'):
						  return false;
					  case JSON_TEXT('0'):
						  #ifdef JSON_STRICT
						  switch(*(p + 2)){
							  case JSON_TEXT('.'):
							  case JSON_TEXT('e'):
							  case JSON_TEXT('E'):
								  leadingzero = false;
								  break;
							  case JSON_TEXT('\0'): 
								  return true;
							  default:
								  leadingzero = true;
								  break;
						  }
						  #endif
						  ++p;
						  break;
					  default:
						  break;
				  }
				  break;
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
				++p;
				#ifdef JSON_STRICT
				leadingzero = true;	   
				#endif
				switch(*p){
				    case JSON_TEXT('.'):
					   decimal = true;
					   break;
				    case JSON_TEXT('e'):
				    case JSON_TEXT('E'):
						#ifdef JSON_STRICT
						leadingzero = false; //not leading, just a zero
						#endif
					   scientific = true;
					   ++p;
					   switch(*p){
						  case JSON_TEXT('\0'):
							 return false;
						  case JSON_TEXT('-'):
						  case JSON_TEXT('+'):
						  #ifndef JSON_STRICT
						  case JSON_TEXT('0'):  //cant have a leading zero in scrict
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
						  default:
							 return false;
					   }
					   break;
				    #ifndef JSON_STRICT
					   case JSON_TEXT('x'):
						  return (str.find_first_not_of(JSON_TEXT("0123456789ABCDEFabcdef"), 2) == json_string::npos);
					   case JSON_TEXT('1'):
					   case JSON_TEXT('2'):
					   case JSON_TEXT('3'):
					   case JSON_TEXT('4'):
					   case JSON_TEXT('5'):
					   case JSON_TEXT('6'):
					   case JSON_TEXT('7'):
						  return (str.find_first_not_of(JSON_TEXT("01234567"), 1) == json_string::npos);
				    #endif
				    case JSON_TEXT('\0'):  //just 0
					   return true;
				    default:
					   return false;
				}
				break;
			 default:
				return false;
		  }
		  ++p;

		  //next digits
		  while (*p){
			 switch(*p){
				case JSON_TEXT('.'):
					 if (json_unlikely(decimal)){
						 return false; //multiple decimals
					 }
					 
					 if (json_unlikely(scientific)){
						 return false;
					 }
				    decimal = true;
				    break;
				case JSON_TEXT('e'):
				case JSON_TEXT('E'):
					 if (json_unlikely(scientific)){
						 return false;
					 }
				    scientific = true;
				    ++p;
				    switch(*p){
					   case JSON_TEXT('\0'):
						  return false;
					   case JSON_TEXT('-'):
					   case JSON_TEXT('+'):
							if (!isdigit(*(p + 1))){
								return false;
							}
							
							#ifdef JSON_STRICT
								if (*(p + 1) == JSON_TEXT('0')){  //no leading zeros on scientific notations
									return false;
								}
							#endif
							break;
						#ifndef JSON_STRICT
					   case JSON_TEXT('0'):  //cant have a leading zero in scrict
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
					   default:
						  return false;
				    }
				    break;
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
			 ++p;
		  }
		#ifdef JSON_STRICT
		   if (leadingzero && !decimal){
			   return false;	  
		   }
		#endif
		  return true;
	   }
    #endif

    #ifdef JSON_STRICT
	   //much faster because no octal or hex support
	   static json_number _atof (const json_char * num){
		  json_number sign = (json_number)1.0;

		  //sign
		  if (*num==JSON_TEXT('-')){
			 sign = -1.0;
			 ++num;
		  } else {
		  }

		  //skip leading zero if one
		  #if defined(JSON_SAFE) || defined(JSON_DEBUG)
			 bool _leadingzeros = *num == JSON_TEXT('0');
		     bool _leadingdigits = false;
		  #endif
		  if (*num == JSON_TEXT('0')){
			 ++num;
		  }
		  #ifdef JSON_STRICT
			else if (json_likely(*num < JSON_TEXT('1') || *num > JSON_TEXT('9'))){
				return std::numeric_limits<json_number>::signaling_NaN();
			}
		  #endif
		   
		   JSON_ASSERT_SAFE(*num != JSON_TEXT('0'), JSON_TEXT("multiple leading zeros"), return std::numeric_limits<json_number>::signaling_NaN(); );

		  // Number
		   json_number n = (json_number)0.0;
		  if (json_likely(*num >= JSON_TEXT('1') && *num <= JSON_TEXT('9'))){
			  #if defined(JSON_SAFE) || defined(JSON_DEBUG)
				  _leadingdigits = true;
			  #endif
			 do {
				n = (n * 10.0) + (*num++ - JSON_TEXT('0'));
			 } while (*num >= JSON_TEXT('0') && *num <= JSON_TEXT('9'));
		  } else {
			  JSON_ASSERT_SAFE(
							   (*num) == JSON_TEXT('.') ||	 //.xxx
							   (*num) == JSON_TEXT('e') ||	 //0Exxx
							   (*num) == JSON_TEXT('E') ||	 //0exxx
							   (*num) == JSON_TEXT('\0')     //end of the number, just zero
							   , JSON_TEXT("first digit not a number, e, period, or terminator"), return std::numeric_limits<json_number>::signaling_NaN(); );
		  }

		  // Fractional part
		  json_number scale = (json_number)0.0;
		  if (*num == JSON_TEXT('.')) {
			  JSON_ASSERT_SAFE(_leadingzeros || _leadingdigits, JSON_TEXT("period without leading anything"), return std::numeric_limits<json_number>::signaling_NaN(); );
			 ++num;
			  for(; *num >= JSON_TEXT('0') && *num <= JSON_TEXT('9');){
				n = (n * 10.0) + (*num++ - JSON_TEXT('0'));
				--scale;
			  };
		  } else {  
			  JSON_ASSERT_SAFE(!_leadingzeros || n == 0, JSON_TEXT("leading zero on an int"), return std::numeric_limits<json_number>::signaling_NaN(); );
			  JSON_ASSERT_SAFE(
							   (*num) == JSON_TEXT('e') ||	 //0Exxx
							   (*num) == JSON_TEXT('E') ||	 //0exxx
							   (*num) == JSON_TEXT('\0')     //end of the number, just zero
							   , JSON_TEXT("next char not an e or terminator"), return std::numeric_limits<json_number>::signaling_NaN(); );
		  }

		  // Exponent
		  int subscale = 0, signsubscale = 1;
		  if (json_unlikely(*num == JSON_TEXT('e') || *num == JSON_TEXT('E'))){
			 ++num;
			 switch(*num){
				case JSON_TEXT('+'):
				    ++num;
				    break;
				case JSON_TEXT('-'):
				    signsubscale = -1;
				    ++num;
					 JSON_ASSERT_SAFE(*num != JSON_TEXT('0'), JSON_TEXT("negative cant be followed by leading zero even after E"), return std::numeric_limits<json_number>::signaling_NaN(); );
				    break;
				 default:
					 break;
			 }
			 JSON_ASSERT_SAFE(*num != JSON_TEXT('\0'), JSON_TEXT("no exponent for scientific notation"), return std::numeric_limits<json_number>::signaling_NaN(); );
			 while (*num >= JSON_TEXT('0') && *num <= JSON_TEXT('9')){
				subscale=(subscale * 10) + (*num++ - JSON_TEXT('0'));
			 }
		  }
		   
		  JSON_ASSERT_SAFE(*num == JSON_TEXT('\0'), JSON_TEXT("done with number, not at terminator"), return std::numeric_limits<json_number>::signaling_NaN(); );
		  return sign * n * pow((json_number)10.0, scale + subscale * signsubscale);	// number = +/- number.fraction * 10^+/- exponent
	   }
    #endif
};

#endif
