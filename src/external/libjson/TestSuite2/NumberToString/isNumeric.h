#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__IS_NUMERIC_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__IS_NUMERIC_H

#include "../BaseTest.h"

class testNumberToString__isNumeric : public BaseTest {
public:
	testNumberToString__isNumeric(const std::string & name);
	virtual ~testNumberToString__isNumeric();
	void testPositive(void);
	void testNegative(void);
	void testPositive_ScientificNotation(void);
	void testNegative_ScientificNotation(void);
	void testPositive_SignedScientificNotation(void);
	void testNegative_SignedScientificNotation(void);
	void testStrict(void);
	void testNotStrict(void);
	void testNotNumbers(void);
};

#endif
