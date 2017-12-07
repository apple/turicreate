#include "_areFloatsEqual.h"
#include "../../Source/NumberToString.h"


/**
 *	Tests that numbers that are actually equal are identified that way
 */
void testNumberToString__areFloatsEqual::testEqual(void){
	assertTrue(_floatsAreEqual( 0.0,  0.0));
	assertTrue(_floatsAreEqual( 1.0,  1.0));
	assertTrue(_floatsAreEqual( 1.1,  1.1));
	assertTrue(_floatsAreEqual(-1.0, -1.0));
	assertTrue(_floatsAreEqual( 0.1,  0.1));
	assertTrue(_floatsAreEqual(-0.1, -0.1));
}


/**
 *	Make sure that numbers that are very different are identified as not equal
 */
void testNumberToString__areFloatsEqual::testNotEqual(void){
	assertFalse(_floatsAreEqual( 1.0, -1.0));
	assertFalse(_floatsAreEqual( 1.0,  0.0));
	assertFalse(_floatsAreEqual(-1.0, -.0));
	assertFalse(_floatsAreEqual( 0.1,  0.0));
	assertFalse(_floatsAreEqual(-0.1, 0.0));
	assertFalse(_floatsAreEqual(1.0, 1.0001));
	assertFalse(_floatsAreEqual(1.0001, 1.0));
}


/**
 *	Make sure numbers that are different, but within the threshold of
 *	floats/doubles being equal are identified as equal
 */
void testNumberToString__areFloatsEqual::testCloseEnough(void){
	//check the exact threshold
	assertFalse(_floatsAreEqual( 0.0,  JSON_FLOAT_THRESHHOLD));
	assertFalse(_floatsAreEqual( 0.0,  -JSON_FLOAT_THRESHHOLD));
	
	//check things beneath that threashold
	assertTrue(_floatsAreEqual(0.0, JSON_FLOAT_THRESHHOLD / 2));
	assertTrue(_floatsAreEqual(0.0,  JSON_FLOAT_THRESHHOLD / -2));
	assertTrue(_floatsAreEqual(-0.1, -0.1));
	assertTrue(_floatsAreEqual(1.000000001, 1.0));
	assertTrue(_floatsAreEqual(1.0, 1.000000001));
}
