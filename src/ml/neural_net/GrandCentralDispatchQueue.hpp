/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <dispatch/dispatch.h>

#include <ml/neural_net/TaskQueue.hpp>

namespace turi {
namespace neural_net {

/// Concrete TaskQueue implementation wrapping Grand Central Dispatch/
class GrandCentralDispatchQueue : public TaskQueue {
 public:
  static std::shared_ptr<GrandCentralDispatchQueue> GetGlobalConcurrentQueue();
  static std::unique_ptr<GrandCentralDispatchQueue> CreateSerialQueue(const char* label);

  explicit GrandCentralDispatchQueue(dispatch_queue_t impl);
  ~GrandCentralDispatchQueue();

  // Not movable or copyable.
  GrandCentralDispatchQueue(const GrandCentralDispatchQueue&) = delete;
  GrandCentralDispatchQueue(GrandCentralDispatchQueue&&) = delete;
  GrandCentralDispatchQueue& operator=(const GrandCentralDispatchQueue&) =
      delete;
  GrandCentralDispatchQueue& operator=(GrandCentralDispatchQueue&&) = delete;

  void DispatchAsync(std::function<void()> task) override;
  void DispatchSync(std::function<void()> task) override;
  void DispatchApply(size_t n, std::function<void(size_t i)> task) override;

 private:
  dispatch_queue_t impl_;
};

}  // namespace neural_net
}  // namespace turi
