#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>
#include <core/storage/lazy_eval/lazy_eval_operation.hpp>
#include <core/storage/lazy_eval/lazy_eval_operation_dag.hpp>

using namespace turi;

struct adder: lazy_eval_operation_base<int> {
  virtual size_t num_arguments() { return 2; }
  std::string name() const { return "add"; }
  virtual void execute(int& output,
                       const std::vector<int*>& parents) {
    std::cout << "Add of " << output << " and " << *parents[0] << "\n";
    output += *(parents[0]);
  }
};

struct multiplier: lazy_eval_operation_base<int> {
  virtual size_t num_arguments() { return 2; }
  std::string name() const { return "multiply"; }
  virtual void execute(int& output,
                       const std::vector<int*>& parents) {
    std::cout << "Multiply of " << output << " and " << *parents[0] << "\n";
    output *= *(parents[0]);
  }
};

struct increment: lazy_eval_operation_base<int> {
  virtual size_t num_arguments() { return 1; }
  std::string name() const { return "increment"; }
  virtual void execute(int& output,
                       const std::vector<int*>& parents) {
    std::cout << "Increment of " << output << "\n";
    output++;
  }
};

struct set_val: lazy_eval_operation_base<int> {
  set_val(int i): val(i) { }
  size_t val;
  std::string name() const { return "assign to " + std::to_string(val); }
  virtual size_t num_arguments() { return 0; }
  virtual void execute(int& output,
                       const std::vector<int*>& parents) {
    std::cout << "Set to " << val << "\n";
    output = val; 
  }
};

static size_t alloc_calls = 0;
int* allocator() {
  std::cout << "Allocate new integer\n";
  ++alloc_calls;
  return new int;
}

static size_t copy_calls = 0;
void copier(int& dest, const int& src) {
  std::cout << "Copy of integer " << src << "\n";
  dest = src;
  copy_calls++;
}



struct lazy_eval_test {
 public:
  void test_lazy_eval() {
    alloc_calls = 0;
    copy_calls = 0;
    lazy_eval_operation_dag<int> dag(allocator, copier);

    lazy_eval_future<int>* zero = dag.add_operation(new set_val(0), {});
    lazy_eval_future<int>* one = dag.add_operation(new increment, {zero});
    lazy_eval_future<int>* two= dag.add_operation(new increment, {one});
    lazy_eval_future<int>* three = dag.add_operation(new increment, {two});
    std::cout << "\n\nCompute of 3 = 0 ++ ++ ++\n";
    TS_ASSERT_EQUALS((*three)(), 3);
    std::cout << "Deleting 0 and 1\n";
    delete zero; delete one; dag.cleanup();
    std::cout << "Recompute 3\n";
    three->reset();
    TS_ASSERT_EQUALS((*three)(), 3);
    std::cout << "Deleting 2\n";
    delete two; dag.cleanup();
    std::cout << "Recompute 3\n";
    three->reset();
    TS_ASSERT_EQUALS((*three)(), 3);
    delete three; dag.cleanup();
  }

  void test_lazy_eval2() {
    alloc_calls = 0;
    copy_calls = 0;
    lazy_eval_operation_dag<int> dag(allocator, copier);
    lazy_eval_future<int>* five = dag.add_value(new int(5));
    lazy_eval_future<int>* two = dag.add_operation(new set_val(2), {});
    lazy_eval_future<int>* seven = dag.add_operation(new adder, {five, two});
    lazy_eval_future<int>* nine = dag.add_operation(new adder, {seven, two});
    std::cout << "\n\nCompute of 9 = (5 + 2) + 2\n";
    TS_ASSERT_EQUALS((*nine)(), 9);
    // delete all
    std::cout << "Delete All\n";
    delete five; delete two; delete seven; delete nine;
    dag.cleanup();
    
  }



  void test_lazy_eval3() {
    alloc_calls = 0;
    copy_calls = 0;
    lazy_eval_operation_dag<int> dag(allocator, copier);
    lazy_eval_future<int>* five = dag.add_operation(new set_val(5), {});
    lazy_eval_future<int>* two = dag.add_value(new int(2));
    lazy_eval_future<int>* seven = dag.add_operation(new adder, {five, two});
    lazy_eval_future<int>* nine = dag.add_operation(new adder, {seven, two});
    lazy_eval_future<int>* eighteen= dag.add_operation(new multiplier, {nine, two});
    std::cout << "\n\nCompute of (5 + 2) == 7\n";
    TS_ASSERT_EQUALS((*seven)(), 7);
    std::cout << "Compute of ((5 + 2) + 2) * 2 \n";
    TS_ASSERT_EQUALS((*eighteen)(), 18);
    seven->reset();
    eighteen->reset();
    std::cout << "Compute of 18 after reset\n";
    TS_ASSERT_EQUALS((*eighteen)(), 18);
    lazy_eval_future<int>* twenty_three = dag.add_operation(new adder, {eighteen, five});
    TS_ASSERT_EQUALS((*twenty_three)(), 23);

    std::cout << "Delete 7, 23\n";
    delete twenty_three; delete seven; eighteen->reset(); 
    std::cout << dag;
    dag.cleanup();
    std::cout << "Evaluate 18\n";
    TS_ASSERT_EQUALS((*eighteen)(), 18);
    std::cout << "Delete 18\n";
    delete eighteen; dag.cleanup();
    std::cout << "Evaluate 9\n";
    TS_ASSERT_EQUALS((*nine)(), 9);
    std::cout << "Evaluate 9 + 2\n";
    lazy_eval_future<int>* eleven= dag.add_operation(new adder, {nine, two});
    TS_ASSERT_EQUALS((*eleven)(), 11);
    std::cout << "Delete 9\n";
    delete nine; 
    std::cout << dag;
    eleven->reset(); 
    dag.cleanup();
    std::cout << "Evaluate 11\n";
    TS_ASSERT_EQUALS((*eleven)(), 11);
    std::cout << "Delete All\n";
    delete eleven; delete two; delete five; dag.cleanup();

  }
};

BOOST_FIXTURE_TEST_SUITE(_lazy_eval_test, lazy_eval_test)
BOOST_AUTO_TEST_CASE(test_lazy_eval) {
  lazy_eval_test::test_lazy_eval();
}
BOOST_AUTO_TEST_CASE(test_lazy_eval2) {
  lazy_eval_test::test_lazy_eval2();
}
BOOST_AUTO_TEST_CASE(test_lazy_eval3) {
  lazy_eval_test::test_lazy_eval3();
}
BOOST_AUTO_TEST_SUITE_END()
