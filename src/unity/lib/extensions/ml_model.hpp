/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_ML_MODEL_HPP
#define TURI_UNITY_ML_MODEL_HPP

#include <unity/lib/variant.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_class_macros.hpp>

#include <unity/lib/extensions/model_base.hpp>
#include <unity/lib/extensions/option_manager.hpp>

#include <export.hpp>

namespace turi {

/**
 * ml_model model base class.
 * ---------------------------------------
 *
 *  Base class for handling machine learning models. This class is meant to
 *  be a guide to aid model writing and not a hard and fast rule of how the
 *  code must be structured.
 *
 *  Each machine learning C++ toolkit contains the following:
 *
 *  *) state: This is the key-value map that stores the "model" attributes.
 *            The value is of type "variant_type" which is fully interfaced
 *            with python. You can add basic types, vectors, SFrames etc.
 *
 *
 *  *) options: Option manager which keeps track of default options, current
 *              options, option ranges, type etc. This must be initialized only
 *              once in the set_options() function.
 *
 *
 * Functions that should always be implemented. Here are some notes about
 * each of these functions that may help guide you in writing your model.
 *
 * *) name: Get the name of this model. You might thinks that this is silly but
 *          the name holds the key to everything. The unity_server can construct
 *          model_base objects and they can be cast to a model of this type.
 *          The name determine how the casting happens. The init_models()
 *          function in unity_server.cpp will give you an idea of how
 *          this interface happens.
 *
 * *) save: Save the model with the turicreate iarc. Turi is a server-client
 *          module. DO NOT SAVE ANYTHING in the client side. Make sure that
 *          everything is in the server side. For example: You might be tempted
 *          do keep options that the user provides into the server side but
 *          DO NOT do that because save and load will break things for you!
 *
 * *) load: Load the model with the turicreate oarc.
 *
 * *) version: A get version for this model
 *
 *
 */
class EXPORT ml_model_base: public model_base {

 public:

  static constexpr size_t ML_MODEL_BASE_VERSION = 0;

  // virtual destructor
  inline virtual ~ml_model_base() { }

  /**
   * Set one of the options in the algorithm. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  virtual void init_options(const std::map<std::string,flexible_type>& _options) {};


  /**
   * Methods with already meaningful default implementations.
   * -------------------------------------------------------------------------
   */


  /**
   * Lists all the keys accessible in the "model" map.
   *
   * \returns List of keys in the model map.
   * \ref model_base for details.
   *
   * Python side interface
   * ------------------------
   *
   * This is the function that the list_fields should call in python.
   */
  std::vector<std::string> list_fields();


  /**
   * Returns the value of a particular key from the state.
   *
   * \returns Value of a key
   * \ref model_base for details.
   *
   * Python side interface
   * ------------------------
   *
   * From the python side, this is interfaced with the get() function or the
   * [] operator in python.
   *
   */
  const variant_type& get_value_from_state(std::string key);


  /**
   * Get current options.
   *
   * \returns Dictionary containing current options.
   *
   * Python side interface
   * ------------------------
   *  Interfaces with the get_current_options function in the Python side.
   */
  const std::map<std::string, flexible_type>& get_current_options() const;

  /**
   * Get default options.
   *
   * \returns Dictionary with default options.
   *
   * Python side interface
   * ------------------------
   *  Interfaces with the get_default_options function in the Python side.
  */
  std::map<std::string, flexible_type> get_default_options() const;

  /**
   * Returns the value of an option. Throws an error if the option does not
   * exist.
   *
   * \param[in] name Name of the option to get.
   */
  const flexible_type& get_option_value(const std::string& name) const;

  /**
   * Get model.
   *
   * \returns Model map.
   */
  const std::map<std::string, variant_type>& get_state() const;

  /**
   * Is this model trained.
   *
   * \returns True if already trained.
   */
  bool is_trained() const;

  /**
   * Set one of the options in the algorithm.
   *
   * The value are checked with the requirements given by the option
   * instance.
   *
   * \param[in] name  Name of the option.
   * \param[in] value Value for the option.
   */
  void set_options(const std::map<std::string, flexible_type>& _options);

  /**
   * Append the key value store of the model.
   *
   * \param[in] dict Options (Key-Valye pairs) to set
   */
  void add_or_update_state(const std::map<std::string, variant_type>& dict);

  /** Returns the option information struct for each of the set
   *  parameters.
   */
  const std::vector<option_handling::option_info>& get_option_info() const;

  // Code to perform the registration for the rest of the tools. 
  BEGIN_BASE_CLASS_MEMBER_REGISTRATION()

  IMPORT_BASE_CLASS_REGISTRATION(model_base);

  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get_value",
                                       ml_model_base::get_value_from_state,
                                       "field");
  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::get_option_value);
  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::is_trained);
  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::get_default_options);
  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::get_state);
  REGISTER_CLASS_MEMBER_FUNCTION(ml_model_base::set_options);

  END_CLASS_MEMBER_REGISTRATION

 protected:

  option_manager options;                                 /* Option manager */
  std::map<std::string, variant_type> state;         /**< All things python */


};


namespace ml_model_sdk {

/**
 * Obtains the registration for the toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

/**
 * Call the default options using a registered model.
 *
 * \param[in] name  Name of the model registered in the class.
 */
std::map<std::string, variant_type> _toolkits_get_default_options(
                       std::string model_name);

}  // namespace ml_model_sdk

}  // namespace turi

#endif
