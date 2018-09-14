#include "mps_float_array.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace turi {
namespace mps {

namespace {

#ifndef NDEBUG
bool is_all_positive(const size_t* shape, size_t dim) {
  return std::all_of(shape, shape + dim, [](size_t s) { return s > 0; });
}
#endif

size_t product(const size_t* shape, size_t dim) {
  return std::accumulate(shape, shape + dim, 1,
                         [](size_t a, size_t b) { return a * b; });
}

}  // namespace

external_float_array::external_float_array(const float* data, size_t size,
                                           const size_t* shape, size_t dim)
  : data_(data), size_(size), shape_(shape), dim_(dim)
{
  assert(is_all_positive(shape, dim));
  assert(size == product(shape, dim));
}

}  // namespace mps
}  // namespace turi
