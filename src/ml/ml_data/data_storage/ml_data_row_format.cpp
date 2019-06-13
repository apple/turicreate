/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdint>
#include <core/globals/globals.hpp>
#include <ml/ml_data/data_storage/ml_data_row_format.hpp>
#include <ml/ml_data/metadata.hpp>
#include <ml/ml_data/ml_data.hpp>
#include <core/util/basic_types.hpp>

////////////////////////////////////////////////////////////////////////////////

namespace turi {
namespace ml_data_internal {

static const size_t ROW_READ_CHECKSUM = 0x259e2e6d7a32c5c0ULL;
static constexpr size_t N_INTEGERS_PER_REFERENCE_BLOCK = 128;

void row_data_block::load(turi::iarchive& iarc) {
  size_t version;
  iarc >> version;

  ASSERT_EQ(version, 1);

  bool all_integers;
  iarc >> all_integers;

  size_t entry_data_size = 0;
  iarc >> entry_data_size;
  entry_data.resize(entry_data_size);

  if (all_integers) {
    // All integers.  Just do it directly,
    for (size_t i = 0; i < entry_data_size;
         i += N_INTEGERS_PER_REFERENCE_BLOCK) {
      size_t limit =
          std::min<size_t>(entry_data_size - i, N_INTEGERS_PER_REFERENCE_BLOCK);
      integer_pack::frame_of_reference_decode_128(
          iarc, limit, reinterpret_cast<uint64_t*>(entry_data.data() + i));
    }
  } else {
    // A mix.  The most complicated one.
    size_t n_integers = 0;
    iarc >> n_integers;

    size_t n_doubles = 0;
    iarc >> n_doubles;

    size_t n_doubles_as_ints;
    iarc >> n_doubles_as_ints;

    DASSERT_EQ(n_doubles + n_integers + n_doubles_as_ints, entry_data_size);

    // First, unpack the bitset.  This specifies whether things
    // are doubles or integers.

    dense_bitset bs, bs_dbl;
    iarc >> bs >> bs_dbl;

    std::vector<uint64_t> other_doubles;

    typedef const uint64_t* __restrict__ read_ptr_type;
    typedef uint64_t* __restrict__ write_ptr_type;

    read_ptr_type double_read_ptr;
    read_ptr_type double_as_int_read_ptr;

    {
      double* double_write_ptr;
      uint64_t* double_as_int_write_ptr;

      if (n_doubles < n_doubles_as_ints) {
        other_doubles.resize(n_doubles);
        double_write_ptr = reinterpret_cast<double*>(other_doubles.data());
        double_as_int_write_ptr = reinterpret_cast<uint64_t*>(
            entry_data.data() + (entry_data_size - n_doubles_as_ints));
      } else {
        other_doubles.resize(n_doubles_as_ints);
        double_as_int_write_ptr =
            reinterpret_cast<uint64_t*>(other_doubles.data());
        double_write_ptr = reinterpret_cast<double*>(
            entry_data.data() + (entry_data_size - n_doubles));
      }

      double_read_ptr = reinterpret_cast<read_ptr_type>(double_write_ptr);
      double_as_int_read_ptr =
          reinterpret_cast<read_ptr_type>(double_as_int_write_ptr);

      // Unpack the doubles
      turi::deserialize(iarc, double_write_ptr, n_doubles * sizeof(double));

      // Unpack the doubles as integers.
      for (size_t i = 0; i < n_doubles_as_ints;
           i += N_INTEGERS_PER_REFERENCE_BLOCK) {
        size_t read_count = std::min<size_t>(n_doubles_as_ints - i,
                                             N_INTEGERS_PER_REFERENCE_BLOCK);
        integer_pack::frame_of_reference_decode_128(
            iarc, read_count, double_as_int_write_ptr + i);
      }
    }

    // Now, go through and pull all the integers out.
    std::vector<uint64_t> integer_buffer;
    integer_buffer.resize(n_integers);

    // All integers.  Just do it directly,
    for (size_t i = 0; i < n_integers; i += N_INTEGERS_PER_REFERENCE_BLOCK) {
      size_t limit =
          std::min<size_t>(n_integers - i, N_INTEGERS_PER_REFERENCE_BLOCK);
      integer_pack::frame_of_reference_decode_128(iarc, limit,
                                                  integer_buffer.data() + i);
    }

    read_ptr_type integer_read_ptr =
        reinterpret_cast<read_ptr_type>(integer_buffer.data());
    write_ptr_type write_ptr =
        reinterpret_cast<write_ptr_type>(entry_data.data());

    for (size_t i = 0, dbl_i = 0; i < entry_data_size; ++i, ++write_ptr) {
      if (bs.get(i)) {
        *write_ptr = *(integer_read_ptr++);
      } else {
        if (bs_dbl.get(dbl_i++)) {
          // It's a double masquerading as an int!
          double v = double(*(double_as_int_read_ptr++));
          memcpy(write_ptr, &v, sizeof(double));
        } else {
          *write_ptr = *(double_read_ptr++);
        }
      }
    }
  }

  iarc >> additional_data;

  uint64_t read_check = 0;
  iarc >> read_check;
  ASSERT_EQ(read_check, ROW_READ_CHECKSUM);
}

void row_data_block::save(turi::oarchive& oarc) const {
  // Save the version.
  size_t version = 1;
  oarc << version;

  ////////////////////////////////////////////////////////////////////////////////

  // Seperate all the integers and all the doubles.
  std::vector<uint64_t> integers;
  std::vector<double> doubles;

  integers.reserve(entry_data.size());
  doubles.reserve(entry_data.size());

  dense_bitset bs(entry_data.size());
  bs.clear();

  for (size_t i = 0; i < entry_data.size(); ++i) {
    if (entry_data[i].index_value <= std::numeric_limits<uint32_t>::max()) {
      integers.push_back(entry_data[i].index_value);
      bs.set_bit_unsync(i);
    } else {
      doubles.push_back(entry_data[i].double_value);
    }
  }

  // Now, split off the doubles that can be encoded as integers.
  std::vector<uint64_t> doubles_as_ints;
  doubles_as_ints.reserve(doubles.size());
  dense_bitset bs_dbl(doubles.size());
  bs_dbl.clear();

  size_t dbl_write_pos = 0;
  for (size_t i = 0; i < doubles.size(); ++i) {
    if (double(uint64_t(doubles[i])) == doubles[i]) {
      doubles_as_ints.push_back(doubles[i]);
      bs_dbl.set_bit_unsync(i);
    } else {
      doubles[dbl_write_pos++] = doubles[i];
    }
  }
  doubles.resize(dbl_write_pos);

  // Now, based on the distribution of the integers, choose one
  // mode.  Almost all doubles? just do all doubles.

  bool all_integers = (doubles.size() == 0 && doubles_as_ints.size() == 0);

  oarc << all_integers;

  ////////////////////////////////////////////////////////////////////////////////
  // Now, we need to actually go through and save all the data in one of several
  // ways.

  size_t entry_data_size = entry_data.size();
  oarc << entry_data_size;

  if (all_integers) {
    // All integers.  Just do it directly,
    for (size_t i = 0; i < entry_data_size;
         i += N_INTEGERS_PER_REFERENCE_BLOCK) {
      size_t limit =
          std::min<size_t>(entry_data_size - i, N_INTEGERS_PER_REFERENCE_BLOCK);
      integer_pack::frame_of_reference_encode_128(
          reinterpret_cast<const uint64_t*>(entry_data.data() + i), limit,
          oarc);
    }
  } else {
    // Store all the quantities first in case we want to specialize
    // the decoding based on what is present.
    size_t n_integers = integers.size();
    oarc << n_integers;

    size_t n_doubles = doubles.size();
    oarc << n_doubles;

    size_t n_doubles_as_ints = doubles_as_ints.size();
    oarc << n_doubles_as_ints;

    // Store the bitmasks.
    oarc << bs << bs_dbl;

    // Store the doubles.
    turi::serialize(oarc, doubles.data(), n_doubles * sizeof(double));

    // Store the doubles_as_ints.
    for (size_t i = 0; i < n_doubles_as_ints;
         i += N_INTEGERS_PER_REFERENCE_BLOCK) {
      size_t limit = std::min<size_t>(n_doubles_as_ints - i,
                                      N_INTEGERS_PER_REFERENCE_BLOCK);
      integer_pack::frame_of_reference_encode_128(doubles_as_ints.data() + i,
                                                  limit, oarc);
    }

    // Store the integers.
    for (size_t i = 0; i < n_integers; i += N_INTEGERS_PER_REFERENCE_BLOCK) {
      size_t limit =
          std::min<size_t>(n_integers - i, N_INTEGERS_PER_REFERENCE_BLOCK);
      integer_pack::frame_of_reference_encode_128(integers.data() + i, limit,
                                                  oarc);
    }
  }

  oarc << additional_data;

  oarc << size_t(ROW_READ_CHECKSUM);
}

size_t ML_DATA_TARGET_ROW_BYTE_MINIMUM = 256 * 1024;

REGISTER_GLOBAL(int64_t, ML_DATA_TARGET_ROW_BYTE_MINIMUM, true);

/** Translates the raw flexible_type data in column_buffer into a
 *  block of rows, indexing it through the metadata classes.  The
 *  output format is described in ml_data.hpp.
 *
 *  If a target column is present, it is assumed to be the first
 *  column in metadata.
 *
 *  \param[out] row2data_idx_map A vector corresponding to the number
 *  of rows.  Will be giving the starting indices in block_output.
 *
 *  \param[out] block_output A destination vector of raw row storage.
 *
 *  \param[in] metadata A vector of column_metadata objects giving the
 *  metadata for each column.
 *
 *  \param[in] column_buffer A vector of vectors, with each inner
 *  vector holding one unindexed column from the raw source and
 *  corresponding to one of the column_metadata objects.  All the
 *  columns must be the same length.
 *
 *  \param[in] row_size_constant If false then write the row size at
 *  the beginning of each row.  If the data consists of all values
 *  with constant width, then this should be true.
 *
 *  \param[in] track_statistics If true then the statistics in
 *  metadata is set with the current values.
 *
 *  \param[in] immutable_metadata If false, then new categorical values
 *  are allowed and inserted into the metadata tracking.  If true,
 *  then new categories are mapped to size_t(-1) and a warning is
 *  printed at the end giving the number of previously unseen
 *  categories.
 *
 *  \param[in] index_remapping If given, map all indices through this
 *  vector.  This allows row blocks to be sorted or shuffled by the
 *  calling function.
 *
 *  \return A vector of length corresponding to the number of rows
 *  giving the starting indices in block_output
 *
 * ROW FORMAT (from ml_data.hpp)
 * ================================================================================
 *
 * ml_data loads data from an existing sframe, indexes it by mapping all
 * categorical values to unique indices in 0, 1,2,...,n, and records
 * statistics about the values.  It then puts it into an efficient
 * row-based data storage structure for use in regression and learning
 * algorithms. The row based storage structure is designed for fast
 * iteration through the rows and target.
 *
 *
 * The rows are stored in segments of a vector of entry_value structs,
 * where entry_value encloses a union of an index_value and a
 * double_value.  Thus it is 8 bytes.  Each vector contains up to
 * ROW_BLOCK_SIZE rows.  The data is stored as an sarray<vector<entry_value> >
 *
 * Raw data layout
 * ---------------
 *
 * Each vector is simply laid out as
 *
 * | <row_size> row 1 | <row_size> row 2 | ... | <row_size> row n |
 *
 * The row size variable is present if CATEGORICAL_VECTOR or
 * DICTIONARY are defined types, as these may induce variable sized
 * rows.  Otherwise, the row sizes are constant and determined from
 * the metadata, thus this does not need to be there.
 *
 * where n <= ROW_BLOCK_SIZE.
 *
 * Each row is laid out according to the "mode" of the columns,
 * determined by column_metadata::mode.  mode can be NUMERIC,
 * CATEGORICAL, NUMERIC_VECTOR, CATEGORICAL_VECTOR, and DICTIONARY.  The
 * last three of these can hold multiple values.
 *
 * Row layout
 * ----------
 *
 * Each row takes a block of data entries laid out according to mode:
 *
 * NUMERIC:
 *
 *   ... | value | ...
 *
 * CATEGORICAL:
 *
 *   ... | index | ...
 *
 * NUMERIC_VECTOR:
 *
 *   ... | value1 value2 ... | ...
 *
 * CATEGORICAL_VECTOR:
 *
 *   ... | length index1 index2 ... | ...
 *
 * DICTIONARY:
 *
 *   ... | length index1 value1 index2 value2 ... | ...
 *
 *   example: {0 : 2.1, 1 : 8.5} would be ... | 2 0 2.1 1 8.5 | ...
 *
 * NUMERIC_ND_VECTOR:
 *
 *   ... | flattend_value1 flattened_value2 ... | ...
 *
 *
 * Since the number of columns, and the mode of each column is constant
 * across rows, the values are all laid out sequentially; for example, a
 * row with columns (numeric, numeric_vector, dictionary) would be
 *
 * ... || row_size | value | v1 v2 ... | length k1 v1 k2 v2 ... || ...
 *
 * Thus a row of [1.0, [2.0, 4.0], {0 : 2.1, 1 : 8.5}] would appear as
 *
 * ... || 8 | 1.0 | 2.0 4.0 | 2 0 2.1 1 8.5 || ...
 * = ... || 8 1.0 2.0 4.0 2 0 2.1 1 8.5 || ...
 *
 * Similarly, a row with columns (numeric, numeric_vector, categorical)
 * would be
 *
 * ... || value | v1 v2 ... | k1 || ...
 * * And a row of [1.0, [2.0, 4.0], 8] would appear as
 *
 * ... || 1.0 | 2 2.0 4.0 | 8 || ...
 * = ... || 1.0 2 2.0 4.0 8 || ...
 * Since ROW_BLOCK_SIZE are stored together in a vector<entry_value>,
 * iterating through rows is extremely fast.
 */
size_t fill_row_buffer_from_column_buffer(
    std::vector<size_t>& row2data_idx_map, row_data_block& block_output,
    const row_metadata& rm,
    const std::vector<std::vector<flexible_type> >& column_buffers,
    size_t thread_idx, bool track_statistics, bool immutable_metadata,
    ml_missing_value_action none_action) {
  if (track_statistics) {
    DASSERT_MSG(!immutable_metadata,
                "Dynamic metadata must be allowed if statistics are tracked.");
  }

#ifndef NDEBUG
  DASSERT_EQ(rm.total_num_columns, column_buffers.size());
  {
    size_t check_column_buffer_size = 0;

    for (size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {
      if (!rm.metadata_vect[c_idx]->is_untranslated_column()) {
        check_column_buffer_size = column_buffers[c_idx].size();
        break;
      }
    }

    for (size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {
      const auto& c = column_buffers[c_idx];
      if (rm.metadata_vect[c_idx]->is_untranslated_column())
        DASSERT_EQ(c.size(), 0);
      else
        DASSERT_EQ(c.size(), check_column_buffer_size);
    }
  }
#endif

  // How many rows in the block?
  size_t block_size = size_t(-1);
  for (size_t c_idx = 0; c_idx < column_buffers.size(); ++c_idx) {
    if (!rm.metadata_vect[c_idx]->is_untranslated_column()) {
      block_size = column_buffers[c_idx].size();
      break;
    }
  }

  /** If they are all untranslated columns, then we're done here... */
  if (block_size == size_t(-1)) return 0;

  DASSERT_TRUE(block_size != 0);

  // Set up the row2data_idx_map
  row2data_idx_map.resize(block_size);

  // Column_buffers for doing the metadata stats indexing stuff.
  std::vector<size_t> index_vect;
  std::vector<double> value_vect;
  std::vector<std::pair<size_t, double> > idx_value_vect;
  std::vector<size_t> exclusion_indices;

  size_t max_row_size = 0;

  block_output.entry_data.clear();

  if (rm.data_size_is_constant)
    block_output.entry_data.reserve(rm.constant_data_size * block_size);

  for (size_t row_buffer_index = 0; row_buffer_index < block_size;
       ++row_buffer_index) {
    ////////////////////////////////////////////////////////////////////////////////
    // Get ready to fill this by defining the row size

    size_t row_size = 0;

    // If the rows are not a constant size, then this writes out the
    // proper row size as the first element.
    size_t n_row_elements_write_index = size_t(-1);

    // record the index of the start of this row.
    row2data_idx_map[row_buffer_index] = block_output.entry_data.size();

    if (!rm.data_size_is_constant) {
      n_row_elements_write_index = block_output.entry_data.size();
      entry_value ev;
      block_output.entry_data.push_back(ev);
    }

    // Okay, now go through the columns and unpack the values.  Make
    // sure they are written correctly.
    for (size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {
      const flexible_type& v = column_buffers[c_idx][row_buffer_index];

      const column_metadata_ptr& m = rm.metadata_vect[c_idx];
      const std::shared_ptr<column_indexer>& m_idx = m->indexer;
      const std::shared_ptr<column_statistics>& m_stats = m->statistics;

      /** Call this to write out an index.
       */
      auto write_index = [&](size_t index) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        ++row_size;
        entry_value ev;
        ev.index_value = index;
        block_output.entry_data.push_back(ev);
      };

      /** Call this to write out a value.
       */
      auto write_value = [&](double value) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        ++row_size;
        entry_value ev;
        ev.double_value = value;
        block_output.entry_data.push_back(ev);
      };

      /** Call this to write out a size.
       */
      auto write_size = [&](size_t size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        entry_value ev;
        ev.index_value = size;
        block_output.entry_data.push_back(ev);
      };

      /** Call this to write out an index/double pair.
       */
      auto write_index_value_pair = [&](std::pair<size_t, double> p)
                                        GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
                                          ++row_size;
                                          entry_value ev;
                                          ev.index_value = p.first;
                                          block_output.entry_data.push_back(ev);
                                          ev.double_value = p.second;
                                          block_output.entry_data.push_back(ev);
                                        };

      /** Call this to throw a NA error.
       */
      auto bad_missing_value_encountered = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {

        log_and_throw(std::string("Missing value (None) encountered in ") +
                      "column '" + m->name + "'. " +
                      "Use the SFrame's dropna function to drop rows with "
                      "'None' values in them.");
      };

      /** Use this to retrieve a missing numeric value.
       */
      auto get_missing_numeric_value =
          [&](size_t feature_index) -> double GL_GCC_ONLY(GL_HOT_NOINLINE) {
        switch (none_action) {
          case ml_missing_value_action::ERROR:
            bad_missing_value_encountered();
            return 0;
          case ml_missing_value_action::IMPUTE:
            return m_stats->mean(feature_index);
          case ml_missing_value_action::USE_NAN:
            return NAN;
          default:
            ASSERT_TRUE(false);
            return 0;
        }
      };

      /** Use this to check that missing dicts and missing
       *  categorical_vectors are handled correctly.
       */
      auto verify_missing_categoricals_okay = [&]() GL_GCC_ONLY(GL_HOT_INLINE) {
        switch (none_action) {
          case ml_missing_value_action::ERROR:
            bad_missing_value_encountered();
          default:
            return;
        }
      };

      ////////////////////////////////////////////////////////////////////////////////
      // Step 3: Go through and write out the data using the above functions

      switch (rm.metadata_vect[c_idx]->mode) {
        case ml_column_mode::NUMERIC: {
          double v_d;

          // Missing value entries for numeric columns.
          if (UNLIKELY(v.get_type() == flex_type_enum::UNDEFINED)) {
            v_d = get_missing_numeric_value(0);
          } else {
            v_d = v;

            /**  Update Statistics  **/
            if (track_statistics) {
              value_vect = {v_d};
              m_stats->update_numeric_statistics(thread_idx, value_vect);
            }
          }

          /**  Write out the data in the proper format. **/
          write_value(v_d);

          break;
        }

        case ml_column_mode::NUMERIC_VECTOR:
        case ml_column_mode::NUMERIC_ND_VECTOR: {
          if (UNLIKELY(v.get_type() == flex_type_enum::UNDEFINED)) {
            // Top level None: An entire entry is missing.
            size_t n_values = m->fixed_column_size();

            for (size_t k = 0; k < n_values; ++k) {
              write_value(get_missing_numeric_value(k));
            }
          } else if (v.get_type() == flex_type_enum::VECTOR) {
            const std::vector<double>& feature_vect = v.get<flex_vec>();

            m->check_fixed_column_size(v);

            /**  Write out the data in the proper format. **/
            size_t n_values = feature_vect.size();

            for (size_t k = 0; k < n_values; ++k) {
              write_value(feature_vect[k]);
            }

            /**  Update Statistics  **/
            if (track_statistics) {
              m_stats->update_numeric_statistics(thread_idx, feature_vect);
            }
          } else if (v.get_type() == flex_type_enum::ND_VECTOR) {
            const flex_nd_vec& _a = v.get<flex_nd_vec>();
            flex_nd_vec _a_canonical;  // Temporary to make sure the
                                       // references stay valid
            const flex_nd_vec& a =
                _a.is_canonical() ? _a : (_a_canonical = _a.canonicalize());

            size_t n_values = a.num_elem();

            for (size_t i = 0; i < n_values; ++i) {
              write_value(a[i]);
            }

            /**  Update Statistics  **/
            m->check_fixed_column_size(v);

            if (track_statistics) {
              m_stats->update_numeric_statistics(thread_idx, a.raw_elements());
            }
          } else {
            ASSERT_TRUE(false);
          }

          break;
        }

        case ml_column_mode::CATEGORICAL: {
          /**  Map out the value.  **/

          // Missing values are always treated as their own category.
          size_t index;

          if (!immutable_metadata) {
            index = m_idx->map_value_to_index(thread_idx, v);
          } else {
            index = m_idx->immutable_map_value_to_index(v);
          }

          /**  Write out the data in the proper format. **/
          write_index(index);

          /**  Update Statistics  **/
          if (track_statistics) {
            index_vect = {index};
            m_stats->update_categorical_statistics(thread_idx, index_vect);
          }

          break;
        }

        case ml_column_mode::CATEGORICAL_VECTOR: {
          // Top level categorical vector is missing.
          if (UNLIKELY(v.get_type() == flex_type_enum::UNDEFINED)) {
            verify_missing_categoricals_okay();
            write_size(0);

          } else {
            const flex_list& vv = v.get<flex_list>();
            size_t n_values = vv.size();

            index_vect.resize(n_values);

            for (size_t k = 0; k < n_values; ++k) {
              if (!immutable_metadata) {
                index_vect[k] = m_idx->map_value_to_index(thread_idx, vv[k]);
              } else {
                index_vect[k] = m_idx->immutable_map_value_to_index(vv[k]);
              }
            }

            // Now, we want to sort the dictionaries by index; this
            // permits easy filling of an Eigen sparse vector when
            // the data is loaded, as we can insert it by index
            // order.
            std::sort(index_vect.begin(), index_vect.end());

            /**  Write out the data in the proper format. **/
            write_size(n_values);
            for (size_t k = 0; k < n_values; ++k) write_index(index_vect[k]);

            /**  Update Statistics  **/
            if (track_statistics)
              m_stats->update_categorical_statistics(thread_idx, index_vect);
          }

          break;
        }

        case ml_column_mode::DICTIONARY: {
          DASSERT_TRUE(exclusion_indices.empty());

          // Top level dictionary is missing
          if (v.get_type() == flex_type_enum::UNDEFINED) {
            verify_missing_categoricals_okay();
            write_size(0);

          } else {
            const flex_dict& dv = v.get<flex_dict>();
            size_t n_values = dv.size();

            idx_value_vect.resize(n_values);

            for (size_t k = 0; k < n_values; ++k) {
              const std::pair<flexible_type, flexible_type>& kvp = dv[k];

              size_t index;
              if (!immutable_metadata) {
                index = m_idx->map_value_to_index(thread_idx, kvp.first);
              } else {
                index = m_idx->immutable_map_value_to_index(kvp.first);
              }

              double value;

              if (kvp.second.get_type() == flex_type_enum::INTEGER ||
                  kvp.second.get_type() == flex_type_enum::FLOAT) {
                value = kvp.second;

              } else if (kvp.second.get_type() == flex_type_enum::UNDEFINED) {
                value = get_missing_numeric_value(index);
                exclusion_indices.push_back(index);

              } else {
                auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
                  log_and_throw(std::string("Dictionary value for key '") +
                                std::string(kvp.first) + "' in column '" +
                                m->name + "' is not numeric.");
                };

                throw_error();
              }

              idx_value_vect[k] = {index, value};
            }

            // Now, we want to sort the dictionaries by index; this
            // permits easy filling of an Eigen sparse vector when
            // the data is loaded, as we can insert it by index
            // order.
            std::sort(idx_value_vect.begin(), idx_value_vect.end());

            /**  Write out the data in the proper format. **/
            write_size(n_values);
            for (size_t k = 0; k < n_values; ++k)
              write_index_value_pair(idx_value_vect[k]);

            /**  Update Statistics  **/
            if (track_statistics) {
              if (!exclusion_indices.empty()) {
                // Efficiently remove the values recorded above as missing.
                auto rm_missing_values = [&]() GL_GCC_ONLY(
                                             GL_HOT_NOINLINE_FLATTEN) {

                  // Fast track the common single-occurance case.
                  if (exclusion_indices.size() == 1) {
                    size_t rm_index = exclusion_indices[0];
                    auto it = std::remove_if(
                        idx_value_vect.begin(), idx_value_vect.end(),
                        [&](const std::pair<size_t, double>& p) {
                          return p.first == rm_index;
                        });
                    idx_value_vect.resize(it - idx_value_vect.begin());
                  } else {
                    std::sort(exclusion_indices.begin(),
                              exclusion_indices.end());
                    auto it = std::remove_if(
                        idx_value_vect.begin(), idx_value_vect.end(),
                        [&](const std::pair<size_t, double>& p) {

                          // Is it in the exclusion_indices?
                          auto fit = std::lower_bound(exclusion_indices.begin(),
                                                      exclusion_indices.end(),
                                                      p.first);

                          return (fit != exclusion_indices.end() &&
                                  *fit == p.first);
                        });
                    idx_value_vect.resize(it - idx_value_vect.begin());
                  }

                  exclusion_indices.clear();
                };
                rm_missing_values();
              }

              m_stats->update_dict_statistics(thread_idx, idx_value_vect);
            }
          }
          break;
        }
        case ml_column_mode::CATEGORICAL_SORTED:
        case ml_column_mode::UNTRANSLATED:
          break;

      }  // end switch over column mode
    }    // End loop over columns

    if (!rm.data_size_is_constant) {
      DASSERT_TRUE(n_row_elements_write_index != size_t(-1));
      block_output.entry_data[n_row_elements_write_index].index_value =
          block_output.entry_data.size() - n_row_elements_write_index;
    }

    // Check the maximimum row size.
    max_row_size = std::max(max_row_size, row_size);

  }  // End loop over rows in buffer

  return max_row_size;
}

/** Truncates a row_data_block in place to n_rows.
 *
 */
void truncate_row_data_block(const row_metadata& rm, row_data_block& row_block,
                             size_t n_rows) {
  entry_value_iterator v_ptr = row_block.entry_data.data();

  if (rm.data_size_is_constant) {
    v_ptr += n_rows * (rm.constant_data_size + 1);
  } else {
    for (size_t i = 0; i < n_rows; ++i) {
      size_t row_size = get_row_data_size(rm, v_ptr);
      v_ptr += row_size;
      DASSERT_TRUE(v_ptr <
                   row_block.entry_data.data() + row_block.entry_data.size());
    }
  }

  // Clip this last row to the data covered by v_ptr.
  row_block.entry_data.resize(
      std::distance(entry_value_iterator(row_block.entry_data.data()), v_ptr));
}

/** Takes a row from position entry_value_iterator, appending it to
 *  output_block.
 */
void append_row_to_row_data_block(const row_metadata& rm,
                                  row_data_block& output_block,
                                  entry_value_iterator src_location) {
  size_t row_size = get_row_data_size(rm, src_location);

  output_block.entry_data.insert(output_block.entry_data.end(), src_location,
                                 src_location + row_size);
}

/** Remap all the indices in a block, rewriting things back.
 */
void reindex_block(const row_metadata& rm, row_data_block& block,
                   const std::vector<std::vector<size_t> >& reindex_map) {
  const size_t num_columns = rm.total_num_columns;

  for (auto row_block_ptr = block.entry_data.begin();
       row_block_ptr != block.entry_data.end();) {
    /** Skip the first entry if the row size is not constant, as this
     * contains the row data size. */
    if (!rm.data_size_is_constant) ++row_block_ptr;

    for (size_t c_idx = 0; c_idx < num_columns; ++c_idx) {
      DASSERT_LT(c_idx, rm.metadata_vect.size());

      /** Call this to read out a size.
       */
      auto read_size = [&]() GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        size_t size = row_block_ptr->index_value;
        ++row_block_ptr;
        DASSERT_FALSE(rm.data_size_is_constant);
        return size;
      };

      ////////////////////////////////////////////////////////////////////////////////
      // Step 2:  Go through and read values based on the type.

      // For some of the modes, an important optimization is to set
      // all_columns_categorical to be true in the data profile, which
      // means that this branch / lookup is optimized out at compile
      // time.

      switch (rm.metadata_vect[c_idx]->mode) {
        case ml_column_mode::NUMERIC: {
          ++row_block_ptr;
          break;
        }

        case ml_column_mode::CATEGORICAL: {
          DASSERT_LT(c_idx, reindex_map.size());
          DASSERT_LT(row_block_ptr->index_value, reindex_map[c_idx].size());
          row_block_ptr->index_value =
              reindex_map[c_idx][row_block_ptr->index_value];
          ++row_block_ptr;
          break;
        }

        case ml_column_mode::NUMERIC_VECTOR: {
          size_t nv = rm.metadata_vect[c_idx]->fixed_column_size();
          row_block_ptr += nv;
          break;
        }

        case ml_column_mode::CATEGORICAL_VECTOR: {
          size_t nv = read_size();
          DASSERT_LT(c_idx, reindex_map.size());

          for (size_t k = 0; k < nv; ++k) {
            DASSERT_LT(row_block_ptr->index_value, reindex_map[c_idx].size());
            row_block_ptr->index_value =
                reindex_map[c_idx][row_block_ptr->index_value];
            ++row_block_ptr;
          }
          break;
        }

        case ml_column_mode::DICTIONARY: {
          size_t nv = read_size();

          DASSERT_LT(c_idx, reindex_map.size());
          for (size_t k = 0; k < nv; ++k) {
            DASSERT_LT(row_block_ptr->index_value, reindex_map[c_idx].size());
            row_block_ptr->index_value =
                reindex_map[c_idx][row_block_ptr->index_value];
            ++row_block_ptr;
            ++row_block_ptr;
          }
          break;
        }

        case ml_column_mode::NUMERIC_ND_VECTOR: {
          size_t nv = rm.metadata_vect[c_idx]->fixed_column_size();
          row_block_ptr += nv;
          break;
        }

        case ml_column_mode::UNTRANSLATED: {
          // Do nothing in this case.
          break;
        }
        default:
          DASSERT_TRUE(false);
      }  // End switch
    }
  }
}

/** Determines the number of ml_data_entry objects needed to fit a
 *  mapped buffer of flexible_type columns into.
 *
 *  \param[in] metadata A vector of column_metadata objects giving the
 *  metadata for each column.
 *
 *  \param[in] column_buffer A vector of vectors, with each inner
 *  vector holding one unindexed column from the raw source and
 *  corresponding to one of the column_metadata objects.  All the
 *  columns must be the same length.
 *
 *  \param[in] none_action Can be set to one of ERROR/SKIP/IMPUTE
 *
 *  \return The block size in number of ml_data_entry objects required.
 */
size_t estimate_num_data_entries(const column_metadata_ptr& m,
                                 const flexible_type& v) {
  switch (m->mode) {
    case ml_column_mode::NUMERIC:
      return 1;

    case ml_column_mode::NUMERIC_VECTOR:
      return m->fixed_column_size();

    case ml_column_mode::CATEGORICAL:
      return 1;

    case ml_column_mode::CATEGORICAL_VECTOR:
      if (LIKELY(v.get_type() == flex_type_enum::LIST))
        return 1 + v.get<flex_list>().size();
      else
        return 0;

    case ml_column_mode::DICTIONARY:
      if (LIKELY(v.get_type() == flex_type_enum::DICT))
        return 1 + 2 * v.get<flex_dict>().size();
      else
        return 0;

    case ml_column_mode::NUMERIC_ND_VECTOR:
      return m->fixed_column_size();

    case ml_column_mode::UNTRANSLATED:
      return 0;

    default:
      ASSERT_TRUE(false);
      return 0;
  }
}
}  // namespace ml_data_internal
}  // namespace turi
