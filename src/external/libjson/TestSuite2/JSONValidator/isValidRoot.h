#ifndef JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_ROOT_H
#define JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_ROOT_H

#include "../BaseTest.h"

class testJSONValidator__isValidRoot : public BaseTest {
public:
	testJSONValidator__isValidRoot(const std::string & name) : BaseTest(name){}
	void testRoots(void);
	void testNotRoots(void);
	void testSuddenEnd(void);
};

#endif
