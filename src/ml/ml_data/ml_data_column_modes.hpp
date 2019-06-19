/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_COLUMN_MODES_H_
#define TURI_DML_DATA_COLUMN_MODES_H_

#include <string>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>

#ifdef ERROR
#undef ERROR
#endif

namespace turi {

class sframe;
/**
 * \ingroup mldata
 * \{
 */
/**
 * The missing value action.
 */
enum class ml_missing_value_action : int {IMPUTE, ERROR, USE_NAN};

/** The main mode of each entry value; determines how it is stored and
 *  how it is translated and what functionality it works with.
 */
enum class ml_column_mode : int {
    NUMERIC = 0,
    CATEGORICAL = 1,
    NUMERIC_VECTOR = 2,
    CATEGORICAL_VECTOR = 3,
    DICTIONARY = 4,
    UNTRANSLATED = 5,
    CATEGORICAL_SORTED = 6,
    NUMERIC_ND_VECTOR = 7};


/** Returns true if the underlying type is treated as a categorical
 *  variable, and false otherwise.
 */
GL_HOT_INLINE_FLATTEN
static inline bool mode_is_categorical(ml_column_mode mode) {

  switch(mode) {
    case ml_column_mode::NUMERIC:            return false;
    case ml_column_mode::CATEGORICAL:        return true;
    case ml_column_mode::NUMERIC_VECTOR:     return false;
    case ml_column_mode::CATEGORICAL_VECTOR: return true;
    case ml_column_mode::DICTIONARY:         return false;
    case ml_column_mode::UNTRANSLATED:       return false;
    case ml_column_mode::CATEGORICAL_SORTED: return true;
    case ml_column_mode::NUMERIC_ND_VECTOR:  return false;
  }
  return false;
}

/** Returns true if the underlying type always results in constant
 *  size pattern, and false otherwise.
 */
GL_HOT_INLINE_FLATTEN
static inline bool mode_has_fixed_size(ml_column_mode mode) {

  switch(mode) {
    case ml_column_mode::NUMERIC:            return true;
    case ml_column_mode::CATEGORICAL:        return true;
    case ml_column_mode::NUMERIC_VECTOR:     return true;
    case ml_column_mode::CATEGORICAL_VECTOR: return false;
    case ml_column_mode::DICTIONARY:         return false;
    case ml_column_mode::UNTRANSLATED:       return true;
    case ml_column_mode::CATEGORICAL_SORTED: return true;
    case ml_column_mode::NUMERIC_ND_VECTOR:  return true;
    default: ASSERT_TRUE(false);             return false;
  }
}

/** Returns true if the underlying type is indexed, and false
 *  otherwise.  This differs form the is_categorical in that
 *  dictionaries are not treated as pure categorical variables, as
 *  they have values associated with them, but they are indexed.
 */
GL_HOT_INLINE_FLATTEN
static inline bool mode_is_indexed(ml_column_mode mode) {

  switch(mode) {
    case ml_column_mode::NUMERIC:            return false;
    case ml_column_mode::CATEGORICAL:        return true;
    case ml_column_mode::NUMERIC_VECTOR:     return false;
    case ml_column_mode::CATEGORICAL_VECTOR: return true;
    case ml_column_mode::DICTIONARY:         return true;
    case ml_column_mode::UNTRANSLATED:       return false;
    case ml_column_mode::CATEGORICAL_SORTED: return true;
    case ml_column_mode::NUMERIC_ND_VECTOR:  return false;
  }
  return false;
}


/**  For error reporting, returns a name of the mode based on the
 *   column mode value.
 */
const char* column_mode_enum_to_name(ml_column_mode mode);


namespace ml_data_internal {

/**
 * Checks to make sure that the column type provided actually matches
 * up with the mode used.  This is just done for error checking.
 * Throws an error message if they are not consistent.
 */
void check_type_consistent_with_mode(const std::string& column_name,
                                     flex_type_enum column_type, ml_column_mode mode);


/** This function handles the translation of column types to the
 *  column modes, which determines how they behave.
 *
 *  The options that affect this are given as follows:
 *
 *  integers_are_categorical : If true, then integers are translated
 *  to categorical values.
 *
 */
ml_column_mode choose_column_mode(
    const std::string& column_name,
    flex_type_enum column_type,
    const std::map<std::string, ml_column_mode>& mode_overrides);


/// \}
}}



#endif /* TURI_DML_DATA_COLUMN_MODES_H_ */
