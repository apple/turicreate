#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <capi/TuriCore.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <vector>
#include <iostream>
#include <ctime>
#include <unity/extensions/random_sframe_generation.hpp>
#include "capi_utils.hpp"

BOOST_AUTO_TEST_CASE(test_sframe_allocation) {
    tc_error* error = NULL;

    tc_sframe* sf = tc_sframe_create_empty(&error);

    TS_ASSERT(error == NULL);

    tc_sframe_destroy(sf);
  }

BOOST_AUTO_TEST_CASE(test_sframe_save_load) {

std::vector<std::pair<std::string, std::vector<double> > > data
  = { {"col1", {1.0, 2., 5., 0.5} },
      {"col2", {2.0, 2., 3., 0.5} },
      {"a",    {5.0, 2., 1., 0.5} },
      {"b",    {7.0, 2., 3., 1.5} } };

for(const char* url : {"sf_tmp_1/"} ) {

    tc_error* error = NULL;

    tc_sframe* sf_src = tc_sframe_create_empty(&error);

    TS_ASSERT(error == NULL);

    for(auto p : data) {

      tc_sarray* sa = make_sarray_double(p.second);

      tc_sframe_add_column(sf_src, p.first.c_str(), sa, &error);

      TS_ASSERT(error == NULL);

      tc_sarray_destroy(sa);
    }

    tc_sframe_save(sf_src, url, &error); 

    TS_ASSERT(error == NULL);

    tc_sframe_destroy(sf_src); 

    tc_sframe* sf = tc_sframe_load(url, &error); 

    TS_ASSERT(error == NULL);

    // Check everything
    for(auto p : data) {
      // Make sure it gets out what we want it to.
      tc_sarray* sa = tc_sframe_extract_column_by_name(sf, p.first.c_str(), &error);

      TS_ASSERT(error == NULL);

      tc_sarray* ref_sa = make_sarray_double(p.second);

      int is_equal = tc_sarray_equals(sa, ref_sa, &error);
      TS_ASSERT(is_equal);

      TS_ASSERT(error == NULL);

      tc_sarray_destroy(sa);
    }

    tc_sframe_destroy(sf);
  }
}


BOOST_AUTO_TEST_CASE(test_sframe_double) {

std::vector<std::pair<std::string, std::vector<double> > > data
  = { {"col1", {1.0, 2., 5., 0.5} },
      {"col2", {2.0, 2., 3., 0.5} },
      {"a",    {5.0, 2., 1., 0.5} },
      {"b",    {7.0, 2., 3., 1.5} } };

    tc_error* error = NULL;

    tc_sframe* sf = tc_sframe_create_empty(&error);

    TS_ASSERT(error == NULL);

    for(auto p : data) {

      tc_sarray* sa = make_sarray_double(p.second);

      tc_sframe_add_column(sf, p.first.c_str(), sa, &error);

      TS_ASSERT(error == NULL);

      tc_sarray_destroy(sa);
    }

    // Check everything
    for(auto p : data) {
      // Make sure it gets out what we want it to.
      tc_sarray* sa = tc_sframe_extract_column_by_name(sf, p.first.c_str(), &error);

      TS_ASSERT(error == NULL);

      tc_sarray* ref_sa = make_sarray_double(p.second);

      int is_equal = tc_sarray_equals(sa, ref_sa, &error);
      TS_ASSERT(is_equal);

      TS_ASSERT(error == NULL);

      tc_sarray_destroy(sa);
    }

    tc_sframe_destroy(sf);
  }

BOOST_AUTO_TEST_CASE(test_sframe_append_test) {
  tc_error* error = NULL;

  std::vector<std::pair<std::string, std::vector<double> > > data

    = { {"col1", {1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5} },
        {"a",    {5.0, 2., 1., 0.5} },
        {"b",    {7.0, 2., 3., 1.5} } };

  std::vector<std::pair<std::string, std::vector<double> > > append_data
    = { {"col1", {1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5} },
        {"a",    {5.0, 2., 1., 0.5} },
        {"b",    {7.0, 2., 3., 1.5} } };

  tc_sframe* sf1 = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  tc_sframe* sf2 = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl_1;
  turi::gl_sframe sf_gl_2;

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf1, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl_1.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  for(auto p : append_data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf2, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl_2.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  turi::gl_sframe gl_combined_sframe = sf_gl_1.append(sf_gl_2);
  tc_sframe* tc_combined_sframe = tc_sframe_append(sf1, sf2, &error);
  TS_ASSERT(error == NULL);

  TS_ASSERT(gl_combined_sframe.column_names() == tc_combined_sframe->value.column_names());
  TS_ASSERT(gl_combined_sframe.column_types() == tc_combined_sframe->value.column_types());

  tc_sframe_destroy(sf1);
  tc_sframe_destroy(sf2);
}

BOOST_AUTO_TEST_CASE(test_sframe_is_materialized_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* sampled_frame = tc_sframe_sample(sf, 0.8, 23, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.sample(0.8, 23);

  TS_ASSERT(tc_sframe_is_materialized(sf, &error) == sf_gl.is_materialized());
  TS_ASSERT(error == NULL);

  TS_ASSERT(tc_sframe_is_materialized(sampled_frame, &error) == sampled_gl_sframe.is_materialized());
  TS_ASSERT(error == NULL);
}

BOOST_AUTO_TEST_CASE(test_sframe_materialize_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* sampled_frame = tc_sframe_sample(sf, 0.8, 23, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.sample(0.8, 23);

  TS_ASSERT(tc_sframe_is_materialized(sf, &error) == sf_gl.is_materialized());
  TS_ASSERT(error == NULL);

  TS_ASSERT(tc_sframe_is_materialized(sampled_frame, &error) == sampled_gl_sframe.is_materialized());
  TS_ASSERT(error == NULL);

  tc_sframe_materialize(sampled_frame, &error);
  TS_ASSERT(error == NULL);

  sampled_gl_sframe.materialize();

  TS_ASSERT(tc_sframe_is_materialized(sampled_frame, &error) == sampled_gl_sframe.is_materialized());
  TS_ASSERT(error == NULL);
}

BOOST_AUTO_TEST_CASE(test_sframe_size_is_known_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5} },
        {"a",    {5.0, 2., 1., 0.5} },
        {"b",    {7.0, 2., 3., 1.5} } };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  bool tc_boolean = tc_sframe_size_is_known(sf, &error);
  TS_ASSERT(error == NULL);

  bool sf_gl_bool = sf_gl.has_size();

  TS_ASSERT(tc_boolean == sf_gl_bool);
  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_contains_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5} },
        {"a",    {5.0, 2., 1., 0.5} },
        {"b",    {7.0, 2., 3., 1.5} } };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  bool tc_boolean = sf_gl.contains_column("col1");
  bool sf_gl_bool = tc_sframe_contains_column(sf, "col1", &error);
  TS_ASSERT(error == NULL);

  bool tc_boolean_2 = sf_gl.contains_column("bla");
  bool sf_gl_bool_2 = tc_sframe_contains_column(sf, "bla", &error);
  TS_ASSERT(error == NULL);

  TS_ASSERT(error == NULL);
  TS_ASSERT(tc_boolean == sf_gl_bool);
  TS_ASSERT(tc_boolean_2 == sf_gl_bool_2);
  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_sample_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* sampled_frame = tc_sframe_sample(sf, 0.8, 23, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.sample(0.8, 23);

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_topk_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* sampled_frame = tc_sframe_topk(sf, "col1", 10, false, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.topk("col1", 10, false);

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_replace_add_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  std::pair<std::string, std::vector<double> > replacing
    = {"col1", {1.5, 2.4, 5.3, 0.3, 1.1, 4., 2., 21, 2.0, 4.2, 1.3, 1.5, 6.0, 4.3, 5.1, 1.9}};

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  turi::flex_list replc_list;

  for (auto it = replacing.second.begin(); it!=replacing.second.end(); ++it) {
      replc_list.push_back(*it);
  }

  turi::gl_sarray repl(replc_list);

  tc_sarray* sa_given = make_sarray_double(replacing.second);

  tc_sframe_replace_add_column(sf, replacing.first.c_str(), sa_given, &error);
  TS_ASSERT(error == NULL);

  sf_gl.replace_add_column(repl, replacing.first.c_str());

  tc_sarray_destroy(sa_given);

  TS_ASSERT(sf_gl.column_names() == sf->value.column_names());
  TS_ASSERT(sf_gl.column_types() == sf->value.column_types());

  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_add_constant_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flexible_type* ft = tc_ft_create_from_double(43.0, &error);
  TS_ASSERT(error == NULL);

  tc_sframe_add_constant_column(sf, "new_column", ft, &error);
  TS_ASSERT(error == NULL);

  turi::flexible_type f_float(43.0);

  sf_gl.add_column(f_float, "new_column");

  TS_ASSERT(sf_gl.column_names() == sf->value.column_names());
  TS_ASSERT(sf_gl.column_types() == sf->value.column_types());

  tc_ft_destroy(ft);
  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_add_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  std::pair<std::string, std::vector<double> > replacing
    = {"col1", {1.5, 2.4, 5.3, 0.3, 1.1, 4., 2., 21, 2.0, 4.2, 1.3, 1.5, 6.0, 4.3, 5.1, 1.9}};

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  turi::flex_list replc_list;

  for (auto it = replacing.second.begin(); it!=replacing.second.end(); ++it) {
      replc_list.push_back(*it);
  }

  turi::gl_sarray repl(replc_list);

  tc_sarray* sa_given = make_sarray_double(replacing.second);

  tc_sframe_add_column(sf, "new_col", sa_given, &error);
  TS_ASSERT(error == NULL);

  sf_gl.add_column(repl, "new_col");

  TS_ASSERT(sf_gl.column_names() == sf->value.column_names());
  TS_ASSERT(sf_gl.column_types() == sf->value.column_types());

  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_add_columns_test) {
  tc_error* error = NULL;

  std::vector<std::pair<std::string, std::vector<double> > > data

    = { {"col1", {1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5} },
        {"a",    {5.0, 2., 1., 0.5} },
        {"b",    {7.0, 2., 3., 1.5} } };

  std::vector<std::pair<std::string, std::vector<double> > > append_data
    = { {"new_col", {1.0, 2., 5., 0.5} },
        {"three_col", {2.0, 2., 3., 0.5} },
        {"cool_address",    {5.0, 2., 1., 0.5} },
        {"more",    {7.0, 2., 3., 1.5} } };

  tc_sframe* sf1 = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  tc_sframe* sf2 = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl_1;
  turi::gl_sframe sf_gl_2;

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf1, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl_1.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  for(auto p : append_data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf2, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl_2.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe_add_columns(sf1, sf2, &error);
  TS_ASSERT(error == NULL);

  sf_gl_1.add_columns(sf_gl_2);

  TS_ASSERT(sf_gl_1.column_names() == sf1->value.column_names());
  TS_ASSERT(sf_gl_1.column_types() == sf1->value.column_types());

  tc_sframe_destroy(sf1);
  tc_sframe_destroy(sf2);
}

BOOST_AUTO_TEST_CASE(test_sframe_swap_columns_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe_swap_columns(sf, "col1", "a", &error);
  TS_ASSERT(error == NULL);

  sf_gl.swap_columns("col1", "a");

  TS_ASSERT(sf_gl.column_names() == sf->value.column_names());
  TS_ASSERT(sf_gl.column_types() == sf->value.column_types());

  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_rename_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  std::map<std::string, std::string> m;
  m.insert(std::pair<std::string, std::string>("col1", "a1"));

  tc_sframe_rename_column(sf, "col1", "a1", &error);
  TS_ASSERT(error == NULL);

  sf_gl.rename(m);

  TS_ASSERT(sf_gl.column_names() == sf->value.column_names());
  TS_ASSERT(sf_gl.column_types() == sf->value.column_types());

  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_fillna_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., NULL, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flexible_type* ft = tc_ft_create_from_double(43.0, &error);
  TS_ASSERT(error == NULL);

  turi::flexible_type f_float(43.0);

  tc_sframe* sampled_frame = tc_sframe_fillna(sf, "col1", ft, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.fillna("col1", f_float);

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_ft_destroy(ft);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_filter_by_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 2., 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  std::pair<std::string, std::vector<double> > replacing
    = {"col1", {5., 2., 1.}};

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  turi::flex_list replacing_list;

  for (auto it = replacing.second.begin(); it!=replacing.second.end(); ++it) {
      replacing_list.push_back(*it);
  }

  turi::gl_sarray rpling(replacing_list);

  tc_sarray* filtering_sa = make_sarray_double(replacing.second);

  tc_sframe* sampled_frame = tc_sframe_filter_by(sf, filtering_sa, "col1", false, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = sf_gl.filter_by(rpling, "col1", false);

  TS_ASSERT(error == NULL);

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sarray_destroy(filtering_sa);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_pack_unpack_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col.1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.3",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.4",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.5",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flexible_type* ft = tc_ft_create_from_double(43.0, &error);
  TS_ASSERT(error == NULL);

  turi::flexible_type f_float(43.0);

  tc_sframe* sampled_frame = tc_sframe_pack_columns_string(sf, "col", "col", tc_ft_type_enum::FT_TYPE_LIST, ft, &error);
  turi::gl_sframe sampled_gl_sframe = sf_gl.pack_columns("col", "col", turi::flex_type_enum::LIST, f_float);

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sframe* unpack_frame = tc_sframe_unpack(sampled_frame, "col", &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe unpack_gl_sframe = sampled_gl_sframe.unpack("col");

  TS_ASSERT(unpack_gl_sframe.column_names() == unpack_frame->value.column_names());
  TS_ASSERT(unpack_gl_sframe.column_types() == unpack_frame->value.column_types());

  tc_ft_destroy(ft);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
  tc_sframe_destroy(unpack_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_stack_unstack_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col.1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.3",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.4",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.5",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flexible_type* ft = tc_ft_create_from_double(43.0, &error);
  TS_ASSERT(error == NULL);

  turi::flexible_type f_float(43.0);

  tc_sframe* pre_sampled_frame = tc_sframe_pack_columns_string(sf, "col", "col", tc_ft_type_enum::FT_TYPE_LIST, ft, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.pack_columns("col", "col", turi::flex_type_enum::LIST, f_float);

  tc_sframe* sampled_frame = tc_sframe_stack(pre_sampled_frame, "col", &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = pre_sampled_gl_sframe.stack("col", "col");

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sframe* unstacked_frame = tc_sframe_unstack(sampled_frame, "col", "col", &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe unstacked_gl_sframe = sampled_gl_sframe.unstack("col", "col");

  TS_ASSERT(unstacked_gl_sframe.column_names() == unstacked_frame->value.column_names());
  TS_ASSERT(unstacked_gl_sframe.column_types() == unstacked_frame->value.column_types());

  tc_ft_destroy(ft);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
  tc_sframe_destroy(unstacked_frame);
  tc_sframe_destroy(pre_sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_stack_and_rename_test) {

  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col.1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.3",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.4",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col.5",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flexible_type* ft = tc_ft_create_from_double(43.0, &error);
  TS_ASSERT(error == NULL);

  turi::flexible_type f_float(43.0);

  tc_sframe* pre_sampled_frame = tc_sframe_pack_columns_string(sf, "col", "col", tc_ft_type_enum::FT_TYPE_LIST, ft, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.pack_columns("col", "col", turi::flex_type_enum::LIST, f_float);

  tc_sframe* sampled_frame = tc_sframe_stack_and_rename(pre_sampled_frame, "col", "col2", false, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sampled_gl_sframe = pre_sampled_gl_sframe.stack("col", "col2");

  TS_ASSERT(sampled_gl_sframe.column_names() == sampled_frame->value.column_names());
  TS_ASSERT(sampled_gl_sframe.column_types() == sampled_frame->value.column_types());

  tc_sframe* unstacked_frame = tc_sframe_unstack(sampled_frame, "col2", "col", &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe unstacked_gl_sframe = sampled_gl_sframe.unstack("col2", "col");

  TS_ASSERT(unstacked_gl_sframe.column_names() == unstacked_frame->value.column_names());
  TS_ASSERT(unstacked_gl_sframe.column_types() == unstacked_frame->value.column_types());

  tc_ft_destroy(ft);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
  tc_sframe_destroy(unstacked_frame);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_unique_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"cola", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"colb",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"colc",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"cold",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* pre_sampled_frame = tc_sframe_unique(sf, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.unique();

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_single_sort_column_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col3",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col4",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col5",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* pre_sampled_frame = tc_sframe_sort_single_column(sf, "col1", true, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.sort("col1", true);

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_sort_multiple_columns_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col3",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col4",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col5",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };

  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flex_list* flex_lst = new_tc_flex_list();

  tc_flexible_type* ft = tc_ft_create_from_cstring("col1", &error);
  TS_ASSERT(error == NULL);

  tc_flexible_type* ft2 = tc_ft_create_from_cstring("col2", &error);
  TS_ASSERT(error == NULL);

  tc_flex_list_add_element(flex_lst, ft, &error);
  TS_ASSERT(error == NULL);

  tc_flex_list_add_element(flex_lst, ft2, &error);
  TS_ASSERT(error == NULL);

  std::vector<std::string> columns_transform;

  columns_transform.push_back("col1");
  columns_transform.push_back("col2");

  tc_sframe* pre_sampled_frame = tc_sframe_sort_multiple_columns(sf, flex_lst, true, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.sort(columns_transform, true);

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_dropna_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., NULL, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };


  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flex_list* flex_lst = new_tc_flex_list();
  tc_flexible_type* ft = tc_ft_create_from_cstring("col1", &error);
  TS_ASSERT(error == NULL);

  tc_flex_list_add_element(flex_lst, ft, &error);
  TS_ASSERT(error == NULL);

  std::vector<std::string> columns_transform;
  columns_transform.push_back("col1");

  tc_sframe* pre_sampled_frame = tc_sframe_dropna(sf, flex_lst, "any", &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl.dropna(columns_transform, "any");

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_slice_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., NULL, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };


  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* pre_sampled_frame = tc_sframe_slice(sf, 1, 3, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl[{1,3}];

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_row_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., NULL, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };


  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_flex_list* fl = tc_sframe_extract_row(sf, 1, &error);
  TS_ASSERT(error == NULL);
  
  TS_ASSERT(fl->value == sf->value[1]); 

  tc_sframe_destroy(sf);
  tc_flex_list_destroy(fl);

}


BOOST_AUTO_TEST_CASE(test_sframe_slice_stride_test) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe sf_gl;

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"col1", {1.0, 2., 5., 0.5, 1.0, 2., 5., NULL, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"col2", {2.0, 2., 3., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"a",    {5.0, 2., 1., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"b",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} },
        {"c",    {7.0, 2., 3., 1.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5, 1.0, 2., 5., 0.5} }
       };


  for(auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  tc_sframe* pre_sampled_frame = tc_sframe_slice_stride(sf, 1, 5, 2, &error);
  TS_ASSERT(error == NULL);

  turi::gl_sframe pre_sampled_gl_sframe = sf_gl[{1, 5, 2}];

  TS_ASSERT(pre_sampled_gl_sframe.column_names() == pre_sampled_frame->value.column_names());
  TS_ASSERT(pre_sampled_gl_sframe.column_types() == pre_sampled_frame->value.column_types());

  tc_sframe_destroy(sf);
  tc_sframe_destroy(pre_sampled_frame);
  TS_ASSERT(true);
}

BOOST_AUTO_TEST_CASE(test_sframe_read_json) {
  tc_error* error = NULL;
  tc_sframe* sf = tc_sframe_read_json("./json_test.json", &error);

  TS_ASSERT(error == NULL);

  size_t nc = tc_sframe_num_columns(sf, &error);
  TS_ASSERT(error == NULL);

  TS_ASSERT(nc == 2);
  
  size_t nr = tc_sframe_num_rows(sf, &error);
  TS_ASSERT(error == NULL);

  TS_ASSERT(nr == 3);

  tc_sframe_destroy(sf);
}

BOOST_AUTO_TEST_CASE(test_sframe_groupby_manual_sframe) {

  tc_error* error = NULL;

  tc_groupby_aggregator* gb_manual = new_tc_groupby_aggregator();
  tc_flex_list *column_list = new_tc_flex_list();
  tc_sframe* sf = tc_sframe_create_empty(&error);
  TS_ASSERT(error == NULL);
  turi::gl_sframe sf_gl;

  /* * +---------+----------+--------+
     * | user_id | movie_id | rating |
     * +---------+----------+--------+
     * |  25904  |   1663   |   3    |
     * |  25907  |   1663   |   3    |
     * |  25923  |   1663   |   3    |
     * |  25924  |   1663   |   3    |
     * |  25928  |   1663   |   2    |
     * |  25933  |   1663   |   4    |
     * |  25934  |   1663   |   4    |
     * |  25935  |   1663   |   4    |
     * |  25936  |   1663   |   5    |
     * |  25937  |   1663   |   2    |
     * +---------+----------+--------+
   */

  std::vector<std::pair<std::string, std::vector<double> > > data
    = { {"user_id",  {25904, 25907, 25923, 25924, 25928, 25933, 25934, 25935, 25936, 25937} },
        {"movie_id", {1663, 1663, 1663, 1663, 1663, 1663, 1663, 1663, 1663, 1663} },
        {"rating",   {3, 3, 3, 3, 2, 4, 4, 4, 5, 2} }
      };

  // make the two sframes
  for (auto p : data) {
    tc_sarray* sa = make_sarray_double(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst;

    for (auto it = p.second.begin(); it!=p.second.end(); ++it) {
        lst.push_back(*it);
    }

    turi::gl_sarray g(lst);

    sf_gl.add_column(g, p.first.c_str());

    tc_sarray_destroy(sa);
  }

  // construct a group_by_aggregator and all the things needed to make a call
  // to tc_sframe_group_by

  tc_flexible_type* user_id_ft = tc_ft_create_from_cstring("user_id", &error);
  TS_ASSERT(error == NULL);

  tc_flex_list_add_element(column_list, user_id_ft, &error);
  TS_ASSERT(error == NULL);

  tc_groupby_aggregator_add_count(gb_manual, "count", &error);
  TS_ASSERT(error == NULL);

  tc_sframe* sampled_frame = tc_sframe_group_by(sf, column_list, gb_manual, &error);
  TS_ASSERT(error == NULL);

  // make groupkeys and operators, call C++ groupby
  std::vector<std::string> group_keys;
  group_keys.push_back("user_id");
  std::map<std::string, turi::aggregate::groupby_descriptor_type> operators;
  operators.insert({"count", turi::aggregate::COUNT()});

  turi::gl_sframe sampled_gl_sframe = sf_gl.groupby(group_keys, operators);

  TS_ASSERT(check_equality_gl_sframe(sampled_frame->value, sampled_gl_sframe));

  tc_groupby_aggregator_destroy(gb_manual);
  tc_flex_list_destroy(column_list);
  tc_sframe_destroy(sf);
  tc_sframe_destroy(sampled_frame);
}

BOOST_AUTO_TEST_CASE(test_sframe_groupby_random_sframe_most_aggregates) {

  tc_error* error = NULL;
  std::string all_types = "RZSVLD";
  // R: real, Z: integer, S: string, V: vector, L: list, D: double
  tc_groupby_aggregator* gb1 = new_tc_groupby_aggregator();
  tc_flex_list *column_list1 = new_tc_flex_list();

  TS_ASSERT(error == NULL);
  size_t n_rows1 = 10000;
  size_t n_columns1 = 100;
  std::string column_types1 = "R";
  for (int index = 1; index < n_columns1; index++) {
    column_types1 += all_types[rand() % 6];
    srand(time(NULL));
  }
  size_t seed = rand();
  turi::gl_sframe sf_gl1 = _generate_random_sframe(n_rows1, column_types1, seed,
    false, 0.0);
  tc_sframe *sf1 = new_tc_sframe(sf_gl1);
  turi::gl_sarray column;
  std::string column_name, zeroth_column, last_column;
  zeroth_column = sf_gl1.column_name(0);
  last_column = sf_gl1.column_name(n_columns1-1);

  TS_ASSERT(sf1->value.num_columns() == sf_gl1.num_columns());
  TS_ASSERT(sf1->value.column_names() == sf_gl1.column_names());
  TS_ASSERT(sf1->value.column_types() == sf_gl1.column_types());
  TS_ASSERT(check_equality_gl_sframe(sf1->value, sf_gl1));

  // C interface
  tc_groupby_aggregator_add_sum(gb1, "a_sum", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_max(gb1, "a_max", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_min(gb1, "a_min", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_mean(gb1, "a_mean", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_avg(gb1, "a_avg", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_var(gb1, "a_var", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_variance(gb1, "a_variance", zeroth_column.c_str(),
    &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_std(gb1, "a_std", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_stdv(gb1, "a_stdv", zeroth_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_select_one(gb1, "a_select_one",
    sf_gl1.column_name(50).c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_count_distinct(gb1, "a_count_distinct",
    sf_gl1.column_name(75).c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_concat_one_column(gb1, "a_concat_one_column",
    sf_gl1.column_name(25).c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_concat_two_columns(gb1, "a_concat_two_columns",
    sf_gl1.column_name(20).c_str(), sf_gl1.column_name(80).c_str(), &error);
  TS_ASSERT(error == NULL);

  tc_groupby_aggregator_add_count(gb1, "a_count", &error);
  TS_ASSERT(error == NULL);

  tc_flexible_type* last_ft = tc_ft_create_from_cstring(last_column.c_str(),
    &error);
  TS_ASSERT(error == NULL);
  tc_flex_list_add_element(column_list1, last_ft, &error);
  TS_ASSERT(error == NULL);

  tc_sframe* sampled_frame1 = tc_sframe_group_by(sf1,column_list1,gb1,&error);
  TS_ASSERT(error == NULL);

  // C++ interface
  std::vector<std::string> group_keys;
  group_keys.push_back(last_column);
  std::map<std::string, turi::aggregate::groupby_descriptor_type> operators;
  operators.insert({"a_sum", turi::aggregate::SUM(zeroth_column)});
  operators.insert({"a_max", turi::aggregate::MAX(zeroth_column)});
  operators.insert({"a_min", turi::aggregate::MIN(zeroth_column)});
  operators.insert({"a_mean", turi::aggregate::MEAN(zeroth_column)});
  operators.insert({"a_avg", turi::aggregate::AVG(zeroth_column)});
  operators.insert({"a_var", turi::aggregate::VAR(zeroth_column)});
  operators.insert({"a_variance", turi::aggregate::VARIANCE(zeroth_column)});
  operators.insert({"a_std", turi::aggregate::STD(zeroth_column)});
  operators.insert({"a_stdv", turi::aggregate::STDV(zeroth_column)});
  operators.insert({"a_select_one",
    turi::aggregate::SELECT_ONE(sf_gl1.column_name(50))});
  operators.insert({"a_count_distinct",
    turi::aggregate::COUNT_DISTINCT(sf_gl1.column_name(75))});
  operators.insert({"a_concat_one_column",
    turi::aggregate::CONCAT(sf_gl1.column_name(25))});
  operators.insert({"a_concat_two_columns",
    turi::aggregate::CONCAT(sf_gl1.column_name(20), sf_gl1.column_name(80))});
  operators.insert({"a_count", turi::aggregate::COUNT()});

  turi::gl_sframe sampled_gl_sframe = sf_gl1.groupby(group_keys, operators);

  // Check for equality
  TS_ASSERT(check_equality_gl_sframe(sampled_frame1->value, sampled_gl_sframe));

  tc_groupby_aggregator_destroy(gb1);
  tc_flex_list_destroy(column_list1);
  tc_sframe_destroy(sf1);
  tc_sframe_destroy(sampled_frame1);
}

BOOST_AUTO_TEST_CASE(test_sframe_groupby_random_sframe_quantiles) {
  tc_error* error = NULL;
  std::string all_types = "RZSVLD";
  // R: real, Z: integer, S: string, V: vector, L: list, D: double
  tc_groupby_aggregator* gb1 = new_tc_groupby_aggregator();
  tc_flex_list *column_list1 = new_tc_flex_list();

  TS_ASSERT(error == NULL);
  size_t n_rows1 = 10000;
  size_t n_columns1 = 100;
  std::string column_types1 = "RZ";
  for (int index = 2; index < n_columns1; index++) {
    column_types1 += all_types[rand() % 6];
    srand(time(NULL));
  }
  size_t seed = rand();
  turi::gl_sframe sf_gl1 = _generate_random_sframe(n_rows1, column_types1, seed,
    false, 0.0);
  tc_sframe *sf1 = new_tc_sframe(sf_gl1);
  turi::gl_sarray column;
  std::string column_name, zeroth_column, first_column, last_column;
  zeroth_column = sf_gl1.column_name(0);
  first_column = sf_gl1.column_name(1);
  last_column = sf_gl1.column_name(n_columns1-1);

  TS_ASSERT(sf1->value.num_columns() == sf_gl1.num_columns());
  TS_ASSERT(sf1->value.column_names() == sf_gl1.column_names());
  TS_ASSERT(sf1->value.column_types() == sf_gl1.column_types());
  TS_ASSERT(check_equality_gl_sframe(sf1->value, sf_gl1));

  tc_flex_list *quantiles = new_tc_flex_list();
  tc_flexible_type *ft_25 = tc_ft_create_from_double(0.25, &error);
  TS_ASSERT(error == NULL);
  tc_flex_list_add_element(quantiles, ft_25, &error);
  TS_ASSERT(error == NULL);
  tc_flexible_type *ft_50 = tc_ft_create_from_double(0.5, &error);
  TS_ASSERT(error == NULL);
  tc_flex_list_add_element(quantiles, ft_50, &error);
  TS_ASSERT(error == NULL);

  // C interface
  tc_groupby_aggregator_add_quantile(gb1, "a_quantile",
    zeroth_column.c_str(), 0.75, &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_quantiles(gb1, "a_quantiles",
    zeroth_column.c_str(), quantiles, &error);
  TS_ASSERT(error == NULL);

  tc_flexible_type* last_ft = tc_ft_create_from_cstring(last_column.c_str(),
    &error);
  TS_ASSERT(error == NULL);
  tc_flex_list_add_element(column_list1, last_ft, &error);
  TS_ASSERT(error == NULL);

  tc_sframe* sampled_frame1 = tc_sframe_group_by(sf1,column_list1,gb1,&error);
  TS_ASSERT(error == NULL);

  // C++ interface
  std::vector<std::string> group_keys;
  std::vector<double> quantiles_transform;
  quantiles_transform.push_back(0.25);
  quantiles_transform.push_back(0.5);
  group_keys.push_back(last_column);
  std::map<std::string, turi::aggregate::groupby_descriptor_type> operators;
  operators.insert({"a_quantile",
    turi::aggregate::QUANTILE(zeroth_column, 0.75)});
  operators.insert({"a_quantiles",
    turi::aggregate::QUANTILE(zeroth_column, quantiles_transform)});

  turi::gl_sframe sampled_gl_sframe = sf_gl1.groupby(group_keys, operators);

  // Check for equality
  TS_ASSERT(sampled_frame1->value.num_columns() == sampled_gl_sframe.num_columns());
  TS_ASSERT(sampled_frame1->value.column_names() == sampled_gl_sframe.column_names());
  TS_ASSERT(sampled_frame1->value.column_types() == sampled_gl_sframe.column_types());
  TS_ASSERT(check_equality_gl_sframe(sampled_frame1->value, sampled_gl_sframe));

  tc_groupby_aggregator_destroy(gb1);
  tc_flex_list_destroy(column_list1);
  tc_flex_list_destroy(quantiles);
  tc_sframe_destroy(sf1);
  tc_sframe_destroy(sampled_frame1);
}
BOOST_AUTO_TEST_CASE(test_sframe_read_json) {
  capi_test_sframe::test_sframe_read_json();
}

BOOST_AUTO_TEST_CASE(test_sframe_groupby_random_sframe_argminmax) {
  tc_error* error = NULL;
  std::string all_types = "RZSVLD";
  // R: real, Z: integer, S: string, V: vector, L: list, D: double
  tc_groupby_aggregator* gb1 = new_tc_groupby_aggregator();
  tc_flex_list *column_list1 = new_tc_flex_list();

  TS_ASSERT(error == NULL);
  size_t n_rows1 = 10000;
  size_t n_columns1 = 100;
  std::string column_types1 = "RZ";
  for (int index = 2; index < n_columns1; index++) {
    column_types1 += all_types[rand() % 6];
    srand(time(NULL));
  }
  size_t seed = rand();
  turi::gl_sframe sf_gl1 = _generate_random_sframe(n_rows1, column_types1, seed,
    false, 0.0);
  tc_sframe *sf1 = new_tc_sframe(sf_gl1);
  turi::gl_sarray column;
  std::string column_name, zeroth_column, first_column, last_column;
  zeroth_column = sf_gl1.column_name(0);
  first_column = sf_gl1.column_name(1);
  last_column = sf_gl1.column_name(n_columns1-1);

  TS_ASSERT(sf1->value.num_columns() == sf_gl1.num_columns());
  TS_ASSERT(sf1->value.column_names() == sf_gl1.column_names());
  TS_ASSERT(sf1->value.column_types() == sf_gl1.column_types());
  TS_ASSERT(check_equality_gl_sframe(sf1->value, sf_gl1));

  // C interface
  tc_groupby_aggregator_add_argmax(gb1, "a_argmax",
    zeroth_column.c_str(), first_column.c_str(), &error);
  TS_ASSERT(error == NULL);
  tc_groupby_aggregator_add_argmin(gb1, "a_argmin",
    zeroth_column.c_str(), first_column.c_str(), &error);
  TS_ASSERT(error == NULL);

  tc_flexible_type* last_ft = tc_ft_create_from_cstring(last_column.c_str(),
    &error);
  TS_ASSERT(error == NULL);
  tc_flex_list_add_element(column_list1, last_ft, &error);
  TS_ASSERT(error == NULL);

  tc_sframe* sampled_frame1 = tc_sframe_group_by(sf1,column_list1,gb1,&error);
  TS_ASSERT(error == NULL);

  // C++ interface
  std::vector<std::string> group_keys;
  group_keys.push_back(last_column);
  std::map<std::string, turi::aggregate::groupby_descriptor_type> operators;
  operators.insert({"a_argmax",
    turi::aggregate::ARGMAX(zeroth_column, first_column)});
  operators.insert({"a_argmin",
    turi::aggregate::ARGMIN(zeroth_column, first_column)});

  turi::gl_sframe sampled_gl_sframe = sf_gl1.groupby(group_keys, operators);

  // Check for equality
  TS_ASSERT(sampled_frame1->value.num_columns() == sampled_gl_sframe.num_columns());
  TS_ASSERT(sampled_frame1->value.column_names() == sampled_gl_sframe.column_names());
  TS_ASSERT(sampled_frame1->value.column_types() == sampled_gl_sframe.column_types());
  TS_ASSERT(check_equality_gl_sframe(sampled_frame1->value, sampled_gl_sframe));

  tc_groupby_aggregator_destroy(gb1);
  tc_flex_list_destroy(column_list1);
  tc_sframe_destroy(sf1);
  tc_sframe_destroy(sampled_frame1);
}
