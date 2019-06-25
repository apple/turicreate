/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <model_server/lib/variant.hpp>
#include <boost/filesystem.hpp>
#include <core/logging/logger.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/globals/globals.hpp>
#include <core/export.hpp>

namespace turi {

EXPORT std::string GLOBALS_MAIN_PROCESS_PATH;
EXPORT std::string GLOBALS_PYTHON_EXECUTABLE;

REGISTER_GLOBAL(std::string, GLOBALS_MAIN_PROCESS_PATH, false);
REGISTER_GLOBAL(std::string, GLOBALS_PYTHON_EXECUTABLE, true);

namespace globals {

template <typename T>
struct value_and_value_check {
  value_and_value_check() = default;
  value_and_value_check(const value_and_value_check<T>&) = default;
  value_and_value_check(value_and_value_check<T>&&) = default;
  value_and_value_check& operator=(const value_and_value_check<T>&) = default;
  value_and_value_check& operator=(value_and_value_check<T>&&) = default;

  value_and_value_check(T* value, std::function<bool(T)> value_check):
      value(value), value_check(value_check) { }

  T* value = NULL;
  std::function<bool(T)> value_check;

  bool perform_check(const T& new_value) const {
    if (!value_check) return true;
    else return value_check(new_value);
  }

  T get_value() const {
    if (value) return *value;
    else return T();
  }

  T* get_ptr() const {
    return value;
  }

  bool set_value(const T& new_value) {
    if (!perform_check(new_value) || value == NULL) return false;
    (*value) = new_value;
    return true;
  }
};


struct get_pointer_visitor: public boost::static_visitor<void*> {
  template <typename Value>
  void* operator()(const Value& val) const {
    return (void*)(val.get_ptr());
  }
};

struct get_value_visitor: public boost::static_visitor<flexible_type> {
  template <typename Value>
  flexible_type operator()(const Value& val) const {
    return flexible_type(val.get_value());
  }
};

struct set_value_visitor: public boost::static_visitor<bool> {
  flexible_type new_value;
  bool operator()(value_and_value_check<double>& val) const {
    if (new_value.get_type() == flex_type_enum::INTEGER ||
        new_value.get_type() == flex_type_enum::FLOAT) {
      return val.set_value((double)new_value);
    } else {
      return false;
    }
  }
  bool operator()(value_and_value_check<int64_t>& val) const {
    if (new_value.get_type() == flex_type_enum::INTEGER ||
        new_value.get_type() == flex_type_enum::FLOAT) {
      return val.set_value((int64_t)new_value);
    } else {
      return false;
    }
  }
  bool operator()(value_and_value_check<std::string>& val) const {
    if (new_value.get_type() == flex_type_enum::STRING) {
      return val.set_value((std::string)new_value);
    } else {
      return false;
    }
  }
};


struct set_value_from_string_visitor: public boost::static_visitor<bool> {
  std::string new_value;
  bool operator()(value_and_value_check<double>& val) const {
    try {
      return val.set_value(std::stod(new_value));
    } catch (std::string e) {
      logstream(LOG_ERROR) << e << std::endl;
    } catch (...) {
      logstream(LOG_ERROR) << "Unknown error setting double value " << std::endl;
    }
    return false;
  }
  bool operator()(value_and_value_check<int64_t>& val) const {
    try {
      return val.set_value(std::stoll(new_value));
    } catch (std::string e) {
      logstream(LOG_ERROR) << e << std::endl;
    } catch (...) {
      logstream(LOG_ERROR) << "Unknown error setting int value " << std::endl;
    }
    return false;
  }
  bool operator()(value_and_value_check<std::string>& val) const {
    try {
      return val.set_value(new_value);
    } catch (std::string e) {
      logstream(LOG_ERROR) << e << std::endl;
    } catch (...) {
      logstream(LOG_ERROR) << "Unknown error setting string value " << std::endl;
    }
    return false;
  }
};

struct global_value {
  std::string name;
  boost::variant<value_and_value_check<double>,
                 value_and_value_check<int64_t>,
                 value_and_value_check<std::string>> value;
  bool runtime_modifiable;
};

std::vector<global_value>& get_global_registry() {
  static std::vector<global_value> global_registry;
  return global_registry;
}

std::map<std::string, size_t>& get_global_registry_map() {
  static std::map<std::string, size_t> global_registry_map;
  return global_registry_map;
}

register_global<double>::register_global(std::string name,
                             double* value,
                             bool runtime_modifiable,
                             std::function<bool(double)> value_check) {
  if (get_global_registry_map().count(name)) {
    logstream(LOG_INFO) << "Configuration variable " << name << " already registered" << std::endl;
    return;
  }
  get_global_registry_map()[name] = get_global_registry().size();
  get_global_registry().push_back(
      global_value{name,
                   value_and_value_check<double>{value, value_check},
                   runtime_modifiable});
  if (runtime_modifiable) {
    logstream(LOG_INFO) << "Registering runtime modifiable configuration variable " << name
                        << " = " << (*value) << " (double)" << std::endl;
  } else {
    logstream(LOG_INFO) << "Registering environment modifiable configuration variable " << name
                        << " = " << (*value) << " (double)" << std::endl;
  }
}


register_global<int64_t>::register_global(std::string name,
                             int64_t* value,
                             bool runtime_modifiable,
                             std::function<bool(int64_t)> value_check) {
  if (get_global_registry_map().count(name)) {
    logstream(LOG_INFO) << "Configuration variable " << name << " already registered" << std::endl;
    return;
  }
  get_global_registry_map()[name] = get_global_registry().size();
  get_global_registry().push_back(
      global_value{name,
                   value_and_value_check<int64_t>{value, value_check},
                   runtime_modifiable});
  if (runtime_modifiable) {
    logstream(LOG_INFO) << "Registering runtime modifiable configuration variable " << name
                        << " = " << (*value) << " (int64_t)" << std::endl;
  } else {
    logstream(LOG_INFO) << "Registering environment modifiable configuration variable " << name
                        << " = " << (*value) << " (int64_t)" << std::endl;
  }
}

register_global<std::string>::register_global(std::string name,
                             std::string* value,
                             bool runtime_modifiable,
                             std::function<bool(std::string)> value_check) {
  if (get_global_registry_map().count(name)) {
    logstream(LOG_INFO) << "Configuration variable " << name << " already registered" << std::endl;
    size_t idx = get_global_registry_map()[name];
    void* ptr = boost::apply_visitor(get_pointer_visitor(), get_global_registry()[idx].value);
    if ((void*)ptr != (void*)value) {
      logstream(LOG_WARNING) << "Different global variable pointer detected" << std::endl;
    }
    return;
  }
  get_global_registry_map()[name] = get_global_registry().size();
  get_global_registry().push_back(
      global_value{name,
                   value_and_value_check<std::string>{value, value_check},
                   runtime_modifiable});
  if (runtime_modifiable) {
    logstream(LOG_INFO) << "Registering runtime modifiable configuration variable " << name
                        << " = " << (*value) << " (string)" << std::endl;
  } else {
    logstream(LOG_INFO) << "Registering environment modifiable configuration variable " << name
                        << " = " << (*value) << " (string)" << std::endl;
  }
}


std::vector<std::pair<std::string, flexible_type> > list_globals(bool runtime_modifiable) {
  std::vector<std::pair<std::string, flexible_type> > ret;
  for (auto& i: get_global_registry()) {
    if (i.runtime_modifiable == runtime_modifiable) {
      ret.push_back({i.name, boost::apply_visitor(get_value_visitor(), i.value)});
    }
  }
  return ret;
}

flexible_type get_global(std::string name) {
  if (get_global_registry_map().count(name) == 0) return FLEX_UNDEFINED;
  size_t idx = get_global_registry_map()[name];
  return boost::apply_visitor(get_value_visitor(), get_global_registry()[idx].value);
}

bool set_global_impl(std::string name, flexible_type val) {
  size_t idx = get_global_registry_map()[name];
  set_value_visitor visitor;
  visitor.new_value = val;
  return boost::apply_visitor(visitor, get_global_registry()[idx].value);
}

set_global_error_codes set_global(std::string name, flexible_type val) {
  if (get_global_registry_map().count(name) == 0) {
    logstream(LOG_INFO) << "Unable to change value of " << name << " to " << val
                        << ". No such configuration variable." << std::endl;
    return set_global_error_codes::NO_NAME;
  }
  size_t idx = get_global_registry_map()[name];
  if (get_global_registry()[idx].runtime_modifiable == false) {
    logstream(LOG_INFO) << "Unable to change value of " << name << " to " << val
                        << ". Variable is not runtime modifiable." << std::endl;
    return set_global_error_codes::NOT_RUNTIME_MODIFIABLE;
  }
  if (set_global_impl(name, val) == false) {
    logstream(LOG_INFO) << "Unable to change value of " << name << " to " << val
                        << ". Invalid value." << std::endl;
    return set_global_error_codes::INVALID_VAL;
  }
  return set_global_error_codes::SUCCESS;
}

void initialize_globals_from_environment(std::string root_path) {
  for (auto& i: get_global_registry()) {
    std::string envname = i.name;
    boost::optional<std::string> envval = getenv_str(envname.c_str());
    if (envval) {
      set_value_from_string_visitor visitor;
      visitor.new_value = *envval;
      if (i.value.apply_visitor(visitor)) {
        logstream(LOG_INFO) << "Setting configuration variable " << i.name
                            << " to " << visitor.new_value << std::endl;
      } else {
        logstream(LOG_EMPH) << "Cannot set configuration variable " << i.name
                            << " to " << visitor.new_value << std::endl;
      }
    }
  }

  // these two special variables cannot be environment overidden,
  // so set them  last
  GLOBALS_MAIN_PROCESS_PATH = root_path;
//   logstream(LOG_INFO) << "Main process path: " << GLOBALS_MAIN_PROCESS_PATH << std::endl;
}

} // globals
} // turicreate
