#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <set>
#include <vector>
#include <core/storage/sgraph_data/hilbert_parallel_for.hpp>

using namespace turi;

struct hilbert_par_for_test {
 public:
  void test_runner(size_t n, size_t threads) {
    std::vector<std::pair<size_t, size_t> > preamble_hits;
    mutex lock;
    std::vector<std::pair<size_t, size_t> > parallel_hits;
    sgraph_compute::hilbert_blocked_parallel_for(n, 
                                 [&](std::vector<std::pair<size_t, size_t> > v) {
                                   std::copy(v.begin(), v.end(), std::inserter(preamble_hits, preamble_hits.end()));
                                 },
                                 [&](std::pair<size_t, size_t> v) {
                                   lock.lock();
                                   parallel_hits.push_back(v);
                                   lock.unlock();
                                 }, threads);
    TS_ASSERT_EQUALS(preamble_hits.size(), n * n);
    TS_ASSERT_EQUALS(parallel_hits.size(), n * n);
    std::set<std::pair<size_t, size_t> > unique_vals;
    std::copy(preamble_hits.begin(), preamble_hits.end(), std::inserter(unique_vals, unique_vals.end()));
    TS_ASSERT_EQUALS(unique_vals.size(), n * n);
    // insert parallel again. We don't need to clear unique
    std::copy(parallel_hits.begin(), parallel_hits.end(), std::inserter(unique_vals, unique_vals.end()));
    TS_ASSERT_EQUALS(unique_vals.size(), n * n);
  }


  void test_hilbert_par_for() {
    test_runner(4, 4);
    // try an odd number
    test_runner(16, 3);
    // sequential?
    test_runner(16, 1);
  }
};

BOOST_FIXTURE_TEST_SUITE(_hilbert_par_for_test, hilbert_par_for_test)
BOOST_AUTO_TEST_CASE(test_hilbert_par_for) {
  hilbert_par_for_test::test_hilbert_par_for();
}
BOOST_AUTO_TEST_SUITE_END()
