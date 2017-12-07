#include "_atof.h"
#include "../../Source/NumberToString.h"

#ifdef JSON_SAFE
	#define assertNaN(one) assertNAN(json_number, one)
#else
	#define assertNaN(one)
#endif

testNumberToString__atof::testNumberToString__atof(const std::string & name) : BaseTest(name){
	//ScopeCoverageHeap(_atof, 14);
}
testNumberToString__atof::~testNumberToString__atof(){
	//AssertScopeCoverageHeap(_atof);
}

/**
 *	Tests regular positive numbers in various forms
 */
void testNumberToString__atof::testPositive(void){
#ifdef JSON_STRICT
	assertFloatEquals(123, NumberToString::_atof(JSON_TEXT("123")));
	assertFloatEquals(12.3, NumberToString::_atof(JSON_TEXT("12.3")));
	assertFloatEquals(0.123, NumberToString::_atof(JSON_TEXT("0.123")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("0")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("0.")));
	assertFloatEquals(1, NumberToString::_atof(JSON_TEXT("1.")));
	assertFloatEquals(1, NumberToString::_atof(JSON_TEXT("1")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("0.0")));
	assertFloatEquals(1, NumberToString::_atof(JSON_TEXT("1.0")));
	assertFloatEquals(1.01, NumberToString::_atof(JSON_TEXT("1.01")));
#endif
}

/**
 *	Tests negative numbers with regular scientifc notation
 */
void testNumberToString__atof::testNegative(void){
#ifdef JSON_STRICT
	assertFloatEquals(-123, NumberToString::_atof(JSON_TEXT("-123")));
	assertFloatEquals(-12.3, NumberToString::_atof(JSON_TEXT("-12.3")));
	assertFloatEquals(-.123, NumberToString::_atof(JSON_TEXT("-0.123")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("-0")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("-0.")));
	assertFloatEquals(-1, NumberToString::_atof(JSON_TEXT("-1")));
	assertFloatEquals(-1, NumberToString::_atof(JSON_TEXT("-1.")));
	assertFloatEquals(0, NumberToString::_atof(JSON_TEXT("-0.0")));
	assertFloatEquals(-1, NumberToString::_atof(JSON_TEXT("-1.0")));
#endif
}

/**
 *	Tests positive numbers with scientific notiation that has a sign in it
 */
void testNumberToString__atof::testPositive_ScientificNotation(void){
#ifdef JSON_STRICT
	assertNAN(json_number, std::numeric_limits<json_number>::signaling_NaN());  //sanity check
	assertFloatEquals(0e3, NumberToString::_atof(JSON_TEXT("0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0e3.3")));

	assertFloatEquals(1e3, NumberToString::_atof(JSON_TEXT("1.e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.e3.3")));
	assertFloatEquals(1e3, NumberToString::_atof(JSON_TEXT("1.0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.0e3.3")));
	
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0.e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1.e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0.0e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1.0e2")));
#endif
}

/**
 *	Tests negative numbers with regular scientifc notation
 */
void testNumberToString__atof::testNegative_ScientificNotation(void){
#ifdef JSON_STRICT
	assertFloatEquals(0e3, NumberToString::_atof(JSON_TEXT("-0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-0e3.3")));
	assertFloatEquals(-1e3, NumberToString::_atof(JSON_TEXT("-1.e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.e3.3")));
	assertFloatEquals(-1e3, NumberToString::_atof(JSON_TEXT("-1.0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.0e3.3")));
	
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0e2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0.e2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1.e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0.0e2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1.0e2")));
#endif
}

/**
 *	Tests positive numbers with scientific notiation that has a sign in it
 */
void testNumberToString__atof::testPositive_SignedScientificNotation(void){
#ifdef JSON_STRICT
	assertFloatEquals(0e-3, NumberToString::_atof(JSON_TEXT("0e-3")));
	assertFloatEquals(0e+3, NumberToString::_atof(JSON_TEXT("0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0e+3.3")));
	assertFloatEquals(1e-3, NumberToString::_atof(JSON_TEXT("1.e-3")));
	assertFloatEquals(1e3, NumberToString::_atof(JSON_TEXT("1.e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.e+3.3")));
	assertFloatEquals(1e-3, NumberToString::_atof(JSON_TEXT("1.0e-3")));
	assertFloatEquals(1e3, NumberToString::_atof(JSON_TEXT("1.0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.0e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("1.0e+3.3")));
	
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0.e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1.e2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("0.0e2")));
	assertFloatEquals(1e2, NumberToString::_atof(JSON_TEXT("1.0e2")));
#endif
}


/**
 *	Tests negative numbers with scientific notiation that has a sign in it
 */
void testNumberToString__atof::testNegative_SignedScientificNotation(void){
#ifdef JSON_STRICT
	assertFloatEquals(0e-3, NumberToString::_atof(JSON_TEXT("-0e-3")));
	assertFloatEquals(0e3, NumberToString::_atof(JSON_TEXT("-0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-0.e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-0.e+3.3")));
	assertFloatEquals(-1e-3, NumberToString::_atof(JSON_TEXT("-1.e-3")));
	assertFloatEquals(-1e3, NumberToString::_atof(JSON_TEXT("-1.e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.e+3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-0.0e-3.3")));
	assertNaN( NumberToString::_atof(JSON_TEXT("-0.0e+3.3")));
	assertFloatEquals(-1e-3, NumberToString::_atof(JSON_TEXT("-1.0e-3")));
	assertFloatEquals(-1e3, NumberToString::_atof(JSON_TEXT("-1.0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.0e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-1.0e+3.3")));
	
	assertFloatEquals(0e-2, NumberToString::_atof(JSON_TEXT("-0e-2")));
	assertFloatEquals(-1e-2, NumberToString::_atof(JSON_TEXT("-1e-2")));
	assertFloatEquals(0e-2, NumberToString::_atof(JSON_TEXT("-0.e-2")));
	assertFloatEquals(-1e-2, NumberToString::_atof(JSON_TEXT("-1.e-2")));
	assertFloatEquals(0e-2, NumberToString::_atof(JSON_TEXT("-0.0e-2")));
	assertFloatEquals(-1e-2, NumberToString::_atof(JSON_TEXT("-1.0e-2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0e+2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1e+2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0.e+2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1.e+2")));
	assertFloatEquals(0e2, NumberToString::_atof(JSON_TEXT("-0.0e+2")));
	assertFloatEquals(-1e2, NumberToString::_atof(JSON_TEXT("-1.0e+2")));
	
	assertNaN(NumberToString::_atof(JSON_TEXT("1e-0123")));  //not valid because of negative and leading zero
#endif
}

void testNumberToString__atof::testStrict(void){
#if defined(JSON_SAFE) || defined(JSON_DEBUG)
#ifdef JSON_STRICT
	assertNaN(NumberToString::_atof(JSON_TEXT("00")));
	assertNaN(NumberToString::_atof(JSON_TEXT("00.01")));
	assertNaN(NumberToString::_atof(JSON_TEXT(".01")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-.01")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+123")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+12.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.123")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e-3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e-3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e-3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e+3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e+3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e+3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e-3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e+3.3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e3.3")));
	
	assertNaN(NumberToString::_atof(JSON_TEXT("0x12FF")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0128")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0123")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-0123")));
	
	assertNaN(NumberToString::_atof(JSON_TEXT("0xABCD")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0124")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.0")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.0e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.0e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e-2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0e+2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1e+2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.e+2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.e+2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+0.0e+2")));
	assertNaN(NumberToString::_atof(JSON_TEXT("+1.0e+2")));
#endif
#endif
}

void testNumberToString__atof::testNotNumbers(void){
#if defined(JSON_SAFE) || defined(JSON_DEBUG)
#ifdef JSON_STRICT
	assertNaN(NumberToString::_atof(JSON_TEXT("-.")));
	assertNaN(NumberToString::_atof(JSON_TEXT("-e3")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0xABCDv")));
	assertNaN(NumberToString::_atof(JSON_TEXT("00124")));
	assertNaN(NumberToString::_atof(JSON_TEXT("09124")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0no")));
	assertNaN(NumberToString::_atof(JSON_TEXT("no")));
	assertNaN(NumberToString::_atof(JSON_TEXT("n1234")));
	assertNaN(NumberToString::_atof(JSON_TEXT("12no")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0en5")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0e")));
	assertNaN(NumberToString::_atof(JSON_TEXT("0E")));
#endif
#endif
}

