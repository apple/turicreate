#include "JSON_FAIL.h"
#include "../../Source/JSONDebug.h"

#if defined JSON_DEBUG
	#ifndef JSON_STDERROR
		static json_string last;
		#ifdef JSON_LIBRARY
			static void callback(const json_char * p){ last = p; }
		#else
			static void callback(const json_string & p){ last = p; }	
		#endif
	#endif
#endif

const json_string fail_constf = JSON_TEXT("fail"); //should pass the same pointer all the way through, no copies
const json_string null_constf = JSON_TEXT("");
#if defined JSON_DEBUG || defined JSON_SAFE
	json_error_callback_t origCallbackf = NULL;
#endif

void testJSONDebug_JSON_FAIL::setUp(const std::string & methodName){
	BaseTest::setUp(methodName);
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			origCallbackf = JSONDebug::register_callback(callback);  //check that the callback was called
			last = null_constf;
		#endif
	#endif
}

void testJSONDebug_JSON_FAIL::tearDown(void){
	BaseTest::tearDown();
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSONDebug::register_callback(origCallbackf);  //check that the callback was called
		#endif
	#endif
}


/**
 *	Make sure fails do call the callback
 */
void testJSONDebug_JSON_FAIL::testFail(void){
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSON_FAIL(fail_constf);
			assertEquals(last, fail_constf);  //make sure the callback was actually called
		#endif
	#endif
}
