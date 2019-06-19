#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/random/random.hpp>
#include <core/random/alias.hpp>
#include <core/parallel/thread_pool.hpp>
#include <cmath>
#include <iostream>
#include <vector>


typedef double vertex_data_type;
typedef double edge_data_type;



template<typename NumType>
void uniform_speed(const size_t max_iter) {
  NumType sum(0);
  turi::timer ti;
  ti.start();
  for(size_t i = 0; i < max_iter; ++i) {
    sum += (NumType)(turi::random::uniform<NumType>(0, 10));
  }
  double slow_time = ti.current_time();
  ti.start();
  for(size_t i = 0; i < max_iter; ++i) {
    sum += (NumType)(turi::random::fast_uniform<NumType>(0, 10));
  }
  double fast_time = ti.current_time();
  std::cout << slow_time << ", " << fast_time << std::endl; 
}


class thread_worker {
public:
  std::vector<int> values;
  void run() {
    namespace random = turi::random;
    for(size_t i = 0; i < values.size(); ++i) {
      values[i] = random::uniform<int>(0,3);
    }
  }
};

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& values) {
  out << "{";
  for(size_t i = 0; i < values.size(); ++i) {
    out << values[i];
    if(i + 1 < values.size()) out << ", ";
  }
  return out << "}";
}


std::vector<int> operator+(const std::vector<int>& v1, 
                           const std::vector<int>& v2) {
  assert(v1.size() == v2.size());
  std::vector<int> result(v1.size());
  for(size_t i = 0; i < result.size(); ++i) {
    result[i] = v1[i] + v2[i];
  }
  return result;
}




struct RandomTestSuite {
  size_t iterations;
  RandomTestSuite() : iterations(1E8) { }

  void test_nondet_generator() {
    turi::random::nondet_seed();
    turi::random::nondet_seed();
    turi::random::nondet_seed();
  }


  void test_random_number_generators() {
    std::cout << std::endl;
    std::cout << "beginning seed" << std::endl;
    namespace random = turi::random;
    turi::random::seed();
    turi::random::time_seed();
    turi::random::nondet_seed();
    turi::random::seed(12345);
    std::cout << "finished" << std::endl;

    const size_t num_iterations(20);
    std::vector<thread_worker> workers(10);
    for(size_t i = 0; i < workers.size(); ++i) 
      workers[i].values.resize(num_iterations);
    turi::thread_group threads;
    for(size_t i = 0; i < workers.size(); ++i) {
      threads.launch(boost::bind(&thread_worker::run, &(workers[i])));
    }
    threads.join();
    for(size_t i = 0; i < workers.size(); ++i) {
      std::cout << workers[i].values << std::endl;
    }
    std::vector<int> sum(workers[0].values.size());
    for(size_t i = 0; i < workers.size(); ++i) {
      sum = sum + workers[i].values;
    }
    std::cout << "Result: " << sum << std::endl;
  }

  void shuffle() {
    namespace random = turi::random;
    random::nondet_seed();
    std::vector<int> numbers(100);
    for(size_t i = 0; i < numbers.size(); ++i) numbers[i] = (int)i + 1;
    for(size_t j = 0; j < 10; ++j) {
      // shuffle the numbers
      random::shuffle(numbers);
      std::cout << numbers << std::endl;
    }
  }

};
  
BOOST_FIXTURE_TEST_SUITE(_RandomTestSuite, RandomTestSuite) 
  BOOST_AUTO_TEST_CASE(test_nondet_generator) {
    RandomTestSuite::test_nondet_generator();
  }
  BOOST_AUTO_TEST_CASE(test_random_number_generators) {
    RandomTestSuite::test_random_number_generators();
  }
BOOST_AUTO_TEST_SUITE_END()


  // void speed() {
  //   namespace random = turi::random;
  //   std::cout << "speed test run: " << std::endl;
  //   const size_t MAX_ITER(10000);
  //   std::cout << "size_t:   "; 
  //   uniform_speed<size_t>(MAX_ITER);
  //   std::cout << "int:      "; 
  //   uniform_speed<int>(MAX_ITER);
  //   std::cout << "uint32_t: "; 
  //   uniform_speed<uint32_t>(MAX_ITER);
  //   std::cout << "uint16_t: "; 
  //   uniform_speed<uint16_t>(MAX_ITER);
  //   std::cout << "char:     "; 
  //   uniform_speed<char>(MAX_ITER);
  //   std::cout << "float:    "; 
  //   uniform_speed<float>(MAX_ITER);
  //   std::cout << "double:   "; 
  //   uniform_speed<double>(MAX_ITER);
    
  //   std::cout << "gaussian: ";
  //   double sum = 0;
  //   turi::timer time;
  //   time.start();
  //   for(size_t i = 0; i < MAX_ITER; ++i) 
  //     sum += random::gaussian();
  //   std::cout << time.current_time() << std::endl;
    
  //   std::cout << "shuffle:  "; 
  //   std::vector<int> numbers(6);
  //   for(size_t i = 0; i < numbers.size(); ++i) numbers[i] = (int)i + 1;
  //   time.start();
  //   for(size_t j = 0; j < MAX_ITER/numbers.size(); ++j) {
  //     // shuffle the numbers
  //     random::shuffle(numbers);
  //   }
  //   std::cout << time.current_time() << ", ";
  //   time.start();
  //   for(size_t j = 0; j < MAX_ITER/numbers.size(); ++j) {
  //     // shuffle the numbers
  //     std::random_shuffle(numbers.begin(), numbers.end());
  //   }
  //   std::cout << time.current_time() << std::endl;    
  // }


  
