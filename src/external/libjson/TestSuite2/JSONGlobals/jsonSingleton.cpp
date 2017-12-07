#include "jsonSingleton.h"
#include "../../Source/JSONGlobals.h"

json_global_decl(std::string, WITHVALUE, "myvalue");
json_global_decl(std::string, WITHOUTVALUE, );

void testJSONGlobals__jsonSingleton::testValue(void){
	std::string * p1 = &jsonSingletonWITHVALUE::getValue();
	std::string * p2 = &json_global(WITHVALUE);
	assertEquals(p1, p2);
	assertEquals(json_global(WITHVALUE), "myvalue");
	assertEquals(jsonSingletonWITHVALUE::getValue(), "myvalue");
}

void testJSONGlobals__jsonSingleton::testNoValue(void){
	std::string * p1 = &jsonSingletonWITHOUTVALUE::getValue();
	std::string * p2 = &json_global(WITHOUTVALUE);
	assertEquals(p1, p2);
	assertEquals(json_global(WITHOUTVALUE), "");
	assertEquals(jsonSingletonWITHOUTVALUE::getValue(), "");
}
