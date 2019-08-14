#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <core/system/cppipc/client/issue.hpp>
struct test {
  size_t a;

  std::string add(size_t c) {
    std::stringstream strm;
    strm << (a + c);
    return strm.str();
  }

  std::string add_more(size_t&& c, const size_t& d, const size_t e, size_t f) {
    std::stringstream strm;
    strm << (a + c + d + e + f);
    return strm.str();
  }

  std::string add_one(std::string s) {
    return s + "1";
  }
};


struct issue_test {
 public:
  void test_basic_argument() {
    // create a dispatch to the add call
    std::stringstream message_stream;
    turi::oarchive message(message_stream);
    // intentionally write a wrong type. It should cast correctly
    // by the issuer
    cppipc::issue(message, &test::add, (char)20);
    
    // parse the issued message 
    turi::iarchive read_message(message_stream);
    size_t arg1;
    read_message >> arg1;

    TS_ASSERT_EQUALS(arg1, 20);
  }

  void test_interesting_arguments() {
    // create a dispatch to the add call
    std::stringstream message_stream;
    turi::oarchive message(message_stream);
    // intentionally write a wrong type. It should cast correctly
    // by the issuer
    cppipc::issue(message, &test::add_more, (char)20, int(20), long(30), (unsigned int)(40));

    
    // parse the issued message 
    turi::iarchive read_message(message_stream);
    size_t arg1, arg2, arg3, arg4;
    read_message >> arg1 >> arg2 >> arg3 >> arg4;
    TS_ASSERT_EQUALS(arg1, 20);
    TS_ASSERT_EQUALS(arg2, 20);
    TS_ASSERT_EQUALS(arg3, 30);
    TS_ASSERT_EQUALS(arg4, 40);
  }


  void test_string_argument() {
    // create a dispatch to the add call
    std::stringstream message_stream;
    turi::oarchive message(message_stream);
    // intentionally write a wrong type. It should cast correctly
    // by the issuer
    cppipc::issue(message, &test::add_one, "hello");

    
    // parse the issued message 
    turi::iarchive read_message(message_stream);
    std::string s;
    read_message >> s;
    TS_ASSERT_EQUALS(s, "hello");
  }
};


BOOST_FIXTURE_TEST_SUITE(_issue_test, issue_test)
BOOST_AUTO_TEST_CASE(test_basic_argument) {
  issue_test::test_basic_argument();
}
BOOST_AUTO_TEST_CASE(test_interesting_arguments) {
  issue_test::test_interesting_arguments();
}
BOOST_AUTO_TEST_CASE(test_string_argument) {
  issue_test::test_string_argument();
}
BOOST_AUTO_TEST_SUITE_END()
