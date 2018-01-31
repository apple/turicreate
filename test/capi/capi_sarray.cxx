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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


    tc_sarray* combined_output = tc_op_sarray_lt_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_gt_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


    tc_sarray* combined_output = tc_op_sarray_le_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_ge_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray combined_gl_output = (g1 >= g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_eq_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_eq_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_op_sarray_lt_ft(sa1, ft1, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_op_sarray_gt_ft(sa1, ft1, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray combined_gl_output = (g1 > f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_ft_destroy(ft1);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_op_sarray_ge_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_op_sarray_ge_ft(sa1, ft1, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_op_sarray_le_ft(sa1, ft1, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_op_sarray_eq_ft(sa1, ft1, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_op_sarray_logical_and_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_bitwise_and_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_logical_or_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_op_sarray_bitwise_or_sarray(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_sarray_apply_mask(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(g1.all()== tc_sarray_all_nonzero(sa1, &error));
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_any_nonzero(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    TS_ASSERT(g1.any() == tc_sarray_any_nonzero(sa1, &error));
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_head(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3, 4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.head(4) == tc_sarray_head(sa1, 4, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_tail(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3,4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.tail(4) == tc_sarray_tail(sa1, 4, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_words(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    TS_ASSERT((g1.count_words(0) == tc_sarray_count_words(sa1, 0, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_word_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.count_ngrams(1, "word", false, true) == tc_sarray_count_word_ngrams(sa1, 1, false, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_character_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.count_ngrams(1, "character", false, true) == tc_sarray_count_character_ngrams(sa1, 1, false, false, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_dict_trim_by_keys(){
    tc_error* error = NULL;
    std::vector<std::pair<std::string, std::string > > data
      = { {"col1", "hello" },
          {"col2", "cool" },
          {"a",    "awesome" },
          {"b",    "build" },
          {"c",    "coolness" }
         };

    std::vector<std::string> keys =
      {"col1", "col2"};


    tc_flex_dict* test_flex_dict = tc_flex_dict_create(&error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      TS_ASSERT(error == NULL);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_ft_destroy(ft1);
      tc_ft_destroy(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list_add_element(fl, ft, &error);
    TS_ASSERT(error == NULL);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_trim_by_keys(sa1, string_list, 1, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1_modified = g1.dict_trim_by_keys(key_flexible, true);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_ft_destroy(ft);
  };

  void test_tc_sarray_dict_trim_by_value_range(){
    tc_error* error = NULL;
    std::vector<std::pair<std::string, int64_t > > data
      = { {"col1",  1},
          {"col2", 3 },
          {"a",    5 },
          {"b",    7 },
          {"c",    9 }
         };


    tc_flex_dict* test_flex_dict = tc_flex_dict_create(&error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flexible_type* ft2 = tc_ft_create_from_int64(p.second, &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      TS_ASSERT(error == NULL);

      flexible_dictionary.push_back({p.first.c_str(), p.second});

      tc_ft_destroy(ft1);
      tc_ft_destroy(ft2);
    }

    turi::flexible_type lower(3);
    turi::flexible_type upper(5);

    tc_flexible_type* lower_flex = tc_ft_create_from_int64(3, &error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* upper_flex = tc_ft_create_from_int64(5, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list* fl = tc_flex_list_create(&error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list_add_element(fl, ft, &error);
    TS_ASSERT(error == NULL);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1(lst1);

    tc_sarray* modified_dict = tc_sarray_dict_trim_by_value_range(sa1, lower_flex, upper_flex, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1_modified = g1.dict_trim_by_values(lower, upper);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_ft_destroy(ft);
  };

  void test_tc_sarray_tc_sarray_max(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.max() == tc_sarray_max(sa1, &error)->value));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.min() == tc_sarray_min(sa1, &error)->value));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.sum() == tc_sarray_sum(sa1, &error)->value));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.mean() == tc_sarray_mean(sa1, &error)->value));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.std() == tc_sarray_std(sa1, &error)->value));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.nnz() == tc_sarray_nnz(sa1, &error)));
    TS_ASSERT(error == NULL);

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
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.num_missing() == tc_sarray_num_missing(sa1, &error)));
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_dict_keys(){
    tc_error* error = NULL;
    std::vector<std::pair<std::string, std::string > > data
      = { {"col1", "hello" },
          {"col2", "cool" },
          {"a",    "awesome" },
          {"b",    "build" },
          {"c",    "coolness" }
         };

    std::vector<std::string> keys =
      {"col1", "col2"};


    tc_flex_dict* test_flex_dict = tc_flex_dict_create(&error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      TS_ASSERT(error == NULL);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_ft_destroy(ft1);
      tc_ft_destroy(ft2);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list_add_element(fl, ft, &error);
    TS_ASSERT(error == NULL);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1(lst1);

    tc_sarray* modified_sa = tc_sarray_dict_keys(sa1, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray modified_sa_gl = g1.dict_keys();

    TS_ASSERT((modified_sa->value == modified_sa_gl).all());

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_dict_has_any_keys(){
    tc_error* error = NULL;
    std::vector<std::pair<std::string, std::string > > data
      = { {"col1", "hello" },
          {"col2", "cool" },
          {"a",    "awesome" },
          {"b",    "build" },
          {"c",    "coolness" }
         };

    std::vector<std::string> keys =
      {"col1", "col2"};


    tc_flex_dict* test_flex_dict = tc_flex_dict_create(&error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      TS_ASSERT(error == NULL);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_ft_destroy(ft1);
      tc_ft_destroy(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list_add_element(fl, ft, &error);
    TS_ASSERT(error == NULL);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_has_any_keys(sa1, string_list, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1_modified = g1.dict_has_any_keys(key_flexible);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_ft_destroy(ft);
  };

  void test_tc_sarray_dict_has_all_keys(){
    tc_error* error = NULL;
    std::vector<std::pair<std::string, std::string > > data
      = { {"col1", "hello" },
          {"col2", "cool" },
          {"a",    "awesome" },
          {"b",    "build" },
          {"c",    "coolness" }
         };

    std::vector<std::string> keys =
      {"col1", "col2"};


    tc_flex_dict* test_flex_dict = tc_flex_dict_create(&error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      TS_ASSERT(error == NULL);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_ft_destroy(ft1);
      tc_ft_destroy(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    TS_ASSERT(error == NULL);

    tc_flex_list_add_element(fl, ft, &error);
    TS_ASSERT(error == NULL);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_has_all_keys(sa1, string_list, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray g1_modified = g1.dict_has_all_keys(key_flexible);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_ft_destroy(ft);
  };

  void test_tc_sarray_sample(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.sample(0.8, 2) == (tc_sarray_sample(sa1, 0.8, 2, &error)->value)).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_count_words_with_delimiters(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0\n2", "2\nr34\nr34", "4.5\nr34rr34\nr4", "9", "389", "23", "32\n43r4", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    std::vector<std::string> delimiters = {"\n"};

    tc_flex_list* fl2 = make_flex_list_string(delimiters);

    turi::flex_list lst2;
    lst2.push_back("\n");

    TS_ASSERT((g1.count_words(0, lst2) == tc_sarray_count_words_with_delimiters(sa1, 0, fl2, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_clip(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    TS_ASSERT(error == NULL);

    tc_flexible_type* ft2 = tc_ft_create_from_double(3, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float_1(1.0);
    turi::flexible_type f_float_2(3.0);

    TS_ASSERT((g1.clip(f_float_1, f_float_2) == tc_sarray_clip(sa1, ft1, ft2, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_drop_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.dropna() == tc_sarray_drop_nan(sa1, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_replace_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    TS_ASSERT(error == NULL);

    turi::flexible_type f_float_1(1.0);

    TS_ASSERT((g1.fillna(f_float_1) == tc_sarray_replace_nan(sa1, ft1, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_topk_index(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.topk_index(3, false) == tc_sarray_topk_index(sa1, 3, false, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_append(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


    tc_sarray* combined_output = tc_sarray_append(sa1, sa2, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray combined_gl_output = g1.append(g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_flex_list_destroy(fl1);
    tc_flex_list_destroy(fl2);

    tc_sarray_destroy(sa1);
    tc_sarray_destroy(sa2);

    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_unique(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    TS_ASSERT((g1.unique() == tc_sarray_unique(sa1, &error)->value).all());
    TS_ASSERT(error == NULL);

    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_is_materialized(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    TS_ASSERT(error == NULL);

    tc_flex_list_destroy(fl1);
    tc_sarray_destroy(sa1);
    TS_ASSERT(error == NULL);
  };

  void test_tc_sarray_materialize(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    TS_ASSERT(error == NULL);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(error == NULL);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);
    TS_ASSERT(error == NULL);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    TS_ASSERT(error == NULL);

    tc_sarray_materialize(combined_output, &error);
    TS_ASSERT(error == NULL);

    combined_gl_output.materialize();

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    TS_ASSERT(error == NULL);

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
