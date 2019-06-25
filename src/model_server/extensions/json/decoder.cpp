/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "decoder.hpp"
#include "types.hpp"

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sgraph.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

using namespace turi;

typedef std::map<std::string, variant_type> schema_t;

// forward declarations
static variant_type _any_from_serializable(const flexible_type& data, const schema_t& schema);
static schema_t schema_from_variant(const variant_type& v);

static flexible_type _dict_get(const flex_dict& dict, const flex_string& key) {
  for (const auto& kv : dict) {
    if (kv.first == key) {
      return kv.second;
    }
  }
  std::stringstream msg;
  msg << "Expected key \"";
  msg << key;
  msg << "\" was not present in dictionary input.";
  log_and_throw(msg.str());
}

static std::string _get_type(const schema_t& schema) {
  variant_type schema_type_as_variant = schema.at("type");
  return variant_get_value<flex_string>(schema_type_as_variant);
}

static bool _is_type(const schema_t& schema, const std::string& type) {
  if(schema.size() == 0) {
    log_and_throw("Malformed schema object provided. Expected a dictionary with at least one key named \"type\".");
  }
  return _get_type(schema) == type;
}

static void _check_type(const schema_t& schema, const std::string& type) {
  ASSERT_TRUE(_is_type(schema, type));
}

static variant_type _list_from_serializable(const flexible_type& data, const schema_t& schema) {
  // TODO distinguish list and vector
  flex_type_enum data_type = data.get_type();
  if (data_type == flex_type_enum::LIST) {
    flex_list data_list = data;
    if (_is_type(schema, JSON::types::LIST)) {
      // list asked for and list passed in
      // types inside could be anything, so we need to deserialize values
      // if a nested schema is provided.
      if (schema.find("nested") == schema.end()) {
        return data;
      }

      std::vector<variant_type> ret;
      variant_type nested_schema_variant = schema.at("nested");
      flex_list nested_schema = variant_get_value<flexible_type>(nested_schema_variant);
      assert(nested_schema.size() == data_list.size());
      for (size_t i=0; i<data_list.size(); i++) {
        flexible_type serialized_value = data_list[i];
        schema_t serialized_schema = schema_from_variant(nested_schema[i]);
        variant_type deserialized_value = _any_from_serializable(serialized_value, serialized_schema);
        ret.push_back(deserialized_value);
      }
      return ret;
    }

    // only other possibility for schema is vector
    _check_type(schema, JSON::types::VECTOR);
    flex_vec ret;
    for (const auto& value : data_list) {
      schema_t value_schema;
      value_schema.insert(std::make_pair("type", JSON::types::FLOAT));
      variant_type deserialized_value = _any_from_serializable(value, value_schema);
      ret.push_back(variant_get_value<flex_float>(deserialized_value));
    }
    return ret;
  } else {
    // this case is needed due to auto-conversion in flexible-type
    // from flex_list (of all number) to flex_vec
    assert(data_type == flex_type_enum::VECTOR);
    if (_is_type(schema, JSON::types::VECTOR)) {
      // vector asked for and vector passed in
      return data;
    }

    // only other possibility for schema is list
    _check_type(schema, JSON::types::LIST);
    flex_list ret = data;
    return ret;
  }
}

static flex_string _decode_image(const flex_string &encoded) {
  typedef boost::archive::iterators::transform_width<
    boost::archive::iterators::binary_from_base64<
      flex_string::const_iterator
    >,
    8,
    6
  > from_base64;

  return boost::algorithm::trim_right_copy_if(
    std::string(from_base64(std::begin(encoded)), from_base64(std::end(encoded))),
    [](char c) {
      return c == '\0';
    }
  );
}

static variant_type _dict_from_serializable(const flexible_type& data, const schema_t& schema) {
  // TODO distinguish dict from SGraph
  flex_dict data_dict = data;
  if (_is_type(schema, JSON::types::DATETIME)) {
    flex_date_time ret;
    int64_t posix_timestamp = _dict_get(data_dict, "posix_timestamp");
    flexible_type tz_flex = _dict_get(data_dict, "tz_15_min_offset");
    int32_t tz_15_min_offset;
    if (tz_flex.get_type() == flex_type_enum::UNDEFINED) {
      tz_15_min_offset = flex_date_time::EMPTY_TIMEZONE;
    } else {
      tz_15_min_offset = tz_flex.get<flex_int>();
    }
    int32_t microsecond = _dict_get(data_dict, "microsecond");
    return flex_date_time(
      posix_timestamp,
      tz_15_min_offset,
      microsecond
    );
  } else if (_is_type(schema, JSON::types::IMAGE)) {
    flex_string image_data_str = _dict_get(data_dict, "image_data");
    image_data_str = _decode_image(image_data_str);
    const char * image_data = image_data_str.c_str();
    flex_int height = _dict_get(data_dict, "height");
    flex_int width = _dict_get(data_dict, "width");
    flex_int channels = _dict_get(data_dict, "channels");
    flex_int image_data_size = _dict_get(data_dict, "size");
    flex_int version = _dict_get(data_dict, "version");
    flex_int format = _dict_get(data_dict, "format");
    return flex_image(
      image_data,
      height,
      width,
      channels,
      image_data_size,
      version,
      format
    );
  } else if (_is_type(schema, JSON::types::SARRAY)) {
    std::string dtype_str = _dict_get(data_dict, "dtype");
    flex_type_enum dtype = flex_type_enum_from_name(dtype_str);
    std::vector<flexible_type> values = _dict_get(data_dict, "values");

    std::vector<flexible_type> deserialized_values;

    schema_t value_schema;
    value_schema.insert(std::make_pair("type", dtype_str));
    for (const auto& value : values) {
      if (value == FLEX_UNDEFINED) {
        // skip deserialization (which enforces schema) for null values
        // otherwise we get NoneType != dtype
        deserialized_values.push_back(value);
      } else {
        variant_type deserialized_value = _any_from_serializable(value, value_schema);
        deserialized_values.push_back(variant_get_value<flexible_type>(deserialized_value));
      }
    }
    return gl_sarray(deserialized_values, dtype);
  } else if (_is_type(schema, JSON::types::SFRAME)) {
    gl_sframe ret;
    flex_list column_names = _dict_get(data_dict, "column_names");
    flex_list serializable_columns = _dict_get(data_dict, "columns");
    if (column_names.size() != serializable_columns.size()) {
      log_and_throw("Array length mismatch in serializable SFrame data. Expected column_names to be the same length as columns.");
    }
    for (size_t i=0; i<column_names.size(); i++) {
      std::string column_name = column_names[i];
      flex_dict serializable_column = serializable_columns[i];
      schema_t sarray_schema;
      sarray_schema.insert(std::make_pair("type", JSON::types::SARRAY));
      variant_type deserialized_column = _any_from_serializable(serializable_column, sarray_schema);
      ret[column_name] = variant_get_value<gl_sarray>(deserialized_column);
    }
    return ret;
  } else if (_is_type(schema, JSON::types::SGRAPH)) {
    schema_t sframe_schema;
    sframe_schema.insert(std::make_pair("type", JSON::types::SFRAME));

    flexible_type serialized_vertices = _dict_get(data_dict, "vertices");
    variant_type deserialized_vertices = _any_from_serializable(serialized_vertices, sframe_schema);
    gl_sframe vertices = variant_get_value<gl_sframe>(deserialized_vertices);

    flexible_type serialized_edges = _dict_get(data_dict, "edges");
    variant_type deserialized_edges = _any_from_serializable(serialized_edges, sframe_schema);
    gl_sframe edges = variant_get_value<gl_sframe>(deserialized_edges);

    return gl_sgraph(vertices, edges);
  } else {
    _check_type(schema, JSON::types::DICT);
    if (schema.find("nested") != schema.end()) {
      schema_t nested_schema = schema_from_variant(schema.at("nested"));
      std::map<std::string, variant_type> ret;
      for (const auto& kv : data_dict) {
        flexible_type serialized_value = kv.second;
        schema_t value_schema = schema_from_variant(nested_schema.at(kv.first));
        variant_type deserialized_value = _any_from_serializable(serialized_value, value_schema);
        ret.insert(std::make_pair(kv.first, deserialized_value));
      }
      return ret;
    } else {
      // no nested schema provided (as would be in the case of SArray)
      // don't try to deserialize, just return the value as-is.
      return data;
    }
  }
}

static flexible_type _string_from_serializable(const flexible_type& data, const schema_t& schema) {
  if (_is_type(schema, JSON::types::FLOAT)) {
    flex_string data_str = data.get<flex_string>();
    if (data_str == "NaN") {
      return NAN;
    } else if (data_str == "Infinity") {
      return std::numeric_limits<flex_float>::infinity();
    } else if (data_str == "-Infinity") {
      return -std::numeric_limits<flex_float>::infinity();
    } else {
      log_and_throw("Unexpected value to _string_from_serializable with float type tag. Expected \"NaN\", \"Infinity\", or \"-Infinity\".");
    }
  } else {
    _check_type(schema, JSON::types::STRING);
    return data;
  }
}

static flexible_type _float_from_serializable(flex_float data, const schema_t& schema) {
  if (_is_type(schema, JSON::types::INTEGER)) {
    // ints can come in as floats due to automatic int->float conversion in
    // flexible_type (more specifically, flex_list -> flex_vec when all values
    // are numbers).
    return static_cast<flex_int>(data);
  }
  _check_type(schema, JSON::types::FLOAT);
  return data;
}

static variant_type _any_from_serializable(const flexible_type& data, const schema_t& schema) {
  switch (data.get_type()) {
    case flex_type_enum::INTEGER:
      _check_type(schema, JSON::types::INTEGER);
      return data.get<flex_int>();
    case flex_type_enum::FLOAT:
      return _float_from_serializable(data.get<flex_float>(), schema);
    case flex_type_enum::STRING:
      return _string_from_serializable(data, schema);
    case flex_type_enum::VECTOR:
    case flex_type_enum::LIST:
      return _list_from_serializable(data, schema);
    case flex_type_enum::UNDEFINED:
      _check_type(schema, JSON::types::UNDEFINED);
      return FLEX_UNDEFINED;
    case flex_type_enum::DICT:
      return _dict_from_serializable(data, schema);
    case flex_type_enum::DATETIME:
    case flex_type_enum::IMAGE:
      log_and_throw("Unexpected input to _any_from_serializable: serializable flex_type does not include datetime or Image types.");
    case flex_type_enum::ND_VECTOR:
      log_and_throw("Unsupported flex_type_enum case: ND_VECTOR");
      break;
    default:
      log_and_throw("Unsupported flex_type_enum case");
      break;
  }
}

static schema_t schema_from_flex_dict(const flexible_type& f) {
  ASSERT_EQ(
    // have to cast because flex_type_enum is not printable
    static_cast<size_t>(f.get_type()),
    static_cast<size_t>(flex_type_enum::DICT)
  );
  flex_dict d = f.get<flex_dict>();
  schema_t ret;
  for (const auto& pair : d) {
    const flex_string& key = pair.first;
    const flexible_type& value = pair.second;
    ret.insert(std::make_pair(key, value));
  }
  return ret;
}

static schema_t schema_from_variant(const variant_type& v) {
  switch (v.which()) {
    case 0:
      {
        // flex_type (should be flex_dict)
        flexible_type f = variant_get_value<flexible_type>(v);
        return schema_from_flex_dict(f);
      }
    case 6:
      // dictionary
      return variant_get_value<schema_t>(v);
    default:
      log_and_throw("Type mismatch: expected dictionary representation of schema.");
  }
}

variant_type JSON::from_serializable(flexible_type data, variant_type schema) {
  schema_t schema_dict = schema_from_variant(schema);
  return _any_from_serializable(data, schema_dict);
}
