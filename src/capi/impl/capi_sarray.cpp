#include <capi/TuriCore.h>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_flexible_type.hpp>
#include <capi/impl/capi_flex_list.hpp>
#include <flexible_type/flexible_type.hpp>
#include <capi/impl/capi_sarray.hpp>
#include <export.hpp>
#include <sstream>

extern "C" {

EXPORT tc_sarray* tc_sarray_create(const tc_flex_list* data, tc_error** error) {
  ERROR_HANDLE_START();

  tc_sarray* ret = new_tc_sarray();
  ret->value = turi::gl_sarray(data->value);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sarray* tc_sarray_create_from_sequence(
    uint64_t start, uint64_t end, tc_error** error) {

  ERROR_HANDLE_START();

  uint64_t _start = std::min(start, end);
  uint64_t _end = std::max(start, end);
  bool reverse = (start > end);

  return new_tc_sarray(turi::gl_sarray::from_sequence(size_t(_start), size_t(_end), reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_from_const(
    const tc_flexible_type* ft, uint64_t n, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, ft, "flexible_type", NULL);

  return new_tc_sarray(turi::gl_sarray::from_const(ft->value, n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_copy(
    const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  return new_tc_sarray(turi::gl_sarray(sa->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_from_list(
    const tc_flex_list* fl, tc_error** error) {

  ERROR_HANDLE_START();

  if(fl == NULL) {
    set_error(error, "flex_list instance null.");
    return NULL;
  }

  return new_tc_sarray(turi::gl_sarray(fl->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_extract_element(
    const tc_sarray* sa, uint64_t index, tc_error** error) {

  ERROR_HANDLE_START();

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

// Gets the type of the sarray.
EXPORT tc_sarray* tc_op_sarray_plus_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);
  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value + sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_minus_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);
  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value - sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_div_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);
  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value / sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_mult_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);
  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value * sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_plus_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value + ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_minus_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value - ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_div_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value / ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_mult_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value * ft2->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sarray* tc_op_sarray_lt_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

    CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

    return new_tc_sarray(sa1->value < sa2->value);

    ERROR_HANDLE_END(error, NULL);

}

EXPORT tc_sarray* tc_op_sarray_gt_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

    CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

    return new_tc_sarray(sa1->value > sa2->value);

    ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_le_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

    CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

    return new_tc_sarray(sa1->value <= sa2->value);

    ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_ge_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

    CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

    return new_tc_sarray(sa1->value >= sa2->value);

    ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_eq_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

    ERROR_HANDLE_START();

    CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

    CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

    return new_tc_sarray(sa1->value == sa2->value);

    ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_lt_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value < ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_gt_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value > ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_ge_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value >= ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_le_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value <= ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_eq_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, ft2, "flexible_type", NULL);

  return new_tc_sarray(sa1->value == ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_logical_and_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value && sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_bitwise_and_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value & sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_logical_or_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value || sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_bitwise_or_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, sa2, "SArray 2", NULL);

  return new_tc_sarray(sa1->value | sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_apply_mask(
    const tc_sarray* sa1, const tc_sarray* mask, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "SArray 1", NULL);

  CHECK_NOT_NULL(error, mask, "mask", NULL);

  return new_tc_sarray(sa1->value[mask->value]);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int tc_sarray_all_nonzero(const tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  return sa1->value.all();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int tc_sarray_any_nonzero(const tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  return sa1->value.any();

  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_sarray_materialize(tc_sarray* sa1, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray");

  sa1->value.materialize();

  ERROR_HANDLE_END(error);
}

EXPORT tc_sarray* tc_sarray_head(const tc_sarray* src, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.head(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_tail(const tc_sarray* src, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.tail(n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_words(
    const tc_sarray* src, int to_lower, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_words(to_lower));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_words_with_delimiters(
    const tc_sarray* src, int to_lower, tc_flex_list* delimiters, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, delimiters, "flex_list", NULL);

  return new_tc_sarray(src->value.count_words(to_lower, delimiters->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_word_ngrams(
    const tc_sarray* src, uint64_t n, bool to_lower, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_ngrams(n, "word", to_lower, true));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_count_character_ngrams(
    const tc_sarray* src, size_t n, bool to_lower, bool ignore_space, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.count_ngrams(n, "character", to_lower, ignore_space));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_trim_by_keys(
    const tc_sarray* src, const tc_flex_list* keys, int exclude_keys, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_trim_by_keys(keys->value, exclude_keys));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_trim_by_value_range(
    const tc_sarray* src, const tc_flexible_type* lower, const tc_flexible_type* upper, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, lower, "flexible_type", NULL);
  CHECK_NOT_NULL(error, upper, "flexible_type", NULL);

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_trim_by_values(lower->value, upper->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_max(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.max());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_min(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.min());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_mean(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.mean());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_sarray_std(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_flexible_type(src->value.std());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_sarray_nnz(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.nnz();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT size_t tc_sarray_num_missing(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.num_missing();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_keys(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dict_keys());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_has_any_keys(const tc_sarray* src, const tc_flex_list* keys, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, keys, "flex_list", NULL);

  return new_tc_sarray(src->value.dict_has_any_keys(keys->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_dict_has_all_keys(const tc_sarray* src, const tc_flex_list* keys, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, keys, "flex_list", NULL);

  return new_tc_sarray(src->value.dict_has_all_keys(keys->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_sample(const tc_sarray* src, double fraction, uint64_t seed, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.sample(fraction, seed));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_datetime_to_str_with_format(const tc_sarray* src, const char* format, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.datetime_to_str(format));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_datetime_to_str(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.datetime_to_str());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_str_to_datetime(const tc_sarray* src, const char* format, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.str_to_datetime(format));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_clip(const tc_sarray* src, tc_flexible_type* lower, tc_flexible_type* upper, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, lower, "flexible_type", NULL);
  CHECK_NOT_NULL(error, upper, "flexible_type", NULL);

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.clip(lower->value, upper->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_drop_nan(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.dropna());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_replace_nan(const tc_sarray* src, tc_flexible_type* value, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, value, "flexible_type", NULL);

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.fillna(value->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_sarray* tc_sarray_topk_index(const tc_sarray* src, size_t topk, int reverse, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.topk_index(topk, reverse));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_append(const tc_sarray* src, const tc_sarray* other, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  CHECK_NOT_NULL(error, other, "sarray", NULL);

  return new_tc_sarray(src->value.append(other->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_unique(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return new_tc_sarray(src->value.unique());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int tc_sarray_is_materialized(const tc_sarray* src, tc_error** error){
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, src, "sarray", NULL);

  return src->value.is_materialized();

  ERROR_HANDLE_END(error, NULL);
}

// Call sum on the tc_sarray
EXPORT tc_flexible_type* tc_sarray_sum(const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  return new_tc_flexible_type(sa->value.sum());

  ERROR_HANDLE_END(error, NULL);
}


EXPORT int tc_sarray_equals(const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error) {

  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa1, "sarray", NULL);

  CHECK_NOT_NULL(error, sa2, "sarray", NULL);

  return (sa1->value == sa2->value).all();

  ERROR_HANDLE_END(error, 0);
}


// Wrap the printing.  Returns a string flexible type.
EXPORT tc_flexible_type* tc_sarray_text_summary(const tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa, "sarray", NULL);

  std::ostringstream ss;
  ss << sa->value;

  return new_tc_flexible_type(ss.str());

  ERROR_HANDLE_END(error, NULL);
}

class apply_wrapper {
 public:
   tc_flexible_type*(*callback)(tc_flexible_type*, void* userdata);
   void (*userdata_release_callback)(void* userdata);
   void* userdata;

   ~apply_wrapper() {
     if (userdata != NULL && userdata_release_callback != NULL) {
       userdata_release_callback(userdata);
     }
   }

   inline turi::flexible_type operator()(const turi::flexible_type& ft) const {
     tc_flexible_type in{ft};
     tc_flexible_type* out = callback(&in, userdata);
     turi::flexible_type ret = out->value;
     tc_ft_destroy(out);
     return ret;
   }
};

EXPORT tc_sarray* tc_sarray_apply(const tc_sarray* sa,
                           tc_flexible_type*(*callback)(tc_flexible_type*, void* userdata),
                           void (*userdata_release_callback)(void* userdata),
                           void* userdata,
                           tc_ft_type_enum type,
                           bool skip_undefined,
                           tc_error** error) {
  ERROR_HANDLE_START();

  CHECK_NOT_NULL(error, sa, "SArray passed in is null.", NULL);

  CHECK_NOT_NULL(error, callback, "Callback function passed in is null.", NULL);

  apply_wrapper wrapper{callback, userdata_release_callback, userdata};
  return new_tc_sarray(sa->value.apply(wrapper, turi::flex_type_enum(type), skip_undefined));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sarray_destroy(tc_sarray* sa) {
  delete sa;
}

}
