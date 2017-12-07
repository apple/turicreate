#include "TestSuite.h"
#include "../Source/JSONNode.h"

void TestSuite::TestChildren(void){
    UnitTest::SetPrefix("TestChildren.cpp - Children");
    #ifdef JSON_LIBRARY
		  #define assertChild(node, index, func, value)\
			 if (JSONNODE * blabla = json_at(node, index)){\
				assertEquals(func(blabla), value);\
			 } else {\
				FAIL("no child");\
			 }

		  JSONNODE * test1 = json_new(JSON_NODE);
		  JSONNODE * test2 = json_new(JSON_NODE);


		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  assertEquals(json_type(test1), JSON_NODE);
		  assertEquals(json_type(test2), JSON_NODE);
		  assertEquals(json_size(test1), 0);
		  assertEquals(json_size(test2), 0);
		  assertTrue(json_equal(test1, test2));


		  json_push_back(test1, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
		  assertEquals(json_size(test1), 1);
		  assertFalse(json_equal(test1, test2));
		  json_push_back(test2, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
		  assertEquals(json_size(test2), 1);
		  assertTrue(json_equal(test1, test2));

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  json_merge(test1, test2);
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				assertEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #else
				assertNotEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #endif
		  #endif

		  UnitTest::SetPrefix("TestChildren.cpp - Children 2");

		  if (JSONNODE * temp = json_at(test1, 0)){
			 json_char * str = json_as_string(temp);
			 assertCStringSame(str, JSON_TEXT("world"));
			 json_free(str);
			 str = json_name(temp);
			 assertCStringSame(str, JSON_TEXT("hi"));
			 json_free(str);
		  } else {
			 FAIL("at failed");
		  }

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  assertEquals(json_size(test1), 1);
		  if (JSONNODE * temp = json_pop_back_at(test1, 0)){
			 json_char * str = json_as_string(temp);
			 assertCStringSame(str, JSON_TEXT("world"));
			 json_free(str);
			 assertEquals(json_size(test1), 0);
			 json_delete(temp);
		  } else {
			 FAIL("POP FAILED");
		  }

		  UnitTest::SetPrefix("TestChildren.cpp - Children 3");

		  json_push_back(test1, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
		  if (JSONNODE * temp = json_pop_back(test1, JSON_TEXT("hi"))){
			 json_char * str = json_as_string(temp);
			 assertCStringSame(str, JSON_TEXT("world"));
			 json_free(str);
			 assertEquals(json_size(test1), 0);
			 json_delete(temp);
		  } else {
			 FAIL("POP name FAILED");
		  }

		  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
			 json_push_back(test1, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
			 if (JSONNODE * temp = json_pop_back_nocase(test1, JSON_TEXT("HI"))){
				json_char * str = json_as_string(temp);
				assertCStringSame(str, JSON_TEXT("world"));
				json_free(str);
				assertEquals(json_size(test1), 0);
				json_delete(temp);
			 } else {
				FAIL("POP name FAILED");
			 }
		  #endif

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  UnitTest::SetPrefix("TestChildren.cpp - Children 4");


		  assertEquals(json_size(test1), 0);
		  json_push_back(test1, json_new_i(JSON_TEXT("one"), 1));
		  json_push_back(test1, json_new_i(JSON_TEXT("two"), 2));
		  json_push_back(test1, json_new_i(JSON_TEXT("three"), 3));
		  json_push_back(test1, json_new_i(JSON_TEXT("four"), 4));
		  json_push_back(test1, json_new_i(JSON_TEXT("five"), 5));
		  json_push_back(test1, json_new_i(JSON_TEXT("six"), 6));
		  assertEquals(json_size(test1), 6);

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);


		  if (JSONNODE * temp = json_pop_back(test1, JSON_TEXT("four"))){
			 assertEquals(json_as_int(temp), 4);
			 assertChild(test1, 0, json_as_int, 1);
			 assertChild(test1, 1, json_as_int, 2);
			 assertChild(test1, 2, json_as_int, 3);
			 assertChild(test1, 3, json_as_int, 5);
			 assertChild(test1, 4, json_as_int, 6);
			 assertEquals(json_size(test1), 5);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
			 json_delete(temp);
		  } else {
			 FAIL("no pop");
		  }

		  UnitTest::SetPrefix("TestChildren.cpp - Children 5");

		  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
			 if (JSONNODE * temp = json_pop_back_nocase(test1, JSON_TEXT("SIX"))){
		  #else
			 if (JSONNODE * temp = json_pop_back(test1, JSON_TEXT("six"))){
		  #endif
			 assertEquals(json_as_int(temp), 6);
			 assertChild(test1, 0, json_as_int, 1);
			 assertChild(test1, 1, json_as_int, 2);
			 assertChild(test1, 2, json_as_int, 3);
			 assertChild(test1, 3, json_as_int, 5);
			 assertEquals(json_size(test1), 4);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
			 json_delete(temp);
		  } else {
			 FAIL("no pop_nocase");
		  }

		  UnitTest::SetPrefix("TestChildren.cpp - Children 6");

		  if (JSONNODE * temp = json_pop_back_at(test1, 2)){
			 assertEquals(json_as_int(temp), 3);
			 assertChild(test1, 0, json_as_int, 1);
			 assertChild(test1, 1, json_as_int, 2);
			 assertChild(test1, 2, json_as_int, 5);
			 assertEquals(json_size(test1), 3);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
			 json_delete(temp);
		  } else {
			 FAIL("no pop 2");
		  }

		  json_delete(test1);
		  json_delete(test2);

		  #ifdef JSON_UNIT_TEST
				  JSONNODE * fresh = json_new(JSON_NODE);
				  json_reserve(fresh, 3);
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mycapacity, 3);
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mysize, 0);
				  json_push_back(fresh, json_new(JSON_NULL));
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mycapacity, 3);
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mysize, 1);
				  json_push_back(fresh, json_new(JSON_NULL));
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mycapacity, 3);
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mysize, 2);
				  json_push_back(fresh, json_new(JSON_NULL));
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mycapacity, 3);
				  assertEquals(((JSONNode*)fresh) -> internal -> CHILDREN -> mysize, 3);
				  json_delete(fresh);
		  #endif


    #else
		  JSONNode test1;
		  JSONNode test2;
		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  assertEquals(test1.type(), JSON_NODE);
		  assertEquals(test2.type(), JSON_NODE);
		  assertEquals(test1.size(), 0);
		  assertEquals(test2.size(), 0);
		  assertEquals(test1, test2);
		  test1.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
		  assertEquals(test1.size(), 1);
		  assertNotEquals(test1, test2);
		  test2.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
		  assertEquals(test2.size(), 1);
		  assertEquals(test1, test2);

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  test1.merge(test2);
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test2.internal);
			 #else
				  assertNotEquals(test1.internal, test2.internal);
			 #endif
		  #endif

		  try {
			 assertEquals(test1.at(0), JSON_TEXT("world"));
			 assertEquals(test1.at(0).name(), JSON_TEXT("hi"));
		  } catch (std::out_of_range){
			 FAIL("exception caught");
		  }

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

		  assertEquals(test1.size(), 1);
		  try {
			 JSONNode res = test1.pop_back(0);
			 assertEquals(res, JSON_TEXT("world"));
			 assertEquals(test1.size(), 0);
			 test1.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
			 res = test1.pop_back(JSON_TEXT("hi"));
			 assertEquals(res, JSON_TEXT("world"));
			 assertEquals(test1.size(), 0);
			 #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
				test1.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
				res = test1.pop_back_nocase(JSON_TEXT("HI"));
				assertEquals(res, JSON_TEXT("world"));
				assertEquals(test1.size(), 0);
			 #endif
		  } catch (std::out_of_range){
			 FAIL("exception caught 2");
		  }

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);


		  assertEquals(test1.size(), 0);
		  test1.push_back(JSONNode(JSON_TEXT("one"), 1));
		  test1.push_back(JSONNode(JSON_TEXT("two"), 2));
		  test1.push_back(JSONNode(JSON_TEXT("three"), 3));
		  test1.push_back(JSONNode(JSON_TEXT("four"), 4));
		  test1.push_back(JSONNode(JSON_TEXT("five"), 5));
		  test1.push_back(JSONNode(JSON_TEXT("six"), 6));
		  assertEquals(test1.size(), 6);

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);

				//echo(test1.dump().write_formatted());

		  JSONNode res;

		  try {
			 res = test1.pop_back(JSON_TEXT("four"));
			 assertEquals(res, 4);
			 assertEquals(test1[0], 1);
			 assertEquals(test1[1], 2);
			 assertEquals(test1[2], 3);
			 assertEquals(test1[3], 5);
			 assertEquals(test1[4], 6);
			 assertEquals(test1.size(), 5);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
		  } catch (std::out_of_range){
			 FAIL("exception caught pop");
		  }

		  try {
			 #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
				res = test1.pop_back_nocase(JSON_TEXT("SIX"));
			 #else
				res = test1.pop_back(JSON_TEXT("six"));
			 #endif
			 assertEquals(res, 6);
			 assertEquals(test1[0], 1);
			 assertEquals(test1[1], 2);
			 assertEquals(test1[2], 3);
			 assertEquals(test1[3], 5);
			 assertEquals(test1.size(), 4);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
		  } catch (std::out_of_range){
			 FAIL("exception caught pop_nocase");
		  }

		  try {
			 res = test1.pop_back(2);
			 assertEquals(res, 3);
			 assertEquals(test1[0], 1);
			 assertEquals(test1[1], 2);
			 assertEquals(test1[2], 5);
			 assertEquals(test1.size(), 3);

			 TestSuite::testParsingItself(test1);
			 TestSuite::testParsingItself(test2);
		  } catch (std::out_of_range){
			 FAIL("exception caught pop 2");
		  }


		  #ifdef JSON_UNIT_TEST
			 JSONNode fresh(JSON_NODE);
			 fresh.reserve(3);
			 assertEquals(fresh.internal -> CHILDREN -> mycapacity, 3);

			 assertEquals(fresh.internal -> CHILDREN -> mysize, 0);
			 fresh.push_back(JSONNode(JSON_NULL));
			 assertEquals(fresh.internal -> CHILDREN -> mycapacity, 3);

			 assertEquals(fresh.internal -> CHILDREN -> mysize, 1);
			 fresh.push_back(JSONNode(JSON_NULL));
			 assertEquals(fresh.internal -> CHILDREN -> mycapacity, 3);
			 assertEquals(fresh.internal -> CHILDREN -> mysize, 2);
			 fresh.push_back(JSONNode(JSON_NULL));
			 assertEquals(fresh.internal -> CHILDREN -> mycapacity, 3);
			 assertEquals(fresh.internal -> CHILDREN -> mysize, 3);
		  #endif
    #endif
}
