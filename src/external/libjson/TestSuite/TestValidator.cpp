#include "TestSuite.h"
#include "../Source/JSONValidator.h"


#define assertValid(x, method, nextchar)\
    {\
	   json_string temp(JSON_TEXT(x));\
	   const json_char * ptr = temp.c_str();\
	   assertTrue(JSONValidator::method(ptr) && (*ptr == JSON_TEXT(nextchar)));\
    }

#define assertNotValid(x, method, nextchar)\
    {\
	   json_string temp(JSON_TEXT(x));\
	   const json_char * ptr = temp.c_str();\
	   assertFalse(JSONValidator::method(ptr) && (*ptr == JSON_TEXT(nextchar)));\
    }

#ifdef JSON_SECURITY_MAX_NEST_LEVEL
    #define assertValid_Depth(x, method, nextchar)\
	   {\
		  json_string temp(JSON_TEXT(x));\
		  const json_char * ptr = temp.c_str();\
		  assertTrue(JSONValidator::method(ptr, 1));\
		  assertEquals(*ptr, JSON_TEXT(nextchar));\
	   }

    #define assertNotValid_Depth(x, method, nextchar)\
	   {\
		  json_string temp(JSON_TEXT(x));\
		  const json_char * ptr = temp.c_str();\
		  assertFalse(JSONValidator::method(ptr, 1));\
	   }
#else
    #define assertValid_Depth(x, method, nextchar) assertValid(x, method, nextchar)
    #define assertNotValid_Depth(x, method, nextchar) assertNotValid(x, method, nextchar)
#endif



void TestSuite::TestValidator(void){
#ifdef JSON_VALIDATE

    UnitTest::SetPrefix("TestValidator.cpp - Validator Root");
    assertTrue(JSONValidator::isValidRoot(JSON_TEXT("{}")));
    assertTrue(JSONValidator::isValidRoot(JSON_TEXT("[]")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{]")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[}")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{}aoe")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[]aoe")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("aoe")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("")));
    assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[\"stuff\":{},]")));

    UnitTest::SetPrefix("TestValidator.cpp - Validator Number");
    assertValid("123,\"next\"", isValidNumber, ',');
    assertValid("12.3,\"next\"", isValidNumber, ',');
    assertValid("0.123,\"next\"", isValidNumber, ',');
    assertValid("0,\"next\"", isValidNumber, ',');
    assertValid("0.,\"next\"", isValidNumber, ',');
    assertValid("0e123,\"next\"", isValidNumber, ',');
    assertValid("0e-123,\"next\"", isValidNumber, ',');
    assertValid("0e+123,\"next\"", isValidNumber, ',');
    assertNotValid("0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("0e+12.3,\"next\"", isValidNumber, ',');
    assertValid("1.e123,\"next\"", isValidNumber, ',');
    assertValid("1.e-123,\"next\"", isValidNumber, ',');
    assertValid("1.e+123,\"next\"", isValidNumber, ',');
    assertNotValid("1.e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("1.e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("1.e+12.3,\"next\"", isValidNumber, ',');
    assertValid("1.0e123,\"next\"", isValidNumber, ',');
    assertValid("1.0e-123,\"next\"", isValidNumber, ',');
    assertValid("1.0e+123,\"next\"", isValidNumber, ',');
    assertNotValid("1.0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("1.0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("1.0e+12.3,\"next\"", isValidNumber, ',');

    assertValid("-123,\"next\"", isValidNumber, ',');
    assertValid("-12.3,\"next\"", isValidNumber, ',');
    assertValid("-0.123,\"next\"", isValidNumber, ',');
    assertValid("-0,\"next\"", isValidNumber, ',');
    assertValid("-0.,\"next\"", isValidNumber, ',');
    assertValid("-0e123,\"next\"", isValidNumber, ',');
    assertValid("-0e-123,\"next\"", isValidNumber, ',');
    assertValid("-0e+123,\"next\"", isValidNumber, ',');
    assertNotValid("-0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-0e+12.3,\"next\"", isValidNumber, ',');
    assertValid("-1.e123,\"next\"", isValidNumber, ',');
    assertValid("-1.e-123,\"next\"", isValidNumber, ',');
    assertValid("-1.e+123,\"next\"", isValidNumber, ',');
    assertNotValid("-1.e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-1.e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-1.e+12.3,\"next\"", isValidNumber, ',');
    assertValid("-1.0e123,\"next\"", isValidNumber, ',');
    assertValid("-1.0e-123,\"next\"", isValidNumber, ',');
    assertValid("-1.0e+123,\"next\"", isValidNumber, ',');
    assertNotValid("-1.0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-1.0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("-1.0e+12.3,\"next\"", isValidNumber, ',');
    assertValid("0123,\"next\"", isValidNumber, ',');  //legal when not strict because leading zeros are ignored
    #ifndef JSON_STRICT
	   assertValid(".01,\"next\"", isValidNumber, ',');
	   assertValid("-.01,\"next\"", isValidNumber, ',');
	   assertValid("+123,\"next\"", isValidNumber, ',');
	   assertValid("+12.3,\"next\"", isValidNumber, ',');
	   assertValid("+0.123,\"next\"", isValidNumber, ',');
	   assertValid("+0,\"next\"", isValidNumber, ',');
	   assertValid("+0.,\"next\"", isValidNumber, ',');
	   assertValid("+0e123,\"next\"", isValidNumber, ',');
	   assertValid("+0e-123,\"next\"", isValidNumber, ',');
	   assertValid("+0e+123,\"next\"", isValidNumber, ',');
	   assertValid("+1.e123,\"next\"", isValidNumber, ',');
	   assertValid("+1.e-123,\"next\"", isValidNumber, ',');
	   assertValid("+1.e+123,\"next\"", isValidNumber, ',');
	   assertValid("+1.0e123,\"next\"", isValidNumber, ',');
	   assertValid("+1.0e-123,\"next\"", isValidNumber, ',');
	   assertValid("+1.0e+123,\"next\"", isValidNumber, ',');
	   assertValid("0x12FF,\"next\"", isValidNumber, ',');
	   #ifdef JSON_OCTAL
			 assertNotValid("0128,\"next\"", isValidNumber, ',');  //because of the 8
	   #else
		  assertValid("0128,\"next\"", isValidNumber, ',');  //because the leading 0 is ignored
	   #endif
    #else
	   assertNotValid(".01,\"next\"", isValidNumber, ',');  //no leading 0 as required by the standard
	   assertNotValid("-.01,\"next\"", isValidNumber, ',');  //no leading 0 as required by the standard
	   assertNotValid("+123,\"next\"", isValidNumber, ',');  //no leading +
	   assertNotValid("+12.3,\"next\"", isValidNumber, ',');
	   assertNotValid("+0.123,\"next\"", isValidNumber, ',');
	   assertNotValid("+0,\"next\"", isValidNumber, ',');
	   assertNotValid("+0.,\"next\"", isValidNumber, ',');
	   assertNotValid("+0e123,\"next\"", isValidNumber, ',');
	   assertNotValid("+0e-123,\"next\"", isValidNumber, ',');
	   assertNotValid("+0e+123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.e123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.e-123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.e+123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.0e123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.0e-123,\"next\"", isValidNumber, ',');
	   assertNotValid("+1.0e+123,\"next\"", isValidNumber, ',');
	   assertNotValid("0x12FF,\"next\"", isValidNumber, ',');
	   assertValid("0128,\"next\"", isValidNumber, ',');  //legal because in STRICT mode, this is not octal
    #endif
    assertNotValid("+1.0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+1.0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+1.0e+12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+1.e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+1.e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+1.e+12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+0e12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+0e-12.3,\"next\"", isValidNumber, ',');
    assertNotValid("+0e+12.3,\"next\"", isValidNumber, ',');

    UnitTest::SetPrefix("TestValidator.cpp - Validator String");
    assertValid("hello\":123", isValidString, ':');
    assertValid("he\\\"ll\\\"o\":123", isValidString, ':');
    assertValid("he\\u1234llo\":123", isValidString, ':');
    assertValid("he\\u0FFFllo\":123", isValidString, ':');
    assertNotValid("he\\uFFFGllo\":123", isValidString, ':');
    #ifndef JSON_STRICT
	   assertValid("he\\xFFllo\":123", isValidString, ':');
		#ifdef JSON_OCTAL
			assertValid("he\\0123llo\":123", isValidString, ':');
		#else
			assertNotValid("he\\0123llo\":123", isValidString, ':');
		#endif
    #else
	   assertNotValid("he\\xFFllo\":123", isValidString, ':');
	   assertNotValid("he\\0123llo\":123", isValidString, ':');
    #endif
    assertNotValid("he\\128llo\":123", isValidString, ':');  //not valid even when not strict because of the 8
    assertNotValid("he\\", isValidString, ':');
    assertNotValid("he\\\"", isValidString, ':');
    assertNotValid("he\\\"llo\\\"", isValidString, ':');
    assertNotValid("hello", isValidString, ':');


    UnitTest::SetPrefix("TestValidator.cpp - Validator Member");
    assertValid_Depth("true,", isValidMember, ',');
    assertNotValid_Depth("tru,", isValidMember, ',');
    assertValid_Depth("false,", isValidMember, ',');
    assertNotValid_Depth("fals,", isValidMember, ',');
    assertValid_Depth("null,", isValidMember, ',');
    assertNotValid_Depth("nul,", isValidMember, ',');
    assertNotValid_Depth("", isValidMember, ',');
    #ifndef JSON_STRICT
	   assertValid_Depth("TRUE,", isValidMember, ',');
	   assertValid_Depth("FALSE,", isValidMember, ',');
	   assertValid_Depth("NULL,", isValidMember, ',');
	   assertValid_Depth(",", isValidMember, ',');
    #else
	   assertNotValid_Depth("TRUE,", isValidMember, ',');
	   assertNotValid_Depth("FALSE,", isValidMember, ',');
	   assertNotValid_Depth("NULL,", isValidMember, ',');
	   assertNotValid_Depth(",", isValidMember, ',');
    #endif
    
    UnitTest::SetPrefix("TestValidator.cpp - Validator Security");
    #ifdef JSON_SECURITY_MAX_NEST_LEVEL
	   #if (JSON_SECURITY_MAX_NEST_LEVEL != 128)
		  #error, test suite only wants a nest security level of 100
	   #endif
	   {
		  json_string json(JSON_TEXT("{"));
		  for(unsigned int i = 0; i < 127; ++i){
			 json += JSON_TEXT("\"n\":{");
		  }
		  json += json_string(128, '}');
		  assertTrue(JSONValidator::isValidRoot(json.c_str()));
	   }
	   {
		  json_string json(JSON_TEXT("{"));
		  for(unsigned int i = 0; i < 128; ++i){
			 json += JSON_TEXT("\"n\":{");
		  }
		  json += json_string(129, '}');
		  assertFalse(JSONValidator::isValidRoot(json.c_str()));
	   }
    #endif
    
#endif
}
