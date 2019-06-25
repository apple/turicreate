/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/indexing/column_indexer.hpp>

#include <toolkits/ml_data_2/indexing/column_unique_indexer.hpp>

#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <map>
#include <model_server/lib/variant.hpp>


namespace turi { namespace v2 { namespace ml_data_internal {

/** Construct and return a column indexer by type.  Current indexer
 *  types are given below:
 *
 *  "unique_indexer" : An indexer in which each value is mapped to a
 *  unique index.  After mapping the values,
 *
 *  "hash_indexer" : An indexer in which each index is simply a 64 bit
 *  hash.  THIS WILL BE IMPLEMENTED IN A LATER PULL REQUEST.
 *
 *  To create a new indexer, simply have it inherit COMMENT.
 *
 */
std::shared_ptr<column_indexer> column_indexer::factory_create(
    const std::map<std::string, variant_type>& creation_options) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Get the indexer type that we want to recover.

  std::string indexer_type = variant_get_value<std::string>(creation_options.at("indexer_type"));

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
  // Step 3: Instantiate that indexer.  If you want to add in a new
  // indexer, you need to add it in here.

  std::shared_ptr<column_indexer> m;

  if(indexer_type == "unique") {
    m.reset(new column_unique_indexer);
  } else {
    ASSERT_MSG(false, (indexer_type + " is not a valid type of indexer.").c_str());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Set up that indexer class with the appropriate
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



}}}
