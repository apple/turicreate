#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__FTOA_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__FTOA_H

#include "../BaseTest.h"

class testNumberToString__ftoa : public BaseTest {
public:
	testNumberToString__ftoa(const std::string & name) : BaseTest(name){}
	void testRandomNumbers(void);
	void testSpecializedInts(void);
};

#endif
