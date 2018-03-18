#ifndef TURI_CAPI_WRAPPER_STRUCTS_HPP_
#define TURI_CAPI_WRAPPER_STRUCTS_HPP_

#include <capi/TuriCore.h>
#include <unity/lib/variant.hpp>
#include <unity/lib/variant_converter.hpp>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/extensions/model_base.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <utility>

struct capi_struct_type_info {
  virtual const char* name() const = 0;
  virtual void free(const void* v) = 0;

  // TODO: memory pool implementations, etc.
};

// Each C API wrapper struct is intended to match a c++ container type that
// allows it to interface to pure C easily.  As such, each wrapper struct has the
// an instance of the wrapping type stored as the value field, and type_info to hold
// some type information associated with it for error checking and, in the 
// future, memory management.
//
// Creation of a wrapping struct should be done with new_* methods.

#define DECLARE_CAPI_WRAPPER_STRUCT(struct_name, wrapping_type)               \
  struct capi_struct_type_info_##struct_name;                                 \
                                                                              \
  /* The typeinfo executor needs to have a singleton instance. */             \
  extern capi_struct_type_info_##struct_name                                  \
      capi_struct_type_info_##struct_name##_inst;                             \
                                                                              \
  extern "C" {                                                                \
                                                                              \
  struct struct_name##_struct {                                               \
    capi_struct_type_info* type_info = nullptr;                               \
    wrapping_type value;                                                      \
  };                                                                          \
                                                                              \
  typedef struct struct_name##_struct struct_name;                            \
  }                                                                           \
                                                                              \
  struct capi_struct_type_info_##struct_name : public capi_struct_type_info { \
    const char* name() const { return #struct_name; }                         \
    void free(const void* v) {                                                \
      if (UNLIKELY(v == nullptr)) {                                           \
        return;                                                               \
      }                                                                       \
      const struct_name* vv = static_cast<const struct_name*>(v);             \
      ASSERT_TRUE(vv->type_info == this);                                     \
      ASSERT_TRUE(this == &(capi_struct_type_info_##struct_name##_inst));     \
      delete vv;                                                              \
    }                                                                         \
  };                                                                          \
                                                                              \
  static inline struct_name* new_##struct_name() {                            \
    struct_name* ret = new struct_name##_struct();                            \
    ret->type_info = &(capi_struct_type_info_##struct_name##_inst);           \
    return ret;                                                               \
  }                                                                           \
                                                                              \
  template <typename... Args>                                                 \
  static inline struct_name* new_##struct_name(Args&&... args) {              \
    struct_name* ret = new_##struct_name();                                   \
    ret->value = wrapping_type(std::forward<Args>(args)...);                  \
    return ret;                                                               \
  }

typedef std::map<std::string, turi::aggregate::groupby_descriptor_type> GROUPBY_AGGREGATOR_MAP;
// TODO: make this more full featured than just a string error message.
DECLARE_CAPI_WRAPPER_STRUCT(tc_error, std::string);
DECLARE_CAPI_WRAPPER_STRUCT(tc_datetime, turi::flex_date_time);
DECLARE_CAPI_WRAPPER_STRUCT(tc_flex_dict, turi::flex_dict);
DECLARE_CAPI_WRAPPER_STRUCT(tc_flex_list, turi::flex_list);
DECLARE_CAPI_WRAPPER_STRUCT(tc_flex_image, turi::flex_image);
DECLARE_CAPI_WRAPPER_STRUCT(tc_ndarray, turi::flex_nd_vec);
DECLARE_CAPI_WRAPPER_STRUCT(tc_flexible_type, turi::flexible_type);
DECLARE_CAPI_WRAPPER_STRUCT(tc_flex_enum_list, std::vector<turi::flex_type_enum>);
DECLARE_CAPI_WRAPPER_STRUCT(tc_string_list, std::vector<std::string>);
DECLARE_CAPI_WRAPPER_STRUCT(tc_double_list, std::vector<double>);
DECLARE_CAPI_WRAPPER_STRUCT(tc_sarray, turi::gl_sarray);
DECLARE_CAPI_WRAPPER_STRUCT(tc_sframe, turi::gl_sframe);
DECLARE_CAPI_WRAPPER_STRUCT(tc_variant, turi::variant_type);
DECLARE_CAPI_WRAPPER_STRUCT(tc_parameters, turi::variant_map_type);
DECLARE_CAPI_WRAPPER_STRUCT(tc_model, std::shared_ptr<turi::model_base>);
DECLARE_CAPI_WRAPPER_STRUCT(tc_groupby_aggregator, GROUPBY_AGGREGATOR_MAP);

#endif
