/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_class_registry.hpp>
#include <model_server/lib/extensions/model_base.hpp>

namespace turi {

bool toolkit_class_registry::register_toolkit_class(
    const std::string& class_name,
    std::function<model_base*()> constructor,
    std::map<std::string, flexible_type> description) {
  log_func_entry();
  if (registry.count(class_name)) {
    return false;
  } else {
    registry[class_name] = constructor;
    description["name"] = class_name;
    descriptions[class_name] = description;
    return true;
  }
}

bool toolkit_class_registry::register_toolkit_class(
    std::vector<toolkit_class_specification> classes, std::string prefix) {
  bool success = true;
  if (prefix.length() > 0) {
    for (size_t i = 0;i < classes.size(); ++i) {
      classes[i].name = prefix + "." + classes[i].name;
    }
  }
  for (size_t i = 0;i < classes.size(); ++i) {
    success &= register_toolkit_class(classes[i].name,
                              classes[i].constructor,
                              classes[i].description);
  }
  return success;
}

std::shared_ptr<model_base> toolkit_class_registry::get_toolkit_class(
    const std::string& class_name) {
  if (registry.count(class_name)) {
    return std::shared_ptr<model_base>(registry[class_name]());
  } else {
    log_and_throw(std::string("Class " + class_name + " does not exist."));
  }
}

std::map<std::string, flexible_type>
toolkit_class_registry::get_toolkit_class_description(const std::string& class_name) {
  if (descriptions.count(class_name)) {
    return descriptions[class_name];
  } else {
    log_and_throw(std::string("Class" + class_name + " does not exist."));
  }
}
std::vector<std::string> toolkit_class_registry::available_toolkit_classes() {
  std::vector<std::string> ret;
  for (const auto& kv : registry) {
    ret.push_back(kv.first);
  }
  return ret;
}

} // namespace turi
