#include "_itoa.h"
#include "../../Source/NumberToString.h"


/**
 *	Test converting a char value into a string
 */
void testNumberToString__itoa::testChar(void){
	//GetScopeCoverage(_itoa, true);
	assertEquals(sizeof(char), 1);
    assertEquals(NumberToString::_itoa<char>((char)127), JSON_TEXT("127"));
    assertEquals(NumberToString::_itoa<char>((char)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<char>((char)0), JSON_TEXT("0"));
	assertEquals(NumberToString::_itoa<char>((char)-0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<char>((char)-15), JSON_TEXT("-15"));
    assertEquals(NumberToString::_itoa<char>((char)-127), JSON_TEXT("-127"));	
	//AssertScopeCoverage(_itoa);
}


/**
 *	Test converting a short value into a string
 */
void testNumberToString__itoa::testShort(void){
	//GetScopeCoverage(_itoa, true);
	assertEquals(sizeof(short), 2);
    assertEquals(NumberToString::_itoa<short>((short)32767), JSON_TEXT("32767"));
	assertEquals(NumberToString::_itoa<short>((short)127), JSON_TEXT("127"));
    assertEquals(NumberToString::_itoa<short>((short)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<short>((short)0), JSON_TEXT("0"));
	assertEquals(NumberToString::_itoa<short>((short)-0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<short>((short)-15), JSON_TEXT("-15"));
	assertEquals(NumberToString::_itoa<short>((short)-127), JSON_TEXT("-127"));
    assertEquals(NumberToString::_itoa<short>((short)-32767), JSON_TEXT("-32767"));	
	//AssertScopeCoverage(_itoa);
}


/**
 *	Test converting an int value into a string
 */
void testNumberToString__itoa::testInt(void){
	//GetScopeCoverage(_itoa, true);
	assertEquals(sizeof(int), 4);
    assertEquals(NumberToString::_itoa<int>((int)2147483647), JSON_TEXT("2147483647"));
	assertEquals(NumberToString::_itoa<int>((int)32767), JSON_TEXT("32767"));
	assertEquals(NumberToString::_itoa<int>((int)127), JSON_TEXT("127"));
    assertEquals(NumberToString::_itoa<int>((int)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<int>((int)0), JSON_TEXT("0"));
	assertEquals(NumberToString::_itoa<int>((int)-0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<int>((int)-15), JSON_TEXT("-15"));
	assertEquals(NumberToString::_itoa<int>((int)-127), JSON_TEXT("-127"));
	assertEquals(NumberToString::_itoa<int>((int)-32767), JSON_TEXT("-32767"));
    assertEquals(NumberToString::_itoa<int>((int)-2147483647), JSON_TEXT("-2147483647"));
	//AssertScopeCoverage(_itoa);
}


/**
 *	Test converting a long value into a string
 */
void testNumberToString__itoa::testLong(void){
	//GetScopeCoverage(_itoa, true);
	#ifdef TEST_LONG_EXTREMES
		if (sizeof(long) >= 8){
			assertEquals(NumberToString::_itoa<long>((long)9223372036854775807L), JSON_TEXT("9223372036854775807"));
			assertEquals(NumberToString::_itoa<long>((long)-9223372036854775807L), JSON_TEXT("-9223372036854775807"));
		}	
	#endif
	assertEquals(NumberToString::_itoa<long>((long)2147483647), JSON_TEXT("2147483647"));
	assertEquals(NumberToString::_itoa<long>((long)32767), JSON_TEXT("32767"));
	assertEquals(NumberToString::_itoa<long>((long)127), JSON_TEXT("127"));
    assertEquals(NumberToString::_itoa<long>((long)15), JSON_TEXT("15"));
    assertEquals(NumberToString::_itoa<long>((long)0), JSON_TEXT("0"));
	assertEquals(NumberToString::_itoa<long>((long)-0), JSON_TEXT("0"));
    assertEquals(NumberToString::_itoa<long>((long)-15), JSON_TEXT("-15"));
	assertEquals(NumberToString::_itoa<long>((long)-127), JSON_TEXT("-127"));
	assertEquals(NumberToString::_itoa<long>((long)-32767), JSON_TEXT("-32767"));
    assertEquals(NumberToString::_itoa<long>((long)-2147483647), JSON_TEXT("-2147483647"));	
	//AssertScopeCoverage(_itoa);
}


/**
 *	Test converting a long long value into a string
 */
void testNumberToString__itoa::testLongLong(void){
	#ifndef JSON_ISO_STRICT
		//GetScopeCoverage(_itoa, true);
		#ifdef TEST_LONG_EXTREMES
			if (sizeof(long long) >= 8){
				assertEquals(NumberToString::_itoa<long long>((long long)9223372036854775807L), JSON_TEXT("9223372036854775807"));
				assertEquals(NumberToString::_itoa<long long>((long long)-9223372036854775807L), JSON_TEXT("-9223372036854775807"));
			}	
		#endif
		assertEquals(NumberToString::_itoa<long long>((long long)2147483647), JSON_TEXT("2147483647"));
		assertEquals(NumberToString::_itoa<long long>((long long)32767), JSON_TEXT("32767"));
		assertEquals(NumberToString::_itoa<long long>((long long)127), JSON_TEXT("127"));
		assertEquals(NumberToString::_itoa<long long>((long long)15), JSON_TEXT("15"));
		assertEquals(NumberToString::_itoa<long long>((long long)0), JSON_TEXT("0"));
		assertEquals(NumberToString::_itoa<long long>((long long)-0), JSON_TEXT("0"));
		assertEquals(NumberToString::_itoa<long long>((long long)-15), JSON_TEXT("-15"));
		assertEquals(NumberToString::_itoa<long long>((long long)-127), JSON_TEXT("-127"));
		assertEquals(NumberToString::_itoa<long long>((long long)-32767), JSON_TEXT("-32767"));
		assertEquals(NumberToString::_itoa<long long>((long long)-2147483647), JSON_TEXT("-2147483647"));
		//AssertScopeCoverage(_itoa);
	#endif
}
