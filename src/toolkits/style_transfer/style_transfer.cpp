/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/style_transfer/style_transfer.hpp>

namespace turi {
namespace style_transfer {

namespace {

constexpr size_t STYLE_TRANSFER_VERSION = 1;

}

void style_transfer::init_options(
    const std::map<std::string, flexible_type>& opts) {}

size_t style_transfer::get_version() const { return STYLE_TRANSFER_VERSION; }

void style_transfer::save_impl(oarchive& oarc) const {
  variant_deep_save(state, oarc);
  oarc << m_transformer_spec->export_params_view();
}

void style_transfer::load_version(iarchive& iarc, size_t version) {
  // variant_deep_load(state, iarc);

  // float_array_map nn_params;
  // iarc >> nn_params;

  // init_resnet(m_transformer_spec,
  // variant_get_value<size_t>(state.at("num_classes")));

  // m_transformer_spec->update_params(nn_params);
}

// TODO: define nn_spec
// TODO: define state
// TODO: figure out what anchor_boxes is
// nn_spec_.reset(new model_spec);
//
// TODO: style_transfer serialization
//
// _load_version(iarc, version, *nn_spec_, state, anchor_boxes());

// '_model': transformer,
// '_training_time_as_string': _seconds_as_string(training_time),
// 'batch_size': batch_size,
// 'num_styles': num_styles,
// 'model': model,
// 'input_image_shape': input_shape,
// 'styles': style_sframe,
// 'num_content_images': len(content_dataset),
// 'training_time': training_time,
// 'max_iterations': max_iterations,
// 'training_iterations': max_iterations,
// 'training_epochs': content_images_loader.cur_epoch,
// 'style_feature': style_feature,
// 'content_feature': content_feature,
// "_index_column": "style",
// 'training_loss': smoothed_loss,

// params = {
//         'batch_size': batch_size,
//         'vgg16_content_loss_layer': 2,  # conv3_3 layer
//         'lr': 0.001,
//         'content_loss_mult': 1.0,
//         'style_loss_mult': [1e-4, 1e-4, 1e-4, 1e-4],  # conv 1-4 layers
//         'finetune_all_params': True,
//         'pretrained_weights': False,
//         'print_loss_breakdown': False,
//         'input_shape': (256, 256),

}  // namespace style_transfer
}  // namespace turi