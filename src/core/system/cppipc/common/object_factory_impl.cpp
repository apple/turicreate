/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/cppipc/common/object_factory_impl.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>
#include <core/logging/logger.hpp>

namespace cppipc {
size_t object_factory_impl::make_object(std::string object_type_name) {
  logstream(LOG_DEBUG) << "Creating object of type : "<< object_type_name << "\n";
  std::shared_ptr<void> ptr;
  if (constructors.count(object_type_name)) ptr = constructors[object_type_name]();
  if (ptr) {
    // register in the server
    size_t id = srv.register_object(ptr);
    logstream(LOG_DEBUG) << "New object with id " << id << " registered\n";
    return id;
  } else {
    return (size_t)(-1);
  }
}

std::string object_factory_impl::ping(std::string pingval) {
  unsigned long long cancel_id = 0;
  if(boost::starts_with(pingval, "ctrlc")) {
    std::string cancel_str = pingval.substr(5, pingval.length() - 5);
    cancel_id = std::stoull(cancel_str);
  }

  if(cancel_id != 0) {
    // If the cancelled command matches the currently running one, change
    // this value to uint64_t(-1) to show that we must cancel.
    bool ret = get_srv_running_command().compare_exchange_strong(cancel_id, (unsigned long long)uint64_t(-1));
    if(ret) {
      logstream(LOG_DEBUG) << "Cancelling command " << cancel_id << std::endl;
    }
  }

  return pingval;
}

void object_factory_impl::delete_object(size_t object_id) {
  logstream(LOG_DEBUG) << "Deleting Object : " << object_id << "\n";
  srv.delete_object(object_id);
}

std::string object_factory_impl::get_status_publish_address() {
  return srv.get_status_address();
}

std::string object_factory_impl::get_control_address() {
  return srv.get_control_address();
}

void object_factory_impl::sync_objects(std::vector<size_t> object_ids, bool active_list) {
  srv.delete_unused_objects(object_ids, active_list);
}

} // namespace cppipc
