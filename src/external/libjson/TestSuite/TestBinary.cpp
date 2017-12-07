#include "TestSuite.h"
#include "../Source/JSON_Base64.h"

#if defined(JSON_BINARY) || defined(JSON_EXPOSE_BASE64)
    void TestSuite::TestBase64(void){
	   UnitTest::SetPrefix("TestBinary.cpp - Base 64");

	   assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"A", 1)), "A");
	   assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"AB", 2)), "AB");
	   assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABC", 3)), "ABC");
	   assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCD", 4)), "ABCD");
	   #ifdef JSON_SAFE
          assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"", 0)), "");
		  assertEquals(JSONBase64::json_decode64(JSON_TEXT("123!abc")), "");
		  assertEquals(JSONBase64::json_decode64(JSON_TEXT("123=abc")), "");
		  assertEquals(JSONBase64::json_decode64(JSON_TEXT("123abc===")), "");
	   #endif

	   unsigned char temp[255];
	   for(unsigned int i = 0; i < 255; ++i){
		  temp[i] = (unsigned char)i;
	   }
	   json_string ts = JSONBase64::json_encode64(temp, 255);
	   std::string rs = JSONBase64::json_decode64(ts);
	   assertEquals(rs.size(), 255);
	   assertEquals(memcmp(rs.data(), temp, 255), 0);
		
		#if defined(JSON_LIBRARY) && defined(JSON_EXPOSE_BASE64)
			json_char * test = json_encode64(temp, 255);
			assertNotNull(test);
			unsigned long _size;
			void * bin = json_decode64(test, & _size);
			assertNotNull(bin);
			assertEquals(_size, 255);
			assertEquals(memcmp(bin, temp, 255), 0);
			json_free(test);
			json_free(bin);
		#endif
    }
#endif
