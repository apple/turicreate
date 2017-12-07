#ifndef TESTSUITE_H
#define TESTSUITE_H

#include "UnitTest.h"
#include "../JSONOptions.h"
#include "../libjson.h"
#include "../Source/JSONNode.h"

#include <iostream>
using namespace std;

/*
    This class tests libjson's internal working and it's
    C++ interface
*/

#ifdef JSON_UNICODE
    #define assertCStringSame(a, b) assertCStringEqualsW(a, b)
    #define assertCStringNotSame(a, b) assertCStringNotEqualsW(a, b)
#else
    #define assertCStringSame(a, b) assertCStringEquals(a, b)
    #define assertCStringNotSame(a, b) assertCStringNotEquals(a, b)
#endif


class TestSuite {
public:
    static void TestSelf(void);
    static void TestString(void);
    static void TestConverters(void);
#ifdef JSON_BINARY
    static void TestBase64(void);
#endif
    static void TestReferenceCounting(void);
    static void TestConstructors(void);
    static void TestAssigning(void);
    static void TestEquality(void);
    static void TestInequality(void);
    static void TestChildren(void);
    static void TestFunctions(void);
    static void TestIterators(void);
    static void TestInspectors(void);
    static void TestNamespace(void);
    static void TestValidator(void);
    static void TestStreams(void);
#ifdef JSON_WRITE_PRIORITY
    static void TestWriter(void);
#endif
#ifdef JSON_COMMENTS
    static void TestComments(void);
#endif
#ifdef JSON_MUTEX_CALLBACKS
    static void TestMutex(void);
    static void TestThreading(void);
#endif
	static void TestSharedString(void);
    static void TestFinal(void);
	
	
	
	
	
#ifdef JSON_LIBRARY
	static void testParsingItself(JSONNODE * x){
	   #if defined(JSON_WRITE_PRIORITY) && defined(JSON_READ_PRIORITY)
		  {
			 json_char * written = json_write(x);
			 JSONNODE * copy = json_parse(written);
			 assertTrue(json_equal(x, copy));
			 json_delete(copy);
			 json_free(written);
		  }
		  {
			 json_char * written = json_write_formatted(x);
			 JSONNODE * copy = json_parse(written);
			 assertTrue(json_equal(x, copy));
			 json_delete(copy);
			 json_free(written);
		  }
		  {
			 json_char * written = json_write_formatted(x);
			 json_char * written2 = json_write(x);
			 json_char * stripped = json_strip_white_space(written);
			 assertCStringSame(written2, stripped);
			 json_free(stripped);
			 json_free(written);
			 json_free(written2);
		  }
	  #endif
	  {
		 JSONNODE * copy = json_duplicate(x);
		 assertTrue(json_equal(x, copy));
		 json_delete(copy);
	  }
   }
#else
	static void testParsingItself(JSONNode & x){
		#if defined(JSON_WRITE_PRIORITY) && defined(JSON_READ_PRIORITY)
			assertEquals(libjson::parse(x.write()), x);
			assertEquals(libjson::parse(x.write_formatted()), x);
			assertEquals(libjson::strip_white_space(x.write_formatted()), x.write());
		#endif
		assertEquals(x, x.duplicate())
	}
#endif
};

#endif

