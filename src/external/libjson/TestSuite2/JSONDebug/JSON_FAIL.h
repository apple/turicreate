#ifndef JSON_TESTSUITE_JSON_DEBUG__JSON_FAIL_H
#define JSON_TESTSUITE_JSON_DEBUG__JSON_FAIL_H

#include "../BaseTest.h"

class testJSONDebug_JSON_FAIL : public BaseTest {
public:
	testJSONDebug_JSON_FAIL(const std::string & name) : BaseTest(name){}
	virtual void setUp(const std::string & methodName);
	virtual void tearDown(void);
	void testFail(void);
};

#endif
