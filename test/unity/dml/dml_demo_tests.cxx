#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <unity/dml/dml_toolkit_runner.hpp>
#include <fileio/temp_files.hpp>
#include <fileio/fs_utils.hpp>

using namespace turi;

struct dml_demo_tests {
  public:

    void test_plus_one() {
      plus_one_test_impl(1);
      plus_one_test_impl(3);
    }

    void plus_one_test_impl(size_t num_workers) {
      setup();
      try {
        std::map<std::string, variant_type> args;
        args["x"] = 1;

        variant_type ret = runner.run("plus_one", args, working_dir, num_workers);

        flexible_type result = variant_get_value<int>(ret);
        TS_ASSERT_EQUALS(result, 2);
      } catch (...) {
        teardown();
        throw;
      }
      teardown();
    }

    void setup() {
      runner.set_library("libdistributed_demo.so");
      working_dir = turi::get_temp_name();
      fileio::create_directory(working_dir);
    }

    void teardown() {
      fileio::delete_path_recursive(working_dir);
    }

    dml_toolkit_runner runner;
    std::string working_dir;
};

BOOST_FIXTURE_TEST_SUITE(_dml_demo_tests, dml_demo_tests)
BOOST_AUTO_TEST_CASE(test_plus_one) {
  dml_demo_tests::test_plus_one();
}
BOOST_AUTO_TEST_SUITE_END()
