/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <export.hpp>
#include <sstream>

extern "C" {

EXPORT tc_sarray* tc_sarray_create_empty(tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sarray();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_from_sequence(
    uint64_t start, uint64_t end, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  uint64_t _start = std::min(start, end);
  uint64_t _end = std::max(start, end);
  bool reverse = (start > end);

  return new_tc_sarray(turi::gl_sarray::from_sequence(size_t(_start), size_t(_end), reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_from_const(
    const tc_flexible_type* ft, uint64_t n, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "flexible_type", NULL);

  return new_tc_sarray(turi::gl_sarray::from_const(ft->value, n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_copy(
    const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  return new_tc_sarray(turi::gl_sarray(sa->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_from_list(
    const tc_flex_list* fl, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(fl == NULL) {
    set_error(error, "flex_list instance null.");
    return NULL;
  }

  return new_tc_sarray(turi::gl_sarray(fl->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sarray* tc_sarray_load(const char* url, tc_error** error) { 
  
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sarray(turi::gl_sarray(url));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_sarray_save(const tc_sarray* sa, const char* url, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  
  CHECK_NOT_NULL(error, sa, "sarray");

  sa->value.save(url, "binary"); 

  ERROR_HANDLE_END(error);
} 

EXPORT void tc_sarray_save_as_text(const tc_sarray* sa, const char* url, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  
  CHECK_NOT_NULL(error, sa, "sarray");

  sa->value.save(url, "text"); 

  ERROR_HANDLE_END(error);
} 


EXPORT tc_flexible_type* tc_sarray_extract_element(
    const tc_sarray* sa, uint64_t index, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  if(sa == NULL) {
    set_error(error, "tc_sarray instance null.");
    return NULL;
  }

  if(index >= sa->value.size()) {
    set_error(error, "index out of range.");
    return NULL;
  }

  return new_tc_flexible_type(sa->value[index]);

  ERROR_HANDLE_END(error, NULL);
}

// Gets the sarry size.
EXPORT uint64_t tc_sarray_size(const tc_sarray* sa) {
  return (sa != NULL) ? sa->value.size() : 0;
}


// Gets the type of the sarray.
EXPORT tc_ft_type_enum tc_sarray_type(const tc_sarray* sa) {
  return (sa != NULL) ? static_cast<tc_ft_type_enum>(sa->value.dtype()) : FT_TYPE_UNDEFINED;
}

/*******************************************************************************/
EXPORT tc_sarray* tc_sarray_apply_mask(
    const tc_sarray* sa1, const tc_sarray* mask, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, mask, "mask", NULL);

  return new_tc_sarray(sa1->value[mask->value]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sarray_all_nonzero(const tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  return sa1->value.all();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sarray_any_nonzero(const tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  return sa1->value.any();

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_sarray_materialize(tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "sarray");

  sa1->value.materialize();

  ERROR_HANDLE_END(error);
}

EXPORT tc_sarray* tc_sarray_head(const tc_sarray* src, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.head(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_tail(const tc_sarray* src, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.tail(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_words(
    const tc_sarray* src, int to_lower, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_words(to_lower));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_words_with_delimiters(
    const tc_sarray* src, int to_lower, tc_flex_list* delimiters, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, delimiters, "flex_list", NULL);

  return new_tc_sarray(src->value.count_words(to_lower, delimiters->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_word_ngrams(
    const tc_sarray* src, uint64_t n, bool to_lower, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_ngrams(n, "word", to_lower, true));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_character_ngrams(
    const tc_sarray* src, size_t n, bool to_lower, bool ignore_space, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_ngrams(n, "character", to_lower, ignore_space));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_trim_by_keys(
    const tc_sarray* src, const tc_flex_list* keys, int exclude_keys, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_trim_by_keys(keys->value, exclude_keys));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_trim_by_value_range(
    const tc_sarray* src, const tc_flexible_type* lower, const tc_flexible_type* upper, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, lower, "flexible_type", NULL);
  CHECK_NOT_NULL(error, upper, "flexible_type", NULL);

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_trim_by_values(lower->value, upper->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_max(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.max());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_min(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.min());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_mean(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.mean());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_std(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.std());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sarray_nnz(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.nnz();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT size_t tc_sarray_num_missing(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.num_missing();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_keys(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_keys());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_has_any_keys(const tc_sarray* src, const tc_flex_list* keys, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, keys, "flex_list", NULL);

  return new_tc_sarray(src->value.dict_has_any_keys(keys->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_has_all_keys(const tc_sarray* src, const tc_flex_list* keys, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, keys, "flex_list", NULL);

  return new_tc_sarray(src->value.dict_has_all_keys(keys->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_sample(const tc_sarray* src, double fraction, uint64_t seed, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.sample(fraction, seed));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_datetime_to_str_with_format(const tc_sarray* src, const char* format, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.datetime_to_str(format));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_datetime_to_str(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.datetime_to_str());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_str_to_datetime(const tc_sarray* src, const char* format, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.str_to_datetime(format));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_clip(const tc_sarray* src, const tc_flexible_type* lower, const tc_flexible_type* upper, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, lower, "flexible_type", NULL);
  CHECK_NOT_NULL(error, upper, "flexible_type", NULL);

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.clip(lower->value, upper->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_drop_na(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dropna());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_to_type(const tc_sarray* src, tc_ft_type_enum dtype, bool undefined_on_failure, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.astype(static_cast<turi::flex_type_enum>(dtype), undefined_on_failure));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_replace_na(const tc_sarray* src, const tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.fillna(value->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sarray* tc_sarray_topk_index(const tc_sarray* src, size_t topk, bool reverse, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.topk_index(topk, reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_append(const tc_sarray* src, const tc_sarray* other, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, other, "sarray", NULL);

  return new_tc_sarray(src->value.append(other->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_unique(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.unique());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sarray_is_materialized(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.is_materialized();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT bool tc_sarray_size_is_known(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.get_proxy()->has_size();

  ERROR_HANDLE_END(error, false);
}

// Call sum on the tc_sarray
EXPORT tc_flexible_type* tc_sarray_sum(const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  return new_tc_flexible_type(sa->value.sum());

  ERROR_HANDLE_END(error, NULL);
}


EXPORT bool tc_sarray_equals(const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, sa2, "sarray", NULL);

  return (sa1->value == sa2->value).all();

  ERROR_HANDLE_END(error, 0);
}


// Wrap the printing.  Returns a string flexible type.
EXPORT tc_flexible_type* tc_sarray_text_summary(const tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  std::ostringstream ss;
  ss << sa->value;

  return new_tc_flexible_type(ss.str());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_apply(
    const tc_sarray* sa,
    tc_flexible_type* (*callback)(
        tc_flexible_type* ft, void* context, tc_error** error),
    void (*context_release_callback)(void* context),
    void* context,
    tc_ft_type_enum type,
    bool skip_undefined,
    tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "SArray passed in is null.", NULL);
  CHECK_NOT_NULL(error, callback, "Callback function passed in is null.", NULL);
  if (context != nullptr) {
    CHECK_NOT_NULL(error, context_release_callback,
                   "Context release function passed in is null.", nullptr);
  }

  // Use shared_ptr to ensure that the user data is released exactly once, after
  // all copies of the lambda below have been destroyed.
  std::shared_ptr<void> shared_context(context, context_release_callback);
  auto wrapper = [callback, shared_context](const turi::flexible_type& ft) {
    tc_error* error = nullptr;

    // Invoke the user callback.
    tc_flexible_type in;
    in.value = ft;
    tc_flexible_type* out;
    out = callback(&in, shared_context.get(), &error);

    // Propagate errors from user code up to whatever C-API throw-catch block
    // (hopefully) encloses the call that triggered this wrapper's invocation.
    if (error != nullptr) {
      std::string message = std::move(error->value);
      tc_release(&error);
      if (out != nullptr) tc_release(out);
      throw message;
    }
    if (out == nullptr) {
      throw std::string("Callback provided to tc_sarray_apply returned null "
                        "without setting error");
    }

    // Return the value that the callback produced.
    turi::flexible_type ret = out->value;
    tc_release(out);
    return ret;
  };

  return new_tc_sarray(sa->value.apply(
      std::move(wrapper), turi::flex_type_enum(type), skip_undefined));

  ERROR_HANDLE_END(error, NULL);
}


// Reduction operations: pass in op as string.  E.g. min, max, sum, mean, std, etc.
EXPORT tc_flexible_type* tc_sarray_reduce(const tc_sarray* sa, const char* op, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "tc_sarray", NULL);

  enum class reduce_op {
    OP_MIN = 0,
    OP_MAX = 1,
    OP_SUM = 2,
    OP_MEAN = 3,
    OP_STD = 4
  };

  static std::map<std::string, reduce_op> _op_map =  //
      {{"min", reduce_op::OP_MIN},
       {"max", reduce_op::OP_MAX},
       {"sum", reduce_op::OP_SUM},
       {"mean", reduce_op::OP_MEAN},
       {"std", reduce_op::OP_STD}};

  auto it = _op_map.find(op);

  if(it == _op_map.end()) {
    std::ostringstream ss;
    ss << "Reduction operator " << op << " not recognized. "
       << "Available operators are ";
    for(const auto& p : _op_map) {
      ss << p.first << " ";
    }
    ss << ".";

    throw std::invalid_argument(ss.str());
  }

  switch (it->second) {
    case reduce_op::OP_MIN:
      return new_tc_flexible_type(sa->value.min());
    case reduce_op::OP_MAX:
      return new_tc_flexible_type(sa->value.max());
    case reduce_op::OP_SUM:
      return new_tc_flexible_type(sa->value.sum());
    case reduce_op::OP_MEAN:
      return new_tc_flexible_type(sa->value.mean());
    case reduce_op::OP_STD:
      return new_tc_flexible_type(sa->value.std());
  }
  return NULL;

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_hash(const tc_sarray* sa, uint64_t salt, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "tc_sarray", NULL);

  return new_tc_sarray(turi::gl_sarray(sa->value.get_proxy()->hash(salt)));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_slice(const tc_sarray* sf, const int64_t start, const int64_t slice, const int64_t end, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sarray(
      sf->value[{static_cast<long long>(start), static_cast<long long>(slice),
                 static_cast<long long>(end)}]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_subslice(const tc_sarray* sf, const int64_t start, const int64_t slice, const int64_t end, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_sarray(sf->value.subslice(start, slice, end));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_to_const(const tc_sarray* sa, const tc_flexible_type* value, tc_ft_type_enum out_type, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "tc_sarray", NULL);
  CHECK_NOT_NULL(error, value, "tc_flexible_type", NULL);

  return new_tc_sarray(turi::gl_sarray(sa->value.get_proxy()->to_const(value->value, static_cast<turi::flex_type_enum>(out_type))));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_which(const tc_sarray* mask,
                                  const tc_sarray* true_sa,
                                  const tc_sarray* false_sa, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, mask, "tc_sarray", NULL);
  CHECK_NOT_NULL(error, true_sa, "tc_sarray", NULL);
  CHECK_NOT_NULL(error, false_sa, "tc_sarray", NULL);

  return new_tc_sarray(
      turi::gl_sarray(mask->value.get_proxy()->ternary_operator(
          true_sa->value.get_proxy(), false_sa->value.get_proxy())));

  ERROR_HANDLE_END(error, NULL);

}

EXPORT tc_sarray* tc_sarray_sort(const tc_sarray* sa, bool ascending, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "tc_sarray", NULL);

  return new_tc_sarray(sa->value.sort(ascending));

  ERROR_HANDLE_END(error, NULL);
}

}
