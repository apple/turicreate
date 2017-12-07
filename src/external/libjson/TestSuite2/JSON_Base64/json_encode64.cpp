#include "json_encode64.h"
#include "../../Source/JSON_Base64.h"

/**
 *	Make sure that these two function reverse each other
 */
void testJSON_Base64__json_encode64::testReverseEachOther(void){
	#if defined(JSON_BINARY) || defined(JSON_EXPOSE_BASE64)
        #ifdef JSON_SAFE
    		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"", 0)), "");
        #endif
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"A", 1)), "A");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"AB", 2)), "AB");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABC", 3)), "ABC");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCD", 4)), "ABCD");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDE", 5)), "ABCDE");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEF", 6)), "ABCDEF");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFG", 7)), "ABCDEFG");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGH", 8)), "ABCDEFGH");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGHI", 9)), "ABCDEFGHI");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGHIJ", 10)), "ABCDEFGHIJ");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGHIJK", 11)), "ABCDEFGHIJK");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGHIJKL", 12)), "ABCDEFGHIJKL");
		assertEquals(JSONBase64::json_decode64(JSONBase64::json_encode64((unsigned char *)"ABCDEFGHIJKLM", 13)), "ABCDEFGHIJKLM");
#endif
}

/**
 *	Make sure all characters work in the code
 */
void testJSON_Base64__json_encode64::testAllChars(void){
	#if defined(JSON_BINARY) || defined(JSON_EXPOSE_BASE64)
	
		//create a binary chunk of data to use with every char
		unsigned char temp[255];
		for(unsigned int i = 0; i < 255; ++i){
			temp[i] = (unsigned char)i;
		}
		
		//loop through all of the lengths
		for(unsigned int length = 1; length < 255; ++length){
			json_string ts = JSONBase64::json_encode64(temp, length);
			std::string rs = JSONBase64::json_decode64(ts);
			assertEquals(rs.size(), length);
			assertEquals(memcmp(rs.data(), temp, length), 0);
		}
	#endif
}
