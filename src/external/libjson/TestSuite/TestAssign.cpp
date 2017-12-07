#include "TestSuite.h"
#include "../Source/JSONNode.h"

void TestSuite::TestAssigning(void){
    UnitTest::SetPrefix("TestAssign.cpp - Assigning");
    #ifdef JSON_LIBRARY
		  //check names
		  JSONNODE * test1 = json_new(JSON_NODE);
		  json_set_name(test1, JSON_TEXT("hello world"));
		  json_char * res = json_name(test1);
		  assertCStringSame(res, JSON_TEXT("hello world"));
		  json_free(res);

		  //check strings
		  json_set_a(test1, JSON_TEXT("Hello world"));
		  assertEquals(json_type(test1), JSON_STRING);
		  res = json_as_string(test1);
		  assertCStringSame(res, JSON_TEXT("Hello world"));
		  json_free(res);

		  //check ints
		  json_set_i(test1, 13);
		  assertEquals(json_type(test1), JSON_NUMBER);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("13"));
		#endif
		  json_free(res);
		  assertEquals(json_as_int(test1), 13);
		  assertEquals(json_as_float(test1), 13.0f);

		  //check doubles work
		  json_set_f(test1, 13.7f);
		  assertEquals(json_type(test1), JSON_NUMBER);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("13.7"));
		#endif
		  json_free(res);
		  assertEquals(json_as_int(test1), 13);
		  assertEquals(json_as_float(test1), 13.7f);

		  //test making sure stripping the trailing period works
		  json_set_f(test1, 13.0f);
		  assertEquals(json_type(test1), JSON_NUMBER);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("13"));
		#endif
		  json_free(res);
		  assertEquals(json_as_int(test1), 13);
		  assertEquals(json_as_float(test1), 13.0f);

		  //check boolean
		  json_set_b(test1, (int)true);
		  assertEquals(json_type(test1), JSON_BOOL);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("true"));
		#endif
		  json_free(res);
		  assertEquals(json_as_bool(test1), true);

		  //check boolean
		  json_set_b(test1, false);
		  assertEquals(json_type(test1), JSON_BOOL);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("false"));
		#endif
		  json_free(res);
		  assertEquals(json_as_bool(test1), false);

		  //check null
		  json_nullify(test1);
		  assertEquals(json_type(test1), JSON_NULL);
		  res = json_as_string(test1);
		#ifdef JSON_CASTABLE
		  assertCStringSame(res, JSON_TEXT("null"));
		#endif
		  json_free(res);

		  json_delete(test1);

    #else
		  //check names
		  JSONNode test1;
		  test1.set_name(JSON_TEXT("hello world"));
		  assertEquals(test1.name(), JSON_TEXT("hello world"));

		  //check strings
		  test1 = JSON_TEXT("Hello world");
		  assertEquals(test1.type(), JSON_STRING);
		  assertEquals(test1.as_string(), JSON_TEXT("Hello world"));

		  //test chars
		  test1 = JSON_TEXT('\0');
		  assertEquals(test1.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("0"));
		#endif
		  assertEquals(test1.as_int(), 0);
		  assertEquals(test1.as_float(), 0.0f);

		  //check ints
		  test1 = 13;
		  assertEquals(test1.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("13"));
		#endif
		  assertEquals(test1.as_int(), 13);
		  assertEquals(test1.as_float(), 13.0f);

		  //check doubles work
		  test1 = 13.7f;
		  assertEquals(test1.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("13.7"));
		#endif
		  assertEquals(test1.as_int(), 13);
		  assertEquals(test1.as_float(), 13.7f);

		  //test making sure stripping hte trailing period works
		  test1 = 13.0f;
		  assertEquals(test1.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("13"));
		#endif
		  assertEquals(test1.as_int(), 13);
		  assertEquals(test1.as_float(), 13.0f);

		  //check boolean
		  test1 = true;
		  assertEquals(test1.type(), JSON_BOOL);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("true"));
		#endif
		  assertEquals(test1.as_bool(), true);

		  //check boolean
		  test1 = false;
		  assertEquals(test1.type(), JSON_BOOL);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("false"));
		#endif
		  assertEquals(test1.as_bool(), false);

		  //check null
		  test1.nullify();
		  assertEquals(test1.type(), JSON_NULL);
		#ifdef JSON_CASTABLE
		  assertEquals(test1.as_string(), JSON_TEXT("null"));
		#endif
    #endif
}
