/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef ML_NEURAL_NET_COMBINE_MAP_HPP_
#define ML_NEURAL_NET_COMBINE_MAP_HPP_

#include <exception>
#include <functional>
#include <memory>
#include <type_traits>

#include <ml/neural_net/combine_base.hpp>

namespace turi {
namespace neural_net {

template <typename T>
class IteratorPublisher;

/**
 * Interface for objects that apply a transform to a value.
 */
template <typename T, typename U>
class Transform {
 public:
  using Input = T;
  using Output = U;

  virtual ~Transform() = default;

  /** Returns the next value in the sequence. May throw on error. */
  virtual Output Invoke(Input value) = 0;
};

/**
 * Templated implementation of Transform that wraps an arbitrary callable type.
 */
template <typename T, typename Callable>
class CallableTransform
    : public Transform<T, typename std::result_of<Callable(T)>::type> {
 public:
  using Input = T;
  using Output = typename std::result_of<Callable(T)>::type;

  CallableTransform(Callable impl) : impl_(std::move(impl)) {}

  Output Invoke(Input input) override { return impl_(std::move(input)); }

 private:
  Callable impl_;
};

/**
 * Concrete operator Publisher that wraps a Transform.
 *
 * The resulting Publisher inherits the semantics of the upstream Publisher that
 * it subscribes to, with regard to the semantics of multiple downstream
 * subscribers. It simply applies the Transform to each value from the upstream.
 */
template <typename T, typename U>
class MapPublisher : public Publisher<U> {
 public:
  using Input = T;
  using Output = U;

  MapPublisher(std::shared_ptr<Publisher<T>> upstream,
               std::shared_ptr<Transform<T, U>> transform)
      : upstream_(std::move(upstream)), transform_(std::move(transform)) {}

  void Receive(std::shared_ptr<Subscriber<Output>> subscriber) override {
    auto impl =
        std::make_shared<MapSubscriber>(transform_, std::move(subscriber));
    upstream_->Subscribe(std::move(impl));
  }

 private:
  class MapSubscriber : public Subscriber<Input> {
   public:
    MapSubscriber(std::shared_ptr<Transform<Input, Output>> transform,
                  std::shared_ptr<Subscriber<Output>> downstream)
        : transform_(std::move(transform)),
          downstream_(std::move(downstream)) {}

    void Receive(std::shared_ptr<Subscription> subscription) override {
      if (downstream_) {
        downstream_->Receive(std::move(subscription));
      }
    }

    Demand Receive(Input element) override {
      // Do nothing if we are already cancelled.
      if (!downstream_) return Demand::None();

      // TODO: Define Optional<T>. (Or require C++17 for std::optional?)
      using OutputStorage =
          typename std::aligned_storage<sizeof(Output), alignof(Output)>::type;
      OutputStorage value_storage;
      Output* value = nullptr;     // Track whether we have a value.
      std::exception_ptr failure;  // Track whether we have a failure.
      try {
        // Use placement new to initialize the value from the transform output.
        // Placement new must be the last statement of the try block, so that
        // the catch block can assume the value is not initialized.
        value = reinterpret_cast<Output*>(&value_storage);
        new (value) Output(transform_->Invoke(std::move(element)));
      } catch (...) {
        value = nullptr;
        failure = std::current_exception();
      }

      Demand demand = Demand::None();
      if (failure) {
        // Leave downstream_ as nullptr to avoid sending any further signals.
        auto downstream = std::move(downstream_);
        downstream->Receive(Completion::Failure(failure));
      } else {
        demand = downstream_->Receive(std::move(*value));
        value->~Output();
      }

      return demand;
    }

    void Receive(Completion completion) override {
      if (downstream_) {
        downstream_->Receive(std::move(completion));
      }
    }

   private:
    std::shared_ptr<Transform<Input, Output>> transform_;
    std::shared_ptr<Subscriber<Output>> downstream_;
    std::exception_ptr failure_;
  };

  std::shared_ptr<Publisher<Input>> upstream_;
  std::shared_ptr<Transform<Input, Output>> transform_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // ML_NEURAL_NET_COMBINE_MAP_HPP_
