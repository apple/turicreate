#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sys/stat.h> 
#include "../UnitTest.h"

using namespace std;

map<string, string> options;
vector<string> lines;
vector<unsigned int> line_numbers;
size_t counter = 0;
string make;
string ArchivedOptions;
string makeStyle;

string makeOptions[] = {
    "single",
    "debug",
    "small"
};

void makeMap(void){
    options["LIBRARY"] = "#define JSON_LIBRARY";
    options["DEBUG"] = "#define JSON_DEBUG";
    options["STREAM"] = "#define JSON_STREAM";
    options["SAFE"] = "#define JSON_SAFE";
    options["STDERROR"] = "#define JSON_STDERROR";
    options["PREPARSE"] = "#define JSON_PREPARSE";
    options["LESS_MEMORY"] = "#define JSON_LESS_MEMORY";
    options["UNICODE"] = "#define JSON_UNICODE";
    options["REF_COUNT"] = "#define JSON_REF_COUNT";
    options["BINARY"] = "#define JSON_BINARY";
    options["MEMORY_CALLBACKS"] = "#define JSON_MEMORY_CALLBACKS";
    options["MEMORY_MANAGE"] = "#define JSON_MEMORY_MANAGE";
    options["MUTEX_CALLBACKS"] = "#define JSON_MUTEX_CALLBACKS";
    options["MUTEX_MANAGE"] = "#define JSON_MUTEX_MANAGE";
    options["ITERATORS"] = "#define JSON_ITERATORS";
    options["WRITER"] = "#define JSON_WRITE_PRIORITY MID";
    options["READER"] = "#define JSON_READ_PRIORITY HIGH";
    options["NEWLINE"] = "#define JSON_NEWLINE \"\\r\\n\"";
    options["COMMENTS"] = "#define JSON_COMMENTS";
    options["INDENT"] = "#define JSON_INDENT \"    \"";
    options["WRITE_BASH_COMMENTS"] = "#define JSON_WRITE_BASH_COMMENTS";
    options["WRITE_SINGLE_LINE_COMMENTS"] = "#define JSON_WRITE_SINGLE_LINE_COMMENTS";
    options["VALIDATE"] = "#define JSON_VALIDATE";
    options["UNIT_TEST"] = "#define JSON_UNIT_TEST";
    options["INDEX_TYPE"] = "#define JSON_INDEX_TYPE unsigned int";  
    options["CASE_INSENSITIVE_FUNCTIONS"] = "#define JSON_CASE_INSENSITIVE_FUNCTIONS";
    options["ESCAPE_WRITES"] = "#define JSON_ESCAPE_WRITES";
    options["STRINGU_HEADER"] = "#define JSON_STRING_HEADER \"../TestSuite/UStringTest.h\"";
    options["STRING_HEADER"] = "#define JSON_STRING_HEADER \"../TestSuite/StringTest.h\"";
	options["CASTABLE"] = "#define JSON_CASTABLE";
	options["STRICT"] = "#define JSON_STRICT";
	options["MEMORY_POOL"] = "#define JSON_MEMORY_POOL 524288";
}

void testRules(unsigned int i){
    remove("./testapp");
    int q = system(make.c_str());
    bool Archive = false;
    if (FILE * fp = fopen("./testapp", "r")){
	   fclose(fp);
	   
	   remove("./out.html");
	   q = system("./testapp");
	   if (FILE * fp = fopen("./out.html", "r")){
		  char buffer[255];
		  size_t qq = fread(&buffer[0], 255, 1, fp);
		  buffer[254] = '\0';
		  fclose(fp);
		  string buf(&buffer[0]);
		  size_t pos = buf.find("Failed Tests: <c style=\"color:#CC0000\">");
		  if (pos == string::npos){
			 FAIL("Something Wrong");
		  } else {
			 if(buf[pos + 39] == '0'){
				PASS("GOOD");
			 } else {
				size_t pp = buf.find('<', pos + 39);
				FAIL(string("Didn't pass ") +  buf.substr(pos + 39, pp - pos - 39) +  " tests");
				ArchivedOptions = std::string("Fail_") + ArchivedOptions;
				Archive = true;
			 }
		  }
	   } else {
		  FAIL("Running crashed");
		  ArchivedOptions = std::string("Crashed_") + ArchivedOptions;
		  Archive = true;
	   }   
    } else {
	   FAIL(string("Compilation failed - ") + lines[i]);
	   ArchivedOptions = std::string("Compile_") + ArchivedOptions;
	   Archive = true;
    }

	//If something broke, make a copy of the options used to make the failure, so it can be easily retested
 	if (Archive){
		if (FILE * fp = fopen("../JSONOptions.h", "r")){
			ArchivedOptions = std::string("../") + ArchivedOptions;
			if (FILE * ofp = fopen(ArchivedOptions.c_str(), "w")){
			     char buffer[2048] = {'\0'};
			  	size_t qq = fread(&buffer[0], 2048, 1, fp);
				fwrite(&buffer[0], strlen(&buffer[0]), 1, ofp);
				fclose(ofp);
			}
			fclose(fp);
		}
	}
}

bool makeTempOptions(unsigned int i){
    string & line = lines[i];
    
    if (FILE * fp = fopen("../JSONOptions.h", "w")){
	   string res("#ifndef JSON_OPTIONS_H\n#define JSON_OPTIONS_H\n");
	   for (
		   map<string, string>::iterator runner = options.begin(), end = options.end();
		   runner != end;
		   ++runner){
		  
		  if (line.find(runner -> first) != string::npos){
			 res += runner -> second + "\n";
		  }
	   }
	   res += "#endif\n";
	   
	   fwrite(res.c_str(), res.length(), 1, fp);
	   fclose(fp);
	   return true;
    }
    return false;
}

bool hideGoodOptions(void){
    struct stat stFileInfo; 
    if (stat("../__JSONOptions.h", &stFileInfo)){
	remove("../JSONOptions.h");
	return true;
    }
    return (rename("../JSONOptions.h", "../__JSONOptions.h") == 0);
}

bool loadTests(){
    ifstream infile("All/Options.txt");
    
    if (!infile){
	   return false;
    }
    
    string line;
    unsigned int iii = 0;
    while (getline(infile, line)){
	   ++iii;
	   size_t pos = line.find_first_not_of(' ');
	   if (pos != string::npos){
		  line = line.substr(pos);
		  pos = line.find_first_not_of("\r\n\t ");
		  if ((line.length() > 5) && (line[0] != '#')){
			 const string temp(line.substr(pos));
			 lines.push_back(string("READER, ") + temp);
			 line_numbers.push_back(iii);
			 if ((temp.find("VALIDATE") == string::npos) && (temp.find("STREAM") == string::npos)){
				lines.push_back(temp);
				line_numbers.push_back(iii);
			 }
		  }
	   }
    }
    infile.close();
    return true;
}

void RunTest(const std::string & version, unsigned int i){
	if(makeTempOptions(i)){
		stringstream mystream;
		mystream << version << " Line " << line_numbers[i];
		cout << "Compiling " << ++counter << " of " << line_numbers.size() * 3 <<  " - " << mystream.str() << endl;
		cout << "     " << lines[i] << endl;
		UnitTest::SetPrefix(mystream.str());
		stringstream options_;
		options_ << version << "_Line_" << line_numbers[i] << "_JSONOptions.h";
		ArchivedOptions = options_.str();
		testRules(i);
		remove("../JSONOptions.h");
		UnitTest::SaveTo("progress.html");
	}
}

void Go(const std::string & version, unsigned int test){
    echo(make);
    if (makeStyle.empty() || (makeStyle == version)){
	   makeStyle.clear();
	   for (unsigned int i = test; i < lines.size(); ++i){   
		  RunTest(version, i);
	   }
    } else {
	   echo("skipping");
    }
}


void RunTests(unsigned int test){
    if (hideGoodOptions()){
	   if(loadTests()){
		  makeMap();
		 for(unsigned int i = 0; i < sizeof(makeOptions); ++i){
			make = "make -j4 " + makeOptions[i];
			Go(makeOptions[i], test);
		 }
	   } else {
		  FAIL("couldn't open options");  
	   }
	   rename("../__JSONOptions.h", "../JSONOptions.h");
    } else {
	   FAIL("Couldn't protect JSONOptions");
    }   
}

int main (int argc, char * const argv[]) { 
    UnitTest::StartTime();
    unsigned int test = 0;
    if (argc == 3){
	   test = atoi(argv[2]) - 1;
	   counter = test;
	   echo("starting on test " << test);
	   makeStyle = argv[1];
	   echo("starting with make " << makeStyle);
    } else if (argc == 2){
	   test = 0;
	   counter = test;
	   echo("starting on test " << test);
	   makeStyle = argv[1];
	   echo("starting with make " << makeStyle);
    }
    
    RunTests(test);

    UnitTest::SaveTo("out.html");
    return 0;
}
