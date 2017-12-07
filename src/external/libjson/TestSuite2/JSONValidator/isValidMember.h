#ifndef JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_MEMBER_H
#define JSON_TESTSUITE_JSON_VALIDATOR__IS_VALID_MEMBER_H

#include "../BaseTest.h"

class testJSONValidator__isValidMember : public BaseTest {
public:
	testJSONValidator__isValidMember(const std::string & name) : BaseTest(name){}
	void testMembers(void);
	void testStrict(void);
	void testNotStrict(void);
	void testNotMembers(void);
	void testSuddenEnd(void);
};

#endif
