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
#include <queue>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <toolkits/object_detection/od_data_iterator.hpp>
#include <toolkits/object_detection/od_model_trainer.hpp>

namespace turi {
namespace object_detection {

class EXPORT object_detector: public ml_model_base {
 public:
  object_detector() = default;

  // ml_model_base interface

  void init_options(const std::map<std::string, flexible_type>& opts) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  void train(gl_sframe data, std::string annotations_column_name,
             std::string image_column_name, variant_type validation_data,
             std::map<std::string, flexible_type> opts);
  variant_type evaluate(gl_sframe data, std::string metric,
                        std::string output_type,
                        std::map<std::string, flexible_type> opts);
  variant_type predict(variant_type data,
                       std::map<std::string, flexible_type> opts);
  virtual std::shared_ptr<coreml::MLModelWrapper> export_to_coreml(
      std::string filename, std::string short_description,
      std::map<std::string, flexible_type> additional_user_defined,
      std::map<std::string, flexible_type> opts);
  void import_from_custom_model(variant_map_type model_data, size_t version);

  // Support for iterative training.
  virtual void init_training(gl_sframe data,
                             std::string annotations_column_name,
                             std::string image_column_name,
                             variant_type validation_data,
                             std::map<std::string, flexible_type> opts);
  virtual void resume_training(gl_sframe data, variant_type validation_data);
  virtual void iterate_training();
  virtual void synchronize_training();
  virtual void finalize_training(bool compute_final_metrics);

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

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::init_training, "data",
                                 "annotations_column_name", "image_column_name",
                                 "validation_data", "options");
  register_defaults("init_training",
                    {{"validation_data", to_variant(gl_sframe())},
                     {"options",
                      to_variant(std::map<std::string, flexible_type>())}});

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::resume_training, "data",
                                 "validation_data");
  register_defaults("resume_training",
                    {{"validation_data", to_variant(gl_sframe())}});

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::iterate_training);
  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::synchronize_training);
  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::finalize_training,
                                 "compute_final_metrics");
  register_defaults("finalize_training", {{"compute_final_metrics", true}});

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::evaluate, "data", "metric",
                                 "output_type", "options");
  register_defaults("evaluate",
                    {
                        {"metric", std::string("auto")},
                        {"output_type", std::string("dict")},
                        {"options",
                         to_variant(std::map<std::string, flexible_type>())},
                    });

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::predict, "data", "options");
  register_defaults("predict",{});

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::export_to_coreml, "filename",
    "short_description", "additional_user_defined", "options");
  register_defaults("export_to_coreml",
         {{"short_description", ""},
          {"additional_user_defined", to_variant(std::map<std::string, flexible_type>())},
          {"options", to_variant(std::map<std::string, flexible_type>())}});

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

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::import_from_custom_model,
                                 "model_data", "version");

  // TODO: Remainder of interface: predict, etc.

  END_CLASS_MEMBER_REGISTRATION

 protected:
  // Constructor allowing tests to set the initial state of this class.
  object_detector(std::map<std::string, variant_type> initial_state,
                  neural_net::float_array_map initial_weights) {
    load(std::move(initial_state), std::move(initial_weights));
  }

  // Resets the internal state. Used by deserialization code and unit tests.
  void load(std::map<std::string, variant_type> state,
            neural_net::float_array_map weights);

  // Assumes state already loaded.
  virtual std::unique_ptr<Checkpoint> load_checkpoint(
      neural_net::float_array_map weights) const;

  // Synchronously loads weights from the backend if necessary.
  const Checkpoint& read_checkpoint() const;

  // Override points allowing subclasses to inject dependencies

  // Factory for data_iterator
  virtual std::unique_ptr<data_iterator> create_iterator(
      data_iterator::parameters iterator_params) const;

  std::unique_ptr<data_iterator> create_iterator(
      gl_sframe data, std::vector<std::string> class_labels, bool repeat,
      bool is_training) const;

  // Factory for compute_context
  virtual
  std::unique_ptr<neural_net::compute_context> create_compute_context() const;

  // Factories for ModelTrainer
  virtual std::unique_ptr<ModelTrainer> create_trainer(
      const Config& config, const std::string& pretrained_model_path,
      std::unique_ptr<neural_net::compute_context> context) const;
  virtual std::unique_ptr<ModelTrainer> create_inference_trainer(
      const Checkpoint& checkpoint,
      std::unique_ptr<neural_net::compute_context> context) const;

  // Establishes training pipelines from the backend.
  void connect_trainer(std::unique_ptr<ModelTrainer> trainer,
                       std::unique_ptr<data_iterator> iterator, int batch_size);

  virtual std::vector<neural_net::image_annotation> convert_yolo_to_annotations(
      const neural_net::float_array& yolo_map,
      const std::vector<std::pair<float, float>>& anchor_boxes,
      float min_confidence);

  virtual variant_type perform_evaluation(gl_sframe data, std::string metric,
                                          std::string output_type,
                                          float confidence_threshold,
                                          float iou_threshold);

  void perform_predict(
      gl_sframe data,
      std::function<void(const std::vector<neural_net::image_annotation>&,
                         const std::vector<neural_net::image_annotation>&,
                         const std::pair<float, float>&)>
          consumer,
      float confidence_threshold, float iou_threshold);

  // Utility code

  template <typename T>
  T read_state(const std::string& key) const {
    return variant_get_value<T>(get_state().at(key));
  }

 private:
  neural_net::float_array_map strip_fwd(
      const neural_net::float_array_map& params) const;

  flex_int get_max_iterations() const;
  flex_int get_training_iterations() const;
  flex_int get_num_classes() const;

  static variant_type convert_map_to_types(const variant_map_type& result_map,
                                           const std::string& output_type,
                                           const flex_list& class_labels);
  static gl_sframe convert_types_to_sframe(const variant_type& data,
                                           const std::string& column_name);

  // Sets certain user options heuristically (from the data).
  void infer_derived_options(neural_net::compute_context* context,
                             data_iterator* iterator);

  // Waits until the number of pending patches is at most `max_pending`.
  void wait_for_training_batches(size_t max_pending = 0);

  // Computes and records training/validation metrics.
  void update_model_metrics(gl_sframe data, gl_sframe validation_data);

  // Primary representation for the trained model. Can be null if the model has
  // been updated since the last checkpoint.
  mutable std::unique_ptr<Checkpoint> checkpoint_;

  // Primary dependencies for training. These should be nonnull while training
  // is in progress.
  gl_sframe training_data_;  // TODO: Avoid storing gl_sframe AND data_iterator.
  gl_sframe validation_data_;
  std::shared_ptr<neural_net::FuturesStream<TrainingOutputBatch>>
      training_futures_;
  std::shared_ptr<neural_net::FuturesStream<std::unique_ptr<Checkpoint>>>
      checkpoint_futures_;

  // Nonnull while training is in progress, if progress printing is enabled.
  std::unique_ptr<table_printer> training_table_printer_;

  std::queue<std::future<std::unique_ptr<TrainingOutputBatch>>>
      pending_training_batches_;

  struct inference_batch : neural_net::image_augmenter::result {
    std::vector<std::pair<float, float>> image_dimensions_batch;
  };
};

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
