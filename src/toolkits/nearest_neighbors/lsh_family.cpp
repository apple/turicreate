/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/nearest_neighbors/lsh_family.hpp>
#include <core/random/random.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>

#include <time.h>
#include <boost/functional/hash.hpp>

namespace turi {
namespace nearest_neighbors {


void lsh_family::init_options(const std::map<std::string, flexible_type>& _opts) {

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(_opts.at(#var));
  __EXTRACT(num_tables);
  __EXTRACT(num_projections_per_table);
#undef __EXTRACT
  num_projections = num_tables * num_projections_per_table;
  num_input_dimensions = 0;

  lookup_table.assign(num_tables, hash_map_container<size_t, std::vector<size_t>>());
}

void lsh_family::save(turi::oarchive& oarc) const {
  oarc << num_input_dimensions
      << num_tables
      << num_projections_per_table
      << num_projections
      << lookup_table;
}

void lsh_family::load(turi::iarchive& iarc) {
  iarc >> num_input_dimensions
      >> num_tables
      >> num_projections_per_table
      >> num_projections
      >> lookup_table;
}

std::vector<int> lsh_family::hash_vector_to_codes(const DenseVector& vec,
                                                  bool is_reference_data) const {
  log_and_throw(std::string("DenseVector is not supported for LSH ")
                + distance_type_name());
  return {};
}

std::vector<int> lsh_family::hash_vector_to_codes(const SparseVector& vec,
                                                  bool is_reference_data) const {
  log_and_throw(std::string("SparseVector is not supported for LSH ")
                + distance_type_name());
  return {};
}

// Random sample a subset, and set w using the average distance
void lsh_euclidean::pre_lsh(const v2::ml_data& mld_ref, bool is_sparse) {
  size_t num_samples = std::min(size_t(100), mld_ref.size());
  v2::ml_data sampled_data = mld_ref.create_subsampled_copy(num_samples, time(NULL));
  DenseMatrix distance_matrix(num_samples, num_samples);

  if (!is_sparse) { // dense
    DenseMatrix sub_matrix(num_samples, num_input_dimensions);
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_eigen_row(sub_matrix.row(it.row_index()));
      }
    });

    all_pairs_squared_euclidean(sub_matrix, sub_matrix, distance_matrix);
    distance_matrix = distance_matrix.unaryExpr([](double x) { return std::sqrt(x); });
  } else { // sparse
    std::vector<SparseVector> sub_matrix(num_samples, SparseVector(num_input_dimensions));
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_observation(sub_matrix[it.row_index()]);
      }
    });

    std::vector<double> vec_norms(num_samples, 0.);
    parallel_for (0, num_samples, [&](size_t idx){
      vec_norms[idx] = sub_matrix[idx].squaredNorm();
    });
    parallel_for (0, num_samples, [&](size_t idx_a){
      for (size_t idx_b = 0; idx_b < num_samples; ++idx_b) {
        distance_matrix(idx_a, idx_b) = std::sqrt(vec_norms[idx_a]
          + vec_norms[idx_b] - 2. * sub_matrix[idx_a].dot(sub_matrix[idx_b]));
      }
    });
  }
  w = std::max(size_t(1), static_cast<size_t>(distance_matrix.mean()));
  rand_vec = rand_vec.unaryExpr([&](double x) { return random::fast_uniform<double>(0., w); });
}

void lsh_euclidean::init_model(size_t num_dimensions) {
  num_input_dimensions = num_dimensions;
  w = 4;
  rand_mat.resize(num_projections, num_input_dimensions);
  rand_vec.resize(num_projections);
  rand_mat = rand_mat.unaryExpr([](double x) { return random::normal(0., 1.); });
  rand_vec = rand_vec.unaryExpr([&](double x) { return random::fast_uniform<double>(0., w); });
}

void lsh_euclidean::save(turi::oarchive& oarc) const {
  lsh_family::save(oarc);
  oarc << w << rand_mat << rand_vec;
}


void lsh_euclidean::load(turi::iarchive& iarc) {
  lsh_family::load(iarc);
  iarc >> w >> rand_mat >> rand_vec;
}

std::vector<int> lsh_euclidean::hash_vector_to_codes(const DenseVector& vec,
                                                     bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);
  DenseVector hash_vec = rand_mat * vec + rand_vec;
  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = static_cast<int>(std::floor(hash_vec(hash_idx) / w));
  });
  return ret;
}

std::vector<int> lsh_euclidean::hash_vector_to_codes(const SparseVector& vec,
                                                     bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);
  if (vec.nonZeros() == 0) return ret;

  DenseVector hash_vec = rand_mat * vec + rand_vec;
  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = static_cast<int>(std::floor(hash_vec(hash_idx) / w));
  });
  return ret;
}


// Random sample a subset, and set w using the average distance
void lsh_manhattan::pre_lsh(const v2::ml_data& mld_ref, bool is_sparse) {
  size_t num_samples = std::min(size_t(100), mld_ref.size());
  v2::ml_data sampled_data = mld_ref.create_subsampled_copy(num_samples, time(NULL));
  DenseMatrix distance_matrix(num_samples, num_samples);

  if (!is_sparse) { // dense
    DenseMatrix sub_matrix(num_samples, num_input_dimensions);
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_eigen_row(sub_matrix.row(it.row_index()));
      }
    });
    parallel_for (0, num_samples, [&](size_t idx_a){
      for (size_t idx_b = 0; idx_b < num_samples; ++idx_b) {
        distance_matrix(idx_a, idx_b) = (sub_matrix.row(idx_a) - sub_matrix.row(idx_b)).cwiseAbs().sum();
      }
    });
  } else { // sparse
    std::vector<SparseVector> sub_matrix(num_samples, SparseVector(num_input_dimensions));
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      for (auto it = sampled_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_observation(sub_matrix[it.row_index()]);
      }
    });

    parallel_for (0, num_samples, [&](size_t idx_a){
      for (size_t idx_b = 0; idx_b < num_samples; ++idx_b) {
        distance_matrix(idx_a, idx_b) = (sub_matrix[idx_a] - sub_matrix[idx_b]).cwiseAbs().sum();
      }
    });
  }

  w = std::max(size_t(1), static_cast<size_t>(distance_matrix.mean()));
  rand_vec = rand_vec.unaryExpr([&](double x) { return random::fast_uniform<double>(0., w); });
}

void lsh_manhattan::init_model(size_t num_dimensions) {
  num_input_dimensions = num_dimensions;
  w = 4;
  rand_mat.resize(num_projections, num_input_dimensions);
  rand_vec.resize(num_projections);
  rand_mat = rand_mat.unaryExpr([](double x) { return random::cauchy(0., 1.); });
  rand_vec = rand_vec.unaryExpr([&](double x) { return random::fast_uniform<double>(0., w); });
}

void lsh_cosine::init_model(size_t num_dimensions) {
  num_input_dimensions = num_dimensions;
  rand_mat.resize(num_projections, num_input_dimensions);
  rand_mat = rand_mat.unaryExpr([](double x) { return random::normal(0., 1.); });
}

void lsh_cosine::save(turi::oarchive& oarc) const {
  lsh_family::save(oarc);
  oarc << rand_mat;
}

void lsh_cosine::load(turi::iarchive& iarc) {
  lsh_family::load(iarc);
  iarc >> rand_mat;
}

std::vector<int> lsh_cosine::hash_vector_to_codes(const DenseVector& vec,
                                                  bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);
  DenseVector hash_vec = rand_mat * vec;
  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = (hash_vec(hash_idx) > 0.) ? 1 : 0;
  });
  return ret;
}

std::vector<int> lsh_cosine::hash_vector_to_codes(const SparseVector& vec,
                                                  bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);

  if (vec.nonZeros() == 0) return ret;

  DenseVector hash_vec = rand_mat * vec;
  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = (hash_vec(hash_idx) > 0.) ? 1 : 0;
  });
  return ret;
}

void lsh_jaccard::init_model(size_t num_dimensions) {
  num_input_dimensions = num_dimensions;
  if (num_input_dimensions < num_projections) {
    log_and_throw("When number of input dimensions is smaller than \
                  num_tables * num_projections_per_table, LSH-Jaccard is not recommended.");
  }
  rand_permutation.assign(num_input_dimensions, 0);
  rand_sign.assign(num_input_dimensions, 0);
  parallel_for (0, num_input_dimensions, [&](size_t idx) {
    rand_permutation[idx] = idx;
    if (random::fast_uniform<double>(0., 1.) > 0.5) {
      rand_sign[idx] = 1;
    }
  });
  random::shuffle(rand_permutation);
}

void lsh_jaccard::save(turi::oarchive& oarc) const {
  lsh_family::save(oarc);
  oarc << rand_permutation
      << rand_sign;
}


void lsh_jaccard::load(turi::iarchive& iarc) {
  lsh_family::load(iarc);
  iarc >> rand_permutation
      >> rand_sign;
}

// The procedure is similar with hash_vector_to_codes(SparseVector)
// See details in that function
std::vector<int> lsh_jaccard::hash_vector_to_codes(const DenseVector& vec,
                                                   bool is_reference_data) const {
  std::vector<int> ret(num_projections, num_input_dimensions);

  // Note that the size of the last chunk might be larger than chunk_size
  size_t chunk_size = num_input_dimensions / num_projections;
  size_t chunk_idx, chunk_offset;
  size_t permuted_idx;
  size_t cnt = 0;
  for (size_t idx = 0; idx < size_t(vec.size()); ++idx) {
    if (vec(idx) < 1e-8) {
      // skip
    } else {
      permuted_idx = rand_permutation[idx];
      chunk_idx = std::min(permuted_idx / chunk_size, num_projections - 1);
      chunk_offset = permuted_idx - chunk_idx * chunk_size;
      DASSERT_LT(chunk_idx, num_projections);
      ret[chunk_idx] = std::min(int(chunk_offset), ret[chunk_idx]);
      ++cnt;
    }
  }
  if (cnt > 0) {
    fill_empty_bins(ret);
  }
  return ret;
}

std::vector<int> lsh_jaccard::hash_vector_to_codes(const SparseVector& vec,
                                                   bool is_reference_data) const {
  // Details in
  // http://jmlr.org/proceedings/papers/v32/shrivastava14.pdf
  // Figure 4
  // and
  // http://www.auai.org/uai2014/proceedings/individuals/225.pdf
  // Figure 5

  // Initialize the hash code. All the values are set to D (num_input_dimensions).
  std::vector<int> ret(num_projections, num_input_dimensions);

  if (vec.nonZeros() == 0) return ret;

  // chunk_size = D/K, where K is num_projections
  // Note that the size of the last chunk might be larger than chunk_size,
  // when D % K != 0
  size_t chunk_size = num_input_dimensions / num_projections;
  size_t chunk_idx, chunk_offset;
  size_t permuted_idx;
  SparseVector::InnerIterator it(vec, 0);
  size_t idx;
  while (it) {
    idx = it.index();
    permuted_idx = rand_permutation[idx];
    // which chunk it is in
    chunk_idx = std::min(permuted_idx / chunk_size, num_projections - 1);
    // the offset inside the chunk
    chunk_offset = permuted_idx - chunk_idx * chunk_size;
    // update
    DASSERT_LT(chunk_idx, num_projections);
    ret[chunk_idx] = std::min(int(chunk_offset), ret[chunk_idx]);
    ++it;
  }

  // there are still some empty bins left
  fill_empty_bins(ret);
  return ret;
}

// this function is used to fill the empty bins in the hash codes
void lsh_jaccard::fill_empty_bins(std::vector<int>& vec) const {
  size_t chunk_size = num_input_dimensions / num_projections;

  int64_t num_input_dimensions_i64 = truncate_check<int64_t>(num_input_dimensions);
  int64_t num_projections_i64 = truncate_check<int64_t>(num_projections);
  int64_t chunk_size_i64 = truncate_check<int64_t>(chunk_size);

  // find the first non-empty bin for left rotating
  int start_idx_left = 0;
  // find the first non-empty bin for right rotating
  int start_idx_right = num_projections - 1;

  while (vec[start_idx_left] == num_input_dimensions_i64 // if it is not updated since initialized
         && start_idx_left < num_projections_i64) {
    ++start_idx_left;
  }

  while (vec[start_idx_right] == num_input_dimensions_i64 // if it is not updated since initialized
         && start_idx_right >= 0) {
    --start_idx_right;
  }

  // no empty bins
  if (start_idx_left == num_projections_i64 || start_idx_right == int(-1)) {
    return;
  }

  // OK. We get a non-empty bin, go back to update empty bins
  int current_offset = vec[start_idx_left];
  int num_straight_empty_bins = 0;

  for (int idx = start_idx_left + num_projections; idx != start_idx_left; --idx) {
    if (vec[idx % num_projections] >= 2 * chunk_size_i64) { // empty
      num_straight_empty_bins += 1;
      // h_j = h_{j+t} + t * C, where t is distance to the nearest non-empty bin
      // C is a constant >= chunk_size, here we take C = chunk_size * 2
      if (rand_sign[idx % num_projections] == 1) {
        vec[idx % num_projections] = current_offset + num_straight_empty_bins * chunk_size * 2;
      }
    } else { // else, update the cached offset
      current_offset = vec[idx % num_projections];
      num_straight_empty_bins = 0;
    }
  }

  current_offset = vec[start_idx_right];
  num_straight_empty_bins = 0;

  for (int idx = start_idx_right; idx != start_idx_right + num_projections_i64; ++idx) {
    if (vec[idx % num_projections] >= 2 * chunk_size_i64) {
      num_straight_empty_bins += 1;
      // h_j = h_{j+t} + t * C, where t is distance to the nearest non-empty bin
      // C is a constant >= chunk_size, here we take C = chunk_size * 2
      if (rand_sign[idx % num_projections] == 0) {
        vec[idx % num_projections] = current_offset + num_straight_empty_bins * chunk_size * 2;
      }
    } else {
      current_offset = vec[idx % num_projections];
      num_straight_empty_bins = 0;
    }
  }
}


void lsh_dot_product::init_model(size_t num_dimensions) {
  num_input_dimensions = num_dimensions;
  max_vec_norm = 0.;

  // one extra dimension for the asymmetric vector transformation
  rand_mat.resize(num_projections, num_input_dimensions);
  rand_mat = rand_mat.unaryExpr([](double x) { return random::normal(0., 1.); });
  rand_vec.resize(num_projections);
  rand_vec = rand_vec.unaryExpr([](double x) { return random::normal(0., 1.); });
}

// get the max of the norm of the reference data
void lsh_dot_product::pre_lsh(const v2::ml_data& mld_ref, bool is_sparse) {
  size_t max_num_threads = thread::cpu_count();
  std::vector<double> local_max_vec_norms(max_num_threads, 0.);
  if (!is_sparse) {
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {
      DenseVector v(num_input_dimensions);
      for (auto it = mld_ref.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_observation(v);
        local_max_vec_norms[thread_idx] = std::max(local_max_vec_norms[thread_idx], v.norm());
      }
    });
  } else {
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {
      SparseVector s(num_input_dimensions);
      for (auto it = mld_ref.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        it.fill_observation(s);
        if (s.nonZeros() > 0) {
          local_max_vec_norms[thread_idx] = std::max(local_max_vec_norms[thread_idx], s.norm());
        }
      }
    });
  }
  max_vec_norm = * std::max_element(local_max_vec_norms.begin(), local_max_vec_norms.end());
}

void lsh_dot_product::save(turi::oarchive& oarc) const {
  lsh_family::save(oarc);
  oarc << max_vec_norm << rand_mat << rand_vec;
}

void lsh_dot_product::load(turi::iarchive& iarc) {
  lsh_family::load(iarc);
  iarc >> max_vec_norm >> rand_mat >> rand_vec;
}

// Implementation of section 4.2 of http://jmlr.org/proceedings/papers/v37/neyshabur15.pdf
std::vector<int> lsh_dot_product::hash_vector_to_codes(const DenseVector& vec,
                                                       bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);
  DenseVector hash_vec(num_projections);

  if (is_reference_data) {
    hash_vec = rand_mat * vec / max_vec_norm +
        rand_vec * std::sqrt(1. - vec.squaredNorm() / (max_vec_norm * max_vec_norm));
  } else {
    // query vecs are normalized
    double vec_norm = vec.norm();
    if (vec_norm > 1e-16) {
      hash_vec = rand_mat * vec / vec_norm;
    }
  }

  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = (hash_vec(hash_idx) > 0.) ? 1 : 0;
  });
  return ret;
}

// Implementation of section 4.2 of http://jmlr.org/proceedings/papers/v37/neyshabur15.pdf
std::vector<int> lsh_dot_product::hash_vector_to_codes(const SparseVector& vec,
                                                       bool is_reference_data) const {
  std::vector<int> ret(num_projections, -1);

  if (vec.nonZeros() == 0) return ret;

  DenseVector hash_vec(num_projections);

  if (is_reference_data) {
    hash_vec = rand_mat * vec / max_vec_norm +
        rand_vec * std::sqrt(1. - vec.squaredNorm() / (max_vec_norm * max_vec_norm));
  } else {
    // query vecs are normalized
    double vec_norm = vec.norm();
    if (vec_norm > 1e-16) {
      hash_vec = rand_mat * vec / vec_norm;
    }
  }
  parallel_for (0, num_projections, [&](size_t hash_idx) {
    ret[hash_idx] = (hash_vec(hash_idx) > 0.) ? 1 : 0;
  });
  return ret;
}


} // namespace nearest_neighbors
} // namespace turi
