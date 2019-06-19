/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <boost/range/combine.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/filesystem.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>

using namespace turi;

const flexible_type None = FLEX_UNDEFINED;

struct gl_sframe_test {
  public:
    void test_constructor() {
      gl_sframe sf{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}};
      _assert_sarray_equals(sf["a"], {1,2,3,4});
      _assert_sarray_equals(sf["b"], {"a","b","c","d"});
      sf["c"] = "x";
      _assert_sarray_equals(sf["c"], {"x","x","x","x"});
      sf["d"] = FLEX_UNDEFINED;
      _assert_sarray_equals(sf["d"], 
                            {FLEX_UNDEFINED, FLEX_UNDEFINED, FLEX_UNDEFINED, FLEX_UNDEFINED});
      TS_ASSERT_EQUALS((int)sf["d"].dtype(), (int)flex_type_enum::FLOAT);

      gl_sframe sf2;
      sf2["a"] = 1;
      sf2["b"] = 2;
     _assert_flexvec_equals(sf2[0], {1, 2});
    }

    void test_copy() {
      gl_sframe sf{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}};
      gl_sframe sf2(sf);
      sf2["c"] = "x";
      TS_ASSERT_EQUALS(sf.num_columns(), 2);
      TS_ASSERT_EQUALS(sf2.num_columns(), 3);

      gl_sframe sf3 = sf2;
      sf3.remove_column("c");
      TS_ASSERT_EQUALS(sf2.num_columns(), 3);
      TS_ASSERT_EQUALS(sf3.num_columns(), 2);

      _assert_sframe_equals(sf, sf3);
      _assert_sarray_equals(sf2["c"], {"x", "x", "x", "x"});
    }

    void test_basic_indexing_and_ranges() {
      gl_sframe sf(_make_reference_frame());
      _assert_flexvec_equals(sf[0], {1, "a"});
      auto res = sf[sf["a"] < 5];
      TS_ASSERT_EQUALS(res.size(), 4);
      _assert_sframe_equals(res, gl_sframe{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}});
      _assert_sframe_equals(sf[{0, 4}], gl_sframe{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}});
      TS_ASSERT_EQUALS((int)sf.column_types()[0], (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)sf.column_types()[1], (int)flex_type_enum::STRING);
    }

    void test_head_and_tail() {
      gl_sframe sf(_make_reference_frame());
      _assert_sframe_equals(sf.head(4), gl_sframe{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}});
      _assert_sframe_equals(sf.tail(4), gl_sframe{{"a", {7,8,9,10}},{"b", {"g","h","i","j"}}});
    }

    void test_apply() {
      gl_sframe sf(_make_reference_frame());
      sf["c"] = sf.apply([](const std::vector<flexible_type>& f) { return f[0]; }, 
                         flex_type_enum::INTEGER);
      _assert_sarray_equals(sf["a"], _to_vec(sf["c"]));
    }

    void test_sample() {
      gl_sframe sf(_make_reference_frame());
      gl_sframe sf2 = sf.sample(0.3);
      TS_ASSERT_LESS_THAN_EQUALS(sf2.size(), sf.size());

      gl_sframe sf3{{"a", {1,2,3,4,5}},
                  {"b", {1.0,2.0,3.0,4.0,5.0}}};
      std::cout <<  sf3;
      std::cout <<  sf3.sample(.3);
      std::cout <<  sf3.sample(.3, 12345);
    }  
    void test_sample_split() {
      gl_sframe sf(_make_reference_frame());
      {
        gl_sframe sfa, sfb;
        std::tie(sfa, sfb) = sf.random_split(0.3);
        gl_sframe sfc = sfa.append(sfb);
        _assert_sframe_equals(sf, sfc.sort("a"));
      }

      {
        auto sf = gl_sframe({{"id", gl_sarray::from_sequence(0, 1024)}});
        gl_sframe sf_train, sf_test;
        std::tie(sf_train, sf_test) = sf.random_split(.95, 12345);
        std::cout <<  sf_test.size() << " " << sf_train.size() << "\n";
      }

      {
        gl_sframe sfa, sfb;
        std::tie(sfa, sfb) = sf.random_split(0);
        TS_ASSERT_EQUALS(sfa.size(), 0);
        _assert_sframe_equals(sf, sfb);
      }

      {
        gl_sframe sfa, sfb;
        std::tie(sfa, sfb) = sf.random_split(1);
        TS_ASSERT_EQUALS(sfb.size(), 0);
        _assert_sframe_equals(sf, sfa);
      }

    }

    void test_groupby() {
      gl_sframe sf;
      sf["a"] = gl_sarray({"a","a","a","a","a","b","b","b","b","b"});
      sf["b"] = 2;
      gl_sframe sf2 = sf.groupby({"a"}, {{"bsum", aggregate::SUM("b")}, {"bcount", aggregate::COUNT()}}).sort("a");
      _assert_sframe_equals(sf2, gl_sframe{{"a", {"a","b"}}, {"bsum", {10, 10}}, {"bcount", {5, 5}}});
    }

    void test_vector_groupby(){
      gl_sframe sf;
      sf["a"] = gl_sarray({"a","a","b","b"});
      sf["b"] = gl_sarray({{1.0,2.0,3.0},{1.0,2.0,3.0},{1.0,2.0,3.0},{1.0,2.0,3.0}});
      gl_sframe sf2 = sf.groupby({"a"}, {{"bsum", aggregate::SUM("b")}, {"bmean", aggregate::MEAN("b")}}).sort("a");
      _assert_sframe_equals(sf2, gl_sframe{{"a", {"a","b"}}, {"bsum", {{2.0,4.0,6.0},{2.0,4.0,6.0}}}, {"bmean", {{1.0,2.0,3.0},{1.0,2.0,3.0}}}});
    }

    void test_user_defined_gourpby() {
      gl_sframe sf;
      sf["a"] = gl_sarray({"a","a","a","a","a","b","b","b","b","b"});
      sf["b"] = 2;

      /**
       * User defined groupby aggregator which sum the log of values
       */
      class log_sum : public group_aggregate_value {
       public:
        log_sum() { acc = 0.0; }

        group_aggregate_value* new_instance() const {
          return new log_sum();
        }

        void add_element_simple(const flexible_type& flex) {
          acc += log2((double)flex);
        }

        void combine(const group_aggregate_value& other) {
          acc += ((const log_sum&)(other)).acc;
        }

        bool support_type(flex_type_enum type) const {
          return type == flex_type_enum::INTEGER || type == flex_type_enum::FLOAT;
        }

        flexible_type emit() const {
          return acc;
        }

        std::string name() const {
          return "log_sum";
        }

        void save(oarchive& oarc) const {
          oarc << acc;
        }

        void load(iarchive& iarc) {
          iarc >> acc;
        }

        double acc;
      };

      gl_sframe sf2 = sf.groupby({"a"},
                                 {{"blog_sum", aggregate::make_aggregator<log_sum>({"b"})}}).sort("a");
      _assert_sframe_equals(sf2, gl_sframe{{"a", {"a","b"}},
                                           {"blog_sum", {5., 5.}}});
    }

    void test_topk() {
      gl_sframe sf(_make_reference_frame());
      _assert_sframe_equals(sf.topk("b", 4), gl_sframe{{"a", {10,9,8,7}},{"b", {"j","i","h","g"}}});
      _assert_sframe_equals(sf.topk("b", 4, true), gl_sframe{{"a", {1,2,3,4}},{"b", {"a","b","c","d"}}});
    }

    void test_join() {
      gl_sframe sf(_make_reference_frame());
      gl_sframe sf2(_make_reference_frame());

      sf2.rename({{"b", "c"}});
      gl_sframe sf3 = sf.join(sf2, {"a"}, "left");

      sf["c"] = sf["b"];
      _assert_sframe_equals(sf3, sf); 
    }

    void test_pack_unpack() {
      auto reference = _make_reference_frame();
      gl_sframe sf(reference.pack_columns(reference.column_names(), "X1"));
      
      gl_sarray sa = _make_reference_frame()
          .apply([](const std::vector<flexible_type>& f) { return f; }, 
                 flex_type_enum::LIST);

      _assert_sarray_equals(sf["X1"], _to_vec(sa));

      auto sf2 = sf.unpack("X1");
      auto colnames = sf2.column_names();
      sf2.rename({{colnames[0],"a"},{colnames[1], "b"}});
      _assert_sframe_equals(_make_reference_frame(), sf2);
    }
    void test_pack_unpack2() {

     auto sf = gl_sframe({{"business", {1,2,3,4}},
                          {"category.retail", {1, FLEX_UNDEFINED, 1, FLEX_UNDEFINED}},
                          {"category.food", {1, 1, FLEX_UNDEFINED, FLEX_UNDEFINED}},
                          {"category.service", {FLEX_UNDEFINED, 1, 1, FLEX_UNDEFINED}},
                          {"category.shop", {1, 1, FLEX_UNDEFINED, 1}}});
     std::cout << sf;
     std::cout <<  sf.pack_columns({"category.retail", "category.food", 
               "category.service", "category.shop"}, 
               "category");

     std::cout << sf.pack_columns({"category.retail", "category.food", 
               "category.service", "category.shop"}, 
               "category", 
               flex_type_enum::DICT);
    }

    void test_stack_unstack() {
      gl_sframe sf(_make_stacking_frame().unstack("a", "a").sort("b"));
      gl_sframe sf2 = _make_stacking_frame().groupby({"b"}, {{"a", aggregate::CONCAT("a")}}).sort("b");
      // to compare equality, need to make sure the unstacked group has the same order
      // since the unstacking can be in arbitrary ordering
      // i.e. we need to sort it.
      auto group_sort = 
          [](const flexible_type& x) -> flexible_type{
            flex_list v = x;
            std::sort(v.begin(), v.end());
            return v;
          };
      sf["a"] = sf["a"].apply(group_sort, flex_type_enum::LIST);
      sf2["a"] = sf2["a"].apply(group_sort, flex_type_enum::LIST);


      _assert_sframe_equals(sf, sf2);
      std::cout << sf << "\n";
      auto sf3 = sf.stack("a","a").sort("a");
      auto sf4 = _make_stacking_frame();
      sf4 = sf4[sf3.column_names()];
      sf3["a"] = sf3["a"].astype(flex_type_enum::INTEGER);
      _assert_sframe_equals(sf3, sf4);
    }
    void test_unique() {
      _assert_sframe_equals(_make_reference_frame().unique().sort("a"), _make_reference_frame());
      gl_sframe sf;
      sf["a"] = gl_sarray({1,1,2,2});
      sf["b"] = gl_sarray({"a","a","b","b"});
      _assert_sframe_equals(sf.unique().sort("a"), gl_sframe{{"a",{1,2}},{"b",{"a","b"}}});
    }

    void test_drop_na() {
      gl_sframe sf;
      sf["a"] = gl_sarray({1,FLEX_UNDEFINED,2,2});
      sf["b"] = gl_sarray({"a","a",FLEX_UNDEFINED,"b"});
      gl_sframe sf2 = sf.dropna({"a","b"}, "any");
      _assert_sframe_equals(sf2, gl_sframe{{"a",{1,2}},{"b",{"a","b"}}});
      _assert_sframe_equals(sf.dropna({"a","b"}, "all"), sf);
      gl_sframe sf3 = sf.fillna("a", 1).fillna("b","b");
      _assert_sframe_equals(sf3, gl_sframe{{"a",{1,1,2,2}},{"b",{"a","a","b","b"}}});
    }

    void test_writer() {
      gl_sframe_writer writer({"a","b"},
                              {flex_type_enum::INTEGER, flex_type_enum::STRING});
      // write one integer and one string into each segment, so we get [{0, "0"}, {1, "1"}... {100, "100"}]
      // then write a few more integers to the end
      for (size_t i = 0;i < writer.num_segments(); ++i) {
        writer.write({i, std::to_string(i)}, i);
      }
      std::vector<std::vector<flexible_type> > values;
      for (size_t i = writer.num_segments(); i < 100; ++i) values.push_back({i, std::to_string(i)});
      writer.write(values.begin(), values.end(), writer.num_segments() - 1);
      gl_sframe frame = writer.close();

      // done!
      gl_sframe expected;
      expected["a"] = gl_sarray::from_sequence(0, 100);
      expected["b"] = expected["a"].astype(flex_type_enum::STRING);
      _assert_sframe_equals(frame, expected);
    }

    void test_logical_filter() {
      gl_sframe g({{"a",{1,2,3,4,5}}, {"id",{1,2,3,4,5}}});
      g = g[g["id"] > 2];
      _assert_sarray_equals(g[{0, 2}]["id"], {3, 4});
    }

    void test_filter_by() {
      gl_sframe g({{"a",{1,2,3,4,5}}, {"id",{1,2,3,4,5}}});
      g = g.filter_by({3,4}, "a");
      TS_ASSERT_EQUALS(g.size(), 2);
      _assert_sarray_equals(g["id"], {3, 4});
      _assert_sarray_equals(g["id"], {3, 4});
    }

    void test_filter_by_exclude() {
      gl_sframe g({{"a",{1,2,3,4,5}}, {"id",{1,2,3,4,5}}});
      g = g.filter_by({1,2,5}, "a", true);
      TS_ASSERT_EQUALS(g.size(), 2);
      _assert_sarray_equals(g["id"], {3, 4});
      _assert_sarray_equals(g["id"], {3, 4});
    }

    void test_save() {
      gl_sframe g({{"a",{1,2,3,4,5}}, {"id",{1,2,3,4,5}}});
      boost::filesystem::path temp = boost::filesystem::unique_path();
      const std::string tempstr    = temp.string();  // optional
      g.save(tempstr);

      gl_sframe g2(tempstr);
      _assert_sarray_equals(g["a"], {1,2,3,4,5});
      _assert_sarray_equals(g["id"], {1,2,3,4,5});
    }

    void test_parallel_range_iterator() {
      turi::gl_sframe sf;
      sf.add_column(gl_sarray::from_const(0, 1000), "src_1");
      sf.add_column(gl_sarray::from_const(1, 1000), "src_2");
      size_t sf_size = sf.size();
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        size_t start_idx = sf_size * thread_idx / num_threads;
        size_t end_idx = sf_size * (thread_idx + 1) / num_threads;
        for (const auto& v: sf.range_iterator(start_idx, end_idx)) { 
          TS_ASSERT_EQUALS((int)v[0], 0);
          TS_ASSERT_EQUALS((int)v[1], 1);
        }
      });
    }

    void test_lazy_iteration() {
      turi::gl_sframe sf;
      sf.add_column(gl_sarray::from_const(0, 1000), "src_1");
      sf.add_column(gl_sarray::from_const(1, 1000), "src_2");
      atomic<size_t> ctr;
      sf.materialize_to_callback([&](size_t thrid,
                                    const std::shared_ptr<sframe_rows>& rows) {
                                      ctr += rows->num_rows();
                                      return false;
                                    });
      TS_ASSERT_EQUALS(ctr.value, 1000);
    }

  void test_sframe_casts() {
    gl_sframe sf = _make_reference_frame();

    sframe sf2 = sf.materialize_to_sframe();
    gl_sframe sf3 = sf2;

    _assert_sframe_equals(sf, sf3);
  }

  void test_sframe_contains_column() {
    gl_sframe sf = _make_reference_frame();

    TS_ASSERT(sf.contains_column(sf.column_name(0)));
    TS_ASSERT(sf.contains_column(sf.column_name(1)));
    TS_ASSERT_EQUALS(sf.column_index(sf.column_name(0)), 0);
    TS_ASSERT_EQUALS(sf.column_index(sf.column_name(1)), 1);
    TS_ASSERT(!sf.contains_column("the-column-of-awesome"));
  }

  void test_sframe_sort_repeated() {
    // The following code would hit some bug conditions related to cache
    // invalidation.
    gl_sframe sf(make_random_sframe(4, "nnn"));

    for(size_t i = 0; i < 500; ++i) {
      sf["X1-n"] = gl_sframe(make_random_sframe(4, "n"))["X1-n"];
      sf.sort("X1-n");
    }
  }

  void test_sframe_iterator_invalidation() {
    // This tests the root cause of the sort bug above.
    gl_sframe sf;
    sf["col"] = gl_sarray{0};

    sf.range_iterator();

    sf["col"] = gl_sarray{1};

    TS_ASSERT_EQUALS((*sf.range_iterator().begin())[0], 1);
  }


    gl_sframe _make_reference_frame() {
      gl_sframe sf;
      sf["a"] = gl_sarray::from_sequence(1, 11);
      sf["b"] = gl_sarray({"a","b","c","d","e","f","g","h","i","j"});
      return sf;
    }

    gl_sframe _make_stacking_frame() {
      gl_sframe sf;
      sf["a"] = gl_sarray::from_sequence(1, 11);
      sf["b"] = gl_sarray({"a","a","a","a","a","b","b","b","b","b"});
      return sf;
    }

    std::vector<flexible_type> _to_vec(gl_sarray sa) {
      std::vector<flexible_type> ret;
      for (auto& v: sa.range_iterator()) { ret.push_back(v); }
      return ret;
    }

    void _assert_flexvec_equals(const std::vector<flexible_type>& sa, 
                                const std::vector<flexible_type>& sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      for (size_t i = 0;i < sa.size() ;++i) {
        if (sa[i] != sb[i]) {
           std::cout << sa[i] << " vs " << sb[i] << std::endl;
        }
        TS_ASSERT_EQUALS(sa[i], sb[i]);
      }
    }
    void _assert_sarray_equals(gl_sarray sa, const std::vector<flexible_type>& vec) {
      TS_ASSERT_EQUALS(sa.size(), vec.size());
      for (size_t i = 0; i < vec.size(); ++i) {
        TS_ASSERT_EQUALS(sa[i], vec[i]);
      }
    }
    std::vector<std::vector<flexible_type> > _to_vec(gl_sframe sa) {
      std::vector<std::vector<flexible_type> > ret;
      for (auto& v: sa.range_iterator()) { ret.push_back(v); }
      return ret;
    }

    void _assert_sframe_equals(gl_sframe sa, gl_sframe sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      TS_ASSERT_EQUALS(sa.num_columns(), sb.num_columns());
      auto a_cols = sa.column_names();
      auto b_cols = sb.column_names();
      std::sort(a_cols.begin(), a_cols.end());
      std::sort(b_cols.begin(), b_cols.end());
      for (size_t i = 0;i < a_cols.size(); ++i) TS_ASSERT_EQUALS(a_cols[i], b_cols[i]);
      sb = sb[sa.column_names()];
      for (size_t i = 0; i < sa.size(); ++i) {
        _assert_flexvec_equals(sa[i], sb[i]);
      }
    }

};

BOOST_FIXTURE_TEST_SUITE(_gl_sframe_test, gl_sframe_test)
BOOST_AUTO_TEST_CASE(test_constructor) {
  gl_sframe_test::test_constructor();
}
BOOST_AUTO_TEST_CASE(test_copy) {
  gl_sframe_test::test_copy();
}
BOOST_AUTO_TEST_CASE(test_basic_indexing_and_ranges) {
  gl_sframe_test::test_basic_indexing_and_ranges();
}
BOOST_AUTO_TEST_CASE(test_head_and_tail) {
  gl_sframe_test::test_head_and_tail();
}
BOOST_AUTO_TEST_CASE(test_apply) {
  gl_sframe_test::test_apply();
}
BOOST_AUTO_TEST_CASE(test_sample) {
  gl_sframe_test::test_sample();
}
BOOST_AUTO_TEST_CASE(test_sample_split) {
  gl_sframe_test::test_sample_split();
}
BOOST_AUTO_TEST_CASE(test_groupby) {
  gl_sframe_test::test_groupby();
}
BOOST_AUTO_TEST_CASE(test_vector_groupby) {
  gl_sframe_test::test_vector_groupby();
}
BOOST_AUTO_TEST_CASE(test_user_defined_gourpby) {
  gl_sframe_test::test_user_defined_gourpby();
}
BOOST_AUTO_TEST_CASE(test_topk) {
  gl_sframe_test::test_topk();
}
BOOST_AUTO_TEST_CASE(test_join) {
  gl_sframe_test::test_join();
}
BOOST_AUTO_TEST_CASE(test_pack_unpack) {
  gl_sframe_test::test_pack_unpack();
}
BOOST_AUTO_TEST_CASE(test_pack_unpack2) {
  gl_sframe_test::test_pack_unpack2();
}
BOOST_AUTO_TEST_CASE(test_stack_unstack) {
  gl_sframe_test::test_stack_unstack();
}
BOOST_AUTO_TEST_CASE(test_unique) {
  gl_sframe_test::test_unique();
}
BOOST_AUTO_TEST_CASE(test_drop_na) {
  gl_sframe_test::test_drop_na();
}
BOOST_AUTO_TEST_CASE(test_writer) {
  gl_sframe_test::test_writer();
}
BOOST_AUTO_TEST_CASE(test_logical_filter) {
  gl_sframe_test::test_logical_filter();
}
BOOST_AUTO_TEST_CASE(test_filter_by) {
  gl_sframe_test::test_filter_by();
}
BOOST_AUTO_TEST_CASE(test_filter_by_exclude) {
  gl_sframe_test::test_filter_by_exclude();
}
BOOST_AUTO_TEST_CASE(test_save) {
  gl_sframe_test::test_save();
}
BOOST_AUTO_TEST_CASE(test_parallel_range_iterator) {
  gl_sframe_test::test_parallel_range_iterator();
}
BOOST_AUTO_TEST_CASE(test_lazy_iteration) {
  gl_sframe_test::test_lazy_iteration();
}
BOOST_AUTO_TEST_CASE(test_sframe_casts) {
  gl_sframe_test::test_sframe_casts();
}
BOOST_AUTO_TEST_CASE(test_sframe_contains_column) {
  gl_sframe_test::test_sframe_contains_column();
}

BOOST_AUTO_TEST_CASE(test_sframe_sort_repeated) {
  gl_sframe_test::test_sframe_sort_repeated();
}
BOOST_AUTO_TEST_CASE(test_sframe_iterator_invalidation) {
  gl_sframe_test::test_sframe_iterator_invalidation();
}

BOOST_AUTO_TEST_SUITE_END()
