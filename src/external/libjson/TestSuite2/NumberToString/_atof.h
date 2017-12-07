#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__ATOF_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__ATOF_H

#include "../BaseTest.h"

class testNumberToString__atof : public BaseTest {
public:
	testNumberToString__atof(const std::string & name);
	virtual ~testNumberToString__atof();
	void testPositive(void);
	void testNegative(void);
	void testPositive_ScientificNotation(void);
	void testNegative_ScientificNotation(void);
	void testPositive_SignedScientificNotation(void);
	void testNegative_SignedScientificNotation(void);
	void testStrict(void);
	void testNotNumbers(void);
};

#endif
