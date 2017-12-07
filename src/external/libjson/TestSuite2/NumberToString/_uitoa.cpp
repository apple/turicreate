#include "_uitoa.h"
#include "../../Source/NumberToString.h"


/**
 *	Test converting a char value into a string
 */
void testNumberToString__uitoa::testChar(void){
	#ifndef JSON_LIBRARY
		assertEquals(sizeof(unsigned char), 1);
		assertEquals(NumberToString::_uitoa<unsigned char>((unsigned char)255), JSON_TEXT("255"));
		assertEquals(NumberToString::_uitoa<unsigned char>((unsigned char)127), JSON_TEXT("127"));
		assertEquals(NumberToString::_uitoa<unsigned char>((unsigned char)15), JSON_TEXT("15"));
		assertEquals(NumberToString::_uitoa<unsigned char>((unsigned char)0), JSON_TEXT("0"));
	#endif
}


/**
 *	Test converting a short value into a string
 */
void testNumberToString__uitoa::testShort(void){
	#ifndef JSON_LIBRARY
		assertEquals(sizeof(unsigned short), 2);
		assertEquals(NumberToString::_uitoa<unsigned short>((unsigned short)65535), JSON_TEXT("65535"));
		assertEquals(NumberToString::_uitoa<unsigned short>((unsigned short)32767), JSON_TEXT("32767"));
		assertEquals(NumberToString::_uitoa<unsigned short>((unsigned short)127), JSON_TEXT("127"));
		assertEquals(NumberToString::_uitoa<unsigned short>((unsigned short)15), JSON_TEXT("15"));
		assertEquals(NumberToString::_uitoa<unsigned short>((unsigned short)0), JSON_TEXT("0"));
	#endif
}


/**
 *	Test converting a int value into a string
 */
void testNumberToString__uitoa::testInt(void){
	#ifndef JSON_LIBRARY
		assertEquals(sizeof(unsigned int), 4);
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)4294967295u), JSON_TEXT("4294967295"));
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)2147483647), JSON_TEXT("2147483647"));
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)32767), JSON_TEXT("32767"));
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)127), JSON_TEXT("127"));
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)15), JSON_TEXT("15"));
		assertEquals(NumberToString::_uitoa<unsigned int>((unsigned int)0), JSON_TEXT("0"));	
	#endif
}


/**
 *	Test converting a long value into a string
 */
void testNumberToString__uitoa::testLong(void){
	#ifndef JSON_LIBRARY
		#ifdef TEST_LONG_EXTREMES
			if (sizeof(unsigned long) >= 8){
				assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)18446744073709551615UL), JSON_TEXT("18446744073709551615"));
				assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)9223372036854775807L), JSON_TEXT("9223372036854775807"));
			}	
		#endif
		assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)2147483647), JSON_TEXT("2147483647"));
		assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)32767), JSON_TEXT("32767"));
		assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)127), JSON_TEXT("127"));
		assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)15), JSON_TEXT("15"));
		assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long)0), JSON_TEXT("0"));
	#endif
}


/**
 *	Test converting a long long value into a string
 */
void testNumberToString__uitoa::testLongLong(void){
	#ifndef JSON_LIBRARY
		#ifndef JSON_ISO_STRICT
			#ifdef TEST_LONG_EXTREMES
				if (sizeof(unsigned long long) >= 8){
					assertEquals(NumberToString::_uitoa<unsigned long>((unsigned long long)18446744073709551615UL), JSON_TEXT("18446744073709551615"));
					assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)9223372036854775807L), JSON_TEXT("9223372036854775807"));
					assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)-9223372036854775807L), JSON_TEXT("-9223372036854775807"));
				}	
			#endif
			assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)2147483647), JSON_TEXT("2147483647"));
			assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)32767), JSON_TEXT("32767"));
			assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)127), JSON_TEXT("127"));
			assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)15), JSON_TEXT("15"));
			assertEquals(NumberToString::_uitoa<unsigned long long>((unsigned long long)0), JSON_TEXT("0"));
		#endif
	#endif
}
