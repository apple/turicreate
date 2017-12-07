#include "TestSuite.h"
#include "../Source/NumberToString.h"
#include "../Source/JSONNode.h"

void TestSuite::TestConverters(void){
    UnitTest::SetPrefix("TestConverters.cpp - Converters");

    assertEquals(sizeof(char), 1);
    assertEquals(NumberToString::_itoa<char>((char)127), JSON_TEXT("127"));
    assertEquals(NumberToString::_itoa<char>((char)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<char>((char)0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<char>((char)-15), JSON_TEXT("-15"));
    assertEquals(NumberToString::_itoa<char>((char)-127), JSON_TEXT("-127"));

    assertEquals(sizeof(short), 2);
    assertEquals(NumberToString::_itoa<short>((short)32767), JSON_TEXT("32767"));
    assertEquals(NumberToString::_itoa<short>((short)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<short>((short)0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<short>((short)-15), JSON_TEXT("-15"));
    assertEquals(NumberToString::_itoa<short>((short)-32767), JSON_TEXT("-32767"));

    assertEquals(sizeof(int), 4);
    assertEquals(NumberToString::_itoa<int>(2147483647), JSON_TEXT("2147483647"));
    assertEquals(NumberToString::_itoa<int>(15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<int>(0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<int>(-15), JSON_TEXT("-15"));
    assertEquals(NumberToString::_itoa<int>(-2147483647), JSON_TEXT("-2147483647"));

    #ifdef TEST_LONG_EXTREMES
	   assertEquals(NumberToString::_itoa<long>(9223372036854775807L), JSON_TEXT("9223372036854775807"));
           assertEquals(NumberToString::_itoa<long>(-9223372036854775807L), JSON_TEXT("-9223372036854775807"));
	   #ifndef JSON_LIBRARY
		  assertEquals(NumberToString::_uitoa<unsigned long>(18446744073709551615UL), JSON_TEXT("18446744073709551615"));
	   #endif
    #endif
    assertEquals(NumberToString::_itoa<long>(15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<long>(0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<long>(-15), JSON_TEXT("-15"));

    #ifndef JSON_LIBRARY
	   assertEquals(NumberToString::_uitoa<unsigned char>(255), JSON_TEXT("255"));
	   assertEquals(NumberToString::_uitoa<unsigned char>(15), JSON_TEXT("15"));
	   assertEquals(NumberToString::_uitoa<unsigned char>(0), JSON_TEXT("0"));

	   assertEquals(NumberToString::_uitoa<unsigned short>(65535), JSON_TEXT("65535"));
	   assertEquals(NumberToString::_uitoa<unsigned short>(15), JSON_TEXT("15"));
	   assertEquals(NumberToString::_uitoa<unsigned short>(0), JSON_TEXT("0"));

	   assertEquals(NumberToString::_uitoa<unsigned int>(4294967295u), JSON_TEXT("4294967295"));
	   assertEquals(NumberToString::_uitoa<unsigned int>(15), JSON_TEXT("15"));
	   assertEquals(NumberToString::_uitoa<unsigned int>(0), JSON_TEXT("0"));

	   assertEquals(NumberToString::_uitoa<unsigned long>(15), JSON_TEXT("15"));
	   assertEquals(NumberToString::_uitoa<unsigned long>(0), JSON_TEXT("0"));
    #endif

    assertEquals(NumberToString::_ftoa((json_number)1.0), JSON_TEXT("1"));
    assertEquals(NumberToString::_ftoa((json_number)1.002), JSON_TEXT("1.002"));
    assertEquals(NumberToString::_ftoa((json_number)10.0), JSON_TEXT("10"));
    assertEquals(NumberToString::_ftoa((json_number)-1.0), JSON_TEXT("-1"));
    assertEquals(NumberToString::_ftoa((json_number)-1.002), JSON_TEXT("-1.002"));
    assertEquals(NumberToString::_ftoa((json_number)-10.0), JSON_TEXT("-10"));
    assertEquals(NumberToString::_ftoa((json_number)0.0), JSON_TEXT("0"));

    assertTrue(_floatsAreEqual(1.1, 1.1));
    assertTrue(_floatsAreEqual(1.000000001, 1.0));
    assertTrue(_floatsAreEqual(1.0, 1.000000001));
    assertFalse(_floatsAreEqual(1.0, 1.0001));
    assertFalse(_floatsAreEqual(1.0001, 1.0));

    #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
	   #ifdef JSON_UNIT_TEST
		    UnitTest::SetPrefix("TestConverters.cpp - Checking case-insensitive");
		    assertTrue(internalJSONNode::AreEqualNoCase(JSON_TEXT("hello"), JSON_TEXT("HeLLo")));
		    assertTrue(internalJSONNode::AreEqualNoCase(JSON_TEXT("hell5o"), JSON_TEXT("HELL5O")));
		    assertTrue(internalJSONNode::AreEqualNoCase(JSON_TEXT("HeLLo"), JSON_TEXT("hello")));
		    assertTrue(internalJSONNode::AreEqualNoCase(JSON_TEXT("HELL5O"), JSON_TEXT("hell5o")));

		    assertFalse(internalJSONNode::AreEqualNoCase(JSON_TEXT("hello"), JSON_TEXT("Hello ")));
		    assertFalse(internalJSONNode::AreEqualNoCase(JSON_TEXT("hello"), JSON_TEXT("hi")));
		    assertFalse(internalJSONNode::AreEqualNoCase(JSON_TEXT("hello"), JSON_TEXT("55555")));
		    assertFalse(internalJSONNode::AreEqualNoCase(JSON_TEXT("hello"), JSON_TEXT("jonny")));
	   #endif
    #endif

    #ifdef JSON_SAFE
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e+2")));

	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e-2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0e+2")));
	   assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e+2")));


	   #ifdef JSON_STRICT
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("0xABCD")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("0124")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.0")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.0e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.0e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e-2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e+2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1e+2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.e+2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e+2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.0e+2")));
		  assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e+2")));
	   #else
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("0xABCD")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("0124")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.0")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.0e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.0e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e-2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e+2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1e+2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.e+2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e+2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.0e+2")));
		  assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e+2")));
	   #endif
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("0xABCDv")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("00124")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("09124")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("0no")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("no")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("n1234")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("12no")));
	   assertFalse(NumberToString::isNumeric(JSON_TEXT("0en5")));
    #endif

}

