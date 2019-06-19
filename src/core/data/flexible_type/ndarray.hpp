/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_NDARRAY
#define TURI_FLEXIBLE_TYPE_NDARRAY
#include <tuple>
#include <iostream>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
namespace turi {
namespace flexible_type_impl {


template <typename T>
static void mod_helper(T& a, const T& b,
                       typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
  a = fmod(a,b);
}
template <typename T>
static void mod_helper(T& a, const T& b,
                       typename std::enable_if<!std::is_floating_point<T>::value>::type* = 0) {
  a %= b;
}
/**
 * A generic dense multidimensional array.
 *
 * This class implements a very minimal generic dense multidimensional
 * array type.
 *
 * The basic layout is simple.
 *  - elems: is a flattened array of all the elements
 *  - start: The offset of the 0th element in elems.
 *  - shape: is the dimensions of the ndarray. The product of all the values in
 *    shape should equal elems.length()
 *  - stride: is used to convert between ND-indices and element indices.
 *
 *
 *
 * Array indexing is computed as follows:
 *
 *   ndarray[i,j,k] = elements[start + i * stride[0] + j * stride[1] + k * stride[2]]
 *
 * Note the stride are not based on the element size. i.e. if we have a simple
 * 1-D array, stride[0] is always 1. (as opposed to sizeof(T)).
 *
 * The NDArray's default construction layout is "C" ordering: the stride
 * array is non-increasing. That is, for a simple 3D ndarray[i,j,k], elements
 * ndarray[i,j,k], ndarray[i,j,k+1] are adjacent.
 *
 * However, due to to explicit representation of offset, shape and stride, almost
 * arbitrary layouts and slices can be represented by the ndarray.
 *
 * The current implementation of the ndarray enforces that all mutation
 * operations require elems to be unique (elems.use_count() == 1). Hence views
 * are not modifiable; modifying a view automtacally incurs a data copy. This
 * might be relaxed in the future (perhaps with a inherited view<T> type) but
 * this assumption allows for simpler memory management right now.
 *
 * Note
 * ----
 * The performance of the ndarray operators is probably not particularly
 * optimized. Really, the \ref increment_index() system should be replaced
 * with an iterator.
 **/
template <typename T>
class ndarray {
 public:
  typedef size_t index_type;
  typedef T value_type;
  typedef std::vector<index_type> index_range_type;
  typedef std::vector<T> container_type;

 private:
  std::shared_ptr<container_type> m_elem;
  index_range_type m_shape;
  index_range_type m_stride;
  index_type m_start = 0;

 public:

  /// construct with custom stride ordering
  ndarray(const container_type& elements = container_type(),
          const index_range_type& shape = index_range_type(),
          const index_range_type& stride = index_range_type(),
          const index_type start = 0):
              ndarray(std::make_shared<container_type>(elements), shape, stride, start) {}


  /// construct with custom stride ordering
  ndarray(const std::shared_ptr<container_type>& elements,
          const index_range_type& shape = index_range_type(),
          const index_range_type& stride = index_range_type(),
          const index_type start = 0):
              m_elem(elements), m_shape(shape), m_stride(stride), m_start(start) {

    // construct m_shape if not given
    if (m_shape.size() == 0 && elements->size() - m_start > 0) {
      m_shape.push_back(elements->size() - m_start);
    }
    // construct m_stide if not given
    if (m_stride.size() == 0 && m_shape.size() > 0) {
      m_stride.resize(m_shape.size());
      int i = m_shape.size() - 1;
      m_stride[i] = 1;
      --i;
      for (;i >= 0; --i) {
        m_stride[i] = m_stride[i + 1] * m_shape[i + 1];
      }
    }

    // if any shape axis is empty
    bool empty = m_shape.size() == 0;
    for (size_t i = 0;i < m_shape.size(); ++i) {
      if (m_shape[i] == 0) {
        empty = true;
      }
    }

    if (empty) {
      m_elem->clear();
      m_shape.clear();
      m_stride.clear();
      m_start = 0;
    }

    ASSERT_TRUE(is_valid());
    for (size_t i = 0;i < m_shape.size(); ++i) {
      ASSERT_TRUE(m_shape[i] > 0);
    }
  }


  /// Construct a new ndarray filled with a default value
  ndarray(const index_range_type& shape, const index_range_type& stride,
          const value_type& default_value) {

    if(shape.empty()) {
      return;
    }

    index_type total_size = 1;

    for(const index_type& t : shape) {
      total_size *= t;
    }

    *this = ndarray(std::make_shared<container_type>(total_size, default_value),
                    shape, stride, 0);
  }

  /// Construct a new ndarray filled with a default value
  ndarray(const index_range_type& shape,
          const value_type& default_value)
    : ndarray(shape, {}, default_value)
    {}

  ndarray(const ndarray<T>&) = default;
  ndarray(ndarray<T>&&) = default;
  ndarray& operator=(const ndarray<T>&) = default;
  ndarray& operator=(ndarray<T>&&) = default;

  /**
   * Ensures that m_elem is a unique copy.
   */
  void ensure_unique() {
    if (m_elem.use_count() > 1) {
      m_elem = std::make_shared<container_type>(*m_elem);
    }
  }

  /**
   * Returns true if the ndarray is empty / has no elements.
   */
  bool empty() const {
    if (m_elem->empty()) {
      return true;
    } else {
      return num_elem() == 0;
    }
  }

  /**
   * Returns the linear index given an N-d index
   * performing bounds checking on the index ranges.
   *
   * \code
   * std::vector<size_t> indices = {1,5,2};
   * arr.at(arr.index(indices)) = 10 // also bounds check the linear index
   * arr[arr.index(indices)] = 10    // does not bounds check the linear index
   * \endcode
   */
  template <typename U>
  index_type index(const std::vector<U>& index) const {
    ASSERT_EQ(m_stride.size(), index.size());

    size_t idx = 0;
    for (size_t i = 0; i < index.size(); ++i) {
      index_type v = index[i];
      ASSERT_LT(v, m_shape[i]);
      idx += v * m_stride[i];
    }
    return idx;
  }

  /**
   * Returns the linear index given an N-d index
   * without performing bounds checking on the index ranges.
   *
   * \code
   * std::vector<size_t> indices = {1,5,2};
   * arr[arr.fast_index(indices)] = 10 // does not bounds check the linear index
   * arr.at(arr.fast_index(indices)) = 10 // bounds check the linear index
   * \endcode
   */
  template <typename U>
  index_type fast_index(const std::vector<U>& index) const {
    size_t idx = 0;
    for (size_t i = 0; i < index.size(); ++i) {
      index_type v = index[i];
      idx += v * m_stride[i];
    }
    return idx;
  }

  /**
   * Returns a reference to an element given the linear index, no bounds
   * checking is performed.
   *
   * Note that if the memory used by this array is shared, this may have
   * unintentional side effects (changing other arrays).
   */
  value_type& operator[](size_t elem_index) {
    ensure_unique();
    return (*m_elem)[m_start + elem_index];
  }

  /**
   * Returns a const reference to an element given the linear index, no bounds
   * checking is performed.
   */
  const value_type& operator[](size_t elem_index) const {
    return (*m_elem)[m_start + elem_index];
  }

  /**
   * Returns a reference to an element given the linear index, performing bounds
   * checking on the index range.
   *
   * Note that if the memory used by this array is shared, this may have
   * unintentional side effects (changing other arrays).
   */
  value_type& at(size_t elem_index) {
    ensure_unique();
    ASSERT_LT(m_start + elem_index, m_elem->size());
    return (*m_elem)[m_start + elem_index];
  }

  /**
   * Returns a const reference to an element given the linear index, performing
   * checking on the index range.
   */
  const value_type& at(size_t elem_index) const {
    ASSERT_LT(m_start + elem_index, m_elem->size());
    return (*m_elem)[m_start + elem_index];
  }

  /**
   * Returns a reference to all the elements in a linear layout; if is_full()
   * is false, this will include unindexable elements.
   *
   * Note that if the memory used by this array is shared, this may have
   * unintentional side effects (changing other arrays).
   */
  container_type& raw_elements() {
    return *m_elem;
  }

  /**
   * Returns a reference to all the elements in a linear layout; if is_full()
   * is false, this will include unindexable elements.
   */
  const container_type& raw_elements() const {
    return *m_elem;
  }

  /**
   * Returns a reference to all the elements in a linear layout, is_full()
   * must be true.
   *
   * Note that if the memory used by this array is shared, this may have
   * unintentional side effects (changing other arrays).
   */
  container_type& elements() {
    ensure_unique();
    ASSERT_TRUE(is_full());
    return *m_elem;
  }
  /**
   * Returns a const reference to all the elements in a linear layout, is_full()
   * must be true.
   */
  const container_type& elements() const {
    ASSERT_TRUE(is_full());
    return *m_elem;
  }

  /**
   * Returns a const reference to the shape.
   */
  const index_range_type& shape() const {
    return m_shape;
  }

  /**
   * Returns a const reference to the stride.
   */
  const index_range_type& stride() const {
    return m_stride;
  }

  /**
   * Returns a const reference to the stride.
   */
  index_type start() const {
    return m_start;
  }

  /**
   * Returns the number of elements in the array.
   *
   * This is equivalent to the product of the values in the shape array.
   * Note that this may not be the same as elements().size().
   */
  size_t num_elem() const {
    if (m_shape.size() == 0) return 0;
    if (m_elem == nullptr) return 0;
    size_t p = 1;
    for (size_t s: m_shape) {
      p = p * s;
    }
    return p;
  }

  /**
   * Returns true if every element in elements() is reachable by an
   * N-d index.
   */
  bool is_full() const {
    return m_start == 0 &&
        num_elem() == m_elem->size() &&
        last_index() == m_elem->size();
  }

  /**
   * Returns true if the shape and stride of the array is laid out
   * correctly such at all array indices are within elements().size().
   *
   * An ndarray can be invalid for instance, if the stride is too large,
   * or if the shape is larger than the total number of elements.
   */
  bool is_valid() const {
    return m_shape.size() == m_stride.size() &&   // shape and stride line up
          num_elem() + m_start <= m_elem->size() &&  // num_elements (as computed by shape) is in m_elem
          last_index() + m_start <= m_elem->size();  // max index (as computed by stride( is in m_elem
  }

  /**
   * Returns true if the stride is ordered canonically.
   * The strides must be non-increasing and non-zero.
  */
  bool has_canonical_stride() const {
    if (m_stride.size() == 0) return true;
    if (m_stride[0] == 0) return false;
    for (size_t i = 1; i < m_stride.size(); ++i) {
      if (m_stride[i] == 0 || m_stride[i - 1] < m_stride[i]) {
        return false;
      }
    }
    return true;
  }

  /// Returns true if the nd-array is in canonical ordering
  bool is_canonical() const {
    return is_full() && has_canonical_stride();
  }

  /**
   * Increments a vector representing an N-D index.
   *
   * Assumes that the index is valid to begin with.
   * Returns 1 + [the index position we incremented] while we have not reached
   * the end of the array. Returns 0 once we increment past the end of the
   * array.
   */
  template <typename U, typename V>
  static size_t inline increment_index(std::vector<U>& idx, const std::vector<V>& _shape) {
    DASSERT_TRUE(idx.size() == _shape.size());
    int i = idx.size() - 1;
    for (;i >= 0 ; --i) {
      ++idx[i];
      if (idx[i] < _shape[i]) break;
      // we hit counter limit we need to advance the next counter;
      idx[i] = 0;
    }
    if (i < 0) return 0;
    else return i + 1;
  }

  /**
   * Increments a vector representing an N-D index.
   *
   * Assumes that the index is valid to begin with.
   * Returns 1 + [the index position we incremented] while we have not reached
   * the end of the array. Returns 0 once we increment past the end of the
   * array.
   */
  template <typename U>
  size_t inline increment_index(std::vector<U>& idx) const {
    return ndarray::increment_index(idx, m_shape);
  }

  /**
   * Returns an ndarray ordered canonically.
   *
   * The canonical ordering is full (\ref is_full()) and the stride array
   * is non-descending.
   *
   * Raises an exception if the array is not valid.
   *
   * \note The performance of this algorithm can probably be improved.
   */
  ndarray<T> canonicalize() const {
    if (is_canonical()) return (*this);
    ASSERT_TRUE(is_valid());

    ndarray<T> ret;
    ret.m_start = 0;
    ret.m_shape = m_shape;
    ret.m_elem->resize(num_elem());
    ret.m_stride.resize(m_shape.size());

    // empty array
    if (ret.m_shape.size() == 0 || ret.m_elem->size() == 0) {
      return ret;
    }

    // compute the stride
    int i = ret.m_shape.size() - 1;
    ret.m_stride[i] = 1;
    --i;
    for (;i >= 0; --i) {
      ret.m_stride[i] = ret.m_stride[i + 1] * ret.m_shape[i + 1];
    }

    std::vector<size_t> idx(m_shape.size(), 0);
    size_t ctr = 0;
    do {
      // directly referencing ret.m_elem is ok here because ret.m_start is 0
      (*ret.m_elem)[ctr] = (*this)[fast_index(idx)];
      ++ctr;
    } while(increment_index(idx));

    return ret;
  }

  /**
   * Forces this ndarray to be full by compacting if necessary.
   */
  void ensure_full() {
    if (!is_full()) {
      (*this) = compact();
    }
  }

  /**
   * Returns a compacted ndarray.
   *
   * A compacted NDArray has the same stride ordering as the original array,
   * but enforces that the array is full. This essentially means that the
   * elements array has the same order of elements, but skipped elements are
   * removed.
   *
   * Raises an exception if the array is not valid.
   *
   * \note The performance of this algorithm can probably be improved.
   */
  ndarray<T> compact() const {
    ASSERT_TRUE(is_valid());
    if (is_full()) return (*this);

    ndarray<T> ret;
    ret.m_start = 0;
    ret.m_shape = m_shape;
    ret.m_elem->resize(num_elem());
    ret.m_stride.resize(m_shape.size());

    // empty array
    if (ret.m_shape.size() == 0 || ret.m_elem->size() == 0) {
      return ret;
    }

    std::vector<std::pair<size_t, size_t>> stride_ordering(m_stride.size());
    for (size_t i = 0;i < m_stride.size(); ++i) stride_ordering[i] = {m_stride[i], i};
    std::sort(stride_ordering.rbegin(), stride_ordering.rend());

    // compute the stride
    ret.m_stride[stride_ordering[0].second] = 1;
    for (size_t i = 1;i < m_stride.size(); ++i) {
      ret.m_stride[stride_ordering[i].second] =
          ret.m_stride[stride_ordering[i - 1].second] * ret.m_shape[stride_ordering[i - 1].second];
    }

    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      // directly referencing ret.m_elem is ok here because ret.m_start is 0
      (*ret.m_elem)[ret.fast_index(idx)] = (*this)[fast_index(idx)];
    } while(increment_index(idx));

    return ret;
  }

  /// serializer
  void save(oarchive& oarc) const {
    ASSERT_TRUE(is_valid());
    oarc << char(0);
    if (is_full()) {
      oarc << m_shape;
      oarc << m_stride;
      oarc << *m_elem;
    } else {
      ndarray<T> c = compact();
      ASSERT_TRUE(c.is_full());
      oarc << c.m_shape;
      oarc << c.m_stride;
      oarc << *(c.m_elem);
    }
  }

  /// deserializer
  void load(iarchive& iarc) {
    char c;
    iarc >> c;
    ASSERT_TRUE(c == 0);
    m_start = 0;
    iarc >> m_shape;
    iarc >> m_stride;
    m_elem = std::make_shared<container_type>();
    iarc >> *m_elem;
  }

  /**
   * Return true if this ndarray has the same shape as another ndarray.
   */
  bool same_shape(const ndarray<T>& other) const {
    if (m_shape.size() != other.m_shape.size()) return false;
    for (size_t i = 0;i < m_shape.size(); ++i) {
      if (m_shape[i] != other.m_shape[i]) return false;
    }
    return true;
  }

  /// element-wise addition. The other array must have the same shape.
  ndarray<T>& operator+=(const ndarray<T>& other) {
    ASSERT_TRUE(same_shape(other));
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] += other[other.fast_index(idx)];
    } while(increment_index(idx));
    return *this;
  }

  /// scalar addition.
  ndarray<T>& operator+=(T other) {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] += other;
    } while(increment_index(idx));
    return *this;
  }

  /// element-wise subtraction. The other array must have the same shape.
  ndarray<T>& operator-=(const ndarray<T>& other) {
    ASSERT_TRUE(same_shape(other));
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] -= other[other.fast_index(idx)];
    } while(increment_index(idx));
    return *this;
  }

  /// scalar subtraction.
  ndarray<T>& operator-=(T other) {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] -= other;
    } while(increment_index(idx));
    return *this;
  }

  /// element-wise multiplication. The other array must have the same shape.
  ndarray<T>& operator*=(const ndarray<T>& other) {
    ASSERT_TRUE(same_shape(other));
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] *= other[other.fast_index(idx)];
    } while(increment_index(idx));
    return *this;
  }

  /// scalar multiplication
  ndarray<T>& operator*=(T other) {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] *= other;
    } while(increment_index(idx));
    return *this;
  }


  /// element-wise division. The other array must have the same shape.
  ndarray<T>& operator/=(const ndarray<T>& other) {
    ASSERT_TRUE(same_shape(other));
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] /= other[other.fast_index(idx)];
    } while(increment_index(idx));
    return *this;
  }

  /// scalar division
  ndarray<T>& operator/=(T other) {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      (*this)[fast_index(idx)] /= other;
    } while(increment_index(idx));
    return *this;
  }

  /// element-wise modulo. The other array must have the same shape.
  ndarray<T>& operator%=(const ndarray<T>& other) {
    ASSERT_TRUE(same_shape(other));
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      T& left = (*this)[fast_index(idx)];
      mod_helper(left, other[other.fast_index(idx)]);
    } while(increment_index(idx));
    return *this;
  }

  /// scalar modulo.
  ndarray<T>& operator%=(T other) {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      T& left = (*this)[fast_index(idx)];
      mod_helper(left, other);
    } while(increment_index(idx));
    return *this;
  }

  /// negation
  ndarray<T>& negate() {
    if (num_elem() == 0) return *this;
    ensure_unique();
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      T& v = (*this)[fast_index(idx)];
      v = -v;
    } while(increment_index(idx));
    return *this;
  }


  /// element-wise equality. The other array must have the same shape.
  bool operator==(const ndarray<T>& other) const {
    if (&other == this) return true;
    if (!same_shape(other)) return false;
    if (num_elem() == 0) return true;
    std::vector<size_t> idx(m_shape.size(), 0);
    do {
      if ((*this)[fast_index(idx)] != other[other.fast_index(idx)]) return false;
    } while(increment_index(idx));
    return true;
  }

  /// inverse of ==
  bool operator!=(const ndarray<T>& other) const {
    return !((*this) == other);
  }

  void print(std::ostream& os) const {
    std::vector<size_t> idx(m_shape.size(), 0);
    if (num_elem() == 0) os << "[]";

    // print all the open square brackets
    for (size_t i = 0;i < idx.size(); ++i) os << "[";
    size_t next_bracket_depth;
    bool is_first_element = true;
    do {
      if (is_first_element == false) os << ",";
      os << (*m_elem)[fast_index(idx)];
      is_first_element = false;
      next_bracket_depth = increment_index(idx);
      if (next_bracket_depth == 0) break;
      for (size_t i = next_bracket_depth ;i < idx.size(); ++i) os << "]";
      if (next_bracket_depth < idx.size()) os << ",";
      for (size_t i = next_bracket_depth;i < idx.size(); ++i) os << "[";
      if (next_bracket_depth < idx.size()) is_first_element = true;
    }while(1);
    for (size_t i = 0;i < idx.size(); ++i) os << "]";
  }

 private:
  /**
   * Returns one past the last valid linear index of the array according to the
   * shape and stride information.
   */
  size_t last_index() const {
    if (m_shape.size() == 0) return 0;
    size_t last_idx = 0;
    for (size_t i = 0; i < m_shape.size(); ++i) {
      last_idx += (m_shape[i]-1) * m_stride[i];
    }
    return last_idx + 1;
  }

};

// pointer to ndarray is constrained to pointer size
// to enforce that it will always fit in a flexible_type.
static_assert(sizeof(ndarray<int>*) == sizeof(size_t), "Pointer to ndarray must be the same size as size_t");

template <typename T>
std::ostream& operator<<(std::ostream& os, const ndarray<T>& n) {
  n.print(os);
  return os;
}

} // flexible_type_impl
} // namespace turi
#endif
