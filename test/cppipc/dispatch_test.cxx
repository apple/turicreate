#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <core/system/cppipc/server/dispatch.hpp>
#include <core/system/cppipc/server/dispatch_impl.hpp>
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

void call_dispatch(cppipc::dispatch* d,
                   void* testobject,
                   std::stringstream& message,
                   std::stringstream& response) {
  turi::iarchive iarc(message);
  turi::oarchive oarc(response);
  d->execute(testobject, NULL /* comm_server here. we don't have one */, 
             iarc, oarc);
}


struct dispatch_test {
 public:
  void test_basic_argument() {
    // create a test object
    test testobject;
    testobject.a = 20;

    // create a dispatch to the add call
    cppipc::dispatch* d = cppipc::create_dispatch(&test::add);

    // build a message
    std::stringstream message_stream, response_stream;
    turi::oarchive message(message_stream);
    message << (size_t)10;

    // perform the call
    call_dispatch(d, &testobject, message_stream, response_stream);

    // parse the response
    turi::iarchive response(response_stream);
    std::string response_string;
    response >> response_string;

    TS_ASSERT_EQUALS(response_string, "30");
  }

  void test_interesting_arguments() {
    // create a test object
    test testobject;
    testobject.a = 20;

    // create a dispatch to the add call
    cppipc::dispatch* d = cppipc::create_dispatch(&test::add_more);

    // build a message
    std::stringstream message_stream, response_stream;
    turi::oarchive message(message_stream);
    message << (size_t)10 << (size_t)20 << (size_t)30 << (size_t)40;


    // perform the call
    call_dispatch(d, &testobject, message_stream, response_stream);


    // parse the response
    turi::iarchive response(response_stream);
    std::string response_string;
    response >> response_string;

    TS_ASSERT_EQUALS(response_string, "120");
  }


  void test_string_argument() {
    // create a test object
    test testobject;

    // create a dispatch to the add call
    cppipc::dispatch* d = cppipc::create_dispatch(&test::add_one);

    // build a message
    std::stringstream message_stream, response_stream;
    turi::oarchive message(message_stream);
    message << std::string("abc");

    // perform the call
    call_dispatch(d, &testobject, message_stream, response_stream);


    // parse the response
    turi::iarchive response(response_stream);
    std::string response_string;
    response >> response_string;

    TS_ASSERT_EQUALS(response_string, "abc1");
  }
};


BOOST_FIXTURE_TEST_SUITE(_dispatch_test, dispatch_test)
BOOST_AUTO_TEST_CASE(test_basic_argument) {
  dispatch_test::test_basic_argument();
}
BOOST_AUTO_TEST_CASE(test_interesting_arguments) {
  dispatch_test::test_interesting_arguments();
}
BOOST_AUTO_TEST_CASE(test_string_argument) {
  dispatch_test::test_string_argument();
}
BOOST_AUTO_TEST_SUITE_END()
