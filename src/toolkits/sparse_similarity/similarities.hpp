/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_ITEM_SIMILARITY_LOOKUP_AGGREGATORS_H_
#define TURI_UNITY_ITEM_SIMILARITY_LOOKUP_AGGREGATORS_H_

#include <core/data/flexible_type/flexible_type.hpp>

namespace turi { namespace sparse_sim {

/** Generic classes for implementing similarities.  All the math of
 *  calculating the actual similarity goes here.  Denote the rating of
 *  a user/item pair is r_ui.
 *
 *
 *  The classes must all define the following types:
 *
 *     item_data_type;
 *     interaction_data_type;
 *     final_item_data_type;
 *     final_interaction_data_type;
 *
 *
 *  And the following methods that do the math computation:
 *
 *     void update_item(item_data_type&, double)
 *     void finalize_item(final_item_data_type&, item_data_type&)
 *
 *     void update_interaction(interaction_data_type& e,
 *                            const item_data_type& v1, const item_data_type& v2,
 *                            double new_v1, double new_v2)
 *
 *     void finalize_interaction(final_interaction_data_type& e_out,
 *                               const final_item_data_type&,
 *                               const final_item_data_type&,
 *                               const interaction_data_type& e,
 *                               const item_data_type& v1,
 *                               const item_data_type& v2)
 *
 *
 *  With these functions, the similarity of two items is calculated using the following
 *  algorithm:
 *
 *
 *  for i1 in items:
 *     vb[i1] <- item_data_type()
 *
 *     for u in users(i1):
 *        update_item(vb[i1], rating[u, i1])
 *
 *     v[i1] <- final_item_data_type()
 *     finalize_item(v[i1], vb[i1])
 *
 *
 *
 *  for i1 in items:
 *     for i2 in items:
 *        eb[i1, i2] <- interaction_data_type()
 *        for u in (users(i1) & users(i2)):
 *            update_interaction(eb[i1, i2], vb[i1], vb[i2], rating[u, i1], rating[u, i2])
 *
 *        e[i1, i2] <- final_interaction_data_type()
 *        finalize_interaction(e[i1, i2], v[i1], v[i2], eb[i1, i2], vb[i1], vb[i2])
 *
 *
 *   Then, the top edge values for each item are saved as the nearest
 *   neighbors, where "top" is determined by compare_interactions:
 *
 *      // Returns true if e1 is better than e2, and false otherwise.
 *      bool compare_interaction_values(const final_interaction_data_type& e1,
 *                               const final_interaction_data_type& e2,
 *                               const final_item_data_type& common_item_data,
 *                               const final_item_data_type& item_data_1,
 *                               const final_item_data_type& item_data_2)
 *
 *  ////////////////////////////////////////////////////////////////////////////////
 *
 *  Additional methods:
 *
 *   // True if vertex operations are not atomic.
 *   static constexpr bool require_item_locking();
 *
 *   // True if edge operations are not atomic.
 *   static constexpr bool require_interaction_locking();
 *
 *   // True if missing elements can be treated as zero, and false otherwise.
 *   static constexpr bool missing_values_are_zero();
 */


// A struct used to indicate that a particular type (typically the
// final vertex type) is unused.
struct unused_value_type : public IS_POD_TYPE {};

// For speed and parallel processing with atomics, we use size_t for
// accumulating the recommendations as we can use efficient atomic
// operations with them.  This then uses fixed point math, scaled by
// the following.
static constexpr int64_t _fixed_precision_scale_factor = (size_t(1) << 24);

typedef int64_t _fixed_precision_type;

static inline _fixed_precision_type _to_fixed(double v) {
  return int64_t(std::round(v * _fixed_precision_scale_factor));
}

static inline double _from_fixed(_fixed_precision_type v) {
  return double(v) / _fixed_precision_scale_factor;
}

class jaccard {
 public:

  static std::string name() { return "jaccard"; }

  // At the end, we have
  typedef size_t item_data_type;
  typedef size_t interaction_data_type;
  typedef _fixed_precision_type final_interaction_data_type;
  typedef unused_value_type final_item_data_type;

  static constexpr bool require_item_locking() { return false; }
  static constexpr bool require_interaction_locking() { return false; }
  static constexpr bool missing_values_are_zero() { return true; }

  void update_item(item_data_type& v, double target) const {
    if(LIKELY(target != 0)) {
      atomic_increment(v);
    }
  }

  void update_item_unsafe(item_data_type& v, double target) const {
    if(LIKELY(target != 0)) {
      ++v;
    }
  }

  void finalize_item(final_item_data_type&, item_data_type&) const { }
  void import_final_item_value(final_item_data_type& it, const flexible_type& src) const { }

  void update_interaction(interaction_data_type& e,
                          const item_data_type& v1, const item_data_type& v2,
                          double new_v1, double new_v2) const {
    if(LIKELY((new_v1 != 0) && (new_v2 != 0)) ) {
      atomic_increment(e);
    }
  }

  void update_interaction_unsafe(interaction_data_type& e,
                                 const item_data_type& v1, const item_data_type& v2,
                                 double new_v1, double new_v2) const {

    // Increment if both e and the double values are non-zero.  This
    // version is easier for the compiler to vectorize.
    e += ((new_v1 != 0) & (new_v2 != 0));
  }

  void finalize_interaction(final_interaction_data_type& e_out,
                     const final_item_data_type&,
                     const final_item_data_type&,
                     const interaction_data_type& e,
                     const item_data_type& v1,
                     const item_data_type& v2) const {

    // The intersection count should be less than the size of either one.
    DASSERT_LE(e, v1);
    DASSERT_LE(e, v2);

    double _e_out = ( (v1 == 0) || (v2 == 0) ) ? 0.0 : double(e) / (v1 + v2 - e);

    DASSERT_GE(_e_out, -1e-3);
    DASSERT_LE(_e_out, 1.0 + 1e-3);

    e_out = _to_fixed(_e_out);
  }

  ////////////////////////////////////////////////////////////////////////////////

  bool compare_interaction_values(const final_interaction_data_type& e1,
                                  const final_interaction_data_type& e2,
                                  const final_item_data_type& common_item_data,
                                  const final_item_data_type& item_data_1,
                                  const final_item_data_type& item_data_2) const {
    return e1 > e2;
  }

  ////////////////////////////////////////////////////////////////////////////////

  // Some methods need this for deserialization or setting things directly.
  void import_final_interaction_value(
      final_interaction_data_type& e, const flexible_type& src) const {

    double v = src.get<flex_float>();

    if(v < -1e-3 || v > 1 + 1e-3) {
      auto error_out = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
        std::ostringstream ss;
        ss << "Values for jaccard similarity type must be between 0 and 1; "
           << "Encountered " << v << ".  Please choose an appropriate "
           << "similarity type or transform your values."
           << std::endl;
        log_and_throw(ss.str().c_str());
      };

      error_out();
    }

    e = _to_fixed(v);
  }

  double export_similarity_score(const final_interaction_data_type& e) const {
    return std::max<double>(0, std::min<double>(1, _from_fixed(e)));
  }

  ////////////////////////////////////////////////////////////////////////////////

  // Finally, accumulation data types.
  typedef size_t prediction_accumulation_type;

  void update_prediction(prediction_accumulation_type& p,
                         const final_interaction_data_type& item_interaction_data,
                         const final_item_data_type& prediction_item_item_data,
                         const final_item_data_type& neighbor_item_item_data,
                         double prediction_item_score) const {

    if(LIKELY(prediction_item_score != 0)) {
      atomic_increment(p, item_interaction_data);
    }
  }

  void update_prediction_unsafe(prediction_accumulation_type& p,
                                const final_interaction_data_type& item_interaction_data,
                                const final_item_data_type& prediction_item_item_data,
                                const final_item_data_type& neighbor_item_item_data,
                                double prediction_item_score) const {

    p += (prediction_item_score != 0) ? item_interaction_data : 0;
  }

  double finalize_prediction(const prediction_accumulation_type& p,
                             const final_item_data_type& prediction_item_data,
                             size_t n_user_ratings) const {
    return _from_fixed(p) / std::max<size_t>(1, n_user_ratings);
  }

};

////////////////////////////////////////////////////////////////////////////////
//

class cosine {
 public:

  static std::string name() { return "cosine"; }

  // The long here is so we can use atomics.
  typedef _fixed_precision_type item_data_type;
  typedef _fixed_precision_type interaction_data_type;

  typedef _fixed_precision_type final_interaction_data_type;
  typedef unused_value_type final_item_data_type;

  static constexpr bool require_item_locking() { return false; }
  static constexpr bool require_interaction_locking() { return false; }
  static constexpr bool missing_values_are_zero() { return true; }

  void update_item(item_data_type& v, double target) const {
    atomic_increment(v, _to_fixed(target * target));
  }

  void update_item_unsafe(item_data_type& v, double target) const {
    v += _to_fixed(target * target);
  }

  // no item final data.
  void finalize_item(final_item_data_type&, item_data_type&) const { }
  void import_final_item_value(final_item_data_type& it, const flexible_type& src) const { }

  void update_interaction(interaction_data_type& e,
                          const item_data_type& v1, const item_data_type& v2,
                          double new_v1, double new_v2) const {

    atomic_increment(e, _to_fixed(new_v1 * new_v2));
  }

  void update_interaction_unsafe(interaction_data_type& e,
                                 const item_data_type& v1, const item_data_type& v2,
                                 double new_v1, double new_v2) const {
    e += _to_fixed(new_v1 * new_v2);
  }

  void finalize_interaction(final_interaction_data_type& e_out,
                            const final_item_data_type&,
                            const final_item_data_type&,
                            const interaction_data_type& e,
                            const item_data_type& v1,
                            const item_data_type& v2) const {

    // e, v1, and v2 all use fixed point math, but the ratio is the
    // same, so just cast to double.
    double _e_out = ((v1 == 0) || (v2 == 0)) ? 0.0 : double(e) / std::sqrt(double(v1) * v2);

    DASSERT_LT(_e_out, 1.0 + 1e-3);
    DASSERT_GT(_e_out, -1.0 - 1e-3);

    e_out = _to_fixed(_e_out);
  }

  ////////////////////////////////////////////////////////////////////////////////

  bool compare_interaction_values(const final_interaction_data_type& e1,
                                  const final_interaction_data_type& e2,
                                  const final_item_data_type& common_item_data,
                                  const final_item_data_type& item_data_1,
                                  const final_item_data_type& item_data_2) const {
    return e1 > e2;
  }

  // Some methods need this for deserialization or setting things directly.
  void import_final_interaction_value(
      final_interaction_data_type& e, const flexible_type& src) const {

    double v = src.get<flex_float>();

    if(v < (-1 - 1e-3) || v > 1 + 1e-3) {
      auto error_out = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
        std::ostringstream ss;
        ss << "Values for cosine similarity type must be between -1 and 1; "
           << "Encountered " << v << ".  Please choose an appropriate "
           << "similarity type or transform your values."
           << std::endl;
        log_and_throw(ss.str().c_str());
      };

      error_out();
    }

    e = _to_fixed(v);
  }

  double export_similarity_score(const final_interaction_data_type& e) const {
    return std::max<double>(-1, std::min<double>(1, _from_fixed(e)));
  }

  ////////////////////////////////////////////////////////////////////////////////

  // Finally, accumulation data types.
  typedef _fixed_precision_type prediction_accumulation_type;

  void update_prediction_unsafe(prediction_accumulation_type& p,
                                const final_interaction_data_type& item_interaction_data,
                                const final_item_data_type& prediction_item_item_data,
                                const final_item_data_type& neighbor_item_item_data,
                                double prediction_item_score) const {

    _fixed_precision_type delta_prediction
        = _fixed_precision_type(item_interaction_data * prediction_item_score);

    p += delta_prediction;
  }

  void update_prediction(prediction_accumulation_type& p,
                         const final_interaction_data_type& item_interaction_data,
                         const final_item_data_type& prediction_item_item_data,
                         const final_item_data_type& neighbor_item_item_data,
                         double prediction_item_score) const {

    _fixed_precision_type delta_prediction
        = _fixed_precision_type(item_interaction_data * prediction_item_score);

    atomic_increment(p, delta_prediction);
  }

  double finalize_prediction(const prediction_accumulation_type& p,
                             const final_item_data_type& prediction_item_item_data,
                             size_t n_user_ratings) const {
    if(n_user_ratings == 0) {
      return 0;
    } else {
      return _from_fixed(p) / n_user_ratings;
    }
  }
};


////////////////////////////////////////////////////////////////////////////////
//

class pearson {
 public:

  static std::string name() { return "pearson"; }

  // At the end, we have
  struct item_data_type {
    size_t count = 0;
    double mean = 0;
    double var_sum = 0;
  };

  typedef double interaction_data_type;

  typedef _fixed_precision_type final_interaction_data_type;
  typedef double final_item_data_type;

  static constexpr bool require_item_locking() { return true; }
  static constexpr bool require_interaction_locking() { return true; }
  static constexpr bool missing_values_are_zero() { return false; }

  void update_item(item_data_type& v, double target) const {
    double old_mean = v.mean;

    // Do the efficient and stable mean+stddev computation.
    v.mean += (target - old_mean) / (v.count + 1);

    v.var_sum += (target - old_mean) * (target - v.mean);
    ++v.count;
  }

  void finalize_item(final_item_data_type& fv, item_data_type& v) const {
    v.var_sum *= double(v.count) / std::max<size_t>(1, v.count - 1);
    fv = v.mean;
  }

  void import_final_item_value(final_item_data_type& it, const flexible_type& src) const {
    it = src.get<flex_float>();
  }

  void update_interaction(interaction_data_type& e,
                          const item_data_type& v1,
                          const item_data_type& v2,
                          double new_v1, double new_v2) const {
    e += (new_v1 - v1.mean) * (new_v2 - v2.mean);
  }

  void update_interaction_unsafe(
      interaction_data_type& e, const item_data_type& v1, const item_data_type& v2,
      double new_v1, double new_v2) const {

    e += (new_v1 - v1.mean) * (new_v2 - v2.mean);
  }


  void finalize_interaction(final_interaction_data_type& e_out,
                            const final_item_data_type&,
                            const final_item_data_type&,
                            const interaction_data_type& e,
                            const item_data_type& v1,
                            const item_data_type& v2) const {

    double denominator_2 = v1.var_sum * v2.var_sum;

    double _e_out = (denominator_2 > 0) ? e / std::sqrt(denominator_2) : 0.0;

    DASSERT_LT(_e_out, 1.0 + 1e-3);
    DASSERT_GT(_e_out, -1.0 - 1e-3);

    e_out = _to_fixed(_e_out);
  }

  // Some methods need this for deserialization or setting things directly.
  void import_final_interaction_value(
      final_interaction_data_type& e, const flexible_type& src) const {

    double v = src.get<flex_float>();

    if(v < (-1 - 1e-3) || v > 1 + 1e-3) {
      auto error_out = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
        std::ostringstream ss;
        ss << "Values for pearson correlation similarity type must be between -1 and 1; "
           << "Encountered " << v << ".  Please choose an appropriate "
           << "similarity type or transform your values."
           << std::endl;
        log_and_throw(ss.str().c_str());
      };

      error_out();
    }

    e = _to_fixed(v);
  }

  double export_similarity_score(const final_interaction_data_type& e) const {
    return std::max<double>(-1, std::min<double>(1, _from_fixed(e)));
  }

  ////////////////////////////////////////////////////////////////////////////////

  // Finally, accumulation data types.
  typedef _fixed_precision_type prediction_accumulation_type;

  bool compare_interaction_values(const final_interaction_data_type& e1,
                                  const final_interaction_data_type& e2,
                                  const final_item_data_type& common_item_data,
                                  const final_item_data_type& item_data_1,
                                  const final_item_data_type& item_data_2) const {
    return e1 > e2;
  }

  void update_prediction(prediction_accumulation_type& p,
                         const final_interaction_data_type& item_interaction_data,
                         const final_item_data_type& prediction_item_item_data,
                         const final_item_data_type& neighbor_item_item_data,
                         double prediction_item_score) const {

    _fixed_precision_type delta_prediction = _fixed_precision_type(std::round(
        item_interaction_data * (prediction_item_score - prediction_item_item_data) ) );

    atomic_increment(p, delta_prediction);
  }

  void update_prediction_unsafe(prediction_accumulation_type& p,
                                const final_interaction_data_type& item_interaction_data,
                                const final_item_data_type& prediction_item_item_data,
                                const final_item_data_type& neighbor_item_item_data,
                                double prediction_item_score) const {

    // Note that this is all done at scale
    _fixed_precision_type delta_prediction = _fixed_precision_type(std::round(
        item_interaction_data * (prediction_item_score - prediction_item_item_data) ) );

    p += delta_prediction;
  }

  double finalize_prediction(const prediction_accumulation_type& p,
                             const final_item_data_type& prediction_item_item_data,
                             size_t n_user_ratings) const {
    if(n_user_ratings <= 0) {
      return 0;
    } else {
      return prediction_item_item_data + _from_fixed(p) / n_user_ratings;
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// Utility functions for things

/** Not all of the aggregators use or need to store the
 *  final_item_data, so don't work with it if we don't need to.
 *  This allows us to selectively access it.
 */
template <typename SimilarityType>
static constexpr bool use_final_item_data() {
  return !std::is_same<unused_value_type,
                       typename SimilarityType::final_item_data_type>::value;
}


}}

#endif /* _AGGREGATORS_H_ */
