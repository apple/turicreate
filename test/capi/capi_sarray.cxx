#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <vector>
#include "capi_utils.hpp"

#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <capi/impl/capi_sarray.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <iostream>

class capi_test_sarray {
 public:

 void test_sarray_double() {

    std::vector<double> v = {1, 2, 4.5, 9, 10000000, -12433};

    tc_error* error = NULL;

    tc_flex_list* fl = make_flex_list_double(v);

    tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

    TS_ASSERT(error == NULL);

    {
      // Make sure it gets out what we want it to.
      for(size_t i = 0; i < v.size(); ++i) {
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
        TS_ASSERT(error == NULL);

        TS_ASSERT(tc_ft_is_double(ft) != 0);

        double val = tc_ft_double(ft, &error);
        TS_ASSERT(error == NULL);

        TS_ASSERT(v[i] == val);

        tc_ft_destroy(ft);
      }
    }
  }

  void test_tc_op_sarray_lt_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_lt_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 < g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_gt_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_gt_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 > g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_le_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_le_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 <= g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_ge_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_ge_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 >= g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_op_sarray_eq_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_eq_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 == g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_lt_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_lt_ft(sa1, ft1, &error);

    turi::gl_sarray combined_gl_output = (g1 < f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_gt_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_gt_ft(sa1, ft1, &error);

    turi::gl_sarray combined_gl_output = (g1 > f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_op_sarray_ge_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_ge_ft(sa1, ft1, &error);

    turi::gl_sarray combined_gl_output = (g1 >= f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_le_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_le_ft(sa1, ft1, &error);

    turi::gl_sarray combined_gl_output = (g1 <= f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_eq_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_eq_ft(sa1, ft1, &error);

    turi::gl_sarray combined_gl_output = (g1 == f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_logical_and_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_logical_and_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 && g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_bitwise_and_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_bitwise_and_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 & g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_logical_or_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_logical_or_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 || g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_bitwise_or_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_bitwise_or_sarray(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1 | g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_apply_mask(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_apply_mask(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = (g1[g2]);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_all_nonzero(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT(g1.all()== tc_sarray_all_nonzero(sa1, &error));

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_any_nonzero(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT(g1.any() == tc_sarray_any_nonzero(sa1, &error));

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_head(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3, 4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.head(4) == tc_sarray_head(sa1, 4, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tail(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3,4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.tail(4) == tc_sarray_tail(sa1, 4, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_words(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.count_words(1) == tc_sarray_count_words(sa1, 0, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_count_words_with_delimiters(){

  };

  void test_tc_sarray_count_word_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.count_ngrams(1, "word", false, true) == tc_sarray_count_word_ngrams(sa1, 1, false, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_character_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.count_ngrams(1, "character", false, true) == tc_sarray_count_character_ngrams(sa1, 1, false, false, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_dict_trim_by_keys(){

  };

  //TODO
  void test_tc_sarray_dict_trim_by_value_range(){

  };

  //TODO
  void test_tc_sarray_tc_sarray_max(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.max() == tc_sarray_max(sa1, &error)->value));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tc_sarray_min(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.min() == tc_sarray_min(sa1, &error)->value));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tc_sarray_sum(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.sum() == tc_sarray_sum(sa1, &error)->value));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tc_sarray_mean(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.mean() == tc_sarray_mean(sa1, &error)->value));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tc_sarray_std(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.std() == tc_sarray_std(sa1, &error)->value));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_nnz(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.nnz() == tc_sarray_nnz(sa1, &error)));

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_num_missing(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.num_missing() == tc_sarray_num_missing(sa1, &error)));

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_dict_keys(){

  };

  //TODO
  void test_tc_sarray_dict_has_any_keys(){

  };

  //TODO
  void test_tc_sarray_dict_has_all_keys(){

  };

  //TODO
  void test_tc_sarray_sample(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.sample(0.8, 2) == (tc_sarray_sample(sa1, 0.8, 2, &error)->value)).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_datetime_to_str_with_format(){

  };

  //TODO
  void test_tc_sarray_tc_sarray_datetime_to_str(){

  };

  //TODO
  void test_tc_sarray_str_to_datetime(){

  };

  void test_tc_sarray_clip(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    tc_flexible_type* ft2 = tc_ft_create_from_double(3, &error);

    turi::flexible_type f_float_1(1.0);
    turi::flexible_type f_float_2(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.clip(f_float_1, f_float_2) == tc_sarray_clip(sa1, ft1, ft2, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_drop_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.dropna() == tc_sarray_drop_nan(sa1, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_replace_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    turi::flexible_type f_float_1(1.0);

    TS_ASSERT((g1.fillna(f_float_1) == tc_sarray_replace_nan(sa1, ft1, &error)->value).all());

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_topk_index(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.topk_index(3, false) == tc_sarray_topk_index(sa1, 3, false, &error)->value).all());

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_append(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_append(sa1, sa2, &error);

    turi::gl_sarray combined_gl_output = g1.append(g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  //TODO
  void test_tc_sarray_unique(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.unique() == tc_sarray_unique(sa1, &error)->value).all());

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_is_materialized(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));


    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_materialize(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));

    tc_sarray_materialize(combined_output, &error);
    combined_gl_output.materialize();

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

};


BOOST_FIXTURE_TEST_SUITE(_capi_test_sarray, capi_test_sarray)
BOOST_AUTO_TEST_CASE(test_sarray_double) {
  capi_test_sarray::test_sarray_double();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_lt_sarray) {
  capi_test_sarray::test_tc_op_sarray_lt_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_gt_sarray) {
  capi_test_sarray::test_tc_op_sarray_gt_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_le_sarray) {
  capi_test_sarray::test_tc_op_sarray_le_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_ge_sarray) {
  capi_test_sarray::test_tc_op_sarray_ge_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_eq_sarray) {
  capi_test_sarray::test_tc_op_sarray_eq_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_lt_ft) {
  capi_test_sarray::test_tc_op_sarray_lt_ft();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_gt_ft) {
  capi_test_sarray::test_tc_op_sarray_gt_ft();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_ge_ft) {
  capi_test_sarray::test_tc_op_sarray_ge_ft();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_le_ft) {
  capi_test_sarray::test_tc_op_sarray_le_ft();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_eq_ft) {
  capi_test_sarray::test_tc_op_sarray_eq_ft();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_logical_and_sarray) {
  capi_test_sarray::test_tc_op_sarray_logical_and_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_bitwise_and_sarray) {
  capi_test_sarray::test_tc_op_sarray_bitwise_and_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_logical_or_sarray) {
  capi_test_sarray::test_tc_op_sarray_logical_or_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_op_sarray_bitwise_or_sarray) {
  capi_test_sarray::test_tc_op_sarray_bitwise_or_sarray();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_apply_mask) {
  capi_test_sarray::test_tc_sarray_apply_mask();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_all_nonzero) {
  capi_test_sarray::test_tc_sarray_all_nonzero();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_any_nonzero) {
  capi_test_sarray::test_tc_sarray_any_nonzero();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_head) {
  capi_test_sarray::test_tc_sarray_head();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tail) {
  capi_test_sarray::test_tc_sarray_tail();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_count_words) {
  capi_test_sarray::test_tc_sarray_count_words();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_count_words_with_delimiters) {
  capi_test_sarray::test_tc_sarray_count_words_with_delimiters();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_count_word_ngrams) {
  capi_test_sarray::test_tc_sarray_count_word_ngrams();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_count_character_ngrams) {
  capi_test_sarray::test_tc_sarray_count_character_ngrams();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_dict_trim_by_keys) {
  capi_test_sarray::test_tc_sarray_dict_trim_by_keys();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_dict_trim_by_value_range) {
  capi_test_sarray::test_tc_sarray_dict_trim_by_value_range();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_max) {
  capi_test_sarray::test_tc_sarray_tc_sarray_max();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_min) {
  capi_test_sarray::test_tc_sarray_tc_sarray_min();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_sum) {
  capi_test_sarray::test_tc_sarray_tc_sarray_sum();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_mean) {
  capi_test_sarray::test_tc_sarray_tc_sarray_mean();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_std) {
  capi_test_sarray::test_tc_sarray_tc_sarray_std();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_nnz) {
  capi_test_sarray::test_tc_sarray_nnz();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_num_missing) {
  capi_test_sarray::test_tc_sarray_num_missing();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_dict_keys) {
  capi_test_sarray::test_tc_sarray_dict_keys();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_dict_has_any_keys) {
  capi_test_sarray::test_tc_sarray_dict_has_any_keys();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_dict_has_all_keys) {
  capi_test_sarray::test_tc_sarray_dict_has_all_keys();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_sample) {
  capi_test_sarray::test_tc_sarray_sample();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_datetime_to_str_with_format) {
  capi_test_sarray::test_tc_sarray_datetime_to_str_with_format();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_tc_sarray_datetime_to_str) {
  capi_test_sarray::test_tc_sarray_tc_sarray_datetime_to_str();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_str_to_datetime) {
  capi_test_sarray::test_tc_sarray_str_to_datetime();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_clip) {
  capi_test_sarray::test_tc_sarray_clip();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_drop_nan) {
  capi_test_sarray::test_tc_sarray_drop_nan();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_replace_nan) {
  capi_test_sarray::test_tc_sarray_replace_nan();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_topk_index) {
  capi_test_sarray::test_tc_sarray_topk_index();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_append) {
  capi_test_sarray::test_tc_sarray_append();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_unique) {
  capi_test_sarray::test_tc_sarray_unique();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_is_materialized) {
  capi_test_sarray::test_tc_sarray_is_materialized();
}
BOOST_AUTO_TEST_CASE(test_tc_sarray_materialize) {
  capi_test_sarray::test_tc_sarray_materialize();
}
BOOST_AUTO_TEST_SUITE_END()
