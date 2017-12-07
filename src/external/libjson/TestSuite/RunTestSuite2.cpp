#include "RunTestSuite2.h"
#include "../TestSuite2/BaseTest.h"
#include "../TestSuite2/JSON_Base64/json_decode64.h"
#include "../TestSuite2/JSON_Base64/json_encode64.h"
#include "../TestSuite2/JSONDebug/JSON_ASSERT.h"
#include "../TestSuite2/JSONDebug/JSON_ASSERT_SAFE.h"
#include "../TestSuite2/JSONDebug/JSON_FAIL.h"
#include "../TestSuite2/JSONDebug/JSON_FAIL_SAFE.h"
#include "../TestSuite2/JSONGlobals/jsonSingleton.h"
#include "../TestSuite2/JSONValidator/isValidArray.h"
#include "../TestSuite2/JSONValidator/isValidMember.h"
#include "../TestSuite2/JSONValidator/isValidNamedObject.h"
#include "../TestSuite2/JSONValidator/isValidNumber.h"
#include "../TestSuite2/JSONValidator/isValidObject.h"
#include "../TestSuite2/JSONValidator/isValidPartialRoot.h"
#include "../TestSuite2/JSONValidator/isValidRoot.h"
#include "../TestSuite2/JSONValidator/isValidString.h"
#include "../TestSuite2/JSONValidator/Resources/validyMacros.h"
#include "../TestSuite2/JSONValidator/securityTest.h"
#include "../TestSuite2/NumberToString/_areFloatsEqual.h"
#include "../TestSuite2/NumberToString/_atof.h"
#include "../TestSuite2/NumberToString/_ftoa.h"
#include "../TestSuite2/NumberToString/_itoa.h"
#include "../TestSuite2/NumberToString/_uitoa.h"
#include "../TestSuite2/NumberToString/getLenSize.h"
#include "../TestSuite2/NumberToString/isNumeric.h"

#define RUNTEST(name) ttt.setUp(#name); ttt.name(); ttt.tearDown()

void RunTestSuite2::RunTests(void){
    {
        testJSON_Base64__json_decode64 ttt("testJSON_Base64__json_decode64");
        RUNTEST(testNotBase64);
    }
    {
        testJSON_Base64__json_encode64 ttt("testJSON_Base64__json_encode64");
        RUNTEST(testReverseEachOther);
        RUNTEST(testAllChars);
    }
    {
        testJSONDebug_JSON_ASSERT ttt("testJSONDebug_JSON_ASSERT");
        RUNTEST(testPass);
        RUNTEST(testFail);
    }
    {
        testJSONDebug_JSON_ASSERT_SAFE ttt("testJSONDebug_JSON_ASSERT_SAFE");
        RUNTEST(testPass);
        RUNTEST(testFail);
    }
    {
        testJSONDebug_JSON_FAIL ttt("testJSONDebug_JSON_FAIL");
        RUNTEST(testFail);
    }
    {
        testJSONDebug_JSON_FAIL_SAFE ttt("testJSONDebug_JSON_FAIL_SAFE");
        RUNTEST(testFail);
    }
    {
        testJSONGlobals__jsonSingleton ttt("testJSONGlobals__jsonSingleton");
        RUNTEST(testValue);
        RUNTEST(testNoValue);
    }
    {
        testJSONValidator__isValidMember ttt("testJSONValidator__isValidMember");
        RUNTEST(testMembers);
        RUNTEST(testStrict);
        RUNTEST(testNotStrict);
        RUNTEST(testNotMembers);
        RUNTEST(testSuddenEnd);
    }
    {
        testJSONValidator__isValidNumber ttt("testJSONValidator__isValidNumber");
        RUNTEST(testPositive);
        RUNTEST(testNegative);
        RUNTEST(testPositive_ScientificNotation);
        RUNTEST(testNegative_ScientificNotation);
        RUNTEST(testPositive_SignedScientificNotation);
        RUNTEST(testNegative_SignedScientificNotation);
        RUNTEST(testSuddenEnd);
    }
    {
        testJSONValidator__isValidRoot ttt("testJSONValidator__isValidRoot");
        RUNTEST(testRoots);
        RUNTEST(testNotRoots);
        RUNTEST(testSuddenEnd);
    }
    {
        testJSONValidator__isValidString ttt("testJSONValidator__isValidString");
        RUNTEST(testNormal);
        RUNTEST(testUnicode);
        RUNTEST(testStrict);
        RUNTEST(testNotStrict);
        RUNTEST(testNotString);
        RUNTEST(testSuddenEnd);
    }
    {
        testJSONValidator__securityTest ttt("testJSONValidator__securityTest");
        RUNTEST(testsecurity);
    }
    {
        testNumberToString__areFloatsEqual ttt("testNumberToString__areFloatsEqual");
        RUNTEST(testEqual);
        RUNTEST(testNotEqual);
        RUNTEST(testCloseEnough);
    }
	
    {
        testNumberToString__atof ttt("testNumberToString__atof");
        RUNTEST(testPositive);
        RUNTEST(testNegative);
        RUNTEST(testPositive_ScientificNotation);
        RUNTEST(testNegative_ScientificNotation);
        RUNTEST(testPositive_SignedScientificNotation);
        RUNTEST(testNegative_SignedScientificNotation);
        RUNTEST(testStrict);
        RUNTEST(testNotNumbers);
    }

    {
        testNumberToString__ftoa ttt("testNumberToString__ftoa");
        RUNTEST(testRandomNumbers);
        RUNTEST(testSpecializedInts);
    }
    {
        testNumberToString__itoa ttt("testNumberToString__itoa");
        RUNTEST(testChar);
        RUNTEST(testShort);
        RUNTEST(testInt);
        RUNTEST(testLong);
        RUNTEST(testLongLong);
    }
    {
        testNumberToString__uitoa ttt("testNumberToString__uitoa");
        RUNTEST(testChar);
        RUNTEST(testShort);
        RUNTEST(testInt);
        RUNTEST(testLong);
        RUNTEST(testLongLong);
    }
    {
        testNumberToString__getLenSize ttt("testNumberToString__getLenSize");
        RUNTEST(testStruct);
    }
    {
        testNumberToString__isNumeric ttt("testNumberToString__isNumeric");
        RUNTEST(testPositive);
        RUNTEST(testNegative);
        RUNTEST(testPositive_ScientificNotation);
        RUNTEST(testNegative_ScientificNotation);
        RUNTEST(testPositive_SignedScientificNotation);
        RUNTEST(testNegative_SignedScientificNotation);
        RUNTEST(testNotNumbers);
    }
}

