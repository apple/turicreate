/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SITERABLE_ALGORITHMS_HPP
#define TURI_UNITY_SITERABLE_ALGORITHMS_HPP
#include <set>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <iterator>
#include <core/parallel/lambda_omp.hpp>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/siterable.hpp>
#include <core/storage/sframe_data/swriter_base.hpp>
#include <core/storage/sframe_data/sarray_reader.hpp>
#include <core/storage/sframe_data/is_sarray_like.hpp>
namespace turi {
/**
 * \ingroup sframe_physical
 * \addtogroup eager_algorithms Generic SFrame Eager Algorithms
 * \{
 */
/**************************************************************************/
/*                                                                        */
/*                      Implementation of transform                       */
/*                                                                        */
/**************************************************************************/

/**
 * Writes input to output calling the transformfn on each input emitting
 * the result to output.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in input:
 *    write transformfn(x) to output
 * \endcode
 *
 * The input object must be a descendent of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * The output object should be of the same number of segments as the input
 * object. If they are of different number of segments, this function will
 * attempt to change the number of segments of the output object. Changing the
 * number of segments is generally a successful operation unless writes have
 * already occured on the output. If the number of segments do not match,
 * and if the number of output segments cannot be set, this function
 * will throw a string exception and fail.
 *
 * \param input The input to read from. Must be a descendent of siterable
 * \param output The output writer to write to. Must be a descendent of swriter_base
 * \param transformfn The transform operation to perform on the input
 *                    to generate the output
 * \param constraint_segments The set of segments to operate on. If empty
 *                            (default) will operate on all segments. Only valid
 *                            segment numbers will be operated on.
 */
template <typename S, typename T, typename TransformFn,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void transform(S&& input, T&& output,
               TransformFn transformfn,
               std::set<size_t> constraint_segments = std::set<size_t>()) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  ASSERT_TRUE(output.is_opened_for_write());
  auto input_reader = input.get_reader(output.num_segments());
  // construct a vector containing a list of segments to operate on
  std::vector<size_t> segments;
  // if constraint segments is empty, we operate on all segments
  if (constraint_segments.empty()) {
    segments.resize(output.num_segments());
    for (size_t i = 0; i < segments.size(); ++i) {
      segments[i] = i;
    }
  } else {
    std::copy(constraint_segments.begin(),
              constraint_segments.end(),
              std::inserter(segments, segments.end()));
  }

  // perform a parallel for through the list of segments
  parallel_for(0, segments.size(),
               [&](size_t idx) {
                 size_t segid = segments[idx];
                 if (segid >= input_reader->num_segments()) return;
                 auto input_begin = input_reader->begin(segid);
                 auto input_end = input_reader->end(segid);
                 auto output_iter = output.get_output_iterator(segid);
                 std::transform(input_begin, input_end, output_iter, transformfn);
               });
}


/**************************************************************************/
/*                                                                        */
/*                       Implementation of copy_if                        */
/*                                                                        */
/**************************************************************************/

/**
 * Filters input to output calling the filterfn on each input and emitting
 * the input to output only if the filter function evaluates to true.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in input:
 *    if (filterfn(x)) write x to output
 * \endcode
 *
 * The input object must be a descendent of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * The output object should be of the same number of segments as the input
 * object. If they are of different number of segments, this function will
 * attempt to change the number of segments of the output object. Changing the
 * number of segments is generally a successful operation unless writes have
 * already occured on the output. If the number of segments do not match,
 * and if the number of output segments cannot be set, this function
 * will throw a string exception and fail.
 *
 * \param input The input to read from. Must be a descendent of siterable
 * \param output The output writer to write to. Must be a descendent of swriter_base
 * \param filterfn The filter operation to perform on the input. If the filterfn
 *                 evaluates to true, the input is copied to the output.
 * \param constraint_segments The set of segments to operate on. If empty
 *                            (default) will operate on all segments. Only valid
 *                            segment numbers will be operated on.
 */
template <typename S, typename T, typename FilterFn,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void copy_if(S&& input, T&& output,
             FilterFn filterfn,
             std::set<size_t> constraint_segments = std::set<size_t>(),
             int random_seed=std::time(NULL)) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  ASSERT_TRUE(output.is_opened_for_write());
  auto input_reader = input.get_reader(output.num_segments());
  // construct a vector containing a list of segments to operate on
  std::vector<size_t> segments;
  // if constraint segments is empty, we operate on all segments
  if (constraint_segments.empty()) {
    segments.resize(input_reader->num_segments());
    for (size_t i = 0; i < segments.size(); ++i) {
      segments[i] = i;
    }
  } else {
    std::copy(constraint_segments.begin(),
              constraint_segments.end(),
              std::inserter(segments, segments.end()));
  }

  // perform a parallel for through the list of segments
  parallel_for(0, segments.size(),
               [&](size_t idx) {
                 if (random_seed != -1){
                   random::get_source().seed(random_seed + idx);
                 }
                 size_t segid = segments[idx];
                 if (segid >= input_reader->num_segments()) return;
                 auto input_begin = input_reader->begin(segid);
                 auto input_end = input_reader->end(segid);
                 auto output_iter = output.get_output_iterator(segid);
                 std::copy_if(input_begin, input_end, output_iter, filterfn);
               });
}

/**
 * Filters input to output calling the filterfn on each input and emitting
 * the transformed input to output only if the filter function evaluates to true.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in input:
 *    if (filterfn(x)) write transform(x) to output
 * \endcode
 *
 * The input object must be a descendent of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * The output object should be of the same number of segments as the input
 * object. If they are of different number of segments, this function will
 * attempt to change the number of segments of the output object. Changing the
 * number of segments is generally a successful operation unless writes have
 * already occured on the output. If the number of segments do not match,
 * and if the number of output segments cannot be set, this function
 * will throw a string exception and fail.
 *
 * The output object should be ready for write and the schema must match
 * the output schema of the transform function.
 *
 * \param input The input to read from. Must be a descendent of siterable
 * \param output The output writer to write to. Must be a descendent of swriter_base
 * \param filterfn The filter operation to perform on the input. If the filterfn
 *                 evaluates to true, the input is copied to the output.
 * \param transformfn The transform operation to perform on the input.
 * \param constraint_segments The set of segments to operate on. If empty
 *                            (default) will operate on all segments. Only valid
 *                            segment numbers will be operated on.
 */
template <typename S, typename T, typename FilterFn, typename TransformFn,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void copy_transform_if(S&& input, T&& output,
             FilterFn filterfn,
             TransformFn transformfn,
             std::set<size_t> constraint_segments = std::set<size_t>(),
             int random_seed=std::time(NULL)) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  ASSERT_TRUE(output.is_opened_for_write());
  auto input_reader = input.get_reader(output.num_segments());
  // construct a vector containing a list of segments to operate on
  std::vector<size_t> segments;
  // if constraint segments is empty, we operate on all segments
  if (constraint_segments.empty()) {
    segments.resize(input_reader->num_segments());
    for (size_t i = 0; i < segments.size(); ++i) {
      segments[i] = i;
    }
  } else {
    std::copy(constraint_segments.begin(),
              constraint_segments.end(),
              std::inserter(segments, segments.end()));
  }

  // perform a parallel for through the list of segments
  parallel_for(0, segments.size(),
               [&](size_t idx) {
                 if (random_seed != -1){
                   random::get_source().seed(random_seed + idx);
                 }
                 size_t segid = segments[idx];
                 if (segid >= input_reader->num_segments()) return;
                 auto input_begin = input_reader->begin(segid);
                 auto input_end = input_reader->end(segid);
                 auto output_iter = output.get_output_iterator(segid);

                 while (input_begin != input_end) {
                   if (filterfn(*input_begin)) {
                     *output_iter = transformfn(*input_begin);
                     ++output_iter;
                   }
                   ++input_begin;
                 }
               });
}

/**************************************************************************/
/*                                                                        */
/*                       Implementation of split                          */
/*                                                                        */
/**************************************************************************/
/**
 * Split input to output1 and output2 calling the filterfn on each input and emitting
 * the input to output1 if the filter function evaluates to true, output2 otherwise.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in input:
 *    if (filterfn(x)) write x to output1
 *    else write x to output2
 * \endcode
 *
 * The input object must be a descendent of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * The output object should be of the same number of segments as the input
 * object. If they are of different number of segments, this function will
 * attempt to change the number of segments of the output object. Changing the
 * number of segments is generally a successful operation unless writes have
 * already occured on the output. If the number of segments do not match,
 * and if the number of output segments cannot be set, this function
 * will throw a string exception and fail.
 *
 * \param input The input to read from. Must be a descendent of siterable
 * \param output1 The output writer to write to. Must be a descendent of swriter_base
 * \param output2 The output writer to write to. Must be a descendent of swriter_base
 * \param filterfn The filter operation to perform on the input. If the filterfn
 *                 evaluates to true, the input is copied to the output1, else output2.
 */
template <typename S, typename T, typename FilterFn,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void split(S&& input, T&& output1, T&& output2,
           FilterFn filterfn,
           int random_seed=std::time(NULL)) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  ASSERT_TRUE(output1.is_opened_for_write());
  ASSERT_TRUE(output2.is_opened_for_write());
  if (output1.set_num_segments(output2.num_segments()) == false) {
    log_and_throw("Expects outputs to have the same number of segments");
  }

  auto input_reader = input.get_reader(output1.num_segments());
  // perform a parallel for through the list of segments
  parallel_for(0, input_reader->num_segments(),
               [&](size_t idx) {
                 if (random_seed != -1) {
                    random::get_source().seed(random_seed + idx);
                 }
                 auto input_begin = input_reader->begin(idx);
                 auto input_end = input_reader->end(idx);
                 auto output_iter1 = output1.get_output_iterator(idx);
                 auto output_iter2 = output2.get_output_iterator(idx);
                 auto iter = input_begin;
                 while(iter != input_end) {
                   auto& val = *iter;
                   if (filterfn(val)) {
                     *output_iter1 = val;
                     ++output_iter1;
                   } else {
                     *output_iter2 = val;
                     ++output_iter2;
                   }
                   ++iter;
                 }
               });
}


/**************************************************************************/
/*                                                                        */
/*     Implementation of copy (from regular iterators to the swriter)     */
/*                                                                        */
/**************************************************************************/

namespace sframe_impl {

template <typename Iterator, typename SWriter>
void do_copy(Iterator begin, Iterator end, SWriter&& writer,
             std::input_iterator_tag) {
  using std::distance;
  size_t length = distance(begin, end);
  size_t items_written = 0;
  // for each output array
  for (size_t i = 0; i < writer.num_segments(); ++i) {
    size_t remaining_length = length - items_written;
    // items to output in this segment is
    // remaining length / number of segments remaining
    size_t items_to_output = remaining_length / (writer.num_segments() - i);
    auto outputiter = writer.get_output_iterator(i);
    for (size_t j = 0;j < items_to_output; ++j) {
      *outputiter = *begin;
      ++outputiter;
      ++begin;
    }
    items_written += items_to_output;
  }
}



template <typename Iterator, typename SWriter>
void do_copy(Iterator begin, Iterator end, SWriter&& writer,
             std::random_access_iterator_tag tag) {
  size_t num_segments = writer.num_segments();
  // get all the output iterators
  size_t length = std::distance(begin, end);
  // size of range each segment gets
  double split_size = (double)length / num_segments;

  parallel_for(0, num_segments,
               [&](size_t segment) {
                 // beginning of this worker's range
                 Iterator segment_begin = begin + split_size * segment;
                 // end of this worker's range
                 Iterator segment_end = begin + split_size * (segment + 1);
                 if (segment == num_segments - 1) segment_end = end;
                 auto outputiter = writer.get_output_iterator(segment);
                 while(segment_begin != segment_end) {
                   *outputiter = *segment_begin;
                   ++outputiter;
                   ++segment_begin;
                 }
               });
}

} // namespace sframe_impl


/**
 * Writes to an SWriter from a standard input iterator sequence.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in range(begin, end) (regular iterator):
 *     write x to output (swriter)
 * \endcode
 *
 * The input must be a pair of iterators, and the output
 * object must be a descendent of \ref swriter_base.
 *
 * \note The precise arrangement of the data in the segment will depend
 * on the iterator type. If the iterator range is at minimal a
 * forward_iterator, The resultant data is blocked across the segments of the
 * writer. i.e. if the writer has 4 segments, the first 1/4 of the data will go
 * to segment 1, the next 1/4 will go to segment 2, etc. Otherwise, if the
 * iterator range is an input iterator, it will be striped across the segments.
 *
 * \param begin The start of the forward iterator sequence to write to the writer.
 * \param end The end of the forward iterator sequence to write to the writer.
 * \param output The output writer to write to. Must be a descendent of swriter_base
 */
template <typename Iterator, typename SWriter,
typename = typename std::enable_if<sframe_impl::is_sarray_like<SWriter>::value>::type>
void copy(Iterator begin, Iterator end, SWriter&& writer) {
  ASSERT_TRUE(writer.is_opened_for_write());
  sframe_impl::do_copy(begin, end, std::forward<SWriter>(writer),
                 typename std::iterator_traits<Iterator>::iterator_category());
}




/**
 * Copies the contents of an SArray to regular output iterator.
 *
 * This class accomplishes the abstract equivalent of:
 * \code
 * for each x in sarray
 *     write x to output
 * \endcode
 *
 * The input object must be a descendent of \ref swriter_base.
 *
 * \param array The SArray to read from
 * \param output The output iterator to write to.
 */
template <typename S, typename Iterator,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type>
void copy(S&& array, Iterator output, size_t limit=(size_t)(-1)) {
  log_func_entry();
  ASSERT_TRUE(array.is_opened_for_read());
  // for each segment
  // copy into the output
  auto reader = array.get_reader();
  size_t ctr = 0;
  for (size_t i = 0;i < reader->num_segments(); ++i) {
    auto begin = reader->begin(i);
    auto end = reader->end(i);
    while(ctr < limit && begin != end) {
      (*output) = std::move(*begin);
      ++output;
      ++begin;
      ++ctr;
    }
    if (ctr >= limit) break;
  }
}

/**
 * Performs a reduction on each segment of an Sarray returning the result of the
 * reduction on each segment.
 *
 * This class accomplishes the abstract equivalent of:
 * \code
 * for each segment in sarray
 *     res = init
 *     for each x in segment:
 *        if (f(x, res) == false) break;
 *     returnval[segment] = res
 * return returnval
 * \endcode
 *
 * The input object must be a descendent of \ref siterable
 *
 * \param array The SArray to read from
 * \param function The reduction function to use. This must be of the form
 *                 bool f(const array_value_type&, reduction_type&).
 * \param init The initial value to use in the reduction
 *
 * \tparam ResultType The result type of each reduction.
 * \tparam S The type of the input array. Must be a descendent of siterable
 * \tparam FunctionType The type of the function
 *
 * \note This function assumes that writes to std::vector<T> can be made in
 * parallel. i.e. ResultType of bool should not be used.
 */
template <typename ResultType, typename S, typename FunctionType,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type>
std::vector<ResultType> reduce(S&& input, FunctionType f,
                                ResultType init = ResultType()) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  std::vector<ResultType> ret;
  size_t dop = thread::cpu_count();
  ret.resize(dop, init);
  auto input_reader = input.get_reader(dop);
  // perform a parallel for through the list of segments
  parallel_for(0, dop,
               [&](size_t idx) {
                 auto input_begin = input_reader->begin(idx);
                 auto input_end = input_reader->end(idx);
                 ResultType reduce_result = init;
                 while(input_begin != input_end) {
                   if (f(*input_begin, reduce_result) == false) break;
                   ++input_begin;
                 }
                 ret[idx] = reduce_result;
               });
  return ret;
}





/**
 * Writes input to output calling the transformfn on each input pair emitting
 * the result to output.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for each x in input1:
 *    read next y in input2
 *    write transformfn(x, y) to output
 * \endcode
 *
 * The input objects must be descendents of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * \param input1 The first input to read from. Must be a descendent of siterable
 * \param input2 The second input to read from. Must be a descendent of siterable
 * \param output The output writer to write to. Must be a descendent of swriter_base
 * \param transformfn The transform operation to perform on the inputs
 *                    to generate the output
 */
template <typename S1, typename S2, typename T, typename TransformFn,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S1>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S2>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void binary_transform(S1&& input1, S2&& input2, T&& output,
               TransformFn transformfn) {
  log_func_entry();
  ASSERT_TRUE(input1.is_opened_for_read());
  ASSERT_TRUE(input2.is_opened_for_read());
  ASSERT_TRUE(output.is_opened_for_write());

  auto input1_reader = input1.get_reader(output.num_segments());
  auto input2_reader = input2.get_reader(output.num_segments());
  // perform a parallel for through the list of segments
  parallel_for(0, output.num_segments(),
               [&](size_t idx) {
                 auto input1_begin = input1_reader->begin(idx);
                 auto input1_end = input1_reader->end(idx);
                 auto input2_begin = input2_reader->begin(idx);
                 auto input2_end = input2_reader->end(idx);
                 auto output_iter = output.get_output_iterator(idx);
                 while(input1_begin != input1_end) {
                   (*output_iter) = transformfn(*input1_begin, *input2_begin);
                   ++input1_begin;
                   ++input2_begin;
                   ++output_iter;
                 }
               });
}



/**************************************************************************/
/*                                                                        */
/*                    Implementation of copy_range                        */
/*                                                                        */
/**************************************************************************/

/**
 * Copies a range of elements to the output.
 *
 * This class accomplishes the abstract equivalent of
 * \code
 * for i = begin; i < end; i += step
 *    write row i in input to to output
 * \endcode
 *
 * The input object must be a descendent of \ref siterable, and the output
 * object must be a descendent of \ref swriter_base. \ref sarray and
 * \ref swriter are two common instances of either.
 *
 * \param input The input to read from. Must be a descendent of siterable
 * \param output The output writer to write to. Must be a descendent of swriter_base
 * \param start The start row to begin reading
 * \param step The row step
 * \param end One past the last row to read
 */
template <typename S, typename T,
typename = typename std::enable_if<sframe_impl::is_sarray_like<S>::value>::type,
typename = typename std::enable_if<sframe_impl::is_sarray_like<T>::value>::type>
void copy_range(S&& input, T&& output,
                size_t start,
                size_t step,
                size_t end) {
  log_func_entry();
  ASSERT_TRUE(input.is_opened_for_read());
  ASSERT_TRUE(output.is_opened_for_write());

  auto reader = input.get_reader();
  end = std::min(end, reader->size());

  if (end < start) {
    log_and_throw("End must be at least start");
  }

  // this is the range of input elements
  size_t element_range = end - start;
  // there are this number of out elements
  size_t num_out_elems = 1 + (element_range - 1) / step;

  parallel_for(0, output.num_segments(),
               [&](size_t idx) {
                 auto writer = output.get_output_iterator(idx);
                 size_t start_idx = idx * num_out_elems / output.num_segments();
                 size_t end_idx = (idx + 1) * num_out_elems / output.num_segments();

                 std::vector<typename std::decay<S>::type::value_type> buffer;
                 if (step == 1) {
                   // special case for step == 1
                   // read a block and write a block
                   for (size_t i = start_idx;
                        i < end_idx;
                        i += DEFAULT_SARRAY_READER_BUFFER_SIZE) {
                     size_t block_read_range_start = start + i;
                     size_t block_read_range_end =
                         block_read_range_start + DEFAULT_SARRAY_READER_BUFFER_SIZE;
                     block_read_range_end = std::min(block_read_range_end, start + end_idx);
                     reader->read_rows(block_read_range_start,
                                       block_read_range_end,
                                       buffer);
                     for (auto& row: buffer) {
                       (*writer) = row;
                       ++writer;
                     }
                   }
                 } else {
                   // for step size > 1, read one element at a time
                   for (size_t i = start_idx; i < end_idx; ++i) {
                     reader->read_rows(start + i * step,
                                       start + i * step + 1,
                                       buffer);
                     if (buffer.size() == 0) break;
                     (*writer) = buffer[0];
                     ++writer;
                   }
                 }
               });
}

/// \}
} // namespace turi
#endif
