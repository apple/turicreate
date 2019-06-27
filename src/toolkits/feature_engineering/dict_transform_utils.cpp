/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/feature_engineering/dict_transform_utils.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sarray.hpp>

namespace turi {

/** Index keys come up in a lot of places, e.g. vectors.  This deals
 *  with that.  (Also good to have it as it's own function in case
 *  we want to update it later on).
 */
static inline flexible_type get_index_key(const flex_int& key) {
  return flexible_type(key).to<flex_string>();
};

/** The recursion function to actually add everything to the rest of
 *  new dictionary.
 *
 *  out -- the final dictionary.
 *  key -- the current key.  May be modified.
 *  v -- the value to add to the dictionary.
 *  separator -- the separator string.
 *  undefined_string -- what to call undefined values.
 *  process_image_value -- how to process images.
 *  process_datetime_value -- how to process datetime values.
 */
GL_HOT_FLATTEN
static void _to_flat_dict_recursion(
    flex_dict& out,
    flex_string& key,
    const flexible_type& v,
    const flex_string& separator,
    const flex_string& undefined_string,
    const std::function<flexible_type(const flex_image&)>& process_image_value,
    const std::function<flexible_type(const flex_date_time&)>& process_datetime_value,
    size_t depth) {

  // You probably hit errors before this in other areas of the code,
  // e.g. flexible type destructors.

  ////////////////////////////////////////////////////////////////////////////////

  /** Checks to error out if we go too deep.
   */
  static constexpr size_t MAX_RECURSION_DEPTH = 64;

  // Where we recurse on the value, along with error checking as well.
  auto recurse_on_value = [&](const flexible_type& recurse_v) GL_GCC_ONLY(GL_HOT_INLINE) {

    auto depth_overflow_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {

      // Check to make sure we haven't reached our recursion depth limit.
      std::ostringstream ss;

      ss << "Maximum nested depth in dictionary of "
         << MAX_RECURSION_DEPTH
         << " exceeded in flattening dictionary/list. "
         << "This is not the type of deep learning we are used to."
         << std::endl;

      throw std::runtime_error(ss.str());
    };

    if(depth > MAX_RECURSION_DEPTH) {
      depth_overflow_error();
    }

    _to_flat_dict_recursion(out, key, recurse_v,
                            separator, undefined_string,
                            process_image_value, process_datetime_value, depth + 1);
  };

  ////////////////////////////////////////////////////////////////////////////////

  /** Add a new sub-key to the current key string.  Put this in a
   *  central place so that all the different paths have a consistent
   *  way of handling the key.
   */
  auto add_to_key_string = [&](const flexible_type& key_v) {

    // Add in the separator if needed.
    if(key.size() != 0) {
      key.append(separator);
    }

    // Write out the key
    switch(v.get_type()) {
      case flex_type_enum::STRING: {
        key.append(key_v.get<flex_string>());
        break;
      }
      case flex_type_enum::UNDEFINED: {
        key.append(undefined_string);
        break;
      }
      case flex_type_enum::INTEGER: {
        flex_string s = get_index_key(key_v.get<flex_int>());
        key.append(s);
        break;
      }
      default: {
        flex_string s = key_v.to<flex_string>();
        key.append(s);
        break;
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Now, handle the value.
  size_t base_key_size = key.size();

  switch(v.get_type()) {
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT: {
      out.push_back({key, v});
      break;
    }
    case flex_type_enum::DICT: {
      const flex_dict& d = v.get<flex_dict>();

      for(const auto& p : d) {
        add_to_key_string(p.first);
        recurse_on_value(p.second);
        key.resize(base_key_size);
      }
      break;
    }

    case flex_type_enum::UNDEFINED:
    case flex_type_enum::STRING: {
      add_to_key_string(v);
      out.push_back({key, 1});
      key.resize(base_key_size);
      break;
    }

    case flex_type_enum::LIST: {
      const flex_list& fl = v.get<flex_list>();

      for(size_t i = 0; i < fl.size(); ++i) {
        add_to_key_string(i);
        recurse_on_value(fl[i]);
        key.resize(base_key_size);
      }

      break;
    }

    case flex_type_enum::VECTOR: {
      const flex_vec& fv = v.get<flex_vec>();

      for(size_t i = 0; i < fv.size(); ++i) {
        add_to_key_string(i);
        out.push_back({key, fv[i]});
        key.resize(base_key_size);
      }

      break;
    }

    case flex_type_enum::IMAGE: {
      flexible_type ft_out = process_image_value(v.get<flex_image>());

      ASSERT_MSG(ft_out.get_type() != flex_type_enum::IMAGE,
                 "Handling function for image types returned an image type.");

      if(ft_out.get_type() != flex_type_enum::UNDEFINED) {
        recurse_on_value(ft_out);
      }
      break;
    }
    case flex_type_enum::DATETIME: {

      flexible_type ft_out = process_datetime_value(v.get<flex_date_time>());

      ASSERT_MSG(ft_out.get_type() != flex_type_enum::DATETIME,
                 "Handling function for datetime types returned an datetime type.");

      if(ft_out.get_type() != flex_type_enum::UNDEFINED) {
        recurse_on_value(ft_out);
      }
      break;
    }

    case flex_type_enum::ND_VECTOR: {
      log_and_throw(std::string("Flexible type case currently unsupported: ND_VECTOR"));
      ASSERT_UNREACHABLE();
    }

    default: {
      log_and_throw(std::string("Flexible type case not recognized"));
      ASSERT_UNREACHABLE();
    }
  }
}

/**  Flattens any types to a non-nested dictionary of (string key :
 *   numeric value) pairs. Each nested key is a concatenation of the
 *   keys in the separation with sep_char separating them.  For
 *   example, if sep_char = ".", then
 *
 *     {"a" : {"b" : 1}, "c" : 2}
 *
 *   becomes
 *
 *     {"a.b" : 1, "c" : 2}.
 *
 *   - List and vector elements are handled by converting the index of
 *     the appropriate element to a string.
 *
 *   - String values are handled by treating them as a single
 *     {"string_value" : 1} pair.
 *
 *   - numeric values in the original are translated into a {"0" :
 *     value} dict.
 *
 *   - FLEX_UNDEFINED values are handled by replacing them with the
 *     string contents of `undefined_string`.
 *
 *   - image and datetime types are handled by calling
 *     process_image_value and process_datetime_value.  These
 *     functions must either throw an exception, which propegates up,
 *     return any other flexible type (e.g. dict, list, vector, etc.),
 *     or return FLEX_UNDEFINED, in which case that value is ignored.
 *
 */
GL_HOT flex_dict to_flat_dict(
    const flexible_type& input,
    const flex_string& separator,
    const flex_string& undefined_string,
    std::function<flexible_type(const flex_image&)> process_image_value,
    std::function<flexible_type(const flex_date_time&)> process_datetime_value) {

  ////////////////////////////////////////////////////////////////////////////////
  // Some utility functions used everywhere -- done here to keep
  // things organized and make sure errors are processed correctly.

  ////////////////////////////////////////////////////////////////////////////////

  /** Based on the input type, we do some processing.
   */
  switch(input.get_type()) {
    case flex_type_enum::DICT: {
      bool need_to_flatten = false;
      for(const auto& p : input.get<flex_dict>()) {
        if(p.first.get_type() != flex_type_enum::STRING) {
          need_to_flatten = true;
        }

        if(p.second.get_type() != flex_type_enum::FLOAT
           && p.second.get_type() != flex_type_enum::INTEGER) {
          need_to_flatten = true;
        }

        if(need_to_flatten) {
          break;
        }
      }

      // If we don't need to flatten it, then don't.
      if(!need_to_flatten) {
        return input.get<flex_dict>();
      } else {
        break;
      }
    }

    case flex_type_enum::LIST: {
      // This will be flattened later on, as it could be arbitrarily
      // recursive.
      break;
    }

    case flex_type_enum::VECTOR: {
      // Vectors are changed to a dictionary of {"0" : v[0], "1" : v[1], ...}.
      const flex_vec& v = input.get<flex_vec>();
      flex_dict _out(v.size());
      for(size_t i = 0; i < v.size(); ++i) {
        _out[i] = {get_index_key(i), v[i]};
      }
      return _out;
    }

    case flex_type_enum::STRING: {
      return flex_dict{ {input, 1} };
    }

    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT: {
      return flex_dict{ {get_index_key(0), input} };
    }
    case flex_type_enum::IMAGE: {
      // This can recurse a maximum of once, as the return value of
      // process_image_value cannot be an image or datetime type,
      // and this is the only place we recurse.
      return to_flat_dict(process_image_value(input),
                          separator, undefined_string,
                          process_image_value, process_datetime_value);
    }
    case flex_type_enum::DATETIME: {
      // This can recurse a maximum of once, as the return value of
      // process_datetime_value cannot be an image or datetime type,
      // and this is the only place we recurse.
      return to_flat_dict(process_datetime_value(input),
                          separator, undefined_string,
                          process_image_value, process_datetime_value);
    }
    case flex_type_enum::UNDEFINED: {
      return flex_dict{ {undefined_string, 1} };
    }
    case flex_type_enum::ND_VECTOR: {
      log_and_throw(std::string("Flexible type case currently unsupported: ND_VECTOR"));
      ASSERT_UNREACHABLE();
    }
    default: {
      log_and_throw(std::string("Flexible type case not recognized"));
      ASSERT_UNREACHABLE();
    }
  }

  ////////////////////////////////////////////////////////////////////////////////

  // Reserve the output container.
  flex_dict out;
  out.reserve(input.size());

  // The current key.
  flex_string key = "";
  key.reserve(256);

  _to_flat_dict_recursion(out, key, input,
                          separator, undefined_string,
                          process_image_value, process_datetime_value, 0);

  return out;
}

////////////////////////////////////////////////////////////////////////////////

static std::function<flexible_type(const flex_image&)> _get_image_handler(
    const std::string& image_policy) {

  if(image_policy == "error") {
    return [](const flex_image&) -> flexible_type {
      log_and_throw("Image types are not allowed when flattening dictionaries.");
    };
  } else if (image_policy == "ignore") {
    return [](const flex_image&) -> flexible_type {
      return FLEX_UNDEFINED;
    };
  } else {
    log_and_throw("At this time, only \"error\" and \"ignore\" are "
                  "implemented for handling of image types.");

    return [](const flex_image&) -> flexible_type { return FLEX_UNDEFINED; };
  }
}

static std::function<flexible_type(const flex_date_time&)> _get_datetime_handler(
    const std::string& datetime_policy) {

  if(datetime_policy == "error") {
    return [](const flex_date_time&) -> flexible_type {
      log_and_throw("Datetime types are not allowed when flattening dictionaries.");
    };
  } else if (datetime_policy == "ignore") {
    return [](const flex_date_time&) -> flexible_type {
      return FLEX_UNDEFINED;
    };
  } else {
    log_and_throw("At this time, only \"error\" and \"ignore\" are "
                  "implemented for handling of datetime types.");

    return [](const flex_date_time&) -> flexible_type { return FLEX_UNDEFINED; };
  }
}

/**  Flattens any types to a non-nested dictionary of (string key :
 *   numeric value) pairs. Each nested key is a concatenation of the
 *   keys in the separation with sep_char separating them.  For
 *   example, if sep_char = ".", then
 *
 *     {"a" : {"b" : 1}, "c" : 2}
 *
 *   becomes
 *
 *     {"a.b" : 1, "c" : 2}.
 *
 *   - List and vector elements are handled by converting the index of
 *     the appropriate element to a string.
 *
 *   - String values are handled by treating them as a single
 *     {"string_value" : 1} pair.
 *
 *   - numeric values in the original are translated into a {"0" :
 *     value} dict.
 *
 *   - FLEX_UNDEFINED values are handled by replacing them with the
 *     string contents of `undefined_string`.
 *
 *   - image and datetime types are handled by calling
 *     process_image_value and process_datetime_value.  These
 *     functions must either throw an exception, which propegates up,
 *     return any other flexible type (e.g. dict, list, vector, etc.),
 *     or return FLEX_UNDEFINED, in which case that value is ignored.
 *
 */
EXPORT flex_dict to_flat_dict(const flexible_type& input,
                              const flex_string& separator,
                              const flex_string& undefined_string,
                              const std::string& image_policy,
                              const std::string& datetime_policy) {

  return to_flat_dict(input, separator, undefined_string,
                      _get_image_handler(image_policy),
                      _get_datetime_handler(datetime_policy));
}


/** Performs dictionary flattening on an SArray.
 */
EXPORT gl_sarray to_sarray_of_flat_dictionaries(gl_sarray input,
                                                const flex_string& sep,
                                                const flex_string& undefined_string,
                                                const std::string& image_policy,
                                                const std::string& datetime_policy) {

  auto image_handler = _get_image_handler(image_policy);
  auto datetime_handler = _get_datetime_handler(datetime_policy);

  std::function<flexible_type(const flexible_type& dt)> flatten_it
      = [=](const flexible_type& x) -> flexible_type GL_GCC_ONLY(GL_HOT_FLATTEN) {

    return to_flat_dict(x, sep, undefined_string, image_handler, datetime_handler);
  };

  return input.apply(flatten_it, flex_type_enum::DICT);
}

}
