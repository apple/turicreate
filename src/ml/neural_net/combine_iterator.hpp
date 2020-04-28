/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef ML_NEURAL_NET_COMBINE_ITERATOR_HPP_
#define ML_NEURAL_NET_COMBINE_ITERATOR_HPP_

#include <exception>
#include <memory>
#include <type_traits>

#include <ml/neural_net/combine_base.hpp>

namespace turi {
namespace neural_net {

template <typename T>
class IteratorPublisher;

/**
 * Interface for objects that produce a sequence of values, using the
 * conventional iterator interface.
 *
 * This type facilitates wrapping traditional iterator-style code as a
 * Publisher.
 */
template <typename T>
class Iterator : public std::enable_shared_from_this<Iterator<T>> {
 public:
  using Output = T;

  virtual ~Iterator() = default;

  /**
   * Returns true as long as the underlying sequence contains more values.
   *
   * \todo If we have Optional<T>, we can remove this method and have Next()
   *       instead return Optional<Output>.
   */
  virtual bool HasNext() const = 0;

  /** Returns the next value in the sequence. May throw on error. */
  virtual Output Next() = 0;

  /** Returns a Publisher wrapping this Iterator. */
  std::shared_ptr<IteratorPublisher<T>> AsPublisher() {
    return std::make_shared<IteratorPublisher<T>>(this->shared_from_this());
  }
};

/**
 * Templated implementation of Iterator that wraps an arbitrary callable type.
 */
template <typename Callable>
class CallableIterator
    : public Iterator<typename std::result_of<Callable()>::type> {
 public:
  using Output = typename std::result_of<Callable()>::type;

  CallableIterator(Callable impl) : impl_(std::move(impl)) {}

  bool HasNext() const override { return true; }

  Output Next() override { return impl_(); }

 private:
  Callable impl_;
};

template <typename Callable>
std::shared_ptr<IteratorPublisher<typename std::result_of<Callable()>::type>>
CreatePublisherFromCallable(Callable impl) {
  return std::make_shared<CallableIterator<Callable>>(std::move(impl))
      ->AsPublisher();
}

/**
 * Concrete Publisher that wraps an Iterator.
 *
 * The resulting Publisher is unicast: each iterated value will go only to
 * whichever Subscriber triggered the iteration.
 */
template <typename T>
class IteratorPublisher : public Publisher<T> {
 public:
  using Output = T;

  explicit IteratorPublisher(std::shared_ptr<Iterator<Output>> iterator)
      : iterator_(std::move(iterator)) {}

  void Receive(std::shared_ptr<Subscriber<Output>> subscriber) override {
    auto subscription =
        std::make_shared<IteratorSubscription>(subscriber, iterator_);
    subscriber->Receive(std::move(subscription));
  }

 private:
  // All of the logic lives in the implementation of Subscription, which relies
  // on the assumption that only one Subscription at a time will access the
  // shared Iterator.
  class IteratorSubscription : public Subscription {
   public:
    IteratorSubscription(std::shared_ptr<Subscriber<Output>> subscriber,
                         std::shared_ptr<Iterator<Output>> iterator)
        : subscriber_(std::move(subscriber)), iterator_(std::move(iterator)) {}

    bool IsActive() const { return subscriber_ != nullptr; }

    void Cancel() override { subscriber_.reset(); }

    void Request(Demand demand) override {
      // Keep sending signals to the Subscriber until we're cancelled or we
      // exhaust the demand.
      while (IsActive() && !demand.IsNone()) {
        // Invoke the iterator to determine what signal we'll send.

        // Don't assume that Output has a (cheap) default constructor.
        // TODO: Use an Optional type instead.
        using OutputStorage =
            typename std::aligned_storage<sizeof(Output),
                                          alignof(Output)>::type;
        OutputStorage value_storage;
        Output* value = nullptr;     // Track whether we have a value.
        std::exception_ptr failure;  // Track whether we have a failure.
        try {
          if (iterator_->HasNext()) {
            // Use placement new to initialize our value from the iterator.
            value = reinterpret_cast<Output*>(&value_storage);
            new (value) Output(iterator_->Next());
          }
        } catch (...) {
          // On any exception, *value was not initialized, since it was the last
          // statement in the try block.
          value = nullptr;
          failure = std::current_exception();
        }

        // Send the appropriate signal.
        if (failure) {
          // Signal failure and ensure we don't send any more signals.
          subscriber_->Receive(Completion::Failure(failure));
          Cancel();
        } else if (!value) {
          // Signal finished and ensure we don't send any more signals.
          subscriber_->Receive(Completion::Finished());
          Cancel();
        } else {
          // Pass the value to the Subscriber, adding any new demand.
          demand.Decrement();
          Demand new_demand = subscriber_->Receive(std::move(*value));
          demand.Add(new_demand);

          // We must manually destroy the (moved-from) value.
          value->~Output();
        }
      }
    }

   private:
    std::shared_ptr<Subscriber<Output>> subscriber_;
    std::shared_ptr<Iterator<Output>> iterator_;
  };

  std::shared_ptr<Iterator<Output>> iterator_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // ML_NEURAL_NET_COMBINE_ITERATOR_HPP_
