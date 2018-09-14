#include "mps_float_array.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace turi {
namespace mps {

namespace {

#ifndef NDEBUG
bool is_positive(size_t x) { return x > 0; }
#endif

size_t multiply(size_t a, size_t b) { return a * b; }

}  // namespace

external_float_array::external_float_array(const float* data, size_t size,
                                           const size_t* shape, size_t dim)
  : data_(data), size_(size), shape_(shape), dim_(dim)
{
  assert(std::all_of(shape, shape + dim, is_positive));
  assert(size == std::accumulate(shape, shape + dim, 1, multiply));
}

float_buffer::float_buffer(const float* data, std::vector<size_t> shape)
  : shape_(std::move(shape)),
    size_(std::accumulate(shape_.begin(), shape_.end(), 1, multiply)),
    data_(data, data + size_)
{
  assert(size_ > 0);
}

shared_float_array::shared_float_array(
    std::shared_ptr<float_array> impl, const float* data, const size_t* shape,
    size_t dim)
  : impl_(std::move(impl)),
    data_(data),
    shape_(shape),
    dim_(dim),
    size_(std::accumulate(shape_, shape_ + dim_, 1, multiply))
{
  // The provided data array must be a view into the impl's data array.
  assert(impl_->data() <= data_);
  assert(data_ + size_ <= impl_->data() + impl_->size());

  // The provided shape array must be a view into the impl's shape array.
  assert(impl_->shape() <= shape_);
  assert(shape_ + dim_ <= impl_->shape() + impl_->dim());
}

// static
std::shared_ptr<float_array> shared_float_array::default_value() {
  // n.b. static variables should have trivial destructors
  static const float default_scalar = 0.f;
  static const std::shared_ptr<float_array>* const singleton =
      new std::shared_ptr<float_array>(
          std::make_shared<external_float_array>(&default_scalar, /* size */ 1,
                                                 nullptr, /* dim */ 0));
  return *singleton;
}

}  // namespace mps
}  // namespace turi
