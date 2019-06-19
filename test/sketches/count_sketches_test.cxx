#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <unordered_map>
#include <ml/sketches/countsketch.hpp>
#include <ml/sketches/countmin.hpp>
#include <core/random/random.hpp>
#include <timer/timer.hpp>

struct countsketch_test {

  /**
   * Create a set of random integers to be used to benchmark
   * the countsketch sketch. 
   * One can choose the number of unique values and the distribution
   * of each element's frequency. 
   * See the documentation for test_benchmark for more details.
   */
  std::vector<std::pair<size_t, size_t>> item_counts(size_t num_unique_items, 
      size_t count_per_item,
      bool exponential) {
 
    std::vector<std::pair<size_t, size_t>> v;
    float alpha = 1.0;  // parameter for exponential distribution

    for (size_t i = 0; i < num_unique_items; ++i) {
      size_t count = count_per_item;
      if (exponential) count = std::floor(count * turi::random::gamma(alpha));
      v.push_back(std::pair<size_t, size_t>(i, count));
    }

    std::sort(v.begin(), v.end(),
         [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
            return lhs.second < rhs.second; } );
    return v;
  }
 
  /**
   * Run an experiment (described more fully in the documentation for test_benchmark).
   *
   * \param m a synthetic dataset
   * \param sketch a sketch object
   * \param num_to_compare the number of objects for which we want to compute RMSE
   * \param verbose 
   */
  template<typename T>
  std::map<std::string, double> run_experiment(std::vector<std::pair<size_t, size_t>> m, 
                                               T sketch,  
                                               size_t num_to_compare, 
                                               bool verbose) {

    std::map<std::string, double> result;
   
    // Compute sketch
    turi::timer ti;
    ti.start();
    for (auto kv : m) {
      sketch.add(kv.first, kv.second);
    }
    result["elapsed"] = ti.current_time();
    result["count"] = m.size();

    // Compute RMSE for least common items
    std::vector<size_t> items;
    std::vector<int64_t> estimated;
    std::vector<int64_t> actual;
    for (size_t i = 0; i < num_to_compare; ++i) { 
      size_t item = m[i].first;
      size_t true_value = m[i].second;
      int64_t estimate = sketch.estimate(item);

      items.push_back(item);
      estimated.push_back(estimate);
      actual.push_back((int64_t) true_value);
      if (verbose) 
        std::cout << item << " : " << true_value << " : " << estimate << std::endl; 
    }
    result["rmse_rare"] = rmse<int64_t>(estimated, actual);

    // Compute RMSE for most common items
    items.clear();
    estimated.clear();
    actual.clear();
    for (size_t i = m.size()-1; i > m.size() - num_to_compare; --i) { 
      size_t item = m[i].first;
      size_t true_value = m[i].second;
      int64_t estimate = sketch.estimate(item); 

      items.push_back(item);
      estimated.push_back(estimate);
      actual.push_back((int64_t) true_value);
      if (verbose) 
        std::cout << item << " : " << true_value << " : " << estimate << std::endl; 
 
    }
    result["rmse_common"] = rmse<int64_t>(estimated, actual); 

    // Compute the density of the sketch, i.e. the ratio of nonzeros
    result["density"] = sketch.density();

    if (verbose) sketch.print();

    return result;
  }

  // Helper function that computes RMSE of two vectors of objects.
  template <typename T>
  double rmse(std::vector<T> y, std::vector<T> yhat) {
    DASSERT_EQ(y.size(), yhat.size());
    double rmse = 0;
    for (size_t i = 0; i < y.size(); ++i) { 
      rmse += std::pow((double) y[i] - (double) yhat[i], 2.0);
    }
    return std::sqrt(rmse / (double) y.size());
  }

 public:

  /**
   * Small example to use for debugging.
   */
  void test_small_example() {

    size_t num_unique = 20;
    size_t mean_count_per_item = 5;
    bool expo = true;
    auto items = item_counts(num_unique, mean_count_per_item, expo);

    size_t num_bits = 4;
    size_t num_hash = 3;

    turi::sketches::countmin<size_t> cm(num_bits, num_hash);
    turi::sketches::countsketch<size_t> cs(num_bits, num_hash);

    for (auto kv : items) {
      std::cout << std::endl;
      for (size_t i = 0; i < kv.second; ++i) {
        cm.add(kv.first);
        cs.add(kv.first);
      }
      cm.print();
      cs.print();
    }

    for (auto kv : items) {
      std::cout << kv.first << ":" << kv.second << ":" << cm.estimate(kv.first) << std::endl;
    }
 
    for (auto kv : items) {
      std::cout << kv.first << ":" << kv.second << ":" << cs.estimate(kv.first) << std::endl;
    }
  } 


  /**
   * This benchmark compares the RMSE for predicting the frequency of objects in a stream when
   * for two sketches: the CountMin sketch and the CountSketch. 
   * The synthetic data set we create has a fixed number of objects (in this case simply integers)
   * and we create a stream where each object is observed a given number of times. We consider
   * the situation where the frequency is uniform across all items and where the frequency
   * has a geometric distribution (more or less); we keep the expected frequency per user fixed.
   *
   * Two metrics are chosen at this point: RMSE for the 20 most common items and RMSE for the 20
   * least common items. 
   *
   * We vary the width and depth of each sketch.
   *
   * The columns of the results table are:
   *   - type of sketch
   *   - number of hash functions (depth)
   *   - number of bits (2^b is the number of bins, i.e. width)
   *   - number of unique objects included in sketch
   *   - 0 if all objects appear with the same frequecy; 1 if exponentially distributed
   *   - RMSE of the observed vs. predicted frequency for the 20 most rare items
   *   - RMSE of the observed vs. predicted frequency for the 20 most common items
   *   - # updates / second (in millions)
   *   - "compression ratio": The size of the sketch / the number of unique elements
   *   - density of the sketch: proportion of nonzero elements in the counts matrix
   */
  void test_benchmark() {

    bool verbose = false;
    turi::random::seed(1002);

    // Set up synthetic data 
    size_t num_to_compare = 20;               // number of items to use when computing RMSE
    size_t num_unique = 100000;   // number of unique objects
    size_t mean_count_per_item = 15;          // expected number of observations per object

    // Set up experiment
    std::vector<size_t> num_hash{5, 10};      // number of hash functions to use for each sketch
    std::vector<size_t> bits{8, 10, 12, 14};  // number of bins to use for each sketch (2^bits)
     
    // Set up reporting
    std::cout.precision(5);
    std::cout << "\nsketch\t# hash\t# bits\t# uniq\texpon.\trmse_r\trmse_c\t"
              << "#items(M)/s\tratio\tdensity" << std::endl;

    // Consider both uniformly distributed and exponentially distributed per-object frequecies
    for (auto expo : {true, false}) {

      // Generate data
      auto items = item_counts(num_unique, mean_count_per_item, expo);

      for (auto h: num_hash) {
        for (auto b: bits) {

          std::vector<std::string> sketch_names = {"CountSketch", "CountMinSketch"};
          for (auto sk : sketch_names) {
            std::map<std::string, double> res;

            // Create sketch
            if (sk == "CountSketch") {
              turi::sketches::countsketch<size_t> cs(b, h);
              res = run_experiment(items, cs, num_to_compare, verbose);
            } else {
              turi::sketches::countmin<size_t> cm(b, h);
              res = run_experiment(items, cm, num_to_compare, verbose);
            }

            // Compute number of updates per second
            double rate = (double) res["count"] / res["elapsed"] / 1000000;

            // Compute "compression ratio": The size of the sketch / the number of unique elements
            double ratio = (double) (h * (1<<b)) / (double) num_unique; 

            std::cout << sk << "\t" << h << "\t" << b << "\t" << 
              num_unique << "\t" << expo << "\t" << 
              res["rmse_rare"] << "\t" << res["rmse_common"] << "\t" <<
              rate << "\t" << ratio << "\t" << res["density"] << std::endl;
          }
        }
      }
    }

    }
};

BOOST_FIXTURE_TEST_SUITE(_countsketch_test, countsketch_test)
BOOST_AUTO_TEST_CASE(test_small_example) {
  countsketch_test::test_small_example();
}
BOOST_AUTO_TEST_CASE(test_benchmark) {
  countsketch_test::test_benchmark();
}
BOOST_AUTO_TEST_SUITE_END()
