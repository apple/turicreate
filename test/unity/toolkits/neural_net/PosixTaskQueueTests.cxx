/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE PosixTaskQueueTests

#include <ml/neural_net/PosixTaskQueue.hpp>

#include <future>
#include <mutex>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

struct TestException : public std::exception {};

static const char kQueueLabel[] = "com.apple.TuriCreate.PosixTaskQueueTests";

BOOST_AUTO_TEST_CASE(TestSerialQueueDispatchAsync) {
  std::shared_ptr<TaskQueue> queue =
      PosixTaskQueue::CreateSerialQueue(kQueueLabel);
  std::promise<void> completion_promise;
  std::future<void> completion_future = completion_promise.get_future();
  queue->DispatchAsync(
      [&completion_promise] { completion_promise.set_value(); });
  std::future_status status =
      completion_future.wait_for(std::chrono::seconds(5));
  TS_ASSERT(status == std::future_status::ready);
}

BOOST_AUTO_TEST_CASE(TestSerialQueueDispatchSync) {
  std::shared_ptr<TaskQueue> queue =
      PosixTaskQueue::CreateSerialQueue(kQueueLabel);
  bool task_executed = false;
  queue->DispatchSync([&task_executed] { task_executed = true; });
  TS_ASSERT(task_executed);
}

BOOST_AUTO_TEST_CASE(TestConcurrentQueueDispatchApplyInvokesAllIndices) {
  std::shared_ptr<TaskQueue> queue = PosixTaskQueue::GetGlobalConcurrentQueue();
  size_t n = 7;
  std::mutex mutex;
  std::vector<bool> called(n, false);  // Protected by mutex
  queue->DispatchApply(n, [&](size_t i) {
    std::lock_guard<std::mutex> guard(mutex);
    TS_ASSERT(!called[i]);
    called[i] = true;
  });
  for (size_t i = 0; i < n; ++i) {
    TS_ASSERT(called[i]);
  }
}

BOOST_AUTO_TEST_CASE(TestConcurrentQueueDispatchApplyRethrowsException) {
  std::shared_ptr<TaskQueue> queue = PosixTaskQueue::GetGlobalConcurrentQueue();
  auto erroneous_task = [](size_t i) {
    if (i == 1) {
      throw TestException();
    }
  };
  TS_ASSERT_THROWS(queue->DispatchApply(7, erroneous_task), TestException);
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
