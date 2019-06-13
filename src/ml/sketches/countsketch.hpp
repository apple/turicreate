/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCHES_COUNTSKETCH_HPP
#define TURI_SKETCHES_COUNTSKETCH_HPP
#include <cmath>
#include <cstdint>
#include <functional>
#include <core/random/random.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/logging/assertions.hpp>
namespace turi {
namespace sketches {

typedef int64_t counter_int;

/**
 * \ingroup sketching
 * An implementation of the CountSketch for estimating the frequency
 * of each item in a stream.
 *
 * Usage is simple.
 * \code
 *   countsketch<T> c; // for approximate counting of a stream with type T
 *   // repeatedly call
 *   c.add(x)      // x can be anything that is hashable.
 *   c.estimate(x) // will return an estimate of the frequency for
 *                 // a given element
 * \endcode
 *
 */
template <typename T>
class countsketch {
 private:

  size_t num_hash = 0; /// Number of hash functions to use
  size_t num_bits = 0; /// 2^b is the number of hash bins
  size_t num_bins = 0; /// equal to 2^b: The number of buckets

  std::vector<size_t> seeds;
  std::vector<size_t> seeds_binary;

  std::vector<std::vector<counter_int> > counts; /// Buckets

 public:
  /**
   * Constructs a CountSketch having "width" 2^bits and "depth".
   * The size of the matrix of counts will be depth x 2^bits.
   *
   * \param bits The number of bins will be 2^bits.
   *
   * \param depth The "depth" of the sketch is the number of hash functions
   * that will be used on each item.
   */
  explicit inline countsketch(size_t bits = 16, size_t depth = 5, size_t seed = 1000):
    num_hash(depth), num_bits(bits), num_bins(1 << num_bits) {
      ASSERT_GE(num_bins, 16);

      random::generator gen;
      gen.seed(seed);
      // Initialize hash functions and count matrix
      for (size_t j = 0; j < num_hash; ++j) {
        seeds.push_back(gen.fast_uniform<size_t>(0, std::numeric_limits<size_t>::max()));
        seeds_binary.push_back(gen.fast_uniform<size_t>(0, std::numeric_limits<size_t>::max()));

        counts.push_back(std::vector<counter_int>(num_bins));
      }
   }

  /**
   * Adds an arbitrary object to be counted. Any object type can be used,
   * and there are no restrictions as long as std::hash<T> can be used to
   * obtain a hash value.
   *
   * Note:
   * Theoretical properties only apply to the situation where count is 1.
   */
  void add(const T& t, size_t count = 1) {

    // Create a 64-bit number from the object
    size_t i = hash64(std::hash<T>()(t));

    for (size_t j = 0; j < num_hash; ++j) {
      // convert trailing bit to 1 or -1
      counter_int s = (counter_int)( hash64(seeds_binary[j] ^ i) & 1);
      s = 2*s - 1;
      // compute which bin to increment
      size_t bin = hash64(seeds[j] ^ i) % num_bins;  // TODO: bit mask
      counts[j][bin] += s * (counter_int) count;
    }
  }

 /**
   * Returns the estimate of the frequency for a given object.
   */
  inline counter_int estimate(const T& t) {

    // Create a 64-bit number from the object
    size_t i = hash64(std::hash<T>()(t));

    // Compute the minimum value across hashes.
    std::vector<counter_int> estimates;
    for (size_t j = 0; j < num_hash; ++j) {
      // convert trailing bit to 1 or -1
      counter_int s = (counter_int) (hash64(seeds_binary[j] ^ i) & 1);  // convert trailing bit to 1 or -1
      s = 2*s - 1;
      // compute which bin to increment
      size_t bin = hash64(seeds[j] ^ i) % num_bins;  // TODO: bit mask
      counter_int estimate = s * counts[j][bin];
      estimates.push_back(estimate);
    }

    // Return the median
    std::nth_element(estimates.begin(),
                     estimates.begin() + estimates.size()/2,
                     estimates.end());

    return estimates[estimates.size()/2];
  }

  /**
   * Merge two CountSketch datastructures.
   * The two countsketch objects must have the same width and depth.
   */
  void combine(const countsketch& other) {
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
  inline void print() {
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
  inline double density() {
    size_t count = 0;
    for (size_t j = 0; j < num_hash; ++j) {
      for (size_t b = 0; b < num_bins; ++b) {
        if (counts[j][b] != 0) count += 1;
      }
    }
    return (double) count / (double) (num_hash * num_bins);
  }

}; // countsketch
} // namespace sketches
} // namespace turi
#endif
