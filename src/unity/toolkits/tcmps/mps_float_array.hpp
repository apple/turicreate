#ifndef MPS_FLOAT_ARRAY_HPP_
#define MPS_FLOAT_ARRAY_HPP_

#include <cstddef>

namespace turi {
namespace mps {

// Pure virtual (but low-level) interface for an n-dimensional array. The inputs
// and outputs of the TCMPS library are largely expressed with this type.
class float_array {
public:
  virtual ~float_array() = default;

  // Returns a pointer to the first float value in the data. This pointer is
  // guaranteed to remain valid for the lifetime of this float_array instance.
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

}  // namespace mps
}  // namespace turi

#endif  // MPS_FLOAT_ARRAY_HPP_
