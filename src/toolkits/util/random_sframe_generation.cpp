/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/random_sframe_generation.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <vector>

using namespace turi;

/**  Creates a random SFrame for testing purposes.  The
 *  column_types gives the types of the column.
 *
 *  \param[in] n_rows The number of observations to run the timing on.
 *  \param[in] random_seed Seed used to determine the running.
 *  \param[in] column_types A string with each character denoting
 *  one type of column.  The legend is as follows:
 *
 *     n:  numeric column, uniform 0-1 distribution.
 *     N:  numeric column, uniform 0-1 distribution, 1% NaNs.
 *     r:  numeric column, uniform -100 to 100 distribution.
 *     R:  numeric column, uniform -10000 to 10000 distribution, 1% NaNs.
 *     b:  binary integer column, uniform distribution
 *     z:  integer column with random integers between 1 and 10.
 *     Z:  integer column with random integers between 1 and 100.
 *     s:  categorical string column with 10 different unique short strings.
 *     S:  categorical string column with 100 different unique short strings.
 *     c:  categorical column with short string keys and 1000 unique values, triangle distribution.
 *     C:  categorical column with short string keys and 100000 unique values, triangle distribution.
 *     x:  categorical column with 128bit hex hashes and 1000 unique values.
 *     X:  categorical column with 256bit hex hashes and 100000 unique values.
 *     h:  column with unique 128bit hex hashes.
 *     H:  column with unique 256bit hex hashes.
 *
 *     l:  categorical list with between 0 and 10 unique integer elements from a pool of 100 unique values.
 *     L:  categorical list with between 0 and 100 unique integer elements from a pool of 1000 unique values.
 *     M:  categorical list with between 0 and 10 unique string elements from a pool of 100 unique values.
 *     m:  categorical list with between 0 and 100 unique string elements from a pool of 1000 unique values.
 *
 *     v:  numeric vector with 10 elements and uniform 0-1 elements.
 *     V:  numeric vector with 1000 elements and uniform 0-1 elements.
 *     w:  numeric vector with 10 elements and uniform 0-1 elements, 1% NANs.
 *     W:  numeric vector with 1000 elements and uniform 0-1 elements, 1% NANs.
 *
 *     d: dictionary with with between 0 and 10 string keys from a
 *        pool of 100 unique keys, and random 0-1 values.
 *
 *     D: dictionary with with between 0 and 100 string keys from a
 *        pool of 1000 unique keys, and random 0-1 values.
 *
 * Target Generation
 * -----------------
 *
 * If generate_target is true, then the target value is a linear
 * combination of the features chosen for each row plus uniform noise.
 *
 * - For each numeric and vector columns, each value, with the range
 *   scaled to [-0.5, 0.5] (so r and R type values affect the target just
 *   as much as n an N), is added to the target value.  NaNs are ignored.
 *
 * - For each categorical or string values, it is hash-mapped to a lookup
 *   table of 512 randomly chosen values, each in [-0.5, 0.5], and the
 *   result is added to the target.
 *
 * - For dictionary columns, the keys are treated as adding a categorical
 *   value and the values are treated as adding a numeric value.
 *
 * At the end, a uniform random value is added to the target in the
 * range [(max_target - min_target) * noise_level], where max_target
 * and min_target are the maximum and minimum target values generated
 * by the above process.
 *
 * The final target values are then scaled to [0, 1].
 *
 */
gl_sframe _generate_random_sframe(size_t n_rows, std::string column_types,
                                  size_t random_seed, bool generate_target, double noise_level) {


  gl_sframe ret_sf(make_random_sframe(n_rows, column_types, generate_target, random_seed));

  if(generate_target) {
    double target_min = ret_sf["target"].min();
    double target_max = ret_sf["target"].max();

    // To prevent divide by zeros
    target_max = std::max(target_min + 1, target_max);

    double apl_noise_adj = (target_max - target_min) * noise_level / 2;

    // Add in the noise and map things to [0,1]
    ret_sf["target"] = ret_sf["target"].apply(
        [=](const flexible_type& x) -> flexible_type {
          double v = x;

          DASSERT_LE(v, target_max);
          DASSERT_GE(v, target_min);

          if(noise_level > 0) {
            flex_int ub = std::min<double>(target_max, v + apl_noise_adj);
            flex_int lb = std::max<double>(target_min, v - apl_noise_adj);
            v = ( (double(hash64( uint64_t(v * 100000), random_seed) % 1000000) / 1000000.0) * (ub - lb) + lb);
            DASSERT_LE(v, target_max);
            DASSERT_GE(v, target_min);
          }

          flex_float res = (flex_float(v - target_min) / (target_max - target_min));
          DASSERT_LE(res, 1.0);
          DASSERT_GE(res, 0.0);
          return res;

        }, flex_type_enum::FLOAT, false);
  }

  return ret_sf;
}

/**  Creates a random SFrame for testing purposes, with num_rows,
 *   column_types, and random_seed the same as _generate_random_sframe
 *   above.  In addition, an integer categorical target column is
 *   added by binning the output of the numerical target column given
 *   in _generate_random_sframe.
 *
 *   The target column, called "target", is an integer value that
 *   represents the binning of the output of a noisy linear function
 *   of the chosen random variables into `num_bins_scale_factor *
 *   num_classes` bins, shuffled, and then each bin is mapped to
 *   num_classes values.  This means that some non-linearity is
 *   present if num_bins_scale_factor > 1, but many patterns can be
 *   learned. For details on how the random linear value is chosen,
 *   see the documentation on
 *   :ref:`generate_random_regression_sframe`.
 *
 *   The noise_level is handled identically to
 *   _generate_random_sframe, except that noise_level is divided by
 *   the number of bins.  Thus it here represents the probability that
 *   a class will be mapped to the wrong neighboring bin.
 */
gl_sframe _generate_random_classification_sframe(size_t n_rows, std::string column_types,
                                                 size_t random_seed, size_t num_classes,
                                                 size_t num_extra_class_bins,
                                                 double misclassification_spread) {

  // Set up the bin mapping.
  size_t n_bins = num_classes + num_extra_class_bins;
  std::vector<size_t> bin_to_class_map(n_bins);

  gl_sframe X = _generate_random_sframe(n_rows, column_types, random_seed, true,
                                        misclassification_spread / n_bins);

  random::seed(random_seed);

  for(size_t i = 0; i < num_classes; ++i) {
    bin_to_class_map[i] = i;
  }

  for(size_t i = num_classes; i < n_bins; ++i) {
    bin_to_class_map[i] = random::fast_uniform<size_t>(0, num_classes - 1);
  }

  random::shuffle(bin_to_class_map);

  std::function<flexible_type(const flexible_type&)> classify
      = [=](const flexible_type& x) {
    size_t b = std::min<flex_int>(n_bins - 1,
        std::max<flex_int>(0,
            flex_int(std::floor(x.get<flex_float>() * n_bins))));

    return bin_to_class_map[b];
  };

  X["target"] = X["target"].apply(classify, flex_type_enum::INTEGER, false);

  return X;
}
