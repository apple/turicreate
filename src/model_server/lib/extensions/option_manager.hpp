/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_OPTION_MANAGER_H_
#define TURI_OPTION_MANAGER_H_

#include <string>
#include <vector>
#include <map>

#include <model_server/lib/extensions/option_info.hpp>

namespace turi {

class flexible_type;

/**
 * \ingroup toolkit_util
 * A general purpose option manager class.  Functions like a
 *  std::map<std::string, flexible_type>, but permits type checking,
 *  description querying, bounds checking, checked categorical values,
 *  etc.
 */
class option_manager {
 public:

  /// Convenience overload for create_option.
  void create_real_option(const std::string& name,
                          const std::string& description,
                          flexible_type default_value,
                          double lower_bound,
                          double upper_bound,
                          bool allowed_overwrite = false);

  /// Convenience overload for create_option.
  void create_integer_option(const std::string& name,
                             const std::string& description,
                             flexible_type default_value,
                             flex_int lower_bound,
                             flex_int upper_bound,
                             bool allowed_overwrite = false);

  /// Convenience overload for create_option.
  void create_categorical_option(const std::string& name,
                                 const std::string& description,
                                 const flexible_type& default_value,
                                 const std::vector<flexible_type>& allowed_possible_values,
                                 bool allowed_overwrite = false);

  /// Convenience overload for create_option.
  void create_string_option(const std::string& name,
                            const std::string& description,
                            const flexible_type& default_value,
                            bool allowed_overwrite = false);

  /// Convenience overload for create_option.
  void create_boolean_option(const std::string& name,
                             const std::string& description,
                             bool default_value,
                             bool allowed_overwrite = false);

  /// Convenience overload for create_option.
  //  \warning: This is meant as a last resort if you cannot use any of the above
  //  options. It does not do any clever error checking.
  void create_flexible_type_option(const std::string& name,
                                   const std::string& description,
                                   const flexible_type& default_value,
                                   bool allowed_overwrite = false);

  /**  Create an option as dictated by option_handling::option_info.
   *
   *   By default, if an option of the same name exists as the one
   *   being created, an error is raised (cause it's probably a
   *   programmer typo).  If allowed_overwrite is true, than the
   *   option is overwritten.  The use case of this is when one module
   *   wraps another and has to change some of its options / defaults.
   */
  void create_option(const option_handling::option_info&, bool allowed_overwrite = false);

  /** Set one of the options.  This values is checked against the
   *  requirements given by the option instance.
   */
  void set_option(const std::string& name, const flexible_type& value);

  /** Sets the options.  These values are checked against the values
   *  in the option reference.
   */
  void set_options(const std::map<std::string, flexible_type>& options);

  /** Delete one of the options. This removes the option from
   *  options_reference_lookup_map and _current_option_values,
   *  but does not remove it from the vector options_reference.
   *  Useful for loading from older model versions with obsolete option names.
   */
  void delete_option(const std::string& name);

  /** Delete a set of options. This removes the options from
   *  options_reference_lookup_map and _current_option_values,
   *  but does not remove them from the vector options_reference.
   *  Useful for loading from older model versions with obsolete option names.
   */
  void delete_options(const std::vector<std::string>& names);

  /** Update the name of an option. Useful for loading from older model versions.
   *  If an option exists with old_name, create a new option with new_name,
   *  copy over values from the old_name, then remove the old option.
   */
  void update_option_name(const std::string& old_name, const std::string& new_name);

  /** Update the name of a set of options.
   *  Useful for loading from older model versions.
   *  If an option exists with old_name, create a new option with new_name,
   *  copy over values from the old_name, then remove the old option.
   */
  void update_option_names(const std::map<std::string, std::string>& name_map);

  /// Returns the option information struct for each of the set
  /// parameters.
  const std::vector<option_handling::option_info>& get_option_info() const;

  /// Returns a map of strings to flexible_type that give the values
  /// of all the current option values.
  const std::map<std::string, flexible_type>& current_option_values() const;

  /** Creates and returns a map of the default options, as specified
   * by the model.
   */
  std::map<std::string, flexible_type> get_default_options() const;

  /// Returns the value of the option
  const flexible_type& value(const std::string& name) const;

  /// Returns the description of the option name.
  const std::string& description(const std::string& name) const;

  /// Returns true if an option exists and false otherwise
  bool is_option(const std::string& name) const;

  /// Serialization -- save
  void save(turi::oarchive& oarc) const;

  /// Serialization -- load
  void load(turi::iarchive& iarc);

 private:

  // A stored cache of the algorithm option information
  std::map<std::string, size_t> options_reference_lookup_map;

  std::vector<option_handling::option_info> options_reference;

  std::map<std::string, flexible_type> _current_option_values;

};

}

#endif /* TURI_OPTION_MANAGER_H_ */
