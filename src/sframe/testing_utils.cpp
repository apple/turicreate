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

// The number of categories and the sizes to use for each of the
// modes below.
static const size_t n_categorical_few = 10;        // 'c'
static const size_t n_categorical_many = 5000;     // 'C'
static const size_t vector_size_small = 10;        // 'v'
static const size_t vector_size_large = 100;       // 'V'
static const size_t dict_size_small = 5;           // 'd'
static const size_t dict_size_large = 100;         // 'D'

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
 *
 */
sframe make_random_sframe(
    size_t n_rows, std::string column_types,
    bool create_target_column) {

  sframe data;

  size_t num_columns = column_types.size();
  size_t n_threads = thread::cpu_count();

  std::vector<std::string> names;
  std::vector<flex_type_enum> types;
  std::vector<bool> is_categorical;

  names.resize(column_types.size());
  types.resize(column_types.size());
  is_categorical.resize(column_types.size());

  ////////////////////////////////////////////////////////////////////////////////
  //  Set up the information lookups for each of the columns: type,
  //  whether it's categorical, and the description to print.
  //
  for(size_t cid = 0; cid < num_columns; cid++){

    names[cid] = std::string("C-") + std::to_string(cid + 1) +  column_types[cid];

    switch(column_types[cid]) {
      case 'n':
        types[cid] = flex_type_enum::FLOAT;
        is_categorical[cid] = false;
        break;

      case 'b':
      case 'c':
      case 'C':
      case 'z':
      case 'Z':
        types[cid] = flex_type_enum::INTEGER;
        is_categorical[cid] = true;
        break;

      case 's':
      case 'S':
        types[cid] = flex_type_enum::STRING;
        is_categorical[cid] = true;
        break;

      case 'v':
      case 'V':
        types[cid] = flex_type_enum::VECTOR;
        is_categorical[cid] = false;
        break;

      case 'u':
      case 'U':
        types[cid] = flex_type_enum::LIST;
        is_categorical[cid] = true;
        break;

      case 'd':
      case 'D':
        types[cid] = flex_type_enum::DICT;
        is_categorical[cid] = true;
        break;

    }
  }

  if(create_target_column) {
    names.push_back("target");
    types.push_back(flex_type_enum::INTEGER);
    column_types += "C";
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Create the sframe with each of the columns as determined above.

  data.open_for_write(names, types, "", n_threads);

  std::string base_string = "TESTING STRING!!!  OH YAY!!!!";

  uint64_t random_seed = random::fast_uniform<size_t>(0, size_t(-1));
  
  in_parallel([&](size_t thread_idx, size_t num_segments) {

    auto it_out = data.get_output_iterator(thread_idx);

    std::vector<flexible_type> row(column_types.size());

    size_t start_idx = (thread_idx * n_rows) / num_segments;
    size_t end_idx = ((thread_idx + 1) * n_rows) / num_segments;

    for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {

      for(size_t c_idx = 0; c_idx < column_types.size(); ++c_idx) {

        auto rng_int = [&](size_t lb, size_t ub){
          return size_t(hash64(i, hash64(c_idx, random_seed)) % (ub - lb + 1)) + lb;
        };

        auto rng_int_seeded = [&](size_t lb, size_t ub, size_t seed){
          return size_t(hash64(i, hash64(c_idx, hash64(seed, random_seed))) % (ub - lb + 1)) + lb;
        };

        auto rng_dbl = [&](double lb, double ub){
          double v01 = double(hash64(i, hash64(c_idx, random_seed))) / std::numeric_limits<uint64_t>::max();
          return lb + (ub - lb) * v01;
        };
        
        auto rng_dbl_seeded = [&](double lb, double ub, size_t seed){
          double v01 = double(hash64(i, hash64(c_idx, hash64(seed, random_seed)))) / std::numeric_limits<uint64_t>::max();
          return lb + (ub - lb) * v01;
        };
        switch(column_types[c_idx]){

          case 'n':
            row[c_idx] = rng_dbl(0,1);
            break;

          case 'b':
            row[c_idx] = (rng_dbl(0,1) < 0.5);
            break;

          case 'z':
            row[c_idx] = rng_int(1, 5);
            break;

          case 'Z':
            row[c_idx] = rng_int(1, 10);
            break;

          case 'c':
            row[c_idx] = rng_int(0, n_categorical_few);
            break;

          case 'C':
            row[c_idx] = rng_int(0, n_categorical_many);
            break;

          case 's':
            row[c_idx] = std::to_string(rng_int(0, n_categorical_few));
            break;

          case 'S':
            row[c_idx] = base_string + std::to_string(rng_int(0, n_categorical_many));
            break;

          case 'v': {
            flex_vec v(vector_size_small);
            for (size_t vidx = 0;vidx < v.size(); ++vidx) {
              v[vidx] = rng_dbl_seeded(0,1, vidx);
            }
            row[c_idx] = v;
            break;
          }

          case 'V': {
            flex_vec v(vector_size_large);
            for (size_t vidx = 0;vidx < v.size(); ++vidx) {
              v[vidx] = rng_dbl_seeded(0,1, vidx);
            }
            row[c_idx] = v;
            break;
          }

          case 'u': {
            size_t s = rng_int(0,10);
            flex_list v(s);
            for (size_t vidx = 0;vidx < v.size(); ++vidx) {
              v[vidx] = rng_int_seeded(0,n_categorical_few, vidx);
            }

            std::sort(v.begin(), v.end());

            row[c_idx] = v;
            break;
          }

          case 'U': {
            size_t s = rng_int(0,1000);
            flex_list v(s);
            for (size_t vidx = 0;vidx < v.size(); ++vidx) {
              v[vidx] = rng_int_seeded(0,n_categorical_many, vidx);
            }

            std::sort(v.begin(), v.end());

            row[c_idx] = v;
            break;
          }

          case 'd': {
            std::unordered_map<flexible_type,flexible_type> m;

            for(size_t i = 0; i < dict_size_small; ++i) {
              flexible_type index = rng_int_seeded(0, 3*dict_size_small, i);
              flexible_type value = rng_int_seeded(1, 100, i);
              m[index] = value;
            }

            flex_dict d(m.begin(), m.end());

            row[c_idx] = d;
            break;
          }

          case 'D': {
            std::unordered_map<flexible_type,flexible_type> m;

            for(size_t i = 0; i < dict_size_large; ++i) {
              flexible_type index = rng_int_seeded(0, n_categorical_many, i);
              flexible_type value = rng_int_seeded(1, 1000, i);
              m[index] = value;
            }

            flex_dict d(m.begin(), m.end());

            row[c_idx] = d;
            break;
          }

          default:
            std::string msg = (std::string("Column type ") + column_types[c_idx]
                               + " not recognized; choose in ncCsSvVdD.");

            ASSERT_MSG(false, msg.c_str());
        }
      }

      *it_out = row;
    }
    });

  data.close();

  DASSERT_EQ(data.num_columns(), column_types.size());
  DASSERT_TRUE(data.column_types() == types);

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
