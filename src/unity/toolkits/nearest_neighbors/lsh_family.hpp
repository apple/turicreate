/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LSH_FAMILY_H_
#define TURI_LSH_FAMILY_H_

#include <memory>
#include <unity/toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/hash_map_container.hpp>

namespace turi {
namespace nearest_neighbors {


class EXPORT lsh_family {
 public:

  lsh_family() = default;
  virtual ~lsh_family() {}
  
  // create a lsh_family pointer using dist_name ("euclidean", "cosine", ect.)
  inline static std::shared_ptr<lsh_family> create_lsh_family(const std::string& dist_name);
  
  // indicator function: whether it is an asymmetric LSH or not
  virtual bool is_asymmetric() const = 0; 

  // distance type name
  virtual std::string distance_type_name() const = 0;
  
  // initialize options
  virtual void init_options(const std::map<std::string, flexible_type>& _opts);
  
  // One pass over the data to get some information about the data
  // For example, for lsh_dot_product, we need to know the max norm of all the
  // reference data.
  virtual void pre_lsh(const v2::ml_data& mld_ref, bool is_sparse) {} 
  
  // initialize the model. num_input_dimensions is needed
  virtual void init_model(size_t num_dimensions) = 0;

  // add reference data one by one
  // Only DenseVector and SparseVector are supported
  template <typename T>
  void add_reference_data(size_t ref_id, const T& t);
  
  // Return a set of candidates for the query vector 
  // Only DenseVector and SparseVector are supported
  template <typename T>
  std::vector<size_t> query(const T& t) const;

  // save & load
  virtual void save(turi::oarchive& oarc) const;
  virtual void load(turi::iarchive& iarc);

 protected:
  // model initialized when add_reference_data is called in the first time
  virtual std::vector<int> hash_vector_to_codes(const DenseVector& vec, 
                                                bool is_reference_data) const;
  virtual std::vector<int> hash_vector_to_codes(const SparseVector& vec, 
                                                bool is_reference_data) const;

 protected:
  size_t num_input_dimensions;
  size_t num_tables;
  size_t num_projections_per_table;
  size_t num_projections;
  std::vector<hash_map_container<size_t, std::vector<size_t>>> lookup_table;
};


/**
 * LSH for Euclidean distance
 */
class EXPORT lsh_euclidean : public lsh_family {
 public:

  virtual bool is_asymmetric() const { return false; } 

  virtual std::string distance_type_name() const {
    return "euclidean";
  }
  
  // Sample a subset of data points, get the average euclidean distance to
  // initilize w
  virtual void pre_lsh(const v2::ml_data& mld_ref, bool is_sparse);

  virtual void init_model(size_t num_dimensions);

  virtual void save(turi::oarchive& oarc) const;
  virtual void load(turi::iarchive& iarc);

 protected:
  virtual std::vector<int> hash_vector_to_codes(const DenseVector& vec, 
                                        bool is_reference_data) const;
  virtual std::vector<int> hash_vector_to_codes(const SparseVector& vec,
                                        bool is_reference_data) const;

 protected:
  size_t w;
  DenseMatrix rand_mat;
  DenseVector rand_vec;
};

/**
 * LSH for squared_euclidean distance
 * The only difference from euclidean happens when calculating the real
 * distances 
 */
class EXPORT lsh_squared_euclidean final : public lsh_euclidean {
 public:
  std::string distance_type_name() const {
    return "squared_euclidean";
  }
};

/**
 * LSH for manhattan distance
 * The only difference from euclidean is initialization
 */
class EXPORT lsh_manhattan final : public lsh_euclidean {
 public:

  std::string distance_type_name() const {
    return "manhattan";
  }
  
  // Sample a subset of data points, get the average manhattan distance to
  // initilize w
  void pre_lsh(const v2::ml_data& mld_ref, bool is_sparse);

  void init_model(size_t num_dimensions);
};

/**
 *  LSH for Cosine distance
 */
class EXPORT lsh_cosine final : public lsh_family {
 public:

  bool is_asymmetric() const { return false; }

  std::string distance_type_name() const {
    return "cosine";
  }
  
  void init_model(size_t num_dimensions);

  void save(turi::oarchive& oarc) const;
  void load(turi::iarchive& iarc);

 protected:
  std::vector<int> hash_vector_to_codes(const DenseVector& vec,
                                        bool is_reference_data) const;
  std::vector<int> hash_vector_to_codes(const SparseVector& vec,
                                        bool is_reference_data) const;

 private:
  DenseMatrix rand_mat;
};

/**
 * LSH for Jaccard similarity
 */
class EXPORT lsh_jaccard final : public lsh_family {
 public:

  bool is_asymmetric() const { return false; }

  std::string distance_type_name() const {
    return "jaccard";
  }
  
  void init_model(size_t num_dimensions);

  void save(turi::oarchive& oarc) const;
  void load(turi::iarchive& iarc);

 protected:
  std::vector<int> hash_vector_to_codes(const DenseVector& vec,
                                        bool is_reference_data) const;
  std::vector<int> hash_vector_to_codes(const SparseVector& vec,
                                        bool is_reference_data) const;
  
  // helper function
  void fill_empty_bins(std::vector<int>& vec) const;

 private:
  // This implements the one permutation MinHash
  std::vector<size_t> rand_permutation;
  std::vector<size_t> rand_sign;
};

/**
 * LSH for dot product 
 */
class EXPORT lsh_dot_product : public lsh_family {
 public:

  bool is_asymmetric() const { return true; }

  virtual std::string distance_type_name() const {
    return "dot_product";
  }
  
  void init_model(size_t num_dimensions);
  
  void pre_lsh(const v2::ml_data& mld_ref, bool is_sparse);

  void save(turi::oarchive& oarc) const;
  void load(turi::iarchive& iarc);

 protected:
  std::vector<int> hash_vector_to_codes(const DenseVector& vec,
                                        bool is_reference_data) const;
  std::vector<int> hash_vector_to_codes(const SparseVector& vec,
                                        bool is_reference_data) const;
  
 private:
  double max_vec_norm;
  DenseMatrix rand_mat;
  DenseVector rand_vec; // rand_vec for the extra column
};

class EXPORT lsh_transformed_dot_product : public lsh_dot_product {
  std::string distance_type_name() const {
    return "transformed_dot_product";
  }
};

std::shared_ptr<lsh_family> lsh_family::create_lsh_family(const std::string& dist_name) {
  if (dist_name == "euclidean") {
    return std::shared_ptr<lsh_family>(new lsh_euclidean);
  } else if (dist_name == "squared_euclidean") {
    return std::shared_ptr<lsh_family>(new lsh_squared_euclidean);
  } else if (dist_name == "manhattan") {
    return std::shared_ptr<lsh_family>(new lsh_manhattan);
  } else if (dist_name == "cosine") {
    return std::shared_ptr<lsh_family>(new lsh_cosine);
  } else if (dist_name == "jaccard") {
    return std::shared_ptr<lsh_family>(new lsh_jaccard);
  } else if (dist_name == "dot_product") {
    return std::shared_ptr<lsh_family>(new lsh_dot_product);
  } else if (dist_name == "transformed_dot_product") {
    return std::shared_ptr<lsh_family>(new lsh_transformed_dot_product);
  } else {
    log_and_throw(dist_name + std::string(" is not supported by LSH! Try another distance or method!"));
    return nullptr; 
  }
}

template <typename T>
void lsh_family::add_reference_data(size_t ref_id, const T& vec) {

  ASSERT_MSG(size_t(vec.size()) == num_input_dimensions, 
             "The input dimension does not match the previous ones!");

  auto hash_vec = hash_vector_to_codes(vec, true);
  DASSERT_TRUE(hash_vec.size() == num_projections);
  
  parallel_for (0, num_tables, [&](size_t table_idx) {
    auto hash_bucket_id = boost::hash_range(
        hash_vec.begin() + table_idx * num_projections_per_table,
        hash_vec.begin() + std::min((table_idx + 1) * num_projections_per_table, num_projections));

    lookup_table[table_idx].update(hash_bucket_id, [ref_id](std::vector<size_t>& v){
                                    v.push_back(ref_id); 
                                  });
  });
}

template <typename T>
std::vector<size_t> lsh_family::query(const T& vec) const {

  ASSERT_MSG(size_t(vec.size()) == num_input_dimensions,
             "The input num_dimensions does not match the reference data!");

  std::unordered_set<size_t> ret;
  auto hash_vec = hash_vector_to_codes(vec, false);
  DASSERT_TRUE(hash_vec.size() == num_projections);
  for (size_t table_idx = 0; table_idx < num_tables; ++table_idx) {
    auto hash_bucket_id = boost::hash_range(
        hash_vec.begin() + table_idx * num_projections_per_table,
        hash_vec.begin() + std::min((table_idx + 1) * num_projections_per_table, num_projections));

    const auto& candidates = lookup_table[table_idx].get(hash_bucket_id);
    ret.insert(candidates.cbegin(), candidates.cend());
  }

  std::vector<size_t> ret_vec(ret.begin(), ret.end());
  return ret_vec;
}


} // namespace nearest_neighbors
} // namespace turi

#endif
