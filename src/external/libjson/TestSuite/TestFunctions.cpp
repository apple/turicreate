#include "TestSuite.h"
#include "../Source/JSONNode.h"

void TestSuite::TestFunctions(void){
    UnitTest::SetPrefix("TestFunctions.cpp - Swap");
    #ifdef JSON_LIBRARY
		  JSONNODE * test1 = json_new(JSON_NODE);
		  JSONNODE * test2 = json_new(JSON_NODE);
		  json_set_i(test1, 14);
		  json_set_i(test2, 35);
		  json_swap(test1, test2);
		  assertEquals_Primitive(json_as_int(test1), 35);
		  assertEquals_Primitive(json_as_int(test2), 14);

		  UnitTest::SetPrefix("TestFunctions.cpp - Duplicate");
		  json_delete(test1);
		  test1 = json_duplicate(test2);
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
		  #endif
		  assertTrue(json_equal(test1, test2));


		  UnitTest::SetPrefix("TestFunctions.cpp - Duplicate with children");
		  JSONNODE * node = json_new(JSON_NODE);
		  json_push_back(node, json_new_i(JSON_TEXT(""), 15));
		  json_push_back(node, json_new_f(JSON_TEXT(""), 27.4f));
		  json_push_back(node, json_new_b(JSON_TEXT(""), true));

		  TestSuite::testParsingItself(node);

		  JSONNODE * dup = json_duplicate(node);
		  assertEquals(json_size(dup), 3);
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(((JSONNode*)node) -> internal, ((JSONNode*)dup) -> internal);
		  #endif
		  assertEquals(json_type(dup), JSON_NODE);

		  TestSuite::testParsingItself(node);
		  TestSuite::testParsingItself(dup);

		  assertEquals_Primitive(json_as_int(json_at(dup, 0)), 15);
		  assertEquals_Primitive(json_as_float(json_at(dup, 1)), 27.4f);
		  assertEquals(json_as_bool(json_at(dup, 2)), true);
		  assertTrue(json_equal(json_at(dup, 0), json_at(node, 0)));
		  assertTrue(json_equal(json_at(dup, 1), json_at(node, 1)));
		  assertTrue(json_equal(json_at(dup, 2), json_at(node, 2)));


		  TestSuite::testParsingItself(dup);

		  #ifdef JSON_ITERATORS
			 for(JSONNODE_ITERATOR it = json_begin(node), end = json_end(node), dup_it = json_begin(dup);
				it != end;
				++it, ++dup_it){
				assertTrue(json_equal(*it, *dup_it));
				#ifdef JSON_UNIT_TEST
				    assertNotEquals(((JSONNode*)(*it)) -> internal, ((JSONNode*)(*dup_it)) -> internal);
				#endif
			 }
		  #endif

		  UnitTest::SetPrefix("TestFunctions.cpp - Nullify");
		  json_nullify(test1);
		  assertEquals(json_type(test1), JSON_NULL);
		  json_char * res = json_name(test1);
		  assertCStringSame(res, JSON_TEXT(""));
		  json_free(res);

		  #ifdef JSON_CASTABLE
			  UnitTest::SetPrefix("TestFunctions.cpp - Cast");
			  json_cast(test1, JSON_NULL);
			  json_set_i(test2, 1);
			  json_cast(test2, JSON_BOOL);
			  assertEquals(json_type(test1), JSON_NULL);
			  assertEquals(json_type(test2), JSON_BOOL);
			  assertEquals(json_as_bool(test2), true);
			  json_set_b(test2, true);
			  assertEquals(json_as_bool(test2), true);

			  json_cast(test2, JSON_NUMBER);
			  assertEquals_Primitive(json_as_float(test2), 1.0f);
			  json_set_f(test2, 0.0f);
			  assertEquals_Primitive(json_as_float(test2), 0.0f);
			  json_cast(test2, JSON_BOOL);
			  assertEquals(json_as_bool(test2), false);
		  #endif

		  UnitTest::SetPrefix("TestFunctions.cpp - Merge");
		  json_set_a(test1, JSON_TEXT("hello"));
		  json_set_a(test2, JSON_TEXT("hello"));
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
		  #endif
		  assertTrue(json_equal(test1, test2));
		  json_merge(test1, test2);
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #else
				  assertNotEquals(((JSONNode*)test1) -> internal, ((JSONNode*)test2) -> internal);
			 #endif
		  #endif

		#ifdef JSON_CASTABLE
			  json_cast(test1, JSON_NODE);
			  json_cast(test2, JSON_NODE);
			  assertEquals(json_type(test1), JSON_NODE);
			  assertEquals(json_type(test2), JSON_NODE);
			  json_push_back(test1, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
			  json_push_back(test2, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));

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

			  TestSuite::testParsingItself(test1);
			  TestSuite::testParsingItself(test2);
		#endif

		  json_delete(test1);
		  json_delete(test2);
		  json_delete(node);
		  json_delete(dup);
    #else
		  JSONNode test1;
		  JSONNode test2;
		  test1 = JSON_TEXT("hello");
		  test2 = JSON_TEXT("world");
		  test1.swap(test2);
		  assertEquals(test1, JSON_TEXT("world"));
		  assertEquals(test2, JSON_TEXT("hello"));

		  UnitTest::SetPrefix("TestFunctions.cpp - Duplicate");
		  test1 = test2.duplicate();
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(test1.internal, test2.internal);
		  #endif
		  assertEquals(test1, test2);

		  UnitTest::SetPrefix("TestFunctions.cpp - Duplicate with children");
		  JSONNode node = JSONNode(JSON_NODE);
		  node.push_back(JSONNode(JSON_TEXT(""), 15));
		  node.push_back(JSONNode(JSON_TEXT(""), JSON_TEXT("hello world")));
		  node.push_back(JSONNode(JSON_TEXT(""), true));

		  TestSuite::testParsingItself(node);

		  JSONNode dup = node.duplicate();
		  assertEquals(dup.size(), 3);
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(node.internal, dup.internal);
		  #endif
		  assertEquals(dup.type(), JSON_NODE);

		  TestSuite::testParsingItself(node);
		  TestSuite::testParsingItself(dup);

		  try {
			 assertEquals(dup.at(0), 15);
			 assertEquals(dup.at(1), JSON_TEXT("hello world"));
			 assertEquals(dup.at(2), true);
			 assertEquals(dup.at(0), node.at(0));
			 assertEquals(dup.at(1), node.at(1));
			 assertEquals(dup.at(2), node.at(2));
		  } catch (std::out_of_range){
			 FAIL("exception caught");
		  }

		  TestSuite::testParsingItself(dup);

		  #ifdef JSON_ITERATORS
			 for(JSONNode::iterator it = node.begin(), end = node.end(), dup_it = dup.begin();
				it != end;
				++it, ++dup_it){
				assertEquals(*it, *dup_it);
				#ifdef JSON_UNIT_TEST
				    assertNotEquals((*it).internal, (*dup_it).internal);
				#endif
			 }
		  #endif

		  UnitTest::SetPrefix("TestFunctions.cpp - Nullify");
		  test1.nullify();
		  assertEquals(test1.type(), JSON_NULL);
		  assertEquals(test1.name(), JSON_TEXT(""));

		  #ifdef JSON_CASTABLE
			 UnitTest::SetPrefix("TestFunctions.cpp - Cast");
			 test1.cast(JSON_NULL);
			 test2 = 1;
			 test2.cast(JSON_BOOL);
			 assertEquals(test1.type(), JSON_NULL);
			 assertEquals(test2.type(), JSON_BOOL);
			 assertEquals(test2, true);
			 test2 = true;
			 assertEquals(test2, true);
			 test2.cast(JSON_NUMBER);
			 assertEquals(test2, 1.0f);
			 test2 = 0.0f;
			 assertEquals(test2, 0.0f);
			 test2.cast(JSON_BOOL);
			 assertEquals(test2, false);
		  #endif
    
		  UnitTest::SetPrefix("TestFunctions.cpp - Merge");
		  test1 = JSON_TEXT("hello");
		  test2 = JSON_TEXT("hello");
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(test1.internal, test2.internal);
		  #endif
		  assertEquals(test1, test2);
		  test1.merge(test2);
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test2.internal);
			 #else
				  assertNotEquals(test1.internal, test2.internal);
			 #endif
		  #endif

		  #ifdef JSON_CASTABLE
			 test1.cast(JSON_NODE);
			 test2.cast(JSON_NODE);
		  #else
			 test1 = JSONNode(JSON_NODE);
			 test2 = JSONNode(JSON_NODE);
		  #endif
		  assertEquals(test1.type(), JSON_NODE);
		  assertEquals(test2.type(), JSON_NODE);
		  test1.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
		  test2.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));

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

		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);
    #endif
}
