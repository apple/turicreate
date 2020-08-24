/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE SpanTests

#include <core/util/Span.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/std/is_detected.hpp>
#include <core/util/test_macros.hpp>

using turi::MakeSpan;
using turi::Span;

//----------------------------------------------------------------------
// Static-sizing interface tests
// (these will fail at compile time)
//----------------------------------------------------------------------

static_assert(!std::is_default_constructible<Span<int, 4>>::value,
              "Span<T, 4>() default constructor is illegal");

static_assert(!std::is_constructible<Span<int, 4>, int*, size_t>::value,
              "Span<T, 4>(ptr, size_t) constructor is illegal");

static_assert(!std::is_constructible<Span<int>, int*>::value,
              "Span<T, DynamicExtent>(ptr) constructor is illegal");

template <typename T>
using Has_SpanGet1_t = decltype(std::declval<T>().template Get<1>());

template <typename T>
using Has_SpanGet2_t = decltype(std::declval<T>().template Get<2>());

static_assert(!std::experimental::is_detected<Has_SpanGet1_t, Span<int>>::value,
              "Span<T, DynamicExtent>::Get<> is illegal");

static_assert(std::experimental::is_detected<Has_SpanGet1_t, Span<int, 2>>::value,
              "Span<T, 2>::Get<1> is legal");

static_assert(!std::experimental::is_detected<Has_SpanGet2_t, Span<int, 2>>::value,
              "Span<T, 2>::Get<2> is illegal");

//----------------------------------------------------------------------
// MakeSpan factory methods for std::vector
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestMakeSpanVectorMutable)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);
  static_assert(std::is_same<decltype(span)::value_type, int>::value,
                "MakeSpan should make non-const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
}

BOOST_AUTO_TEST_CASE(TestMakeSpanVectorForcedImmutable)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan<const int>(values);
  static_assert(std::is_same<decltype(span)::value_type, decltype(span)::const_value_type>::value,
                "MakeSpan should make const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
}

BOOST_AUTO_TEST_CASE(TestMakeSpanVectorImmutable)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);
  static_assert(std::is_same<decltype(span)::value_type, decltype(span)::const_value_type>::value,
                "MakeSpan should make const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
}

//----------------------------------------------------------------------
// MakeSpan factory methods for std::array
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestMakeSpanArrayMutable)
{
  std::array<int, 4> values = {{1, 2, 3, 4}};
  auto span = MakeSpan(values);
  static_assert(std::is_same<decltype(span)::value_type, int>::value,
                "MakeSpan should make non-const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
  TS_ASSERT_EQUALS(span.Get<0>(), 1);
}

BOOST_AUTO_TEST_CASE(TestMakeSpanArrayForcedImmutable)
{
  std::array<int, 4> values = {{1, 2, 3, 4}};
  auto span = MakeSpan<const int>(values);
  static_assert(std::is_same<decltype(span)::value_type, decltype(span)::const_value_type>::value,
                "MakeSpan should make const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
  TS_ASSERT_EQUALS(span.Get<0>(), 1);
}

BOOST_AUTO_TEST_CASE(TestMakeSpanArrayImmutable)
{
  const std::array<int, 4> values = {{1, 2, 3, 4}};
  auto span = MakeSpan(values);
  static_assert(std::is_same<decltype(span)::value_type, decltype(span)::const_value_type>::value,
                "MakeSpan should make const T");

  TS_ASSERT(!span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 4);
  TS_ASSERT_EQUALS(span.Data(), values.data());
  TS_ASSERT_EQUALS(span.Get<0>(), 1);
}

//----------------------------------------------------------------------
// Constructors and operators
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestDefaultConstructor)
{
  Span<int> span;
  TS_ASSERT(span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 0);
  TS_ASSERT_EQUALS(span.Data(), nullptr);
}

BOOST_AUTO_TEST_CASE(TestEmpty)
{
  // test 0 length but valud ptr
  std::vector<int> v = {1, 2, 3, 4};
  Span<int> span(v.data(), 0);
  TS_ASSERT(span.IsEmpty());
  TS_ASSERT_EQUALS(span.Size(), 0);
  TS_ASSERT_EQUALS(span.Data(), nullptr);
}

BOOST_AUTO_TEST_CASE(TestCopyAndAssignment)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  auto copied(span);
  TS_ASSERT_EQUALS(copied.Size(), 4);

  Span<int> other;
  other = copied;
  TS_ASSERT_EQUALS(other.Size(), 4);
}

//----------------------------------------------------------------------
// Random Access
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestAccessMutable)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  TS_ASSERT_EQUALS(span[0], 1);
  span[0] = 10;
  TS_ASSERT_EQUALS(span[0], 10);

#if !defined(NDEBUG)
  TS_ASSERT_THROWS_ANYTHING(span[5]);
#endif

  TS_ASSERT_EQUALS(span.At(1), 2);
  span.At(1) = 20;
  TS_ASSERT_EQUALS(span.At(1), 20);

  TS_ASSERT_THROWS_ANYTHING(span.At(5));
}

BOOST_AUTO_TEST_CASE(TestAccessImmutable)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  TS_ASSERT_EQUALS(span[0], 1);
  TS_ASSERT_EQUALS(span.At(1), 2);
  TS_ASSERT_THROWS_ANYTHING(span.At(5));

  static_assert(std::is_same<decltype(span[0]), const int&>::value,
                "span[0] should not be mutable");
  static_assert(std::is_same<decltype(span.At(0)), const int&>::value,
                "span.At(0) should not be mutable");
}

//----------------------------------------------------------------------
// Static-sized Random Access
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestStaticSizedAccessMutable)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values).StaticResize<4>();

  TS_ASSERT_EQUALS(span.Get<0>(), 1);

  span.Get<0>() = 10;
  TS_ASSERT_EQUALS(span.Get<0>(), 10);
}

BOOST_AUTO_TEST_CASE(TestStaticSizedAccessImmutable)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values).StaticResize<4>();

  TS_ASSERT_EQUALS(span.Get<0>(), 1);

  static_assert(std::is_same<decltype(span.Get<0>()), const int&>::value,
                "Get<0>() should be immutable");
}

//----------------------------------------------------------------------
// Immutable Iteration
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestIteratorImmutable)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 0;
  for (auto& i : span) {
    static_assert(std::is_same<decltype(i), const int&>::value, "iterator must be const");
    TS_ASSERT_EQUALS(i, ++counter);
  }
  TS_ASSERT_EQUALS(counter, 4);
}

BOOST_AUTO_TEST_CASE(TestIteratorImmutableExplicitBeginEnd)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 0;
  for (auto itr = span.begin(); itr != span.end(); ++itr) {
    static_assert(std::is_same<decltype(*itr), const int&>::value, "iterator must be const");
    TS_ASSERT_EQUALS(*itr, ++counter);
  }
  TS_ASSERT_EQUALS(counter, 4);
}

BOOST_AUTO_TEST_CASE(TestIteratorImmutableExplicitCbeginCend)
{
  const std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 0;
  for (auto itr = span.cbegin(); itr != span.cend(); ++itr) {
    static_assert(std::is_same<decltype(*itr), const int&>::value, "iterator must be const");
    TS_ASSERT_EQUALS(*itr, ++counter);
  }
  TS_ASSERT_EQUALS(counter, 4);
}

//----------------------------------------------------------------------
// Mutable Iteration
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestIteratorMutable)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 0;
  for (auto& i : span) {
    TS_ASSERT_EQUALS(i, ++counter);
    i++;
    TS_ASSERT_EQUALS(i, counter + 1);
    TS_ASSERT_EQUALS(span[counter - 1], i);
  }
  TS_ASSERT_EQUALS(counter, 4);
}

BOOST_AUTO_TEST_CASE(TestIteratorMutableExplicitBeginEnd)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 0;
  for (auto itr = span.begin(); itr != span.end(); ++itr) {
    TS_ASSERT_EQUALS(*itr, ++counter);
    (*itr)++;
    TS_ASSERT_EQUALS(*itr, counter + 1);
  }
  TS_ASSERT_EQUALS(counter, 4);
}

BOOST_AUTO_TEST_CASE(TestIteratorMutableExplicitCbeginCend)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  int counter = 1;
  for (auto itr = span.cbegin(); itr != span.cend(); ++itr) {
    static_assert(std::is_same<decltype(*itr), const int&>::value, "iterator must be const");
    TS_ASSERT_EQUALS(*itr, counter++);
  }
  TS_ASSERT_EQUALS(counter, 5);
}

//----------------------------------------------------------------------
// Slicing
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestSlicingZeroLength)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);
  TS_ASSERT_THROWS_ANYTHING(span.Slice(1, 0));
}

BOOST_AUTO_TEST_CASE(TestSlicingUnbounded)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  {
    auto sub = span.Slice(2);
    TS_ASSERT_EQUALS(sub.Size(), 2);
    TS_ASSERT_EQUALS(sub[0], 3);
    TS_ASSERT_EQUALS(sub[1], 4);
  }

  {
    auto sub = span.Slice(0);
    TS_ASSERT_EQUALS(sub.Size(), span.Size());
    TS_ASSERT_EQUALS(sub.Data(), span.Data());
  }
}

BOOST_AUTO_TEST_CASE(TestSlicingUnboundedEdge)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  auto sub = span.Slice(3);
  TS_ASSERT_EQUALS(sub.Size(), 1);
  TS_ASSERT_EQUALS(sub[0], 4);
}

BOOST_AUTO_TEST_CASE(TestSlicingIllegalBounds)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);
  TS_ASSERT_THROWS_ANYTHING(span.Slice(4));
  TS_ASSERT_THROWS_ANYTHING(span.Slice(0, 6));
}

BOOST_AUTO_TEST_CASE(TestSlicingBounded)
{
  std::vector<int> values = {1, 2, 3, 4};
  auto span = MakeSpan(values);

  {
    auto sub = span.Slice(1, 2);
    TS_ASSERT_EQUALS(sub.Size(), 2);
    TS_ASSERT_EQUALS(sub[0], 2);
    TS_ASSERT_EQUALS(sub[1], 3);

    auto subsub = sub.Slice(0, 1);
    TS_ASSERT_EQUALS(subsub.Size(), 1);
    TS_ASSERT_EQUALS(subsub[0], 2);
  }

  {
    auto sub = span.Slice(3, 1);
    TS_ASSERT_EQUALS(sub.Size(), 1);
    TS_ASSERT_EQUALS(sub[0], 4);
  }

  {
    auto sub = span.Slice(0, span.Size());
    TS_ASSERT_EQUALS(sub.Size(), span.Size());
    TS_ASSERT_EQUALS(sub.Data(), span.Data());
  }
}

BOOST_AUTO_TEST_CASE(TestSlicingByDimension)
{
  std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);

  {
    auto sub = span.SliceByDimension(3, 0);
    TS_ASSERT_EQUALS(sub.Size(), 2);
    TS_ASSERT_EQUALS(sub[0], 1);
    TS_ASSERT_EQUALS(sub[1], 2);
  }

  {
    auto sub = span.SliceByDimension(3, 2);
    TS_ASSERT_EQUALS(sub.Size(), 2);
    TS_ASSERT_EQUALS(sub[0], 5);
    TS_ASSERT_EQUALS(sub[1], 6);
  }
}

BOOST_AUTO_TEST_CASE(TestSlicingByInvalidDimension)
{
  std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);
  TS_ASSERT_THROWS_ANYTHING(span.SliceByDimension(4, 0));
}

BOOST_AUTO_TEST_CASE(TestSlicingByDimensionWithInvalidIndex)
{
  std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);
  TS_ASSERT_THROWS_ANYTHING(span.SliceByDimension(3, 3));
}

//----------------------------------------------------------------------
// Complex Iteration with Slicing
//----------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(TestIterationIllegal)
{
  const std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);
  TS_ASSERT_THROWS_ANYTHING(span.IterateSlices(5));
}

BOOST_AUTO_TEST_CASE(TestIterationStaticSlices)
{
  const std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);

  // iterate 3 values at a time
  int counter = 0;
  for (auto row : span.IterateSlices<2>()) {
    TS_ASSERT_EQUALS(row.Size(), 2);
    TS_ASSERT_EQUALS(row.Get<0>(), counter + 1);
    TS_ASSERT_EQUALS(row.Get<1>(), counter + 2);

    counter += 2;
  }
  TS_ASSERT_EQUALS(counter, 6);
}

BOOST_AUTO_TEST_CASE(TestIterationDynamicSlices)
{
  const std::vector<int> values = {1, 2, 3, 4, 5, 6};
  auto span = MakeSpan(values);

  // iterate 3 values at a time
  int counter = 0;
  for (auto row : span.IterateSlices(3)) {
    TS_ASSERT_EQUALS(row.Size(), 3);

    for (auto i : row) {
      TS_ASSERT_EQUALS(i, ++counter);
    }
  }
  TS_ASSERT_EQUALS(counter, 6);
}

BOOST_AUTO_TEST_CASE(TestIterationMultipleDims)
{
  // clang-format off
  const std::vector<int> values = {
    // shape: [2, 3, 4]
    /*0*/
      1, 2, 3, 4,
      5, 6, 7, 8,
      9, 10, 11, 12,
    /*1*/
      1, 2, 3, 4,
      5, 6, 7, 8,
      9, 10, 11, 12
  };
  // clang-format on

  const std::vector<int> expectedRowSums = {10, 26, 42};

  auto span = MakeSpan(values);

  for (auto span2d : span.IterateByDimension(2)) {
    TS_ASSERT_EQUALS(span2d.Size(), 12);

    size_t rowIndex = 0;
    for (auto row : span2d.IterateByDimension(3)) {
      TS_ASSERT_EQUALS(row.Size(), 4);

      int sum = 0;
      for (auto i : row) {
        sum += i;
      }

      TS_ASSERT_EQUALS(sum, expectedRowSums[rowIndex++]);
    }
  }
}
