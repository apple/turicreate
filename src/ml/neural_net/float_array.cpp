/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/float_array.hpp>

#include <algorithm>
#include <cassert>
#include <numeric>

#include <core/logging/assertions.hpp>

namespace turi {
namespace neural_net {

namespace {

#ifndef NDEBUG
bool is_positive(size_t x) { return x > 0; }
#endif

size_t multiply(size_t a, size_t b) { return a * b; }

template <class T>
std::future<T> wrap_as_future(T val) {
  std::promise<T> promise;
  promise.set_value(std::move(val));
  return promise.get_future();
}

}  // namespace

external_float_array::external_float_array(const float* data, size_t size,
                                           const size_t* shape, size_t dim)
  : data_(data), size_(size), shape_(shape), dim_(dim)
{
  assert(std::all_of(shape, shape + dim, is_positive));
  assert(size == std::accumulate(shape, shape + dim, 1u, multiply));
}

float_buffer::float_buffer(const float* data, std::vector<size_t> shape)
  : shape_(std::move(shape)),
    size_(std::accumulate(shape_.begin(), shape_.end(), 1u, multiply)),
    data_(data, data + size_)
{
  assert(size_ > 0);
}

float_buffer::float_buffer(std::vector<float> data, std::vector<size_t> shape)
  : shape_(std::move(shape)),
    size_(std::accumulate(shape_.begin(), shape_.end(), 1u, multiply)),
    data_(std::move(data))
{
  assert(data_.size() == size_);
}

shared_float_array::shared_float_array(
    std::shared_ptr<float_array> impl, size_t offset, const size_t* shape,
    size_t dim)
  : impl_(std::move(impl)),
    offset_(offset),
    shape_(shape),
    dim_(dim),
    size_(std::accumulate(shape_, shape_ + dim_, 1u, multiply))
{
  // The provided data array must be a view into the impl's data array.
  assert(offset_ + size_ <= impl_->size());

  // The provided shape array must be a view into the impl's shape array.
  assert(impl_->shape() <= shape_);
  assert(shape_ + dim_ <= impl_->shape() + impl_->dim());
}

shared_float_array shared_float_array::operator[](size_t idx) const {

  ASSERT_GT(dim_, 0);
  ASSERT_LT(idx, shape_[0]);

  size_t stride = size_ / shape_[0];
  size_t offset = stride * idx;
  return shared_float_array(impl_, offset_ + offset, shape_ + 1, dim_ - 1);
}

std::ostream &operator<<(std::ostream &os, const float_array &arr) {
  const size_t *sh = arr.shape();
  os << "[";
  for (size_t i = 0; i < arr.dim(); i++) {
    os << sh[i] << " ";
  }
  os << "]";

  const float *ptr = arr.data();
  for (size_t i = 0; i < arr.size(); i++) {
    os << ptr[i] << " ";
  }
  os << "\n";
  return os;
}

void shared_float_array::save(oarchive& oarc) const {
  // Write shape.
  serialize_iterator(oarc, shape(), shape() + dim(), dim());

  // Write data.
  serialize_iterator(oarc, data(), data() + size(), size());
}

void shared_float_array::load(iarchive& iarc) {
  // Read shape.
  std::vector<size_t> shape;
  iarc >> shape;

  // Read data.
  std::vector<float> data;
  iarc >> data;

  // Overwrite self with a new float_array wrapping the deserialized data.
  *this = wrap(std::move(data), std::move(shape));
}

// static
std::shared_ptr<float_array> shared_float_array::default_value() {
  // n.b. static variables should have trivial destructors
  static const std::shared_ptr<float_array>* const singleton =
      new std::shared_ptr<float_array>(std::make_shared<float_scalar>(0.f));
  return *singleton;
}

deferred_float_array::deferred_float_array(
    std::shared_future<shared_float_array> data_future,
    std::vector<size_t> shape)
  : data_future_(std::move(data_future)),
    shape_(std::move(shape)),
    size_(std::accumulate(shape_.begin(), shape_.end(), 1u, multiply))
{}

deferred_float_array::deferred_float_array(shared_float_array params)
  : deferred_float_array(wrap_as_future(params),
                         std::vector<size_t>(params.shape(),
                                             params.shape() + params.dim()))
{}

const float* deferred_float_array::data() const {
  const shared_float_array& float_array = data_future_.get();
  assert(size_ == float_array.size());
  assert(shape_.size() == float_array.dim());
  assert(std::equal(shape_.begin(), shape_.end(), float_array.shape()));
  return float_array.data();
}

}  // namespace neural_net
}  // namespace turi
