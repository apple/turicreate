#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sframe_data/shuffle.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <timer/timer.hpp>

using namespace turi;

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::set<flexible_type>::iterator)
struct shuffle_test {

    /**
     * Create input sframe contains 5k rows and 2 columns: "key" and "value".
     * The key contains the row id, and the corresponding value is identical to the key.
     */
    sframe create_input_sframe(size_t num_rows) {
      std::vector<flexible_type> row_ids;
      for (size_t i = 0; i < num_rows; ++i) {
        row_ids.push_back(i);
      }
      std::shared_ptr<sarray<flexible_type>> key_column(new sarray<flexible_type>);
      std::shared_ptr<sarray<flexible_type>> value_column(new sarray<flexible_type>);
      key_column->open_for_write();
      value_column->open_for_write();
      turi::copy(row_ids.begin(), row_ids.end(), *key_column);
      turi::copy(row_ids.begin(), row_ids.end(), *value_column);
      key_column->close();
      value_column->close();

      sframe ret({key_column, value_column}, {"key", "value"});
      return ret;
    }

  public:

    /**
     * Test we can shuffle an sframe with 5000 rows into
     * odd rows and even rows.
     */
    void test_basic_shuffle() {
      size_t num_rows = 5000;
      sframe sframe_in = create_input_sframe(num_rows);

      // shuffle the sframe into odd rows and even rows.
      std::function<size_t(const std::vector<flexible_type>&)> hash_fn = 
        [&](const std::vector<flexible_type>& row) {
          return (size_t)(row[0] % 2 == 0);
        };

      std::vector<sframe> sframe_out  = shuffle(sframe_in, 2, hash_fn);

      TS_ASSERT_EQUALS(sframe_out[0].num_rows(), num_rows / 2);
      TS_ASSERT_EQUALS(sframe_out[1].num_rows(), num_rows / 2);

      std::vector<std::vector<flexible_type>> odd_rows, even_rows;
      sframe_out[0].get_reader()->read_rows(0, num_rows / 2, odd_rows);
      sframe_out[1].get_reader()->read_rows(0, num_rows / 2, even_rows);

      std::set<flexible_type> expected_odd_ids;
      for (size_t i = 0; i < num_rows/2; ++i) {
        expected_odd_ids.insert(2 * i + 1);
      }
      std::set<flexible_type> expected_even_ids;
      for (size_t i = 0; i < num_rows/2; ++i) {
        expected_even_ids.insert(2 * i);
      }
      for (auto& row: odd_rows) {
        auto iter = expected_odd_ids.find(row[0]);
        TS_ASSERT_DIFFERS(iter, expected_odd_ids.end());
        expected_odd_ids.erase(iter);
      }
      for (auto& row: even_rows) {
        auto iter = expected_even_ids.find(row[0]);
        TS_ASSERT_DIFFERS(iter, expected_even_ids.end());
        expected_even_ids.erase(iter);
      }
    }

    /**
     * Test that we can shuffle different input size and different output size.
     * input size = [1000, 5000, 9000]
     * output_size = [5, 11, 23, 31, 47, 59]
     */
    void test_stress() {
      for (size_t input_size = 1000; input_size < 10000; input_size += 4000) {
        sframe sframe_in = create_input_sframe(input_size);
        for (size_t output_size : {5, 11, 23, 31, 47, 59}) {
          std::cout << "Input size: " << input_size << std::endl;
          std::cout << "Output size: " << output_size << std::endl;
          __test_shuffle__(sframe_in, output_size);
        }
      }
    }

    /**
     * Benchmar test.
     * input size = 10M
     * output_size = [5, 11, 23, 31, 47, 59]
     */
    void test_bench() {
#ifdef NDEBUG
        size_t input_size = 20000000;
        sframe sframe_in = create_input_sframe(input_size);
        for (size_t output_size : {5, 11, 23, 31, 47, 59}) {
          __test_shuffle__(sframe_in, output_size);
        }
#endif
    }

    /**
     * Test the edge case that we can shuffle empty sframe or sframe with one rows.
     */
    void test_edge() {
      sframe sframe_in = create_input_sframe(0);
      for (size_t output_size = 1; output_size < 5; ++output_size) {
        __test_shuffle__(sframe_in, output_size);
      }

      sframe_in = create_input_sframe(1);
      for (size_t output_size = 1; output_size < 5; ++output_size) {
        __test_shuffle__(sframe_in, output_size);
      }
    }

    /**
     *
     * Helper function to test we can shuffle an sframe
     * rows into any number of output sframes.
     */
    void __test_shuffle__(sframe sframe_in, size_t n) {
      std::function<size_t(const std::vector<flexible_type>&)> hash_fn = 
        [&](const std::vector<flexible_type>& row) {
          return (size_t)(row[0] % n == 0);
        };
      std::cout << "Input size: " << sframe_in.num_rows() << std::endl;
      std::cout << "Output size: " << n << std::endl;
      timer my_timer;
      std::vector<sframe> sframe_out = shuffle(sframe_in, n, hash_fn);
      std::cout << "Takes " << my_timer.current_time() << " secs" << std::endl;

      // // Check that shuffle preserve the size.
      // size_t num_rows = 0;
      // for (auto sf: sframe_out) {
      //   num_rows += sf.num_rows();
      // }
      // TS_ASSERT_EQUALS(num_rows, sframe_in.num_rows());
      //
      // // Check the correctness of shuffle.
      // size_t sf_id = 0;
      // for (auto sf: sframe_out) {
      //   std::vector<std::vector<flexible_type>> buffer;
      //   sf.get_reader()->read_rows(0, sf.num_rows(), buffer);
      //   for (auto& row: buffer) {
      //     TS_ASSERT_EQUALS(row[0], row[1]);
      //     TS_ASSERT_EQUALS(hash_fn(row) % n, sf_id);
      //   }
      //   ++sf_id;
      // }
    }
};

BOOST_FIXTURE_TEST_SUITE(_shuffle_test, shuffle_test)
BOOST_AUTO_TEST_CASE(test_basic_shuffle) {
  shuffle_test::test_basic_shuffle();
}
BOOST_AUTO_TEST_CASE(test_stress) {
  shuffle_test::test_stress();
}
BOOST_AUTO_TEST_CASE(test_bench) {
  shuffle_test::test_bench();
}
BOOST_AUTO_TEST_CASE(test_edge) {
  shuffle_test::test_edge();
}
BOOST_AUTO_TEST_SUITE_END()
