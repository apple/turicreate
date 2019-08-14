/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>

// Indexer and SFrame Index mapping
#include <toolkits/ml_data_2/indexing/column_indexer.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <core/util/try_finally.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {

static flex_type_enum get_new_indexed_column_type(
    flex_type_enum src_type,
    const std::shared_ptr<column_indexer>& indexer) {

  // Set the type
  switch(indexer->mode) {

    case ml_column_mode::NUMERIC:
      return flex_type_enum::FLOAT;
    case ml_column_mode::NUMERIC_VECTOR:
      return flex_type_enum::VECTOR;
    case ml_column_mode::CATEGORICAL_VECTOR:
    case ml_column_mode::CATEGORICAL:
    case ml_column_mode::DICTIONARY: {
      switch(src_type) {
        case flex_type_enum::DICT:      return flex_type_enum::DICT;
        case flex_type_enum::LIST: return flex_type_enum::LIST;
        default:                        return flex_type_enum::INTEGER;
      }
    }
    default:
      ASSERT_MSG(false, "get_new_indexed_column_type: MODE MISMATCH.");
      return flex_type_enum::INTEGER;
  }
}


/**
 * Translate from external SArray to SArray. If
 * allow_new_categorical_values is false, indexer is not changed and
 * unmapped values are set to -1, with warning.
 */
std::shared_ptr<sarray<flexible_type> > map_to_indexed_sarray(
    const std::shared_ptr<column_indexer>& indexer,
    const std::shared_ptr<sarray<flexible_type> >& src,
    bool allow_new_categorical_values) {

  atomic<size_t> examples_with_new_categories = 0;

  parallel_sframe_iterator_initializer it_init(sframe({src}, {"column"}));

  ////////////////////////////////////////////////////////////////////////////////
  if(indexer->mode == ml_column_mode::NUMERIC
     || indexer->mode == ml_column_mode::NUMERIC_VECTOR) {

    return src;
  }

  size_t num_segments = thread::cpu_count();

  std::shared_ptr<sarray<flexible_type> > new_x(new sarray<flexible_type>);
  new_x->open_for_write(num_segments);

  flex_type_enum src_type = src->get_type();

  new_x->set_type(get_new_indexed_column_type(src_type, indexer));

  scoped_finally indexer_finalizer;

  indexer->initialize();
  indexer_finalizer.add([indexer](){indexer->finalize();});

  ////////////////////////////////////////////////////////////////////////////////
  // Figure out the run_mode.

  ml_column_mode run_mode = indexer->mode;

  // Figure out the mode of the output transformation.  We want to allow
  // cross-indexing of categorical and dictionary types.
  if(run_mode == ml_column_mode::CATEGORICAL
     || run_mode == ml_column_mode::CATEGORICAL_VECTOR
     || run_mode == ml_column_mode::DICTIONARY) {

    switch(src_type) {
      case flex_type_enum::DICT:
        run_mode = ml_column_mode::DICTIONARY;
        break;
      case flex_type_enum::LIST:
        run_mode = ml_column_mode::CATEGORICAL_VECTOR;
        break;
      default:
        run_mode = ml_column_mode::CATEGORICAL;
        break;
    }
  } else {
    if(src_type != indexer->original_column_type)
      log_and_throw(std::string("Type mismatch on column ") + indexer->column_name
                    + ("; Column type does not match column type "
                       "specified at model creation time."));
  }

  ////////////////////////////////////////////////////////////////////////////////-
  // Do the transformation

  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // If we don't have that many rows, deterministically do this.  This
    // makes a number of test cases much easier to write.
    bool deterministic_mode = (src->size() <= 10000);

    if(deterministic_mode) {
      if(thread_idx != 0) {
        return;
      } else {
        num_threads = 1;
      }
    }

    auto it_out = new_x->get_output_iterator(thread_idx);

    // Buffers for the writing
    flex_vec  it_out_vec;
    flex_list  it_out_rec;
    flex_dict it_out_dict;

    for(parallel_sframe_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it, ++it_out) {

      const flexible_type& v = it.value(0);

      switch(run_mode) {
        case ml_column_mode::CATEGORICAL: {

          size_t index;
          if(allow_new_categorical_values) {
            index = indexer->map_value_to_index(thread_idx, v);
          } else {
            index = indexer->immutable_map_value_to_index(v);

            if(index == size_t(-1))
              ++examples_with_new_categories;
          }

          *it_out = index;
          break;
        }

        case ml_column_mode::CATEGORICAL_VECTOR: {
          const flex_list& vv = v.get<flex_list>();
          size_t n_values = vv.size();

          it_out_rec.resize(n_values);

          for(size_t k = 0; k < n_values; ++k) {
            size_t index;

            if(allow_new_categorical_values) {
              index = indexer->map_value_to_index(thread_idx, vv[k]);
            } else {
              index = indexer->immutable_map_value_to_index(vv[k]);

              if(index == size_t(-1))
                ++examples_with_new_categories;
            }

            it_out_rec[k] = index;
          }

          *it_out = it_out_rec;
          break;
        }

        case ml_column_mode::DICTIONARY: {

          const flex_dict& dv = v.get<flex_dict>();
          size_t n_values = dv.size();

          it_out_dict.resize(n_values);

          for(size_t k = 0; k < n_values; ++k) {
            const std::pair<flexible_type, flexible_type>& kvp = dv[k];

            size_t index;

            if(allow_new_categorical_values) {
              index = indexer->map_value_to_index(thread_idx, kvp.first);
            } else {
              index = indexer->immutable_map_value_to_index(kvp.first);

              if(index == size_t(-1))
                ++examples_with_new_categories;
            }

            it_out_dict[k] = {index, kvp.second};
          }

          *it_out = it_out_dict;
          break;
        }

        default:
          DASSERT_TRUE(false);
          break;
      } // End switch
    } // end value iteration
  }); // End parallel evaluation

  indexer_finalizer.execute_and_clear();

  if (examples_with_new_categories > 0){
    logprogress_stream << "Warning: " << examples_with_new_categories
                       << " examples have categories in column '" << indexer->column_name
                       << "' that were not present during train time. "
                       << "Best effort was made for these examples." << std::endl;
  }
  new_x->close();
  return new_x;
}

/**
 * Translate from external SArray to SArray. If
 * allow_new_categorical_values is false, indexer is not changed and
 * unmapped values are set to -1, with warning.
 */
std::shared_ptr<sarray<flexible_type> > map_from_indexed_sarray(
    const std::shared_ptr<column_indexer>& indexer,
    const std::shared_ptr<sarray<flexible_type> >& src) {

  parallel_sframe_iterator_initializer it_init(sframe({src}, {"column"}));

  ////////////////////////////////////////////////////////////////////////////////

  if(indexer->mode == ml_column_mode::NUMERIC
     || indexer->mode == ml_column_mode::NUMERIC_VECTOR) {

    return src;
  }

  size_t num_segments = thread::cpu_count();

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the output type based on the input type. Only dealing with
  // categorical variables at this point.

  flex_type_enum src_type = src->get_type();
  flex_type_enum out_type;
  ml_column_mode run_mode;

  switch(src_type) {
    case flex_type_enum::DICT:
      run_mode = ml_column_mode::DICTIONARY;
      out_type = flex_type_enum::DICT;
      break;
    case flex_type_enum::LIST:
      run_mode = ml_column_mode::CATEGORICAL_VECTOR;
      out_type = flex_type_enum::LIST;
      break;
    default:
      run_mode = ml_column_mode::CATEGORICAL;
      out_type = indexer->original_column_type;

      if(out_type == flex_type_enum::DICT || out_type == flex_type_enum::LIST) {
        // With these types, it's a bit trickier.  We have to use the
        // type of the values stored in the array, since the indexer
        // does not itself store this information.

        std::set<flex_type_enum> value_types_present = indexer->extract_key_types();

        // If undefined is in there, it is typically present with
        // other values.
        if(value_types_present.find(flex_type_enum::UNDEFINED) != value_types_present.end())
          value_types_present.erase(flex_type_enum::UNDEFINED);

        // If no data is present, then use undefined.
        if(value_types_present.size() == 0)
          value_types_present.insert(flex_type_enum::UNDEFINED);

        if(value_types_present.size() == 1) {
          out_type = *value_types_present.begin();
        } else {
          logprogress_stream << ("WARNING: Differing categorical types present in list or "
                                 "dictionary; promoting all to string type.") << std::endl;
          out_type = flex_type_enum::STRING;
        }
      }

      break;
  }

  std::shared_ptr<sarray<flexible_type> > new_x(new sarray<flexible_type>);
  new_x->open_for_write(num_segments);
  new_x->set_type(out_type);

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      auto it_out = new_x->get_output_iterator(thread_idx);

      // Buffers for the writing
      flex_vec  it_out_vec;
      flex_list  it_out_rec;
      flex_dict it_out_dict;

      for(parallel_sframe_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it, ++it_out) {

        const flexible_type& v = it.value(0);

        switch(run_mode) {
          case ml_column_mode::CATEGORICAL: {
            *it_out = indexer->map_index_to_value(v);
            break;
          }

          case ml_column_mode::CATEGORICAL_VECTOR: {
            const flex_list& vv = v.get<flex_list>();
            size_t n_values = vv.size();

            it_out_rec.resize(n_values);

            for(size_t k = 0; k < n_values; ++k)
              it_out_rec[k] = indexer->map_index_to_value(vv[k]);

            *it_out = it_out_rec;
            break;
          }

          case ml_column_mode::DICTIONARY: {

            const flex_dict& dv = v.get<flex_dict>();
            size_t n_values = dv.size();

            it_out_dict.resize(n_values);

            for(size_t k = 0; k < n_values; ++k) {
              const std::pair<flexible_type, flexible_type>& kvp = dv[k];

              it_out_dict[k] = {indexer->map_index_to_value(kvp.first), kvp.second};
            }

            *it_out = it_out_dict;
            break;
          }

          default:
            DASSERT_TRUE(false);
            break;
        } // End switch
      } // end value iteration
    }); // End parallel evaluation

  new_x->close();
  return new_x;
}




/**
 * Translate from external SFrame to indexed SFrame
 */
sframe map_to_indexed_sframe(
    const std::vector<std::shared_ptr<column_indexer> >& metadata,
    sframe unindexed_x,
    bool allow_new_categorical_values) {

  const size_t n_columns = metadata.size();

  std::vector<std::string> column_names(n_columns);

  // If the original one is empty, create an empty sframe with the
  // proper columns and return that.
  if(unindexed_x.size() == 0) {
    std::vector<flex_type_enum> column_types(n_columns);
    for(size_t column_idx = 0; column_idx < n_columns; ++column_idx) {
      column_names[column_idx] = metadata[column_idx]->column_name;
      column_types[column_idx] = get_new_indexed_column_type(
          metadata[column_idx]->original_column_type, metadata[column_idx]);
    }

    sframe out;
    out.open_for_write(column_names, column_types);
    out.close();

    return out;
  }

  std::vector<std::shared_ptr<sarray<flexible_type> > > out_columns(metadata.size());

  // It is now parallelized within the sarray mapping; no need to do
  // this here.
  for(size_t column_idx = 0; column_idx < n_columns; ++column_idx) {

    std::string name = metadata[column_idx]->column_name;
    column_names[column_idx] = name;

    auto in_column = unindexed_x.select_column(name);

    out_columns[column_idx] = map_to_indexed_sarray(metadata[column_idx], in_column,
                                                    allow_new_categorical_values);
  }

  sframe sf = sframe(out_columns, column_names);

  DASSERT_EQ(sf.size(), unindexed_x.size());

  return sf;
}

  /**
   * Translates an external SFrame into the corresponding indexed
   * SFrame representation, as dictated by the indexing in
   * column_indexer.  Only the columns specified in metadata are
   * used, and all of these must be present.
   *
   * If allow_new_categorical_values is false, then the metadata is
   * not changed.  New categorical values are mapped to size_t(-1)
   * with a warning.
   *
   * Categorical: If a column is categorical, each unique value is mapped to
   * a unique index in the range 0, ..., n-1, where n is the number of unique
   * values.
   *
   * Numeric: The column type is checked to be INT/FLOAT, then
   * returned as-is.
   *
   * Numeric Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Categorical Vector: If the dictated column type is VECTOR, it is
   * checked to make sure it is numeric and of homogeneous size.
   *
   * Dictionary : If the dictated column type is DICT, it is checked to make
   * sure the values are numeric. The keys are then translated to 0..n-1
   * where n is the number of unique keys.
   *
   * \overload
   *
   * \param[in,out] metadata The metadata used for the mapping.
   * \param[in] unindexed_x The SFrame to map to indices.
   * \param[in] allow_new_categorical_values Whether to allow new categories.
   *
   * \returns Indexed SFrame.
   */
sframe map_to_indexed_sframe(
    const std::shared_ptr<ml_metadata>& metadata,
    sframe unindexed_x,
    bool allow_new_categorical_values) {

  std::vector<std::shared_ptr<ml_data_internal::column_indexer> > indexer_vect(metadata->num_columns());

  for(size_t i = 0; i < metadata->num_columns(); ++i) {
    indexer_vect[i] = metadata->indexer(i);
  }

  return map_to_indexed_sframe(indexer_vect, unindexed_x, allow_new_categorical_values);
}


/**
 * Translate from indexed SFrame to external SFrame
 */
sframe map_from_indexed_sframe(
    const std::vector<std::shared_ptr<column_indexer> >& metadata,
    sframe indexed_x) {

  const size_t n_columns = indexed_x.num_columns();

  std::vector<std::shared_ptr<sarray<flexible_type> > > out_columns(n_columns);

  std::vector<std::string> column_names(n_columns);

  for(size_t column_idx = 0; column_idx < n_columns; ++column_idx) {

    column_names[column_idx] = indexed_x.column_name(column_idx);

    auto in_column = indexed_x.select_column(column_names[column_idx]);

    if(column_idx >= metadata.size() || metadata[column_idx] == nullptr) {
      out_columns[column_idx] = in_column;
    } else {
      out_columns[column_idx] = map_from_indexed_sarray(metadata[column_idx], in_column);
      column_names[column_idx] = metadata[column_idx]->column_name;
    }
  }

  sframe sf = sframe(out_columns, column_names);

  DASSERT_EQ(sf.size(), indexed_x.size());

  return sf;
}

/**
 * Translates an indexed SFrame into the original non-indexed
 * representation, as dictated by the indexing in column_indexer.
 *
 * \param[in,out] metadata The metadata used for the mapping.
 * \param[in] indexing_x The indexed SArray to map to external values.
 *
 * \returns Indexed SFrame in original format.
 */
sframe map_from_indexed_sframe(
    const std::shared_ptr<ml_metadata>& metadata,
    sframe indexed_x) {

  std::vector<std::shared_ptr<ml_data_internal::column_indexer> > indexer_vect(metadata->num_columns());

  for(size_t i = 0; i < metadata->num_columns(); ++i) {
    indexer_vect[i] = metadata->indexer(i);
  }

  return map_from_indexed_sframe(indexer_vect, indexed_x);
}

////////////////////////////////////////////////////////////////////////////////

sframe map_from_custom_indexed_sframe(
    const std::map<std::string, std::shared_ptr<column_indexer> >& metadata_map,
    sframe indexed_x) {

  std::vector<std::shared_ptr<column_indexer> > metadata_v(indexed_x.num_columns());

  size_t mapped_count = 0;

  for(size_t i = 0; i < indexed_x.num_columns(); ++i) {
    const std::string& name = indexed_x.column_name(i);
    auto it = metadata_map.find(name);
    if(it != metadata_map.end()) {
      metadata_v[i] = it->second;
      ++mapped_count;
    }
  }

  // Make sure all the columns got mapped.
  DASSERT_EQ(mapped_count, metadata_map.size());

  return map_from_indexed_sframe(metadata_v, indexed_x);
}


}}
