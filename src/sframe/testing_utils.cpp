/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sframe/testing_utils.hpp>

namespace turi {

sframe make_testing_sframe(const std::vector<std::string>& names,
                           const std::vector<flex_type_enum>& types,
                           const std::vector<std::vector<flexible_type> >& data) {

  sframe out;

  out.open_for_write(names, types);

  size_t num_segments = out.num_segments();

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    auto it_out = out.get_output_iterator(sidx);

    size_t start_idx = (sidx * data.size()) / num_segments;
    size_t end_idx   = ((sidx+1) * data.size()) / num_segments;

    for(size_t i = start_idx; i < end_idx; ++i, ++it_out)
      *it_out = data[i];
  }

  out.close();

  return out;
}

sframe make_testing_sframe(const std::vector<std::string>& names,
                           const std::vector<std::vector<flexible_type> >& data) {

  std::vector<flex_type_enum> types(names.size());

  DASSERT_FALSE(data.empty());
  DASSERT_EQ(data.front().size(), names.size());

  for(size_t i = 0; i < names.size(); ++i) {
    types[i] = data.front()[i].get_type();
  }

  return make_testing_sframe(names, types, data);
}


sframe make_integer_testing_sframe(const std::vector<std::string>& names,
                                   const std::vector<std::vector<size_t> >& data) {

  sframe out;

  std::vector<flex_type_enum> types(names.size(), flex_type_enum::INTEGER);


  out.open_for_write(names, types);

  size_t num_segments = out.num_segments();

  std::vector<flexible_type> x;

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    auto it_out = out.get_output_iterator(sidx);

    size_t start_idx = (sidx * data.size()) / num_segments;
    size_t end_idx   = ((sidx+1) * data.size()) / num_segments;


    for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
      x.assign(data[i].begin(), data[i].end());
      *it_out = x;
    }
  }

  out.close();

  return out;
}

std::shared_ptr<sarray<flexible_type> >
make_testing_sarray(flex_type_enum type,
                    const std::vector<flexible_type>& data) {

  std::shared_ptr<sarray<flexible_type> > new_x(new sarray<flexible_type>);

  new_x->open_for_write();
  new_x->set_type(type);

  size_t num_segments = new_x->num_segments();

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    auto it_out = new_x->get_output_iterator(sidx);

    size_t start_idx = (sidx * data.size()) / num_segments;
    size_t end_idx   = ((sidx+1) * data.size()) / num_segments;

    for(size_t i = start_idx; i < end_idx; ++i, ++it_out)
      *it_out = data[i];
  }

  new_x->close();

  return new_x;
}

std::vector<std::vector<flexible_type> > testing_extract_sframe_data(const sframe& sf) {

  std::vector<std::vector<flexible_type> > ret;

  auto reader = sf.get_reader();

  size_t num_segments = sf.num_segments();

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {
    auto src_it     = reader->begin(sidx);
    auto src_it_end = reader->end(sidx);

    for(; src_it != src_it_end; ++src_it)
      ret.push_back(*src_it);
  }

  return ret;
}

/**  Creates a random SFrame for testing purposes.  The
 *  column_types gives the types of the column.
 *
 *  \param[in] n_rows The number of observations to run the timing on.
 *  \param[in] column_types A string with each character denoting
 *  one type of column.  The legend is as follows:
 *
 *     n:  numeric column.
 *     b:  categorical column with 2 categories.
 *     z:  categorical column with 5 categories.
 *     Z:  categorical column with 10 categories.
 *     c:  categorical column with 100 categories.
 *     C:  categorical column with 1000000 categories.
 *     s:  categorical column with short string keys and 1000 categories.
 *     S:  categorical column with short string keys and 100000 categories.
 *     v:  numeric vector with 10 elements.
 *     V:  numeric vector with 1000 elements.
 *     u:  categorical set with up to 10 elements.
 *     U:  categorical set with up to 1000 elements.
 *     d:  dictionary with 10 entries.
 *     D:  dictionary with 100 entries.
 *     1:  1d ndarray of dimension 10
 *     2:  2d ndarray of dimension 4x3
 *     3:  3d ndarray of dimension 4x3x2
 *     4:  4d ndarray of dimension 4x3x2x2
 *     A:  3d ndarray of dimension 4x3x2, randomized non-canonical striding.
 *
 */
sframe make_random_sframe(
    size_t n_rows, std::string column_types,
    bool generate_target, size_t _random_seed) {

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

      case '1':
      case '2':
      case '3':
      case '4':
      case 'A':
        types[c_idx] = flex_type_enum::ND_VECTOR;
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

  // For generating a target that can mostly be learned
  static const size_t n_bins = 16;
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
          double v = lb + (ub - lb) * v01;

          // Now, round it to the nearest 2**12 in order to accomodate possible float32 vs float64 issues.
          double C = double(1 << 12);
          return std::round(v * C) / C;
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

        auto rng_nd_vec = [&](const flex_nd_vec::index_range_type& shape,
                              const flex_nd_vec::index_range_type& stride) {
          flex_nd_vec v(shape, stride, 0.0);

          size_t n = v.num_elem();
          for (size_t vidx = 0; vidx < n; ++vidx) {
            v[vidx] = rng_dbl(0, 1);
          }
          return v;
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

          case '1': { row[c_idx] = rng_nd_vec({10}, {});       break; }
          case '2': { row[c_idx] = rng_nd_vec({4,3}, {});      break; }
          case '3': { row[c_idx] = rng_nd_vec({4,3,2}, {});    break; }
          case '4': { row[c_idx] = rng_nd_vec({4,3,2,2}, {});  break; }
          case 'A': {
            flex_nd_vec::index_range_type shape = {2,3,4};
            flex_nd_vec::index_range_type stride(3);

            flex_nd_vec::index_range_type _order = {0, 1, 2};
            stride.resize(3, 0);
            size_t cur_stride = 1;
            while (!_order.empty()) {
              int pick_index =
                  rng_int(0, _order.size() - 1);
              int index = _order[pick_index];
              _order.erase(_order.begin() + pick_index);

              stride[index] = cur_stride;
              cur_stride *= shape[index];
            }

            row[c_idx] = rng_nd_vec(shape, stride);

            break;
          }

          default:
            std::string msg = (std::string("Column type ") + column_types[c_idx] + " not recognized.");
            ASSERT_MSG(false, msg.c_str());
        }
      }

      if(generate_target) {
        row[target_column] = target_value;
      }

      *it_out = row;
    }
    });

  data.close();

  return data;
}

sframe slice_sframe(const sframe& src, size_t row_lb, size_t row_ub) {
  
  ASSERT_LE(row_lb, row_ub); 
  ASSERT_LE(row_ub, src.num_rows()); 
  
  sframe out;

  out.open_for_write(src.column_names(), src.column_types(), "", thread::cpu_count());
  auto reader = src.get_reader();

  // Really inefficient due to the transpose. 
  in_parallel([&](size_t thread_index, size_t num_threads) {
      std::vector<std::vector<flexible_type> > out_values(1); 

      size_t num_rows = (row_ub - row_lb); 
      size_t start_idx = row_lb + (num_rows * thread_index) / num_threads;
      size_t end_idx = row_lb + (num_rows * (thread_index+1)) / num_threads;
      auto it_out = out.get_output_iterator(thread_index);
      for(size_t i = start_idx; i < end_idx; ++i, ++it_out) { 
        reader->read_rows(i, i+1, out_values);
        *it_out = out_values.front();
      }
    });

  out.close();

  return out; 
}


}
