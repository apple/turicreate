/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <array>
#include <limits>
#include <type_traits>
#include <vector>

#include <core/util/Verify.hpp>

namespace turi {

constexpr std::size_t DynamicExtent = std::numeric_limits<std::size_t>::max();

namespace span_helpers {

//----------------------------------------------------------------------
// helper traits
//----------------------------------------------------------------------

template <size_t Extent>
struct IsDynamicExtent {
  static constexpr bool value = false;
};

template <>
struct IsDynamicExtent<DynamicExtent> {
  static constexpr bool value = true;
};

template <size_t Index, size_t Extent>
struct IsIndexValid {
  static constexpr bool value = (Index < Extent);
};

template <size_t Index>
struct IsIndexValid<Index, DynamicExtent> {
  static constexpr bool value = false;
};

//----------------------------------------------------------------------
// helper storage size
//----------------------------------------------------------------------

template <size_t Extent>
class SpanSize final {
 public:
  SpanSize() = default;
  ~SpanSize() = default;

  constexpr size_t Size() const noexcept { return size_; }

 private:
  static constexpr size_t size_ = Extent;
};

template <>
class SpanSize<DynamicExtent> final {
 public:
  SpanSize() = delete;
  ~SpanSize() = default;

  SpanSize(size_t size)
    : size_(size)
  {
  }

  size_t Size() const noexcept { return size_; }

 private:
  size_t size_;
};

}  // namespace span_helpers

//----------------------------------------------------------------------
// Span<T, Extent> is a custom implementation of an array view, similar
// to std::span introduced in C++20.
//
// If Extent is specified, Span supports compile-time bounds checking
// when the Get<> method is used.
//
// This version of Span also supports iterating slices and dimensions
// of multi-dimensional contiguous memory blocks.
//----------------------------------------------------------------------

template <typename T, size_t Extent = DynamicExtent>
class Span final {
 public:
  using value_type = T;
  using pointer = typename std::add_pointer<value_type>::type;
  using reference = typename std::add_lvalue_reference<value_type>::type;
  using iterator = pointer;

  using const_value_type = typename std::add_const<value_type>::type;
  using const_pointer = typename std::add_pointer<const_value_type>::type;
  using const_iterator = const_pointer;

  template <size_t Extent_>
  using SpanSize = span_helpers::SpanSize<Extent_>;

  template <size_t Extent_>
  using IsDynamicExtent = span_helpers::IsDynamicExtent<Extent_>;

  template <size_t Index, size_t Extent_>
  using IsIndexValid = span_helpers::IsIndexValid<Index, Extent_>;

  class SliceIterator final {
   public:
    SliceIterator(pointer p, size_t stride)
      : ptr_(p)
      , stride_(stride)
    {
    }

    bool operator==(const SliceIterator& other) const noexcept
    {
      return ptr_ == other.ptr_ && stride_ == other.stride_;
    }

    bool operator!=(const SliceIterator& other) const noexcept { return !(*this == other); }

    SliceIterator& operator++() noexcept
    {
      ptr_ += stride_;
      return *this;
    }

    SliceIterator operator++(int) const noexcept { return SliceIterator(ptr_ + stride_, stride_); }

    Span<T> operator*() const { return Span<T>(ptr_, stride_); }

   private:
    pointer ptr_;
    size_t stride_;
  };

  template <size_t Stride>
  class StaticSliceIterator final {
   public:
    StaticSliceIterator(pointer p)
      : ptr_(p)
    {
    }

    bool operator==(const StaticSliceIterator<Stride>& other) const noexcept
    {
      return ptr_ == other.ptr_;
    }

    bool operator!=(const StaticSliceIterator<Stride>& other) const noexcept
    {
      return !(*this == other);
    }

    StaticSliceIterator& operator++() noexcept
    {
      ptr_ += Stride;
      return *this;
    }

    StaticSliceIterator operator++(int) const noexcept
    {
      return StaticSliceIterator<Stride>(ptr_ + Stride);
    }

    Span<T, Stride> operator*() const { return Span<T, Stride>(ptr_); }

   private:
    pointer ptr_;
  };

  template <typename Iterator>
  class IteratorProvider final {
   public:
    IteratorProvider(Iterator begin, Iterator end)
      : begin_(begin)
      , end_(end)
    {
    }

    Iterator begin() const { return begin_; }

    Iterator end() const { return end_; }

   private:
    Iterator begin_;
    Iterator end_;
  };

  ~Span() = default;

  Span(const Span<T, Extent>&) = default;
  Span(Span<T, Extent>&&) = default;

  Span<T, Extent>& operator=(const Span<T, Extent>&) = default;
  Span<T, Extent>& operator=(Span<T, Extent>&&) = default;

  template <size_t Extent__ = Extent,
            typename std::enable_if<IsDynamicExtent<Extent__>::value, int>::type = 0>
  Span()
    : ptr_(nullptr)
    , size_(0)
  {
  }

  template <size_t Extent__ = Extent,
            typename std::enable_if<!IsDynamicExtent<Extent__>::value, int>::type = 0>
  Span(pointer p)
    : ptr_(p)
  {
  }

  template <size_t Extent__ = Extent,
            typename std::enable_if<IsDynamicExtent<Extent__>::value, int>::type = 0>
  Span(pointer p, size_t size)
    : ptr_(size == 0 ? nullptr : p)
    , size_(size)
  {
  }

  //
  // properties
  //

  pointer Data() const noexcept { return ptr_; }

  size_t Size() const noexcept { return size_.Size(); }

  constexpr bool IsEmpty() const noexcept { return Size() == 0; }

  //
  // random access
  //

  reference operator[](size_t index) const
  {
    VerifyDebugIsTrue(index < Size(), TuriErrorCode::IndexOutOfBounds);
    return ptr_[index];
  }

  reference At(size_t index) const
  {
    VerifyIsTrue(index < Size(), TuriErrorCode::IndexOutOfBounds);
    return ptr_[index];
  }

  // Get<N>() returns a reference to the value at index N.
  // This method only exists for fixed-sized Span instantiations.
  // The bounds of N are compile-time checked.
  template <size_t Index, typename std::enable_if<!IsDynamicExtent<Extent>::value &&
                                                      IsIndexValid<Index, Extent>::value,
                                                  int>::type = 0>
  reference Get() const
  {
    return (*this)[Index];
  }

  //
  // slicing
  //

  Span<T> Slice(size_t index) const
  {
    VerifyIsTrue(index < Size(), TuriErrorCode::IndexOutOfBounds);
    return Span<T>(Data() + index, Size() - index);
  }

  Span<T> Slice(size_t index, size_t size) const
  {
    VerifyIsTrue(size > 0 && index < Size() && index + size <= Size(),
                 TuriErrorCode::IndexOutOfBounds);
    return Span<T>(Data() + index, size);
  }

  Span<T> SliceByDimension(size_t num_slices, size_t slice_index) const
  {
    VerifyIsTrue(Size() % num_slices == 0, TuriErrorCode::IndexOutOfBounds);
    size_t stride = Size() / num_slices;
    return Slice(slice_index * stride, stride);
  }

  //
  // reinterpreting data
  //

  template <size_t NewExtent>
  Span<T, NewExtent> StaticResize() const
  {
    VerifyIsTrue(NewExtent <= Size(), TuriErrorCode::IndexOutOfBounds);
    return Span<T, NewExtent>(Data());
  }

  //
  // basic C++ iterators
  //

  iterator begin() const noexcept { return Data(); }

  iterator end() const noexcept { return Data() + Size(); }

  const_iterator cbegin() const noexcept { return Data(); }

  const_iterator cend() const noexcept { return Data() + Size(); }

  //
  // complex C++ iterators
  //

  IteratorProvider<SliceIterator> IterateSlices(size_t sliceSize) const
  {
    VerifyIsTrue(Size() % sliceSize == 0, TuriErrorCode::IndexOutOfBounds);

    return IteratorProvider<SliceIterator>(SliceIterator(Data(), sliceSize),
                                           SliceIterator(Data() + Size(), sliceSize));
  }

  template <size_t SliceSize>
  IteratorProvider<StaticSliceIterator<SliceSize>> IterateSlices() const
  {
    VerifyIsTrue(Size() % SliceSize == 0, TuriErrorCode::IndexOutOfBounds);

    return IteratorProvider<StaticSliceIterator<SliceSize>>(
        StaticSliceIterator<SliceSize>(Data()), StaticSliceIterator<SliceSize>(Data() + Size()));
  }

  IteratorProvider<SliceIterator> IterateByDimension(size_t dim) const
  {
    return IterateSlices(Size() / dim);
  }

 private:
  pointer ptr_;
  SpanSize<Extent> size_;
};

// MakeSpan for std::vector<T> yields Span<T, DynamicExtent>
// Examples:
// (1) create a mutable span
//   std::vector<int> v = { 1, 2, 3 };
//   auto span = MakeSpan(v); // span is Span<int>
// (2) create an immutable span from a mutable vector
//   std::vector<int> v = { 1, 2, 3 };
//   auto span = MakeSpan<const int>(v); // span is Span<const int>
// (3) create an immutable span
//   const std::vector<int> v = { 1, 2, 3 };
//   auto span = MakeSpan(v); // span is Span<const int>

template <typename T>
Span<T> MakeSpan(std::vector<T>& v) noexcept
{
  return Span<T>(v.data(), v.size());
}

template <typename T, typename MutableT = typename std::remove_const<T>::type>
Span<T> MakeSpan(const std::vector<MutableT>& v) noexcept
{
  return Span<T>(v.data(), v.size());
}

template <typename T, typename ConstT = typename std::add_const<T>::type>
Span<ConstT> MakeSpan(const std::vector<T>& v) noexcept
{
  return Span<ConstT>(v.data(), v.size());
}

// MakeSpan for std::array<T, N> yields Span<T, N>.
// Examples:
// (1) create a mutable span
//   std::array<int, 3> v = { 1, 2, 3 };
//   auto span = MakeSpan(v); // span is Span<int, 3>
// (2) create an immutable span from a mutable vector
//   std::array<int, 3> v = { 1, 2, 3 };
//   auto span = MakeSpan<const int>(v); // span is Span<const int, 3>
// (3) create an immutable span
//   const std::array<int, 3> v = { 1, 2, 3 };
//   auto span = MakeSpan(v); // span is Span<const int, 3>

template <typename T, size_t N>
Span<T, N> MakeSpan(std::array<T, N>& v) noexcept
{
  return Span<T, N>(v.data());
}

template <typename T, size_t N, typename MutableT = typename std::remove_const<T>::type>
Span<T, N> MakeSpan(const std::array<MutableT, N>& v) noexcept
{
  return Span<T, N>(v.data());
}

template <typename T, size_t N, typename ConstT = typename std::add_const<T>::type>
Span<ConstT, N> MakeSpan(const std::array<T, N>& v) noexcept
{
  return Span<ConstT, N>(v.data());
}

}  // namespace turi
