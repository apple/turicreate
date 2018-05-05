/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/util/random_sframe_generation.hpp>
#include <util/cityhash_tc.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
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
                                  size_t _random_seed, bool generate_target, double noise_level) {

  ASSERT_MSG(n_rows > 0, "Number of rows must be greater than 0.");

  sframe data;

  size_t num_columns = column_types.size();
  size_t n_threads = thread::cpu_count();

  std::vector<std::string> names;
  std::vector<flex_type_enum> types;

  names.resize(column_types.size());
  types.resize(column_types.size());

  ////////////////////////////////////////////////////////////////////////////////
  //  Set up the information lookups for each of the columns: type,
  //  whether it's categorical, and the description to print.
  //
  for(size_t c_idx = 0; c_idx < num_columns; c_idx++){

    names[c_idx] = std::string("X") + std::to_string(c_idx + 1) + "-" + column_types[c_idx];

    switch(column_types[c_idx]) {
      case 'n':
      case 'N':
      case 'r':
      case 'R':
        types[c_idx] = flex_type_enum::FLOAT;
        break;

      case 'b':
      case 'z':
      case 'Z':
        types[c_idx] = flex_type_enum::INTEGER;
        break;

      case 'c':
      case 'C':
      case 's':
      case 'S':
      case 'x':
      case 'X':
      case 'h':
      case 'H':
        types[c_idx] = flex_type_enum::STRING;
        break;

      case 'v':
      case 'V':
      case 'w':
      case 'W':
        types[c_idx] = flex_type_enum::VECTOR;
        break;

      case 'l':
      case 'L':
      case 'm':
      case 'M':
        types[c_idx] = flex_type_enum::LIST;
        break;

      case 'd':
      case 'D':
        types[c_idx] = flex_type_enum::DICT;
        break;

      default:
        std::string msg = (std::string("Column type ") + column_types[c_idx] + " not recognized.");
        ASSERT_MSG(false, msg.c_str());
        break;

    }
  }

  size_t target_column = names.size();
  if(generate_target) {
    names.push_back("target");
    types.push_back(flex_type_enum::INTEGER); // Changed to float later on.
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Create the sframe with each of the columns as determined above.

  data.open_for_write(names, types, "", n_threads);

  static const size_t n_bins = 256;
  static const size_t n_target_precision = (1 << 24);

  // Hash it once for a bit of extra random_ness.
  uint64_t random_seed = hash64(_random_seed);

  std::vector<flex_int> target_adjust;

  if(generate_target) {
    target_adjust.resize(n_bins);
    size_t c = 0;
    for(flex_int & x : target_adjust) {
      x = long(hash64(++c, random_seed) % n_target_precision) - (n_target_precision/2);
    }
  }

  // Accumulators for the min and max target values for easy scaling later.
  std::vector<flex_int> target_min_v(n_threads, 0);
  std::vector<flex_int> target_max_v(n_threads, 0);

  in_parallel([&](size_t thread_idx, size_t num_segments) {

    auto it_out = data.get_output_iterator(thread_idx);

    std::vector<flexible_type> row(column_types.size() + (generate_target ? 1 : 0));

    size_t start_idx = (thread_idx * n_rows) / num_segments;
    size_t end_idx = ((thread_idx + 1) * n_rows) / num_segments;

    for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
      /** Base random number generators.  If there is a target
       *  present, then these also affect the target
       */
      size_t _rng_state = hash64(i, random_seed);

      // Start the target seed for this row.
      flex_int target_value = 0;

      // Go through the columns, randomly filling each.
      for(size_t c_idx = 0; c_idx < column_types.size(); ++c_idx) {

        auto rng_int = [&](size_t lb, size_t ub) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          size_t z = size_t(hash64(++_rng_state) % (ub - lb + 1));
          if(generate_target) {
            target_value += target_adjust[z % target_adjust.size()];
          }
          return z + lb;
        };

        auto rng_dbl = [&](double lb, double ub) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

          double v01 = double(hash64(++_rng_state)) / std::numeric_limits<uint64_t>::max();
          if(generate_target) {
            target_value += long(std::round(n_target_precision * v01) - (n_target_precision/2));
          }
          return lb + (ub - lb) * v01;
        };

        /** Composite random number generators.
         */
        auto rng_dbl_nan = [&](double lb, double ub) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          return (hash64(++_rng_state) < (std::numeric_limits<uint64_t>::max() / 100)
                  ? NAN : rng_dbl(lb,ub));
        };

        // Generate a random hex string of the form "C-###"
        auto rng_str = [&](size_t pool_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          // Everything is deterministic from a random set (1, ...,
          // pool_size), allowing for limiting the number of random things available
          char ret[16];
          std::fill(ret, ret + 16, '\0');
          snprintf(&ret[0], 16, "C-%ld", rng_int(0, pool_size-1));
          return std::string(ret);
        };

        // Generate a random hex key
        auto rng_hex = [&](size_t length, size_t pool_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

          static const char charset[] = "0123456789abcdef";

          // Everything is deterministic from a random set (1, ...,
          // pool_size), allowing for limiting the number of random things available
          size_t x = hash64(random_seed, hash64(++_rng_state) % pool_size);

          std::string ret;
          ret.reserve(length);

          for(size_t i = 0; i < length; i += 8) {
            uint64_t number = x;

            for(size_t j = 0; j < 16; ++j) {
              ret.push_back(charset[number & 0xF]);
              number >>= 4;
              if(ret.size() >= length) {
                return ret;
              }
            }

            x = hash64(x);
          }

          return ret;
        };

        // Generate a random list
        auto rng_list = [&](size_t max_size, size_t key_pool_size, bool string_values) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN)  {
          size_t s = rng_int(0,max_size);
          flex_list v(s);
          for(flexible_type& f : v) {
            if(string_values) {
              f = rng_str(key_pool_size);
            } else {
              f = rng_int(1, key_pool_size);
            }
          }

          return v;
        };

        // Generate a random vector
        auto rng_vec = [&](size_t s) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          flex_vec v(s);
          for(double& f : v) f = rng_dbl(0,1);
          return v;
        };

        // Generate a random vector
        auto rng_vec_nan = [&](size_t s) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          flex_vec v(s);
          for(double& f : v) f = rng_dbl_nan(0,1);
          return v;
        };

        // Generate a random dictionary
        auto rng_dict = [&](size_t max_size, size_t key_pool_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          std::map<size_t,double> m;

          size_t s = rng_int(0, max_size);

          for(size_t i = 0; i < s; ++i) {
            size_t index = rng_int(1, key_pool_size);
            double value = rng_dbl(0, 1);
            m[index] = value;
          }

          flex_dict d;
          d.reserve(m.size());
          char key[16];

          for(const auto& p : m) {
            snprintf(key, 15, "K-%ld", p.first);
            d.push_back( {flex_string(key), p.second} );
          }

          return d;
        };

        // Based on the column output type, write it out
        switch(column_types[c_idx]){

          case 'n': { row[c_idx] = rng_dbl(0,1);               break; }
          case 'N': { row[c_idx] = rng_dbl_nan(0,1);           break; }
          case 'r': { row[c_idx] = rng_dbl(-100,100);          break; }
          case 'R': { row[c_idx] = rng_dbl_nan(-1000,1000);    break; }
          case 'b': { row[c_idx] = rng_int(0, 1);              break; }
          case 'z': { row[c_idx] = rng_int(1, 10);             break; }
          case 'Z': { row[c_idx] = rng_int(1, 100);            break; }
          case 's': { row[c_idx] = rng_str(10);                break; }
          case 'S': { row[c_idx] = rng_str(100);               break; }
          case 'c': { row[c_idx] = rng_str(1000);              break; }
          case 'C': { row[c_idx] = rng_str(100000);            break; }
          case 'x': { row[c_idx] = rng_hex(32, 1000);          break; }
          case 'X': { row[c_idx] = rng_hex(64, 100000);        break; }
          case 'h': { row[c_idx] = rng_hex(32, size_t(-1));    break; }
          case 'H': { row[c_idx] = rng_hex(64, size_t(-1));    break; }
          case 'v': { row[c_idx] = rng_vec(10);                break; }
          case 'V': { row[c_idx] = rng_vec(100);               break; }
          case 'w': { row[c_idx] = rng_vec_nan(10);            break; }
          case 'W': { row[c_idx] = rng_vec_nan(100);           break; }
          case 'l': { row[c_idx] = rng_list(10, 100, false);   break; }
          case 'L': { row[c_idx] = rng_list(100, 1000, false); break; }
          case 'm': { row[c_idx] = rng_list(10, 100, true);    break; }
          case 'M': { row[c_idx] = rng_list(100, 1000, true);  break; }
          case 'd': { row[c_idx] = rng_dict(10, 100);          break; }
          case 'D': { row[c_idx] = rng_dict(100, 1000);        break; }

          default:
            std::string msg = (std::string("Column type ") + column_types[c_idx] + " not recognized.");
            ASSERT_MSG(false, msg.c_str());
        }
      }

      if(generate_target) {
        row[target_column] = target_value;
        target_min_v[thread_idx] = std::min(target_min_v[thread_idx], target_value);
        target_max_v[thread_idx] = std::max(target_max_v[thread_idx], target_value);
      }

      *it_out = row;
    }
    });

  data.close();

  gl_sframe ret_sf(data);

  if(generate_target) {
    flex_int target_min = *std::min_element(target_min_v.begin(), target_min_v.end());
    flex_int target_max = *std::max_element(target_max_v.begin(), target_max_v.end());

    // To prevent divide by zeros
    target_max = std::max(target_min + 1, target_max);

    long noise_level_int = long(std::ceil( (n_target_precision) * noise_level));

    // Add in the noise and map things to [0,1]
    ret_sf["target"] = ret_sf["target"].apply(
        [=](const flexible_type& x) -> flexible_type {
          flex_int v = x.get<flex_int>();

          DASSERT_LE(v, target_max);
          DASSERT_GE(v, target_min);

          if(noise_level_int != 0) {
            flex_int ub = std::min(target_max, v + noise_level_int);
            flex_int lb = std::max(target_min, v - noise_level_int);
            v = (hash64(v, random_seed) % (ub - lb + 1) + lb);
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
