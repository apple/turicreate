#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <set>
#include <unordered_map>
#include <core/data/flexible_type/flexible_type.hpp>
#include <ml/sketches/space_saving_flextype.hpp>
#include <core/random/random.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/logging/logger.hpp>

using namespace turi;
using namespace turi::sketches;

struct space_saving_test {
  template <typename SketchType> 
  double random_integer_length_test(size_t len, 
                                    size_t random_range,
                                    double epsilon) {
    SketchType ss(epsilon);
    
    std::vector<size_t> v(len);
    std::map<size_t, size_t> true_counter;
    for (size_t i = 0;i < len; ++i) {
      v[i] = turi::random::fast_uniform<size_t>(0, random_range - 1);
      ++true_counter[v[i]];
    }
    turi::timer ti;
    for (size_t i = 0;i < len; ++i) {
      if(v[i] % 2 == 0) {
        ss.add(v[i]);
      } else {
        ss.add(double(v[i]));  // So the types get a bit mixed up
      }
    }
    double rt = ti.current_time();
    std::vector<size_t> frequent_items;
    for(auto x : true_counter) {
      if (x.second >= epsilon * len) {
        frequent_items.push_back(x.first);
      }
    }
    // check that we did indeed find all the items with count >= epsilon * N
    auto ret = ss.frequent_items();
    std::sort(ret.begin(), ret.end());
    std::set<turi::flexible_type> ss_returned_values;
    for(auto x : ret) {
      ss_returned_values.insert(x.first);
    }

    for(auto x: frequent_items) {
      TS_ASSERT(ss_returned_values.count(x) != 0);
    }
    return rt;
  }

  template <typename SketchType> 
  double parallel_combine_test(size_t len, 
                               size_t random_range,
                               double epsilon) {
    std::vector<SketchType> ssarr(16, SketchType(epsilon));

    std::vector<size_t> v(len);
    std::unordered_map<size_t, size_t> true_counter;
    for (size_t i = 0;i < len; ++i) {
      v[i] = turi::random::fast_uniform<size_t>(0, random_range - 1);
      ++true_counter[v[i]];
    }
    turi::timer ti;
    for (size_t i = 0;i < len; ++i) {
      ssarr[i % ssarr.size()].add(v[i]);
    }
    // merge
    SketchType ss;
    for (size_t i = 0;i < ssarr.size(); ++i) {
      ss.combine(ssarr[i]);
    }
    double rt = ti.current_time();
    // check that we did indeed find all the items with count >= epsilon * N
    std::vector<size_t> frequent_items;
    for(auto x : true_counter) {
      if (x.second >= epsilon * len) {
        frequent_items.push_back(x.first);
      }
    }
    auto ret = ss.frequent_items();
    std::set<turi::flexible_type> ss_returned_values;
    for(auto x : ret) {
      ss_returned_values.insert(x.first);
    }
    for(auto x: frequent_items) {
      TS_ASSERT(ss_returned_values.count(x) != 0);
    }
    return rt;
  }
 public:
  void test_perf() {
    turi::sketches::space_saving_flextype ss(0.0001);
    turi::timer ti;
    for (size_t i = 0;i < 10*1024*1024; ++i) {
      ss.add(i);
    }
    std::cout << "\n Time: " << ti.current_time() << "\n";
  }
  
  void test_stuff() {
    turi::random::seed(1001);
    std::vector<size_t> lens{1024, 65536, 256*1024};
    std::vector<size_t> ranges{128, 1024, 65536, 256*1024};
    std::vector<double> epsilon{0.1,0.01,0.005};
    global_logger().set_log_level(LOG_INFO);

    size_t n_idx = lens.size() * ranges.size() * epsilon.size(); 

    in_parallel([&](size_t thread_idx, size_t n_threads) {
        for(size_t run_idx = thread_idx; run_idx < n_idx; run_idx += n_threads) {
            
          auto len   = lens[ run_idx / (epsilon.size() * ranges.size()) ];
          auto range = ranges[ (run_idx / epsilon.size()) % ranges.size()];
          auto eps   = epsilon[run_idx % epsilon.size()];
        
          auto result1 = random_integer_length_test<space_saving<flex_int> >(len, range, eps);
          logstream(LOG_INFO) << "integer:   Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result1
                    << std::endl;

          auto result2 = random_integer_length_test<space_saving<flexible_type> >(len, range, eps);

          logstream(LOG_INFO) << "flex type: Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result2
                    << std::endl;
          
          auto result3 = random_integer_length_test<space_saving_flextype>(len, range, eps);

          logstream(LOG_INFO) << "_flextype: Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result3
                    << std::endl;
        }
      }); 
        
    std::cout << "\n\nReset random seed and repeating with \'parallel\' test\n";
    turi::random::seed(1001);
      
    in_parallel([&](size_t thread_idx, size_t n_threads) {
        for(size_t run_idx = thread_idx; run_idx < n_idx; run_idx += n_threads) {
          
          auto len   = lens[ run_idx / (epsilon.size() * ranges.size()) ];
          auto range = ranges[ (run_idx / epsilon.size()) % ranges.size()];
          auto eps   = epsilon[run_idx % epsilon.size()];
        
          auto result1 = parallel_combine_test<space_saving<flex_int> >(len, range, eps);
          logstream(LOG_INFO) << "integer:   Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result1
                    << std::endl;

          auto result2 = parallel_combine_test<space_saving<flexible_type> >(len, range, eps);
          logstream(LOG_INFO) << "flex type: Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result2
                    << std::endl;

          auto result3 = parallel_combine_test<space_saving_flextype>(len, range, eps);
          logstream(LOG_INFO) << "_flextype: Array length: " << len << "\t" 
                    << "Numeric Range: " << range << "\t"
                    << "Epsilon:   " << eps << "  \t"
                    << result3
                    << std::endl;
        }
      });
      
  }

  
};

BOOST_FIXTURE_TEST_SUITE(_space_saving_test, space_saving_test)
BOOST_AUTO_TEST_CASE(test_perf) {
  space_saving_test::test_perf();
}
BOOST_AUTO_TEST_CASE(test_stuff) {
  space_saving_test::test_stuff();
}
BOOST_AUTO_TEST_SUITE_END()
