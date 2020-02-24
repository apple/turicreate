/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_combine_futures_subscriber

#include <ml/neural_net/combine_futures_subscriber.hpp>

#include <stdexcept>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/combine_mock.hpp>

namespace turi {
namespace neural_net {
namespace {

struct TestException : public std::exception {};

// Futures generated before the Publisher has acknowledged the Subscriber should
// still be fulfilled.
BOOST_AUTO_TEST_CASE(test_request_before_subscription) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto publisher = std::make_shared<MockPublisher<int>>();
  auto subscription = std::make_shared<MockSubscription>();

  // Register the FuturesSubscriber with a mock Publisher, but don't return a
  // Subscription immediately.
  publisher->subscriber_callbacks.push([&](std::shared_ptr<Subscriber<int>> s) {
    TS_ASSERT_EQUALS(subscriber, s);
  });
  publisher->Subscribe(subscriber);
  TS_ASSERT(publisher->subscriber_callbacks.empty());

  // Obtain the first future.
  std::future<std::unique_ptr<int>> result = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result.valid());

  // The future should not be ready yet.
  std::future_status status = result.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Now return a Subscription to the FuturesSubscriber. The MockPublisher
  // should immediately expect a Demand for the first future's value.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  subscriber->Receive(subscription);
  TS_ASSERT(subscription->demand_callbacks.empty());

  // Allow the MockPublisher to return Completion::Finished(). The future
  // should now be ready.
  subscriber->Receive(Completion::Finished());
  status = result.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // The future should contain a non-value.
  std::unique_ptr<int> value = result.get();
  TS_ASSERT(!value);
}

std::shared_ptr<MockSubscription> PerformSetup(
    std::shared_ptr<Subscriber<int>> subscriber) {
  // Create a MockSubscription that test code can use to monitor the behavior of
  // the FuturesSubscriber instance under test.
  auto subscription = std::make_shared<MockSubscription>();

  // Register the Subscriber with a MockPublisher that just injects the
  // MockSubscription.
  auto publisher = std::make_shared<MockPublisher<int>>();
  publisher->subscriber_callbacks.push(
      [&](std::shared_ptr<Subscriber<int>> subscriber) {
        subscriber->Receive(subscription);
      });
  publisher->Subscribe(subscriber);
  TS_ASSERT(publisher->subscriber_callbacks.empty());

  return subscription;
}

// Requests for values after Completion should return futures that are
// immediately ready.
BOOST_AUTO_TEST_CASE(test_request_after_finished) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto subscription = PerformSetup(subscriber);

  subscriber->Receive(Completion::Finished());

  std::future<std::unique_ptr<int>> result = subscriber->Request();
  TS_ASSERT(result.valid());

  std::future_status status = result.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  std::unique_ptr<int> value = result.get();
  TS_ASSERT(value == nullptr);
}

// Requests for values before Completion should eventually return non-values if
// the Publisher sends Completion::Finished() instead of values.
BOOST_AUTO_TEST_CASE(test_request_before_finished) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto subscription = PerformSetup(subscriber);

  // Create the first future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result1 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result1.valid());

  // The first future should not be ready.
  std::future_status status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Create the second future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result2 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result2.valid());

  // The second future should not be ready.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Send Completion::Finished().
  subscriber->Receive(Completion::Finished());

  // Both futures should be ready.
  status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // Both futures should have non-values.
  std::unique_ptr<int> value1 = result1.get();
  TS_ASSERT(value1 == nullptr);
  std::unique_ptr<int> value2 = result2.get();
  TS_ASSERT(value2 == nullptr);
}

// Requesting a value should correctly fulfill the future, when the Publisher
// sends the value synchronously on demand.
BOOST_AUTO_TEST_CASE(test_synchronous_response) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto subscription = PerformSetup(subscriber);

  subscription->demand_callbacks.push([&](Demand demand) {
    TS_ASSERT_EQUALS(demand.max(), 1);
    subscriber->Receive(9);
  });
  std::future<std::unique_ptr<int>> result = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result.valid());

  std::future_status status = result.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  std::unique_ptr<int> value = result.get();
  TS_ASSERT(value != nullptr);
  TS_ASSERT_EQUALS(*value, 9);
}

// Requesting a value should correctly fulfill the future, even when the
// Publisher sends the value later.
BOOST_AUTO_TEST_CASE(test_asynchronous_response) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto subscription = PerformSetup(subscriber);

  // Create the first future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result1 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result1.valid());

  // The first future should not be ready yet.
  std::future_status status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Create the second future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result2 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result2.valid());

  // The second future should not be ready yet.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Send the first value.
  subscriber->Receive(5);

  // The first future should be ready.
  status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // The first future should contain the first value.
  std::unique_ptr<int> value1 = result1.get();
  TS_ASSERT(value1 != nullptr);
  TS_ASSERT_EQUALS(*value1, 5);

  // The second future should still not be ready.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Send the second value.
  subscriber->Receive(8);

  // Now the second future should be ready.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // The second future should contain the second value.
  std::unique_ptr<int> value2 = result2.get();
  TS_ASSERT(value2 != nullptr);
  TS_ASSERT_EQUALS(*value2, 8);
}

// Requests for values before Completion should eventually return exceptions if
// the Publisher sends Completion::Failure(...).
BOOST_AUTO_TEST_CASE(test_failure) {
  auto subscriber = std::make_shared<FuturesSubscriber<int>>();
  auto subscription = PerformSetup(subscriber);

  // Create the first future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result1 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result1.valid());

  // The first future should not be ready yet.
  std::future_status status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Create the second future.
  subscription->demand_callbacks.push(
      [&](Demand demand) { TS_ASSERT_EQUALS(demand.max(), 1); });
  std::future<std::unique_ptr<int>> result2 = subscriber->Request();
  TS_ASSERT(subscription->demand_callbacks.empty());
  TS_ASSERT(result2.valid());

  // The second future should not be ready yet.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::timeout);

  // Send Completion::Failure(...).
  try {
    throw TestException();
  } catch (...) {
    subscriber->Receive(Completion::Failure(std::current_exception()));
  }

  // The first future should now be ready.
  status = result1.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // The second future should now be ready.
  status = result2.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);

  // Both futures should now throw the specified exception.
  TS_ASSERT_THROWS(result1.get(), TestException);
  TS_ASSERT_THROWS(result2.get(), TestException);

  // Subsequent futures should also throw the same exception.
  std::future<std::unique_ptr<int>> result3 = subscriber->Request();
  status = result3.wait_for(std::chrono::seconds(0));
  TS_ASSERT(status == std::future_status::ready);
  TS_ASSERT_THROWS(result3.get(), TestException);
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
