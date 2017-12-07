#ifndef JSON_TESTSUITE_JSON_DEBUG__JSON_ASSERT_H
#define JSON_TESTSUITE_JSON_DEBUG__JSON_ASSERT_H

#include "../BaseTest.h"

class testJSONDebug_JSON_ASSERT : public BaseTest {
public:
	testJSONDebug_JSON_ASSERT(const std::string & name) : BaseTest(name){}
	virtual void setUp(const std::string & methodName);
	virtual void tearDown(void);
	void testPass(void);
	void testFail(void);
};
	
#endif
