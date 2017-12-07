#ifndef JSON_TESTSUITE_JSON_GLOBALS__JSON_SINGLETON_H
#define JSON_TESTSUITE_JSON_GLOBALS__JSON_SINGLETON_H

#include "../BaseTest.h"

class testJSONGlobals__jsonSingleton : public BaseTest {
public:
	testJSONGlobals__jsonSingleton(const std::string & name) : BaseTest(name){}
	void testValue(void);
	void testNoValue(void);
};

#endif
