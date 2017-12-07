#ifndef JSON_TESTSUITE_JSON_VALIDATOR__SECURITY_TEST_H
#define JSON_TESTSUITE_JSON_VALIDATOR__SECURITY_TEST_H

#include "../BaseTest.h"

class testJSONValidator__securityTest : public BaseTest {
public:
	testJSONValidator__securityTest(const std::string & name) : BaseTest(name){}
	void testsecurity(void);
};

#endif
