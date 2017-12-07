#include "TestSuite.h"

void TestSuite::TestReferenceCounting(void){
    UnitTest::SetPrefix("TestRefCounting.cpp - Reference Counting");
    #ifdef JSON_LIBRARY
    #else
		  JSONNode test1;
		  #ifdef JSON_UNIT_TEST
			 assertNotNull(test1.internal);
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal -> refcount, 1);
			 #endif
		  #endif

		  //copy ctor, should simply increment the reference counter
		  JSONNode test2 = JSONNode(test1);
		  #ifdef JSON_REF_COUNT
			 #ifdef JSON_UNIT_TEST
				assertEquals(test1.internal, test2.internal);
			 #endif
			 assertEquals(test1, test2);
			 #ifdef JSON_UNIT_TEST
				assertEquals(test1.internal -> refcount, 2);
			 #endif
		  #else
			 #ifdef JSON_UNIT_TEST
				assertNotEquals(test1.internal, test2.internal);
			 #endif
			 assertEquals(test1, test2);
		  #endif

		  //assignment operator, should simply increment the reference counter
		  JSONNode test3 = test2;
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test3.internal);
				  assertEquals(test2.internal, test3.internal);
				  assertEquals(test1.internal -> refcount, 3);
			 #else
				  assertNotEquals(test1.internal, test3.internal);
				  assertNotEquals(test2.internal, test3.internal);
			 #endif
		  #endif

		  //assigning something, it should copy now
		  test2 = "hello";
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test3.internal);
				  assertNotEquals(test2.internal, test3.internal);
				  assertEquals(test1.internal -> refcount, 2);
				  assertEquals(test2.internal -> refcount, 1);
			 #else
				  assertNotEquals(test1.internal, test3.internal);
				  assertNotEquals(test2.internal, test3.internal);
			 #endif
		  #endif

		  //assigning something, it should copy now
		  test1 = 15;
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(test1.internal, test3.internal);
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal -> refcount, 1);
				  assertEquals(test3.internal -> refcount, 1);
			 #endif
		  #endif

		  test1 = test2;
		  #ifdef JSON_REF_COUNT
			 #ifdef JSON_UNIT_TEST
				  assertEquals(test1.internal, test2.internal);
				  assertEquals(test1.internal -> refcount, 2);
			 #endif
		  #else
			 #ifdef JSON_UNIT_TEST
				assertNotEquals(test1.internal, test2.internal);
			 #endif
			 assertEquals(test1, test2);
		  #endif

		  test1.set_name(JSON_TEXT("hello world"));
		  #ifdef JSON_UNIT_TEST
			 assertNotEquals(test1.internal, test2.internal);
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal -> refcount, 1);
				  assertEquals(test1.internal -> refcount, 1);
			 #endif
		  #endif

		  //test tree copying and partial tree copying
		  UnitTest::SetPrefix("TestRefCounting.cpp - Partial Copy");
		  test1 = JSONNode(JSON_NODE);
		  test1.push_back(JSONNode(JSON_NODE));
		  test1.push_back(JSONNode(JSON_TEXT(""), 5));
		  assertEquals(test1.size(), 2);
		  test2 = test1;
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal -> refcount, 2);
				  assertEquals(test1.internal, test2.internal);
			 #else
				  assertNotEquals(test1.internal, test2.internal);
			 #endif
		  #endif
		  #ifdef JSON_READ_PRIORITY
			 assertEquals(test1, libjson::parse(JSON_TEXT("{\"\":{},\"\":5}")));
			 assertEquals(test1, test1);
			 assertEquals(libjson::parse(JSON_TEXT("{\"\":{},\"\":5}")), libjson::parse(JSON_TEXT("{\"\":{},\"\":5}")));
			 TestSuite::testParsingItself(test1);
		  #endif

		  test2[1] = 15;
		  assertEquals(test1[1], 5);
		  assertEquals(test2[1], 15);
		  test1 = test2;
		  #ifdef JSON_UNIT_TEST
			 #ifdef JSON_REF_COUNT
				  assertEquals(test1.internal, test2.internal);
			 #else
				  assertNotEquals(test1.internal, test2.internal);
			 #endif
		  #endif
		  test1[0].push_back(JSONNode(JSON_TEXT(""), 1));
		  test1[0].push_back(JSONNode(JSON_TEXT(""), 2));
		  assertEquals(test1[0].size(), 2);
		  assertEquals(test2[0].size(), 0);
		  TestSuite::testParsingItself(test1);
		  TestSuite::testParsingItself(test2);
    #endif
}
