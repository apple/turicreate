#include "TestSuite.h"

#ifdef JSON_WRITE_PRIORITY
	extern bool used_ascii_one;
	
	void myDoTests(bool asciichar);
	void myDoTests(bool asciichar){
        used_ascii_one = asciichar;
		#ifdef JSON_LIBRARY
		  #define assertWrite(node, func, expected)\
			 {\
				json_char * _temp = func(node);\
				assertCStringSame(_temp, expected);\
				json_free(_temp);\
			 }

		  JSONNODE * test1 = json_new(JSON_NODE);
		  assertWrite(test1, json_write, JSON_TEXT("{}"));
		  json_push_back(test1, json_new_a(JSON_TEXT("Hello"), JSON_TEXT("World")));
		  json_push_back(test1, json_new_b(JSON_TEXT("libjson"), true));
		  
		  assertWrite(test1, json_write, JSON_TEXT("{\"Hello\":\"World\",\"libjson\":true}"));
		  #ifdef JSON_NEWLINE
			 assertEquals(JSON_NEWLINE, JSON_TEXT("\r\n"));
			 #ifdef JSON_INDENT
				assertEquals(JSON_INDENT, JSON_TEXT("    "))
				assertWrite(test1, json_write_formatted, JSON_TEXT("{\r\n    \"Hello\" : \"World\",\r\n    \"libjson\" : true\r\n}"));
			 #else
				assertWrite(test1, json_write_formatted, JSON_TEXT("{\r\n\t\"Hello\" : \"World\",\r\n\t\"libjson\" : true\r\n}"));
			 #endif
		  #else
			 #ifdef JSON_INDENT
				assertEquals(JSON_INDENT, JSON_TEXT("    "))
				assertWrite(test1, json_write_formatted, JSON_TEXT("{\n    \"Hello\" : \"World\",\n    \"libjson\" : true\n}"));
			 #else
				assertWrite(test1, json_write_formatted, JSON_TEXT("{\n\t\"Hello\" : \"World\",\n\t\"libjson\" : true\n}"));
			 #endif
		  #endif
		  json_delete(test1);

		  JSONNODE * test2 = json_new(JSON_ARRAY);
		  assertWrite(test2, json_write, JSON_TEXT("[]"));
		  json_delete(test2);


		  JSONNODE * card = json_new(JSON_ARRAY);
		  JSONNODE *c = json_new(JSON_ARRAY);
		  json_push_back(c, json_new_a(JSON_TEXT("name"), JSON_TEXT("Entr\033e Audio Int\033gr\033e 1")));
		  json_push_back(c, json_new_i(NULL, 0));
		  json_push_back(card, c);
		  #ifdef JSON_READ_PRIORITY
			 #ifndef JSON_ESCAPE_WRITES
				assertWrite(card, json_write, JSON_TEXT("[[\"Entr\033e Audio Int\033gr\033e 1\",0]]"))
				JSONNODE * ass = json_parse(JSON_TEXT("[[\"Entr\033e Audio Int\033gr\033e 1\",0]]"));
			 #else
				assertWrite(card, json_write, JSON_TEXT("[[\"Entr\\u001Be Audio Int\\u001Bgr\\u001Be 1\",0]]"))
				JSONNODE * ass = json_parse(JSON_TEXT("[[\"Entr\\u001Be Audio Int\\u001Bgr\\u001Be 1\",0]]"));
			 #endif
			 JSONNODE * item = json_at(json_at(ass, 0), 0);
			 assertWrite(item, json_as_string, JSON_TEXT("Entr\033e Audio Int\033gr\033e 1"));
			 json_delete(ass);
		  #endif
		  json_delete(card);


		  #ifdef JSON_COMMENTS
			 JSONNODE * test3 = json_new(JSON_NODE);
			 json_push_back(test3, json_new_a(JSON_TEXT("Hi"), JSON_TEXT("\\There\\")));
			 json_push_back(test3, json_new_a(JSON_TEXT("Hello"), JSON_TEXT("World")));
			 json_set_comment(json_at(test3, 0), JSON_TEXT("Testing stuff"));
			 json_set_comment(json_at(test3, 1), JSON_TEXT("Multi\r\nLine\nUnix and Windows"));
			 assertWrite(test3, json_write, JSON_TEXT("{\"Hi\":\"\\\\There\\\\\",\"Hello\":\"World\"}"));
			 #if !defined( JSON_INDENT) && !defined(JSON_NEWLINE)
				#ifdef JSON_WRITE_BASH_COMMENTS
				    assertWrite(test3, json_write_formatted, JSON_TEXT("{\n\t\n\t#Testing stuff\n\t\"Hi\" : \"\\\\There\\\\\",\n\t\n\t#Multi\n\t#Line\n\t#Unix and Windows\n\t\"Hello\" : \"World\"\n}"));
				#elif defined(JSON_WRITE_SINGLE_LINE_COMMENTS)
				    assertWrite(test3, json_write_formatted, JSON_TEXT("{\n\t\n\t//Testing stuff\n\t\"Hi\" : \"\\\\There\\\\\",\n\t\n\t//Multi\n\t//Line\n\t//Unix and Windows\n\t\"Hello\" : \"World\"\n}"));
				#else
				    assertWrite(test3, json_write_formatted, JSON_TEXT("{\n\t\n\t//Testing stuff\n\t\"Hi\" : \"\\\\There\\\\\",\n\t\n\t/*\n\t\tMulti\n\t\tLine\n\t\tUnix and Windows\n\t*/\n\t\"Hello\" : \"World\"\n}"));
				#endif
			 #endif
			 json_delete(test3);
		  #endif

		  #ifdef JSON_ARRAY_SIZE_ON_ONE_LINE
			 #if (JSON_ARRAY_SIZE_ON_ONE_LINE == 2)
				JSONNODE * arrarrr = json_new(JSON_ARRAY);
				json_push_back(arrarrr, json_new_i(JSON_TEXT(""), 12));
				json_push_back(arrarrr, json_new_i(JSON_TEXT(""), 8));
				assertWrite(arrarrr, json_write_formatted, JSON_TEXT("[12, 8]"));
				assertWrite(arrarrr, json_write, JSON_TEXT("[12,8]"));
				json_delete(arrarrr);
			 #endif
		  #endif

	   #else
		  JSONNode test1(JSON_NODE);
		  assertEquals(test1.write(), JSON_TEXT("{}"));
		  test1.push_back(JSONNode(JSON_TEXT("Hello"), JSON_TEXT("\\World\\")));
		  test1.push_back(JSONNode(JSON_TEXT("libjson"), true));
		  assertEquals(test1.write(), JSON_TEXT("{\"Hello\":\"\\\\World\\\\\",\"libjson\":true}"));
		  #ifdef JSON_NEWLINE
			 assertEquals(JSON_NEWLINE, JSON_TEXT("\r\n"));
			 #ifdef JSON_INDENT
				assertEquals(JSON_INDENT, JSON_TEXT("    "))
				assertEquals(test1.write_formatted(), JSON_TEXT("{\r\n    \"Hello\" : \"\\\\World\\\\\",\r\n    \"libjson\" : true\r\n}"));
			 #else
				assertEquals(test1.write_formatted(), JSON_TEXT("{\r\n\t\"Hello\" : \"\\\\World\\\\\",\r\n\t\"libjson\" : true\r\n}"));
			 #endif
		  #else
			 #ifdef JSON_INDENT
				assertEquals(JSON_INDENT, JSON_TEXT("    "))
				assertEquals(test1.write_formatted(), JSON_TEXT("{\n    \"Hello\" : \"\\\\World\\\\\",\n    \"libjson\" : true\n}"));
			 #else
				assertEquals(test1.write_formatted(), JSON_TEXT("{\n\t\"Hello\" : \"\\\\World\\\\\",\n\t\"libjson\" : true\n}"));
			 #endif
		  #endif

		  JSONNode test2(JSON_ARRAY);
		  assertEquals(test2.write(), JSON_TEXT("[]"));

		  #ifdef JSON_COMMENTS
			 JSONNode test3(JSON_NODE);
			 test3.push_back(JSONNode(JSON_TEXT("Hi"), JSON_TEXT("There")));
			 test3.push_back(JSONNode(JSON_TEXT("Hello"), JSON_TEXT("World")));
			 test3[0].set_comment(JSON_TEXT("Testing stuff"));
			 test3[1].set_comment(JSON_TEXT("Multi\r\nLine\nUnix and Windows"));
			 assertEquals(test3.write(), JSON_TEXT("{\"Hi\":\"There\",\"Hello\":\"World\"}"));
			 #if !defined( JSON_INDENT) && !defined(JSON_NEWLINE)
				#ifdef JSON_WRITE_BASH_COMMENTS
				    assertEquals(test3.write_formatted(), JSON_TEXT("{\n\t\n\t#Testing stuff\n\t\"Hi\" : \"There\",\n\t\n\t#Multi\n\t#Line\n\t#Unix and Windows\n\t\"Hello\" : \"World\"\n}"));
				#elif defined(JSON_WRITE_SINGLE_LINE_COMMENTS)
				    assertEquals(test3.write_formatted(), JSON_TEXT("{\n\t\n\t//Testing stuff\n\t\"Hi\" : \"There\",\n\t\n\t//Multi\n\t//Line\n\t//Unix and Windows\n\t\"Hello\" : \"World\"\n}"));
				#else
				    assertEquals(test3.write_formatted(), JSON_TEXT("{\n\t\n\t//Testing stuff\n\t\"Hi\" : \"There\",\n\t\n\t/*\n\t\tMulti\n\t\tLine\n\t\tUnix and Windows\n\t*/\n\t\"Hello\" : \"World\"\n}"));
				#endif
			 #endif

		  #endif
		
          #ifdef JSON_READ_PRIORITY
              used_ascii_one = asciichar;
		      const json_char * str = JSON_TEXT("{ \"mt\":\"\\\"str\\\"\" }"); // str={"mt":"\"str\""} 
              json_string check = libjson::strip_white_space(str);
              assertEquals(check, JSON_TEXT("{\"mt\":\"\\\"str\\\"\"}"));
            
              used_ascii_one = asciichar;
		      JSONNode obj = libjson::parse(str); 
		      json_string objstr = obj.write(); 
              assertEquals(objstr, JSON_TEXT("{\"mt\":\"\\\"str\\\"\"}"));


                UnitTest::SetPrefix("TestWriter.cpp - ascii one char");
        	   used_ascii_one = asciichar;
        	   JSONNode n = libjson::parse(JSON_TEXT("[ \"I said: \\\"foo!\\\"\" ]"));
        	   json_string result = n.write_formatted();
        	   assertEquals(result, JSON_TEXT("[\n\t\"I said: \\\"foo!\\\"\"\n]"));
          #endif
	   #endif
	}

    void TestSuite::TestWriter(void){
	   UnitTest::SetPrefix("TestWriter.cpp - Writing (no ascii one)");
	   myDoTests(false);
	   
	   UnitTest::SetPrefix("TestWriter.cpp - Writing (yes ascii one)");
	   myDoTests(true);
	}
#endif
