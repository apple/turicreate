#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

class layer_wrapper_tests {
	public:
		void example_test() {
			TS_ASSERT(true);
		}
};

BOOST_FIXTURE_TEST_SUITE(_layer_wrapper_tests, layer_wrapper_tests)

BOOST_AUTO_TEST_CASE(example_test) {
  layer_wrapper_tests::example_test();
}

BOOST_AUTO_TEST_SUITE_END()