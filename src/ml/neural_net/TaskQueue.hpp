/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <functional>
#include <memory>

namespace turi {
namespace neural_net {

/// Abstract task queue interface modeled after Grand Central Dispatch
class TaskQueue {
 public:
  /// Returns a task queue that does not enforce any ordering among its tasks
  /// and that shares system resources with other task queues created by this
  /// function.
  static std::shared_ptr<TaskQueue> GetGlobalConcurrentQueue();

  /// Returns a task queue that guarantees that if task A is submitted before
  /// task B, then task A will finish before task B begins. Accepts a label that
  /// may be used by the system to identify work done by this queue.
  static std::unique_ptr<TaskQueue> CreateSerialQueue(const char* label);

  virtual ~TaskQueue() = default;

  /// Submits a function to this task queue without waiting for the function to
  /// finish. The task must not throw an exception.
  virtual void DispatchAsync(std::function<void()> task) = 0;

  /// Submits a function to this task queue and waits for the function to
  /// execute. The task must not throw an exception.
  virtual void DispatchSync(std::function<void()> task) = 0;

  /// Submits a function to this task queue n times, with arguments ranging from
  /// 0 to n - 1. When dispatched to a concurrent queue, the function must be
  /// reentrant. Rethrows the first exception thrown by any task invocation.
  virtual void DispatchApply(size_t n, std::function<void(size_t i)> task) = 0;
};

}  // namespace neural_net
}  // namespace turi
