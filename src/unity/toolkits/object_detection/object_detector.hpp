/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
#define TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_

#include <functional>
#include <map>
#include <memory>

#include <table_printer/table_printer.hpp>
#include <unity/lib/extensions/ml_model.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/neural_net/cnn_module.hpp>
#include <unity/toolkits/neural_net/image_augmentation.hpp>
#include <unity/toolkits/neural_net/model_spec.hpp>
#include <unity/toolkits/object_detection/od_data_iterator.hpp>

namespace turi {
namespace object_detection {

class EXPORT object_detector: public ml_model_base {
 public:

  // ml_model_base interface

  void init_options(const std::map<std::string,
                    flexible_type>& options) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  void train(gl_sframe data, std::string annotations_column_name,
             std::string image_column_name,
             std::map<std::string, flexible_type> options);

  // Register with Unity server

  BEGIN_CLASS_MEMBER_REGISTRATION("object_detector")

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", object_detector::get_value_from_state, "field");

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::train, "data",
                                 "annotations_column_name",
                                 "image_column_name", "options");
  REGISTER_CLASS_MEMBER_DOCSTRING(
      object_detector::train,
      "\n"
      "Options\n"
      "-------\n"
      "mlmodel_path : string\n"
      "    Path to the CoreML specification with the pre-trained model parameters.\n"
      "batch_size: int\n"
      "    The number of images per training iteration. If 0, then it will be\n"
      "    automatically determined based on resource availability.\n"
      "max_iterations : int\n"
      "    The number of training iterations. If 0, then it will be automatically\n"
      "    be determined based on the amount of data you provide.\n"
  );
  // TODO: Addition training options: batch_size, max_iterations, etc.

  // TODO: Remainder of interface: predict, export_to_coreml, etc.

  END_CLASS_MEMBER_REGISTRATION

 protected:

  // Override points allowing subclasses to inject dependencies

  // Factory for data_iterator
  virtual std::unique_ptr<data_iterator> create_iterator(
      gl_sframe data, std::string annotations_column_name,
      std::string image_column_name) const;

  // Factory for image_augmenter
  virtual std::unique_ptr<neural_net::image_augmenter> create_augmenter(
      const neural_net::image_augmenter::options& opts) const;

  // Returns the initial neural network to train (represented by its CoreML
  // spec), given the path to a mlmodel file containing the pretrained weights.
  virtual std::unique_ptr<neural_net::model_spec> init_model(
      const std::string& pretrained_mlmodel_path) const;

  // Factory for cnn_module
  virtual std::unique_ptr<neural_net::cnn_module> create_cnn_module(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const neural_net::float_array_map& config,
      const neural_net::float_array_map& weights) const;


  // Support for iterative training.
  // TODO: Expose via forthcoming C-API checkpointing mechanism?

  void init_train(gl_sframe data, std::string annotations_column_name,
                  std::string image_column_name,
                  std::map<std::string, flexible_type> options);
  void perform_training_iteration();

 private:
  neural_net::shared_float_array prepare_label_batch(
      std::vector<std::vector<neural_net::image_annotation>> annotations_batch)
      const;
  flex_int get_max_iterations() const;
  flex_int get_training_iterations() const;

  // Waits until the number of pending patches is at most `max_pending`.
  void wait_for_training_batches(size_t max_pending = 0);

  // Primary representation for the trained model.
  std::unique_ptr<neural_net::model_spec> nn_spec_;

  // Primary dependencies for training. These should be nonnull while training
  // is in progress.
  std::unique_ptr<data_iterator> training_data_iterator_;
  std::unique_ptr<neural_net::image_augmenter> training_data_augmenter_;
  std::unique_ptr<neural_net::cnn_module> training_module_;

  // Nonnull while training is in progress, if progress printing is enabled.
  std::unique_ptr<table_printer> training_table_printer_;

  // Map from iteration index to the loss future.
  std::map<size_t, neural_net::deferred_float_array> pending_training_batches_;
};

}  // object_detection
}  // turi 

#endif  // TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
