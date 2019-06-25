#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/thread_pool.hpp>
#include <core/parallel/atomic.hpp>
#include <core/logging/assertions.hpp>
#include <timer/timer.hpp>
#include <boost/bind.hpp>
#include <thread>
#include <chrono>

using namespace turi;

atomic<int> testval;

void test_inc() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  testval.inc();
}

void test_dec() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  testval.dec();
}



void thread_assert_false() {
  ASSERT_TRUE(false);
}


void test_group_exception_forwarding(){
  std::cout << "\n";
  std::cout << "----------------------------------------------------------------\n";
  std::cout << "This test will print a  large number of assertional failures\n";
  std::cout << "and back traces. This is intentional as we are testing the\n" ;
  std::cout << "exception forwarding scheme\n";
  std::cout << "----------------------------------------------------------------\n";
  std::cout << std::endl;

  thread_group group;

  
  thread thr3;
  thr3.launch(thread_assert_false);
  try {
    thr3.join();
  }
  catch(const char* c) {
    logstream(LOG_INFO) << "Exception " << c << " forwarded successfully!" << std::endl;
  }
  
  
  for (size_t i = 0;i < 10; ++i) {
    group.launch(thread_assert_false);
  }
  
  size_t numcaught = 0;
  try {
    group.join();
  }
  catch (std::string c){
    logstream(LOG_INFO) << "Exception " << c << " forwarded successfully!" << std::endl;
    numcaught++;
  }
  logstream(LOG_INFO) << "Caught " << numcaught << " exceptions!" << std::endl;
  TS_ASSERT(numcaught > 0);
}

void test_pool(){
  testval.value = 0;
  thread_pool pool(4);
  for (size_t j = 0;j < 10; ++j) {
    for (size_t i = 0;i < 10; ++i) {
      pool.launch(test_inc);
    }
    for (size_t i = 0;i < 10; ++i) {
      pool.launch(test_dec);
    }
    pool.set_cpu_affinity(j % 2);
  }
  
  pool.join();
  TS_ASSERT_EQUALS(testval.value, 0);
}

void test_pool_exception_forwarding(){
  std::cout << "\n";
  std::cout << "----------------------------------------------------------------\n";
  std::cout << "This test will print a  large number of assertional failures\n";
  std::cout << "and back traces. This is intentional as we are testing the\n" ;
  std::cout << "exception forwarding scheme\n";
  std::cout << "----------------------------------------------------------------\n";
  std::cout << std::endl;
  thread_pool thpool(10);
  parallel_task_queue pool(thpool);


  
  thread thr3;
  thr3.launch(thread_assert_false);
  try {
    thr3.join();
  }
  catch(std::string c) {
    logstream(LOG_INFO) << "Exception " << c << " forwarded successfully!" << std::endl;
  }
  
  
  for (size_t i = 0;i < 10; ++i) {
    pool.launch(thread_assert_false);
    if (i == 50) {
      thpool.set_cpu_affinity(true);
    }
  }
  
  size_t numcaught = 0;
  while (1) {
    try {
      pool.join();
      break;
    }
    catch (std::string c){
      logstream(LOG_INFO) << "Exception " << c << " forwarded successfully!" << std::endl;
      numcaught++;
    }
  }
  logstream(LOG_INFO) << "Caught " << numcaught << " exceptions!" << std::endl;
  TS_ASSERT(numcaught > 0);
}






struct ThreadToolsTestSuite  {
public:

  void test_thread_pool(void) {
   test_pool();
  }

  // TODO: Make this test WORK again
  void thread_group_exception(void) {
    test_group_exception_forwarding();
  }

  void  test_thread_pool_exception(void) {
    test_pool_exception_forwarding();
  }

};

BOOST_FIXTURE_TEST_SUITE(_ThreadToolsTestSuite, ThreadToolsTestSuite)
BOOST_AUTO_TEST_CASE(test_thread_pool) {
  ThreadToolsTestSuite::test_thread_pool();
}
BOOST_AUTO_TEST_CASE(test_thread_pool_exception) {
  ThreadToolsTestSuite::test_thread_pool_exception();
}
BOOST_AUTO_TEST_SUITE_END()
