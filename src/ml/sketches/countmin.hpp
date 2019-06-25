/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCHES_COUNTMIN_HPP
#define TURI_SKETCHES_COUNTMIN_HPP
#include <cmath>
#include <cstdint>
#include <functional>
#include <core/random/random.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
namespace turi {
namespace sketches {

/**
 * \ingroup sketching
 * An implementation of the countmin sketch for estimating the frequency
 * of each item in a stream.
 *
 * For more information on the details of the sketch:
 * http://dimacs.rutgers.edu/~graham/pubs/papers/cmsoft.pdf
 * The implementation generally follows the pseudocode in Figure 2.
 * The resulting probabilistic data structure
 *
 * Usage is simple.
 * \code
 *   countmin<T> cm; // for approximate counting of a stream with type T
 *   // repeatedly call
 *   cm.add(x)      // x can be anything that is hashable.
 *   cm.estimate(x) // will return an estimate of the frequency for
 *                  // a given element
 * \endcode
 * One can obtain guarantees on the error in answering a query is within a factor of
 * ε with probability δ if one sets the
 *    width=ceiling(e / ε)
 *    depth=ceiling(log(1/δ)
 * where e is Euler's constant.
 *
 */
template <typename T>
class countmin {
 private:

  size_t num_hash = 0; /// Number of hash functions to use
  size_t num_bits = 0; /// 2^b is the number of hash bins
  size_t num_bins = 0; /// equal to 2^b: The number of buckets

  std::vector<size_t> seeds;
  std::vector<std::vector<size_t> > counts; /// Buckets

 public:
  /**
   * Constructs a countmin sketch having "width" 2^bits and "depth".
   * The size of the matrix of counts will be depth x 2^bits.
   *
   * \param bits The number of bins will be 2^bits.
   *
   * \param depth The "depth" of the sketch is the number of hash functions
   * that will be used on each item.
   */
  explicit inline countmin(size_t bits = 16, size_t depth = 4, size_t seed=1000):
    num_hash(depth), num_bits(bits), num_bins(1 << num_bits) {
      ASSERT_GE(num_bins, 16);

      random::generator gen;
      gen.seed(seed);
      // Initialize hash functions and count matrix
      for (size_t j = 0; j < num_hash; ++j) {
        seeds.push_back(gen.fast_uniform<size_t>(0, std::numeric_limits<size_t>::max()));
        counts.push_back(std::vector<size_t>(num_bins));
      }
   }

  /**
   * Adds an arbitrary object to be counted. Any object type can be used,
   * and there are no restrictions as long as std::hash<T> can be used to
   * obtain a hash value.
   */
  void add(const T& t, size_t count = 1) {
    // we use std::hash first, to bring it to a 64-bit number
    size_t i = hash64(std::hash<T>()(t));
    for (size_t j = 0; j < num_hash; ++j) {
      size_t bin = hash64(seeds[j] ^ i) % num_bins;  // TODO: bit mask
      counts[j][bin] += count;
    }
  }

  /**
   * Adds an arbitrary object to be counted. Any object type can be used,
   * and there are no restrictions as long as std::hash<T> can be used to
   * obtain a hash value.
   */
  void atomic_add(const T& t, size_t count = 1) {
    // we use std::hash first, to bring it to a 64-bit number
    size_t i = hash64(std::hash<T>()(t));
    for (size_t j = 0; j < num_hash; ++j) {
      size_t bin = hash64(seeds[j] ^ i) % num_bins;  // TODO: bit mask
      __sync_add_and_fetch(&(counts[j][bin]), count);
    }
  }
 /**
   * Returns the estimate of the frequency for a given object.
   */
  inline size_t estimate(const T& t) const {

    size_t E = std::numeric_limits<size_t>::max();
    size_t i = hash64(std::hash<T>()(t));

    // Compute the minimum value across hashes.
    for (size_t j = 0; j < num_hash; ++j) {
      size_t bin = hash64(seeds[j] ^ i) % num_bins;
      if (counts[j][bin] < E)
        E = counts[j][bin];
    }
    return E;
  }

  /**
   * Merge two countmin datastructures.
   * The two countmin objects must have the same width and depth.
   */
  void combine(const countmin& other) {
    ASSERT_EQ(num_bins, other.num_bins);
    ASSERT_EQ(num_hash, other.num_hash);

    for (size_t j = 0; j < num_hash; ++j) {
      for (size_t b = 0; b < num_bins; ++b) {
        counts[j][b] += other.counts[j][b];
      }
    }
  }

  ///// HELPER FUNCTIONS /////////////////

  /**
   * Prints the internal matrix containing the current counts.
   */
  inline void print() const {
    for (size_t j = 0; j < num_hash; ++j) {
      std::cout << ">>> ";
      for (size_t b = 0; b < num_bins; ++b) {
        std::cout << counts[j][b] << " ";
      }
      std::cout << std::endl;
    }
  }

  /**
   * Computes the density of the internal counts matrix.
   */
  inline double density() const {
    size_t count = 0;
    for (size_t j = 0; j < num_hash; ++j) {
      for (size_t b = 0; b < num_bins; ++b) {
        if (counts[j][b] != 0) count += 1;
      }
    }
    return (double) count / (double) (num_hash * num_bins);
  }

  void save(oarchive& oarc) const {
    oarc << num_hash
         << num_bits
         << num_bins
         << seeds
         << counts;
  }
  void load(iarchive& iarc) {
    iarc >> num_hash
         >> num_bits
         >> num_bins
         >> seeds
         >> counts;
  }
}; // countmin
} // namespace sketches
} // namespace turi
#endif
