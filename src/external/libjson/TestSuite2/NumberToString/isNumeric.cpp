#include "isNumeric.h"
#include "../../Source/NumberToString.h"

/*
 *
 *	!!! ATTENTION !!!
 *
 *	libjson currently has three number parsing methods, they are being merged 
 *	behind the scenes, but all three interfaces must be consistent, so every set
 *	of numbers need to be tested in all three spots
 *
 *	JSONValidator/isValidNumber     *this file*
 *	NumberToString/isNumeric
 *	NumberToString/_atof
 */

testNumberToString__isNumeric::testNumberToString__isNumeric(const std::string & name) : BaseTest(name){
	/*
	#ifndef JSON_STRICT
		ScopeCoverageHeap(isNumeric, 34);
	#else
		ScopeCoverageHeap(isNumeric, 35);
	#endif
	 */
}
testNumberToString__isNumeric::~testNumberToString__isNumeric(){
	//AssertScopeCoverageHeap(isNumeric);
}


/**
 *	Tests regular positive numbers in various forms
 */
void testNumberToString__isNumeric::testPositive(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.01")));
	#endif
}


/**
 *	Tests negative numbers with regular scientifc notation
 */
void testNumberToString__isNumeric::testNegative(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0")));
	#endif
}

/**
 *	Tests positive numbers with scientific notiation that has a sign in it
 */
void testNumberToString__isNumeric::testPositive_ScientificNotation(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0e12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.e12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.0e12.3")));
		
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e2")));
	#endif
}

/**
 *	Tests negative numbers with regular scientifc notation
 */
void testNumberToString__isNumeric::testNegative_ScientificNotation(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-0e12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.e12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.0e12.3")));
		
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0.0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e2")));
	#endif
}


/**
 *	Tests positive numbers with scientific notiation that has a sign in it
 */
void testNumberToString__isNumeric::testPositive_SignedScientificNotation(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0e-12.3")));  //period not supposed to be in there, exponent must be int
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0e+12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.e+12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.0e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("1.0e+12.3")));
		
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("0.0e2")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("1.0e2")));
	#endif
}


/**
 *	Tests negative numbers with scientific notiation that has a sign in it
 */
void testNumberToString__isNumeric::testNegative_SignedScientificNotation(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-0e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-0.e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-0.e+12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.e+12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-0.0e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-0.0e+12.3")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e-123")));
		assertTrue(NumberToString::isNumeric(JSON_TEXT("-1.0e+123")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.0e-12.3")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-1.0e+12.3")));
		
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
	#endif
}


/**
 *	Tests that in strict mode, libjson isn't relaxed about what is and isn't
 *	a valid number.  libjson by default accepts a few extra common notations.
 */
void testNumberToString__isNumeric::testStrict(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		#ifdef JSON_STRICT
			assertFalse(NumberToString::isNumeric(JSON_TEXT("00")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("00.01")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT(".01")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("-.01")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0.")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e-123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e+123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e-123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e+123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e-123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e+123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e-12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+0e+12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e-12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e+12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e-12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.0e+12.3")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1.e12.3")));
	
			assertFalse(NumberToString::isNumeric(JSON_TEXT("0x12FF")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("0128")));  //legal because in STRICT mode, this is not octal, leading zero is ignored
			assertFalse(NumberToString::isNumeric(JSON_TEXT("0123")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("-0128")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("-0123")));
	
			assertFalse(NumberToString::isNumeric(JSON_TEXT("0xABCD")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("0124")));
			assertFalse(NumberToString::isNumeric(JSON_TEXT("+1")));
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
	
			assertFalse(NumberToString::isNumeric(JSON_TEXT("1e-0123")));  //not valid because of negative and leading zero
		#endif
	#endif
}
	

/**
 *	Tests that the extra common notations that libjson supports all
 *	test out as valid
 */
void testNumberToString__isNumeric::testNotStrict(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		#ifndef JSON_STRICT
			assertTrue(NumberToString::isNumeric(JSON_TEXT("00")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("00.01")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT(".01")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("-.01")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+12.3")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0.")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e-123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+0e+123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e-123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e+123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e-123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.0e+123")));
			assertTrue(NumberToString::isNumeric(JSON_TEXT("+1.e123")));
	
			assertTrue(NumberToString::isNumeric(JSON_TEXT("0x12FF")));
			#ifdef JSON_OCTAL
				assertFalse(NumberToString::isNumeric(JSON_TEXT("0128")));  //because of the 8
				assertTrue(NumberToString::isNumeric(JSON_TEXT("0123")));
				assertFalse(NumberToString::isNumeric(JSON_TEXT("-0128")));
				assertTrue(NumberToString::_atof(JSON_TEXT("-0123")));
			#else
				assertTrue(NumberToString::isNumeric(JSON_TEXT("0128")));  //because the leading 0 is ignored
				assertTrue(NumberToString::isNumeric(JSON_TEXT("0123")));
				assertTrue(NumberToString::isNumeric(JSON_TEXT("-0128")));  //because the leading 0 is ignored
				assertTrue(NumberToString::isNumeric(JSON_TEXT("-0123")));
			#endif
	
	
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
	
			assertTrue(NumberToString::isNumeric(JSON_TEXT("1e-0123")));
		#endif
	#endif
}


/**
 *	This tests values that aren't numbers at all, to make sure they are
 *	flagged as not valid
 */
void testNumberToString__isNumeric::testNotNumbers(void){
	#if defined(JSON_SAFE) || defined(JSON_DEBUG)
		assertFalse(NumberToString::isNumeric(JSON_TEXT("")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-.")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("-e12")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0xABCDv")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("00124")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("09124")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0no")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("no")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("n1234")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("12no")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0en5")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0e")));
		assertFalse(NumberToString::isNumeric(JSON_TEXT("0E")));
	#endif
}
