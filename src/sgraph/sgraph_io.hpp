/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_IO_HPP
#define TURI_SGRAPH_IO_HPP

#include<string>

class sgraph;

namespace turi {

/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_main Main SGraph Objects
 * \{
 */

  /**
   * Write the content of the graph into a json file.
   */
  void save_sgraph_to_json(const sgraph& g,
                           std::string targetfile);

  /**
   * Write the content of the graph into a collection csv files under target directory.
   * The vertex data are saved to vertex-groupid-partitionid.csv and edge data
   * are saved to edge-groupid-partitionid.csv.
   */
  void save_sgraph_to_csv(const sgraph& g,
                          std::string targetdir);

/// \}
}

#endif
