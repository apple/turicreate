#include "json_decode64.h"
#include "../../Source/JSON_Base64.h"

void testJSON_Base64__json_decode64::testNotBase64(void){
	#if defined(JSON_BINARY) || defined(JSON_EXPOSE_BASE64)
		#ifdef JSON_SAFE
			assertEquals(JSONBase64::json_decode64(JSON_TEXT("123!abc")), "");
			assertEquals(JSONBase64::json_decode64(JSON_TEXT("123=abc")), "");
			assertEquals(JSONBase64::json_decode64(JSON_TEXT("123abc===")), "");
		#endif
	#endif
}
