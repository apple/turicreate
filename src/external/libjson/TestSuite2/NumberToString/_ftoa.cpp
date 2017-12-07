#include "_ftoa.h"
#include "../../Source/NumberToString.h"


/**
 *	Test that the float to string function works
 */
void testNumberToString__ftoa::testRandomNumbers(void){
	//random numbers to varying precision
	assertEquals(NumberToString::_ftoa((json_number)  1.2),    JSON_TEXT(  "1.2"));
	assertEquals(NumberToString::_ftoa((json_number) -1.2),    JSON_TEXT( "-1.2"));
    assertEquals(NumberToString::_ftoa((json_number)  1.02),   JSON_TEXT(  "1.02"));
	assertEquals(NumberToString::_ftoa((json_number) -1.02),   JSON_TEXT( "-1.02"));
    assertEquals(NumberToString::_ftoa((json_number)  1.002),  JSON_TEXT(  "1.002"));
	assertEquals(NumberToString::_ftoa((json_number) -1.002),  JSON_TEXT( "-1.002"));
    assertEquals(NumberToString::_ftoa((json_number)  3.1415), JSON_TEXT(  "3.1415"));
	assertEquals(NumberToString::_ftoa((json_number) -3.1415), JSON_TEXT( "-3.1415"));
}


/**
 *	This function reverts to one of the int functions in case of an int because
 *	they are faster.  This tests that.
 */
void testNumberToString__ftoa::testSpecializedInts(void){
	assertEquals(NumberToString::_ftoa((json_number)  1.0),    JSON_TEXT(  "1"));
    assertEquals(NumberToString::_ftoa((json_number) 10.0),    JSON_TEXT( "10"));
    assertEquals(NumberToString::_ftoa((json_number) -1.0),    JSON_TEXT( "-1"));
    assertEquals(NumberToString::_ftoa((json_number)-10.0),    JSON_TEXT("-10"));
    assertEquals(NumberToString::_ftoa((json_number)  0.0),    JSON_TEXT(  "0"));
	assertEquals(NumberToString::_ftoa((json_number) -0.0),    JSON_TEXT(  "0"));
	
	//close enough to an int
	assertEquals(NumberToString::_ftoa((json_number)  1.000000001),  JSON_TEXT( "1"));
	assertEquals(NumberToString::_ftoa((json_number) -1.000000001), JSON_TEXT( "-1"));
	assertEquals(NumberToString::_ftoa((json_number)  0.000000001),  JSON_TEXT( "0"));
	assertEquals(NumberToString::_ftoa((json_number) -0.000000001),  JSON_TEXT( "0"));
}
