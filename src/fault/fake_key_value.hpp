/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#ifndef ZOOKEEPER_KEY_VALUE_HPP
#define ZOOKEEPER_KEY_VALUE_HPP
#include <boost/function.hpp>

namespace turi {
namespace zookeeper_util {


/**
 * This is a fake class used to mock the key_value object in the event that 
 * zookeeper is unnecessary.
 */ 
class key_value {
 public:
  
  ///  Joins a zookeeper cluster. 
  ///  Zookeeper nodes will be created in the prefix "prefix".
  inline key_value(std::vector<std::string> zkhosts, 
                   std::string prefix,
                   std::string serveridentifier) {}
  /// destructor
  inline ~key_value() {};

  /** Inserts a value to the key value store. Returns true on success.
   * False on failure (indicating the key already exists)
   */
  inline bool insert(const std::string& key, const std::string& value) { return true; }

  /** Modifies the value in the key value store. Returns true on success.
   * False on failure. This instance must own the key (created the key) 
   * to modify its value.
   */
  inline bool modify(const std::string& key, const std::string& value) {return true; }

  /** Removed a key in the key value store. Returns true on success.
   * False on failure. This instance must own the key (created the key) 
   * to delete the key.
   */
  inline bool erase(const std::string& key) { return true; }


  /// Gets a value of a key. First element of the pair is if the key was found
  inline std::pair<bool, std::string> get(const std::string& key) {
    return std::pair<bool, std::string>(false, "");
  }


  typedef boost::function<void(key_value*,
                               const std::vector<std::string>& out_newkeys,
                               const std::vector<std::string>& out_deletedkeys,
                               const std::vector<std::string>& out_modifiedkeys) 
                          >  callback_type;

  /** Adds a callback which will be triggered when any key/value 
   * changes. The callback arguments will be the key_value object, 
   * and the new complete key-value mapping.
   * Calling this function will a NULL argument deletes
   * the callback. Note that the callback may be triggered in a different thread. 
   *
   * Returns the id of the callback. Calling remove_callback with the id
   * disables the callback. 
   */
  int add_callback(callback_type fn) {
    return 0;
  }


  /** Removes a callback identified by an ID. Returns true on success,
   * false on failure */
  bool remove_callback(int fnid) {
    return true;
  }
};


} // namespace zookeeper
} // namespace turi
#endif

