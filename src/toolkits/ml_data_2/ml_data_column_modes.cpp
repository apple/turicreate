/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/logging/logger.hpp>

namespace turi { namespace v2 {

/**  For error reporting, returns a name of the mode based on the
 *   column mode value.
 */
const char* column_mode_enum_to_name(ml_column_mode mode) {

  switch(mode) {
    case ml_column_mode::NUMERIC: return "numeric";
    case ml_column_mode::NUMERIC_VECTOR: return "numeric_vector";
    case ml_column_mode::CATEGORICAL: return "categorical";
    case ml_column_mode::CATEGORICAL_VECTOR: return "categorical_vector";
    case ml_column_mode::DICTIONARY: return "dictionary";
    case ml_column_mode::UNTRANSLATED: return "untranslated";
    default: ASSERT_TRUE(false); return "";
  }
}

namespace ml_data_internal {

/**
 * Checks to make sure that the column type provided actually matches
 * up with the mode used.  This is just done for error checking.
 * Throws an error message if they are not consistent.
 */
void check_type_consistent_with_mode(const std::string& column_name,
                                     flex_type_enum column_type, ml_column_mode mode) {

  auto raise_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
    log_and_throw(std::string("Column '") + column_name
                  + "' has type "
                  + flex_type_enum_to_name(column_type)
                  + "; cannot be treated as "
                  + column_mode_enum_to_name(mode) + ".");
  };

  switch(mode) {
    case ml_column_mode::NUMERIC:
      if(column_type != flex_type_enum::INTEGER
         && column_type != flex_type_enum::FLOAT)

        raise_error();
      return;

    case ml_column_mode::CATEGORICAL:
      if(column_type != flex_type_enum::INTEGER
         && column_type != flex_type_enum::STRING
         && column_type != flex_type_enum::UNDEFINED)
        raise_error();
      return;

    case ml_column_mode::NUMERIC_VECTOR:
      if(column_type != flex_type_enum::VECTOR)
        raise_error();
      return;

    case ml_column_mode::CATEGORICAL_VECTOR:
      if(column_type != flex_type_enum::LIST)
        raise_error();
      return;

    case ml_column_mode::DICTIONARY:
      if(column_type != flex_type_enum::DICT)
        raise_error();
      return;

    case ml_column_mode::UNTRANSLATED:
      return;
  }
}

/** This function handles the translation of column types to the
 *  column modes, which determines how they behave.
 *
 *  The options that affect this are given as follows:
 *
 *  integer_columns_categorical_by_default
 *      : If true, then integers are translated to categorical values.
 *
 */
ml_column_mode choose_column_mode(
    const std::string& column_name,
    flex_type_enum column_type,
    const std::map<std::string, flexible_type>& options,
    const std::map<std::string, ml_column_mode>& mode_overrides) {

  bool int_is_cat = options.at("integer_columns_categorical_by_default");

  ml_column_mode mode = ml_column_mode::NUMERIC;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  See if there are any mode overrides present.  If there
  // are, check the column mode compatibility.  Need to make sure
  // that none of the modes will cause problems.

  if(mode_overrides.count(column_name)) {

    mode = mode_overrides.at(column_name);

    bool type_mode_okay = false;

    auto check_type_in = [&](std::initializer_list<flex_type_enum> s) {
      for(flex_type_enum t : s)
        if(column_type == t)
          type_mode_okay = true;
    };

    switch(mode) {
      case ml_column_mode::NUMERIC:
        check_type_in({flex_type_enum::FLOAT, flex_type_enum::INTEGER});
        break;
      case ml_column_mode::NUMERIC_VECTOR:
        check_type_in({flex_type_enum::VECTOR});
        break;
      case ml_column_mode::CATEGORICAL:
        check_type_in({flex_type_enum::FLOAT, flex_type_enum::INTEGER,
                flex_type_enum::STRING, flex_type_enum::UNDEFINED});
        break;
      case ml_column_mode::CATEGORICAL_VECTOR:
        check_type_in({flex_type_enum::LIST});
        break;
      case ml_column_mode::DICTIONARY:
        check_type_in({flex_type_enum::DICT});
        break;
      case ml_column_mode::UNTRANSLATED:
        type_mode_okay = true;
        break;
      default:
        break;
    }

    if(!type_mode_okay) {
      std::ostringstream ss;

      ss << "In column " << column_name << ", "
         << "column type " << flex_type_enum_to_name(column_type)
         << " is not compatible with requested mode "
         << column_mode_enum_to_name(mode)
         << ".";

      log_and_throw(ss.str().c_str());
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.  If no overrides are given, then choose the mode based
    // on the type and the options.

  } else {

    switch(column_type) {
      case flex_type_enum::FLOAT:
        mode = ml_column_mode::NUMERIC;
        break;

      case flex_type_enum::INTEGER:
        mode = int_is_cat ? ml_column_mode::CATEGORICAL : ml_column_mode::NUMERIC;
        break;

      case flex_type_enum::STRING:
        mode = ml_column_mode::CATEGORICAL;
        break;

      case flex_type_enum::VECTOR:
        mode = ml_column_mode::NUMERIC_VECTOR;
        break;

      case flex_type_enum::LIST:
        mode = ml_column_mode::CATEGORICAL_VECTOR;
        break;

      case flex_type_enum::DICT:
        mode = ml_column_mode::DICTIONARY;
        break;

      case flex_type_enum::UNDEFINED:
        mode = ml_column_mode::CATEGORICAL;
        break;

      default:
        log_and_throw(std::string("Type of column '")
                      + column_name + "' not yet supported by given model.");
        break;
    }

    if(column_type == flex_type_enum::UNDEFINED) {
      logstream(LOG_WARNING)
          << "Type of column " << column_name << " is undefined; "
          << "treated as categorical.  This may not yield the desired behavior."
          << std::endl;
    }

  }

  return mode;
}

}}}
