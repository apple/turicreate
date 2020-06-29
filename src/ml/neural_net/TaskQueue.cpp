/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/TaskQueue.hpp>

#ifdef __APPLE__
#include <ml/neural_net/GrandCentralDispatchQueue.hpp>
#else
#include <ml/neural_net/PosixTaskQueue.hpp>
#endif

namespace turi {
namespace neural_net {

// static
std::shared_ptr<TaskQueue> TaskQueue::GetGlobalConcurrentQueue() {
#ifdef __APPLE__
  return GrandCentralDispatchQueue::GetGlobalConcurrentQueue();
#else
  return PosixTaskQueue::GetGlobalConcurrentQueue();
#endif
}

// static
std::unique_ptr<TaskQueue> TaskQueue::CreateSerialQueue(const char* label)
{
#ifdef __APPLE__
  return GrandCentralDispatchQueue::CreateSerialQueue(label);
#else
  return PosixTaskQueue::CreateSerialQueue(label);
#endif
}

}  // namespace neural_net
}  // namespace turi
