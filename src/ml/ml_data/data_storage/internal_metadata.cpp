/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/data_storage/internal_metadata.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>
#include <ml/ml_data/metadata.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

namespace turi { namespace ml_data_internal {

void column_metadata::setup(
    bool is_target_column,
    const std::string& column_name,
    const std::shared_ptr<sarray<flexible_type> >& column,
    const std::map<std::string, ml_column_mode>& mode_overrides)
{

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set column constants

  name = column_name;
  original_column_type = column->get_type();
  mode = choose_column_mode(column_name, original_column_type, mode_overrides);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set the column indexer

  indexer.reset(new column_indexer(column_name, mode, original_column_type));
  statistics.reset(new column_statistics(column_name, mode, original_column_type));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: If the column has a fixed size, we need to figure that out.

  if(turi::mode_has_fixed_size(mode)) {

    switch(mode) {
      case ml_column_mode::NUMERIC:
      case ml_column_mode::CATEGORICAL:
        column_data_size_if_fixed = 1;
        break;

      case ml_column_mode::NUMERIC_VECTOR: {

        // We need to go through and read it until
        // we hit one that is not a missing
        // value.
        size_t num_rows = column->size();
        auto reader = column->get_reader();

        std::vector<flexible_type> buffer;

        column_data_size_if_fixed = size_t(-1);
        size_t row = 0;
        while(row < num_rows) {
          reader->read_rows(row, row + 1, buffer);

          if(buffer[0].get_type() == flex_type_enum::VECTOR) {
            column_data_size_if_fixed = buffer[0].get<flex_vec>().size();
            break;
          } else if(buffer[0].get_type() == flex_type_enum::ND_VECTOR) {
            const auto& ndv = buffer[0].get<flex_nd_vec>();
            if(ndv.shape().size() != 1) {
              log_and_throw("ND Vector with number of dimensions greater than 1 encountered "
                            "in 1d vector column.");
            }
            column_data_size_if_fixed = ndv.shape()[0];
            break;
          } else if(buffer[0].get_type() == flex_type_enum::UNDEFINED) {
            ++row;
            continue;
          } else {
            log_and_throw("Non-vector type encountered in column of vectors.");
          }
        }

        if(column_data_size_if_fixed == size_t(-1)) {
          DASSERT_EQ(row, num_rows);
          logstream(LOG_WARNING) << "Column with only missing values encountered." << std::endl;
          column_data_size_if_fixed = 0;
        }

        break;
      }

      case ml_column_mode::NUMERIC_ND_VECTOR: {

        // We need to go through and read it until
        // we hit one that is not a missing
        // value.
        size_t num_rows = column->size();
        auto reader = column->get_reader();

        std::vector<flexible_type> buffer;
        nd_array_size.clear();
        bool shape_set = false;

        size_t row = 0;
        while(row < num_rows) {
          reader->read_rows(row, row + 1, buffer);

          if(buffer[0].get_type() == flex_type_enum::VECTOR) {
            nd_array_size = {buffer[0].get<flex_vec>().size()};
            shape_set = true;
            break;
          } else if(buffer[0].get_type() == flex_type_enum::ND_VECTOR) {
            nd_array_size = buffer[0].get<flex_nd_vec>().shape();
            shape_set = true;
            break;
          } else if(buffer[0].get_type() == flex_type_enum::UNDEFINED) {
            ++row;
            continue;
          } else {
            log_and_throw("Non-vector type encountered in column of vectors.");
          }
        }

        if(!shape_set) {
          DASSERT_EQ(row, num_rows);
          logstream(LOG_WARNING) << "Column with only missing values encountered." << std::endl;
          nd_array_size.clear();
          column_data_size_if_fixed = 0;
        } else {
          column_data_size_if_fixed = 1;
          for(const auto& s : nd_array_size) {
            column_data_size_if_fixed *= s;
          }
        }

        break;
      }

      case ml_column_mode::UNTRANSLATED:

        // This isn't put into the row_block, so it doesn't count.
        column_data_size_if_fixed = 0;
        break;

        // The rest of these shouldn't be encountered here.
      case ml_column_mode::CATEGORICAL_VECTOR:
      case ml_column_mode::DICTIONARY:
      default:
        DASSERT_TRUE(false);
    }

    // DONE!
  }
}

/** Finalize training.
 */
void column_metadata::set_training_index_size() {
  index_size_at_train_time = column_size();
}

/** If the global index offsets haven't been loaded already from the
 *  serialization method, then set them.
 */
void column_metadata::set_training_index_offset(size_t previous_total) {
  DASSERT_TRUE(index_size_at_train_time != size_t(-1));
  global_index_offset_at_train_time = previous_total;
}

#ifndef NDEBUG
void column_metadata::_debug_is_equal(const column_metadata& other) const {
  DASSERT_TRUE(name == other.name);
  DASSERT_TRUE(mode == other.mode);
  DASSERT_TRUE(original_column_type == other.original_column_type);

  DASSERT_EQ(index_size_at_train_time, other.index_size_at_train_time);
  DASSERT_EQ(column_data_size_if_fixed, other.column_data_size_if_fixed);
  DASSERT_EQ(global_index_offset_at_train_time, other.global_index_offset_at_train_time);

  indexer->debug_check_is_equal(other.indexer);
  statistics->_debug_check_is_approx_equal(other.statistics);
}
#endif


/** Serialization -- save.
 */
void column_metadata::save(turi::oarchive& oarc) const {

  size_t version = 3;

  std::map<std::string, variant_type> data = {
    {"version",                   to_variant(version)},
    {"name",                      to_variant(name)},
    {"mode",                      to_variant(mode)},
    {"index_size_at_train_time",  to_variant(index_size_at_train_time)},
    {"original_column_type",      to_variant(original_column_type)},
    {"column_data_size_if_fixed", to_variant(column_data_size_if_fixed)},
    {"nd_array_size",             to_variant(nd_array_size)},
    {"global_index_offset_at_train_time", to_variant(global_index_offset_at_train_time)}};

  variant_deep_save(data, oarc);

  oarc << indexer << statistics;
}

/** Serialization -- load.
 */
void column_metadata::load(turi::iarchive& iarc) {

  std::map<std::string, variant_type> data;
  variant_deep_load(data, iarc);

  // Extract the version part.
  size_t version = 1;
  if(data.count("version")) {
    version = variant_get_value<size_t>(data.at("version"));
  }

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(name);
  __EXTRACT(mode);
  __EXTRACT(original_column_type);
  __EXTRACT(index_size_at_train_time);
  __EXTRACT(column_data_size_if_fixed);

  // Now we need to see if the offset values are in there.  If they
  // are not, then we have an old version.

  if(version >= 2) {
    __EXTRACT(global_index_offset_at_train_time);
  } else {

    // If -1, this will get reset by the wrapping metadata.  We have
    // to do some gymnastics here for backward compatability of
    // models.
    global_index_offset_at_train_time = size_t(-1);
  }

  // Handle the added version 3
  if(version >= 3) {
    __EXTRACT(nd_array_size);
  } else {
    nd_array_size.clear();
  }

#undef __EXTRACT

  iarc >> indexer >> statistics;
}

/**  Set up the row metadata, along
 *
 */
void row_metadata::setup(const std::vector<column_metadata_ptr>& _metadata_vect, bool _has_target) {

  // Need to special case this one
  if(_metadata_vect.size() == 0) {
    has_target = 0;
    metadata_vect.clear();
    total_num_columns = 0;
    num_x_columns = 0;
    target_is_indexed = false;
    constant_data_size = 0;
    data_size_is_constant = true;
    return;
  }

  has_target = _has_target;
  metadata_vect = _metadata_vect;

  total_num_columns = metadata_vect.size();
  num_x_columns = (total_num_columns - (has_target ? 1 : 0));

  target_is_indexed = (has_target && mode_is_indexed(metadata_vect.back()->mode));

  // Now just need to set data_size_is_constant and data_size.
  size_t num_columns = metadata_vect.size();

  constant_data_size = 0;
  data_size_is_constant = true;

  for(size_t c_idx = 0; c_idx < num_columns; ++c_idx) {
    if(metadata_vect[c_idx]->mode_has_fixed_size() ) {
      constant_data_size += metadata_vect[c_idx]->fixed_column_size();
    } else {
      data_size_is_constant = false;
      break;
    }
  }

  if(!data_size_is_constant)
    constant_data_size = 0;
}

/** Serialization -- save.
 */
void row_metadata::save(turi::oarchive& oarc) const {
  oarc << has_target
       << target_is_indexed
       << data_size_is_constant
       << constant_data_size
       << num_x_columns
       << total_num_columns
       << metadata_vect;
}

/** Serialization -- load.
 */
void row_metadata::load(turi::iarchive& iarc) {
  iarc >> has_target
       >> target_is_indexed
       >> data_size_is_constant
       >> constant_data_size
       >> num_x_columns
       >> total_num_columns
       >> metadata_vect;
}

/** Checks for internal consistency.
 *
 */
#ifndef NDEBUG
void row_metadata::_debug_is_equal(const row_metadata& other) const {
  DASSERT_EQ(has_target, other.has_target);
  DASSERT_EQ(target_is_indexed, other.target_is_indexed);
  DASSERT_EQ(data_size_is_constant, other.data_size_is_constant);
  DASSERT_EQ(constant_data_size, other.constant_data_size);
  DASSERT_EQ(num_x_columns, other.num_x_columns);
  DASSERT_EQ(total_num_columns, other.total_num_columns);

  DASSERT_EQ(metadata_vect.size(), other.metadata_vect.size());
  for(size_t i = 0; i < metadata_vect.size(); ++i) {
    metadata_vect[i]->_debug_is_equal(*other.metadata_vect[i]);
  }
}
#endif



}}
