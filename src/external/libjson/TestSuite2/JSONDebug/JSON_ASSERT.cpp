#include "JSON_ASSERT.h"
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

const json_string fail_consta = JSON_TEXT("fail"); //should pass the same pointer all the way through, no copies
const json_string null_consta = JSON_TEXT("");
#if defined JSON_DEBUG || defined JSON_SAFE
	json_error_callback_t origCallbacka = NULL;
#endif

void testJSONDebug_JSON_ASSERT::setUp(const std::string & methodName){
	BaseTest::setUp(methodName);
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			origCallbacka = JSONDebug::register_callback(callback);  //check that the callback was called
			last = null_consta;
		#endif
	#endif
}

void testJSONDebug_JSON_ASSERT::tearDown(void){
	BaseTest::tearDown();
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSONDebug::register_callback(origCallbacka);  //check that the callback was called
		#endif
	#endif
}


/**
 *	Make sure asserts that pass do not call the callback or run extra code
 */
void testJSONDebug_JSON_ASSERT::testPass(void){
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSON_ASSERT(1 == 1, fail_consta);
			assertEquals(last, null_consta);  //make sure the callback was not called
		#endif
	#endif
}


/**
 *	Make sure asserts that fail do call the callback and run extra code
 */
void testJSONDebug_JSON_ASSERT::testFail(void){
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSON_ASSERT(1 == 0, fail_consta);
			assertEquals(last, fail_consta);  //make sure the callback was actually called
		#endif
	#endif
}
