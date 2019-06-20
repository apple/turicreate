/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/**
 * @file log_level_setter.hpp
 */

#ifndef TURI_LOG_LEVEL_SETTER_HPP
#define TURI_LOG_LEVEL_SETTER_HPP

#include <core/logging/logger.hpp>

/**
 * \ingroup turilogger
 * Class for setting global log level, unsets log level on destruction.
 *
 * Create a log_level_setter object to change the loglevel as desired.
 * Upon destruction of the object, the loglevel will be reset to the
 * previous logging level.
 *
 * auto e = log_level_setter(LOG_NONE); // quiets the logging that follows
 */
class log_level_setter {
 private:
  int prev_level;
 public:

  /**
   * Set global log level to the provided log level.
   * \param loglevel desired loglevel. See logger.hpp:97 for a description of
   * each level.
   */
  log_level_setter(int loglevel);

  /**
   * Destructor resets global log level to the previous level.
   */
  ~log_level_setter();
};


#endif
