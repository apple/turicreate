/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <future>
#include <memory>
#include <queue>

#include <core/util/std/make_unique.hpp>
#include <core/util/Verify.hpp>
#include <ml/neural_net/combine_base.hpp>

namespace turi {
namespace neural_net {

/**
 * Subscriber that synchronously produces futures for promises to be fulfilled
 * by its publisher.
 *
 * This type is useful for integrating Publishers into existing code bases that
 * rely on synchronous behavior or futures.
 *
 * Client code MUST call FuturesSubscriber::Cancel() to tear down a
 * FuturesSubscriber instance. This requirement can be handled automatically
 * using the FuturesStream wrapper class below.
 */
template <typename T>
class FuturesSubscriber : public Subscriber<T> {
 public:
  using Input = T;

  /**
   * Submits a request for a value to the Publisher but immediately returns a
   * future for that value.
   *
   * If the publisher returned a failure for this request or any previous
   * request from this subscriber, then the future will store that exception. If
   * the publisher returned Completion::IsFinished() for this request or any
   * previous request, or if Cancel is called, then the future will store null.
   *
   * \todo Use an Optional<T> type instead of unique_ptr to avoid allocation.
   */
  std::future<std::unique_ptr<T>> Request() {
    std::promise<std::unique_ptr<T>> promise;
    auto future = promise.get_future();
    if (failure_) {
      // We've already observed an exception. Set it now.
      promise.set_exception(failure_);
    } else if (completed_) {
      // We've already observed the end of the sequence. Signal completion now.
      promise.set_value(nullptr);
    } else {
      // Enqueue this promise and submit a request to the Publisher.
      promises_.push(std::move(promise));
      if (subscription_) {
        subscription_->Request(Demand(1));
      }
    }
    return future;
  }

  void Cancel() {
    if (completed_) return;

    completed_ = true;

    // Cancel the subscription if active.
    if (subscription_) {
      subscription_->Cancel();
      subscription_.reset();
    }

    // Fulfill any outstanding promises.
    while (!promises_.empty()) {
      promises_.front().set_value(nullptr);
      promises_.pop();
    }
  }

  void Receive(std::shared_ptr<Subscription> subscription) override {
    // It is a programmer error to attach the same Subscriber to more than one
    // Publisher.
    VerifyIsTrue(subscription_ == nullptr, TuriErrorCode::LogicError);

    // Reject any subscriptions after the first. Reject the first subscription
    // if we cancelled before it could start.
    if (subscription_ || completed_) {
      subscription->Cancel();
      return;
    }

    subscription_ = std::move(subscription);

    // If we already have promises queued, request their values now.
    if (!promises_.empty()) {
      Demand demand(static_cast<int>(promises_.size()));
      subscription_->Request(demand);
    }
  }

  Demand Receive(Input element) override {
    // Do nothing if we were cancelled.
    if (completed_) return Demand::None();

    // Wrap the value in a unique_ptr and fulfill the promise.
    auto input = std::make_unique<Input>(std::move(element));
    promises_.front().set_value(std::move(input));
    promises_.pop();
    return Demand::None();
  }

  void Receive(Completion completion) override {
    completed_ = true;
    if (!completion.IsFinished()) {
      failure_ = completion.failure();
    }

    // Fulfill any outstanding promises.
    while (!promises_.empty()) {
      auto promise = std::move(promises_.front());
      promises_.pop();
      if (failure_) {
        promise.set_exception(completion.failure());
      } else {
        promise.set_value(nullptr);
      }
    }
  }

 private:
  std::shared_ptr<Subscription> subscription_;

  std::queue<std::promise<std::unique_ptr<T>>> promises_;

  bool completed_ = false;
  std::exception_ptr failure_;
};

/**
 * Simple wrapper class around FuturesSubscriber that calls Cancel() on
 * destruction of the wrapper.
 */
template <typename T>
class FuturesStream {
 public:
  explicit FuturesStream(std::shared_ptr<FuturesSubscriber<T>> subscriber)
      : subscriber_(std::move(subscriber)) {}

  ~FuturesStream() { subscriber_->Cancel(); }

  std::future<std::unique_ptr<T>> Next() { return subscriber_->Request(); }

 private:
  std::shared_ptr<FuturesSubscriber<T>> subscriber_;
};

}  // namespace neural_net
}  // namespace turi
