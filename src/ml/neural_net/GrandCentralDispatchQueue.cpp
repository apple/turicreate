/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/GrandCentralDispatchQueue.hpp>

#include <exception>
#include <mutex>

#include <core/util/std/make_unique.hpp>

namespace turi {
namespace neural_net {

// static
std::shared_ptr<GrandCentralDispatchQueue>
GrandCentralDispatchQueue::GetGlobalConcurrentQueue() {
  // We use a pointer to a shared_ptr to guarantee that the singleton is never
  // deconstructed, even when main() ends, in case background threads are still
  // trying to call this function.
  static const auto* const singleton =
      new std::shared_ptr<GrandCentralDispatchQueue>(
          std::make_shared<GrandCentralDispatchQueue>(
              dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0)));
  return *singleton;
}

// static
std::unique_ptr<GrandCentralDispatchQueue> GrandCentralDispatchQueue::CreateSerialQueue(
    const char* label)
{
  dispatch_queue_t impl = dispatch_queue_create(label, DISPATCH_QUEUE_SERIAL);
  auto result = std::make_unique<GrandCentralDispatchQueue>(impl);
  dispatch_release(impl);
  return result;
}

GrandCentralDispatchQueue::GrandCentralDispatchQueue(dispatch_queue_t impl)
    : impl_(impl) {
  dispatch_retain(impl_);
}

GrandCentralDispatchQueue::~GrandCentralDispatchQueue() {
  dispatch_release(impl_);
}

void GrandCentralDispatchQueue::DispatchAsync(std::function<void()> task) {
  dispatch_async(impl_, ^{
    task();
  });
}

void GrandCentralDispatchQueue::DispatchSync(std::function<void()> task) {
  dispatch_sync(impl_, ^{
    task();
  });
}

void GrandCentralDispatchQueue::DispatchApply(
    size_t n, std::function<void(size_t i)> task) {
  // Use a mutex-protected exception pointer to communicate exceptions to the
  // caller, since GCD treats any uncaught C++ exception as a fatal error. Note
  // that we cannot capture a mutable std::mutex instance in an Objective-C
  // block, so we capture a pointer to the mutex instead.
  std::mutex mutex;
  std::mutex* mutex_ptr = &mutex;
  __block std::exception_ptr error;

  dispatch_apply(n, impl_, ^(size_t i) {
    try {
      task(i);
    } catch (...) {
      std::exception_ptr local_error = std::current_exception();
      std::lock_guard<std::mutex> guard(*mutex_ptr);
      if (!error) {
        error = local_error;
      }
    }
  });

  if (error) {
    std::rethrow_exception(error);
  }
}

}  // namespace neural_net
}  // namespace turi
