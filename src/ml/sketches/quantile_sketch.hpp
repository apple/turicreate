/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCH_QUANTILE_SKETCH_HPP
#define TURI_SKETCH_QUANTILE_SKETCH_HPP
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <tuple>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/util/basic_types.hpp>

namespace turi {
namespace sketches {

//forward decl of streaming_quantile_sketch
template <typename T, typename Comparator = std::less<T>>
class streaming_quantile_sketch;

/**
 * \ingroup sketching
 * This class implements a quantile sketching datastructure as described in
 * Qi Zhang and Wei Wang. A Fast Algorithm for Approximate Quantiles in
 * High Speed Data Streams. SSDBM 2007.
 *
 * It constructs a low memory sketch from a stream of known length.
 * To summarize the basic idea, it draws samples from the stream and maintains
 * for each sample a lower bound and an upper bound on its rank in the stream.
 *
 * The known length condition is not strictly true. While the sketch is
 * constructed for particular stream length N, this datastructure does not
 * limit the number of elements can be inserted, but then in which case the
 * accuracy guarantee no longer applies.
 *
 * Usage is simple:
 * \code
 * // constrcut sketch at a particular size
 * quantile_sketch<double> sketch(100000, 0.01);
 * ...
 * insert any number elements into the sketch using
 * quality guarantees only apply when <100000 elements are inserted.
 * sketch.add(value)
 * ...
 * sketch.finalize() // finalize must be called before queries can be made
 * \endcode
 *
 * To query, two family of functions are provided.
 *
 * The regular query functions take linear time in the size of the sketch, and
 * have accuracy guarantees when <=desired_n elements are inserted (100000
 * elements in the above example)
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
 * If I have 16 data streams, each of size N/16
 * I can construct 16 quantile_sketches, each initialized with
 * desired_n = N (note. N here! not N/16)
 *
 * Each sketch can then be ran on each data stream independently.
 * Then all can be combined into one:
 *
 * \code
 * quantile_sketch sketch(N)
 * for each sub-stream sketch:
 *   sketch.combine(sub-stream-sketch)
 * sketch.finalize()
 * \endcode
 *
 * \tparam Type to contain in the sketch. type must be less-than-comparable,
 * constructible, copy constructible, assignable.
 *
 * Technical Details
 * -----------------
 * The basic mechanic of the sketch lies in making a hierarchy of sketches, each
 * having size m_b. The first sketch has error 0. The second sketch has error
 * 1/b, the 3rd sketch has error 2/b and so on. These sketches are treated as
 * like a binary sequence:
 *   - When a buffer of size b has been accumulated, it gets sorted and
 *     inserted into sketch 1
 *   - If sketch 1 exists, the two are merged (no increase in error) and
 *     compressed (increase error by 1/b) and the result tries to get placed as
 *     sketch 2.
 *   - If sketch 2 exists, this repeats.
 *
 * The procedure thus seem like binary addition.
 *
 * By picking the size b appropriately for a given value of N and \epsilon, At
 * the end, all the sketches can be merged to get a sketch of error \epsilon.
 *
 * Here is where we deviate from the paper slightly. The "compress" procedure
 * has the advantage of decreasing the total amount of memory required,
 * By finishing/finalizing with a simple merge, means that the resultant array
 * could be much bigger than desired. The way the binary addition works, also
 * means the resultant sketch size is somewhat unpredictable.
 * Thus here lies the modification.
 *
 * We compute the sketch as usual, but sizing "b" to end up with a final merged
 * error of \epsilon / 2. We then perform one addition compress(\epsilon/2)
 * phase to compress the final sketch into requiring only O(1/epsilon) memory.
 */
template <typename T, typename Comparator = std::less<T>>
class quantile_sketch {
 public:

   /**
    * Default constructor. Does nothing. init() MUST be called before
    * any operations are performed.
    */
  quantile_sketch() {}

  /**
   * Constructs a quantile_sketch with a desired target number of elements,
   * and a desired accuracy. See init() for details.
   */
  explicit quantile_sketch(size_t desired_n, double epsilon = 0.005, const Comparator& comparator = Comparator()) {
    init(desired_n, epsilon, comparator);
  }

  /**
   * Initializes the quantile sketch, resetting it.
   * epsilon is the desired accuracy of the quantiles.
   * An \epsilon-approximate quantile is an element in the data stream
   * whose approximate rank r' is within [r-\epsilon n and r + \epsilon n]
   * i.e. a 0.0001 approximate median in a 1,000,000 element array may have
   * true rank [499,000, and 501,000].
   *
   * The runtime memory requirements of this sketch is
   * O((1/\epsilon) (log \epsilon N)^2)
   * The final memory requirements after finalization is O(1/\epsilon).
   *
   * To give a sense of the runtime memory requirements,
   * at \epsilon = 0.01
   * \verbatim
   * N = 100,000: sketch has size = 45528 bytes
   * N = 1,000,000: sketch has size = 96144 bytes
   * N = 10,000,000: sketch has size = 123504 bytes
   * N = 100,000,000: sketch has size = 316536 bytes
   * \endverbatim
   *
   * (Increasing N by 10 roughly increases the sketch size by 2. Note that
   * the growth function is a little wierd. this is due to the structure of the
   * algorithm which maintains multi-level sketches, then during the final
   * phase, just merging all those sketches together. This results in somewhat
   * oddly behaved memory utilization as a function of N.)
   *
   * At N = 1,000,000
   * \verbatim
   * epsilon = 0.1: sketch has size = 12480 bytes
   * epsilon = 0.01: sketch has size = 96144 bytes
   * epsilon = 0.001: sketch has size = 442776 bytes
   * epsilon = 0.0001: sketch has size = 3271440 bytes
   * \endverbatim
   *
   * (decreasing epsilon by 10 roughly increases the sketch size by 5)
   *
   *
   * \param desired_n An UPPER BOUND on the number of elements to be inserted,
   * to guarantee epsilon accuracy. The epsilon accuracy guarantee is only true
   * if you do not insert more than n elements.
   * \param epsilon The desired accuracy
   */
  void init(size_t desired_n, double epsilon, const Comparator& comparator = Comparator()) {
    m_comparator = element_less_than(comparator);
    m_n = desired_n;
    size_t l = (epsilon * m_n);
    // if m_n or epsilon is too small, l could be < 2 then log2(l) implodes
    // wondrously.
    if (l == 0) l = 2;
    m_b = 2 * std::floor(std::log2(l) / epsilon);
    /*
     * Note that the m_b formula here is twice what was recommended in the
     * paper. This is an extension here. By sketching with half the error,
     * we can introduce an addition \epsilon/2 compression stage at the end
     * after finalization (which the original paper did not do) which allows
     * the final memory utilization to be substantially lower.
     */
    // too small n... Lets store everything
    if (m_b == 0) m_b = desired_n;
    m_epsilon = epsilon;
    m_elements_inserted = 0;

    m_levels.clear();
    m_levels.resize(1);
    m_query.clear();
  }

  /**
   * Returns the number of elements stored in the sketch
   */
  size_t sketch_size() {
    size_t sum = m_query.size();
    for (size_t i = 0; i < m_levels.size(); ++i) {
      sum += m_levels[i].size();
    }
    return sum;
  }


  /**
   * Returns the approximate current memory usage of the sketch in bytes.
   */
  size_t memory_usage() {
    return sketch_size() * sizeof(element);
  }

  /**
   * Inserts a value into the sketch. Not safe to use in parallel.
   */
  void add(T t) {
    // finalize not yet called
    DASSERT_EQ(m_query.size(), 0);
    // insert into level 0
    m_levels[0].emplace_back(t);
    ++m_elements_inserted;

    if (m_levels[0].size() == m_b) {
      // once level 0 is full, insert into the multilevel sketch
      sort_level_0();
      compress(m_levels[0], 1.0 / m_b);
      std::vector<element> sc = std::move(m_levels[0]);
      m_levels[0].clear();
      compact(sc);
    }
  }

  /**
   * Finalizes the sketch datastructure.
   * Must be called before queries are made. Once finalize() is called,
   * elements may no longer be inserted.
   */
  void finalize() {
    sort_level_0();
    m_query = recursive_merge_of_all_levels(0, m_levels.size());
    // we perform one more compression at the end to further reduce memory
    // utilization
    compress(m_query, m_epsilon / 2.0);
    // sort m_query by the center of the rank range [r_min, r_max]
    std::sort(m_query.begin(),
              m_query.end(),
              rank_center_comparator);
    m_levels.clear();
    m_n = m_elements_inserted;
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
   * \note This function is guaranteed to provide an epsilon accurate value if
   * only the desired number of elements (as set in the constructor) are
   * inserted.  In all other cases, this may not.
   *
   * \note The complexity of this function can be logarithmic in the sketch size
   * in the best case (appears to be all the time, empirically), or linear in
   * the worst case. Essentially, it first tests the result of the fast_query
   * to see if that fits the bound requirements. If it does not, it takes a full
   * linear search. The fast_query seems to work pretty much all the time on
   * randomized inputs of varying distributions.
   */
  T query(size_t rank) const {
    if (m_query.size() == 0) return T();
    // finalize called. I have a queryable value
    ++rank;
    // special handling for min and max
    if (rank <= 1) {
      return m_query[0].val;
    }
    if (rank >= m_elements_inserted) {
      return m_query[m_query.size() - 1].val;
    }
    int lower_index = (int)rank - m_elements_inserted * m_epsilon;
    int upper_index = (int)rank + m_elements_inserted * m_epsilon;
    lower_index = std::max<int>(lower_index, 0);
    // see if fast_query can give us a good answer now
    auto fast_query_iter = fast_query_iterator(rank - 1);
    static_assert(std::is_same<size_t, decltype(fast_query_iter->rmin)>::value,
                  "rmin expected to have type size_t");
    static_assert(std::is_same<size_t, decltype(fast_query_iter->rmax)>::value,
                  "rmax expected to have type size_t");
    if (truncate_check<int64_t>(fast_query_iter->rmin) >= lower_index &&
        truncate_check<int64_t>(fast_query_iter->rmax) <= upper_index) {
      return fast_query_iter->val;
    }

    size_t tightest = -1;
    size_t tightest_range = -1;
    // the algorithm provided in the paper tries to find the element such that
    // lower_index <= rmin    rmax <= upper_index
    // However, it is not entirely obvious from the algorithm, that such an
    // element always exists. Furthermore, in the event that I have inserted
    // less than, or greater than the required number of elements, it is not
    // obvious that this will succeed either.
    for (size_t i = 0;i < m_query.size(); ++i) {
      static_assert(std::is_same<size_t, decltype(m_query[i].rmin)>::value,
                    "rmin expected to have type size_t");
      static_assert(std::is_same<size_t, decltype(m_query[i].rmax)>::value,
                    "rmax expected to have type size_t");
      if (truncate_check<int64_t>(m_query[i].rmin) >= lower_index &&
          truncate_check<int64_t>(m_query[i].rmax) <= upper_index){
        size_t center = (m_query[i].rmax + m_query[i].rmin) / 2;
        if (std::fabs(center - rank) < tightest_range) {
          tightest = i;
          tightest_range = std::fabs(center - rank);
        }
      }
    }
    if (tightest == (size_t)(-1)){
      // in the event that we are unable to find an element, we fall back
      // to the fast query method.
      return fast_query_iter->val;
    } else {
      return m_query[tightest].val;
    }
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
   * \note This function is guaranteed to provide an epsilon accurate value if
   * only the desired number of elements (as set in the constructor) are
   * inserted.  In all other cases, this may not.
   *
   * \note The complexity of this function can be logarithmic in the sketch size
   * in the best case (appears to be all the time, empirically), or linear in
   * the worst case. Essentially, it first tests the result of the fast_query
   * to see if that fits the bound requirements. If it does not, it takes a full
   * linear search. The fast_query seems to work pretty much all the time on
   * randomized inputs of varying distributions.
   */
  T query_quantile(double quantile) const {
    if (quantile < 0) quantile = 0;
    if (quantile > 1) quantile = 1;
    return query(quantile * m_elements_inserted);
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
   * \note The complexity of this function is logarithmic in the sketch size.
   */
  T fast_query(size_t rank) const {
    return fast_query_iterator(rank)->val;
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
  T fast_query_quantile(double quantile) const {
    if (quantile < 0) quantile = 0;
    if (quantile > 1) quantile = 1;
    return fast_query(quantile * m_elements_inserted);
  }

  /**
   * Merges two unfinalized quantile_sketch into one quantile sketch.
   * Both this and the other quantile sketch must not be finalized().
   * This allows two quantile sketches to be generated on two disjoint streams
   * of data, and then combined at the end. For quality guarantees to apply,
   * BOTH sketches must be constructed with the final number of elements.
   *
   * Example:
   * If I have 16 data streams, each of size N/16
   * I can construct 16 quantile_sketches, each initialized with
   * desired_n = N (note. N here! not N/16)
   *
   * Each sketch can then be ran on each data stream independently.
   * Then all can be combined into one:
   *
   * \code
   * quantile_sketch sketch(N)
   * for each sub-stream sketch:
   *   sketch.combine(sub-stream-sketch)
   * sketch.finalize()
   * \endcode
   *
   * \note The predictions produced by the combined sketch is not generally
   * going to be the same as the sequentially produced sketch
   */
  void combine(const quantile_sketch& other) {
    ASSERT_EQ(m_query.size(), 0);
    ASSERT_EQ(other.m_query.size(), 0);
    if (m_levels.size() < other.m_levels.size()) {
      m_levels.resize(other.m_levels.size());
    }
    if (other.m_levels.size() > 0) {
      // merge all levels above level 0
      for (size_t i = 1; i < other.m_levels.size(); ++i) {
        if (other.m_levels[i].size() > 0) {
          // compact modifies sc
          std::vector<element> sc = other.m_levels[i];
          compact(sc, i);
        }
      }
      // insert level 0 the regular way
      for (size_t i = 0; i < other.m_levels[0].size(); ++i) {
        add(other.m_levels[0][i].val);
      }
      // update the number of elements inserted.
      // (note we have to substract off the level 0 elements inserted the
      // regular way)
      m_elements_inserted += other.m_elements_inserted - other.m_levels[0].size();
    }
  }


  void save(oarchive& oarc) const {
    oarc << m_n << m_b << m_elements_inserted << m_epsilon
         << m_levels << m_query;
  }
  void load(iarchive& iarc) {
    iarc >> m_n >> m_b >> m_elements_inserted >> m_epsilon
         >> m_levels >> m_query;
  }

 private:
  /**
   * The storage for each value. Contains the value, the lower bound on the
   * rank, and the upper bound of the rank.
   */
  struct element: public IS_POD_TYPE {
    element() {}
    explicit element(T val)
      :val(val), rmin(-1), rmax(-1) {}
    explicit element(T val, size_t rmin, size_t rmax)
      :val(val), rmin(rmin), rmax(rmax){}

    // returns the center of the rmin/rmax range
    float rcenter() const {
      return ((float)(rmin) + rmax) / 2;
    }

    T val; // The value of the element
    size_t rmin = -1; // The lower bound on the rank of the element
    size_t rmax = -1; // The upper bound on the rank of the element
  };

  struct element_less_than {
    element_less_than() {};
    element_less_than(const Comparator& comparator) {
      m_comparator = comparator;
    }

    bool operator()(const element& e1, const element& e2) {
      return m_comparator(e1.val, e2.val);
    }

    Comparator m_comparator;
  };


  size_t m_n = 0; // total number of elements expected
  size_t m_b = 0; // size of each bucket
  size_t m_elements_inserted = 0; // number of elements inserted so far
  double m_epsilon = 0.01; // desired accuracy

  /// The multi-level sketch. Used when inserting into the sketch structure.
  std::vector<std::vector<element> > m_levels;

  /** After the last element is inserted, the final sketch is
   *  sorted and constructed here.
   */
  std::vector<element> m_query;

  /* The comparator to sort elements */
  element_less_than m_comparator;

  /**
   * Sort the initial insertion buffer (level 0) and sets up its rmin/rmax so
   * that it can be used in the rest of the sketch.
   */
  void sort_level_0() {
    std::sort(m_levels[0].begin(), m_levels[0].end(), m_comparator);
    for (size_t i = 0;i < m_levels[0].size(); ++i) {
      // paper uses ranks from 1 to N. We unfortunately need to keep to this
      // to ensure that the r_min computation and r_max computation in
      // merge() works correctly and matches the paper
      m_levels[0][i].rmin = i + 1;
      m_levels[0][i].rmax = i + 1;
    }
  }

  /**
   * Compresses the vector by introducing some amount of additional error.
   */
  void compress(std::vector<element>& vec, double additional_error) {
    double b = 1.0 / additional_error;
    size_t num_values = std::ceil(2.0 * b) + 1;
    if (num_values < 2) num_values = 2; // we must get at least min and max
    compress_to_size(vec, num_values);
  }

  /**
   * Decreases the size of a sketch by selecting a subset of elements
   */
  void compress_to_size(std::vector<element>& vec, size_t selection_size) {
    if (selection_size >= vec.size()) return;
    double step_size = (double)vec.size() / selection_size;
    for (size_t i = 0;i < selection_size - 1; ++i) {
      size_t targ = step_size * i;
      if (targ >= vec.size()) targ = vec.size() - 1;
      vec[i] = vec[targ];
    }
    vec[selection_size - 1] = vec[vec.size() - 1];
    vec.resize(selection_size);
  }

  /**
   * Performs a "merge" on two sorted sketches, updating r_min / r_max as
   * we go along.
   */
  std::vector<element> merge(std::vector<element>& left,
                             std::vector<element>& right) {
    // trivial cases
    if (left.size() == 0) return right;
    if (right.size() == 0) return left;

    // output array
    std::vector<element> ret(left.size() + right.size());
    // left iteration index
    size_t leftidx = 0;
    // right iteration index
    size_t rightidx = 0;
    // output index
    size_t outidx = 0;
    // parallel iteration over left and right
    while(leftidx < left.size() && rightidx < right.size()) {
      if (m_comparator(left[leftidx], right[rightidx]) ||
         (left[leftidx].val == right[rightidx].val)) {
        // left is smaller
        ret[outidx] = element(
          std::move(left[leftidx].val),
          left[leftidx].rmin + (rightidx > 0 ? right[rightidx - 1].rmin : 0),
          left[leftidx].rmax + (right[rightidx].rmax > 0 ? right[rightidx].rmax - 1 : 0));
        ++leftidx;
      } else {
        // right is smaller
        ret[outidx] = element(
          std::move(right[rightidx].val),
          right[rightidx].rmin + (leftidx > 0 ? left[leftidx - 1].rmin : 0),
          right[rightidx].rmax + (left[leftidx].rmax > 0 ? left[leftidx].rmax - 1 : 0));
        ++rightidx;
      }
      ++outidx;
    }
    // right is finished. left is remaining
    while(leftidx < left.size()) {
      ret[outidx] = element(
        left[leftidx].val,
        // note that the rmax formula here is different
        left[leftidx].rmin + right[rightidx - 1].rmin,
        left[leftidx].rmax + right[rightidx - 1].rmax);
      ++leftidx;
      ++outidx;
    }
    // left is finished. right is remaining
    while(rightidx < right.size()) {
      ret[outidx] = element(
        right[rightidx].val,
        // note that the rmax formula here is different
        right[rightidx].rmin + left[leftidx - 1].rmin,
        right[rightidx].rmax + left[leftidx - 1].rmax);
      ++rightidx;
      ++outidx;
    }
    return ret;
  }

  /**
   * Inserts level 0 into the remainder of the multilevel sketch.
   * Corresponds to algorithm 1 in the paper. Called "update".
   *
   * Essentially, attempts to insert sc into the starting level.
   * If there is no sketch at that level, moves it in and returns.
   * If there is a sketch at that level, merge + compresses it with the sketch
   * at that level, clears the sketch at that level then attempts to insert
   * the new sketch at the next higher level.
   * (it sort of resembles binary addition)
   */
  void compact(std::vector<element>& sc, size_t starting_level = 1) {
    for (size_t i = starting_level;i < m_levels.size(); ++i) {
      if (m_levels[i].size() == 0) {
        m_levels[i] = std::move(sc);
        return;
      } else {
        sc = merge(sc, m_levels[i]);
        compress(sc, 1.0 / m_b);
        m_levels[i].clear();
      }
    }
    m_levels.push_back(sc);
  }

  /**
   * Constructs the query array through a binary recursive merge of all
   * levels of the multilevel sketch.
   */
  std::vector<element> recursive_merge_of_all_levels(size_t start, size_t end) {
    if (end - start == 1) return std::move(m_levels[start]);
    else if (end - start == 2) return merge(m_levels[start], m_levels[start + 1]);
    else {
      size_t midpoint = start + (end - start) / 2;
      std::vector<element> left = recursive_merge_of_all_levels(start, midpoint);
      std::vector<element> right = recursive_merge_of_all_levels(midpoint, end);
      return merge(left, right);
    }
  }

  /**
   * Compares two element objects by the center of the rmin/rmax range,
   * returning true if the center of the left range, is lesser than the
   * center of the right range.
   */
  static bool rank_center_comparator(const element& left,
                                     const element& right) {
    float center_left = left.rcenter();
    float center_right = right.rcenter();
    return center_left < center_right;
  }



  typename std::vector<element>::const_iterator fast_query_iterator(size_t rank) const {
    ++rank;
    // special handling for min and max
    if (rank <= 1) {
      return m_query.begin();
    }
    if (rank >= m_elements_inserted) {
      return m_query.begin() + (m_query.size() - 1);
    }
    element search_elem(T(), rank, rank);
    // This will find the first element >= to the query
    auto iter = std::lower_bound(m_query.begin(), m_query.end(),
                                 search_elem, rank_center_comparator);
    if (iter == m_query.end()) {
      // at the end, must be the last element
      return m_query.begin() + (m_query.size() - 1);
    } else if (iter == m_query.begin()) {
      // at the start, must be the first element
      return iter;
    } else {
      // closest could be current, or one left.
      auto leftiter = iter - 1;
      float left_distance = std::fabs(leftiter->rcenter() - rank);
      float right_distance = std::fabs(iter->rcenter() - rank);
      return left_distance < right_distance ? leftiter : iter;
    }
  }

  friend class streaming_quantile_sketch<T, Comparator>;
};

} // namespace sketch
} // namespace turi
#endif
