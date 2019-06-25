/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include <vector>
#include "capi_utils.hpp"

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <iostream>

class capi_test_sarray {
 public:

 void test_sarray_double() {

    std::vector<double> v = {1, 2, 4.5, 9, 10000000, -12433};

    tc_error* error = NULL;

    tc_flex_list* fl = make_flex_list_double(v);

    tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

      CAPI_CHECK_ERROR(error);

    {
      // Make sure it gets out what we want it to.
      for(size_t i = 0; i < v.size(); ++i) {
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
          CAPI_CHECK_ERROR(error);

        TS_ASSERT(tc_ft_is_double(ft) != 0);

        double val = tc_ft_double(ft, &error);
          CAPI_CHECK_ERROR(error);

        TS_ASSERT(v[i] == val);

          tc_release(ft);
      }
    }
  }

 void test_sarray_save_load() {

     std::string out_name = turi::fileio::get_system_temp_directory() + "/sa_tmp_1/";

    std::vector<double> v = {1, 2, 4.5, 9, 10000000, -12433, 123123};

    tc_error* error = NULL;

    tc_flex_list* fl = make_flex_list_double(v);

    tc_sarray* sa_sv = tc_sarray_create_from_list(fl, &error);

      CAPI_CHECK_ERROR(error);

      tc_sarray_save(sa_sv, out_name.c_str(), &error);

      CAPI_CHECK_ERROR(error);

      tc_release(sa_sv);

      tc_sarray* sa = tc_sarray_load(out_name.c_str(), &error);

      CAPI_CHECK_ERROR(error);

    {
      // Make sure it gets out what we want it to.
      for(size_t i = 0; i < v.size(); ++i) {
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
          CAPI_CHECK_ERROR(error);

        TS_ASSERT(tc_ft_is_double(ft) != 0);

        double val = tc_ft_double(ft, &error);
          CAPI_CHECK_ERROR(error);

        TS_ASSERT(v[i] == val);

          tc_release(ft);
      }
    }
}


  void test_tc_op_sarray_lt_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


      tc_sarray* combined_output = tc_binary_op_ss(sa1, "<", sa2, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 < g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(fl2);

      tc_release(sa1);
      tc_release(sa2);

      CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_gt_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

      tc_sarray* combined_output = tc_binary_op_ss(sa1, ">", sa2, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 > g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(fl2);

      tc_release(sa1);
      tc_release(sa2);

      CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_le_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


      tc_sarray* combined_output = tc_binary_op_ss(sa1, "<=", sa2, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 <= g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(fl2);

      tc_release(sa1);
      tc_release(sa2);

      CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_ge_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

      tc_sarray* combined_output = tc_binary_op_ss(sa1, ">=", sa2, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 >= g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(fl2);

      tc_release(sa1);
      tc_release(sa2);

      CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_eq_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

      tc_sarray* combined_output = tc_binary_op_ss(sa1, "==", sa2, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 == g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(fl2);

      tc_release(sa1);
      tc_release(sa2);

      CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_lt_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
      CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
      CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

      tc_sarray* combined_output = tc_binary_op_sf(sa1, "<", ft1, &error);
      CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 < f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

      tc_release(fl1);
      tc_release(ft1);

      CAPI_CHECK_ERROR(error);
      tc_release(sa1);
  };

  void test_tc_op_sarray_gt_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_binary_op_sf(sa1, ">", ft1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 > f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_op_sarray_ge_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_binary_op_sf(sa1, ">=", ft1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 >= f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_op_sarray_le_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_binary_op_sf(sa1, "<=", ft1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 <= f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_op_sarray_eq_ft(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    tc_sarray* combined_output = tc_binary_op_sf(sa1, "==", ft1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 == f_float);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_op_sarray_logical_and_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    CAPI_CHECK_ERROR(error);

    tc_sarray* combined_output = tc_binary_op_ss(sa1, "&", sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 && g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_bitwise_and_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_binary_op_ss(sa1, "&", sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 & g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_logical_or_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_binary_op_ss(sa1, "|", sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 || g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_op_sarray_bitwise_or_sarray(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_binary_op_ss(sa1, "|", sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1 | g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_sarray_apply_slice(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(fl1->value);
    tc_release(fl1);

    tc_sarray* combined_output = tc_sarray_slice(sa1, 2,2,5, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1[{2,2,5}]);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(sa1);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_sarray_apply_mask(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);

    tc_sarray* combined_output = tc_sarray_apply_mask(sa1, sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = (g1[g2]);

    TS_ASSERT((combined_gl_output == combined_output->value).all());
    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_sarray_all_nonzero(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT(g1.all()== tc_sarray_all_nonzero(sa1, &error));
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_any_nonzero(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    TS_ASSERT(g1.any() == tc_sarray_any_nonzero(sa1, &error));
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_head(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3, 4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.head(4) == tc_sarray_head(sa1, 4, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_tail(){
    tc_error* error = NULL;

    std::vector<double> v1 = {0, 2, 4.5, 9, 389, 23, 32,4,3, 3,4, 53, 53,5,3};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.tail(4) == tc_sarray_tail(sa1, 4, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_count_words(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);


    TS_ASSERT((g1.count_words(0) == tc_sarray_count_words(sa1, 0, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_count_word_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.count_ngrams(1, "word", false, true) == tc_sarray_count_word_ngrams(sa1, 1, false, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_count_character_ngrams(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0", "2", "4.5", "9", "389", "23", "32", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.count_ngrams(1, "character", false, true) == tc_sarray_count_character_ngrams(sa1, 1, false, false, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
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
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      CAPI_CHECK_ERROR(error);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_release(ft1);
      tc_release(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list_add_element(fl, ft, &error);
    CAPI_CHECK_ERROR(error);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_trim_by_keys(sa1, string_list, 1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1_modified = g1.dict_trim_by_keys(key_flexible, true);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_release(ft);
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
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flexible_type* ft2 = tc_ft_create_from_int64(p.second, &error);
      CAPI_CHECK_ERROR(error);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      CAPI_CHECK_ERROR(error);

      flexible_dictionary.push_back({p.first.c_str(), p.second});

      tc_release(ft1);
      tc_release(ft2);
    }

    turi::flexible_type lower(3);
    turi::flexible_type upper(5);

    tc_flexible_type* lower_flex = tc_ft_create_from_int64(3, &error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* upper_flex = tc_ft_create_from_int64(5, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list* fl = tc_flex_list_create(&error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list_add_element(fl, ft, &error);
    CAPI_CHECK_ERROR(error);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(lst1);

    tc_sarray* modified_dict = tc_sarray_dict_trim_by_value_range(sa1, lower_flex, upper_flex, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1_modified = g1.dict_trim_by_values(lower, upper);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_release(ft);
  };

  void test_tc_sarray_tc_sarray_max(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT((g1.max() == tc_sarray_max(sa1, &error)->value));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_tc_sarray_min(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT((g1.min() == tc_sarray_min(sa1, &error)->value));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_tc_sarray_sum(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT((g1.sum() == tc_sarray_sum(sa1, &error)->value));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_tc_sarray_mean(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.mean() == tc_sarray_mean(sa1, &error)->value));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_tc_sarray_std(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.std() == tc_sarray_std(sa1, &error)->value));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_nnz(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float(3.0);

    TS_ASSERT((g1.nnz() == tc_sarray_nnz(sa1, &error)));
    CAPI_CHECK_ERROR(error);

    tc_release(fl1);
    tc_release(ft1);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_num_missing(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.num_missing() == tc_sarray_num_missing(sa1, &error)));
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
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
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      CAPI_CHECK_ERROR(error);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_release(ft1);
      tc_release(ft2);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list_add_element(fl, ft, &error);
    CAPI_CHECK_ERROR(error);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(lst1);

    tc_sarray* modified_sa = tc_sarray_dict_keys(sa1, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray modified_sa_gl = g1.dict_keys();

    TS_ASSERT((modified_sa->value == modified_sa_gl).all());

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
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
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      CAPI_CHECK_ERROR(error);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_release(ft1);
      tc_release(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list_add_element(fl, ft, &error);
    CAPI_CHECK_ERROR(error);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_has_any_keys(sa1, string_list, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1_modified = g1.dict_has_any_keys(key_flexible);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_release(ft);
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
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;
    turi::flex_dict flexible_dictionary;

    for(auto p : data) {
      tc_flexible_type* ft1 = tc_ft_create_from_cstring(p.first.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flexible_type* ft2 = tc_ft_create_from_cstring(p.second.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_flex_dict_add_element(test_flex_dict, ft1, ft2, &error);
      CAPI_CHECK_ERROR(error);

      flexible_dictionary.push_back({p.first.c_str(), p.second.c_str()});

      tc_release(ft1);
      tc_release(ft2);
    }

    std::vector<turi::flexible_type> key_flexible;

    for(auto q : keys){
      turi::flexible_type q_string(q.c_str());

      key_flexible.push_back(q_string);
    }

    tc_flex_list* fl = tc_flex_list_create(&error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft = tc_ft_create_from_flex_dict(test_flex_dict, &error);
    CAPI_CHECK_ERROR(error);

    tc_flex_list_add_element(fl, ft, &error);
    CAPI_CHECK_ERROR(error);

    lst1.push_back(flexible_dictionary);

    tc_sarray* sa1 = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1(lst1);

    tc_flex_list* string_list = make_flex_list_string(keys);

    tc_sarray* modified_dict = tc_sarray_dict_has_all_keys(sa1, string_list, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray g1_modified = g1.dict_has_all_keys(key_flexible);


    TS_ASSERT((g1_modified == modified_dict->value).all());
    TS_ASSERT((g1 == sa1->value).all());

    tc_release(ft);
  };

  void test_tc_sarray_sample(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.sample(0.8, 2) == (tc_sarray_sample(sa1, 0.8, 2, &error)->value)).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_count_words_with_delimiters(){
    tc_error* error = NULL;

    std::vector<std::string> v1 = {"0\n2", "2\nr34\nr34", "4.5\nr34rr34\nr4", "9", "389", "23", "32\n43r4", "4", "3", "3", "4", "53", "53", "5", "3"};

    tc_flex_list* fl1 = make_flex_list_string(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

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
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_clip(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft2 = tc_ft_create_from_double(3, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float_1(1.0);
    turi::flexible_type f_float_2(3.0);

    TS_ASSERT((g1.clip(f_float_1, f_float_2) == tc_sarray_clip(sa1, ft1, ft2, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_drop_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT((g1.dropna() == tc_sarray_drop_na(sa1, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_replace_nan(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_flexible_type* ft1 = tc_ft_create_from_double(1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flexible_type f_float_1(1.0);

    TS_ASSERT((g1.fillna(f_float_1) == tc_sarray_replace_na(sa1, ft1, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_topk_index(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    TS_ASSERT((g1.topk_index(3, false) == tc_sarray_topk_index(sa1, 3, false, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_append(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};
    std::vector<double> v2 = {3, 2, 23, 53, 32, 345};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    tc_flex_list* fl2 = make_flex_list_double(v2);
    tc_sarray* sa2 = tc_sarray_create_from_list(fl2, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst2;

    for (auto it = v2.begin(); it!=v2.end(); ++it) {
        lst2.push_back(*it);
    }

    turi::gl_sarray g2(lst2);


    tc_sarray* combined_output = tc_sarray_append(sa1, sa2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = g1.append(g2);

    TS_ASSERT((combined_gl_output == combined_output->value).all());

    tc_release(fl1);
    tc_release(fl2);

    tc_release(sa1);
    tc_release(sa2);

    CAPI_CHECK_ERROR(error);
  };

  void test_tc_sarray_unique(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT((g1.unique() == tc_sarray_unique(sa1, &error)->value).all());
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(sa1);
  };

  void test_tc_sarray_is_materialized(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_materialize(){
    tc_error* error = NULL;

    std::vector<double> v1 = {1, 2, 4.5, 9, 389, 23};

    tc_flex_list* fl1 = make_flex_list_double(v1);
    tc_sarray* sa1 = tc_sarray_create_from_list(fl1, &error);
    CAPI_CHECK_ERROR(error);

    turi::flex_list lst1;

    for (auto it = v1.begin(); it!=v1.end(); ++it) {
        lst1.push_back(*it);
    }

    turi::gl_sarray g1(lst1);

    CAPI_CHECK_ERROR(error);

    tc_sarray* combined_output = tc_sarray_sample(sa1, 0.8, 2, &error);
    CAPI_CHECK_ERROR(error);

    turi::gl_sarray combined_gl_output = g1.sample(0.8, 2);

    TS_ASSERT((combined_output->value == combined_gl_output).all());

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    CAPI_CHECK_ERROR(error);

    tc_sarray_materialize(combined_output, &error);
    CAPI_CHECK_ERROR(error);

    combined_gl_output.materialize();

    TS_ASSERT((tc_sarray_is_materialized(combined_output, &error) == combined_gl_output.is_materialized()));
    CAPI_CHECK_ERROR(error);

    CAPI_CHECK_ERROR(error);
    tc_release(fl1);
    tc_release(sa1);
  };

  void test_tc_sarray_apply() {
    // Construct an SArray of arbitary double values.
    tc_error* error = nullptr;
    std::vector<double> v = {1, 2, 4.5, 9, 389, 23};
    tc_flex_list* fl = make_flex_list_double(v);
    tc_sarray* sa = tc_sarray_create_from_list(fl, &error);
    CAPI_CHECK_ERROR(error);

    // Create a simple lambda, which interprets each flexible type as a double
    // and doubles it.
    double multiplier = 2.0;
    auto lambda = [multiplier](
        tc_flexible_type* ft, tc_error** error) -> tc_flexible_type* {
      double value = tc_ft_double(ft, error);
      if (*error != nullptr) return nullptr;
      return tc_ft_create_from_double(value * multiplier, error);
    };
    using lambda_type = decltype(lambda);

    // Wrap the lambda as C function pointers and void* context.
    void* userdata = static_cast<void*>(new lambda_type(lambda));
    auto deleter = [](void* p) {
      auto* ptr = static_cast<lambda_type*>(p);
      delete ptr;
    };
    auto dispatcher = [](tc_flexible_type* ft, void* data, tc_error** error) {
      auto* ptr = static_cast<lambda_type*>(data);
      return (*ptr)(ft, error);
    };

    // Dispatch the wrapped lambda.
    tc_sarray* ret = tc_sarray_apply(
        sa, dispatcher, deleter, userdata, FT_TYPE_FLOAT, true, &error);
    CAPI_CHECK_ERROR(error);
    TS_ASSERT(ret != nullptr);

    // Verify the contents of the resulting SArray.
    uint64_t length = tc_sarray_size(ret);
    TS_ASSERT(length == v.size());
    for (uint64_t i = 0; i < length; ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(ret, i, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(tc_ft_is_double(ft));

      double val = tc_ft_double(ft, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(val == v[i] * 2.0);

      tc_release(ft);
    }

    // Clean up.
    tc_release(ret);
    tc_release(sa);
    tc_release(fl);
  }

  void test_range_sequence_division() {
    /*
     * 5/29/19, 12:12 PM Alejandro Isaza:
     * Summary:
     * Creating two columns with `tc_v1_sarray_create_from_sequence` and
     dividing them gives invalid results.

     * Results:
     * The result columns is [1, 1, 1, 1], expected it to be [0, 0, 0, 0].
     **/

    tc_error* error = nullptr;
    tc_sarray* col1 = tc_sarray_create_from_sequence(1, 5, &error);
    CAPI_CHECK_ERROR(error);
    tc_sarray* col2 = tc_sarray_create_from_sequence(2, 6, &error);
    CAPI_CHECK_ERROR(error);

    tc_sarray* result = tc_binary_op_ss(col1, "/", col2, &error);
    CAPI_CHECK_ERROR(error);

    using turi::flex_float;
    for (size_t i = 0; i < 4; ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(result, i, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(tc_ft_type(ft) == tc_ft_type_enum::FT_TYPE_FLOAT);
      flex_float result = tc_ft_double(ft, &error);
      CAPI_CHECK_ERROR(error);

      /* std::cout << tc_ft_type(ft) << " " << result << std::endl; */
      TS_ASSERT_EQUALS(result, static_cast<flex_float>(i + 1) / (i + 2));
    }

    tc_release(result);

    /* will use the optimizer to fuse 2 identical seq */
    tc_sarray* col3 = tc_sarray_create_from_sequence(1, 5, &error);
    CAPI_CHECK_ERROR(error);

    result = tc_binary_op_ss(col1, "/", col3, &error);
    CAPI_CHECK_ERROR(error);


    for (size_t i = 0; i < 4; ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(result, i, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(tc_ft_type(ft) == tc_ft_type_enum::FT_TYPE_FLOAT);
      turi::flex_int result = tc_ft_int64(ft, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT_EQUALS(result, 1);
    }
  }
};


BOOST_FIXTURE_TEST_SUITE(_capi_test_sarray, capi_test_sarray)
BOOST_AUTO_TEST_CASE(test_sarray_save_load) {
  capi_test_sarray::test_sarray_save_load();
}
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
BOOST_AUTO_TEST_CASE(test_sc_sarray_apply) {
  capi_test_sarray::test_tc_sarray_apply();
}

BOOST_AUTO_TEST_CASE(test_range_sequence_division) {
  capi_test_sarray::test_range_sequence_division();
}

BOOST_AUTO_TEST_SUITE_END()
