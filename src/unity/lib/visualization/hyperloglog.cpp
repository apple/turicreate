/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "hyperloglog.hpp"

namespace turi {
namespace visualization {

/**
 * Hyperloglog sketch for estimating the number
 * of unique elements in a datastream.
 *
 * Implements the hyperloglog sketch algorithm as described in:
 *   Philippe Flajolet, Eric Fusy, Olivier Gandouet and Frederic Meunier.
 *   HyperLogLog: the analysis of a near-optimal cardinality
 *   estimation algorithm. Conference on Analysis of Algorithms (AofA) 2007.
 *
 * see sketches/hyperloglog.hpp
 */

// default number of buckets
const static size_t BATCH_SIZE = 10000000;
const static size_t BUCKET_SIZE = 16;
const static size_t m_m = 1 << BUCKET_SIZE;

hyperloglog::hyperloglog() : m_hll(BUCKET_SIZE) {}

void hyperloglog::init(const gl_sarray& source) {
  m_source = source;
}

bool hyperloglog::eof() const {
  return m_currentIdx >= m_source.size();
}

/**
 * Returns the standard error.
 *
 * Quoting Flajolet et al.
 * Let sig ~ 1.04 / sqrt(m) represent the standard error; the estimates provided
 * by HYPERLOGLOG are expected to be within sig, 2sig, 3sig of the exact count
 * in respectively 65%, 95%, 99% of all the cases.
 */
double hyperloglog::error_bound() {
  return m_estimate * 1.04 / std::sqrt(m_m);
}

double hyperloglog::get() {

  if (eof()) {
    // return the final cached estimate
    return m_estimate;
  }

  size_t start = m_currentIdx;
  size_t end = std::min(m_currentIdx + BATCH_SIZE, m_source.size());

  for (const auto& value : m_source.range_iterator(start, end)) {
    m_hll.add<flexible_type>(value);
  }
  m_currentIdx = end;

  // cache the estimate calculation
  m_estimate = m_hll.estimate();
  return m_estimate;
}

}}
