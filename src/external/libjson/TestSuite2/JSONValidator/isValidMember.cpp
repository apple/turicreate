#include "isValidMember.h"
#include "Resources/validyMacros.h"
#include "../../Source/JSONValidator.h"

/*
 *
 *	!!! ATTENTION !!!
 *
 *	libjson currently has three number parsing methods, they are being merged 
 *	behind the scenes, but all three interfaces must be consistent, so every set
 *	of numbers need to be tested in all three spots
 *
 *	JSONValidator/isValidMember     *this file*
 *	* Soon to come actual parser *
 */


/**
 *	This tests the three valid members that is not string, number, or container
 */
void testJSONValidator__isValidMember::testMembers(void){
	#ifdef JSON_VALIDATE
		assertValid_Depth("true,", isValidMember, ',');
		assertValid_Depth("false,", isValidMember, ',');
		assertValid_Depth("null,", isValidMember, ',');
	#endif
}


/**
 *	This tests that json's case sensitive rules are to be obeyed
 */
void testJSONValidator__isValidMember::testStrict(void){
	#ifdef JSON_VALIDATE
		#ifdef JSON_STRICT
			assertNotValid_Depth("TRUE,", isValidMember, ',');
			assertNotValid_Depth("FALSE,", isValidMember, ',');
			assertNotValid_Depth("NULL,", isValidMember, ',');
			assertNotValid_Depth(",", isValidMember, ',');  //also accepted as null usually
		#endif
	#endif
}


/**
 *	This tests that json's case sensitive rules are not obeyed normally
 */
void testJSONValidator__isValidMember::testNotStrict(void){
	#ifdef JSON_VALIDATE
		#ifndef JSON_STRICT
			assertValid_Depth("TRUE,", isValidMember, ',');
			assertValid_Depth("FALSE,", isValidMember, ',');
			assertValid_Depth("NULL,", isValidMember, ',');
			assertValid_Depth(",", isValidMember, ',');  //also accepted as null usually
		#endif
	#endif
}


/**
 *	This tests that non member values are not allowed
 */
void testJSONValidator__isValidMember::testNotMembers(void){
	#ifdef JSON_VALIDATE
		assertNotValid_Depth("tru,", isValidMember, ',');
		assertNotValid_Depth("fals,", isValidMember, ',');
		assertNotValid_Depth("nul,", isValidMember, ',');
		assertNotValid_Depth("", isValidMember, ',');  //needs a comma after it because of how the pipeline works
		assertNotValid_Depth("xxx,", isValidMember, ',');
		assertNotValid_Depth("nonsense,", isValidMember, ',');
	#endif
}


/**
 *	This tests that for all cases, if the string suddely ends, it recovers
 */
void testJSONValidator__isValidMember::testSuddenEnd(void){
	#ifdef JSON_VALIDATE
		assertNotValid_Depth("", isValidMember, ',');
	
		//--- void testJSONValidator__isValidMember::testSuddenEnd(void){
		assertNotValid_Depth("true", isValidMember, ',');
		assertNotValid_Depth("false", isValidMember, ',');
		assertNotValid_Depth("null", isValidMember, ',');

		//strict stuff
		assertNotValid_Depth("TRUE", isValidMember, ',');
		assertNotValid_Depth("FALSE", isValidMember, ',');
		assertNotValid_Depth("NULL", isValidMember, ',');

		//--- void testJSONValidator__isValidMember::testNotMembers(void){
		assertNotValid_Depth("tru", isValidMember, ',');
		assertNotValid_Depth("fals", isValidMember, ',');
		assertNotValid_Depth("nul", isValidMember, ',');
		assertNotValid_Depth("", isValidMember, ',');  //needs a comma after it because of how the pipeline works
		assertNotValid_Depth("xxx", isValidMember, ',');
		assertNotValid_Depth("nonsense", isValidMember, ',');
		assertNotValid_Depth("1234", isValidMember, ',');
	#endif
}
