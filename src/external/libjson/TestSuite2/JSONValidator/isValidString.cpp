#include "isValidString.h"
#include "Resources/validyMacros.h"
#include "../../Source/JSONValidator.h"

void testJSONValidator__isValidString::testNormal(void){
	assertValid("hello\":123", isValidString, ':');
    assertValid("he\\\"ll\\\"o\":123", isValidString, ':');
}

void testJSONValidator__isValidString::testUnicode(void){
	assertValid("he\\u1234llo\":123", isValidString, ':');
    assertValid("he\\u0FFFllo\":123", isValidString, ':');
    assertNotValid("he\\uFFFGllo\":123", isValidString, ':');
}

void testJSONValidator__isValidString::testStrict(void){
	#ifdef JSON_STRICT
		assertNotValid("he\\xFFllo\":123", isValidString, ':');
		assertNotValid("he\\0123llo\":123", isValidString, ':');
	#endif
}

void testJSONValidator__isValidString::testNotStrict(void){
	#ifndef JSON_STRICT
		assertValid("he\\xFFllo\":123", isValidString, ':');
		#ifdef JSON_OCTAL
			assertValid("he\\0123llo\":123", isValidString, ':');
		#else
			assertNotValid("he\\0123llo\":123", isValidString, ':');
		#endif
	#endif
}

void testJSONValidator__isValidString::testNotString(void){
	assertNotValid("he\\128llo\":123", isValidString, ':');  //not valid even when not strict because of the 8
}

void testJSONValidator__isValidString::testSuddenEnd(void){
    assertNotValid("he\\", isValidString, ':');
    assertNotValid("he\\\"", isValidString, ':');  //escaped quote
    assertNotValid("he\\\"llo\\\"", isValidString, ':');  //two esacaped quotes
	
	//--- void testJSONValidator__isValidString::testNormal(void){
	assertNotValid("hello", isValidString, ':');
    assertNotValid("he\\\"ll\\\"o", isValidString, ':');
	
	//--- void testJSONValidator__isValidString::testUnicode(void){
	assertNotValid("he\\u1234llo", isValidString, ':');
    assertNotValid("he\\u0FF", isValidString, ':');
	assertNotValid("he\\u0F", isValidString, ':');
	assertNotValid("he\\u0", isValidString, ':');
	assertNotValid("he\\u", isValidString, ':');
	assertNotValid("he\\", isValidString, ':');
	
	//strict stuff
	assertNotValid("he\\xFF", isValidString, ':');
	assertNotValid("he\\xF", isValidString, ':');
	assertNotValid("he\\x", isValidString, ':');
	assertNotValid("he\\0123", isValidString, ':');
	assertNotValid("he\\012", isValidString, ':');
	assertNotValid("he\\01", isValidString, ':');
	assertNotValid("he\\0", isValidString, ':');
}
