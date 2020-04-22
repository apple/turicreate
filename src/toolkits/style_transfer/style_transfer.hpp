/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TOOLKITS_STYLE_TRANSFER_H_
#define __TOOLKITS_STYLE_TRANSFER_H_

#include <memory>

#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <toolkits/coreml_export/neural_net_models_exporter.hpp>
#include <toolkits/style_transfer/st_model_trainer.hpp>
#include <toolkits/style_transfer/style_transfer_data_iterator.hpp>

namespace turi {
namespace style_transfer {

// TODO: Move these helper functions to st_model_trainer.cpp
std::vector<std::pair<flex_int, flex_image>> process_output(
    const neural_net::shared_float_array& contents, size_t index);
neural_net::float_array_map prepare_batch(const std::vector<st_example>& batch,
                                          size_t width, size_t height,
                                          bool train = true);
neural_net::float_array_map prepare_predict(const st_example& example);

class EXPORT style_transfer : public ml_model_base {
 public:
  void init_options(const std::map<std::string, flexible_type>& opts) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  virtual std::shared_ptr<coreml::MLModelWrapper> export_to_coreml(
      std::string filename, std::string short_description,
      std::map<std::string, flexible_type> additional_user_defined,
      std::map<std::string, flexible_type> opts);

  void train(gl_sarray style, gl_sarray content,
             std::map<std::string, flexible_type> opts);

  virtual void init_training(gl_sarray style, gl_sarray content,
                             std::map<std::string, flexible_type> opts);
  virtual void resume_training(gl_sarray style, gl_sarray content,
                               std::map<std::string, flexible_type> opts);

  virtual void iterate_training();
  virtual void synchronize_training();
  virtual void finalize_training();

  gl_sframe predict(variant_type data,
                    std::map<std::string, flexible_type> opts);

  gl_sframe get_styles(variant_type style_index);

  void import_from_custom_model(variant_map_type model_data, size_t version);

  BEGIN_CLASS_MEMBER_REGISTRATION("style_transfer")
  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::train, "style", "content",
                                 "opts");

  REGISTER_CLASS_MEMBER_DOCSTRING(
      style_transfer::train,
      "\n"
      "Options\n"
      "-------\n"
      "resnet_mlmodel_path : string\n"
      "    Path to the Resnet CoreML specification with the pre-trained model\n"
      "    parameters.\n"
      "vgg_mlmodel_path: string\n"
      "    Path to the VGG16 CoreML specification with the pre-trained model\n"
      "    parameters.\n"
      "num_styles: int\n"
      "    The defined number of styles for the style transfer model\n"
      "batch_size : int\n"
      "    The number of images per training iteration. If 0, then it will be\n"
      "    automatically determined based on resource availability.\n"
      "max_iterations : int\n"
      "    The number of training iterations. If 0, then it will be "
      "automatically\n"
      "    be determined based on the amount of data you provide.\n"
      "image_width : int\n"
      "    The input image width to the model\n"
      "image_height : int\n"
      "    The input image height to the model\n");

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::init_training, "style",
                                 "content", "opts");
  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::resume_training, "style",
                                 "content", "opts");
  register_defaults("resume_training",
                    {{"opts",
                      to_variant(std::map<std::string, flexible_type>())}});

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::iterate_training);
  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::synchronize_training);
  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::finalize_training);

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::export_to_coreml, "filename",
    "short_description", "additional_user_defined", "options");
  register_defaults("export_to_coreml",
         {{"short_description", ""},
          {"additional_user_defined", to_variant(std::map<std::string, flexible_type>())},
          {"options", to_variant(std::map<std::string, flexible_type>())}});

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::predict, "data", "options");

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::import_from_custom_model,
                                 "model_data", "version");

  REGISTER_CLASS_MEMBER_FUNCTION(style_transfer::get_styles, "style_index");
  register_defaults("get_styles", {{"style_index", FLEX_UNDEFINED}});

  END_CLASS_MEMBER_REGISTRATION

 protected:
  // Override points allowing subclasses to inject dependencies

  // Factory for data_iterator
  virtual std::unique_ptr<data_iterator> create_iterator(
      data_iterator::parameters iterator_params) const;

  std::unique_ptr<data_iterator> create_iterator(gl_sarray content,
                                                 gl_sarray style, bool repeat,
                                                 bool training,
                                                 int random_seed) const;

  // Factory for compute_context
  virtual std::unique_ptr<neural_net::compute_context> create_compute_context()
      const;

  // Factories for Checkpoint
  virtual std::unique_ptr<Checkpoint> load_checkpoint(
      neural_net::float_array_map weights) const;

  virtual std::unique_ptr<Checkpoint> create_checkpoint(
      Config config, const std::string& resnet_model_path) const;

  // Establishes training pipelines from the backend.
  void connect_trainer(gl_sarray style, gl_sarray content,
                       const std::string& vgg_mlmodel_path,
                       std::unique_ptr<neural_net::compute_context> context);

  void perform_predict(gl_sarray images, gl_sframe_writer& result,
                       const std::vector<int>& style_idx, bool verbose);

  // Synchronously loads weights from the backend if necessary
  const Checkpoint& read_checkpoint() const;

  Config get_config() const;

  template <typename T>
  T read_state(const std::string& key) const {
    return variant_get_value<T>(get_state().at(key));
  }

  template <typename T>
  typename std::map<std::string, T>::iterator _read_iter_opts(
      std::map<std::string, T>& opts, const std::string& key) const {
    auto iter = opts.find(key);
    if (iter == opts.end())
      log_and_throw("Expected option \"" + key + "\" not found.");
    return iter;
  }

  template <typename T>
  T read_opts(std::map<std::string, turi::variant_type>& opts,
              const std::string& key) const {
    auto iter = _read_iter_opts<turi::variant_type>(opts, key);
    return variant_get_value<T>(iter->second);
  }

  template <typename T>
  T read_opts(std::map<std::string, turi::flexible_type>& opts,
              const std::string& key) const {
    auto iter = _read_iter_opts<turi::flexible_type>(opts, key);
    return iter->second.get<T>();
  }

 private:
  // Primary representation for the trained model. Can be null if the model has
  // been updated since the last checkpoint.
  mutable std::unique_ptr<Checkpoint> checkpoint_;

  // Primary dependencies for training. These should be nonnull while training
  // is in progress.
  std::shared_ptr<ModelTrainer> model_trainer_;
  std::shared_ptr<neural_net::FuturesStream<TrainingProgress>>
      training_futures_;
  std::shared_ptr<neural_net::FuturesStream<std::unique_ptr<Checkpoint>>>
      checkpoint_futures_;

  std::unique_ptr<neural_net::model_spec> m_resnet_spec;
  std::unique_ptr<neural_net::model_spec> m_vgg_spec;

  std::unique_ptr<data_iterator> m_training_data_iterator;
  std::unique_ptr<neural_net::compute_context> m_training_compute_context;
  std::unique_ptr<neural_net::model_backend> m_training_model;

  std::unique_ptr<table_printer> training_table_printer_;

  static gl_sarray convert_types_to_sarray(const variant_type& data);

  /**
   * convert_style_indices_to_filter
   *
   * This function takes a variant type and converts it into a boolean filter.
   * The elements at the indices we want to keep are set to a value of `1`, the
   * elements we don't want to keep are set to a value of `0`.
   */
  gl_sarray convert_style_indices_to_filter(const variant_type& data);
  gl_sframe style_sframe_with_index(gl_sarray styles);

  flex_int get_max_iterations() const;
  flex_int get_training_iterations() const;
  flex_int get_num_classes() const;

  void infer_derived_options(neural_net::compute_context* context);
};

}  // namespace style_transfer
}  // namespace turi

#endif  // __TOOLKITS_STYLE_TRANSFER_H_
