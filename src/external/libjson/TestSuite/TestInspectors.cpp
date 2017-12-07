#include "TestSuite.h"
#include "../Source/JSONNode.h"
#include <cstdlib>

void TestSuite::TestInspectors(void){
    UnitTest::SetPrefix("TestInspectors.cpp - Inspectors");
    #ifdef JSON_LIBRARY
		  JSONNODE * test = json_new(JSON_NULL);
		  assertEquals(json_type(test), JSON_NULL);
		  json_char * res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT(""));
		  json_free(res);
		  assertEquals_Primitive(json_as_int(test), 0);
		  assertEquals_Primitive(json_as_float(test), 0.0f);
		  assertEquals(json_as_bool(test), false);

		  json_set_f(test, 15.5f);
		  assertEquals(json_type(test), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("15.5"));
		  json_free(res);
		#endif
		  assertEquals_Primitive(json_as_int(test), 15);
		  assertEquals_Primitive(json_as_float(test), 15.5f);
		  #ifdef JSON_CASTABLE
				assertEquals(json_as_bool(test), true);
		  #endif

		  json_set_f(test, 0.0f);
		  assertEquals(json_type(test), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  res = json_as_string(test);
		  assertCStringSame(res, JSON_TEXT("0"));
		  json_free(res);
		#endif
		  assertEquals_Primitive(json_as_int(test), 0);
		  assertEquals_Primitive(json_as_float(test), 0.0f);
		  #ifdef JSON_CASTABLE
			assertEquals(json_as_bool(test), false);
		  #endif
	
		  json_set_b(test, true);
		  assertEquals(json_type(test), JSON_BOOL);
		  #ifdef JSON_CASTABLE
			res = json_as_string(test);
			assertCStringSame(res, JSON_TEXT("true"));
			json_free(res);
			assertEquals_Primitive(json_as_int(test), 1);
			assertEquals_Primitive(json_as_float(test), 1.0f);
		  #endif
		  assertEquals(json_as_bool(test), true);

		  json_set_b(test, false);
		  assertEquals(json_type(test), JSON_BOOL);
		  #ifdef JSON_CASTABLE
			res = json_as_string(test);
			assertCStringSame(res, JSON_TEXT("false"));
			json_free(res);
			assertEquals_Primitive(json_as_int(test), 0);
			assertEquals_Primitive(json_as_float(test), 0.0f);
		  #endif
		  assertEquals(json_as_bool(test), false);
		  #ifdef JSON_CASTABLE
				  json_cast(test, JSON_NODE);
				  assertEquals(json_type(test), JSON_NODE);
				  assertEquals(json_size(test), 0);
				  json_push_back(test, json_new_a(JSON_TEXT("hi"), JSON_TEXT("world")));
				  json_push_back(test, json_new_a(JSON_TEXT("hello"), JSON_TEXT("mars")));
				  json_push_back(test, json_new_a(JSON_TEXT("salut"), JSON_TEXT("france")));
				  assertEquals(json_size(test), 3);
				  TestSuite::testParsingItself(test);

				  JSONNODE * casted = json_as_array(test);
				  #ifdef JSON_UNIT_TEST
					 assertNotEquals(((JSONNode*)casted) -> internal, ((JSONNode*)test) -> internal);
				  #endif
				  assertEquals(json_type(casted), JSON_ARRAY);
				  assertEquals(json_type(test), JSON_NODE);
				  assertEquals(json_size(test), 3);
				  assertEquals(json_size(casted), 3);
				  TestSuite::testParsingItself(casted);
		   #endif
				  UnitTest::SetPrefix("TestInspectors.cpp - Location");

		   #ifdef JSON_CASTABLE
				  #define CheckAt(parent, locale, text)\
					 if(JSONNODE * temp = json_at(parent, locale)){\
						json_char * _res = json_as_string(temp);\
						assertCStringSame(_res, text);\
						json_free(_res);\
					 } else {\
						FAIL(std::string("CheckAt: ") + #parent + "[" + #locale + "]");\
					 }

				  #define CheckNameAt(parent, locale, text)\
					 if(JSONNODE * temp = json_at(parent, locale)){\
						json_char * _res = json_name(temp);\
						assertCStringSame(_res, text);\
						json_free(_res);\
					 } else {\
						FAIL(std::string("CheckNameAt: ") + #parent + "[" + #locale + "]");\
					 }

		          CheckAt(casted, 0, JSON_TEXT("world"));
				  CheckAt(casted, 1, JSON_TEXT("mars"));
				  CheckAt(casted, 2, JSON_TEXT("france"));
				  CheckNameAt(casted, 0, JSON_TEXT(""));
				  CheckNameAt(casted, 1, JSON_TEXT(""));
				  CheckNameAt(casted, 2, JSON_TEXT(""));
	
				  CheckAt(test, 0, JSON_TEXT("world"));
				  CheckAt(test, 1, JSON_TEXT("mars"));
				  CheckAt(test, 2, JSON_TEXT("france"));
				  CheckNameAt(test, 0, JSON_TEXT("hi"));
				  CheckNameAt(test, 1, JSON_TEXT("hello"));
				  CheckNameAt(test, 2, JSON_TEXT("salut"));
		  

				  #define CheckGet(parent, locale, text)\
					 if(JSONNODE * temp = json_get(parent, locale)){\
						json_char * _res = json_as_string(temp);\
						assertCStringSame(_res, text);\
						json_free(_res);\
					 } else {\
						FAIL(std::string("CheckGet: ") + #parent + "[" + #locale + "]");\
					 }

				  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
					 #define CheckGetNoCase(parent, locale, text)\
						if(JSONNODE * temp = json_get_nocase(parent, locale)){\
							json_char * _res = json_as_string(temp);\
							assertCStringSame(_res, text);\
							json_free(_res);\
						} else {\
							FAIL(std::string("CheckGetNoCase: ") + #parent + "[" + #locale + "]");\
						}
				  #else
					 #define CheckGetNoCase(parent, locale, text)
				  #endif

				  CheckGet(test, JSON_TEXT("hi"), JSON_TEXT("world"));
				  CheckGetNoCase(test, JSON_TEXT("HI"), JSON_TEXT("world"));
				  CheckGet(test, JSON_TEXT("hello"), JSON_TEXT("mars"));
				  CheckGetNoCase(test, JSON_TEXT("HELLO"), JSON_TEXT("mars"));
				  CheckGet(test, JSON_TEXT("salut"), JSON_TEXT("france"));
				  CheckGetNoCase(test, JSON_TEXT("SALUT"), JSON_TEXT("france"));
	
				assertNull(json_get(test, JSON_TEXT("meh")));
				#ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
					assertNull(json_get_nocase(test, JSON_TEXT("meh")));
				#endif
			#endif


		  #ifdef JSON_ITERATORS
			#ifdef JSON_CASTABLE
			 UnitTest::SetPrefix("TestInspectors.cpp - Iterators");
			 for(JSONNODE_ITERATOR it = json_begin(casted), end = json_end(casted); it != end; ++it){
				json_char * _res = json_name(*it);
				assertCStringSame(_res, JSON_TEXT(""));
				json_free(_res);
			 }
			#endif
		  #endif

		  #ifdef JSON_BINARY
			 UnitTest::SetPrefix("TestInspectors.cpp - Binary");
			 json_set_binary(test, (const unsigned char *)"Hello World", 11);
			 assertEquals(json_type(test), JSON_STRING);
			 json_char * _res = json_as_string(test);
			 assertCStringSame(_res, JSON_TEXT("SGVsbG8gV29ybGQ="));
			 json_free(_res);

			 unsigned long i;
			 if(char * bin = (char*)json_as_binary(test, &i)){
				assertEquals(i, 11);
				char * terminated = (char*)std::memcpy(std::malloc(i + 1), bin, i);
				terminated[i] = '\0';
				assertCStringEquals(terminated, "Hello World");
				json_free(bin);
				std::free(terminated);
			 } else {
				FAIL("as_binary failed");
			 }

			 json_set_a(test, JSON_TEXT("Hello World"));
			 assertEquals(json_type(test), JSON_STRING);
			 _res = json_as_string(test);
			 assertCStringSame(_res, JSON_TEXT("Hello World"));
			 json_free(_res);

			 #ifdef JSON_SAFE
				assertEquals(json_as_binary(test, &i), 0);
				assertEquals(i, 0);
			 #endif
		  #endif


		  json_delete(test);
		  #ifdef JSON_CASTABLE
			json_delete(casted);
		  #endif
    #else
		  JSONNode test = JSONNode(JSON_NULL);
		  #ifdef JSON_CASTABLE
			 assertEquals(test.as_string(), JSON_TEXT(""));
			 assertEquals(test.as_int(), 0);
			 assertEquals(test.as_float(), 0.0f);
			 assertEquals(test.as_bool(), false);
		  #endif

		  test = 15.5f;
		  assertEquals(test.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("15.5"));
		#endif
		  assertEquals(test.as_int(), 15);
		  assertEquals(test.as_float(), 15.5f);
		  #ifdef JSON_CASTABLE
			 assertEquals(test.as_bool(), true);
		  #endif

		  test = 0.0f;
		  assertEquals(test.type(), JSON_NUMBER);
		#ifdef JSON_CASTABLE
		  assertEquals(test.as_string(), JSON_TEXT("0"));
		#endif
		  assertEquals(test.as_int(), 0);
		  assertEquals(test.as_float(), 0.0f);
		  #ifdef JSON_CASTABLE
			 assertEquals(test.as_bool(), false);
		  #endif

		  test = true;
		  assertEquals(test.type(), JSON_BOOL);
		  #ifdef JSON_CASTABLE
			 assertEquals(test.as_string(), JSON_TEXT("true"));
			 assertEquals(test.as_int(), 1);
			 assertEquals(test.as_float(), 1.0f);
		  #endif
		  assertEquals(test.as_bool(), true);

		  test = false;
		  assertEquals(test.type(), JSON_BOOL);
		  #ifdef JSON_CASTABLE
			 assertEquals(test.as_string(), JSON_TEXT("false"));
			 assertEquals(test.as_int(), 0);
			 assertEquals(test.as_float(), 0.0f);
		  #endif
		  assertEquals(test.as_bool(), false);

		  #ifdef JSON_CASTABLE
			 test.cast(JSON_NODE);
		  #else
			 test = JSONNode(JSON_NODE);
		  #endif
		  assertEquals(test.type(), JSON_NODE);
		  assertEquals(test.size(), 0);
		  test.push_back(JSONNode(JSON_TEXT("hi"), JSON_TEXT("world")));
		  test.push_back(JSONNode(JSON_TEXT("hello"), JSON_TEXT("mars")));
		  test.push_back(JSONNode(JSON_TEXT("salut"), JSON_TEXT("france")));
		  assertEquals(test.size(), 3);
		  TestSuite::testParsingItself(test);

		  #ifdef JSON_CASTABLE
			 JSONNode casted = test.as_array();
			 #ifdef JSON_UNIT_TEST
				assertNotEquals(casted.internal, test.internal);
			 #endif
			 assertEquals(casted.type(), JSON_ARRAY);
			 assertEquals(test.type(), JSON_NODE);
			 assertEquals(test.size(), 3);
			 assertEquals(casted.size(), 3);
			 TestSuite::testParsingItself(casted);
		  #endif

		  UnitTest::SetPrefix("TestInspectors.cpp - Location");

		  try {
			 #ifdef JSON_CASTABLE
				assertEquals(casted.at(0), JSON_TEXT("world"));
				assertEquals(casted.at(1), JSON_TEXT("mars"));
				assertEquals(casted.at(2), JSON_TEXT("france"));
				assertEquals(casted.at(0).name(), JSON_TEXT(""));
				assertEquals(casted.at(1).name(), JSON_TEXT(""));
				assertEquals(casted.at(2).name(), JSON_TEXT(""));
			 #endif
			 assertEquals(test.at(0), JSON_TEXT("world"));
			 assertEquals(test.at(1), JSON_TEXT("mars"));
			 assertEquals(test.at(2), JSON_TEXT("france"));
			 assertEquals(test.at(0).name(), JSON_TEXT("hi"));
			 assertEquals(test.at(1).name(), JSON_TEXT("hello"));
			 assertEquals(test.at(2).name(), JSON_TEXT("salut"));
		  } catch (std::out_of_range){
			 FAIL("exception caught");
		  }

		  try {
			 assertEquals(test.at(JSON_TEXT("hi")), JSON_TEXT("world"));
			 assertEquals(test.at(JSON_TEXT("hello")), JSON_TEXT("mars"));
			 assertEquals(test.at(JSON_TEXT("salut")), JSON_TEXT("france"));
			 #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
				assertEquals(test.at_nocase(JSON_TEXT("SALUT")), JSON_TEXT("france"));
				assertEquals(test.at_nocase(JSON_TEXT("HELLO")), JSON_TEXT("mars"));
				assertEquals(test.at_nocase(JSON_TEXT("HI")), JSON_TEXT("world"));
			 #endif
		  } catch (std::out_of_range){
			 FAIL("exception caught");
		  }

		  assertException(test.at(JSON_TEXT("meh")), std::out_of_range);
		  #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
			 assertException(test.at_nocase(JSON_TEXT("meh")), std::out_of_range);
		  #endif

		  assertEquals(test[JSON_TEXT("hi")], json_string(JSON_TEXT("world")));
		  assertEquals(test[JSON_TEXT("hello")], json_string(JSON_TEXT("mars")));
		  assertEquals(test[JSON_TEXT("salut")], json_string(JSON_TEXT("france")));
		  assertEquals(test[0], JSON_TEXT("world"));
		  assertEquals(test[1], JSON_TEXT("mars"));
		  assertEquals(test[2], JSON_TEXT("france"));

		  #ifdef JSON_ITERATORS
			#ifdef JSON_CASTABLE
			 UnitTest::SetPrefix("TestInspectors.cpp - Iterators");
			 for(JSONNode::iterator it = casted.begin(), end = casted.end(); it != end; ++it){
				assertEquals((*it).name(), JSON_TEXT(""));
			 }
			#endif
		  #endif

		  #ifdef JSON_BINARY
			 UnitTest::SetPrefix("TestInspectors.cpp - Binary");
			 test.set_binary((const unsigned char *)"Hello World", 11);
			 assertEquals(test.type(), JSON_STRING);
			 assertEquals(test.as_string(), JSON_TEXT("SGVsbG8gV29ybGQ="));
			 assertEquals(test.as_binary(), "Hello World");
			 assertEquals(test.as_binary().size(), 11);

			 test = JSON_TEXT("Hello World");
			 assertEquals(test.type(), JSON_STRING);
			 assertEquals(test.as_string(), JSON_TEXT("Hello World"));
			 #ifdef JSON_SAFE
				assertEquals(test.as_binary(), "");
			 #endif
		  #endif
		  
         #ifdef JSON_READ_PRIORITY
			//This is a regression test for a bug in at()
			json_string buffer(JSON_TEXT("{ \"myValue1\" : \"foo\", \"myValue2\" : \"bar\"}"));
			JSONNode current = libjson::parse(buffer);
			try {
				JSONNode & value1 = current[JSON_TEXT("myValue1")];
				assertEquals(value1.as_string(), JSON_TEXT("foo"));
				JSONNode & value2 = current[JSON_TEXT("myValue2")];
				assertEquals(value2.as_string(), JSON_TEXT("bar"));
			} catch (...){
				assertTrue(false);
			}
        #endif
    #endif
}
