#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__ARE_FLOATS_EQUAL_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__ARE_FLOATS_EQUAL_H

#include "../BaseTest.h"

class testNumberToString__areFloatsEqual : public BaseTest {
public:
	testNumberToString__areFloatsEqual(const std::string & name) : BaseTest(name){}
	void testEqual(void);
	void testNotEqual(void);
	void testCloseEnough(void);
};

#endif
