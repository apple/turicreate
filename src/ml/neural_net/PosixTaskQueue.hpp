/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <core/parallel/thread_pool.hpp>
#include <ml/neural_net/TaskQueue.hpp>

namespace turi {
namespace neural_net {

/// Abstract implementation of TaskQueue that wraps turi::thread_pool
class PosixTaskQueue : public TaskQueue {
 public:
  static std::shared_ptr<PosixTaskQueue> GetGlobalConcurrentQueue();
  static std::unique_ptr<PosixTaskQueue> CreateSerialQueue(const char* label);

  void DispatchAsync(std::function<void()> task) override;
  void DispatchSync(std::function<void()> task) override;

 protected:
  virtual thread_pool& GetThreadPool() = 0;
};

/// Concrete implementation of PosixTaskQueue that owns a private thread_pool
/// instance.
class SerialPosixTaskQueue : public PosixTaskQueue {
 public:
  explicit SerialPosixTaskQueue(size_t num_threads);
  ~SerialPosixTaskQueue() override;

  // Not copyable or movable.
  SerialPosixTaskQueue(const SerialPosixTaskQueue&) = delete;
  SerialPosixTaskQueue(SerialPosixTaskQueue&&) = delete;
  SerialPosixTaskQueue& operator=(const SerialPosixTaskQueue&) = delete;
  SerialPosixTaskQueue& operator=(SerialPosixTaskQueue&&) = delete;

  void DispatchApply(size_t n, std::function<void(size_t i)> task) override;

 protected:
  thread_pool& GetThreadPool() override;

 private:
  thread_pool threads_;
};

/// Concrete implementation of PosixTaskQueue that wraps the global singleton
/// thread_pool.
class GlobalPosixTaskQueue : public PosixTaskQueue {
 public:
  GlobalPosixTaskQueue();
  ~GlobalPosixTaskQueue() override;

  // Copyable or movable (but what's the point?)
  GlobalPosixTaskQueue(const GlobalPosixTaskQueue&);
  GlobalPosixTaskQueue(GlobalPosixTaskQueue&&);
  GlobalPosixTaskQueue& operator=(const GlobalPosixTaskQueue&);
  GlobalPosixTaskQueue& operator=(GlobalPosixTaskQueue&&);

  void DispatchApply(size_t n, std::function<void(size_t i)> task) override;

 protected:
  thread_pool& GetThreadPool() override;
};

}  // namespace neural_net
}  // namespace turi
