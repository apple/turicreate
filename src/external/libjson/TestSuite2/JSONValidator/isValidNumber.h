#ifndef JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_NUMBER_H
#define JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_NUMBER_H

#include "../BaseTest.h"

class testJSONValidator__isValidNumber : public BaseTest {
public:
	testJSONValidator__isValidNumber(const std::string & name) : BaseTest(name){}
	void testPositive(void);
	void testNegative(void);
	void testPositive_ScientificNotation(void);
	void testNegative_ScientificNotation(void);
	void testPositive_SignedScientificNotation(void);
	void testNegative_SignedScientificNotation(void);
	void testStrict(void);
	void testNotStrict(void);
	void testNotNumbers(void);
	void testSuddenEnd(void);
};
	
#endif
