/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_combine_map

#include <ml/neural_net/combine_map.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/combine_mock.hpp>

namespace turi {
namespace neural_net {
namespace {

struct TestException : public std::exception {};

template <typename T, typename U>
class MockTransform : public Transform<T, U> {
 public:
  using Input = T;
  using Output = U;

  Output Invoke(Input value) {
    return Call(&invoke_callbacks, std::move(value));
  }
  std::queue<std::function<Output(Input)>> invoke_callbacks;
};

// Calls to MapPublisher::Receive(Subscriber) should be forwarded to the
// upstream Publisher.
BOOST_AUTO_TEST_CASE(test_subscription) {
  auto upstream = std::make_shared<MockPublisher<std::string>>();
  auto transform = std::make_shared<MockTransform<std::string, int>>();
  auto downstream = std::make_shared<MockSubscriber<int>>();
  auto subscription = std::make_shared<MockSubscription>();

  upstream->subscriber_callbacks.emplace(
      [&](std::shared_ptr<Subscriber<std::string>> subscriber) {
        TS_ASSERT(subscriber != nullptr);
      });
  upstream->Map(transform)->Subscribe(downstream);
  TS_ASSERT(upstream->subscriber_callbacks.empty());
}

// Handle common test setup.
std::shared_ptr<Subscriber<int>> PerformSetup(
    std::shared_ptr<MockTransform<int, int>> transform,
    std::shared_ptr<MockSubscriber<int>> downstream) {
  // We will capture the internal Subscriber that MapPublisher generates for its
  // upstream Publisher.
  std::shared_ptr<Subscriber<int>> map_subscriber;

  // The actual subscription and upstream are arbitrary.
  auto subscription = std::make_shared<MockSubscription>();
  auto upstream = std::make_shared<MockPublisher<int>>();

  // The upstream should expect the internal Subscriber, save a reference to it,
  // and pass a subscription to it.
  upstream->subscriber_callbacks.emplace(
      [&](std::shared_ptr<Subscriber<int>> s) {
        map_subscriber = s;
        map_subscriber->Receive(subscription);
      });

  // The internal Subscriber should forward the subscription to the downstream
  // Subscriber.
  downstream->subscription_callbacks.emplace(
      [&](std::shared_ptr<Subscription> s) {
        TS_ASSERT_EQUALS(s, subscription);
      });

  // Trigger subscription.
  upstream->Map(transform)->Subscribe(downstream);
  TS_ASSERT(upstream->subscriber_callbacks.empty());
  TS_ASSERT(downstream->subscription_callbacks.empty());

  // Return the internal subscriber for test code to manipulate.
  return map_subscriber;
}

// Values sent (by the upstream) to the internal Subscriber should be passed
// through the transform and on to the downstream.
BOOST_AUTO_TEST_CASE(test_send_value) {
  auto transform = std::make_shared<MockTransform<int, int>>();
  auto downstream = std::make_shared<MockSubscriber<int>>();
  auto map_subscriber = PerformSetup(transform, downstream);

  // We will send 5 from upstream. The original value should enter transform.
  transform->invoke_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return 25;
  });

  // And the output of the transform should reach the downstream.
  downstream->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 25);
    return Demand::None();
  });

  // Trigger a value.
  map_subscriber->Receive(5);
  TS_ASSERT(transform->invoke_callbacks.empty());
  TS_ASSERT(downstream->input_callbacks.empty());
}

// Completions sent (by the upstream) to the internal Subscriber should be
// passed through to the downstream, ignoring the transform.
BOOST_AUTO_TEST_CASE(test_upstream_failure) {
  auto transform = std::make_shared<MockTransform<int, int>>();
  auto downstream = std::make_shared<MockSubscriber<int>>();
  auto map_subscriber = PerformSetup(transform, downstream);

  std::exception_ptr failure;
  try {
    throw TestException();
  } catch (...) {
    failure = std::current_exception();
  }

  downstream->completion_callbacks.push([&](Completion completion) {
    TS_ASSERT(!completion.IsFinished());
    TS_ASSERT_THROWS(std::rethrow_exception(completion.failure()),
                     TestException);
  });

  map_subscriber->Receive(Completion::Failure(failure));
  TS_ASSERT(downstream->completion_callbacks.empty());
}

// Exceptions thrown by the transform should trigger a completion.
BOOST_AUTO_TEST_CASE(test_transform_failure) {
  auto transform = std::make_shared<MockTransform<int, int>>();
  auto downstream = std::make_shared<MockSubscriber<int>>();
  auto map_subscriber = PerformSetup(transform, downstream);

  transform->invoke_callbacks.push([](int x) -> int { throw TestException(); });

  downstream->completion_callbacks.push([&](Completion completion) {
    TS_ASSERT(!completion.IsFinished());
    TS_ASSERT_THROWS(std::rethrow_exception(completion.failure()),
                     TestException);
  });

  map_subscriber->Receive(7);
  TS_ASSERT(transform->invoke_callbacks.empty());
  TS_ASSERT(downstream->completion_callbacks.empty());

  // Subsequent inputs should not trigger any downstream signals.
  map_subscriber->Receive(8);
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
