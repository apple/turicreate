#include "isValidRoot.h"
#include "Resources/validyMacros.h"
#include "../../Source/JSONValidator.h"

void testJSONValidator__isValidRoot::testRoots(void){
	#ifdef JSON_VALIDATE
		assertTrue(JSONValidator::isValidRoot(JSON_TEXT("{}")));
		assertTrue(JSONValidator::isValidRoot(JSON_TEXT("[]")));
		assertTrue(JSONValidator::isValidRoot(JSON_TEXT("[\"stuff\"]")));
	#endif
}

void testJSONValidator__isValidRoot::testNotRoots(void){
	#ifdef JSON_VALIDATE
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{]")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[}")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{}aoe")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[]aoe")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("aoe")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[\"stuff\":{},]")));
	#endif
}

void testJSONValidator__isValidRoot::testSuddenEnd(void){
	#ifdef JSON_VALIDATE
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("")));
	
		//--- void testJSONValidator__isValidRoot::testRoots(void){
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[\"stuff")));
	
		//---void testJSONValidator__isValidRoot::testNotRoots(void){
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("{}aoe")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("[]aoe")));
		assertFalse(JSONValidator::isValidRoot(JSON_TEXT("aoe")));
	#endif
}
