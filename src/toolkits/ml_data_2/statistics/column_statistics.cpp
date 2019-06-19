/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/statistics/column_statistics.hpp>
#include <toolkits/ml_data_2/statistics/basic_column_statistics.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/variant.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

/** Construct and return a column statistics tracker by type.  Current
 *  statistics tracker types are given below:
 *
 *  "dense" : tracker that uses dense vectors to track everything.
 *  Compatible with the dense indexer type.
 *
 *  "hash_tracker" : An statistics in which each index is simply a 64 bit
 *  hash.  THIS WILL BE IMPLEMENTED IN A LATER PULL REQUEST.
 *
 *  To create a new column_statistics class, simply have it inherit
 *  COMMENT.
 */
std::shared_ptr<column_statistics> column_statistics::factory_create(
    const std::map<std::string, variant_type>& creation_options) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Get the statistics type that we want to recover.

  std::string statistics_type = variant_get_value<std::string>(creation_options.at("statistics_type"));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Get the version if it is not present already.  This isn't
  // needed yet, but will (hopefully) futureproof this part of the code.

  size_t version __attribute__((unused));  // Delete the attribute when it is used.

  if(creation_options.count("version")) {
    version = variant_get_value<size_t>(creation_options.at("version"));
  } else {
    version = 1;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Instantiate that statistics.  If you want to add in a new
  // statistics, you need to add it in here.

  std::shared_ptr<column_statistics> m;

  if(statistics_type == "basic-dense") {
    m.reset(new basic_column_statistics);
  } else {
    ASSERT_MSG(false, (statistics_type + " is not a valid type of statistics tracker.").c_str());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Set up that statistics class with the appropriate
  // parameters.

#define __EXTRACT(var)                                                  \
  m->var = variant_get_value<decltype(m->var)>(creation_options.at(#var));

  __EXTRACT(options);
  __EXTRACT(column_name);
  __EXTRACT(mode);
  __EXTRACT(original_column_type);

#undef __EXTRACT

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5: Save the creation parameters so that they can be reused
  // during serialization.

  m->creation_options = creation_options;

  return m;
}

/**
 * Equality testing -- slow!  Use for debugging/testing
 */
bool column_statistics::operator==(const column_statistics& other) const {
  if(mode != other.mode
     || options != other.options
     || original_column_type != other.original_column_type
     || column_name != other.column_name)
    return false;

  return is_equal(&other);

}

/**
 * Inequality testing -- slow!  Use for debugging/testing
 */
bool column_statistics::operator!=(const column_statistics& other) const {
  return (!(*this == other));
}


}}}
