/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/ml_data_2/data_storage/internal_metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

void column_metadata::setup(
    bool is_target_column,
    const std::string& _column_name,
    const std::shared_ptr<sarray<flexible_type> >& column,
    const std::map<std::string, ml_column_mode>& mode_overrides,
    const std::map<std::string, flexible_type>& options)
{

  name = _column_name;
  original_column_type = column->get_type();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set the column mode

  mode = choose_column_mode(name, original_column_type, options, mode_overrides);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set the column indexer

  std::string indexer_type;

  if(is_target_column) {
    indexer_type    = options.at("target_column_indexer_type").get<std::string>();
  } else {
    indexer_type    = options.at("column_indexer_type").get<std::string>();
  }

  indexer = column_indexer::factory_create(
      { {"indexer_type",             to_variant(indexer_type)},
        {"column_name",              to_variant(name)},
        {"mode",                     to_variant(mode)},
        {"original_column_type",     to_variant(original_column_type)},
        {"options",                  to_variant(options)} });

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the statistics.

  std::string statistics_type;

  if(is_target_column) {
    statistics_type = options.at("target_column_statistics_type").get<std::string>();
  } else {
    statistics_type = options.at("column_statistics_type").get<std::string>();
  }

  statistics = column_statistics::factory_create(
      { {"statistics_type",      to_variant(statistics_type)},
        {"column_name",          to_variant(name)},
        {"mode",                 to_variant(mode)},
        {"original_column_type", to_variant(original_column_type)},
        {"options",              to_variant(options)} });


  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: If the column has a fixed size, we need to figure that out.

  if(v2::mode_has_fixed_size(mode)) {

    switch(mode) {
      case ml_column_mode::NUMERIC:
      case ml_column_mode::CATEGORICAL:
        column_data_size_if_fixed = 1;
        break;

      case ml_column_mode::NUMERIC_VECTOR: {

        // This is the only one that needs some care.  We need to go
        // through and read it until we hit one that is not a missing
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
          } else if(buffer[0].get_type() == flex_type_enum::UNDEFINED) {
            ++row;
            continue;
          } else {
            ASSERT_MSG(false, "Non-vector type encountered in column of vectors.");
          }
        }

        if(column_data_size_if_fixed == size_t(-1)) {
          DASSERT_EQ(row, num_rows);
          logstream(LOG_WARNING) << "Column with only missing values encountered." << std::endl;
          column_data_size_if_fixed = 0;
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
  if(global_index_offset_at_train_time == size_t(-1))
    global_index_offset_at_train_time = previous_total;
}

/** Serialization -- save.
 */
void column_metadata::save(turi::oarchive& oarc) const {

  size_t version = 2;

  std::map<std::string, variant_type> data = {
    {"version",                   to_variant(version)},
    {"name",                      to_variant(name)},
    {"mode",                      to_variant(mode)},
    {"index_size_at_train_time",  to_variant(index_size_at_train_time)},
    {"original_column_type",      to_variant(original_column_type)},
    {"column_data_size_if_fixed", to_variant(column_data_size_if_fixed)},
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

#undef __EXTRACT


  iarc >> indexer >> statistics;
}
  
std::shared_ptr<column_metadata> column_metadata::create_cleared_copy() const { 
  auto ret = std::make_shared<column_metadata>(*this);

  ret->indexer = indexer->create_cleared_copy();
  ret->statistics = statistics->create_cleared_copy();

  return ret; 
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


}}}
