#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <limits>
#include <user_pagefault/type_heuristic_encode.hpp>
#include <random/random.hpp>
using namespace turi;

const size_t NUM_ELEM = 10000000;
struct types_heuristic_encode_test {

 public:
  char* create_integer_sequence() {
    int64_t* ret = new int64_t[NUM_ELEM];
    for (size_t i = 0;i < NUM_ELEM; ++i) {
      ret[i] = random::fast_uniform<int64_t>(-1000, 1000);
    }
    return reinterpret_cast<char*>(ret);
  }


  char* create_double_sequence() {
    double* ret = new double[NUM_ELEM];
    for (size_t i = 0;i < NUM_ELEM; ++i) {
      ret[i] = random::fast_uniform<double>(0, 1);
    }
    return reinterpret_cast<char*>(ret);
  }

  char* create_double_integral_sequence() {
    double* ret = new double[NUM_ELEM];
    for (size_t i = 0;i < NUM_ELEM; ++i) {
      ret[i] = (double)(random::fast_uniform<int64_t>(-1000, 1000));
    }
    return reinterpret_cast<char*>(ret);
  }

  char* create_random_sequence() {
    uint64_t* ret = new uint64_t[ NUM_ELEM];
    for (size_t i = 0;i < NUM_ELEM; ++i) {
      ret[i] = random::fast_uniform<uint64_t>(0, std::numeric_limits<uint64_t>::max());
    }
    return reinterpret_cast<char*>(ret);
  }

  void compress_decompress(char* input) {
    char* output = (char*)malloc(1024);
    size_t output_length = 1024;
    type_heuristic_encode::compress(input, NUM_ELEM * 8, &output, output_length);

    std::cout << "Compressed " << NUM_ELEM * 8 << " to: " << output_length << std::endl;

    char* decompressed_output = (char*)malloc(NUM_ELEM * 8);
    type_heuristic_encode::decompress(output, output_length, decompressed_output);

    for (size_t i = 0; i < NUM_ELEM * 8; ++i) {
      // we check this way here cos the TS_ASSERT_EQUAL is kinda slow
      if (input[i] != decompressed_output[i]) {
        TS_ASSERT_EQUALS(input[i], decompressed_output[i]);
      }
    }
    free(output);
    free(decompressed_output);
  }
 public:


  void test_integer() {
    char* c = create_integer_sequence();
    compress_decompress(c);
    delete c;
  }


  void test_double() {
    char* c = create_double_sequence();
    compress_decompress(c);
    delete c;
  }


  void test_double_integral_sequence() {
    char* c = create_double_integral_sequence();
    compress_decompress(c);
    delete c;
  }

  void test_random() {
    char* c = create_random_sequence();
    compress_decompress(c);
    delete c;
  }
};


BOOST_FIXTURE_TEST_SUITE(_types_heuristic_encode_test, types_heuristic_encode_test)
BOOST_AUTO_TEST_CASE(test_integer) {
  types_heuristic_encode_test::test_integer();
}
BOOST_AUTO_TEST_CASE(test_double) {
  types_heuristic_encode_test::test_double();
}
BOOST_AUTO_TEST_CASE(test_double_integral_sequence) {
  types_heuristic_encode_test::test_double_integral_sequence();
}
BOOST_AUTO_TEST_CASE(test_random) {
  types_heuristic_encode_test::test_random();
}
BOOST_AUTO_TEST_SUITE_END()
