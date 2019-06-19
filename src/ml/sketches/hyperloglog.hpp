/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCH_HYPERLOGLOG_HPP
#define TURI_SKETCH_HYPERLOGLOG_HPP
#include <cmath>
#include <cstdint>
#include <functional>
#include <core/util/cityhash_tc.hpp>
#include <core/logging/assertions.hpp>
namespace turi {
namespace sketches {
/**
 * \ingroup sketching
 * An implementation of the hyperloglog sketch for estimating the number of
 * unique elements in a datastream.
 *
 * Implements the hyperloglog sketch algorithm as described in:
 *   Philippe Flajolet, Eric Fusy, Olivier Gandouet and Frederic Meunier.
 *   HyperLogLog: the analysis of a near-optimal cardinality
 *   estimation algorithm. Conference on Analysis of Algorithms (AofA) 2007.
 *
 * with further reference from:
 *   Stefan Heule, Marc Nunkesser and Alexander Hall.
 *   HyperLogLog in Practice: Algorithmic Engineering of a State of The
 *   Art Cardinality Estimation Algorithm.
 *   Proceedings of the EDBT 2013 Conference.
 *
 *
 * Usage is simple.
 * \code
 *   hyperloglog hll;
 *   // repeatedly call
 *   hll.add(stuff) // this is a templatized function.
 *                  // will accept anything it can hash using std::hash.
 *   hll.estimate() // will return an estimate of the number of unique element
 *   hll.error_bound() // will return the standard deviation on the estimate
 * \endcode
 */
class hyperloglog {
 private:
  size_t m_b = 0; /// 2^b is the number of hash bins
  size_t m_m = 0; /// equal to 2^b: The number of buckets
  double m_alpha = 0;
  std::vector<unsigned char> m_buckets; /// Buckets

 public:
  /**
   * Constructs a hyperloglog sketch using 2^b buckets.
   * The resultant hyperloglog datastructure will require
   * 2^b bytes of memory. b must be at least 4. (i.e. 2^4 buckets).
   */
  explicit inline hyperloglog(size_t b = 16):
      m_b(b), m_m(1 << b), m_buckets(m_m, 0) {
     ASSERT_GE(m_m, 16);
      // constants from SFlajolet et al. Fig 3
     switch(m_m) {
      case 16:
       m_alpha = 0.673;
       break;
      case 32:
       m_alpha = 0.697;
       break;
      case 64:
       m_alpha = 0.709;
       break;
      default:
       m_alpha = 0.7213 / (1 + 1.079 / (double)m_m);
     }
   };

  /**
   * Adds an arbitrary object to be counted. Any object type can be used,
   * and there are no restrictions as long as std::hash<T> can be used to
   * obtain a hash value.
   */
  template <typename T>
  void add(const T& t) {
    // we use std::hash first, to bring it to a 64-bit number
    // Then cityhash's hash64 twice to distribute the hash.
    // empirically, one hash64 does not produce enough scattering to
    // get a good estimate
    size_t h = hash64(hash64(std::hash<T>()(t)));
    size_t index = h >> (64 - m_b);
    DASSERT_LT(index, m_buckets.size());
    unsigned char pos = h != 0 ? 1 + __builtin_clz(h) : sizeof(size_t);
    m_buckets[index] = std::max(m_buckets[index], pos);
  }

  /**
   * Merge two hyperloglog datastructures.
   * The two data structures must be constructed with the same number of
   * buckets. Combining of two sketches constructed on two disjoint data
   * streams produces identical results to generating one sketch on the both
   * data streams.
   */
  void combine(const hyperloglog& other) {
    ASSERT_EQ(m_buckets.size(), other.m_buckets.size());
    for (size_t i = 0;i < m_buckets.size(); ++i) {
      m_buckets[i] = std::max(m_buckets[i], other.m_buckets[i]);
    }
  }


  /**
   * Returns the standard error of the estimate.
   */
  inline double error_bound() {
    return estimate() * 1.04 / std::sqrt(m_m);
  }

  /**
   * Returns the estimate of the number of unique items.
   */
  inline double estimate() {
    double E = 0;
    for (size_t i = 0;i < m_buckets.size(); ++i) {
      E += std::pow(2.0, -(double)m_buckets[i]);
    }
    E = m_alpha * m_m * m_m / E;
    // perform bias correction for small values
    if (E <= 2.5 * m_m) {
      size_t zero_count = 0;
      for(auto i: m_buckets) zero_count += (i == 0);
      if (zero_count != 0) E = m_m * std::log((double)m_m / zero_count);
    }
    // we do not need correction for large values 64-bit hash, assume
    // collisions are unlikely
    return E;
  }
}; // hyperloglog
} // namespace sketch
} // namespace turi
#endif
