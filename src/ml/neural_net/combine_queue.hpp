/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <core/util/Verify.hpp>
#include <ml/neural_net/TaskQueue.hpp>
#include <ml/neural_net/combine_base.hpp>

namespace turi {
namespace neural_net {

/**
 * Publisher that implements the Publisher::SubscribeOn operator.
 *
 * The resulting Publisher simply dispatches subscription requests, demands, and
 * cancellations to a specified task queue. It inherits the semantics of the
 * upstream Publisher that it wraps, with regard to the behavior with multiple
 * downstream subscribers.
 */
template <typename T>
class SubscribeOnQueuePublisher : public Publisher<T> {
 public:
  using Output = T;

  SubscribeOnQueuePublisher(std::shared_ptr<Publisher<Output>> upstream,
                            std::shared_ptr<TaskQueue> queue)
      : upstream_(std::move(upstream)), queue_(std::move(queue)) {}
  ~SubscribeOnQueuePublisher() override = default;

  // Any attempt to copy or move instances of this class is likely an error. All
  // instances should be allocated with std::make_shared.
  SubscribeOnQueuePublisher(const SubscribeOnQueuePublisher&) = delete;
  SubscribeOnQueuePublisher(SubscribeOnQueuePublisher&&) = delete;
  SubscribeOnQueuePublisher& operator=(const SubscribeOnQueuePublisher&) =
      delete;
  SubscribeOnQueuePublisher& operator=(SubscribeOnQueuePublisher&&) = delete;

  void Receive(std::shared_ptr<Subscriber<Output>> subscriber) override {
    // Pass a proxy for this subscriber to the upstream publisher, but do so on
    // the requested task queue.
    std::shared_ptr<Publisher<Output>> upstream = upstream_;
    auto impl = std::make_shared<Proxy>(std::move(subscriber), queue_);
    queue_->DispatchAsync([upstream, impl] { upstream->Subscribe(impl); });
  }

 private:
  // This class serves as an intermediary between the upstream publisher and the
  // downstream subscriber.
  class Proxy : public std::enable_shared_from_this<Proxy>,
                public Subscriber<Output>,
                public Subscription {
   public:
    Proxy(std::shared_ptr<Subscriber<Output>> downstream,
          std::shared_ptr<TaskQueue> queue)
        : downstream_(std::move(downstream)), queue_(std::move(queue)) {}
    ~Proxy() override = default;

    // Any attempt to copy or move instances of this class is likely an error.
    // All instances should be allocated with std::make_shared.
    Proxy(const Proxy&) = delete;
    Proxy(Proxy&&) = delete;
    Proxy& operator=(const Proxy&) = delete;
    Proxy& operator=(Proxy&&) = delete;

    // Subscriber<Output> interface, for upstream publisher

    void Receive(std::shared_ptr<Subscription> subscription) override {
      VerifyIsTrue(downstream_, TuriErrorCode::LogicError);  // We cannot have been canceled yet.

      // Intercept (and store) the subscription we receive from the upstream
      // publisher.
      subscription_ = std::move(subscription);

      // Pass ourselves to the downstream subscriber. We will serve as a
      // proxy. From here on out, we can be canceled at any time.
      downstream_->Receive(this->shared_from_this());
    }

    Demand Receive(Output element) override {
      // Do nothing if we are already cancelled.
      if (!downstream_) return Demand::None();

      return downstream_->Receive(std::move(element));
    }

    void Receive(Completion completion) override {
      // Do nothing if we are already cancelled.
      if (!downstream_) return;

      downstream_->Receive(std::move(completion));
    }

    // Subscription interface, for downstream subscriber

    void Cancel() override {
      // Do nothing if we are already cancelled.
      if (!downstream_) return;

      // Ensure that we send no further signals to the downstream subscriber.
      downstream_ = nullptr;

      // Forward the cancel request to the upstream publisher, but do so on the
      // requested task queue.
      std::shared_ptr<Subscription> subscription = subscription_;
      queue_->DispatchAsync([subscription] { subscription->Cancel(); });
    }

    void Request(Demand demand) override {
      // Do nothing if we are already cancelled.
      if (!downstream_) return;

      // Forward the request to the upstream publisher, but do so on the
      // requested task queue.
      std::shared_ptr<Subscription> subscription = subscription_;
      queue_->DispatchAsync(
          [subscription, demand] { subscription->Request(demand); });
    }

   private:
    std::shared_ptr<Subscriber<Output>> downstream_;
    std::shared_ptr<TaskQueue> queue_;
    std::shared_ptr<Subscription> subscription_;
  };

  std::shared_ptr<Publisher<Output>> upstream_;
  std::shared_ptr<TaskQueue> queue_;
};

/**
 * Publisher that implements the Publisher::ReceiveOn operator.
 *
 * The resulting Publisher simply dispatches subscriptions, values, and
 * completions to a specified task queue. It inherits the semantics of the
 * upstream Publisher that it wraps, with regard to the behavior with multiple
 * downstream subscribers.
 */
template <typename T>
class ReceiveOnQueuePublisher : public Publisher<T> {
 public:
  using Output = T;

  ReceiveOnQueuePublisher(std::shared_ptr<Publisher<Output>> upstream,
                          std::shared_ptr<TaskQueue> queue)
      : upstream_(std::move(upstream)), queue_(std::move(queue)) {}
  ~ReceiveOnQueuePublisher() override = default;

  // Any attempt to copy or move instances of this class is likely an error. All
  // instances should be allocated with std::make_shared.
  ReceiveOnQueuePublisher(const ReceiveOnQueuePublisher&) = delete;
  ReceiveOnQueuePublisher(ReceiveOnQueuePublisher&&) = delete;
  ReceiveOnQueuePublisher& operator=(const ReceiveOnQueuePublisher&) = delete;
  ReceiveOnQueuePublisher& operator=(ReceiveOnQueuePublisher&&) = delete;

  void Receive(std::shared_ptr<Subscriber<Output>> subscriber) override {
    auto proxy = std::make_shared<Proxy>(std::move(subscriber), queue_);
    upstream_->Subscribe(std::move(proxy));
  }

 private:
  // This class serves as an intermediary between the upstream publisher and the
  // downstream subscriber.
  class Proxy : public Subscriber<Output> {
   public:
    Proxy(std::shared_ptr<Subscriber<Output>> downstream,
          std::shared_ptr<TaskQueue> queue)
        : downstream_(std::move(downstream)), queue_(std::move(queue)) {}
    ~Proxy() override = default;

    // Any attempt to copy or move instances of this class is likely an error.
    // All instances should be allocated with std::make_shared.
    Proxy(const Proxy&) = delete;
    Proxy(Proxy&&) = delete;
    Proxy& operator=(const Proxy&) = delete;
    Proxy& operator=(Proxy&&) = delete;

    void Receive(std::shared_ptr<Subscription> subscription) override {
      // Store a reference to the subscription so we can request incremental
      // demands resulting from async delivery of values.
      subscription_ = subscription;

      // Send the subscription to the downstream subscriber on the requested
      // task queue.
      std::shared_ptr<Subscriber<Output>> downstream = downstream_;
      queue_->DispatchAsync(
          [downstream, subscription] { downstream->Receive(subscription); });
    }

    Demand Receive(Output element) override {
      // Send the element to the downstream subscriber on the requested task
      // queue.
      std::shared_ptr<Subscriber<Output>> downstream = downstream_;
      std::shared_ptr<Output> shared_element =
          std::make_shared<Output>(std::move(element));
      std::shared_ptr<Subscription> subscription = subscription_;
      queue_->DispatchAsync([downstream, subscription, shared_element] {
        Demand demand = downstream->Receive(std::move(*shared_element));

        // If the subscriber immediately demands more, dispatch a new request.
        if (!demand.IsNone()) {
          subscription->Request(demand);
        }
      });

      // Don't wait for the subscriber to respond.
      return Demand::None();
    }

    void Receive(Completion completion) override {
      // Send the completion to the downstream subscriber on the requested task
      // queue.
      std::shared_ptr<Subscriber<Output>> downstream = downstream_;
      queue_->DispatchAsync(
          [downstream, completion] { downstream->Receive(completion); });
    }

   private:
    std::shared_ptr<Subscriber<Output>> downstream_;
    std::shared_ptr<TaskQueue> queue_;
    std::shared_ptr<Subscription> subscription_;
  };

  std::shared_ptr<Publisher<Output>> upstream_;
  std::shared_ptr<TaskQueue> queue_;
};

}  // namespace neural_net
}  // namespace turi
