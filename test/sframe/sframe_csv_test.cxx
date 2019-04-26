#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <typeinfo>
#include <boost/filesystem.hpp>
#include <sframe/sframe.hpp>
#include <sframe/algorithm.hpp>
#include <sframe/csv_writer.hpp>
#include <flexible_type/flexible_type.hpp>
#include <flexible_type/string_escape.hpp>
#include <sframe/parallel_csv_parser.hpp>
#include <sframe/csv_line_tokenizer.hpp>
#include <flexible_type/string_escape.hpp>
#include <random/random.hpp>

using namespace turi;
struct csv_test {
  csv_line_tokenizer tokenizer;
  bool header = true;
  size_t skip_rows = 0;
  std::string file;
  std::vector<std::vector<flexible_type> > values;
  std::vector<std::pair<std::string, flex_type_enum> > types;
  std::vector<std::string> parse_column_subset;

  bool perform_subset_test = true;
  bool failure_expect = false;
};

csv_test basic(std::string dlm=",", std::string line_ending="\n") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  strm << "float" << dlm << "int" << dlm << "str" << dlm << "vec" << dlm << "dict" << dlm << "rec" << line_ending
       << "1.1" << dlm << "1" << dlm << "one" << dlm << "[1,1,1]" << dlm << "{1:1,\"a\":\"a\"}" << dlm << "[a,a]" << line_ending
       << "2.2" << dlm << "2" << dlm << "two" << dlm << "[2,2,2]" << dlm << "{2:2,\"b\":\"b\"}" << dlm << "[b,b]" << line_ending
       << "3.3" << dlm << "3" << dlm << "three" << dlm << "[3,3,3]" << dlm << "{3:3,\"c\":\"c\"}" << dlm << "[c,c]" << line_ending;
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});

  if (line_ending != "\n" && line_ending != "\r\n" && line_ending != "\r") {
    ret.tokenizer.line_terminator = line_ending;
  }

  ret.types = {{"float", flex_type_enum::FLOAT},
              {"int", flex_type_enum::INTEGER},
              {"str", flex_type_enum::STRING},
              {"vec", flex_type_enum::VECTOR},
              {"dict",flex_type_enum::DICT},
              {"rec",flex_type_enum::LIST}};
  return ret;
}


csv_test basic_comments_and_skips(std::string dlm=",", std::string line_ending="\n") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  strm << "junk" << line_ending;
  strm << "trash" << line_ending;
  strm << " # a commented string" << line_ending;
  strm << "float" << dlm << "int" << dlm << "str" << dlm << "vec" << dlm << "dict" << dlm << "rec" << line_ending
       << "1.1" << dlm << "1" << dlm << "one" << dlm << "[1,1,1]" << dlm << "{1:1,\"a\":\"a\"}" << dlm << "[a,a]" << line_ending
       << "# another commented string" << line_ending
       << "  # yet another commented string" << line_ending
       << "2.2" << dlm << "2" << dlm << "two" << dlm << "[2,2,2]" << dlm << "{2:2,\"b\":\"b\"}" << dlm << "[b,b]" << line_ending
       << "3.3" << dlm << "3" << dlm << "three" << dlm << "[3,3,3]" << dlm << "{3:3,\"c\":\"c\"}" << dlm << "[c,c]" << line_ending;
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});

  if (line_ending != "\n" && line_ending != "\r\n" && line_ending != "\r") {
    ret.tokenizer.line_terminator = line_ending;
    ret.tokenizer.comment_char = '#';
    ret.tokenizer.has_comment_char = true;
  }
  ret.skip_rows = 2;

  ret.types = {{"float", flex_type_enum::FLOAT},
              {"int", flex_type_enum::INTEGER},
              {"str", flex_type_enum::STRING},
              {"vec", flex_type_enum::VECTOR},
              {"dict",flex_type_enum::DICT},
              {"rec",flex_type_enum::LIST}};
  return ret;
}

std::string default_escape_string(std::string s) {
  std::string out;
  size_t outputlength;
  escape_string(s, '\\',true,'\"',true, false, out, outputlength);
  out.resize(outputlength);
  return out;
}

csv_test quoted_basic(std::string dlm=",", std::string line_ending="\n") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  auto esc = default_escape_string;
  strm << esc("float") << dlm << esc("int") << dlm << esc("str") << dlm << esc("vec") << dlm << esc("dict") << dlm << esc("rec") << line_ending
       << esc("1.1") << dlm << esc("1") << dlm << esc("one") << dlm << esc("[1,1,1]") << dlm << esc("{1:1,\"a\":\"a\"}") << dlm << esc("[a,a]") << line_ending
       << esc("2.2") << dlm << esc("2") << dlm << esc("two") << dlm << esc("[2,2,2]") << dlm << esc("{2:2,\"b\":\"b\"}") << dlm << esc("[b,b]") << line_ending
       << esc("3.3") << dlm << esc("3") << dlm << esc("three") << dlm << esc("[3,3,3]") << dlm << esc("{3:3,\"c\":\"c\"}") << dlm << esc("[c,c]") << line_ending;
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});

  if (line_ending != "\n" && line_ending != "\r\n" && line_ending != "\r") {
    ret.tokenizer.line_terminator = line_ending;
  }
  ret.types = {{"float", flex_type_enum::FLOAT},
              {"int", flex_type_enum::INTEGER},
              {"str", flex_type_enum::STRING},
              {"vec", flex_type_enum::VECTOR},
              {"dict",flex_type_enum::DICT},
              {"rec",flex_type_enum::LIST}};
  return ret;
}

csv_test test_type_inference(std::string dlm=",", std::string line_ending="\n") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  strm << "float" << dlm << "int" << dlm << "str" << dlm << "vec" << dlm << "dict" << dlm << "rec" << line_ending
       << "1.1" << dlm << "1" << dlm << "one" << dlm << "[1,1,1]" << dlm << "{1:1,\"a\":\"a\"}" << dlm << "[a,a]" << line_ending
       << "2.2" << dlm << "2" << dlm << "two" << dlm << "[2,2,2]" << dlm << "{2:2,\"b\":\"b\"}" << dlm << "[b,b]" << line_ending
       << "3.3" << dlm << "3" << dlm << "three" << dlm << "[3,3,3]" << dlm << "{3:3,\"c\":\"c\"}" << dlm << "[c,c]" << line_ending;
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});

  if (line_ending != "\n" && line_ending != "\r\n" && line_ending != "\r") {
    ret.tokenizer.line_terminator = line_ending;
  }
  ret.types = {{"float", flex_type_enum::UNDEFINED},
              {"int", flex_type_enum::UNDEFINED},
              {"str", flex_type_enum::UNDEFINED},
              {"vec", flex_type_enum::UNDEFINED},
              {"dict",flex_type_enum::UNDEFINED},
              {"rec",flex_type_enum::UNDEFINED}};
  return ret;
}

csv_test test_quoted_type_inference(std::string dlm=",", std::string line_ending="\n") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  auto esc = default_escape_string;
  strm << "float" << dlm << "int" << dlm << "str" << dlm << "vec" << dlm << "dict" << dlm << "rec" << line_ending
       << esc("1.1") << dlm << esc("1") << dlm << esc("one") << dlm << esc("[1,1,1]") << dlm << esc("{1:1,\"a\":\"a\"}") << dlm << esc("[a,a]") << line_ending
       << esc("2.2") << dlm << esc("2") << dlm << esc("two") << dlm << esc("[2,2,2]") << dlm << esc("{2:2,\"b\":\"b\"}") << dlm << esc("[b,b]") << line_ending
       << esc("3.3") << dlm << esc("3") << dlm << esc("three") << dlm << esc("[3,3,3]") << dlm << esc("{3:3,\"c\":\"c\"}") << dlm << esc("[c,c]") << line_ending;
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});

  if (line_ending != "\n" && line_ending != "\r\n" && line_ending != "\r") {
    ret.tokenizer.line_terminator = line_ending;
  }
  ret.types = {{"float", flex_type_enum::UNDEFINED},
              {"int", flex_type_enum::UNDEFINED},
              {"str", flex_type_enum::UNDEFINED},
              {"vec", flex_type_enum::UNDEFINED},
              {"dict",flex_type_enum::UNDEFINED},
              {"rec",flex_type_enum::UNDEFINED}};
  return ret;
}


csv_test test_embedded_strings(std::string dlm=",") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  strm << "str" << dlm << "vec" << "\n"
       << "[abc" << dlm << "[1,1,1]" << "\n"
       << "cde]" << dlm << "[2,2,2]" << "\n"
       << "a[a]b" << dlm << "[3,3,3]" << "\n"
       << "\"[abc\"" << dlm << "[1,1,1]" << "\n"
       << "\"cde]\"" << dlm << "[2,2,2]" << "\n"
       << "\"a[a]b\"" << dlm << "[3,3,3]" << "\n";
  ret.file = strm.str();
  ret.values.push_back({"[abc", flex_vec{1,1,1}});
  ret.values.push_back({"cde]", flex_vec{2,2,2}});
  ret.values.push_back({"a[a]b", flex_vec{3,3,3}});
  ret.values.push_back({"[abc", flex_vec{1,1,1}});
  ret.values.push_back({"cde]", flex_vec{2,2,2}});
  ret.values.push_back({"a[a]b", flex_vec{3,3,3}});

  ret.types = {{"str", flex_type_enum::STRING},
               {"vec", flex_type_enum::VECTOR}};
  return ret;
}


csv_test test_quoted_embedded_strings(std::string dlm=",") {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = dlm;
  std::stringstream strm;
  auto esc = default_escape_string;
  strm << "str" << dlm << "vec" << "\n"
       << esc("[abc") << dlm << esc("[1,1,1]") << "\n"
       << esc("cde]") << dlm << esc("[2,2,2]") << "\n"
       << esc("a[a]b") << dlm << esc("[3,3,3]") << "\n"
       << esc("[abc") << dlm << esc("[1,1,1]") << "\n"
       << esc("cde]") << dlm << esc("[2,2,2]") << "\n"
       << esc("a[a]b") << dlm << esc("[3,3,3]") << "\n";
  ret.file = strm.str();
  ret.values.push_back({"[abc", flex_vec{1,1,1}});
  ret.values.push_back({"cde]", flex_vec{2,2,2}});
  ret.values.push_back({"a[a]b", flex_vec{3,3,3}});
  ret.values.push_back({"[abc", flex_vec{1,1,1}});
  ret.values.push_back({"cde]", flex_vec{2,2,2}});
  ret.values.push_back({"a[a]b", flex_vec{3,3,3}});

  ret.types = {{"str", flex_type_enum::STRING},
               {"vec", flex_type_enum::VECTOR}};
  return ret;
}

csv_test interesting() {
  csv_test ret;
  std::stringstream strm;
  strm << "#this is a comment\n"
       << "float;int;vec;str #this is another comment\n"
       << "1.1 ;1;[1 2 3];\"hello\\\\\"\n"
       << "2.2;2; [4 5 6];\"wor;ld\"\n"
       << " 3.3; 3;[9 2];\"\"\"w\"\"\"\n" // double quote
       << "Pokemon  ;;; NA "; // missing last value
  ret.file = strm.str();
  ret.tokenizer.delimiter = ";";
  ret.tokenizer.double_quote = true;
  ret.tokenizer.na_values = {"NA","Pokemon",""};

  ret.values.push_back({1.1,1,flex_vec{1,2,3},"hello\\"});
  ret.values.push_back({2.2,2,flex_vec{4,5,6},"wor;ld"});
  ret.values.push_back({3.3,3,flex_vec{9,2},"\"w\""});
  ret.values.push_back({flex_undefined(), flex_undefined(), flex_undefined(), flex_undefined()});

  ret.types = {{"float", flex_type_enum::FLOAT},
               {"int", flex_type_enum::INTEGER},
               {"vec", flex_type_enum::VECTOR},
               {"str", flex_type_enum::STRING}};
  return ret;
}

csv_test excess_white_space() {
  // tests a basic parse of one of every CSV parseable type
  csv_test ret;
  ret.tokenizer.delimiter = " "; 
  std::stringstream strm;
  std::string dlm = " ";
  // interestingly.... we do not correctly handle excess spaces in the header?
  strm << "float" << dlm << "int" << dlm << "str " << dlm << "vec   " << dlm << "dict" << dlm << "rec" << "\n" 
       << "  1.1" << dlm << " 1" << dlm << "one  " << dlm << "[1,1,1] " << dlm << " {1 : 1 , \"a\"  : \"a\"}   " << dlm << "[a,a]" << "\n" 
       << " 2.2" << dlm << "2" << dlm << "two" << dlm << "  [2,2,2]" << dlm << "{2:2,\"b\":\"b\"}" << dlm << "[b,b]" << "\n" 
       << "3.3" << dlm << "3" << dlm << "three" << dlm << "[3,3,3]" << dlm << " {3:3,  \"c\":\"c\"}" << dlm << "[c,c]  \t" << "\n";
  ret.file = strm.str();
  ret.values.push_back({1.1,1,"one",flex_vec{1,1,1},flex_dict{{1,1},{"a","a"}},flex_list{"a","a"}});
  ret.values.push_back({2.2,2,"two",flex_vec{2,2,2},flex_dict{{2,2},{"b","b"}},flex_list{"b","b"}});
  ret.values.push_back({3.3,3,"three",flex_vec{3,3,3},flex_dict{{3,3},{"c","c"}},flex_list{"c","c"}});
  ret.types = {{"float", flex_type_enum::FLOAT},
              {"int", flex_type_enum::INTEGER},
              {"str", flex_type_enum::STRING},
              {"vec", flex_type_enum::VECTOR},
              {"dict",flex_type_enum::DICT},
              {"rec",flex_type_enum::LIST}};
  // this test actually does not stand up to subsetting. The reason
  // is that if the dict column is not selected in the subset, we are unaware 
  // that it is a dict column and will try to slice it based on the space 
  // separators in it, and that will implode. It is not clear that there is
  // a good strategy here...
  // The problem is in getting this to behave, as well as issue 1514.
  // (see test below, or the wierd bracketing test below).
  //
  // The "correct solution" is to both select the columns you want, AND
  // provide the type hints even for columns you do not want. But that requires
  // extending the csv parser in some messy ways.
  ret.perform_subset_test = false;
  return ret;
}

csv_test wierd_bracketing_thing() {
  csv_test ret;
  std::stringstream strm;
  strm << "str1 str2 str3\n"
       << "{    }    }\n"
       << "[    :    ]\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = " ";
  ret.tokenizer.double_quote = false;

  ret.values.push_back({"{","}","}"});
  ret.values.push_back({"[",":","]"});
  ret.values.push_back({flex_undefined(), flex_undefined(), flex_undefined(), flex_undefined()});

  ret.types = {{"float", flex_type_enum::FLOAT},
               {"int", flex_type_enum::INTEGER},
               {"vec", flex_type_enum::VECTOR},
               {"str", flex_type_enum::STRING}};
  return ret;
}

csv_test test_na_values() {
  csv_test ret;
  std::stringstream strm;
  strm << "a,b,c\n"
       << "NA,PIKA,CHU\n"
       << "1.0,2,3\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.na_values = {"NA","PIKA","CHU"};

  ret.values.push_back({flex_undefined(), flex_undefined(), flex_undefined()});
  ret.values.push_back({1.0,2,3});

  ret.types = {{"a", flex_type_enum::FLOAT},
               {"b", flex_type_enum::INTEGER},
               {"c", flex_type_enum::INTEGER}};
  return ret;
}

csv_test test_na_values2() {
  csv_test ret;
  std::stringstream strm;
  strm << "k,v\n"
       << "a,1\n"
       << "b,1\n"
       << "c,-8\n"
       << "d,3\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.na_values = {"-8"};

  ret.values.push_back({"a", 1});
  ret.values.push_back({"b", 1});
  ret.values.push_back({"c", flex_undefined()});
  ret.values.push_back({"d", 3});

  ret.types = {{"k", flex_type_enum::STRING},
               {"v", flex_type_enum::INTEGER}};
  return ret;
}


csv_test test_true_values() {
  csv_test ret;
  std::stringstream strm;
  strm << "k,v\n"
       << "a,1\n"
       << "b,1\n"
       << "c,-8\n"
       << "d,3\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.true_values = {"-8"};

  ret.values.push_back({"a", 1});
  ret.values.push_back({"b", 1});
  ret.values.push_back({"c", 1});
  ret.values.push_back({"d", 3});

  ret.types = {{"k", flex_type_enum::STRING},
               {"v", flex_type_enum::INTEGER}};
  return ret;
}


csv_test test_false_values() {
  csv_test ret;
  std::stringstream strm;
  strm << "k,v\n"
       << "a,1\n"
       << "b,1\n"
       << "c,-8\n"
       << "d,3\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.false_values = {"-8"};

  ret.values.push_back({"a", 1});
  ret.values.push_back({"b", 1});
  ret.values.push_back({"c", 0});
  ret.values.push_back({"d", 3});

  ret.types = {{"k", flex_type_enum::STRING},
               {"v", flex_type_enum::INTEGER}};
  return ret;
}

csv_test test_substitutions_raw_string_matches1() {
  csv_test ret;
  std::stringstream strm;
  strm << "k,v\n"
       << "\"true\",true\n"
       << "\"false\",false\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.true_values = {"true"};
  ret.tokenizer.false_values = {"false"};
  ret.tokenizer.only_raw_string_substitutions = true;

  ret.values.push_back({"true", 1});
  ret.values.push_back({"false", 0});

  ret.types = {{"k", flex_type_enum::STRING},
               {"v", flex_type_enum::INTEGER}};
  return ret;
}

csv_test test_substitutions_raw_string_matches2() {
  csv_test ret;
  std::stringstream strm;
  strm << "k,v\n"
       << "\"true\",true\n"
       << "\"false\",false\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.true_values = {"\"true\""};
  ret.tokenizer.false_values = {"\"false\""};
  ret.tokenizer.only_raw_string_substitutions = true;

  ret.values.push_back({1, "true"});
  ret.values.push_back({0, "false"});

  ret.types = {{"k", flex_type_enum::INTEGER},
               {"v", flex_type_enum::STRING}};
  return ret;
}

csv_test test_missing_tab_values() {
  csv_test ret;
  std::stringstream strm;
  strm << "a\tb\tc\n"
       << "1\t\t  b\n"
       << "2\t\t\n"
       << "3\t  c\t d \n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = "\t";
  ret.values.push_back({1, flex_undefined(), "b"});
  ret.values.push_back({2, flex_undefined(), flex_undefined()});
  ret.values.push_back({3, "c","d"});

  ret.types = {{"a", flex_type_enum::UNDEFINED},
               {"b", flex_type_enum::UNDEFINED},
               {"c", flex_type_enum::UNDEFINED}};
  return ret;
}


csv_test another_wierd_bracketing_thing_issue_1514() {
  csv_test ret;
  std::stringstream strm;
  strm << "X1\tX2\tX3\tX4\tX5\tX6\tX7\tX8\tX9\n"
       << "1\t{\t()\t{}\t{}\t(}\t})\t}\tdebugging\n"
       << "3\t--\t({})\t{()}\t{}\t({\t{)\t}\tdebugging\n";

  ret.file = strm.str();
  ret.tokenizer.delimiter = "\t";

  ret.values.push_back({"1","{","()","{}","{}","(}","})","}","debugging"});
  ret.values.push_back({"3","--","({})","{()}","{}","({","{)","}","debugging"});

  ret.types = {{"X1", flex_type_enum::STRING},
               {"X2", flex_type_enum::STRING},
               {"X3", flex_type_enum::STRING},
               {"X4", flex_type_enum::STRING},
               {"X5", flex_type_enum::STRING},
               {"X6", flex_type_enum::STRING},
               {"X7", flex_type_enum::STRING},
               {"X8", flex_type_enum::STRING},
               {"X9", flex_type_enum::STRING}};
  return ret;
}


csv_test string_integers() {
  csv_test ret;
  std::stringstream strm;
  strm << "int,str\n"
       << "1,\"\"\"1\"\"\"\n"
       << "2,\"\\\"2\\\"\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.double_quote=true;

  ret.values.push_back({1,"\"1\""});
  ret.values.push_back({2,"\"2\""});

  ret.types = {{"int", flex_type_enum::UNDEFINED},
               {"str", flex_type_enum::UNDEFINED}};
  return ret;
}


csv_test string_integers2() {
  csv_test ret;
  std::stringstream strm;
  strm << "int,str\n"
       << "1,\"1\"\n"
       << "2,\"2\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.double_quote=true;

  ret.values.push_back({1,1});
  ret.values.push_back({2,2});

  ret.types = {{"int", flex_type_enum::UNDEFINED},
               {"str", flex_type_enum::UNDEFINED}};
  return ret;
}


csv_test newline_in_strings() {
  csv_test ret;
  std::stringstream strm;
  strm << "int,str\n"
       << "1,\"a\nb\"\n"
       << "2,\"c\nd\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.double_quote=true;

  ret.values.push_back({1,"a\nb"});
  ret.values.push_back({2,"c\nd"});

  ret.types = {{"int", flex_type_enum::UNDEFINED},
               {"str", flex_type_enum::UNDEFINED}};
  return ret;
}

csv_test newline_in_strings2() {
  csv_test ret;
  std::stringstream strm;
  strm << "int,str\n"
       << "1,\"a\"\"\\\"\\n\n#123\nb\"\n" // "a""\"\n
                                        // #123
                                        // b"
       << "2,\"c\nd\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.double_quote=true;
  ret.tokenizer.has_comment_char=true;
  ret.tokenizer.comment_char='#';

  ret.values.push_back({1,"a\"\"\n\n#123\nb"});
  ret.values.push_back({2,"c\nd"});

  ret.types = {{"int", flex_type_enum::UNDEFINED},
               {"str", flex_type_enum::UNDEFINED}};
  return ret;
}

csv_test newline_in_strings3() {
  csv_test ret;
  std::stringstream strm;
  strm << "int,str\n"
       << "1,\"a\"\"\\\"\\n\n#123\nb\"\n" // "a""\"\n
                                        // #123
                                        // b"
       << "#IGNORE THIS\n"
       << "2,\"c\nd\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = ",";
  ret.tokenizer.double_quote=true;
  ret.tokenizer.has_comment_char=true;
  ret.tokenizer.comment_char='#';

  ret.values.push_back({1,"a\"\"\n\n#123\nb"});
  ret.values.push_back({2,"c\nd"});

  ret.types = {{"int", flex_type_enum::UNDEFINED},
               {"str", flex_type_enum::UNDEFINED}};
  return ret;
}



csv_test alternate_endline_test() {
  csv_test ret;
  std::stringstream strm;
  strm << "a b czzz 1 2 3zzz\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = " ";
  ret.tokenizer.line_terminator = "zzz";

  ret.values.push_back({1,2,3});

  ret.types = {{"a", flex_type_enum::UNDEFINED},
               {"b", flex_type_enum::UNDEFINED},
               {"c", flex_type_enum::UNDEFINED}};
  return ret;
}



csv_test incorrectly_quoted_1() {
  csv_test ret;
  std::stringstream strm;
  strm << "a, b";
  strm << "\"a\", \"b\"";
  strm << "\"a\", \"b";
  strm << "\"a\", \"b\"";
  ret.file = strm.str();
  ret.failure_expect = true;
  return ret;
}



csv_test escape_parsing() {
  csv_test ret;
  std::stringstream strm;
  strm << "str1 str2\n"
       << "\"\\n\"  \"\\n\"\n"
       << "\"\\t\"  \"\\0abf\"\n"
       << "\"\\\"a\"  \"\\\"b\"\n"
       << "{\"a\":\"\\\"\"} [a,\"b\",\"\\\"c\"]\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = " ";

  ret.values.push_back({"\n","\n"});
  ret.values.push_back({"\t","\\0abf"});
  ret.values.push_back({"\"a","\"b"});
  ret.values.push_back({flex_dict({{"a","\""}}),flex_list({"a","b","\"c"})});

  ret.types = {{"str1", flex_type_enum::UNDEFINED},
               {"str2", flex_type_enum::UNDEFINED}};
  return ret;
}

csv_test escape_parsing_string_hint() {
  csv_test ret;
  std::stringstream strm;
  strm << "str1 str2\n"
       << "\"\\n\"  \"\\n\"\n"
       << "\"\\t\"  \"\\0abf\"\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = " ";

  ret.values.push_back({"\n","\n"});
  ret.values.push_back({"\t","\\0abf"});

  ret.types = {{"str1", flex_type_enum::STRING},
               {"str2", flex_type_enum::STRING}};
  return ret;
}


csv_test non_escaped_parsing() {
  csv_test ret;
  std::stringstream strm;
  strm << "str1 str2\n"
       << "\\n  \\n\n"
       << "\\t  \\0abf\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = " ";

  ret.values.push_back({"\\n","\\n"});
  ret.values.push_back({"\\t","\\0abf"});

  ret.types = {{"str1", flex_type_enum::STRING},
               {"str2", flex_type_enum::STRING}};
  return ret;
}


csv_test single_string_column() {
  csv_test ret;
  std::stringstream strm;
  strm << "str1\n"
       << "\"\"\n"
       << "{\"a\":\"b\"}\n"
       << "{\"\":\"\"}\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = "\n";

  ret.values.push_back({""});
  ret.values.push_back({"{\"a\":\"b\"}"});
  ret.values.push_back({"{\"\":\"\"}"});

  ret.types = {{"str1", flex_type_enum::STRING}};
  return ret;
}

csv_test unicode_surrogate_pairs() {
  csv_test ret;
  std::stringstream strm;
  strm << "dict\n";
  strm << "{\"good_surrogates\": \"\\uD834\\uDD1E\"}\n";
  strm << "{\"bad_surrogates\": \"\\uD834\u2019\"}";
  strm << "{\"bad_surrogates2\": \"\\uD834\" }";
  strm << "{\"bad_surrogates3\": \"\\uD834\\uDD\" }";
  strm << "{\"bad_json\": \"\\u442G\" }";
  ret.file = strm.str();
  ret.tokenizer.delimiter = "\n";
  ret.values.push_back({flex_dict{{"good_surrogates", "𝄞"}}});
  // that quote there is a special apostrophe character inserted by some 
  // text editors when you type it's
  ret.values.push_back({flex_dict{{"bad_surrogates", "\\uD834’"}}}); 
  ret.values.push_back({flex_dict{{"bad_surrogates2", "\\uD834"}}});
  ret.values.push_back({flex_dict{{"bad_surrogates3", "\\uD834\\uDD"}}});
  ret.values.push_back({flex_dict{{"bad_json", "\\u442G"}}});

  ret.types = {{"dict", flex_type_enum::DICT}};
  return ret;
}

csv_test multiline_json() {
  csv_test ret;
  std::stringstream strm;
  strm << R"({
       "glossary": 123,
       "fish": 456
        })";
  ret.file = strm.str();
  ret.tokenizer.delimiter = "";
  ret.tokenizer.line_terminator = "";
  ret.header = false;
  ret.values.push_back({flex_dict{{"glossary", 123}, {"fish", 456}}});
  ret.types = {{"X1", flex_type_enum::DICT}};
  return ret;
}

csv_test tab_delimited_csv_with_list() {
  csv_test ret;
  std::stringstream strm;
  strm << "xxx\t[1,2,3]\t[1,2,3]\n";
  ret.file = strm.str();
  ret.tokenizer.delimiter = "\t";
  ret.header = false;

  ret.values.push_back({"xxx", flex_list{1,2,3}, flex_list{1,2,3}});

  ret.types = {{"X1", flex_type_enum::STRING},
               {"X2", flex_type_enum::LIST},
               {"X3", flex_type_enum::LIST}};
  return ret;
}

struct test_equality_visitor {
  template <typename T, typename U>
  void operator()(T& t, const U& u) const { TS_FAIL("type mismatch"); }
  void operator()(flex_image t, flex_image u) const { TS_FAIL("Cannot compare images"); }
  void operator()(flex_undefined t, flex_undefined u) const { }
  void operator()(flex_int t, flex_int u) const { TS_ASSERT_EQUALS(t, u); }
  void operator()(flex_float t, flex_float u) const { TS_ASSERT_DELTA(t, u, 1E-5); }
  void operator()(flex_string t, flex_string u) const { TS_ASSERT_EQUALS(t, u); }
  void operator()(flex_date_time t, flex_date_time u) const { 
    TS_ASSERT_EQUALS(t.posix_timestamp(), u.posix_timestamp()); 
    TS_ASSERT_EQUALS(t.time_zone_offset(), u.time_zone_offset()); 
    TS_ASSERT_EQUALS(t.microsecond(), u.microsecond()); 
  }
  void operator()(flex_vec t, flex_vec u) const { 
    TS_ASSERT_EQUALS(t.size(), u.size());
    for (size_t i = 0;i < t.size(); ++i) {
      TS_ASSERT_DELTA(t[i], u[i], 1E-5);
    }
  }
  void operator()(flex_list t, flex_list u) const { 
    TS_ASSERT_EQUALS(t.size(), u.size());
    for (size_t i = 0;i < t.size(); ++i) {
      t[i].apply_visitor(*this, u[i]);
    }
  }
  void operator()(flex_dict t, flex_dict u) const { 
    TS_ASSERT_EQUALS(t.size(), u.size());
    for (size_t i = 0;i < t.size(); ++i) {
      t[i].first.apply_visitor(*this, u[i].first);
      t[i].second.apply_visitor(*this, u[i].second);
    }
  }
};

struct sframe_test  {
 public:
   // helper for the test below.
   // Makes sure the parsed data matches up with the file, then 
   // writes the file to a new csv file and attempts to parse it again.
   void evaluate(const csv_test& data) {
     std::string filename = get_temp_name() + ".csv";
     std::ofstream fout(filename);
     fout << data.file;
     fout.close();
     auto frame = validate_file(data, filename);
     if (data.failure_expect) return;

     if (data.perform_subset_test) {
       // try random column subsets
       srand(12345);
       std::vector<std::string> colnames;
       for (auto& entry: data.types) colnames.push_back(entry.first);
       std::random_shuffle(colnames.begin(), colnames.end());
       if (colnames.size() > 1) colnames.resize(colnames.size() / 2);
       csv_test subset_test = make_csv_test_subset(data, colnames);
       validate_file(subset_test, filename);
     }
     // write the frame out as csv and read it back in
     // reset the parser configuration and the column configuration.
     csv_test new_test;
     new_test.values = data.values;
     new_test.types = data.types;
     new_test.tokenizer.na_values = data.tokenizer.na_values;
     csv_writer writer;
     writer.double_quote = false;
     filename = get_temp_name() + ".csv";
     frame.save_as_csv(filename, writer);
     validate_file(new_test, filename);
   }

   template <typename T>
   T permute(T arr, const std::vector<size_t>& order) {
     size_t outsize = 0;
     for (size_t i = 0;i < order.size(); ++i) {
       if (order[i] != (size_t)(-1)) outsize = std::max(outsize, order[i]);
     }
     T ret;
     ret.resize(outsize + 1);
     for (size_t i = 0;i < arr.size(); ++i) {
       if (order[i] != (size_t)(-1)) ret[order[i]] = arr[i];
     }
     return ret;
   }
   csv_test make_csv_test_subset(csv_test data, 
                               std::vector<std::string> column_subset) {
     data.parse_column_subset = column_subset;
     // permute order is the same length as the input
     // permute_order[i] is the output column for input column i
     // if permute_order[i] == -1, the column is dropped
     std::vector<size_t> permute_order(data.types.size(), (size_t)(-1));
     for (size_t i = 0;i < column_subset.size(); ++i) {
       for (size_t col = 0;col < data.types.size(); ++col) {
         if (data.types[col].first == column_subset[i]) {
           permute_order[col] = i;
           break;
         }
       }
     }
     // erase the column from all the test data
     data.types = permute(data.types, permute_order);
     for (auto& row: data.values) row = permute(row, permute_order);
     return data;
   }

   sframe validate_file(const csv_test& data, 
                        std::string filename) {
     csv_line_tokenizer tokenizer = data.tokenizer;
     tokenizer.init();
     sframe frame;
     std::map<std::string, flex_type_enum> typelist(data.types.begin(), data.types.end());
  
     frame.init_from_csvs(filename,
                          tokenizer,
                          data.header,
                          false, // continue on failure
                          false,  // do not store errors
                          typelist,
                          data.parse_column_subset,
                          0, // row limit
                          data.skip_rows);
     if (data.failure_expect) {
       TS_ASSERT_EQUALS(frame.num_rows(), 0);
       return frame;
     }

     TS_ASSERT_EQUALS(frame.num_rows(), data.values.size());
     TS_ASSERT_EQUALS(frame.num_columns(), data.types.size());
     for (size_t i = 0;i < data.types.size(); ++i) {
       TS_ASSERT_EQUALS(frame.column_name(i), data.types[i].first);
       TS_ASSERT_EQUALS(frame.column_type(i), data.types[i].second);
     }

     std::vector<std::vector<flexible_type> > vals;
     turi::copy(frame, std::inserter(vals, vals.end()));

     TS_ASSERT_EQUALS(vals.size(), data.values.size());
     for (size_t i = 0;i < vals.size(); ++i) {
       TS_ASSERT_EQUALS(vals[i].size(), data.values[i].size());
       for (size_t j = 0; j < vals[i].size(); ++j) {
         vals[i][j].apply_visitor(test_equality_visitor(), data.values[i][j]);
       }
     }
     return frame;
   }

   void test_string_escaping() {
     std::string s = "hello";
     unescape_string(s, true, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "hello");

     s = "\\\"world\\\"";
     unescape_string(s, true, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "\"world\"");

     s = "\\\\world\\\\";
     unescape_string(s, true, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "\\world\\");

     s = "\\";
     unescape_string(s, true, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "\\");

     s = "\\\"\"\"a\\\"\"\"";
     unescape_string(s, true, '\\', '\"', true);
     TS_ASSERT_EQUALS(s, "\"\"a\"\"");

     s = "\\\'\\\"\\\\\\/\\b\\r\\n";
     unescape_string(s, true, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "\'\"\\/\b\r\n");

     s = "\\world\\";
     unescape_string(s, false, '\\', '\"', false);
     TS_ASSERT_EQUALS(s, "\\world\\");
   }
   void test_substitutions() {
     evaluate(test_na_values());
     evaluate(test_na_values2());
     evaluate(test_true_values());
     evaluate(test_false_values());
     evaluate(test_substitutions_raw_string_matches1());
     evaluate(test_substitutions_raw_string_matches2());
   }

   void test_csvs() {
     evaluate(basic_comments_and_skips());
     evaluate(basic());
     evaluate(basic(",", "\r"));
     evaluate(basic(",", "\r\n"));
     evaluate(basic(",", "abc"));
     evaluate(basic(",", "aaaaaa"));
     evaluate(basic(" "));
     evaluate(basic(" ", "\r"));
     evaluate(basic(" ", "\r\n"));
     evaluate(basic(" ", "abc"));
     evaluate(basic(" ", "bbbbbb"));
     evaluate(basic(";"));
     evaluate(basic(";", "\r"));
     evaluate(basic(";", "\r\n"));
     evaluate(basic(";", "pokemon"));
     evaluate(basic("::"));
     evaluate(basic("  "));
     evaluate(basic("\t\t"));
     evaluate(interesting());
     evaluate(excess_white_space());
     evaluate(test_embedded_strings(","));
     evaluate(test_embedded_strings(" "));
     evaluate(test_embedded_strings("\t"));
     evaluate(test_embedded_strings("\t\t"));
     evaluate(test_embedded_strings("  "));
     evaluate(test_embedded_strings("::"));
     evaluate(another_wierd_bracketing_thing_issue_1514());
     evaluate(test_type_inference(","));
     evaluate(test_type_inference(",", "zzz"));
     evaluate(string_integers());
     evaluate(string_integers2());
     evaluate(newline_in_strings());
     evaluate(newline_in_strings2());
     evaluate(newline_in_strings3());
     evaluate(escape_parsing());
     evaluate(escape_parsing_string_hint());
     evaluate(non_escaped_parsing());
     evaluate(single_string_column());
     evaluate(test_missing_tab_values());
     evaluate(tab_delimited_csv_with_list());
   }


   void test_quoted_csvs() {
     evaluate(quoted_basic());
     evaluate(quoted_basic());
     evaluate(quoted_basic(",", "\r"));
     evaluate(quoted_basic(",", "\r\n"));
     evaluate(quoted_basic(",", "abc"));
     evaluate(quoted_basic(",", "aaaaaa"));
     evaluate(quoted_basic(" "));
     evaluate(quoted_basic(" ", "\r"));
     evaluate(quoted_basic(" ", "\r\n"));
     evaluate(quoted_basic(" ", "pokemon"));
     evaluate(quoted_basic(";"));
     evaluate(quoted_basic(";", "\r"));
     evaluate(quoted_basic(";", "\r\n"));
     evaluate(quoted_basic("::"));
     evaluate(quoted_basic("  "));
     evaluate(quoted_basic("\t\t"));
     evaluate(test_quoted_embedded_strings(","));
     evaluate(test_quoted_embedded_strings(" "));
     evaluate(test_quoted_embedded_strings("\t"));
     evaluate(test_quoted_embedded_strings("\t\t"));
     evaluate(test_quoted_embedded_strings("  "));
     evaluate(test_quoted_embedded_strings("::"));
     evaluate(test_quoted_type_inference(","));
     evaluate(test_quoted_type_inference(",", "zzz"));
   }

   void test_json() {
     evaluate(multiline_json());
   }

   void test_alternate_line_endings() {
     evaluate(alternate_endline_test());
   }
   void test_invalid_csv_cases() {
     evaluate(incorrectly_quoted_1());
   }
};

BOOST_FIXTURE_TEST_SUITE(_sframe_test, sframe_test)
BOOST_AUTO_TEST_CASE(test_string_escaping) {
  sframe_test::test_string_escaping();
}
BOOST_AUTO_TEST_CASE(test_substitutions) {
  sframe_test::test_substitutions();
}
BOOST_AUTO_TEST_CASE(test_csvs) {
  sframe_test::test_csvs();
}
BOOST_AUTO_TEST_CASE(test_quoted_csvs) {
  sframe_test::test_quoted_csvs();
}
BOOST_AUTO_TEST_CASE(test_json) {
  sframe_test::test_json();
}
BOOST_AUTO_TEST_CASE(test_alternate_line_endings) {
  sframe_test::test_alternate_line_endings();
}
BOOST_AUTO_TEST_CASE(test_invalid_csv_cases) {
  sframe_test::test_invalid_csv_cases();
}
BOOST_AUTO_TEST_SUITE_END()
