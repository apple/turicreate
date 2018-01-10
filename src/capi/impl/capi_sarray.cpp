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

  return new_tc_sarray(turi::gl_sarray::from_const(ft->value, n));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_sarray_create_copy(
    const tc_sarray* sa, tc_error** error) {

  ERROR_HANDLE_START();

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

  return new_tc_sarray(sa1->value + sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_minus_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value - sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_div_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value / sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_mult_sarray(
    const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value * sa2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_plus_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value + ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_minus_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value - ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_div_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value / ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_sarray* tc_op_sarray_mult_ft(
    const tc_sarray* sa1, const tc_flexible_type* ft2, tc_error** error){

  ERROR_HANDLE_START();

  return new_tc_sarray(sa1->value * ft2->value);

  ERROR_HANDLE_END(error, NULL);
}

// Call sum on the tc_sarray
EXPORT tc_flexible_type* tc_sarray_sum(const tc_sarray* sa, tc_error** error) { 

  ERROR_HANDLE_START();

  if(sa == NULL) { 
    set_error(error, "SArray passed in is null.");
    return NULL; 
  }

  return new_tc_flexible_type(sa->value.sum());

  ERROR_HANDLE_END(error, NULL);
}


EXPORT int tc_sarray_equals(const tc_sarray* sa1, const tc_sarray* sa2, tc_error** error) {

  ERROR_HANDLE_START();

  if(sa1 == NULL) { 
    set_error(error, "SArray passed in is null.");
    return sa2 != NULL; 
  }

  if(sa2 == NULL) { 
    set_error(error, "SArray passed in is null.");
    return sa1 != NULL; 
  }

  return (sa1->value == sa2->value).all();

  ERROR_HANDLE_END(error, 0);
}


// Wrap the printing.  Returns a string flexible type.
EXPORT tc_flexible_type* tc_sarray_text_summary(const tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();

  if(sa == NULL) { 
    set_error(error, "SArray passed in to summarize is null.");
    return NULL; 
  }

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

  if(sa == NULL) {
    set_error(error, "SArray passed in is null.");
    return NULL;
  }
  if(callback == NULL) {
    set_error(error, "callback function passed in is null.");
    return NULL;
  }

  apply_wrapper wrapper{callback, userdata_release_callback, userdata};
  return new_tc_sarray(sa->value.apply(wrapper, turi::flex_type_enum(type), skip_undefined));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT void tc_sarray_destroy(tc_sarray* sa) { 
  delete sa; 
}

}




