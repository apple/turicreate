/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SARRAY_INTERFACE_HPP
#define TURI_UNITY_SARRAY_INTERFACE_HPP
#include <memory>
#include <vector>
#include <string>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/api/function_closure_info.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <cppipc/magic_macros.hpp>

namespace turi {

class unity_sframe_base;
typedef std::map<std::string, flexible_type> func_options_map;

GENERATE_INTERFACE_AND_PROXY(unity_sarray_base, unity_sarray_proxy,
      (void, construct_from_vector, (const std::vector<flexible_type>&)(flex_type_enum))
      (void, construct_from_const, (const flexible_type&)(size_t)(flex_type_enum))
      (void, construct_from_sarray_index, (std::string))
      (void, construct_from_files, (std::string)(flex_type_enum))
      (void, construct_from_autodetect, (std::string)(flex_type_enum))
      (void, construct_from_json_record_files, (std::string))
      (void, save_array, (std::string) )
      (void, clear, )
      (size_t, size, )
      (bool, has_size, )
      (flex_type_enum, dtype, )
      (std::shared_ptr<unity_sarray_base>, head, (size_t))
      (std::vector<flexible_type>, _head, (size_t))
      (std::shared_ptr<unity_sarray_base>, vector_slice, (size_t)(size_t))
      (std::shared_ptr<unity_sarray_base>, transform, (const std::string&)(flex_type_enum)(bool)(int))
      (std::shared_ptr<unity_sarray_base>, transform_native, (const function_closure_info&)(flex_type_enum)(bool)(int))
      (std::shared_ptr<unity_sarray_base>, filter, (const std::string&)(bool)(int))
      (std::shared_ptr<unity_sarray_base>, logical_filter, (std::shared_ptr<unity_sarray_base>))
      (std::shared_ptr<unity_sarray_base>, topk_index, (size_t)(bool))
      (bool, all, )
      (bool, any, )
      (flexible_type, max, )
      (flexible_type, min, )
      (flexible_type, sum, )
      (flexible_type, mean, )
      (flexible_type, std, (size_t))
      (flexible_type, var, (size_t))
      (size_t, num_missing, )
      (size_t, nnz, )
      (std::shared_ptr<unity_sarray_base>, astype, (flex_type_enum)(bool))
      (std::shared_ptr<unity_sarray_base>, lazy_astype, (flex_type_enum)(bool))
      (std::shared_ptr<unity_sarray_base>, datetime_to_str,(std::string))
      (std::shared_ptr<unity_sarray_base>, str_to_datetime,(std::string))
      (std::shared_ptr<unity_sarray_base>, left_scalar_operator, (flexible_type)(std::string))
      (std::shared_ptr<unity_sarray_base>, right_scalar_operator, (flexible_type)(std::string))
      (std::shared_ptr<unity_sarray_base>, vector_operator, (std::shared_ptr<unity_sarray_base>)(std::string))
      (std::shared_ptr<unity_sarray_base>, drop_missing_values, )
      (std::shared_ptr<unity_sarray_base>, fill_missing_values, (flexible_type))
      (std::shared_ptr<unity_sarray_base>, clip, (flexible_type)(flexible_type))
      (std::shared_ptr<unity_sarray_base>, sample, (float)(int)(bool))
      (std::shared_ptr<unity_sarray_base>, hash, (int))
      (std::shared_ptr<unity_sarray_base>, tail, (size_t))
      (std::vector<flexible_type>, _tail, (size_t))
      (void, begin_iterator, )
      (std::vector<flexible_type>, iterator_get_next, (size_t))
      (void, materialize, )
      (bool, is_materialized, )
      (std::shared_ptr<unity_sarray_base>, append, (std::shared_ptr<unity_sarray_base>))
      (std::shared_ptr<unity_sarray_base>, count_bag_of_words, (func_options_map))
      (std::shared_ptr<unity_sarray_base>, count_character_ngrams, (size_t) (func_options_map))
      (std::shared_ptr<unity_sarray_base>, count_ngrams, (size_t) (func_options_map))
      (std::shared_ptr<unity_sarray_base>, dict_trim_by_keys, (const std::vector<flexible_type>&)(bool))
      (std::shared_ptr<unity_sarray_base>, dict_trim_by_values, (const flexible_type&)(const flexible_type&))
      (std::shared_ptr<unity_sarray_base>, dict_keys,)
      (std::shared_ptr<unity_sarray_base>, dict_values,)
      (std::shared_ptr<unity_sarray_base>, dict_has_any_keys,(const std::vector<flexible_type>&))
      (std::shared_ptr<unity_sarray_base>, dict_has_all_keys,(const std::vector<flexible_type>&))
      (std::shared_ptr<unity_sarray_base>, item_length, )
      (std::shared_ptr<unity_sframe_base>, unpack_dict, (const std::string&)(const std::vector<flexible_type>&)(const flexible_type&))
      (std::shared_ptr<unity_sframe_base>, expand, (const std::string&)(const std::vector<flexible_type>&)(const std::vector<flex_type_enum>&))
      (std::shared_ptr<unity_sframe_base>, unpack, (const std::string&)(const std::vector<flexible_type>&)(const std::vector<flex_type_enum>&)(const flexible_type&))
      (size_t, get_content_identifier, )
      (std::shared_ptr<unity_sarray_base>, copy_range, (size_t)(size_t)(size_t))
      (std::shared_ptr<unity_sarray_base>, subslice, (flexible_type)(flexible_type)(flexible_type))
      (std::vector<flexible_type>, to_vector, )
      (std::shared_ptr<unity_sarray_base>, builtin_rolling_apply,(const std::string&)(ssize_t)(ssize_t)(size_t))
      (std::shared_ptr<unity_sarray_base>, builtin_cumulative_aggregate,(const std::string&))
      (std::shared_ptr<unity_sarray_base>, ternary_operator,(std::shared_ptr<unity_sarray_base>)(std::shared_ptr<unity_sarray_base>))
      (std::shared_ptr<unity_sarray_base>, to_const,(const flexible_type&)(flex_type_enum))
      (void, show, (const std::string&)(const std::string&)(const std::string&)(const std::string&))
      (std::shared_ptr<model_base>, plot,(const std::string&)(const std::string&)(const std::string&)(const std::string&))
    )
} // namespace turi
#endif // TURI_UNITY_SARRAY_INTERFACE_HPP
#include <unity/lib/api/unity_sframe_interface.hpp>
