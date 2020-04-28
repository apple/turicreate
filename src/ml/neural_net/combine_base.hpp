/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef ML_NEURAL_NET_COMBINE_BASE_HPP_
#define ML_NEURAL_NET_COMBINE_BASE_HPP_

/**
 * \file combine_base.hpp
 *
 * Defines the core data types for a reactive-streams library inspired by the
 * Swift Combine framework. Client code should generally import combine.hpp.
 */

#include <exception>
#include <memory>

namespace turi {
namespace neural_net {

// Forward declarations for types defined by other headers included by
// combine.hpp.

template <typename T, typename Callable>
class CallableTransform;

template <typename T>
class FuturesStream;

template <typename T>
class FuturesSubscriber;

template <typename T, typename U>
class MapPublisher;

template <typename T, typename U>
class Transform;

/**
 * Simple type expressing how many values a Subscriber is ready to receive
 * from its Publisher.
 */
class Demand {
 public:
  static Demand Unlimited() { return Demand(-1); }
  static Demand None() { return Demand(0); }

  /** Any negative value is interpreted as "unlimited". */
  explicit Demand(int max) : max_(max) {}

  bool IsUnlimited() const { return max_ < 0; }
  bool IsNone() const { return max_ == 0; }

  /** Returns a negative number to indicate "unlimited." */
  int max() const { return max_; }

  /** Additively combines another Demand value into this one. */
  Demand& Add(Demand other) {
    if (IsUnlimited() || other.IsUnlimited()) {
      max_ = -1;
    } else {
      max_ += other.max_;
    }
    return *this;
  }

  /** Decrease this demand by one if the current max is positive and finite. */
  Demand& Decrement() {
    if (max_ > 0) {
      --max_;
    }
    return *this;
  }

 private:
  int max_ = 0;
};

/**
 * Interface for objects that Publishers send to Subscribers to allow the
 * Subscribers to (potentially asynchronously) control the flow of values that
 * the Subscriber receives from the Publisher.
 */
class Subscription {
 public:
  virtual ~Subscription() = default;

  /**
   * Requests the Publisher to stop sending anything to the Subscriber.
   *
   * After receiving Cancel() from a Subscriber, a Publisher should thereafter
   * ignore all future messages from that Subscriber, including future calls to
   * Cancel.
   *
   * Publishers must support Subscribers calling Cancel() from inside
   * Subscriber::Receive(...).
   */
  virtual void Cancel() = 0;

  /**
   * Requests the Publisher to send the indicated number of values to the
   * Subscriber.
   *
   * Publishers must support Subscribers calling Request(Demand) from inside
   * Subscriber::Receive(Subscription), but Subscribers should avoid calling
   * Request(Demand) inside Subscriber::Receive(Input). Instead, they should
   * send additional Demand via the return value of Subscriber::Receive(Input)
   * (to help prevent infinite recursion).
   */
  virtual void Request(Demand demand) = 0;
};

/**
 * Type representing a message from a Publisher to a Subscriber indicating that
 * the Subscriber will no longer receive any further messages.
 */
class Completion {
 public:
  /** Returns an instance that signals successful completion. */
  static Completion Finished() { return Completion(); }

  /**
   * Returns an instance that signals failure, described by the given
   * exception.
   */
  static Completion Failure(std::exception_ptr e) { return Completion(e); }

  bool IsFinished() const { return failure_ == nullptr; }

  /** Returns the exception if a failure and a null pointer otherwise. */
  std::exception_ptr failure() const { return failure_; }

 private:
  explicit Completion(std::exception_ptr e = nullptr) : failure_(e) {}

  std::exception_ptr failure_;
};

/**
 * Interface for objects that consume values from a Publisher.
 *
 * Unless otherwise specified by the concrete implementation, external
 * synchronization must be used to avoid concurrent calls the Subscriber
 * interface from different threads.
 */
template <typename T>
class Subscriber {
 public:
  /** The type of the values that this Subscriber consumes. */
  using Input = T;

  virtual ~Subscriber() = default;

  /**
   * The first signal that a Subscriber receives from a Publisher, passing the
   * Subscription that the Subscriber can use to control the flow of values.
   *
   * A Subscriber may only have one Publisher. If it somehow receives more than
   * one Subscription, it should call Subscription::Cancel() on any instances
   * received after the first.
   *
   * A Subscriber is explictly allowed to demand values synchronously from
   * within its implementation of this method.
   */
  virtual void Receive(std::shared_ptr<Subscription> subscription) = 0;

  /**
   * Transmits a value from the Publisher to this Subscriber.
   *
   * A Subcriber should never receive more calls to this method than the total
   * Demand it has requested from its publisher. Subscribers should only demand
   * more elements from within this method via its return value.
   */
  virtual Demand Receive(Input element) = 0;

  /**
   * Signals completion of the stream of values from the Publisher.
   *
   * A Subscriber should not receive any further signals of any kind after
   * receiving a Completion.
   */
  virtual void Receive(Completion completion) = 0;
};

/**
 * Interface for objects that produce values on demand from its Subscribers.
 *
 * Unless otherwise specified by the concrete implementation, external
 * synchronization must be used to avoid concurrent calls on multiple threads to
 * a Publisher, including via the Subscriptions that it passes to its
 * Subscribers.
 *
 * Each concrete implementation defines whether it is unicast or multicast:
 * whether multiple Subscribers observe the same values or not. (An
 * implementation might only support one Subscriber, by passing an immediate
 * Completion to each Subscriber after the first.)
 *
 * Note: instances of this class are intended to be stored using shared_ptr.
 * Many of the operators rely on generating strong references to the instance
 * being augmented.
 */
template <typename T>
class Publisher : public std::enable_shared_from_this<Publisher<T>> {
 public:
  /** The type of values that this Publisher produces. */
  using Output = T;

  virtual ~Publisher() = default;

  /**
   * Establishes a connection between this Publisher and the given Subcriber.
   *
   * The Publisher must eventually call Subscriber::Receive(Subscription) on the
   * given Subscriber (and may do so synchronously). The Publisher must then
   * conform to the protocol established by the Subscription.
   */
  virtual void Receive(std::shared_ptr<Subscriber<Output>> subscriber) = 0;

  // Convenienience methods, supporting the chaining together of operations.
  // Many of these rely on the forward declarations above. Client code should
  // include combine.hpp to ensure these are defined before they are used.

  void Subscribe(std::shared_ptr<Subscriber<Output>> subscriber) {
    Receive(std::move(subscriber));
  }

  std::shared_ptr<FuturesStream<Output>> AsFutures() {
    auto subscriber = std::make_shared<FuturesSubscriber<Output>>();
    Subscribe(subscriber);
    return std::make_shared<FuturesStream<Output>>(std::move(subscriber));
  }

  template <typename TransformType>
  std::shared_ptr<Publisher<typename TransformType::Output>> Map(
      std::shared_ptr<TransformType> transform) {
    using TransformInput = typename TransformType::Input;
    using TransformOutput = typename TransformType::Output;
    return std::make_shared<MapPublisher<TransformInput, TransformOutput>>(
        this->shared_from_this(), std::move(transform));
  }

  template <typename Callable>
  std::shared_ptr<Publisher<typename std::result_of<Callable(Output)>::type>>
  Map(Callable fn) {
    using TransformType = CallableTransform<Output, Callable>;
    auto transform = std::make_shared<TransformType>(std::move(fn));
    return Map(std::move(transform));
  }
};

}  // namespace neural_net
}  // namespace turi

#endif  // ML_NEURAL_NET_COMBINE_BASE_HPP_
