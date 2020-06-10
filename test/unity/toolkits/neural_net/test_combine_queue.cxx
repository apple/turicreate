/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_combine_queue

#include <ml/neural_net/combine_queue.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/combine_mock.hpp>

namespace turi {
namespace neural_net {
namespace {

/// Implementation of TaskQueue that accumulates async tasks in a queue that
/// must be manually drained
class FakeTaskQueue : public TaskQueue {
 public:
  void DispatchAsync(std::function<void()> task) override {
    async_tasks_.emplace_back(std::move(task));
  }

  // Not implemented (unused by test code below)
  void DispatchSync(std::function<void()> task) override {}
  void DispatchApply(size_t n, std::function<void(size_t i)> task) override {}

  void PerformOneAsyncTask() {
    std::function<void()> task = std::move(async_tasks_.front());
    async_tasks_.pop_front();
    task();
  }
  void PerformAllAsyncTasks() {
    while (!async_tasks_.empty()) {
      PerformOneAsyncTask();
    }
  }

 private:
  std::deque<std::function<void()>> async_tasks_;
};

struct CombineQueueTestFixture {
  CombineQueueTestFixture()
      : task_queue_(std::make_shared<FakeTaskQueue>()),
        mock_publisher_(std::make_shared<MockPublisher<int>>()),
        mock_subscriber_(std::make_shared<MockSubscriber<int>>()) {}

  // The task queue on which the implementations will schedule signals.
  std::shared_ptr<FakeTaskQueue> task_queue_;

  // The actual publisher and subscriber joined by the queue-scheduling proxy.
  std::shared_ptr<MockPublisher<int>> mock_publisher_;

  std::shared_ptr<MockSubscriber<int>> mock_subscriber_;
};

struct SubscribeOnQueueTestFixture : public CombineQueueTestFixture {
  SubscribeOnQueueTestFixture() {
    // Schedule the subscription on the fake task queue. Neither mock should
    // receive any messages yet.
    mock_publisher_->SubscribeOn(task_queue_)->Subscribe(mock_subscriber_);

    // When we advance the task queue, the publisher should see (the proxy for)
    // the subscriber and yield a mock subscription.
    mock_subscription_ = std::make_shared<MockSubscription>();
    auto handle_subscriber =
        [this](std::shared_ptr<Subscriber<int>> subscriber) {
          // Record the subscriber that we see.
          received_subscriber_ = subscriber;

          // Yield the mock subscription.
          subscriber->Receive(mock_subscription_);
        };
    mock_publisher_->subscriber_callbacks.emplace(handle_subscriber);

    // When the publisher yields the mock subscription, the subscriber should
    // immediately see it.
    auto handle_subscription =
        [this](std::shared_ptr<Subscription> subscription) {
          received_subscription_ = std::move(subscription);
        };
    mock_subscriber_->subscription_callbacks.emplace(handle_subscription);

    // Trigger the actual subscription.
    task_queue_->PerformOneAsyncTask();
  }

  // The mock subscription that the mock publisher emits.
  std::shared_ptr<MockSubscription> mock_subscription_;

  // The subscriber that the mock publisher receives.
  std::shared_ptr<Subscriber<int>> received_subscriber_;

  // The subscription that the mock subscriber receives.
  std::shared_ptr<Subscription> received_subscription_;
};

BOOST_FIXTURE_TEST_SUITE(SubscribeOnQueueTest, SubscribeOnQueueTestFixture);

BOOST_AUTO_TEST_CASE(TestDemandDispatchesToQueue) {
  // Schedule a demand for a value. The publisher should not see the demand yet.
  received_subscription_->Request(Demand(1));

  // When we advance the task queue, the mock subscription should see the
  // request.
  auto handle_demand = [this](Demand demand) {
    TS_ASSERT_EQUALS(demand.max(), 1);

    // Send one value to the subscriber we received.
    received_subscriber_->Receive(7);
  };
  mock_subscription_->demand_callbacks.emplace(handle_demand);

  // When the publisher's subscription sends a value, the actual subscriber
  // should immediately see it.
  auto handle_input = [](int input) {
    TS_ASSERT_EQUALS(input, 7);

    return Demand::None();  // Generates no further requests.
  };
  mock_subscriber_->input_callbacks.emplace(handle_input);

  // Trigger the demand.
  task_queue_->PerformAllAsyncTasks();
}

BOOST_AUTO_TEST_CASE(TestCancelDispatchesToQueueAndFinalizes) {
  // Schedule a cancellation. The publisher should not see the cancellation yet.
  received_subscription_->Cancel();

  // When we advance the task queue, the mock subscription should see the
  // cancellation.
  auto handle_cancel = [] {
    // No need to do anything.
  };
  mock_subscription_->cancel_callbacks.emplace(handle_cancel);

  // Trigger the cancellation.
  task_queue_->PerformAllAsyncTasks();
  TS_ASSERT(mock_subscription_->cancel_callbacks.empty());

  // No further demands or cancellations should reach the publisher.
  received_subscription_->Request(Demand(1));
  received_subscription_->Cancel();
  task_queue_->PerformAllAsyncTasks();
}

BOOST_AUTO_TEST_CASE(TestCancelSuppressesMessagesInFlight) {
  // Schedule a cancellation. The publisher should not see the cancellation yet.
  received_subscription_->Cancel();

  // No signal that the publisher sends should reach the actual subscriber, even
  // before the task queue runs. This is not necessary for correctness, since
  // any values sent must have been requested before the cancellation. So this
  // is more of an optimization, to suppress unnecessary work.
  received_subscriber_->Receive(8);
  received_subscriber_->Receive(Completion::Finished());
}

BOOST_AUTO_TEST_SUITE_END();

struct ReceiveOnQueueTestFixture : public CombineQueueTestFixture {
  ReceiveOnQueueTestFixture() {
    // When we connect the subscriber to the publisher, it should immediately
    // receive a subscription.
    mock_subscription_ = std::make_shared<MockSubscription>();
    auto handle_subscriber =
        [this](std::shared_ptr<Subscriber<int>> subscriber) {
          // Record the subscriber that we see.
          received_subscriber_ = subscriber;

          // Yield the mock subscription.
          subscriber->Receive(mock_subscription_);
        };
    mock_publisher_->subscriber_callbacks.emplace(handle_subscriber);

    // Connect the subscriber to the publisher, dispatching the output from the
    // publisher to the fake task queue. The subscriber should not see anything
    // until the task queue actually runs.
    mock_publisher_->ReceiveOn(task_queue_)->Subscribe(mock_subscriber_);
    TS_ASSERT(mock_publisher_->subscriber_callbacks.empty());

    // When the task queue runs, the subscriber should finally see the
    // subscription.
    auto handle_subscription =
        [this](std::shared_ptr<Subscription> subscription) {
          received_subscription_ = std::move(subscription);
        };
    mock_subscriber_->subscription_callbacks.emplace(handle_subscription);

    // Trigger delivery of the subscription.
    task_queue_->PerformOneAsyncTask();
  }

  // The mock subscription that the mock publisher emits.
  std::shared_ptr<MockSubscription> mock_subscription_;

  // The subscriber that the mock publisher receives.
  std::shared_ptr<Subscriber<int>> received_subscriber_;

  // The subscription that the mock subscriber receives.
  std::shared_ptr<Subscription> received_subscription_;
};

BOOST_FIXTURE_TEST_SUITE(ReceiveOnQueueTest, ReceiveOnQueueTestFixture);

BOOST_AUTO_TEST_CASE(TestElementsDispatchedToQueue) {
  // When we request 1 element, the publisher should see the request
  // immediately.
  auto handle_demand = [this](Demand demand) {
    TS_ASSERT_EQUALS(demand.max(), 1);

    Demand incremental_demand = received_subscriber_->Receive(3);

    // Async delivery of inputs always yield no incremental demand.
    TS_ASSERT(incremental_demand.IsNone());
  };
  mock_subscription_->demand_callbacks.emplace(handle_demand);

  // Request 1 input. The subscriber shouldn't see it until the task queue runs.
  received_subscription_->Request(Demand(1));
  TS_ASSERT(mock_subscription_->demand_callbacks.empty());

  // When the task queue runs, the subscriber should finally see the element.
  auto handle_input = [](int input) {
    TS_ASSERT_EQUALS(input, 3);

    // Yield an incremental demand for another input.
    return Demand(1);
  };
  mock_subscriber_->input_callbacks.emplace(handle_input);

  // The incremental demand should reach the publisher synchronously.
  auto handle_incremental_demand = [this](Demand demand) {
    TS_ASSERT_EQUALS(demand.max(), 1);

    Demand incremental_demand = received_subscriber_->Receive(5);

    // Async delivery of inputs always yield no incremental demand.
    TS_ASSERT(incremental_demand.IsNone());
  };
  mock_subscription_->demand_callbacks.emplace(handle_incremental_demand);

  // Trigger delivery of the input.
  task_queue_->PerformOneAsyncTask();
  TS_ASSERT(mock_subscriber_->input_callbacks.empty());
  TS_ASSERT(mock_subscription_->demand_callbacks.empty());

  // The second input, from the incremental demand, should be delivered during
  // the next task in the queue.
  auto handle_incremental_input = [](int input) {
    TS_ASSERT_EQUALS(input, 5);

    return Demand(0);
  };
  mock_subscriber_->input_callbacks.emplace(handle_incremental_input);
  task_queue_->PerformAllAsyncTasks();
}

BOOST_AUTO_TEST_CASE(TestCompletionDispatchedToQueue) {
  // When we request 1 element, the publisher should see the request
  // immediately.
  auto handle_demand = [this](Demand demand) {
    TS_ASSERT_EQUALS(demand.max(), 1);

    received_subscriber_->Receive(Completion::Finished());
  };
  mock_subscription_->demand_callbacks.emplace(handle_demand);

  // Request 1 input. The subscriber shouldn't see the completion until the task
  // queue runs.
  received_subscription_->Request(Demand(1));
  TS_ASSERT(mock_subscription_->demand_callbacks.empty());

  // When the task queue runs, the subscriber should finally see the completion.
  auto handle_completion = [](Completion completion) {
    TS_ASSERT(completion.IsFinished());
  };
  mock_subscriber_->completion_callbacks.emplace(handle_completion);

  // Trigger delivery.
  task_queue_->PerformAllAsyncTasks();
}

BOOST_AUTO_TEST_SUITE_END();

}  // namespace
}  // namespace neural_net
}  // namespace turi
