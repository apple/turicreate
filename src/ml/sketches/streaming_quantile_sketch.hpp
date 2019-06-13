/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCH_STREAMING_QUANTILE_SKETCH_HPP
#define TURI_SKETCH_STREAMING_QUANTILE_SKETCH_HPP
#include <vector>
#include <ml/sketches/quantile_sketch.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
namespace turi {
namespace sketches {

/**
 * \ingroup sketching
 * This class implements the *streaming* quantile sketching datastructure as
 * described in Qi Zhang and Wei Wang. A Fast Algorithm for Approximate
 * Quantiles in High Speed Data Streams. SSDBM 2007.
 *
 * This sketch is basically a streaming version of the \ref quantile_sketch
 *
 * It constructs a low memory sketch from a stream of unknown length.
 * To summarize the basic idea, it draws samples from the stream and maintains
 * for each sample a lower bound and an upper bound on its rank in the stream.
 *
 * Usage:
 * \code
 * // construct a sketch at a particular accuracy
 * quantile_sketch<double> sketch(0.01);
 * ...
 * insert any number elements into the sketch
 * sketch.add(value)
 * ...
 * sketch.finalize() // finalize must be called before queries can be made
 * \endcode
 *
 * To query, two family of functions are provided.
 *
 * The regular query functions take linear time in the size of the sketch, and
 * have accuracy guarantees
 * \code
 * sketch.query(rank) // rank is between [0, N-1]
 * sketch.query_quantile(quantile) // quantile is between [0, 1.0]
 * \endcode
 *
 * The fast query functions take logarithmic time in the size of the sketch,
 * and have no accuracy guarantees.
 * \code
 * sketch.fast_query(rank) // rank is between [0, N-1]
 * sketch.fast_query_quantile(quantile) // quantile is between [0, 1.0]
 * \endcode
 *
 * The sketch structure also support parallel sketching:
 *
 * Example:
 * If I have 16 data streams, I can construct 16 streaming_quantile_sketches:
 *
 * Each sketch can then be ran on each data stream independently.
 * Then all can be combined into one. Note that the process for doing this on
 * the streaming_quantile_sketch is somewhat more complicated. It involves a
 * partial finalization of each substream (\ref sub_stream_finalize() ), before
 * combining. And after combining, a different finalization call must be made
 * (\ref combine_finalize())
 *
 * \code
 * // add stuff to each sub stream
 * for each sub-stream sketch:
 *   sub-stream-sketch.add(...)
 *
 * // pre-finalize each sub stream
 * for each sub-stream sketch:
 *   sub-stream-sketch.substream_finalize();
 *
 * // combine into one sketch
 * quantile_sketch sketch(0.01)
 * for each sub-stream sketch:
 *   sketch.combine(sub-stream-sketch)
 *
 * // finalize the main sketch
 * sketch.combine_finalize()
 * \endcode
 *
 * \tparam Type to contain in the sketch. type must be less-than-comparable,
 * constructible, copy constructible, assignable.
 *
 * Technical Details
 * -----------------
 * We create a sequence of fixed length sketches, with each subsequent sketch
 * sized to sketch twice the amount of data as the previous one.
 * Each sketch is built to have a final finalized error of \epsilon / 3.
 *
 * If no sub-streams are used, the finalize() phase will merge all sketches
 * and recompress to introduce an additional error of (2 * \epsilon / 3)
 *
 * If sub-streams are used, the sub-stream finalize will merge all the
 * sub-stream sketches to introduce an additional error of (\epsilon / 3).
 * Then each of the substream sketches are merged into the final sketch and
 * compressed again to introduce an additional error of (\epsilon / 3)
 *
 */
template <typename T, typename Comparator>
class streaming_quantile_sketch {
 public:

  /**
   * Constructs a quantile_sketch with a desired target number of elements,
   * and a desired accuracy. See init() for details.
   */
  explicit streaming_quantile_sketch(double epsilon = 0.005, const Comparator& comparator = Comparator())
   {
    init(epsilon, comparator);
  }

  /**
   * Reinitializes the quantile sketch, resetting it.
   * epsilon is the desired accuracy of the quantiles.
   *
   * \param epsilon The desired accuracy
   */
  void init(double epsilon, const Comparator& comparator) {
    m_comparator = comparator;
    m_epsilon = epsilon;
    m_elements_inserted = 0;
    m_levels.clear();
    m_levels.resize(1);
    m_initial_sketch_size = std::max<size_t>(1, 1.0 / m_epsilon);
    // we construct each intermediate sketch with epsilon / 3.0 error
    m_levels[0].init(m_initial_sketch_size, m_epsilon / 3.0, comparator);
    // init the final sketch to clear it. Really doesn't matter what
    // size we init it with at this point. We just need it to be initted
    m_final.init(m_initial_sketch_size, m_epsilon, comparator);
  }

  /**
   * Returns the number of elements stored in the sketch
   */
  size_t sketch_size() {
    size_t sum = 0;
    for (size_t i = 0; i < m_levels.size(); ++i) {
      sum += m_levels[i].sketch_size();
    }
    sum += m_final.sketch_size();
    return sum;
  }


  /**
   * Returns the approximate current memory usage of the sketch in bytes.
   */
  size_t memory_usage() {
    size_t sum = 0;
    for (size_t i = 0; i < m_levels.size(); ++i) {
      sum += m_levels[i].memory_usage();
    }
    sum += m_final.memory_usage();
    return sum;
  }

  /**
   * Inserts a value into the sketch. Not safe to use in parallel.
   */
  void add(T t) {
    // maximum size of level i is m_initial_sketch_size * 2^i
    // are we full?
    size_t curlevel = m_levels.size() - 1;
    if (m_levels[curlevel].size() >= (m_initial_sketch_size << curlevel)) {
      // we are full. make a new level
      m_levels.push_back(quantile_sketch<T, Comparator>());
      ++curlevel;
      m_levels[curlevel].init(m_initial_sketch_size << curlevel, m_epsilon / 3.0, m_comparator);
    }
    m_levels[curlevel].add(t);
    ++m_elements_inserted;
  }

  /**
   * Finalizes the sketch datastructure.
   * Must be called before queries are made. Once finalize() is called,
   * elements may no longer be inserted.
   */
  void finalize() {
    m_final.init(m_elements_inserted, 2 * m_epsilon / 3.0, m_comparator);
    m_final.finalize();
    for (size_t i = 0;i < m_levels.size(); ++i) {
      m_levels[i].finalize();
    }
    // we need intrusive access to the sketch. Basically, we will merge
    // all the sketch data into one.

    for (size_t i = 0;i < m_levels.size(); ++i) {
      m_final.m_query = m_final.merge(m_final.m_query, m_levels[i].m_query);
    }
    // since each intermediate sketch was constructed with epsilon / 2.0 error,
    // we can compress the final sketch with epsilon / 2.0 error to
    // make up a sketch with epsilon error.
    m_final.compress(m_final.m_query, 2 * m_epsilon / 3.0);

    m_final.m_elements_inserted = m_elements_inserted;

    m_levels.clear();
  }

  /**
   * Returns the number of elements inserted into the sketch.
   */
  size_t size() const {
    return m_elements_inserted;
  }

  /**
   * Queries for the value at rank k of all the elements inserted.
   * Assuming N elements are inserted. Rank 0 will be the minimum value,
   * rank N-1, the maximum value.
   *
   * Note that this rank is in relation to the total number of elements
   * inserted. For instance,
   * \code
   * quantile_sketch<double> sketch(1000);
   * ...
   * insert 10,000 elements into the sketch
   * \endcode
   * sketch.query(9999) will be the maximum.
   *
   * \note finalize() must be called prior to using this function.
   *
   * \note The complexity of this function can be logarithmic in the sketch size
   * in the best case (appears to be all the time, empirically), or linear in
   * the worst case. Essentially, it first tests the result of the fast_query
   * to see if that fits the bound requirements. If it does not, it takes a full
   * linear search. The fast_query seems to work pretty much all the time on
   * randomized inputs of varying distributions.
   */
  T query(size_t rank) {
    return m_final.query(rank);
  }


  /**
   * Returns the value at a quantile. For instance:
   * \code
   * sketch.query_quantile(0.01) // returns the 1% quantile
   * sketch.query_quantile(0.50) // returns the median
   * sketch.query_quantile(1.00) // returns the max
   * \endcode
   *
   * quantile values below 0 will be interpreted as 0 (hence the minimum).
   * quantile values above 1 will be interpreted as 1 (hence the maximum).
   *
   * \note finalize() must be called prior to using this function.
   *
   * \note The complexity of this function is linear in the sketch size.
   */
  T query_quantile(double quantile) {
    return m_final.query_quantile(quantile);
  }

  /**
   * Quickly queries for the value at rank k of all the elements inserted.
   * Assuming N elements are inserted. Rank 0 will be the minimum value,
   * rank N-1, the maximum value.
   *
   * Note that this rank is in relation to the total number of elements
   * inserted. For instance,
   * \code
   * quantile_sketch<double> sketch(1000);
   * ...
   * insert 10,000 elements into the sketch
   * \endcode
   * sketch.fast_query(9999) will be the maximum.
   *
   * Unlike query(), this function is never guaranteed to provide an
   * epsilon error bound. Instead, this function assumes that each element in
   * the sketch has a point estimate of its rank (the center of its rmin/rmax
   * range), and looks for the element closest to the desired rank.
   *
   * \note finalize() must be called prior to using this function.
   *
   * \note The complexity of this function can be logarithmic in the sketch size
   * in the best case (appears to be all the time, empirically), or linear in
   * the worst case. Essentially, it first tests the result of the fast_query
   * to see if that fits the bound requirements. If it does not, it takes a full
   * linear search. The fast_query seems to work pretty much all the time on
   * randomized inputs of varying distributions.
   */
  T fast_query(size_t rank) {
    return m_final.fast_query(rank);
  }


  /**
   * Quickly returns the value at a quantile. For instance:
   * \code
   * sketch.fast_query_quantile(0.01) // returns the 1% quantile
   * sketch.fast_query_quantile(0.50) // returns the median
   * sketch.fast_query_quantile(1.00) // returns the max
   * \endcode
   *
   * quantile values below 0 will be interpreted as 0 (hence the minimum).
   * quantile values above 1 will be interpreted as 1 (hence the maximum).
   *
   * Unlike query_quantile(), this function is never guaranteed to provide an
   * epsilon error bound. Instead, this function assumes that each element in
   * the sketch has a point estimate of its rank (the center of its rmin/rmax
   * range), and looks for the element closest to the desired rank.
   *
   * \note finalize() must be called prior to using this function.
   *
   * \note The complexity of this function is logarithmic in the sketch size.
   */
  T fast_query_quantile(double quantile) {
    return m_final.fast_query_quantile(quantile);
  }


  /**
   * Finalizes the sketch datastructure, in preparation for combining
   * with other streaming quantile sketches.
   * Once substream_finalize() is called, elements may no longer be inserted.
   *
   * \see combine_finalize() combine()
   */
  void substream_finalize() {
    m_final.init(m_elements_inserted, m_epsilon / 3.0, m_comparator);
    m_final.finalize();
    for (size_t i = 0;i < m_levels.size(); ++i) {
      m_levels[i].finalize();
    }
    // we need intrusive access to the sketch. Basically, we will merge
    // all the sketch data into one.

    for (size_t i = 0;i < m_levels.size(); ++i) {
      m_final.m_query = m_final.merge(m_final.m_query, m_levels[i].m_query);
    }
    m_final.compress(m_final.m_query, m_epsilon / 3.0);

    m_final.m_elements_inserted = m_elements_inserted;

    m_levels.clear();
  }

  /**
   * Merges two substream_finalized quantile_sketch into one quantile sketch.
   * Both this and the other quantile sketch must be substream_finalized().
   * This allows two quantile sketches to be generated on two disjoint streams
   * of data, and then combined at the end.
   *
   * Example:
   * If I have 16 data streams, I can construct 16 quantile_sketches,
   *
   * Each sketch can then be ran on each data stream independently.
   * Then all can be combined into one:
   *
   * \code
   * // add stuff to each sub stream
   * for each sub-stream sketch:
   *   sub-stream-sketch.add(...)
   *
   * // pre-finalize each sub stream
   * for each sub-stream sketch:
   *   sub-stream-sketch.substream_finalize();
   *
   * // combine into one sketch
   * quantile_sketch sketch(0.01)
   * for each sub-stream sketch:
   *   sketch.combine(sub-stream-sketch)
   *
   * // finalize the main sketch
   * sketch.combine_finalize()
   * \endcode
   *
   * \note The predictions produced by the combined sketch is not generally
   * going to be the same as the sequentially produced sketch
   *
   * \see combine_finalize() substream_finalize()
   */
  void combine(streaming_quantile_sketch other) {
    if (m_elements_inserted == 0) {
      m_final = other.m_final;
    } else {
      m_final.m_query = m_final.merge(m_final.m_query, other.m_final.m_query);
    }
    m_elements_inserted += other.m_elements_inserted;
  }

  /**
   * Stops combining, and finishes the final sketch making it available for
   * querying.  \see combine() substream_finalize()
   */
  void combine_finalize() {
    m_final.compress(m_final.m_query, m_epsilon / 3.0);
    m_final.m_elements_inserted = m_elements_inserted;
    m_final.m_epsilon = m_epsilon;
  }

  void save(oarchive& oarc) const {
    oarc << m_epsilon << m_elements_inserted
         << m_initial_sketch_size << m_levels << m_final;
  }
  void load(iarchive& iarc) {
    iarc >> m_epsilon >> m_elements_inserted
         >> m_initial_sketch_size >> m_levels >> m_final;
  }

 private:
  double m_epsilon = 0.01; // desired accuracy
  size_t m_elements_inserted = 0;

  // basically, we create a series of exponentially larger sketches.
  // starting at something sane... like 16.
  size_t m_initial_sketch_size = 16;
  std::vector<quantile_sketch<T, Comparator> > m_levels;
  quantile_sketch<T, Comparator> m_final;

  Comparator m_comparator;
};

} // namespace sketch
} // namespace turi
#endif // TURI_SKETCH_STREAMING_QUANTILE_SKETCH_HPP
