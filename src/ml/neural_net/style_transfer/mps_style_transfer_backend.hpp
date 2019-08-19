/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_STYLE_TRANSFER_STYLE_TRANSFER_H_
#define TURI_STYLE_TRANSFER_STYLE_TRANSFER_H_

#include <functional>
#include <map>
#include <memory>

#include <ml/neural_net/float_array.hpp>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace style_transfer {

class EXPORT style_transfer : public turi::neural_net::model_backend {
public:
  style_transfer(const turi::neural_net::float_array_map &config,
                 const turi::neural_net::float_array_map &weights);
  
  ~style_transfer() = default;

  turi::neural_net::float_array_map export_weights() const override;
  turi::neural_net::float_array_map predict(const turi::neural_net::float_array_map& inputs) const override;
  void set_learning_rate(float lr) override;
  turi::neural_net::float_array_map train(const turi::neural_net::float_array_map& inputs) override;
private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

} // namespace style_transfer
} // namespace turi

#endif