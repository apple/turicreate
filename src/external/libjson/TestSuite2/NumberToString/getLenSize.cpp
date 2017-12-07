#include "getLenSize.h"
#include "../../Source/NumberToString.h"


/**
 *	Make sure the specialized template returns the right 
 *	number of bytes required
 */
void testNumberToString__getLenSize::testStruct(void){
	assertEquals(getLenSize<1>::GETLEN, 5);
	assertEquals(getLenSize<2>::GETLEN, 7);
	assertEquals(getLenSize<4>::GETLEN, 12);
	assertEquals(getLenSize<8>::GETLEN, 22);
	assertEquals(getLenSize<16>::GETLEN, 41);
}
