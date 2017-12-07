#include "TestSuite.h"

#ifdef JSON_STREAM
unsigned int counter = 0;
unsigned int errorCounter = 0;

void errorCallback(void *);
void errorCallback(void *){
	++errorCounter;
}

#ifdef JSON_LIBRARY
void Callback(JSONNODE * test, void *);
void Callback(JSONNODE * test, void *){
    ++counter;
    switch(counter){
	   case 1:
		  assertEquals(json_type(test), JSON_NODE);
		  assertTrue(json_empty(test));
		  break;
	   case 2:
		  assertEquals(json_type(test), JSON_ARRAY);
		  assertTrue(json_empty(test));
		  break;
	   case 3:{
		  assertEquals(json_type(test), JSON_NODE);
		  assertEquals(json_size(test), 1);
		  json_char * temp = json_name(json_at(test, 0));
		  assertCStringSame(temp, JSON_TEXT("hello"));
		  json_free(temp);
		  assertEquals(json_as_int(json_at(test, 0)), 1);
		  break;}
	   case 4:
		  assertEquals(json_type(test), JSON_ARRAY);
		  assertEquals(json_size(test), 3);
		  break;
	   case 5:{
		  assertEquals(json_type(test), JSON_NODE);
		  assertEquals(json_size(test), 1);
		  json_char * temp = json_name(json_at(test, 0));
		  assertCStringSame(temp, JSON_TEXT("hi"));
		  json_free(temp);
		  assertEquals(json_size(json_at(test, 0)), 1);
		  assertEquals(json_type(json_at(json_at(test, 0),0)), JSON_NUMBER);
		  temp = json_name(json_at(json_at(test, 0),0));
		  assertCStringSame(temp, JSON_TEXT("one"));
		  json_free(temp);
		  assertEquals(json_as_int(json_at(json_at(test, 0),0)), 1);
		  break;}
    }
}
#else
void Callback(JSONNode & test, void * ide);
void Callback(JSONNode & test, void * ide){
	assertEquals(ide, (void*)0xDEADBEEF);
    ++counter;
    switch(counter){
	   case 1:
		  assertEquals(test.type(), JSON_NODE);
		  assertTrue(test.empty());
		  break;
	   case 2:
		  assertEquals(test.type(), JSON_ARRAY);
		  assertTrue(test.empty());
		  break;
	   case 3:
		  assertEquals(test.type(), JSON_NODE);
		  assertEquals(test.size(), 1);
		  assertEquals(test[0].name(), JSON_TEXT("hello"));
		  assertEquals(test[0].as_int(), 1);
		  break;
	   case 4:
		  assertEquals(test.type(), JSON_ARRAY);
		  assertEquals(test.size(), 3);
		  break;
	   case 5:
		  assertEquals(test.type(), JSON_NODE);
		  assertEquals(test.size(), 1);
		  assertEquals(test[0].name(), JSON_TEXT("hi"));
		  assertEquals(test[0].size(), 1)
		  assertEquals(test[0][0].type(), JSON_NUMBER);
		  assertEquals(test[0][0].name(), JSON_TEXT("one"));
		  assertEquals(test[0][0].as_int(), 1);
		  break;
    }
}
#endif  //library
#endif  //stream


void TestSuite::TestStreams(void){
#ifdef JSON_STREAM
    UnitTest::SetPrefix("TestStreams.cpp - Streams");
    counter = 0;
	errorCounter = 0;

    #ifdef JSON_LIBRARY
	   JSONSTREAM * test = json_new_stream(Callback, errorCallback, (void*)0xDEADBEEF);
	   json_stream_push(test, JSON_TEXT("{}[]"));
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   json_stream_push(test, JSON_TEXT("{\"hel"));
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   json_stream_push(test, JSON_TEXT("lo\" : 1"));
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   json_stream_push(test, JSON_TEXT("}["));
	   assertEquals(3, counter);
	   assertEquals(0, errorCounter);
	   json_stream_push(test, JSON_TEXT("1,2,3]{\"hi\" : { \"one\" : 1}"));
	   assertEquals(4, counter);
	   assertEquals(0, errorCounter);
	   json_stream_push(test, JSON_TEXT("}"));
	   assertEquals(5, counter);
	   assertEquals(0, errorCounter);
	
		#ifdef JSON_SAFE
			json_stream_push(test, JSON_TEXT("{\"hello\":12keaueuataueaouhe"));
			assertEquals(1, errorCounter);
		#endif
	   json_delete_stream(test);
    #else
	   JSONStream test(Callback, errorCallback, (void*)0xDEADBEEF);
	   test << JSON_TEXT("{}[]");
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   test << JSON_TEXT("{\"hel");
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   test << JSON_TEXT("lo\" : 1");
	   assertEquals(2, counter);
	   assertEquals(0, errorCounter);
	   test << JSON_TEXT("}[");
	   assertEquals(3, counter);
	   assertEquals(0, errorCounter);
	   test << JSON_TEXT("1,2,3]{\"hi\" : { \"one\" : 1}");
	   assertEquals(4, counter);
	   assertEquals(0, errorCounter);
	   test << JSON_TEXT("}");
	   assertEquals(5, counter);
	   assertEquals(0, errorCounter);
	
		#ifdef JSON_SAFE
			test << JSON_TEXT("{\"hello\":12keaueuataueaouhe");
			assertEquals(1, errorCounter);
		#endif
	
		#ifdef JSON_SECURITY_MAX_STREAM_OBJECTS
			test.reset();
			unsigned int currentCount = errorCounter;
			json_string safe;
			for(int i = 0; i < JSON_SECURITY_MAX_STREAM_OBJECTS; ++i){
				safe += JSON_TEXT("{}");
			}
			test << safe;
			assertEquals(133, counter);
			assertEquals(currentCount, errorCounter);
	
			test.reset();
			json_string unsafe;
			for(int i = 0; i <= JSON_SECURITY_MAX_STREAM_OBJECTS + 1; ++i){
				unsafe += JSON_TEXT("{}");
			}
			test << unsafe;
			assertEquals(261, counter);
			assertEquals(currentCount + 1, errorCounter);
		#endif
    #endif
#endif
}
