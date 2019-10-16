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
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <toolkits/style_transfer/style_transfer_data_iterator.hpp>

namespace turi {
namespace style_transfer {

class EXPORT style_transfer : public ml_model_base {
 public:
  void init_options(const std::map<std::string, flexible_type>& opts) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  void train(gl_sarray style, gl_sarray content,
             std::map<std::string, flexible_type> opts);

  BEGIN_CLASS_MEMBER_REGISTRATION("style_transfer")
  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);
  END_CLASS_MEMBER_REGISTRATION

 protected:
  virtual std::unique_ptr<data_iterator> create_iterator(
      data_iterator::parameters iterator_params) const;

  std::unique_ptr<data_iterator> create_iterator(gl_sarray style,
                                                 gl_sarray content, bool repeat,
                                                 int random_seed) const;

  virtual std::unique_ptr<neural_net::compute_context> create_compute_context()
      const;

  virtual void init_train(gl_sarray style, gl_sarray content,
                          std::map<std::string, flexible_type> opts);

  virtual void perform_training_iteration();

  template <typename T>
  T read_state(const std::string& key) const {
    return variant_get_value<T>(get_state().at(key));
  }

 private:
  std::unique_ptr<neural_net::model_spec> m_resnet_spec;
  std::unique_ptr<neural_net::model_spec> m_vgg_spec;

  std::unique_ptr<data_iterator> m_training_data_iterator;
  std::unique_ptr<neural_net::compute_context> m_training_compute_context;
  std::unique_ptr<neural_net::model_backend> m_training_model;

  flex_int get_max_iterations() const;
  flex_int get_training_iterations() const;
  flex_int get_num_classes() const;

  void infer_derived_options();
};

}  // namespace style_transfer
}  // namespace turi

#endif  // __TOOLKITS_STYLE_TRANSFER_H_