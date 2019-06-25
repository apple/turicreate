#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <ml/sketches/hyperloglog.hpp>
#include <core/random/random.hpp>
struct hyperloglog_test {
  void random_integer_length_test(size_t len, 
                                  size_t random_range,
                                  size_t hllbits) {
    turi::sketches::hyperloglog hll(hllbits);
    std::vector<size_t> v(len);
    for (size_t i = 0;i < len; ++i) {
      v[i] = turi::random::fast_uniform<size_t>(0, random_range - 1);
      hll.add(v[i]);
    }
    std::sort(v.begin(), v.end());
    auto ret = std::unique(v.begin(), v.end());
    size_t num_unique = std::distance(v.begin(), ret);

    auto lower = hll.estimate() - 2 * hll.error_bound();
    auto upper = hll.estimate() + 2 * hll.error_bound();
    std::cout << num_unique << 
              " vs (" << lower << ", " 
                     << upper << ")\n";
    TS_ASSERT_LESS_THAN(lower, num_unique);
    TS_ASSERT_LESS_THAN(num_unique, upper);
  }

  void parallel_combine_test(size_t len, 
                             size_t random_range,
                             size_t hllbits) {
    // make a bunch of "parallel" hyperloglogs which can be combined
    using turi::sketches::hyperloglog;
    std::vector<hyperloglog> hllarr(16, hyperloglog(hllbits));
    // test against the sequential one
    hyperloglog sequential_hll(hllbits);

    std::vector<size_t> v(len);
    for (size_t i = 0;i < len; ++i) {
      v[i] = turi::random::fast_uniform<size_t>(0, random_range - 1);
      hllarr[i% 16].add(v[i]);
      sequential_hll.add(v[i]);
    }
    // make the final hyperloglog which comprises of all the 
    // hyperloglogs combined
    hyperloglog hll(hllbits);
    for (size_t i = 0;i < hllarr.size(); ++i) hll.combine(hllarr[i]);

    std::sort(v.begin(), v.end());
    auto ret = std::unique(v.begin(), v.end());
    size_t num_unique = std::distance(v.begin(), ret);

    auto lower = hll.estimate() - 2 * hll.error_bound();
    auto upper = hll.estimate() + 2 * hll.error_bound();
    std::cout << num_unique << 
              " vs (" << lower << ", " 
                     << upper << ")\n";
    TS_ASSERT_LESS_THAN(lower, num_unique);
    TS_ASSERT_LESS_THAN(num_unique, upper);
    TS_ASSERT_EQUALS(hll.estimate(), sequential_hll.estimate());
  }
 public:
  void test_stuff() {
    turi::random::seed(1001);
    std::vector<size_t> lens{1024, 65536, 1024*1024};
    std::vector<size_t> ranges{128, 1024, 65536, 1024*1024};
    std::vector<size_t> bits{8, 12, 16};
    for (auto len: lens) {
      for (auto range: ranges) {
        for (auto bit: bits) {
          std::cout << "Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Num Buckets: 2^" << bit << "\n";
          random_integer_length_test(len, range, bit);
        }
      }
    }

    std::cout << "\n\nReset random seed and repeating with \'parallel\' test\n";
    turi::random::seed(1001);
    for (auto len: lens) {
      for (auto range: ranges) {
        for (auto bit: bits) {
          std::cout << "Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Num Buckets: 2^" << bit << "\n";
          parallel_combine_test(len, range, bit);
        }
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_hyperloglog_test, hyperloglog_test)
BOOST_AUTO_TEST_CASE(test_stuff) {
  hyperloglog_test::test_stuff();
}
BOOST_AUTO_TEST_SUITE_END()
