#include "TestSuite.h"
#include "../Source/JSONNode.h"

void TestSuite::TestConstructors(void){
    UnitTest::SetPrefix("TestCtor.cpp - Constructors");
    #ifdef JSON_LIBRARY
		  JSONNODE * test = json_new(JSON_NULL);
		  assertEquals(json_type(test), JSON_NULL);
		  json_delete(test);

		  test = json_new_a(JSON_TEXT("hello"), JSON_TEXT("world"));
		  json_char * res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("world"));
		  json_free(res);
		  res = json_name(test);
		  assertCStringSame(res, JSON_TEXT("hello"));
		  json_free(res);
		  assertEquals(json_type(test), JSON_STRING);
		  json_delete(test);

		  test = json_new_i(JSON_TEXT("hello"), 15);
		#ifdef JSON_CASTABLE
		  res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("15"));
		  json_free(res);
		#endif
		  assertEquals_Primitive(json_as_int(test), 15);
		  assertEquals_Primitive(json_as_float(test), 15.0f);
		  res = json_name(test);
		  assertCStringSame(res, JSON_TEXT("hello"));
		  json_free(res);
		  assertEquals(json_type(test), JSON_NUMBER);
		  json_delete(test);

		  test = json_new_f(JSON_TEXT("hello"), 15.5f);
		  assertEquals_Primitive(json_as_int(test), 15);
		  assertEquals_Primitive(json_as_float(test), 15.5f);
		#ifdef JSON_CASTABLE
		  res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("15.5"));
		  json_free(res);
		#endif
		  res = json_name(test);
		  assertCStringSame(res, JSON_TEXT("hello"));
		  json_free(res);
		  assertEquals(json_type(test), JSON_NUMBER);
		  json_delete(test);

		  test = json_new_b(JSON_TEXT("hello"), (int)true);
		#ifdef JSON_CASTABLE
		  res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("true"));
		  json_free(res);
		#endif
		  assertEquals(json_as_bool(test), (int)true);
		  res = json_name(test);
		  assertCStringSame(res, JSON_TEXT("hello"));
		  json_free(res);
		  assertEquals(json_type(test), JSON_BOOL);

		  JSONNODE * cpy = json_copy(test);
		  assertTrue(json_equal(cpy, test));
		  json_delete(cpy);

		  json_delete(test);
    #else
		  JSONNode test = JSONNode(JSON_NULL);
		  assertEquals(test.type(), JSON_NULL);

		  test = JSONNode(JSON_TEXT("hello"), JSON_TEXT("world"));
		  assertEquals(test, JSON_TEXT("world"));
		  assertEquals(test.as_string(), JSON_TEXT("world"));
		  assertEquals(test.name(), JSON_TEXT("hello"));
		  assertEquals(test.type(), JSON_STRING);

		  test = JSONNode(JSON_TEXT("hello"), 15);
		  assertEquals(test, 15);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("15"));
		#endif
		  assertEquals(test.as_int(), 15);
		  assertEquals(test.as_float(), 15.0f);
		  assertEquals(test.name(), JSON_TEXT("hello"));
		  assertEquals(test.type(), JSON_NUMBER);

		  test = JSONNode(JSON_TEXT("hello"), 15.5f);
		  assertEquals(test, 15.5f);
		  assertEquals(test.as_int(), 15);
		  assertEquals(test.as_float(), 15.5f);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("15.5"));
		#endif
		  assertEquals(test.name(), JSON_TEXT("hello"));
		  assertEquals(test.type(), JSON_NUMBER);

		  test = JSONNode(JSON_TEXT("hello"), true);
		  assertEquals(test, true);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("true"));
		#endif
		  assertEquals(test.as_bool(), true);
		  assertEquals(test.name(), JSON_TEXT("hello"));
		  assertEquals(test.type(), JSON_BOOL);

		  test = JSONNode(json_string(JSON_TEXT("hello")), JSON_TEXT('\0'));
		  assertEquals(test, 0);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("0"));
		#endif
		  assertEquals(test.as_int(), 0);
		  assertEquals(test.as_float(), 0.0f);
		  assertEquals(test.name(), JSON_TEXT("hello"));
		  assertEquals(test.type(), JSON_NUMBER);
    #endif
}
