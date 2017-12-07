#include "JSON_ASSERT_SAFE.h"
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

const json_string fail_const = JSON_TEXT("fail"); //should pass the same pointer all the way through, no copies
const json_string null_const = JSON_TEXT("");
#if defined JSON_DEBUG || defined JSON_SAFE
	json_error_callback_t origCallback = NULL;
#endif

void testJSONDebug_JSON_ASSERT_SAFE::setUp(const std::string & methodName){
	BaseTest::setUp(methodName);
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			origCallback = JSONDebug::register_callback(callback);  //check that the callback was called
			last = null_const;
		#endif
	#endif
}

void testJSONDebug_JSON_ASSERT_SAFE::tearDown(void){
	BaseTest::tearDown();
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			JSONDebug::register_callback(origCallback);  //check that the callback was called
		#endif
	#endif
}


/**
 *	Make sure asserts that pass do not call the callback or run extra code
 */
void testJSONDebug_JSON_ASSERT_SAFE::testPass(void){
	int i = 0;
	JSON_ASSERT_SAFE(1 == 1, fail_const, i = 1;);
	assertEquals(i, 0);
	
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			assertEquals(last, null_const);  //make sure the callback was not called
		#endif
	#endif
}


/**
 *	Make sure asserts that fail do call the callback and run extra code
 */
void testJSONDebug_JSON_ASSERT_SAFE::testFail(void){
	int i = 0;
	JSON_ASSERT_SAFE(1 == 0, fail_const, i = 1;);
	#if defined(JSON_SAFE)
		assertEquals(i, 1);  //safe caught the code
	#else
		assertEquals(i, 0);  //fell through because no safety catch
	#endif
	
	#if defined JSON_DEBUG
		#ifndef JSON_STDERROR
			assertEquals(last, fail_const);  //make sure the callback was actually called
		#endif
	#endif
}
