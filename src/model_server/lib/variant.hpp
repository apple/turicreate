/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_VARIANT_HPP
#define TURI_UNITY_VARIANT_HPP
#include <string>
#include <map>
#include <vector>
#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/get.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/dataframe.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <model_server/lib/variant.hpp>

namespace turi {
class model_base;
struct function_closure_info;
class unity_sframe_base;
class unity_sgraph_base;
class unity_sarray_base;

/**
 * A variant object that can be communicated between Python and C++ which
 * contains either a
 * \li flexible_type
 * \li std::shared_ptr<unity_sgraph>
 * \li dataframe_t
 * \li model
 * \li std::shared_ptr<unity_sframe>
 * \li std::shared_ptr<unity_sarray>
 * \li std::map<variant>
 * \li std::vector<variant>
 *
 * See the boost variant documentation for details.
 *
 * The variant should not be accessed directly. See \ref to_variant
 * and \ref variant_get_value for powerful ways to extract or store values
 * from a variant.
 */
typedef typename boost::make_recursive_variant<
            flexible_type,
            std::shared_ptr<unity_sgraph_base>,
            dataframe_t,
            std::shared_ptr<model_base>,
            std::shared_ptr<unity_sframe_base>,
            std::shared_ptr<unity_sarray_base>,
            std::map<std::string, boost::recursive_variant_>,
            std::vector<boost::recursive_variant_>,
            boost::recursive_wrapper<function_closure_info> >::type variant_type;

/*
 * A map of string to variant. Also a type the variant type can store.
 */
typedef std::map<std::string, variant_type> variant_map_type;

/*
 * A map of vector to variant. Also a type that the variant_type can store
 */
typedef std::vector<variant_type> variant_vector_type;


/**
 * Given variant.which() gets the name of the type inside it.
 */
inline std::string get_variant_which_name(int i) {
  switch(i) {
   case 0:
     return "flexible_type";
   case 1:
     return "SGraph";
   case 2:
     return "Dataframe";
   case 3:
     return "Model";
   case 4:
     return "SFrame";
   case 5:
     return "SArray";
   case 6:
     return "Dictionary";
   case 7:
     return "List";
   case 8:
     return "Function";
   default:
     return "";
  }
}

} // namespace turi


namespace turi{ namespace archive_detail {

template <>
struct serialize_impl<oarchive, turi::variant_type, false> {
  static void exec(oarchive& arc, const turi::variant_type& tval);
};

template <>
struct deserialize_impl<iarchive, turi::variant_type, false> {
  static void exec(iarchive& arc, turi::variant_type& tval);
};
}
}



namespace turi {
// A list of accessors to help Cython access the variant
template <typename T>
GL_COLD_NOINLINE_ERROR
void _throw_variant_error(const variant_type& v) {
    std::string errormsg =  //
        std::string("Variant type error: Expecting ") +
        get_variant_which_name(variant_type(T()).which()) + " but got a " +
        get_variant_which_name(v.which());
    log_and_throw(errormsg);
}


/**
 * Gets a reference to a content of a variant.
 * Throws if variant contains an inappropriate type.
 */
template <typename T>
static inline T& variant_get_ref(variant_type& v) {
  try {
    return boost::get<T>(v);
  } catch (...) {
    _throw_variant_error<T>(v);
  }
}

/**
 * Gets a const reference to the content of a variant.
 * Throws if variant contains an inappropriate type.
 */
template <typename T>
static inline const T& variant_get_ref(const variant_type& v) {
  try {
  return boost::get<T>(v);
  } catch (...) {
    _throw_variant_error<T>(v);
  }
}





// Convenience functions
template <typename T>
GL_HOT_INLINE_FLATTEN
inline bool variant_is(const variant_type&) {
   return false;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flexible_type>(const variant_type& t) {
   return t.which() == 0;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_string>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::STRING);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_vec>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::VECTOR);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_int>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::INTEGER);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_float>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::FLOAT);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_list>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::LIST);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_dict>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::DICT);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_image>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::IMAGE);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_date_time>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::DATETIME);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_nd_vec>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::ND_VECTOR);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<flex_undefined>(const variant_type& t) {
   return variant_is<flexible_type>(t) && (variant_get_ref<flexible_type>(t).get_type() == flex_type_enum::UNDEFINED);
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<std::shared_ptr<unity_sgraph_base> >(const variant_type& t) {
   return t.which() == 1;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<dataframe_t>(const variant_type& t) {
   return t.which() == 2;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<std::shared_ptr<model_base> >(const variant_type& t) {
   return t.which() == 3;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<std::shared_ptr<unity_sframe_base> >(const variant_type& t) {
   return t.which() == 4;
}

class gl_sframe;

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<gl_sframe>(const variant_type& t) {
   return t.which() == 4;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<std::shared_ptr<unity_sarray_base> >(const variant_type& t) {
   return t.which() == 5;
}

class gl_sarray;

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<gl_sarray>(const variant_type& t) {
   return t.which() == 5;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<variant_map_type>(const variant_type& t) {
   return t.which() == 6;
}

template <>
GL_HOT_INLINE_FLATTEN
inline bool variant_is<boost::recursive_wrapper<function_closure_info> >(const variant_type& t) {
   return t.which() == 7;
}

}


#include <model_server/lib/variant_converter.hpp>

namespace turi {
/**
 * Stores an arbitrary value in a variant
 */
template <typename T>
inline void variant_set_value(variant_type& v, const T& f) {
  v = variant_converter<typename std::decay<T>::type>().set(f);
}


/**
 * Converts an arbitrary value to a variant.
 * T can be \b alot of possibilities. See the \ref sec_supported_type_list
 * "supported type list" for details.
 */
template <typename T>
inline variant_type to_variant(const T& f) {
  return variant_converter<typename std::decay<T>::type>().set(f);
}
} // namespace turi

namespace turi {
/**
 * Reads an arbitrary type from a variant.
 * T can be \b alot of possibilities. See the \ref sec_supported_type_list
 * "supported type list" for details.
 */
template <typename T>
inline typename std::decay<T>::type variant_get_value(const variant_type& v) {
  return variant_converter<typename std::decay<T>::type>().get(v);
}

} // namespace turi
#include <model_server/lib/api/function_closure_info.hpp>
#endif
