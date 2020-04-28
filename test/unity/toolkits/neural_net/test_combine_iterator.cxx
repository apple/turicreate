/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_combine_iterator

#include <ml/neural_net/combine_iterator.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/combine_mock.hpp>

namespace turi {
namespace neural_net {
namespace {

struct TestException : public std::exception {};

template <typename T>
class MockIterator : public Iterator<T> {
 public:
  using Output = T;
  bool HasNext() const override { return !next_callbacks.empty(); }
  Output Next() override { return Call(&next_callbacks); }
  std::queue<std::function<Output()>> next_callbacks;
};

// Subscribers are allowed to call Subscription::Cancel() inside
// Subscriber::Receive(Subscription).
BOOST_AUTO_TEST_CASE(test_cancel_on_subscription) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();

  subscriber->subscription_callbacks.emplace(
      [&](std::shared_ptr<Subscription> subscription) {
        subscription->Cancel();
        subscription->Request(Demand(1));  // Should have no effect.
      });
  iterator->AsPublisher()->Subscribe(subscriber);
  TS_ASSERT(subscriber->subscription_callbacks.empty());
}

// Subscribers are allowed to call Subscription::Request() inside
// Subscriber::Receive(Subscription).
BOOST_AUTO_TEST_CASE(test_demand_on_subscription) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();

  subscriber->subscription_callbacks.emplace(
      [&](std::shared_ptr<Subscription> subscription) {
        // Program the iterator to return 8 once.
        iterator->next_callbacks.push([] { return 8; });

        // Expect the subscriber to receive 8.
        subscriber->input_callbacks.push([](int input) {
          TS_ASSERT_EQUALS(input, 8);
          return Demand::None();
        });

        // Trigger the iterator.
        subscription->Request(Demand(1));
        TS_ASSERT(subscriber->input_callbacks.empty());
      });
  iterator->AsPublisher()->Subscribe(subscriber);
  TS_ASSERT(subscriber->subscription_callbacks.empty());
}

// Handle common test setup.
std::shared_ptr<Subscription> PerformSetup(
    std::shared_ptr<MockIterator<int>> iterator,
    std::shared_ptr<MockSubscriber<int>> subscriber) {
  // Register the subscriber with the IteratorPublisher, capturing the
  // subscription that the IteratorPublisher sends.
  std::shared_ptr<Subscription> subscription;
  subscriber->subscription_callbacks.emplace(
      [&](std::shared_ptr<Subscription> s) { subscription = std::move(s); });
  iterator->AsPublisher()->Subscribe(subscriber);
  TS_ASSERT(subscriber->subscription_callbacks.empty());

  // Return the subscription for test code to manipulate.
  return subscription;
}

// Demanding values from an empty iterator triggers Completion::Finished().
BOOST_AUTO_TEST_CASE(test_empty_iterator) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  subscriber->completion_callbacks.push(
      [](Completion completion) { TS_ASSERT(completion.IsFinished()); });
  subscription->Request(Demand(1));
  TS_ASSERT(subscriber->completion_callbacks.empty());
}

// Demanding more values than the iterator contains yields the complete
// sequence, followed by Completion::Finished().
BOOST_AUTO_TEST_CASE(test_demand_too_much) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });
  subscriber->completion_callbacks.push(
      [](Completion completion) { TS_ASSERT(completion.IsFinished()); });

  subscription->Request(Demand(3));
  TS_ASSERT(subscriber->input_callbacks.empty());
  TS_ASSERT(subscriber->completion_callbacks.empty());
}

// Demanding fewer values than the iterator contains only yields the amount
// requested.
BOOST_AUTO_TEST_CASE(test_demand_too_little) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });
  iterator->next_callbacks.push([] { return 8; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });

  subscription->Request(Demand(2));
  TS_ASSERT(subscriber->input_callbacks.empty());
}

// Demanding the exact number of values remaining yields all the values without
// Completion::Finished().
BOOST_AUTO_TEST_CASE(test_demand_just_right) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });

  subscription->Request(Demand(2));
  TS_ASSERT(subscriber->input_callbacks.empty());
}

// Subscribers can request more input in Subscriber::Receive(Input).
BOOST_AUTO_TEST_CASE(test_demand_more_on_input) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand(1);
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });

  subscription->Request(Demand(1));
  TS_ASSERT(subscriber->input_callbacks.empty());
}

// Unlimited demand yields all values.
BOOST_AUTO_TEST_CASE(test_demand_unlimited) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });
  iterator->next_callbacks.push([] { return 8; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 8);
    return Demand::None();
  });
  subscriber->completion_callbacks.push(
      [](Completion completion) { TS_ASSERT(completion.IsFinished()); });

  subscription->Request(Demand::Unlimited());
  TS_ASSERT(subscriber->input_callbacks.empty());
  TS_ASSERT(subscriber->completion_callbacks.empty());
}

// Adding unlimited demand to an existing demand yields all values.
BOOST_AUTO_TEST_CASE(test_demand_unlimited_on_input) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });
  iterator->next_callbacks.push([] { return 8; });

  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::Unlimited();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });
  subscriber->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 8);
    return Demand::None();
  });
  subscriber->completion_callbacks.push(
      [](Completion completion) { TS_ASSERT(completion.IsFinished()); });

  subscription->Request(Demand(1));
  TS_ASSERT(subscriber->input_callbacks.empty());
  TS_ASSERT(subscriber->completion_callbacks.empty());
}

// An iterator exception propagates as a Completion::Failure(...).
BOOST_AUTO_TEST_CASE(test_failure) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber = std::make_shared<MockSubscriber<int>>();
  auto subscription = PerformSetup(iterator, subscriber);

  iterator->next_callbacks.push([&]() -> int { throw TestException(); });

  subscriber->completion_callbacks.push([&](Completion completion) {
    TS_ASSERT(!completion.IsFinished());
    TS_ASSERT_THROWS(std::rethrow_exception(completion.failure()),
                     TestException);
  });

  subscription->Request(Demand(1));
  TS_ASSERT(subscriber->completion_callbacks.empty());
}

// Two subscribers receive different portions of the iterated sequence.
BOOST_AUTO_TEST_CASE(test_unicast) {
  auto iterator = std::make_shared<MockIterator<int>>();
  auto subscriber1 = std::make_shared<MockSubscriber<int>>();
  auto subscriber2 = std::make_shared<MockSubscriber<int>>();
  auto subscription1 = PerformSetup(iterator, subscriber1);
  auto subscription2 = PerformSetup(iterator, subscriber2);

  iterator->next_callbacks.push([] { return 3; });
  iterator->next_callbacks.push([] { return 5; });
  iterator->next_callbacks.push([] { return 8; });

  subscriber1->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 3);
    return Demand::None();
  });
  subscription1->Request(Demand(1));
  TS_ASSERT(subscriber1->input_callbacks.empty());

  subscriber2->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 5);
    return Demand::None();
  });
  subscription2->Request(Demand(1));
  TS_ASSERT(subscriber2->input_callbacks.empty());

  subscriber1->input_callbacks.push([](int x) {
    TS_ASSERT_EQUALS(x, 8);
    return Demand::None();
  });
  subscription1->Request(Demand(1));
  TS_ASSERT(subscriber1->input_callbacks.empty());
}

}  // namespace
}  // namespace neural_net
}  // namespace turi
