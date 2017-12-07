#include "TestSuite.h"

#ifdef JSON_STRING_HEADER
    #ifdef JSON_UNICODE
	   #include "UStringTest.h"
    #else
	   #include "StringTest.h"
    #endif
#else
    //otherwise it will use the regular STL strings and act as a control
    #include "../libjson.h"
#endif

static void assertConstEmpty(const json_string & s){
    assertEquals(s.length(), 0);
    assertTrue(s.empty());
    assertCStringSame(s.c_str(), JSON_TEXT(""));
    assertEquals(s, s);
    assertEquals(s, JSON_TEXT(""));
}

static void assertEmpty(json_string & s){
    assertEquals(s.length(), 0);
    assertTrue(s.empty());
    assertCStringSame(s.c_str(), JSON_TEXT(""));
    assertEquals(s, s);
    assertEquals(s, JSON_TEXT(""));
    assertConstEmpty(s);
}

static void assertSame(json_string & s, json_string & m){
    assertEquals(s, m);
    assertCStringSame(s.c_str(), m.c_str());
    assertEquals(s.length(), m.length());
    s.swap(m);
    assertEquals(s, m);
    assertCStringSame(s.c_str(), m.c_str());
    assertEquals(s.length(), m.length());
}

static void assertDifferent(json_string & s, json_string & m){
    assertNotEquals(s, m);
    assertCStringNotSame(s.c_str(), m.c_str());
}

void TestSuite::TestString(void){
    UnitTest::SetPrefix("TestString.cpp - Test String Class");
    {
	   json_string s;
	   assertEmpty(s);
    }

    {
	   json_string s;
	   assertEmpty(s);
	   json_string m(s);
	   assertEmpty(m);
	   assertEmpty(s);
	   assertSame(s, m);
    }

    {
	   json_string s(JSON_TEXT("hello"));
	   assertEquals(s.length(), 5);
	   assertFalse(s.empty());
	   assertCStringSame(s.c_str(), JSON_TEXT("hello"));
	   assertEquals(s, s);
	   assertEquals(s, JSON_TEXT("hello"));
	   s.clear();
	   assertEmpty(s);
    }

    {
	   json_string s(5, 'h');
	   assertEquals(s.length(), 5);
	   assertFalse(s.empty());
	   assertCStringSame(s.c_str(), JSON_TEXT("hhhhh"));
	   assertEquals(s, s);
	   assertEquals(s, JSON_TEXT("hhhhh"));
	   s.clear();
	   assertEmpty(s);
    }

    {
	   json_string s(5, 'h');
	   json_string m(s);
	   assertSame(s, m);
    }

    {
	   json_string s(5, 'h');
	   json_string m(s);
	   assertSame(s, m);
	   s.clear();
	   assertEmpty(s);
	   assertEquals(s.length(), 0);
	   assertDifferent(s, m);
    }


    {
	   json_string s(JSON_TEXT("hello"));
	   json_string m = s;
	   assertSame(s, m);
	   m = s.substr(1, 3);
	   assertEquals(m.length(), 3);
	   assertEquals(m, JSON_TEXT("ell"));
    }

    {
	   json_string s(JSON_TEXT("hello"));
	   json_string m = s;
	   assertSame(s, m);
	   m = s.substr(1);
	   assertEquals(m.length(), 4);
	   assertEquals(m, JSON_TEXT("ello"));
    }

    {
	   json_string s(JSON_TEXT("hello"));
	   s += JSON_TEXT(" world");
	   assertEquals(s.length(), 11);
	   assertEquals(s, JSON_TEXT("hello world"));
    }


    {
	   json_string s(JSON_TEXT("hello"));
	   json_string m = s + JSON_TEXT(" world ") + s;
	   assertEquals(m.length(), 17);
	   assertEquals(m, JSON_TEXT("hello world hello"));
    }

    {
	   json_string s(JSON_TEXT("hello"));
	   s += 'a';
	   s += 'a';
	   s += 'a';
	   s += 'a';
	   assertEquals(s.length(), 9);
	   assertEquals(s, JSON_TEXT("helloaaaa"));
    }

    {
	   json_string s(JSON_TEXT("hello world"));
	   size_t pos = s.find('w');
	   assertEquals(pos, 6);
    }

    {
	   json_string s(JSON_TEXT("hello world"));
	   size_t pos = s.find('z');
	   assertEquals(pos, json_string::npos);
    }

    {
	   json_string s(JSON_TEXT("hello world"));
	   size_t pos = s.find_first_not_of(JSON_TEXT("helo"));
	   assertEquals(pos, 5);
    }

    {
	   json_string s(JSON_TEXT("hello world"));
	   size_t pos = s.find_first_of(JSON_TEXT("ol"));
	   assertEquals(pos, 2);
    }

    {
	   json_string s(JSON_TEXT("hello world"));
	   s.erase(s.begin(), s.begin() + 3);
	   assertEquals(s, JSON_TEXT("lo world"));
    }
	
	{
	   json_string s(JSON_TEXT("hello world"), 5);
	   assertEquals(s, JSON_TEXT("hello"));
    }
	
	#ifndef JSON_LIBRARY	
        #ifndef JSON_STRING_HEADER
	        {
		        json_string s(JSON_TEXT("hello world"));
		        std::wstring wtest(L"hello world");
		        std::string stest("hello world");
		        assertEquals(libjson::to_std_string(s), stest);
		        assertEquals(stest, libjson::to_std_string(s));
		        assertEquals(libjson::to_std_wstring(s), wtest);
		        assertEquals(wtest, libjson::to_std_wstring(s));
		
		        assertEquals(s, libjson::to_json_string(stest));
		        assertEquals(libjson::to_json_string(stest), s);
		        assertEquals(s, libjson::to_json_string(wtest));
		        assertEquals(libjson::to_json_string(wtest), s);
	        }
        #endif
	#endif
}
