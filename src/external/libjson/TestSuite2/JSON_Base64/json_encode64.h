#ifndef JSON_TESTSUITE_JSON_BASE64__JSON_ENCODE64_H
#define JSON_TESTSUITE_JSON_BASE64__JSON_ENCODE64_H

#include "../BaseTest.h"

class testJSON_Base64__json_encode64 : public BaseTest {
public:
	testJSON_Base64__json_encode64(const std::string & name) : BaseTest(name){}
	void testReverseEachOther(void);
	void testAllChars(void);
};

#endif
