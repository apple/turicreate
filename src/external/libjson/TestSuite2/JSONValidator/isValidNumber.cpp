#include "isValidNumber.h"
#include "Resources/validyMacros.h"
#include "../../Source/JSONValidator.h"

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
 *	* Soon to come actual parser *
 */


/**
 *	Tests regular positive numbers in various forms
 */
void testJSONValidator__isValidNumber::testPositive(void){
	#ifdef JSON_VALIDATE
		assertValid("123,\"next\"", isValidNumber, ',');
		assertValid("12.3,\"next\"", isValidNumber, ',');
		assertValid("0.123,\"next\"", isValidNumber, ',');
		assertValid("0,\"next\"", isValidNumber, ',');
		assertValid("0.,\"next\"", isValidNumber, ',');
		assertValid("1.,\"next\"", isValidNumber, ',');
		assertValid("1,\"next\"", isValidNumber, ',');
		assertValid("0.0,\"next\"", isValidNumber, ',');
		assertValid("1.0,\"next\"", isValidNumber, ',');
		assertValid("1.01,\"next\"", isValidNumber, ',');
		//signed positives are legal when not in strict mode, this is tested below
	#endif
}


/**
 *	Tests regular negative numbers in various forms
 */
void testJSONValidator__isValidNumber::testNegative(void){
	#ifdef JSON_VALIDATE
		assertValid("-123,\"next\"", isValidNumber, ',');
		assertValid("-12.3,\"next\"", isValidNumber, ',');
		assertValid("-0.123,\"next\"", isValidNumber, ',');
		assertValid("-0,\"next\"", isValidNumber, ',');
		assertValid("-0.,\"next\"", isValidNumber, ',');
		assertValid("-1,\"next\"", isValidNumber, ',');
		assertValid("-1.,\"next\"", isValidNumber, ',');
		assertValid("-0.0,\"next\"", isValidNumber, ',');
		assertValid("-1.0,\"next\"", isValidNumber, ',');
		assertValid("-1.01,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	Tests positive numbers with regular scientific notation
 */
void testJSONValidator__isValidNumber::testPositive_ScientificNotation(void){
	#ifdef JSON_VALIDATE
		assertValid("0e123,\"next\"", isValidNumber, ',');  //TODO is 0e... a valid scientific number? its always zero
		assertNotValid("0e12.3,\"next\"", isValidNumber, ',');
		assertValid("1.e123,\"next\"", isValidNumber, ',');
		assertNotValid("1.e12.3,\"next\"", isValidNumber, ',');
		assertValid("1.0e123,\"next\"", isValidNumber, ',');
		assertNotValid("1.0e12.3,\"next\"", isValidNumber, ',');
		
		assertValid("0e2,\"next\"", isValidNumber, ',');
		assertValid("1e2,\"next\"", isValidNumber, ',');
		assertValid("0.e2,\"next\"", isValidNumber, ',');
		assertValid("1.e2,\"next\"", isValidNumber, ',');
		assertValid("0.0e2,\"next\"", isValidNumber, ',');
		assertValid("1.0e2,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	Tests negative numbers with regular scientifc notation
 */
void testJSONValidator__isValidNumber::testNegative_ScientificNotation(void){
	#ifdef JSON_VALIDATE
		assertValid("-0e123,\"next\"", isValidNumber, ',');
		assertNotValid("-0e12.3,\"next\"", isValidNumber, ',');
		assertValid("-1.e123,\"next\"", isValidNumber, ',');
		assertNotValid("-1.e12.3,\"next\"", isValidNumber, ',');
		assertValid("-1.0e123,\"next\"", isValidNumber, ',');
		assertNotValid("-1.0e12.3,\"next\"", isValidNumber, ',');
	
		assertValid("-0e2,\"next\"", isValidNumber, ',');
		assertValid("-1e2,\"next\"", isValidNumber, ',');
		assertValid("-0.e2,\"next\"", isValidNumber, ',');
		assertValid("-1.e2,\"next\"", isValidNumber, ',');
		assertValid("-0.0e2,\"next\"", isValidNumber, ',');
		assertValid("-1.0e2,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	Tests positive numbers with scientific notiation that has a sign in it
 */
void testJSONValidator__isValidNumber::testPositive_SignedScientificNotation(void){
	#ifdef JSON_VALIDATE
		assertValid("0e-123,\"next\"", isValidNumber, ',');
		assertValid("0e+123,\"next\"", isValidNumber, ',');
		assertNotValid("0e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("0e+12.3,\"next\"", isValidNumber, ',');
		assertValid("1.e-123,\"next\"", isValidNumber, ',');
		assertValid("1.e+123,\"next\"", isValidNumber, ',');
		assertNotValid("1.e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("1.e+12.3,\"next\"", isValidNumber, ',');
		assertValid("1.0e-123,\"next\"", isValidNumber, ',');
		assertValid("1.0e+123,\"next\"", isValidNumber, ',');
		assertNotValid("1.0e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("1.0e+12.3,\"next\"", isValidNumber, ',');
	
		assertValid("0e2,\"next\"", isValidNumber, ',');
		assertValid("1e2,\"next\"", isValidNumber, ',');
		assertValid("0.e2,\"next\"", isValidNumber, ',');
		assertValid("1.e2,\"next\"", isValidNumber, ',');
		assertValid("0.0e2,\"next\"", isValidNumber, ',');
		assertValid("1.0e2,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	Tests negative numbers with scientific notiation that has a sign in it
 */
void testJSONValidator__isValidNumber::testNegative_SignedScientificNotation(void){
	#ifdef JSON_VALIDATE
		assertValid("-0e-123,\"next\"", isValidNumber, ',');
		assertValid("-0e+123,\"next\"", isValidNumber, ',');
		assertNotValid("-0e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("-0e+12.3,\"next\"", isValidNumber, ',');
		assertValid("-0.e-123,\"next\"", isValidNumber, ',');
		assertValid("-0.e+123,\"next\"", isValidNumber, ',');
		assertValid("-1.e-123,\"next\"", isValidNumber, ',');
		assertValid("-1.e+123,\"next\"", isValidNumber, ',');
		assertNotValid("-1.e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("-1.e+12.3,\"next\"", isValidNumber, ',');
		assertValid("-0.0e-123,\"next\"", isValidNumber, ',');
		assertValid("-0.0e+123,\"next\"", isValidNumber, ',');
		assertValid("-1.0e-123,\"next\"", isValidNumber, ',');
		assertValid("-1.0e+123,\"next\"", isValidNumber, ',');
		assertNotValid("-1.0e-12.3,\"next\"", isValidNumber, ',');
		assertNotValid("-1.0e+12.3,\"next\"", isValidNumber, ',');
	
		assertValid("-0e-2,\"next\"", isValidNumber, ',');
		assertValid("-1e-2,\"next\"", isValidNumber, ',');
		assertValid("-0.e-2,\"next\"", isValidNumber, ',');
		assertValid("-1.e-2,\"next\"", isValidNumber, ',');
		assertValid("-0.0e-2,\"next\"", isValidNumber, ',');
		assertValid("-1.0e-2,\"next\"", isValidNumber, ',');
		assertValid("-0e+2,\"next\"", isValidNumber, ',');
		assertValid("-1e+2,\"next\"", isValidNumber, ',');
		assertValid("-0.e+2,\"next\"", isValidNumber, ',');
		assertValid("-1.e+2,\"next\"", isValidNumber, ',');
		assertValid("-0.0e+2,\"next\"", isValidNumber, ',');
		assertValid("-1.0e+2,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	Tests that in strict mode, libjson isn't relaxed about what is and isn't
 *	a valid number.  libjson by default accepts a few extra common notations.
 */
void testJSONValidator__isValidNumber::testStrict(void){
	#ifdef JSON_VALIDATE
		#ifdef JSON_STRICT
			assertNotValid("00,\"next\"", isValidNumber, ',');
			assertNotValid("00.01,\"next\"", isValidNumber, ',');
			assertNotValid(".01,\"next\"", isValidNumber, ',');  //no leading 0 as required by the standard
			assertNotValid("-.01,\"next\"", isValidNumber, ',');  //no leading 0 as required by the standard
			assertNotValid("+123,\"next\"", isValidNumber, ',');  //no leading +
			assertNotValid("+12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+0.123,\"next\"", isValidNumber, ',');
			assertNotValid("+0e123,\"next\"", isValidNumber, ',');
			assertNotValid("+0e-123,\"next\"", isValidNumber, ',');
			assertNotValid("+0e+123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e-123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e+123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e-123,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e+123,\"next\"", isValidNumber, ',');
			assertNotValid("+0e12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+0e-12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+0e+12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e-12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e+12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e-12.3,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e+12.3,\"next\"", isValidNumber, ',');
	
			assertNotValid("0x12FF,\"next\"", isValidNumber, ',');
			assertValid("0128,\"next\"", isValidNumber, ',');  //legal because in STRICT mode, this is not octal, leading zero is ignored

			assertNotValid("0xABCD,\"next\"", isValidNumber, ',');
			assertNotValid("0124,\"next\"", isValidNumber, ',');
			assertNotValid("+1,\"next\"", isValidNumber, ',');
			assertNotValid("+1.,\"next\"", isValidNumber, ',');
			assertNotValid("+0.0,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0,\"next\"", isValidNumber, ',');
			assertNotValid("+0e2,\"next\"", isValidNumber, ',');
			assertNotValid("+1e2,\"next\"", isValidNumber, ',');
			assertNotValid("+0.e2,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e2,\"next\"", isValidNumber, ',');
			assertNotValid("+0.0e2,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e2,\"next\"", isValidNumber, ',');
			assertNotValid("+0e-2,\"next\"", isValidNumber, ',');
			assertNotValid("+1e-2,\"next\"", isValidNumber, ',');
			assertNotValid("+0.e-2,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e-2,\"next\"", isValidNumber, ',');
			assertNotValid("+0e+2,\"next\"", isValidNumber, ',');
			assertNotValid("+1e+2,\"next\"", isValidNumber, ',');
			assertNotValid("+0.e+2,\"next\"", isValidNumber, ',');
			assertNotValid("+1.e+2,\"next\"", isValidNumber, ',');
			assertNotValid("+0.0e+2,\"next\"", isValidNumber, ',');
			assertNotValid("+1.0e+2,\"next\"", isValidNumber, ',');
	
			assertNotValid("1e-0123,\"next\"", isValidNumber, ',');  //not valid because of negative and leading zero
		#endif
	#endif
}


/**
 *	Tests that the extra common notations that libjson supports all
 *	test out as valid
 */
void testJSONValidator__isValidNumber::testNotStrict(void){
	#ifdef JSON_VALIDATE
		#ifndef JSON_STRICT
			assertValid("00,\"next\"", isValidNumber, ',');
			assertValid("00.01,\"next\"", isValidNumber, ',');
			assertValid(".01,\"next\"", isValidNumber, ',');
			assertValid("-.01,\"next\"", isValidNumber, ',');
			assertValid("+123,\"next\"", isValidNumber, ',');
			assertValid("+12.3,\"next\"", isValidNumber, ',');
			assertValid("+0.123,\"next\"", isValidNumber, ',');
			assertValid("+0,\"next\"", isValidNumber, ',');
			assertValid("+0.,\"next\"", isValidNumber, ',');
			assertValid("+0e123,\"next\"", isValidNumber, ',');
			assertValid("+0e-123,\"next\"", isValidNumber, ',');
			assertValid("+0e+123,\"next\"", isValidNumber, ',');
			assertValid("+1.e123,\"next\"", isValidNumber, ',');
			assertValid("+1.e-123,\"next\"", isValidNumber, ',');
			assertValid("+1.e+123,\"next\"", isValidNumber, ',');
			assertValid("+1.0e123,\"next\"", isValidNumber, ',');
			assertValid("+1.0e-123,\"next\"", isValidNumber, ',');
			assertValid("+1.0e+123,\"next\"", isValidNumber, ',');
			assertValid("+0e12.3,\"next\"", isValidNumber, ',');
			assertValid("+0e-12.3,\"next\"", isValidNumber, ',');
			assertValid("+0e+12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.e12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.e-12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.e+12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.0e12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.0e-12.3,\"next\"", isValidNumber, ',');
			assertValid("+1.0e+12.3,\"next\"", isValidNumber, ',');
	
			assertValid("0x12FF,\"next\"", isValidNumber, ',');
			#ifdef JSON_OCTAL
				assertNotValid("0128,\"next\"", isValidNumber, ',');  //because of the 8
				assertValid("0123,\"next\"", isValidNumber, ',');
				assertNotValid("-0128,\"next\"", isValidNumber, ',');
				assertValid("-0123,\"next\"", isValidNumber, ',');
			#else
				assertValid("0128,\"next\"", isValidNumber, ',');  //because of the 8
				assertValid("0123,\"next\"", isValidNumber, ',');
				assertValid("-0128,\"next\"", isValidNumber, ',');
				assertValid("-0123,\"next\"", isValidNumber, ',');
			#endif
	
	
			assertValid("0xABCD,\"next\"", isValidNumber, ',');
			assertValid("0124,\"next\"", isValidNumber, ',');
			assertValid("+1,\"next\"", isValidNumber, ',');
			assertValid("+1.,\"next\"", isValidNumber, ',');
			assertValid("+0.0,\"next\"", isValidNumber, ',');
			assertValid("+1.0,\"next\"", isValidNumber, ',');
			assertValid("+0e2,\"next\"", isValidNumber, ',');
			assertValid("+1e2,\"next\"", isValidNumber, ',');
			assertValid("+0.e2,\"next\"", isValidNumber, ',');
			assertValid("+1.e2,\"next\"", isValidNumber, ',');
			assertValid("+0.0e2,\"next\"", isValidNumber, ',');
			assertValid("+1.0e2,\"next\"", isValidNumber, ',');
			assertValid("+0e-2,\"next\"", isValidNumber, ',');
			assertValid("+1e-2,\"next\"", isValidNumber, ',');
			assertValid("+0.e-2,\"next\"", isValidNumber, ',');
			assertValid("+1.e-2,\"next\"", isValidNumber, ',');
			assertValid("+0e+2,\"next\"", isValidNumber, ',');
			assertValid("+1e+2,\"next\"", isValidNumber, ',');
			assertValid("+0.e+2,\"next\"", isValidNumber, ',');
			assertValid("+1.e+2,\"next\"", isValidNumber, ',');
			assertValid("+0.0e+2,\"next\"", isValidNumber, ',');
			assertValid("+1.0e+2,\"next\"", isValidNumber, ',');
	
			assertValid("1e-0123,\"next\"", isValidNumber, ',');
		#endif
	#endif
}


/**
 *	This tests values that aren't numbers at all, to make sure they are
 *	flagged as not valid
 */
void testJSONValidator__isValidNumber::testNotNumbers(void){
	#ifdef JSON_VALIDATE
		assertNotValid("-.,\"next\"", isValidNumber, ',');
		assertNotValid("-e,\"next\"", isValidNumber, ',');
		assertNotValid("0xABCDv,\"next\"", isValidNumber, ',');
		assertNotValid("001234,\"next\"", isValidNumber, ',');
		assertNotValid("09124,\"next\"", isValidNumber, ',');
		assertNotValid("0no,\"next\"", isValidNumber, ',');
		assertNotValid("no,\"next\"", isValidNumber, ',');
		assertNotValid("n1234,\"next\"", isValidNumber, ',');
		assertNotValid("12no,\"next\"", isValidNumber, ',');
		assertNotValid("0en5,\"next\"", isValidNumber, ',');
	#endif
}


/**
 *	This test checks that for all above mentioned valids,
 *	if the string cuts off suddenly, it recovers
 */
void testJSONValidator__isValidNumber::testSuddenEnd(void){
	#ifdef JSON_VALIDATE
		assertNotValid("", isValidNumber, ',');
	
		//--- void testJSONValidator__isValidNumber::testPositive(void){
		assertNotValid("123", isValidNumber, ',');
		assertNotValid("12.3", isValidNumber, ',');
		assertNotValid("0.123", isValidNumber, ',');
		assertNotValid("0", isValidNumber, ',');
		assertNotValid("0.", isValidNumber, ',');
		assertNotValid("1.", isValidNumber, ',');
		assertNotValid("1", isValidNumber, ',');
		assertNotValid("0.0", isValidNumber, ',');
		assertNotValid("1.0", isValidNumber, ',');
		assertNotValid("1.01", isValidNumber, ',');
		assertNotValid("0123", isValidNumber, ',');
		
		//--- void testJSONValidator__isValidNumber::testNegative(void){
		assertNotValid("-123", isValidNumber, ',');
		assertNotValid("-12.3", isValidNumber, ',');
		assertNotValid("-0.123", isValidNumber, ',');
		assertNotValid("-0", isValidNumber, ',');
		assertNotValid("-0.", isValidNumber, ',');
		assertNotValid("-1", isValidNumber, ',');
		assertNotValid("-1.", isValidNumber, ',');
		assertNotValid("-0.0", isValidNumber, ',');
		assertNotValid("-1.0", isValidNumber, ',');
		assertNotValid("-1.01", isValidNumber, ',');	
		assertNotValid("-0123", isValidNumber, ','); 
		
		//--- void testJSONValidator__isValidNumber::testPositive_ScientificNotation(void){
		assertNotValid("0e", isValidNumber, ',');
		assertNotValid("0E", isValidNumber, ',');
		assertNotValid("0e123", isValidNumber, ',');
		assertNotValid("0e12.3", isValidNumber, ',');
		assertNotValid("1.e123", isValidNumber, ',');
		assertNotValid("1.e12.3", isValidNumber, ',');
		assertNotValid("1.0e123", isValidNumber, ',');
		assertNotValid("1.0e12.3", isValidNumber, ',');
		assertNotValid("0e2", isValidNumber, ',');
		assertNotValid("1e2", isValidNumber, ',');
		assertNotValid("0.e2", isValidNumber, ',');
		assertNotValid("1.e2", isValidNumber, ',');
		assertNotValid("0.0e2", isValidNumber, ',');
		assertNotValid("1.0e2", isValidNumber, ',');
		
		//--- void testJSONValidator__isValidNumber::testNegative_ScientificNotation(void){
		assertNotValid("-0e123", isValidNumber, ',');
		assertNotValid("-0e12.3", isValidNumber, ',');
		assertNotValid("-1.e123", isValidNumber, ',');
		assertNotValid("-1.e12.3", isValidNumber, ',');
		assertNotValid("-1.0e123", isValidNumber, ',');
		assertNotValid("-1.0e12.3", isValidNumber, ',');	
		assertNotValid("-0e2", isValidNumber, ',');
		assertNotValid("-1e2", isValidNumber, ',');
		assertNotValid("-0.e2", isValidNumber, ',');
		assertNotValid("-1.e2", isValidNumber, ',');
		assertNotValid("-0.0e2", isValidNumber, ',');
		assertNotValid("-1.0e2", isValidNumber, ',');
		
		//--- void testJSONValidator__isValidNumber::testPositive_SignedScientificNotation(void){
		assertNotValid("0e-123", isValidNumber, ',');
		assertNotValid("0e+123", isValidNumber, ',');
		assertNotValid("0e-12.3", isValidNumber, ',');
		assertNotValid("0e+12.3", isValidNumber, ',');
		assertNotValid("1.e-123", isValidNumber, ',');
		assertNotValid("1.e+123", isValidNumber, ',');
		assertNotValid("1.e-12.3", isValidNumber, ',');
		assertNotValid("1.e+12.3", isValidNumber, ',');
		assertNotValid("1.0e-123", isValidNumber, ',');
		assertNotValid("1.0e+123", isValidNumber, ',');
		assertNotValid("1.0e-12.3", isValidNumber, ',');
		assertNotValid("1.0e+12.3", isValidNumber, ',');
		assertNotValid("0e2", isValidNumber, ',');
		assertNotValid("1e2", isValidNumber, ',');
		assertNotValid("0.e2", isValidNumber, ',');
		assertNotValid("1.e2", isValidNumber, ',');
		assertNotValid("0.0e2", isValidNumber, ',');
		assertNotValid("1.0e2", isValidNumber, ',');
		
		//---	void testJSONValidator__isValidNumber::testNegative_SignedScientificNotation(void){
		assertNotValid("-0e-123", isValidNumber, ',');
		assertNotValid("-0e+123", isValidNumber, ',');
		assertNotValid("-0e-12.3", isValidNumber, ',');
		assertNotValid("-0e+12.3", isValidNumber, ',');
		assertNotValid("-0.e-123", isValidNumber, ',');
		assertNotValid("-0.e+123", isValidNumber, ',');
		assertNotValid("-1.e-123", isValidNumber, ',');
		assertNotValid("-1.e+123", isValidNumber, ',');
		assertNotValid("-1.e-12.3", isValidNumber, ',');
		assertNotValid("-1.e+12.3", isValidNumber, ',');
		assertNotValid("-0.0e-123", isValidNumber, ',');
		assertNotValid("-0.0e+123", isValidNumber, ',');
		assertNotValid("-1.0e-123", isValidNumber, ',');
		assertNotValid("-1.0e+123", isValidNumber, ',');
		assertNotValid("-1.0e-12.3", isValidNumber, ',');
		assertNotValid("-1.0e+12.3", isValidNumber, ',');
		assertNotValid("-0e-2", isValidNumber, ',');
		assertNotValid("-1e-2", isValidNumber, ',');
		assertNotValid("-0.e-2", isValidNumber, ',');
		assertNotValid("-1.e-2", isValidNumber, ',');
		assertNotValid("-0.0e-2", isValidNumber, ',');
		assertNotValid("-1.0e-2", isValidNumber, ',');
		assertNotValid("-0e+2", isValidNumber, ',');
		assertNotValid("-1e+2", isValidNumber, ',');
		assertNotValid("-0.e+2", isValidNumber, ',');
		assertNotValid("-1.e+2", isValidNumber, ',');
		assertNotValid("-0.0e+2", isValidNumber, ',');
		assertNotValid("-1.0e+2", isValidNumber, ',');
		
		//strict stuff
		assertNotValid(".01", isValidNumber, ',');  //no leading 0 as required by the standard
		assertNotValid("-.01", isValidNumber, ',');  //no leading 0 as required by the standard
		assertNotValid("+123", isValidNumber, ',');  //no leading +
		assertNotValid("+12.3", isValidNumber, ',');
		assertNotValid("+0.123", isValidNumber, ',');
		assertNotValid("+0e123", isValidNumber, ',');
		assertNotValid("+0e-123", isValidNumber, ',');
		assertNotValid("+0e+123", isValidNumber, ',');
		assertNotValid("+1.e123", isValidNumber, ',');
		assertNotValid("+1.e-123", isValidNumber, ',');
		assertNotValid("+1.e+123", isValidNumber, ',');
		assertNotValid("+1.0e123", isValidNumber, ',');
		assertNotValid("+1.0e-123", isValidNumber, ',');
		assertNotValid("+1.0e+123", isValidNumber, ',');
		assertNotValid("+0e12.3", isValidNumber, ',');
		assertNotValid("+0e-12.3", isValidNumber, ',');
		assertNotValid("+0e+12.3", isValidNumber, ',');
		assertNotValid("+1.e12.3", isValidNumber, ',');
		assertNotValid("+1.e-12.3", isValidNumber, ',');
		assertNotValid("+1.e+12.3", isValidNumber, ',');
		assertNotValid("+1.0e12.3", isValidNumber, ',');
		assertNotValid("+1.0e-12.3", isValidNumber, ',');
		assertNotValid("+1.0e+12.3", isValidNumber, ',');
		assertNotValid("0x12FF", isValidNumber, ',');
		assertNotValid("0128", isValidNumber, ',');  //legal because in STRICT mode, this is not octal, leading zero is ignored
		assertNotValid("0xABCD", isValidNumber, ',');
		assertNotValid("0124", isValidNumber, ',');
		assertNotValid("+1", isValidNumber, ',');
		assertNotValid("+1.", isValidNumber, ',');
		assertNotValid("+0.0", isValidNumber, ',');
		assertNotValid("+1.0", isValidNumber, ',');
		assertNotValid("+0e2", isValidNumber, ',');
		assertNotValid("+1e2", isValidNumber, ',');
		assertNotValid("+0.e2", isValidNumber, ',');
		assertNotValid("+1.e2", isValidNumber, ',');
		assertNotValid("+0.0e2", isValidNumber, ',');
		assertNotValid("+1.0e2", isValidNumber, ',');
		assertNotValid("+0e-2", isValidNumber, ',');
		assertNotValid("+1e-2", isValidNumber, ',');
		assertNotValid("+0.e-2", isValidNumber, ',');
		assertNotValid("+1.e-2", isValidNumber, ',');
		assertNotValid("+0e+2", isValidNumber, ',');
		assertNotValid("+1e+2", isValidNumber, ',');
		assertNotValid("+0.e+2", isValidNumber, ',');
		assertNotValid("+1.e+2", isValidNumber, ',');
		assertNotValid("+0.0e+2", isValidNumber, ',');
		assertNotValid("+1.0e+2", isValidNumber, ',');
		assertNotValid("0128", isValidNumber, ',');  //because of the 8
		
		
		//--- void testJSONValidator__isValidNumber::testNotNumbers(void){
		assertNotValid("0xABCDv", isValidNumber, ',');
		assertNotValid("001234", isValidNumber, ',');
		assertNotValid("09124", isValidNumber, ',');
		assertNotValid("0no", isValidNumber, ',');
		assertNotValid("no", isValidNumber, ',');
		assertNotValid("n1234", isValidNumber, ',');
		assertNotValid("12no", isValidNumber, ',');
		assertNotValid("0en5", isValidNumber, ',');
	#endif
}
