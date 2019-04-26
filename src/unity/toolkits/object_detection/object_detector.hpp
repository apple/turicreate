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
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <unity/toolkits/neural_net/compute_context.hpp>
#include <unity/toolkits/neural_net/image_augmentation.hpp>
#include <unity/toolkits/neural_net/model_backend.hpp>
#include <unity/toolkits/neural_net/model_spec.hpp>
#include <unity/toolkits/object_detection/od_data_iterator.hpp>

namespace turi {
namespace object_detection {

class EXPORT object_detector: public ml_model_base {
 public:

  // ml_model_base interface

  void init_options(const std::map<std::string, flexible_type>& opts) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  void train(gl_sframe data, std::string annotations_column_name,
             std::string image_column_name, variant_type validation_data,
             std::map<std::string, flexible_type> opts);
  variant_map_type evaluate(gl_sframe data, std::string metric,
                            std::map<std::string, flexible_type> opts);
  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml(
      std::string filename, std::map<std::string, flexible_type> opts);

  // Register with Unity server

  BEGIN_CLASS_MEMBER_REGISTRATION("object_detector")

  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::train, "data",
                                 "annotations_column_name",
                                 "image_column_name", "validation_data",
                                 "options");
  register_defaults("train",
                    {{"validation_data", to_variant(gl_sframe())},
                     {"options",
                      to_variant(std::map<std::string, flexible_type>())}});
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

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::evaluate, "data", "metric",
                                 "options");
  register_defaults("evaluate",
      {{"metric", std::string("auto")},
       {"options", to_variant(std::map<std::string, flexible_type>())}});

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::export_to_coreml, "filename",
    "options");
  register_defaults("export_to_coreml", {{"options", to_variant(std::map<std::string, flexible_type>())}});

  REGISTER_CLASS_MEMBER_DOCSTRING(
      object_detector::export_to_coreml,
      "\n"
      "Options\n"
      "-------\n"
      "include_non_maximum_suppression : bool\n"
      "    A boolean value \"True\" or \"False\" to indicate the use of Non Maximum Suppression.\n"
      "iou_threshold: double\n"
      "    The allowable IOU overlap between bounding box detections for the same object.\n"
      "    If no value is specified, a default value of 0.45 is used.\n"
      "confidence_threshold : double\n"
      "    The minimum required object confidence score per bounding box detection.\n"
      "    All bounding box detections with object confidence score lower than\n"
      "    the confidence_threshold are eliminiated. If no value is specified,\n"
      "    a default value of 0.25 is used.\n"
  );

  // TODO: Remainder of interface: predict, etc.

  END_CLASS_MEMBER_REGISTRATION

 protected:

  // Override points allowing subclasses to inject dependencies

  // Factory for data_iterator
  virtual std::unique_ptr<data_iterator> create_iterator(
      gl_sframe data, std::vector<std::string> class_labels, bool repeat) const;

  // Factory for compute_context
  virtual
  std::unique_ptr<neural_net::compute_context> create_compute_context() const;

  // Returns the initial neural network to train (represented by its CoreML
  // spec), given the path to a mlmodel file containing the pretrained weights.
  virtual std::unique_ptr<neural_net::model_spec> init_model(
      const std::string& pretrained_mlmodel_path) const;


  // Support for iterative training.
  // TODO: Expose via forthcoming C-API checkpointing mechanism?

  virtual void init_train(gl_sframe data, std::string annotations_column_name,
                          std::string image_column_name,
                          std::map<std::string, flexible_type> opts);
  virtual void perform_training_iteration();

  virtual variant_map_type perform_evaluation(gl_sframe data,
                                              std::string metric);

  // Utility code

  template <typename T>
  T read_state(const std::string& key) const {
    return variant_get_value<T>(get_state().at(key));
  }

 private:

  neural_net::float_array_map get_model_params() const;

  neural_net::shared_float_array prepare_label_batch(
      std::vector<std::vector<neural_net::image_annotation>> annotations_batch)
      const;
  flex_int get_max_iterations() const;
  flex_int get_training_iterations() const;

  // Waits until the number of pending patches is at most `max_pending`.
  void wait_for_training_batches(size_t max_pending = 0);

  // Computes and records training/validation metrics.
  void update_model_metrics(gl_sframe data, gl_sframe validation_data);

  // Primary representation for the trained model.
  std::unique_ptr<neural_net::model_spec> nn_spec_;

  // Primary dependencies for training. These should be nonnull while training
  // is in progress.
  std::unique_ptr<neural_net::compute_context> training_compute_context_;
  std::unique_ptr<data_iterator> training_data_iterator_;
  std::unique_ptr<neural_net::image_augmenter> training_data_augmenter_;
  std::unique_ptr<neural_net::model_backend> training_model_;

  // Nonnull while training is in progress, if progress printing is enabled.
  std::unique_ptr<table_printer> training_table_printer_;

  // Map from iteration index to the loss future.
  std::map<size_t, neural_net::shared_float_array> pending_training_batches_;
};

}  // object_detection
}  // turi 

#endif  // TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
