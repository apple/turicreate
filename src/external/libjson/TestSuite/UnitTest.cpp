#include "UnitTest.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <cstdlib>

std::vector<std::string> Fails;
std::vector<std::string> All;
bool ReturnOnFail = false;
bool Echo = true;
std::string Prefix;
clock_t started = 0;
#if (!defined(CLOCKS_PER_SEC))
    #define noTimeFormatting
#endif

std::string timing();
std::string timing(){
    clock_t clockticks = clock() - started;
    std::stringstream out;


    if (CLOCKS_PER_SEC == 1000000){
	   if (clockticks < 10000){
		  out << clockticks << " microseconds";
		  return out.str();
	   } else if (clockticks < 10000000){
		  out << clockticks / 1000 << " milliseconds";
		  return out.str();
	   }
	} else if (CLOCKS_PER_SEC == 1000){
	   if (clockticks < 10000){
		  out << clockticks << " milliseconds";
		  return out.str();
	   }
    } else {
	   out << clockticks << " clockticks";
	   return out.str();
    }

    #ifndef noTimeFormatting
		if ((CLOCKS_PER_SEC == 1000000) || (CLOCKS_PER_SEC == 1000)){
		   clock_t seconds = clockticks / CLOCKS_PER_SEC;
		   if (seconds < 60){
			  out << seconds << " seconds";
		   } else if (seconds < 7200) {
			  out << seconds / 60 << " minutes";
		   } else {
			  out << seconds / 3600 << " hours";
		   }
		   return out.str();
		}
    #endif
}

void UnitTest::SelfCheck(void){
    assertTrue(true);
    assertFalse(false);
    assertEquals(1, 1);
    assertNotEquals(1, 0);

    assertGreaterThan(1, 0);
    assertGreaterThanEqualTo(1, 0);
    assertGreaterThanEqualTo(1, 1);

    assertLessThan(0, 1);
    assertLessThanEqualTo(0, 1);
    assertLessThanEqualTo(1, 1);

    assertCStringEquals("Hello", "Hello");
    assertCStringNotEquals("Hello", "World");

    assertCStringEqualsW(L"Hello", L"Hello");
    assertCStringNotEqualsW(L"Hello", L"World");

    std::vector<std::string> exception_Test;
    assertException(std::string res = exception_Test.at(15), std::out_of_range);
}

std::string fix(const std::string & str);
std::string fix(const std::string & str){
    std::string fff(str);
    size_t pos = fff.find('\n');
    while(pos != std::string::npos){
	   fff = fff.substr(0, pos) + "\\n" + fff.substr(pos + 1);
	   pos = fff.find('\n', pos + 1);
    }
    pos = fff.find('\t');
    while(pos != std::string::npos){
	   fff = fff.substr(0, pos) + "\\t" + fff.substr(pos + 1);
	   pos = fff.find('\t', pos + 1);
    }
    pos = fff.find('\r');
    while(pos != std::string::npos){
	   fff = fff.substr(0, pos) + "\\r" + fff.substr(pos + 1);
	   pos = fff.find('\r', pos + 1);
    }
    pos = fff.find('\"');
    while(pos != std::string::npos){
	   fff = fff.substr(0, pos) + "\\\"" + fff.substr(pos + 1);
	   pos = fff.find('\"', pos + 2);
    }
    return fff;
}

void UnitTest::PushFailure(const std::string & fail){
    Fails.push_back(fail);
    if (test_likely(Echo)) std::cout << fail << std::endl;
    All.push_back(std::string("<b style=\"color:#000000;background:#FF0000\">") + fail + "</b><br>");
}

void UnitTest::PushSuccess(const std::string & pass){
    All.push_back(std::string("<b style=\"color:#000000;background:#00FF00\">") + pass + "</b><br>");
}

std::string UnitTest::ToString(void){
    std::stringstream out;
    out << "Number of failed tests: " << Fails.size();
    std::string result(out.str());
    for(std::vector<std::string>::iterator it = Fails.begin(), end = Fails.end(); it != end; ++it){
	   result += *it;
	   result += "\n";
    }
    return result;
}

std::string UnitTest::ToHTML(void){
    std::string result("<html><head><title>Test Suite Results</title></head><body><a style=\"font-size:14\">");
    std::stringstream out;
    out << "Passed Tests: <c style=\"color:#00CC00\">" << All.size() - Fails.size() << "</c><br>Failed Tests: <c style=\"color:#CC0000\">" << Fails.size() << "</c><br>Total Tests: " << All.size() << "<br>";
    if (test_likely(started)){
	   out << "Elapsed time: " << timing() << "<br><br>";
    } else {
	   out << "<br>";
    }
    result += out.str();
    for(std::vector<std::string>::iterator it = All.begin(), end = All.end(); it != end; ++it){
	   result += *it;
    }
    return result + "</a></body></html>";
}

#include <iostream>
#include <cstdio>
void UnitTest::SaveTo(const std::string & location){
    FILE * fp = fopen(location.c_str(), "w");
    if (test_likely(fp != 0)){
	   std::string html(ToHTML());
	   fwrite(html.c_str(), html.length(), 1, fp);
	   fclose(fp);
	   system("pwd");
		std::cout << "Saved file to " << location << std::endl;
    } else {
        std::cout << "Couldn't save file" << std::endl;
    }
	
	if (test_likely(Echo)) std::cout << "Passed tests: " << All.size() - Fails.size() << std::endl << "Failed tests: " << Fails.size() << std::endl;
}

bool UnitTest::GetReturnOnFail(void){ return ReturnOnFail; }
void UnitTest::SetReturnOnFail(bool option){ ReturnOnFail = option; }
void UnitTest::SetEcho(bool option){ Echo = option; }
void UnitTest::SetPrefix(const std::string & prefix){
    std::cout << prefix << std::endl;
    Prefix = prefix;
}
std::string UnitTest::GetPrefix(void){ return Prefix; }
void UnitTest::echo_(const std::string & out){
    All.push_back(fix(out) + "<br>");
    std::cout << out << std::endl;
}

void UnitTest::StartTime(void){
    started = clock();
}
