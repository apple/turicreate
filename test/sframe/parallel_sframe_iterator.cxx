#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <iterator>

using namespace turi;

struct parallel_sframe_iterator_test  {
 public:

  void test_simple_explicit_1_iter() {
    sframe sf = make_testing_sframe({"A", "B"}, {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                    { {1, 2}, {2, 3}, {4, 5}, {6, 7}, {8, 9} });

    DASSERT_EQ(sf.size(), 5);

    std::vector<int> hit_row(5,false);

    for(parallel_sframe_iterator it(sf); !it.done(); ++it) {
      DASSERT_FALSE(hit_row[it.row_index()]);
      hit_row[it.row_index()] = true;
    }

    for(size_t i = 0; i < hit_row.size(); ++i) {
      DASSERT_TRUE(hit_row[i]);
    }
  }

  void test_simple_explicit_4_iter() {
    sframe sf = make_testing_sframe({"A", "B"}, {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                    { {1, 2}, {2, 3}, {4, 5}, {6, 7}, {8, 9} });

    DASSERT_EQ(sf.size(), 5);

    std::vector<int> hit_row(5,false);

    parallel_sframe_iterator_initializer it_init(sf);
    for(size_t i = 0; i < 4; ++i) {
      for(parallel_sframe_iterator it(it_init, i, 4); !it.done(); ++it) {
        DASSERT_FALSE(hit_row[it.row_index()]);
        hit_row[it.row_index()] = true;
      }
    }

    for(size_t i = 0; i < hit_row.size(); ++i) {
      DASSERT_TRUE(hit_row[i]);
    }
  }

  void test_simple_explicit_4_iter_no_initializer() {
    sframe sf = make_testing_sframe({"A", "B"}, {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                    { {1, 2}, {2, 3}, {4, 5}, {6, 7}, {8, 9} });

    DASSERT_EQ(sf.size(), 5);

    std::vector<int> hit_row(5,false);

    for(size_t i = 0; i < 4; ++i) {
      for(parallel_sframe_iterator it(sf, i, 4); !it.done(); ++it) {
        DASSERT_FALSE(hit_row[it.row_index()]);
        hit_row[it.row_index()] = true;
      }
    }

    for(size_t i = 0; i < hit_row.size(); ++i) {
      DASSERT_TRUE(hit_row[i]);
    }
  }

  void test_simple_explicit_parallel() {
    sframe sf = make_testing_sframe({"A", "B"}, {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                    { {1, 2}, {2, 3}, {4, 5}, {6, 7}, {8, 9} });

    DASSERT_EQ(sf.size(), 5);

    std::vector<int> hit_row(5, false);

    parallel_for(size_t(0), 4, [&](size_t i) {
        for(parallel_sframe_iterator it(sf, i, 4); !it.done(); ++it) {
          DASSERT_FALSE(hit_row[it.row_index()]);
          hit_row[it.row_index()] = true;
        }
      });

    for(size_t i = 0; i < hit_row.size(); ++i) {
      DASSERT_TRUE(hit_row[i]);
    }
  }


  void _test_correct(const std::vector<size_t>& num_columns_by_sframe, size_t num_elements,
                     const std::vector<size_t>& num_threads_to_check) {

    std::vector<sframe> sfv;
    
    size_t num_segments = 16;

    size_t cur_value = 0; 
    
    for(size_t i = 0; i < num_columns_by_sframe.size(); ++i) {
      size_t num_columns = num_columns_by_sframe[i];

      // Set up the sframe
      std::vector<std::string> names;
      std::vector<flex_type_enum> types;

      for(size_t i = 0; i < num_columns; ++i) {
        names.push_back(std::string("X") + std::to_string(sfv.size()) + "-" + std::to_string(i));
        types.push_back(flex_type_enum::INTEGER); 
      }
      
      sframe out;

      out.open_for_write(names, types, "", num_segments);

      std::vector<flexible_type> x(num_columns); 

      for(size_t sidx = 0; sidx < num_segments; ++sidx) {

        auto it_out = out.get_output_iterator(sidx);

        size_t start_idx = (sidx * num_elements) / num_segments;
        size_t end_idx   = ((sidx+1) * num_elements) / num_segments;

        for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
          for(size_t j = 0; j < num_columns; ++j) {
            x[j] = cur_value;
            ++cur_value;
          }
          
          *it_out = x;
        }
      }
      
      out.close();

      sfv.push_back(out); 
    }

    // Build the reference
    std::vector<std::vector<std::vector<flexible_type> > > reference(sfv.size()); 
    
    for(size_t i = 0; i < sfv.size(); ++i)
      reference[i] = testing_extract_sframe_data(sfv[i]);

    // Now run the checks
    parallel_sframe_iterator_initializer it_iter(sfv);

    mutex write_lock;
    atomic<size_t> hit_count = 0;
    
    // Per SFrame check
    auto check_sframe = [&](size_t sf_idx,
                            size_t thread_idx, size_t nt,
                            std::vector<std::vector<flexible_type> >& check_x) {


      for(parallel_sframe_iterator it(it_iter, thread_idx, nt); !it.done(); ++it) {

        auto& x = check_x[it.row_index()];
        
        it.fill(sf_idx, x);
            
        TS_ASSERT_EQUALS(x.size(), sfv[sf_idx].num_columns()); 

        for(size_t j = 0; j < sfv[sf_idx].num_columns(); ++j) {
          TS_ASSERT_EQUALS(x[j], reference[sf_idx][it.row_index()][j]); 
          TS_ASSERT_EQUALS(it.value(sf_idx, j), x[j]);
        }
        ++hit_count;
      }
    };

    // Full row check 
    auto check_all = [&](
        size_t thread_idx, size_t num_threads) {
      
      size_t total_num_columns = 0;
      for(const auto& sf : sfv)
        total_num_columns += sf.num_columns();
      
      std::vector<flexible_type> x; 
      for(parallel_sframe_iterator it(it_iter, thread_idx, num_threads); !it.done(); ++it) {
        
        it.fill(x);
            
        TS_ASSERT_EQUALS(x.size(), total_num_columns); 

        size_t col_idx = 0;
        
        for(size_t sf_idx = 0; sf_idx < sfv.size(); ++sf_idx) {
          for(size_t j = 0; j < sfv[sf_idx].num_columns(); ++j) {
            TS_ASSERT_EQUALS(x[col_idx], reference[sf_idx][it.row_index()][j]);
            TS_ASSERT_EQUALS(it.value(col_idx), x[col_idx]);
            ++col_idx; 
          }
        }

        ++hit_count;
      }
    };

    // Check block reading.
    size_t mbStart = 1;
    size_t mbEnd = 3;
    parallel_sframe_iterator_initializer it_iter_block(sfv); 
    it_iter_block.set_global_block(mbStart, mbEnd);
    auto check_all_block = [&](
        size_t thread_idx, size_t num_threads) {
      
      size_t total_num_columns = 0;
      for(const auto& sf : sfv)
        total_num_columns += sf.num_columns();
      
      std::vector<flexible_type> x; 
      for(parallel_sframe_iterator it(it_iter_block, thread_idx, num_threads);
          !it.done(); ++it) {
        
        it.fill(x);
            
        TS_ASSERT_EQUALS(x.size(), total_num_columns); 
        TS_ASSERT(it.row_index() >= mbStart); 
        TS_ASSERT(it.row_index() < mbEnd); 

        size_t col_idx = 0;
        for(size_t sf_idx = 0; sf_idx < sfv.size(); ++sf_idx) {
          for(size_t j = 0; j < sfv[sf_idx].num_columns(); ++j) {
            TS_ASSERT_EQUALS(x[col_idx], reference[sf_idx][it.row_index()][j]);
            TS_ASSERT_EQUALS(it.value(col_idx), x[col_idx]);
            ++col_idx; 
          }
        }
      }
    };

    for(size_t sf_idx = 0; sf_idx < sfv.size(); ++sf_idx) {
      for(size_t nt : num_threads_to_check) {
        std::vector<std::vector<flexible_type> > check_x;
        check_x.resize(reference[sf_idx].size()); 

        hit_count = 0;
        for(size_t thread_idx = 0; thread_idx < nt; ++thread_idx)
          check_sframe(sf_idx, thread_idx, nt, check_x);

        TS_ASSERT(check_x == reference[sf_idx]); 

        TS_ASSERT_EQUALS(sfv[0].size(), size_t(hit_count));
      }

      {
        std::vector<std::vector<flexible_type> > check_x;
        check_x.resize(reference[sf_idx].size());

        hit_count = 0;
        // Now do the same, but in parallel 
        in_parallel([&](size_t thread_idx, size_t nt) { check_sframe(sf_idx, thread_idx, nt, check_x); } );

        TS_ASSERT_EQUALS(sfv[0].size(), size_t(hit_count));
        
        TS_ASSERT(check_x == reference[sf_idx]);
      }
    }
    
    // Now, do the same, but considering the full vector. 
    for(size_t nt : num_threads_to_check) {
      hit_count = 0;
      for(size_t thread_idx = 0; thread_idx < nt; ++thread_idx)
        check_all(thread_idx, nt);

      TS_ASSERT_EQUALS(sfv[0].size(), size_t(hit_count));
    }
      
    // Now do the same, but in parallel 
    in_parallel(check_all);   
    in_parallel(check_all_block);   
  }

  void test_tiny_1() {  _test_correct({1}, 100, {1}); } 
  void test_tiny_2() {  _test_correct({1}, 4, {1, 4, 16});   }
  void test_tiny_3() {  _test_correct({1, 1}, 100, {1}); }
  void test_tiny_4() {  _test_correct({1, 1}, 4, {1, 4, 16});   }
  void test_tiny_5() {  _test_correct({1, 2, 3, 4}, 4, {1, 4, 16});   }
  void test_tiny_6() {  _test_correct({1, 2, 3, 4}, 100, {1, 4, 16});   }

  void test_medium() {  _test_correct({1, 2, 3, 4}, 1000, {1, 4, 16}); }

  void test_many_parallel() {  _test_correct({1, 2, 3, 4}, 100, {1000}); }

  void test_large() {  _test_correct({1, 2}, 1000000, {2}); }

  void _run_test_writing_out(size_t n) {

    std::vector<std::vector<flexible_type> > data(n);
    std::vector<size_t> correct(n);

    size_t v = 0;
    for(size_t i = 0; i < n; ++i) {
      data[i].resize(3);
      correct[i] = 0;
      for(size_t j = 0; j < 3; ++j) {
        data[i][j] = ++v;
        correct[i] += v;
      }
    }

    sframe sf = make_testing_sframe({"A", "B", "C"},
                                    {flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                    data);

    // Write it out
    const size_t max_n_threads = thread::cpu_count();

    std::shared_ptr<sarray<flexible_type> > out(new sarray<flexible_type>);

    size_t num_segments = max_n_threads;

    out->open_for_write(num_segments);
    out->set_type(flex_type_enum::INTEGER);

    parallel_sframe_iterator_initializer it_init(sf);

    atomic<size_t> n_writes = 0;
  
    in_parallel([&](size_t thread_idx, size_t n_threads) {

        auto it_out = out->get_output_iterator(thread_idx);

        std::vector<flexible_type> x;

        for(parallel_sframe_iterator it(it_init, thread_idx, n_threads); !it.done(); ++it, ++it_out) {
          it.fill(x);

          size_t out_value = 0;
          for(size_t v : x)
            out_value += v;

          *it_out = out_value;
          ++n_writes;
        }
      });

    DASSERT_EQ(n_writes, n);

    out->close();

    DASSERT_EQ(out->size(), n);

    std::vector<size_t> out_res = testing_extract_column<size_t>(out);

    for(size_t i = 0; i < n; ++i) {
      DASSERT_EQ(size_t(out_res[i]), correct[i]);
    }
  }

  void test_simultaneous_write_small_1() {
    _run_test_writing_out(1);
  }

  void test_simultaneous_write_small_2() {
    _run_test_writing_out(2);
  }

  void test_simultaneous_write_small_10() {
    _run_test_writing_out(10);
  }

  void test_simultaneous_write_small_100() {
    _run_test_writing_out(100);
  }

};

BOOST_FIXTURE_TEST_SUITE(_parallel_sframe_iterator_test, parallel_sframe_iterator_test)
BOOST_AUTO_TEST_CASE(test_simple_explicit_1_iter) {
  parallel_sframe_iterator_test::test_simple_explicit_1_iter();
}
BOOST_AUTO_TEST_CASE(test_simple_explicit_4_iter) {
  parallel_sframe_iterator_test::test_simple_explicit_4_iter();
}
BOOST_AUTO_TEST_CASE(test_simple_explicit_4_iter_no_initializer) {
  parallel_sframe_iterator_test::test_simple_explicit_4_iter_no_initializer();
}
BOOST_AUTO_TEST_CASE(test_simple_explicit_parallel) {
  parallel_sframe_iterator_test::test_simple_explicit_parallel();
}
BOOST_AUTO_TEST_CASE(test_tiny_1) {
  parallel_sframe_iterator_test::test_tiny_1();
}
BOOST_AUTO_TEST_CASE(test_tiny_2) {
  parallel_sframe_iterator_test::test_tiny_2();
}
BOOST_AUTO_TEST_CASE(test_tiny_3) {
  parallel_sframe_iterator_test::test_tiny_3();
}
BOOST_AUTO_TEST_CASE(test_tiny_4) {
  parallel_sframe_iterator_test::test_tiny_4();
}
BOOST_AUTO_TEST_CASE(test_tiny_5) {
  parallel_sframe_iterator_test::test_tiny_5();
}
BOOST_AUTO_TEST_CASE(test_tiny_6) {
  parallel_sframe_iterator_test::test_tiny_6();
}
BOOST_AUTO_TEST_CASE(test_medium) {
  parallel_sframe_iterator_test::test_medium();
}
BOOST_AUTO_TEST_CASE(test_many_parallel) {
  parallel_sframe_iterator_test::test_many_parallel();
}
BOOST_AUTO_TEST_CASE(test_large) {
  parallel_sframe_iterator_test::test_large();
}
BOOST_AUTO_TEST_CASE(test_simultaneous_write_small_1) {
  parallel_sframe_iterator_test::test_simultaneous_write_small_1();
}
BOOST_AUTO_TEST_CASE(test_simultaneous_write_small_2) {
  parallel_sframe_iterator_test::test_simultaneous_write_small_2();
}
BOOST_AUTO_TEST_CASE(test_simultaneous_write_small_10) {
  parallel_sframe_iterator_test::test_simultaneous_write_small_10();
}
BOOST_AUTO_TEST_CASE(test_simultaneous_write_small_100) {
  parallel_sframe_iterator_test::test_simultaneous_write_small_100();
}
BOOST_AUTO_TEST_SUITE_END()
