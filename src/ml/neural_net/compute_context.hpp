/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_

#include <memory>

#include <core/export.hpp>
#include <core/system/exceptions/TuriException.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace neural_net {

/** A struct to define all the parameters used to create the activity classifier
 * model backend
 */
struct ac_parameters {
  /** Defines the batch size */
  int batch_size;

  /** Defines the number of features in the data */
  int num_features;

  /** Each group of this many consecutive samples from the same session are
   * assumed to have the same class label.
   */
  int prediction_window;

  /** Defines the number of classes */
  int num_classes;

  /** Each session is segmented into chunks of this many prediction windows. */
  int num_predictions_per_chunk;

  /** Setting random seed makes results reproducible. */
  int random_seed;

  /**  Set to true, when the data is used for training. */
  bool is_training;

  /** Defines the weights of the network */
  float_array_map weights;
};
/**
 * Interface for factories that produce concrete data augmentation and neural
 * network module instances, used to abstract across backend implementations and
 * hardware resources.
 */
class EXPORT compute_context {
 public:
  /** Function that yields a compute context. */
  using factory = std::function<std::unique_ptr<compute_context>()>;

  /**
   * To solve for layering/dependency issues, we allow compute_context::factory
   * values to be defined at runtime. Instantiating this class, preferably at
   * static init time, adjusts the behavior of the create() function below.
   */
  class registration {
   public:
    // Registers `factory_fn` at the given priority.
    registration(int priority, factory factory_fn, factory tf_factory_fn_, factory mlc_factory_fn_);

    // Removes the registration. In practice, simplest just not to deallocate...
    ~registration();

    int priority() const { return priority_; }
    std::unique_ptr<compute_context> create_context() const {
      return factory_fn_();
    }

    std::unique_ptr<compute_context> create_tensorflow_context() const {
      return tf_factory_fn_ ? tf_factory_fn_() : nullptr;
    }

    std::unique_ptr<compute_context> create_mlc_context() const
    {
      return mlc_factory_fn_ ? mlc_factory_fn_() : nullptr;
    }

   private:
    int priority_;
    factory factory_fn_;
    factory tf_factory_fn_;
    factory mlc_factory_fn_;
  };

  /**
   * Requests a compute_context from each registered compute_context::factory,
   * in ascending order by "priority", until one returns non-nil. Factories
   * should be registered so that this function yields a backend appropriate to
   * the current platform and hardware.
   */
  static std::unique_ptr<compute_context> create();

  static std::unique_ptr<compute_context> create_tf();

  static std::unique_ptr<compute_context> create_mlc();

  virtual ~compute_context();

  /**
   * Prints (human readable) device information.
   */
  virtual void print_training_device_info() const = 0;

  /**
   * Provides a measure of the memory resources available.
   *
   * Returns the maximum memory size in bytes that neural networks should
   * allocate, typically used to determine batch sizes (often heuristically).
   */
  virtual size_t memory_budget() const = 0;

  /**
   * Creates an object detection network.
   *
   * \todo Define a object_detector_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  virtual std::unique_ptr<model_backend> create_object_detector(int n, int c_in, int h_in, int w_in,
                                                                int c_out, int h_out, int w_out,
                                                                const float_array_map& config,
                                                                const float_array_map& weights)
  {
    throw TuriException(TuriErrorCode::NotImplemented);
  }

  /**
   * Creates an activity classification network.
   *
   * \todo Define an activity_classifier_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  virtual std::unique_ptr<model_backend> create_activity_classifier(
      const ac_parameters& ac_params) {
    throw TuriException(TuriErrorCode::NotImplemented);
  }

  /**
   * Creates a style transfer network
   *
   * \todo Define an style_transfer_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   */
  virtual std::unique_ptr<model_backend> create_style_transfer(const float_array_map& config,
                                                               const float_array_map& weights)
  {
    throw TuriException(TuriErrorCode::NotImplemented);
  }

  /**
   * Creates a drawing classification network.
   *
   * \todo Define a drawing_classifier_config struct to encapsulate these
   *       parameters in a more self-documenting and typesafe way.
   * \todo Initialize the network directly from a model_spec, in lieu of passing
   *       weights as a float_array_map.
   * \todo what args here?
   */
  virtual std::unique_ptr<model_backend> create_drawing_classifier(
      /* TODO: const float_array_map& config if needed */
      const float_array_map& weights, size_t batch_size, size_t num_classes) {
    throw TuriException(TuriErrorCode::NotImplemented);
  }

  /**
   * Creates an image augmenter.
   */
  virtual std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options& opts)
  {
    throw TuriException(TuriErrorCode::NotImplemented);
  }

  /**
   * Creates a multilevel perceptron classifier.
   */
  virtual std::unique_ptr<turi::neural_net::model_backend> create_multilayer_perceptron_classifier(
      int n, int c_in, int c_out, const std::vector<size_t>& layer_sizes,
      const turi::neural_net::float_array_map& config)
  {
    throw TuriException(TuriErrorCode::NotImplemented);
  }
};

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_COMPUTE_CONTEXT_HPP_
