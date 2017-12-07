#ifndef JSON_TESTSUITE_BASETEST_H
#define JSON_TESTSUITE_BASETEST_H

#include "../TestSuite/UnitTest.h"
#include <string>

class libjson_CodeCoverage;

class BaseTest {
public:
	BaseTest(const std::string & name) : _name(name), coverage(0) {}
	virtual ~BaseTest(void){};
	virtual void setUp(const std::string & methodName){ UnitTest::SetPrefix(_name + "::" + methodName); }
	virtual void tearDown(void){}
protected:
	const std::string _name;
	libjson_CodeCoverage * coverage;
private:
	BaseTest(const BaseTest &);
	BaseTest & operator=(const BaseTest &);
};

#endif

