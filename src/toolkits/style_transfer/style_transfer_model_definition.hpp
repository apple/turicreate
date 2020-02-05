/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TOOLKITS_STYLE_TRANSFER_MODEL_DEFINITION_H_
#define __TOOLKITS_STYLE_TRANSFER_MODEL_DEFINITION_H_

#include <memory>
#include <string>

#include <ml/neural_net/model_spec.hpp>

namespace turi {
namespace style_transfer {

std::unique_ptr<neural_net::model_spec> init_resnet(const std::string& path);
std::unique_ptr<neural_net::model_spec> init_resnet(size_t num_styles,
													int random_seed=0);
std::unique_ptr<neural_net::model_spec> init_resnet(const std::string& path,
                                                    size_t num_styles);
std::unique_ptr<neural_net::model_spec> init_vgg_16();
std::unique_ptr<neural_net::model_spec> init_vgg_16(const std::string& path);

}  // namespace style_transfer
}  // namespace turi

#endif