#ifndef JSON_TESTSUITE_JSON_BASE64__JSON_DECODE64_H
#define JSON_TESTSUITE_JSON_BASE64__JSON_DECODE64_H

#include "../BaseTest.h"

class testJSON_Base64__json_decode64 : public BaseTest {
public:
	testJSON_Base64__json_decode64(const std::string & name) : BaseTest(name){}
	void testNotBase64(void);
};

#endif
