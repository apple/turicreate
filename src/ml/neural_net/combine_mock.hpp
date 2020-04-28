/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef ML_NEURAL_NET_COMBINE_MOCK_HPP_
#define ML_NEURAL_NET_COMBINE_MOCK_HPP_

#include <ml/neural_net/combine_base.hpp>

#include <functional>
#include <queue>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace neural_net {

/**
 * Helper function to reduce verbosity of writing mocks.
 *
 * Pops the first callback from the given queue and invokes it with the provided
 * arguments.
 */
template <typename R, typename... Args>
R Call(std::queue<std::function<R(Args...)>> *callbacks, Args &&... args) {
  TS_ASSERT(!callbacks->empty());
  auto callback = std::move(callbacks->front());
  callbacks->pop();
  return callback(std::forward<Args>(args)...);
}

class MockSubscription : public Subscription {
 public:
  void Cancel() override { return Call(&cancel_callbacks); }
  std::queue<std::function<void()>> cancel_callbacks;

  void Request(Demand demand) override {
    return Call(&demand_callbacks, std::move(demand));
  }
  std::queue<std::function<void(Demand)>> demand_callbacks;
};

template <typename T>
class MockSubscriber : public Subscriber<T> {
 public:
  using Input = T;

  void Receive(std::shared_ptr<Subscription> subscription) override {
    return Call(&subscription_callbacks, std::move(subscription));
  }
  std::queue<std::function<void(std::shared_ptr<Subscription>)>>
      subscription_callbacks;

  Demand Receive(Input element) override {
    return Call(&input_callbacks, std::move(element));
  }
  std::queue<std::function<Demand(Input)>> input_callbacks;

  void Receive(Completion completion) override {
    return Call(&completion_callbacks, std::move(completion));
  }
  std::queue<std::function<void(Completion)>> completion_callbacks;
};

template <typename T>
class MockPublisher : public Publisher<T> {
 public:
  using Output = T;

  void Receive(std::shared_ptr<Subscriber<Output>> subscriber) override {
    return Call(&subscriber_callbacks, std::move(subscriber));
  }
  std::queue<std::function<void(std::shared_ptr<Subscriber<Output>>)>>
      subscriber_callbacks;
};

}  // namespace neural_net
}  // namespace turi

#endif  // ML_NEURAL_NET_COMBINE_MOCK_HPP_
