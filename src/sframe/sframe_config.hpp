/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_CONFIG_HPP
#define TURI_SFRAME_CONFIG_HPP
#include <cstddef>
namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
** Global configuration for sframe, keep them as non-constants because we want to
** allow user/server to change the configuration according to the environment
**/
namespace sframe_config {
  /**
  **  The max buffer size to keep for sorting in memory
  **/
  extern size_t SFRAME_SORT_BUFFER_SIZE;

  /**
  **  The number of rows to read each time for paralleliterator
  **/
  extern size_t SFRAME_READ_BATCH_SIZE;
}

/// \}
}
#endif //TURI_SFRAME_CONFIG_HPP
