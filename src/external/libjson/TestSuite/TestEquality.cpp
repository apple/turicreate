#include "TestSuite.h"
#include "../Source/JSONNode.h"

void TestSuite::TestEquality(void){
    UnitTest::SetPrefix("TestEquality.cpp - Equality");
    #ifdef JSON_LIBRARY
		  JSONNODE * test1 = json_new(JSON_NODE);
		  JSONNODE * test2 = json_new(JSON_NODE);
		  assertTrue(json_equal(test1, test2));

		  //check literally the same internal pointer
		  json_set_n(test2, test1);
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #else
				  assertNotEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #endif
		  #endif
		  assertTrue(json_equal(test1, test2));

		  json_set_a(test1, JSON_TEXT("hello"));
		  json_set_a(test2, JSON_TEXT("hello"));
		  assertTrue(json_equal(test1, test2));

		  json_set_f(test1, 13.5f);
		  json_set_f(test2, 13.5f);
		  assertTrue(json_equal(test1, test2));

		  json_set_i(test1, 13);
		  json_set_f(test2, 13.0f);
		  assertTrue(json_equal(test1, test2));

		  json_set_b(test1, true);
		  json_set_b(test2, (int)true);
		  assertTrue(json_equal(test1, test2));

		  json_set_b(test1, false);
		  json_set_b(test2, (int)false);
		  assertTrue(json_equal(test1, test2));

		  json_nullify(test1);
		  json_nullify(test2);
		  assertTrue(json_equal(test1, test2));
		  JSONNODE * test3 = json_new(JSON_NULL);
		  assertTrue(json_equal(test1, test3));
		  assertTrue(json_equal(test3, test3));

		  json_delete(test1);
		  json_delete(test2);
		  json_delete(test3);
    #else
		  JSONNode test1;
		  JSONNode test2;
		  assertEquals(test1, test2);

		  //check literally the same internal pointer
		  test2 = test1;
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test2.internal);
			 #else
				  assertNotEquals(test1.internal, test2.internal);
			 #endif
		  #endif
		  assertEquals(test1, test2);

		  test1 = JSON_TEXT("hello");
		  test2 = JSON_TEXT("hello");
		  assertEquals(test1, JSON_TEXT("hello"));
		  assertEquals(test1, test2);

		  test1 = 13.5f;
		  test2 = 13.5f;
		  assertEquals(test1, 13.5f);
		  assertEquals(test1, test2);

		  test1 = 13;
		  test2 = 13.0f;
		  assertEquals(test1, 13.0f);
		  assertEquals(test1, 13);
		  assertEquals(test1, test2);

		  test1 = true;
		  test2 = true;
		  assertEquals(test1, true);
		  assertEquals(test1, test2);

		  test1 = false;
		  test2 = false;
		  assertEquals(test1, false);
		  assertEquals(test1, test2);

		  test1.nullify();
		  test2.nullify();
		  assertEquals(test1, test2);
		  JSONNode test3 = JSONNode(JSON_NULL);
		  assertEquals(test1, test3);
		  assertEquals(test2, test3);
    #endif
}
