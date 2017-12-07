#ifndef JSON_TESTSUITE_JSON_DEBUG__JSON_ASSERT_SAFE_H
#define JSON_TESTSUITE_JSON_DEBUG__JSON_ASSERT_SAFE_H

#include "../BaseTest.h"

class testJSONDebug_JSON_ASSERT_SAFE : public BaseTest {
public:
	testJSONDebug_JSON_ASSERT_SAFE(const std::string & name) : BaseTest(name){}
	virtual void setUp(const std::string & methodName);
	virtual void tearDown(void);
	void testPass(void);
	void testFail(void);
};

#endif
