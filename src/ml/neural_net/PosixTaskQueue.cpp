/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/PosixTaskQueue.hpp>

#include <core/parallel/lambda_omp.hpp>
#include <core/util/std/make_unique.hpp>

namespace turi {
namespace neural_net {

// static
std::shared_ptr<PosixTaskQueue> PosixTaskQueue::GetGlobalConcurrentQueue() {
  // We use a pointer to a shared_ptr to guarantee that the singleton is never
  // deconstructed, even when main() ends, in case background threads are still
  // trying to call this function.
  static const auto* const singleton = new std::shared_ptr<PosixTaskQueue>(
      std::make_shared<GlobalPosixTaskQueue>());
  return *singleton;
}

// static
std::unique_ptr<PosixTaskQueue> PosixTaskQueue::CreateSerialQueue(const char* label)
{
  return std::make_unique<SerialPosixTaskQueue>(/* num_threads */ 1);
}

void PosixTaskQueue::DispatchAsync(std::function<void()> task) {
  GetThreadPool().launch(task);
}

void PosixTaskQueue::DispatchSync(std::function<void()> task) {
  parallel_task_queue queue(GetThreadPool());
  queue.launch(task);
  queue.join();
}

SerialPosixTaskQueue::SerialPosixTaskQueue(size_t num_threads)
    : threads_(num_threads) {}

SerialPosixTaskQueue::~SerialPosixTaskQueue() = default;

thread_pool& SerialPosixTaskQueue::GetThreadPool() { return threads_; }

void SerialPosixTaskQueue::DispatchApply(size_t n,
                                         std::function<void(size_t i)> task) {
  DispatchSync([n, task] {
    for (size_t i = 0; i < n; ++i) {
      task(i);
    }
  });
}

GlobalPosixTaskQueue::GlobalPosixTaskQueue() = default;
GlobalPosixTaskQueue::~GlobalPosixTaskQueue() = default;

GlobalPosixTaskQueue::GlobalPosixTaskQueue(const GlobalPosixTaskQueue&) =
    default;
GlobalPosixTaskQueue::GlobalPosixTaskQueue(GlobalPosixTaskQueue&&) = default;
GlobalPosixTaskQueue& GlobalPosixTaskQueue::operator=(
    const GlobalPosixTaskQueue&) = default;
GlobalPosixTaskQueue& GlobalPosixTaskQueue::operator=(GlobalPosixTaskQueue&&) =
    default;

thread_pool& GlobalPosixTaskQueue::GetThreadPool() {
  return thread_pool::get_instance();
}

void GlobalPosixTaskQueue::DispatchApply(size_t n,
                                         std::function<void(size_t i)> task) {
  // Just use turi::parallel_for, which always uses thread_pool::get_instance().
  // This implementation slices the n logical iterations into k slices and
  // dispatches to k threads, where k is the number of CPU cores.
  parallel_for(0, n, task);
}

}  // namespace neural_net
}  // namespace turi
