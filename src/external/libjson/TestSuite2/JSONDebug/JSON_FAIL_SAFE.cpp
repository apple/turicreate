#include "JSON_FAIL_SAFE.h"
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

const json_string fail_constfs = JSON_TEXT("fail"); //should pass the same pointer all the way through, no copies
const json_string null_constfs = JSON_TEXT("");
#if defined JSON_DEBUG || defined JSON_SAFE
	json_error_callback_t origCallbackfs = NULL;
#endif

void testJSONDebug_JSON_FAIL_SAFE::setUp(const std::string & methodName){
	BaseTest::setUp(methodName);
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			origCallbackfs = JSONDebug::register_callback(callback);  //check that the callback was called
			last = null_constfs;
		#endif
	#endif
}

void testJSONDebug_JSON_FAIL_SAFE::tearDown(void){
	BaseTest::tearDown();
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSONDebug::register_callback(origCallbackfs);  //check that the callback was called
		#endif
	#endif
}


/**
 *	Make sure fails do call the callback and run extra code
 */
void testJSONDebug_JSON_FAIL_SAFE::testFail(void){
	int i = 0;
	JSON_FAIL_SAFE(fail_constfs, i = 1;);
	#if defined(JSON_SAFE)
		assertEquals(i, 1);  //safe caught the code
	#else
		assertEquals(i, 0);  //fell through because no safety catch
	#endif
	
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			assertEquals(last, fail_constfs);  //make sure the callback was actually called
		#endif
	#endif
}
