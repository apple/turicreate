#ifndef JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_STRING_H
#define JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_STRING_H

#include "../BaseTest.h"

class testJSONValidator__isValidString : public BaseTest {
public:
	testJSONValidator__isValidString(const std::string & name) : BaseTest(name){}
	void testNormal(void);
	void testUnicode(void);
	void testStrict(void);
	void testNotStrict(void);
	void testNotString(void);
	void testSuddenEnd(void);
};

#endif
