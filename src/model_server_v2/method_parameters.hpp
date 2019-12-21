/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_METHOD_PARAMETERS_HPP_
#define TURI_METHOD_PARAMETERS_HPP_

#include <model_server/lib/variant.hpp>
#include <string>
#include <array>

namespace turi {
namespace v2 {


/** Struct to hold information about the user specified parameter of a method.
 *   
 *  Includes information about a possible default value.
 */
struct Parameter {

  Parameter() {}

  // Allow implicit cast from string here..
  Parameter(const std::string& n) : name(n) {}
  Parameter(std::string&& n) : name(std::move(n)) {}
 
  // Specify parameter with a default value
  Parameter(const std::string& n, const variant_type& v)
  : name(n), has_default(true), default_value(v) 
  {}

  Parameter(std::string&& n, variant_type&& v)
  : name(std::move(n)), has_default(true), default_value(std::move(v)) 
  {}


  // TODO: expand this out into a proper container class.


  // Name 
  std::string name;

  // Optional default value
  bool has_default = false;
  variant_type default_value; 
};



template <typename... FuncParams>
void validate_parameter_list(const std::vector<Parameter>& params) {

  // Validates that the parameter list works with the given types of 
  // the function. 
  if(sizeof...(FuncParams) != params.size()) { 
    throw std::invalid_argument("Mismatch in number of specified parameters."); 
  }

  // TODO: validate uniqueness of names.
  // TODO: validate defaults can be cast to proper types. 
}


////////////////////////////////////////////////////////////////////////////////

// How the arguments are bundled up and packaged.
struct argument_pack {
  std::vector<variant_type> ordered_arguments; 
  variant_map_type named_arguments; 
};

/** Method for resolving incoming arguments to a method. 
 * 
 */
template <int n>
void resolve_method_arguments(std::array<const variant_type*, n>& arg_v,
  const std::vector<Parameter>& parameter_list, 
  const argument_pack& args) { 

  size_t n_ordered = args.ordered_arguments.size();
  for(size_t i = 0; i < n_ordered; ++i) { 
    arg_v[i] = &args.ordered_arguments[i];
  }

  // TODO: check if more ordered arguments given than are 
  // possible here.
  size_t used_counter = n_ordered;
  for(size_t i = n_ordered; i < n; ++i) {
    auto it = args.named_arguments.find(parameter_list[i].name);
    if(it == args.named_arguments.end()) {
      if(parameter_list[i].has_default) { 
        arg_v[i] = &(parameter_list[i].default_value);
      } else {
        // TODO: intelligent error message.
        throw std::string("Missing argument.");
      }
    } else {
      arg_v[i] = &(it->second);
      ++used_counter; 
    }
  }

  // TODO: check that all the arguments have been used up.  If not,
  // generate a good error message.
}

}
}

#endif
