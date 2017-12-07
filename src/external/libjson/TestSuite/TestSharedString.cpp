
#include "TestSuite.h"
#include "../Source/JSONSharedString.h"

void TestSuite::TestSharedString(void){
	UnitTest::SetPrefix("TestSharedString.cpp - Seeing how much regular strings share");
	json_string sharey = JSON_TEXT("Hello world");
	json_string sharey2 = sharey;
	if (sharey2.data() == sharey.data()) echo("Assignment shares data");
	sharey2 = json_string(sharey);
	if (sharey2.data() == sharey.data()) echo("Copy ctor shares data");
	sharey2 = json_string(sharey.begin(), sharey.end());
	if (sharey2.data() == sharey.data()) echo("Copy with iterators shares data");
	sharey2 = sharey.substr(0);
	if (sharey2.data() == sharey.data()) echo("substr shares data");

	json_string value = JSON_TEXT("Hello, I am a string with lots of words");
	json_shared_string shared = json_shared_string(value);

	UnitTest::SetPrefix("TestSharedString.cpp - Whole String");
	//make it out of a string, make sure they are equal
	assertEquals(value.length(), shared.length());
	assertEquals(value, json_string(shared.std_begin(), shared.std_end()));
	#ifdef JSON_UNIT_TEST
		assertEquals(1, shared._str -> refCount);
	#endif
	
	UnitTest::SetPrefix("TestSharedString.cpp - Substring");
	//take a substring out of it, make sure its using the same reference
	json_shared_string hello = json_shared_string(shared, 0, 5);
	json_string shello = value.substr(0, 5);
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, hello._str);
		assertEquals(2, shared._str -> refCount);
	#endif	
	assertEquals(shello, json_string(hello.std_begin(), hello.std_end()));
		
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, hello._str);
		assertEquals(2, shared._str -> refCount);
	#endif	

	UnitTest::SetPrefix("TestSharedString.cpp - Substring to String");
	//make sure converting it to a string actually does the convert
	assertEquals(json_string(JSON_TEXT("Hello")), hello.toString());
	#ifdef JSON_UNIT_TEST
		assertNotEquals(shared._str, hello._str);
		assertEquals(1, shared._str -> refCount);
		assertEquals(1, hello._str -> refCount);
	#endif
	
	UnitTest::SetPrefix("TestSharedString.cpp - Substring of substring offset zero");
	json_shared_string rest = json_shared_string(shared, 7);
	json_string srest = value.substr(7);
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, rest._str);
		assertEquals(7,rest.offset);
		assertEquals(2, shared._str -> refCount);
	#endif	
	assertEquals(srest, json_string(rest.std_begin(), rest.std_end()));
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, rest._str);
		assertEquals(2, shared._str -> refCount);
	#endif	
	
	json_shared_string I_am_a_string = json_shared_string(rest, 0, 13);
	json_string sI_am_a_string = srest.substr(0, 13);
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, I_am_a_string._str);
		assertEquals(7,rest.offset);
		assertEquals(3, shared._str -> refCount);
	#endif	
	assertEquals(sI_am_a_string, json_string(I_am_a_string.std_begin(), I_am_a_string.std_end()));
	assertEquals(srest, json_string(rest.std_begin(), rest.std_end()));
	#ifdef JSON_UNIT_TEST
		assertEquals(shared._str, I_am_a_string._str);
		assertEquals(3, shared._str -> refCount);
	#endif	
	
	
	UnitTest::SetPrefix("TestSharedString.cpp - Finding Ref 1");
	assertEquals(0, hello.find(JSON_TEXT('H')));
	assertEquals(shello.find(JSON_TEXT('H')), hello.find(JSON_TEXT('H')));
	assertEquals(4, hello.find(JSON_TEXT('o')));
	assertEquals(shello.find(JSON_TEXT('o')), hello.find(JSON_TEXT('o')));
	assertEquals(json_string::npos, hello.find(JSON_TEXT('z')));
	assertEquals(shello.find(JSON_TEXT('z')), hello.find(JSON_TEXT('z')));
	
	UnitTest::SetPrefix("TestSharedString.cpp - Finding Shared");
	assertEquals(0, I_am_a_string.find(JSON_TEXT('I')));
	assertEquals(sI_am_a_string.find(JSON_TEXT('I')), I_am_a_string.find(JSON_TEXT('I')));
	assertEquals(7, I_am_a_string.find(JSON_TEXT('s')));
	assertEquals(sI_am_a_string.find(JSON_TEXT('s')), I_am_a_string.find(JSON_TEXT('s')));
	assertEquals(json_string::npos, I_am_a_string.find(JSON_TEXT('z')));
	assertEquals(sI_am_a_string.find(JSON_TEXT('z')), I_am_a_string.find(JSON_TEXT('z')));
	//still sharing memory with the parent string, which contains a w
	assertEquals(json_string::npos, I_am_a_string.find(JSON_TEXT('w')));
	assertEquals(sI_am_a_string.find(JSON_TEXT('w')), I_am_a_string.find(JSON_TEXT('w')));
	
	UnitTest::SetPrefix("TestSharedString.cpp - Iterator substrings");
	json_string blah = JSON_TEXT("hello world");
	json_shared_string blahs(blah);
	#ifdef JSON_UNIT_TEST
		assertEquals(blahs._str -> refCount, 1);
	#endif
	json_string sub = json_string(blah.begin(), blah.end());
	json_shared_string subs = json_shared_string(blahs.begin(), blahs.end());
	#ifdef JSON_UNIT_TEST
		assertEquals(blahs._str, subs._str);
		assertEquals(blahs._str -> refCount, 2);
	#endif
	assertEquals(blah, blahs.toString());
	assertEquals(sub, subs.toString());
	assertEquals(sub.length(), subs.length());
	sub = json_string(blah.begin(), blah.begin() + 5);
	subs = json_shared_string(blahs.begin(), blahs.begin() + 5);
	#ifdef JSON_UNIT_TEST
		assertEquals(blahs._str, subs._str);
		assertEquals(blahs._str -> refCount, 2);
	#endif
	assertEquals(blah, blahs.toString());
	assertEquals(sub, subs.toString());
	assertEquals(sub.length(), subs.length());
}

