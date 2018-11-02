/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_FLOAT_ARRAY_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_FLOAT_ARRAY_HPP_

#include <cstddef>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace turi {
namespace neural_net {

// Pure virtual (but low-level) interface for an n-dimensional array. The inputs
// and outputs of the TCMPS library are largely expressed with this type.
class float_array {
public:
  virtual ~float_array() = default;

  // Returns a pointer to the first float value in the data. This pointer is
  // guaranteed to remain valid for the lifetime of this float_array instance.
  // Note that for some implementations, calling this function may trigger
  // synchronization with (waiting for) a thread writing the data.
  virtual const float* data() const = 0;

  // Returns the total number of float values present, beginning at the pointer
  // returned by data(). This number must equal the product of all the sizes in
  // the shape array.
  virtual size_t size() const = 0;

  // Returns a pointer to the first element of the shape array. This pointer is
  // guaranteed to remain valid for the lifetime of this float_array instance.
  virtual const size_t* shape() const = 0;

  // Returns the total number of elements in the array returned by shape().
  virtual size_t dim() const = 0;
};

// Wrapper around raw C pointers into an external n-dimensional array. Users
// must manually ensure that the external array outlives instances of this
// wrapper.
class external_float_array: public float_array {
public:
  external_float_array(const float* data, size_t size, const size_t* shape,
                       size_t dim);

  explicit external_float_array(const float_array& array)
    : external_float_array(array.data(), array.size(), array.shape(),
                           array.dim())
  {}

  const float* data() const override { return data_; }
  size_t size() const override { return size_; }

  const size_t* shape() const override { return shape_; }
  size_t dim() const override { return dim_; }

private:
  const float* data_ = nullptr;
  size_t size_ = 0;

  const size_t* shape_ = nullptr;
  size_t dim_ = 0;
};

// A float_array implementation that directly owns the memory containing the
// float data.
class float_buffer: public float_array {
public:
  // Copies enough float values from `data` to fill the given `shape`.
  float_buffer(const float* data, std::vector<size_t> shape);

  // Adopts an existing float vector `data`, which must have size consistent
  // with the provided `shape`.
  float_buffer(std::vector<float> data, std::vector<size_t> shape);

  const float* data() const override { return data_.data(); }
  size_t size() const override { return size_; }

  const size_t* shape() const override { return shape_.data(); }
  size_t dim() const override { return shape_.size(); }

private:
  std::vector<size_t> shape_;
  size_t size_ = 0;
  std::vector<float> data_;
};

// A float_array implementation that just wraps a single scalar value.
class float_scalar: public float_array {
public:
  float_scalar() = default;

  float_scalar(float value): value_(value) {}

  const float* data() const override { return &value_; }
  size_t size() const override { return 1; }

  const size_t* shape() const override { return nullptr; }
  size_t dim() const override { return 0; }

private:
  float value_ = 0.f;
};

// A float_array implementation that maintains a view into another float_array
// (that is possibly shared with others shared_float_array instances). Instances
// of this class can be efficiently copied (in constant time and incurring no
// additional allocations).
class shared_float_array: public float_array {
public:
  // Convenience functions for creating a shared float_buffer.
  static shared_float_array copy(const float* data, std::vector<size_t> shape) {
    return shared_float_array(
        std::make_shared<float_buffer>(data, std::move(shape)));
  }
  static shared_float_array wrap(std::vector<float> data,
                                 std::vector<size_t> shape) {
    return shared_float_array(
        std::make_shared<float_buffer>(std::move(data), std::move(shape)));
  }
  static shared_float_array wrap(float value) {
    return shared_float_array(std::make_shared<float_scalar>(value));
  }

  // Simply wraps an existing float_array shared_ptr.
  explicit shared_float_array(std::shared_ptr<float_array> impl)
    : shared_float_array(impl, /* offset */ 0, impl->shape(), impl->dim())
  {}

  // Creates an array containing the scalar 0.f.
  shared_float_array(): shared_float_array(default_value()) {}

  const float* data() const override { return impl_->data() + offset_; }
  size_t size() const override { return size_; }

  const size_t* shape() const override { return shape_; }
  size_t dim() const override { return dim_; }

  // TODO: Operations such as reshape, slice, etc.?

protected:
  shared_float_array(std::shared_ptr<float_array> impl, size_t offset,
                     const size_t* shape, size_t dim);

private:
  static std::shared_ptr<float_array> default_value();

  std::shared_ptr<float_array> impl_;

  size_t offset_ = 0;
  const size_t* shape_ = nullptr;
  size_t dim_ = 0;
  size_t size_ = 0;
};

// A float_array implementation that wraps a future shared_float_array.
class deferred_float_array: public float_array {
public:
  // Wraps `data_future`, which must have a (future) shape matching the provided
  // (known upfront) `shape`.
  deferred_float_array(std::shared_future<shared_float_array> data_future,
                       std::vector<size_t> shape);

  // Waits for the data future if necessary.
  const float* data() const override;

  // The size and shape of the array are known at construction time.
  size_t size() const override { return size_; }
  const size_t* shape() const override { return shape_.data(); }
  size_t dim() const override { return shape_.size(); }

private:
  std::shared_future<shared_float_array> data_future_;
  std::vector<size_t> shape_;
  size_t size_ = 0;
};

// Convenient typedef for data structure used to pass configuration and weights.
using float_array_map = std::map<std::string, shared_float_array>;

}  // namespace neural_net
}  // namespace turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_FLOAT_ARRAY_HPP_
