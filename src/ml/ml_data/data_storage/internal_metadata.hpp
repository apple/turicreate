/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_COLUMN_METADATA_H_
#define TURI_DML_DATA_COLUMN_METADATA_H_

#include <ml/ml_data/column_indexer.hpp>
#include <ml/ml_data/column_statistics.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>
#include <core/storage/sframe_data/sarray.hpp>

namespace turi {

struct metadata_load;
class ml_metadata;

namespace ml_data_internal {

/**  The metadata information for a single column.  This is meant to
 *   be used internally to ml_data; there is no reason that the
 *   structures outside of ml_data need to access this; ml_metadata
 *   should be used instead.
 *
 *   This structure is necessary as many of the internal processing
 *   routines use a vector of column metadata to handle all the
 *   processing.  Having this structure, which organizes all the parts
 *   of the column metadata into one place, greatly simplifies this
 *   processing.
 */
struct column_metadata {
  ////////////////////////////////////////////////////////////////////////////////
  // Public data members

  std::string name = "";
  ml_column_mode mode;
  flex_type_enum original_column_type;
  std::shared_ptr<ml_data_internal::column_indexer> indexer = nullptr;
  std::shared_ptr<ml_data_internal::column_statistics> statistics = nullptr;

  ////////////////////////////////////////////////////////////////////////////////
  // Construction

  /** Generates a new column_metadata class using the data arrays and
   *  the types.
   */
  void setup(bool is_target_column, const std::string& name,
             const std::shared_ptr<sarray<flexible_type> >& column,
             const std::map<std::string, ml_column_mode>& mode_overrides);

  /** Finalize training.
   */
  void set_training_index_size();
  void set_training_index_offset(size_t previous_total);

  ////////////////////////////////////////////////////////////////////////////////
  // Some data sizing stuff

  /** Returns true if the mode of this column has a fixed mode size
   *  and false otherwise.
   *
   */
  bool mode_has_fixed_size() const {
    bool has_fixed_size = (column_data_size_if_fixed != size_t(-1));

    DASSERT_TRUE(has_fixed_size == turi::mode_has_fixed_size(mode));

    return has_fixed_size;
  }

  /** Returns true if this column is untranslated and false otherwise.
   */
  bool is_untranslated_column() const {
    return (mode == ml_column_mode::UNTRANSLATED);
  }

  /** Returns the size of the index at training time.
   */
  size_t index_size() const {
    DASSERT_TRUE(index_size_at_train_time != size_t(-1));
    return index_size_at_train_time;
  }

  /** Returns the size of the index at training time.
   */
  size_t global_index_offset() const {
    // This should be set
    DASSERT_TRUE(index_size_at_train_time != size_t(-1));
    DASSERT_TRUE(global_index_offset_at_train_time != size_t(-1));
    return global_index_offset_at_train_time;
  }

    /** For debug testing.  Make sure things are equal.
     */
#ifndef NDEBUG
  void _debug_is_equal(const column_metadata& other) const;
#else
  void _debug_is_equal(const column_metadata& other) const {}
#endif

 private:
  friend struct turi::metadata_load;

  /** This is set to hold the size of the numeric column if it is
   *  fixed, and size_t(-1) otherwise.
   */
  size_t index_size_at_train_time = size_t(-1);
  size_t column_data_size_if_fixed = size_t(-1);

  /** To be used only if it's an ndarray column type.
   */
  flex_nd_vec::index_range_type nd_array_size;

  size_t global_index_offset_at_train_time = size_t(-1);

 public:
  const size_t fixed_column_size() const {
    DASSERT_TRUE(mode_has_fixed_size());
    return column_data_size_if_fixed;
  }

  /** During loading, we need to verify that the columns indeed have
   *  the correct column sizes.
   */
  inline void check_fixed_column_size(const flexible_type& v) const
      GL_HOT_INLINE_FLATTEN;

  /** Returns the current size of the column.
   *
   */
  inline size_t column_size() const {
    if (mode_is_indexed(mode)) {
      return indexer->indexed_column_size();
    } else {
      DASSERT_TRUE(mode_has_fixed_size());
      return column_data_size_if_fixed;
    }
  }

  /** Returns the current shape of the column as if it's an nd_vec
   *
   */
  inline const flex_nd_vec::index_range_type& nd_column_shape() const {
    DASSERT_TRUE(mode_has_fixed_size());
    return nd_array_size;
  }


  /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const;

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc);
};

typedef std::shared_ptr<column_metadata> column_metadata_ptr;

////////////////////////////////////////////////////////////////////////////////

/**  This structure holds the main data being passed around
 *   internally.  It contains all the information needed to quickly
 *   unpack a row from the internal data structure.
 */
struct row_metadata {
  row_metadata() {}

  /** Constructs all the information from a vector of points.
   */
  void setup(
      const std::vector<std::shared_ptr<column_metadata> >& _metadata_vect,
      bool _has_target);

  /** Constructs all the information from a vector of points.
   */
  void set_index_sizes(const std::shared_ptr<ml_metadata>& m);

  bool has_target = false;
  bool target_is_indexed = false;

  /** True if the data size is constant, and false otherwise.
   */
  bool data_size_is_constant = false;

  /** If the data size is constant, then this gives its
   * size. Otherwise, it's set to 0.
   */
  size_t constant_data_size = 0;

  /** To be used only if it's an ndarray column type.
   */
  flex_nd_vec::index_range_type nd_array_size;

  /** Number of columns, not including target.
   */
  size_t num_x_columns = 0;

  /** Total number of columns, including possible target.
   */
  size_t total_num_columns = 0;

  /** Pointers to the original metadata vectors.
   */
  std::vector<column_metadata_ptr> metadata_vect;

  /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const;

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc);

#ifndef NDEBUG
  void _debug_is_equal(const row_metadata& rm) const;
#else
  void _debug_is_equal(const row_metadata& rm) const {}
#endif
};

////////////////////////////////////////////////////////////////////////////////
// A few implementations of the above functions

inline void column_metadata::check_fixed_column_size(
    const flexible_type& f) const {

  auto throw_error_1d = [&](size_t nv) GL_GCC_ONLY(GL_COLD_NOINLINE) {
    log_and_throw(std::string("Dataset mismatch. Numeric feature '") + name +
                  "' must contain lists of consistent size. (Found "
                  "lists/arrays of sizes " +
                  std::to_string(nv) + " and " +
                  std::to_string(column_data_size_if_fixed) + ").");
  };

  auto throw_error_nd =
      [&](const flex_nd_vec::index_range_type& shape)
          GL_GCC_ONLY(GL_COLD_NOINLINE) {

            if (shape.size() == 1 && nd_array_size.size() <= 1) {
              throw_error_1d(shape[0]);
            } else {
              std::ostringstream error;

              error << "Dataset mismatch. Numeric feature '" << name
                    << "' must contain lists of consistent size. (Found "
                       "lists/arrays of sizes ";

              if (nd_array_size.empty()) {
                error << "(" << column_data_size_if_fixed << ",)";
              } else {
                error << "(";
                for (const auto& d : nd_array_size) {
                  error << d << ",";
                }
                error << ")";
              }

              error << " and (";
              for (const auto& d : shape) {
                error << d << ",";
              }
              error << ").";
            }
          };

  // The only mode needed to be checked right now is this one.
  if (mode == ml_column_mode::NUMERIC_VECTOR) {
    DASSERT_TRUE(column_data_size_if_fixed != size_t(-1));
    DASSERT_LE(nd_array_size.size(), 1);

    if (f.get_type() == flex_type_enum::VECTOR) {
      const flex_vec& v = f.get<flex_vec>();
      size_t nv = v.size();

      DASSERT_TRUE(column_data_size_if_fixed != size_t(-1));

      if (UNLIKELY(nv != column_data_size_if_fixed)) {
        throw_error_1d(nv);
      }
    } else if (f.get_type() == flex_type_enum::ND_VECTOR) {
      const flex_nd_vec& v = f.get<flex_nd_vec>();
      const auto& shape = v.shape();

      if(UNLIKELY(shape.size() != 1 || shape[0] != column_data_size_if_fixed)) {
        throw_error_nd(shape);
      }
    } else {
      ASSERT_TRUE(false);
    }

  } else if (mode == ml_column_mode::NUMERIC_ND_VECTOR) {
    DASSERT_TRUE(column_data_size_if_fixed != size_t(-1));
    DASSERT_FALSE(nd_array_size.empty());

    if (UNLIKELY(f.get_type() == flex_type_enum::VECTOR)) {
      const flex_vec& v = f.get<flex_vec>();
      if (nd_array_size.size() != 1) {
        throw_error_nd({v.size()});
      }
      size_t nv = v.size();

      DASSERT_TRUE(column_data_size_if_fixed != size_t(-1));

      if (UNLIKELY(nv != column_data_size_if_fixed)) {
        throw_error_1d(nv);
      }

    } else if (f.get_type() == flex_type_enum::ND_VECTOR) {
      const flex_nd_vec& v = f.get<flex_nd_vec>();
      const auto& shape = v.shape();

      if(UNLIKELY(shape.size() != nd_array_size.size())) {
        throw_error_nd(shape);
      }

      for(size_t i = 0; i < shape.size(); ++i) {
        if (UNLIKELY(shape[i] != nd_array_size[i])) {
          throw_error_nd(shape);
        }
      }
    } else {
      ASSERT_TRUE(false);
    }
  }
}

}}

////////////////////////////////////////////////////////////////////////////////
// Implement serialization for
// std::shared_ptr<column_metadata>

BEGIN_OUT_OF_PLACE_SAVE(
    arc, std::shared_ptr<turi::ml_data_internal::column_metadata>, m) {
  if (m == nullptr) {
    arc << false;
  } else {
    arc << true;
    arc << (*m);
  }
}
END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(
    arc, std::shared_ptr<turi::ml_data_internal::column_metadata>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if (is_not_nullptr) {
    m.reset(new turi::ml_data_internal::column_metadata);
    arc >> (*m);
  } else {
    m = std::shared_ptr<turi::ml_data_internal::column_metadata>(nullptr);
  }
}
END_OUT_OF_PLACE_LOAD()

#endif /* TURI_DML_DATA_COLUMN_METADATA_H_ */
