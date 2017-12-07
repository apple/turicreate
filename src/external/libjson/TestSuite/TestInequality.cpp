#include "TestSuite.h"

void TestSuite::TestInequality(void){
    UnitTest::SetPrefix("TestInequality.cpp - Inequality");
    #ifdef JSON_LIBRARY
		  JSONNODE * test1 = json_new(JSON_NODE);
		  JSONNODE * test2 = json_new(JSON_NODE);
		  json_set_a(test1, JSON_TEXT("hello"));
		  json_set_a(test2, JSON_TEXT("world"));
		  assertFalse(json_equal(test1, test2));

		  json_set_i(test2,13);
		  assertFalse(json_equal(test1, test2));

		  json_set_f(test2, 13.5f);
		  assertFalse(json_equal(test1, test2));

		  json_set_b(test2, true);
		  assertFalse(json_equal(test1, test2));

		  json_set_b(test2, false);
		  assertFalse(json_equal(test1, test2));

		  json_nullify(test2);
		  assertFalse(json_equal(test1, test2));
		  json_delete(test1);
		  json_delete(test2);
    #else
		  JSONNode test1;
		  JSONNode test2;
		  test1 = JSON_TEXT("hello");
		  test2 = JSON_TEXT("world");
		  assertNotEquals(test1, test2);
		  assertNotEquals(test1, JSON_TEXT("hi"));
		  assertNotEquals(test2, 13.5f);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, true);
		  assertNotEquals(test2, false);

		  test2 = 13;
		  assertNotEquals(test1, test2);
		  assertNotEquals(test2, 13.5f);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, true);
		  assertNotEquals(test2, false);
		  assertNotEquals(test2, JSON_TEXT("13"));  //not the same type

		  test2 = 13.5f;
		  assertNotEquals(test1, test2);
		  assertNotEquals(test2, 13);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, true);
		  assertNotEquals(test2, false);
		  assertNotEquals(test2, JSON_TEXT("13.5"));  //not the same type

		  test2 = true;
		  assertNotEquals(test1, test2);
		  assertNotEquals(test2, 13.5f);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, false);
		  assertNotEquals(test2, JSON_TEXT("true"));  //not the same type

		  test2 = false;
		  assertNotEquals(test1, test2);
		  assertNotEquals(test2, 13.5f);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, true);
		  assertNotEquals(test2, JSON_TEXT("false"));  //not the same type

		  test2.nullify();
		  assertNotEquals(test1, test2);
		  assertNotEquals(test2, 13.5f);
		  assertNotEquals(test2, 14);
		  assertNotEquals(test2, true);
		  assertNotEquals(test2, false);
		  assertNotEquals(test2, "null");  //not the same type
    #endif
}
