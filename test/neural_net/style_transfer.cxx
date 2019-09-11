#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

struct style_transfer_test {
public:
  void test_encoding() {
    std::cout << "Here is it" << std::endl;
  }
};

BOOST_FIXTURE_TEST_SUITE(_style_transfer_test, style_transfer_test)
BOOST_AUTO_TEST_CASE(test_encoding) {
  style_transfer_test::test_encoding();
}

BOOST_AUTO_TEST_SUITE_END()
