#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <cassert>
#include <iostream>
#include <ml/sketches/quantile_sketch.hpp>
#include <ml/sketches/streaming_quantile_sketch.hpp>
#include <core/random/random.hpp>
struct quantile_sketch_test {
  static constexpr double epsilon = 0.01;
  typedef turi::sketches::quantile_sketch<double> sketch_type;
  typedef turi::sketches::streaming_quantile_sketch<double> streaming_sketch_type;

  template <typename SKETCH_TYPE>
  void compare_quantiles_at(std::vector<double>& values,
                            SKETCH_TYPE& sketch,
                            double quantile) {
    size_t index = quantile * values.size();
    index = std::min(values.size() - 1, index);
    int lower_index = index - values.size() * epsilon;
    int upper_index = index + values.size() * epsilon;
    lower_index = std::max<int>(lower_index, 0);
    upper_index = std::min<int>(upper_index, values.size() - 1);
    double lower = values[lower_index];
    double upper = values[upper_index];
    double query = sketch.query(index);

    TS_ASSERT_LESS_THAN_EQUALS(lower, query);
    TS_ASSERT_LESS_THAN_EQUALS(query, upper);
    TS_ASSERT_EQUALS(query, sketch.query_quantile(quantile));
    TS_ASSERT_EQUALS(sketch.fast_query(index), 
                     sketch.fast_query_quantile(quantile));

    std::cout << quantile*100 << "% : " << values[index] << " vs " 
              << sketch.query_quantile(quantile) 
              << " true epsilon interval:[" << lower << ", " << upper << "]  "
              << "(fast query: " << sketch.fast_query_quantile(quantile) << ")\n";
  }

  void quantile_test(const std::vector<double>& values) {
    std::vector<double> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    /////////////////////// Fixed Sketches /////////////////////////////
    {
      // single sketch
      sketch_type sketch(values.size(), epsilon);
      for (size_t i = 0;i < values.size(); ++i) {
        sketch.add(values[i]);
      }
      sketch.finalize();
      TS_ASSERT_EQUALS(sketch.size(), values.size());
      std::cout << "------ Sequential Sketching ------\n";
      std::cout << "Sketch size = " << sketch.memory_usage() << " bytes\n";
      compare_quantiles_at(sorted_values, sketch, 0);
      compare_quantiles_at(sorted_values, sketch, 0.01);
      compare_quantiles_at(sorted_values, sketch, 0.05);
      compare_quantiles_at(sorted_values, sketch, 0.50);
      compare_quantiles_at(sorted_values, sketch, 0.95);
      compare_quantiles_at(sorted_values, sketch, 0.99);
      compare_quantiles_at(sorted_values, sketch, 1);
      std::cout << "\n";


      // test multiple streams
      std::vector<sketch_type> sketches(16, sketch_type(values.size(), epsilon));
      for (size_t i = 0;i < values.size(); ++i) {
        sketches[i % sketches.size()].add(values[i]);
      }
      sketch_type final_sketch(values.size(), epsilon); 
      for (size_t i = 0;i < sketches.size(); ++i) {
        final_sketch.combine(sketches[i]);
      }
      final_sketch.finalize();
      TS_ASSERT_EQUALS(final_sketch.size(), values.size());
      std::cout << "------ 16-way Parallel Sketching of the same stream ------\n";
      std::cout << "Sketch size = " << final_sketch.memory_usage() << " bytes\n";
      compare_quantiles_at(sorted_values, final_sketch, 0);
      compare_quantiles_at(sorted_values, final_sketch, 0.01);
      compare_quantiles_at(sorted_values, final_sketch, 0.05);
      compare_quantiles_at(sorted_values, final_sketch, 0.50);
      compare_quantiles_at(sorted_values, final_sketch, 0.95);
      compare_quantiles_at(sorted_values, final_sketch, 0.99);
      compare_quantiles_at(sorted_values, final_sketch, 1);
      std::cout << "\n\n";
    }
    ////////////////////// Streaming ////////////////////////////////
    {
      // streaming sketch
      streaming_sketch_type sketch(epsilon);
      for (size_t i = 0;i < values.size(); ++i) {
        sketch.add(values[i]);
      }
      sketch.finalize();
      TS_ASSERT_EQUALS(sketch.size(), values.size());
      std::cout << "------ Sequential Streaming Sketching ------\n";
      std::cout << "Sketch size = " << sketch.memory_usage() << " bytes\n";
      compare_quantiles_at(sorted_values, sketch, 0);
      compare_quantiles_at(sorted_values, sketch, 0.01);
      compare_quantiles_at(sorted_values, sketch, 0.05);
      compare_quantiles_at(sorted_values, sketch, 0.50);
      compare_quantiles_at(sorted_values, sketch, 0.95);
      compare_quantiles_at(sorted_values, sketch, 0.99);
      compare_quantiles_at(sorted_values, sketch, 1);
      std::cout << "\n";


      // test multiple streams
      std::vector<streaming_sketch_type> sketches(16, streaming_sketch_type(epsilon));
      for (size_t i = 0;i < values.size(); ++i) {
        sketches[i % sketches.size()].add(values[i]);
      }
      streaming_sketch_type final_sketch(epsilon); 
      for (size_t i = 0;i < sketches.size(); ++i) {
        sketches[i].substream_finalize();
        final_sketch.combine(sketches[i]);
      }
      final_sketch.combine_finalize();
      TS_ASSERT_EQUALS(final_sketch.size(), values.size());
      std::cout << "------ 16-way Parallel Streaming Sketching of the same stream ------\n";
      std::cout << "Sketch size = " << final_sketch.memory_usage() << " bytes\n";
      compare_quantiles_at(sorted_values, final_sketch, 0);
      compare_quantiles_at(sorted_values, final_sketch, 0.01);
      compare_quantiles_at(sorted_values, final_sketch, 0.05);
      compare_quantiles_at(sorted_values, final_sketch, 0.50);
      compare_quantiles_at(sorted_values, final_sketch, 0.95);
      compare_quantiles_at(sorted_values, final_sketch, 0.99);
      compare_quantiles_at(sorted_values, final_sketch, 1);
      std::cout << "\n\n";
    }
  }

  std::vector<double> vals;

  void generate_gaussian_vals() {
    turi::random::seed(1001);
    for (size_t i = 0;i < vals.size(); ++i) {
      vals[i] = turi::random::gaussian(0, 10);
    }
  }
  void generate_gamma_vals() {
    turi::random::seed(1001);
    for (size_t i = 0;i < vals.size(); ++i) {
      vals[i] = turi::random::gamma();
    }
  }
  void generate_uniform_vals() {
    turi::random::seed(1001);
    for (size_t i = 0;i < vals.size(); ++i) {
      vals[i] = turi::random::uniform<double>(0, 1);
    }
  }
 public:
  quantile_sketch_test() {
    vals.resize(1000*1000);
  }
  void test_gaussian() {
    std::cout << "Gaussian:\n";
    generate_gaussian_vals();
    quantile_test(vals);
  }
  void test_sorted_gaussian() {
    std::cout << "Gaussian Sorted:\n";
    generate_gaussian_vals();
    std::sort(vals.begin(), vals.end());
    quantile_test(vals);
  }

  void test_gamma() {
    std::cout << "Gamma:\n";
    generate_gamma_vals();
    quantile_test(vals);
  }
  void test_sorted_gamma() {
    std::cout << "Gamma Sorted:\n";
    generate_gamma_vals();
    std::sort(vals.begin(), vals.end());
    quantile_test(vals);
  }

  void test_uniform() {
    std::cout << "Uniform:\n";
    generate_uniform_vals();
    quantile_test(vals);
  }
  void test_sorted_uniform() {
    std::cout << "Uniform Sorted:\n";
    generate_uniform_vals();
    std::sort(vals.begin(), vals.end());
    quantile_test(vals);
  }
};

BOOST_FIXTURE_TEST_SUITE(_quantile_sketch_test, quantile_sketch_test)
BOOST_AUTO_TEST_CASE(test_gaussian) {
  quantile_sketch_test::test_gaussian();
}
BOOST_AUTO_TEST_CASE(test_sorted_gaussian) {
  quantile_sketch_test::test_sorted_gaussian();
}
BOOST_AUTO_TEST_CASE(test_gamma) {
  quantile_sketch_test::test_gamma();
}
BOOST_AUTO_TEST_CASE(test_sorted_gamma) {
  quantile_sketch_test::test_sorted_gamma();
}
BOOST_AUTO_TEST_CASE(test_uniform) {
  quantile_sketch_test::test_uniform();
}
BOOST_AUTO_TEST_CASE(test_sorted_uniform) {
  quantile_sketch_test::test_sorted_uniform();
}
BOOST_AUTO_TEST_SUITE_END()
