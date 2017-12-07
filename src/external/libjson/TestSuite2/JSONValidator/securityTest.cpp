#include "securityTest.h"
#include "Resources/validyMacros.h"
#include "../../Source/JSONValidator.h"

void testJSONValidator__securityTest::testsecurity(void){
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
}
