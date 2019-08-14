/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_registry.hpp>
#include <model_server/lib/unity_global.hpp>
#include <model_server/lib/unity_global_singleton.hpp>

namespace turi {


bool toolkit_function_registry::register_toolkit_function(
    toolkit_function_specification spec,
    std::string prefix) {
  log_func_entry();
  // if there is something in the registry with this name, fail
  if (prefix.length() > 0) {
    spec.name = prefix + "." + spec.name;
  }
  if (registry.count(spec.name)) return false;
  registry[spec.name] = spec;
  return true;
}


bool toolkit_function_registry::register_toolkit_function(
    std::vector<toolkit_function_specification> specvec,
    std::string prefix) {
  log_func_entry();
  // if there is something in the registry with this name, fail
  for (auto& spec: specvec) {
    if (prefix.length() > 0) {
      spec.name = prefix + "." + spec.name;
    }
    if (registry.count(spec.name)) return false;
  }
  // now register
  for (const auto& spec: specvec) {
    registry[spec.name] = spec;
  }
  return true;
}


bool toolkit_function_registry::unregister_toolkit_function(std::string name) {
  log_func_entry();
  // look for the name
  auto iter = registry.find(name);
  if (iter != registry.end()) {
    // found! erase
    registry.erase(iter);
    return true;
  } else {
    // not found! fail
    return false;
  }
}



const toolkit_function_specification* toolkit_function_registry::get_toolkit_function_info(std::string name) {
  // look for the name
  auto iter = registry.find(name);
  return iter == registry.end() ? NULL : &(iter->second);
}


std::function<variant_type(const std::vector<variant_type>&)>
toolkit_function_registry::get_native_function(std::string toolkit_fn_name) {
  const auto* toolkit_fn_spec = get_toolkit_function_info(toolkit_fn_name);
  // basic error checking.
  if (toolkit_fn_spec == nullptr) {
    throw std::string("toolkit function " + toolkit_fn_name + " not found");
    return nullptr;
  }
  if (toolkit_fn_spec->native_execute_function == nullptr) {
    throw std::string("toolkit function " + toolkit_fn_name +
                      " cannot be run as a native lambda since it was not"
                      " compiled and registered using the SDK registration scheme.");
    return nullptr;
  }
  return toolkit_fn_spec->native_execute_function;
}


std::function<variant_type(const std::vector<variant_type>&)>
toolkit_function_registry::get_native_function(const function_closure_info& closure) {
  const auto* toolkit_fn_spec = get_toolkit_function_info(closure.native_fn_name);
  // basic error checking.
  if (toolkit_fn_spec == nullptr) {
    throw std::string("toolkit function " + closure.native_fn_name + " not found");
    return nullptr;
  }
  if (toolkit_fn_spec->native_execute_function == nullptr) {
    throw std::string("toolkit function " + closure.native_fn_name +
                      " cannot be run as a native lambda since it was not"
                      " compiled and registered using the SDK registration scheme.");
    return nullptr;
  }
  // now. we need to wrap the closure
  // some basic checking to make sure the closure is complete
  if (closure.arguments.size() != toolkit_fn_spec->description.at("arguments").size()) {
    throw std::string("Incomplete closure specified for toolkit function " +
                      closure.native_fn_name);
  }

  // first fast path for the identity case
  bool is_fast_path = true;
  for (size_t i = 0;i < closure.arguments.size(); ++i) {
    // every argument is a parameter matching the input ordering
    if (!(closure.arguments[i].first == function_closure_info::PARAMETER &&
        closure.arguments[i].second->which() == 0 &&
        variant_get_value<size_t>(*(closure.arguments[i].second)) == i)) {
      is_fast_path = false;
      break;
    }
  }
  // identity call. no transformation needed
  if (is_fast_path) {
    return toolkit_fn_spec->native_execute_function;
  }
  // how many arguments are there really after the closure application
  int real_num_args = -1;
  for (size_t i = 0;i < closure.arguments.size(); ++i) {
    if (closure.arguments[i].first == function_closure_info::PARAMETER) {
      int argnum = variant_get_value<int>(*(closure.arguments[i].second));
      real_num_args = std::max(real_num_args, argnum);
    }
  }
  ++real_num_args;
  // more complicated path. we need to build up a lambda
  auto native_execute_function = toolkit_fn_spec->native_execute_function;

  auto retlambda =
      [real_num_args,native_execute_function,closure](const std::vector<variant_type>& inargs)->variant_type {
        if (inargs.size() < (size_t)real_num_args) {
          throw std::string("Wrong number of arguments");
        }
        std::vector<variant_type> realargs(closure.arguments.size());
        for (size_t i = 0;i < closure.arguments.size(); ++i) {
          if (closure.arguments[i].first == function_closure_info::CAPTURED_VALUE) {
            realargs[i] = *(closure.arguments[i].second);
          } else {
            int index = variant_get_value<int>(*(closure.arguments[i].second));
            realargs[i] = inargs[index];
          }
        }
        return native_execute_function(realargs);
      };
  return retlambda;
}

std::vector<std::string> toolkit_function_registry::available_toolkit_functions() {
  std::vector<std::string> ret;
  for(auto entry : registry) {
    ret.push_back(entry.first);
  }
  return ret;
}

} // namespace turi
