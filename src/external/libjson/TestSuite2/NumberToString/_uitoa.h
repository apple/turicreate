#ifndef JSON_TESTSUITE_NUMBER_TO_STRING__UITOA_H
#define JSON_TESTSUITE_NUMBER_TO_STRING__UITOA_H

#include "../BaseTest.h"

class testNumberToString__uitoa : public BaseTest {
public:
	testNumberToString__uitoa(const std::string & name) : BaseTest(name){}
	void testChar(void);
	void testShort(void);
	void testInt(void);
	void testLong(void);
	void testLongLong(void);
};

#endif
