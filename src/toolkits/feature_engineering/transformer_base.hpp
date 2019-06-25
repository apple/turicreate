/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TRANSFORMER_BASE_H
#define TURI_TRANSFORMER_BASE_H

#include <core/data/sframe/gl_sframe.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <core/export.hpp>

namespace turi{
namespace sdk_model {
namespace feature_engineering {

/**
 * transformer_base model interface.
 * ---------------------------------------
 *
 *  Base class for handling feature engineering transformers. This class is meant
 *  to be a guide to aid model writing for feature engineering modules.
 *
 *  Each C++ toolkit contains the following:
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
 * *) init_transformer : Initializer the transformer i.e this is the same as
 *                       __init__ from the python side.
 *
 * *) fit  : Fit the transformer with the data (SFrame)
 *
 * *) transform : Transform the data (SFrame) to another SFrame after the model
 *                has been fit.
 *
 * *) init_options : Initialize the options manager.
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
 */
class EXPORT transformer_base : public ml_model_base {
 public:

  static constexpr size_t TRANSFORMER_BASE_VERSION = 0;

  public:

  virtual ~transformer_base() {}

  /**
   * Returns the current model version
   */
  virtual size_t get_version() const = 0;

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const = 0;

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  virtual void load_version(iarchive& iarc, size_t version) = 0;


  /**
   * Set one of the options in the algorithm. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  virtual void init_options(const std::map<std::string,
                                            flexible_type>& _options) = 0;

  /**
   * Init the transformer and return an instantiated object.
   *
   * \param[in] options (**kwargs from python)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "__init__" in python.
   *
   */
  virtual void init_transformer(const std::map<std::string,
                                              flexible_type>& _options) = 0;
  /**
   * Fit the transformer and make it ready for transformations.
   *
   * \param[in] data  (SFrame of data)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "fit" in python.
   *
   */
  virtual void fit(gl_sframe data) = 0;

  /**
   * Transform the given data.
   *
   * \param[in] data  (SFrame of data)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "transform" in python.
   *
   */
  virtual gl_sframe transform(gl_sframe data) = 0;

  /**
   * Fit and transform the given data. Intended as an optimization because
   * fit and transform are usually always called together. The default
   * implementaiton calls fit and then transform.
   *
   * \param[in] data  (SFrame of data)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "fit_transform" in python.
   *
   */
  gl_sframe fit_transform(gl_sframe data) {
     fit(data);
     return transform(data);
  }

};

} // feature_engineering
} // sdk_model
} // turicreate
#endif
