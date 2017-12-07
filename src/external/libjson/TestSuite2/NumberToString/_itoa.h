#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__ITOA_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__ITOA_H

#include "../BaseTest.h"

class testNumberToString__itoa : public BaseTest {
public:
	testNumberToString__itoa(const std::string & name) : BaseTest(name){}
	void testChar(void);
	void testShort(void);
	void testInt(void);
	void testLong(void);
	void testLongLong(void);
};

#endif
