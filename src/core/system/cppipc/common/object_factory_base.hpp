/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_COMMON_OBJECT_FACTORY_BASE_HPP
#define CPPIPC_COMMON_OBJECT_FACTORY_BASE_HPP
#include <map>
#include <string>
#include <typeinfo>
#include <core/system/cppipc/cppipc.hpp>
namespace cppipc {


/**
 * \internal
 * \ingroup cppipc
 * The object factory is the root object and has special powers.
 */
class object_factory_base {
 public:
  /**
   * Creates an object with type "objectname" on the server
   * and returns the object ID of the object
   */
  virtual size_t make_object(std::string objectname) = 0;

  /**
   * Replies with the pingval.
   */
  virtual std::string ping(std::string pingval) = 0;

  /**
   * Deletes the object refered to by an object ID
   */
  virtual void delete_object(size_t object_id) = 0;

  /**
   * Get the address on which the server is publishing status updates.
   */
  virtual std::string get_status_publish_address() = 0;

  /**
   * Get the address which the server is receiving control messages
   */
  virtual std::string get_control_address() = 0;

  /**
   * Takes a list of active objects on the client side and garbage collects
   * the objects on the server that are now unused by the client.
   *
   * If active_list is true, the object_ids refer to the list of objects that
   * are still active: in other words, the server should delete any objects that
   * are not in the active_list.
   *
   * If active_list is false, the object_ids refer to the list of objects that
   * are no longer active: in other words, the server should delete any objects
   * that are in the active list
   */
  virtual void sync_objects(std::vector<size_t> object_ids, bool active_list) = 0;

  virtual ~object_factory_base() { }

  REGISTRATION_BEGIN(object_factory)
      REGISTER(object_factory_base::make_object)
      REGISTER(object_factory_base::ping)
      REGISTER(object_factory_base::delete_object)
      REGISTER(object_factory_base::get_status_publish_address)
      REGISTER(object_factory_base::get_control_address)
      REGISTER(object_factory_base::sync_objects)
  REGISTRATION_END
};



} // cppipc
#endif
