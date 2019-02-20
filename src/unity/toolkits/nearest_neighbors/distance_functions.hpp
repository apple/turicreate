/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_NN_DISTANCE_FUNCTIONS_H_
#define TURI_NN_DISTANCE_FUNCTIONS_H_

#include <string>
#include <Eigen/SparseCore>
#include <Eigen/Core>
#include <memory>
#include <util/logit_math.hpp>
#include <unity/toolkits/util/algorithmic_utils.hpp>
#include <flexible_type/flexible_type_base_types.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <boost/algorithm/string.hpp>

namespace turi {
namespace nearest_neighbors {

typedef Eigen::VectorXd DenseVector;
typedef Eigen::MatrixXd DenseMatrix;
typedef Eigen::SparseVector<double> SparseVector;
typedef Eigen::SparseMatrix<double, 1> SparseMatrix; // row-major

/**
 * Compute the Euclidean distance between the Cartesian product of rows in two
 * matrices.
 */
void inline all_pairs_squared_euclidean(const DenseMatrix& A,
                                        const DenseMatrix& B, 
                                        DenseMatrix& dists) {

  DASSERT_EQ(A.cols(), B.cols());
  DASSERT_EQ(A.rows(), dists.rows());
  DASSERT_EQ(B.rows(), dists.cols());

  dists = -2 * A * B.transpose();

  for (size_t i = 0; i < (size_t)A.rows(); ++i) {
    dists.row(i).array() += A.row(i).squaredNorm();
  }

  for (size_t j = 0; j < (size_t)B.rows(); ++j) {
    dists.col(j).array() += B.row(j).squaredNorm();
  }
}


void inline all_pairs_cosine(const DenseMatrix& A, const DenseMatrix& B, 
                             DenseMatrix& dists) {

  DASSERT_EQ(A.cols(), B.cols());
  DASSERT_EQ(A.rows(), dists.rows());
  DASSERT_EQ(B.rows(), dists.cols());

  dists = -1 * A * B.transpose();

  for (size_t i = 0; i < (size_t)A.rows(); ++i) {
    double row_norm = std::max(1e-16, A.row(i).norm());
    dists.row(i).array() /= row_norm;
  }

  for (size_t j = 0; j < (size_t)B.rows(); ++j) {
    double col_norm = std::max(1e-16, B.row(j).norm());
    dists.col(j).array() /= col_norm;
  }

  dists.array() += 1;
}


void inline all_pairs_dot_product(const DenseMatrix& A, const DenseMatrix& B, 
                                  DenseMatrix& dists) {

  DASSERT_EQ(A.cols(), B.cols());
  DASSERT_EQ(A.rows(), dists.rows());
  DASSERT_EQ(B.rows(), dists.cols());

  dists = A * B.transpose();
  dists = dists.cwiseMax(1e-10).cwiseInverse();
}


void inline all_pairs_transformed_dot_product(const DenseMatrix& A,
                                              const DenseMatrix& B, 
                                              DenseMatrix& dists) {

  DASSERT_EQ(A.cols(), B.cols());
  DASSERT_EQ(A.rows(), dists.rows());
  DASSERT_EQ(B.rows(), dists.cols());

  dists = A * B.transpose();
  dists = dists.unaryExpr([](double x) { return log1pen(x); });
}


struct distance_metric {

  virtual ~distance_metric() = default;

  // factory methods
  static inline std::shared_ptr<distance_metric> make_dist_instance(const std::string& dist_name);

  static inline std::shared_ptr<distance_metric> make_distance_metric(
    function_closure_info fn); 

  virtual double distance(const DenseVector& a, const DenseVector& b) const {
    ASSERT_MSG(false, "Dense vector type not supported by this distance metric.");
    ASSERT_UNREACHABLE();
  }

  virtual double distance(const SparseVector& a, const SparseVector& b) const {
    ASSERT_MSG(false, "Sparse vector type not supported by this distance metric.");
    ASSERT_UNREACHABLE();
  }

  virtual double distance(const std::string& a, const std::string& b) const {
    ASSERT_MSG(false, "String type not supported by this distance metric.");
    ASSERT_UNREACHABLE();
  }

  virtual double distance(const std::vector<double>& a, const std::vector<double>& b) const {
    ASSERT_MSG(false, "Vector of double type not supported by this distance metric.");
    ASSERT_UNREACHABLE();
  }

};

/** squared_euclidean distance.
 */
struct gaussian_kernel final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const  {
    return 1 - std::exp( - ( (a - b).squaredNorm() ) );
  }

  double distance(const SparseVector& a, const SparseVector& b) const  {
    return 1 - std::exp(-( (a.squaredNorm() + b.squaredNorm() - 2 * (a.dot(b))) ));
  }

};

/** squared_euclidean distance.
 */
struct squared_euclidean final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const  {
    return (a - b).squaredNorm();
  }

  double distance(const SparseVector& a, const SparseVector& b) const  {
    return (a.squaredNorm() + b.squaredNorm() - 2 * (a.dot(b)));
  }

};

/** euclidean distance.
 */
struct euclidean final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);
    return std::sqrt((a - b).squaredNorm());
  }

  double distance(const SparseVector& a, const SparseVector& b) const {
    return std::sqrt(a.squaredNorm() + b.squaredNorm() - 2 * (a.dot(b)));
  }

};

/** manhattan distance.
 */
struct manhattan final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);
    return (a - b).cwiseAbs().sum();
  }

  double distance(const SparseVector& a, const SparseVector& b) const {
    return (a - b).cwiseAbs().sum();
  }

};

/** cosine distance.
 */
struct cosine final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);

    double similarity = (double)a.dot(b) / std::max(1e-16, a.norm() * b.norm());
    return  1 - similarity;
  }

  double distance(const SparseVector& a, const SparseVector& b) const {
    double similarity = (double)a.dot(b) / std::max(1e-16, a.norm() * b.norm());
    return  1 - similarity;
  }

};

/* dot_product distance
 */
struct dot_product final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);

    double dot_product = (double)a.dot(b);
    return 1.0 / std::max(dot_product, 1e-10);
  }

  double distance(const SparseVector& a, const SparseVector& b) const {
    double dot_product = (double)a.dot(b);
    return 1.0 / std::max(dot_product, 1e-10);
  }
};

/* transformed_dot_product distance
 */
struct transformed_dot_product final : public distance_metric {

  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);

    double dot_product = (double)a.dot(b);
    return log1pen(dot_product);
  }

  double distance(const SparseVector& a, const SparseVector& b) const {
    double dot_product = (double)a.dot(b);
    return log1pen(dot_product);
  }
};

/* jaccard distance 
 */
struct jaccard final : public distance_metric {
  using distance_metric::distance;
  
  double distance(const DenseVector& a, const DenseVector& b) const {
    DASSERT_EQ(a.size(), b.size());
    double intersection_size = 0.;
    double union_size = 0.;    
    for(size_t idx = 0; idx < std::min<size_t>(a.size(), b.size()); ++idx) {
      if (a(idx) > 0. || b(idx) > 0.) {
        union_size += 1.;
        if (a(idx) > 0. && b(idx) > 0.) {
          intersection_size += 1.;
        }  
      }
    }
    if (union_size == 0.) return 1.;
    return 1. - intersection_size / union_size;
  }

  double distance(const SparseVector& a, const SparseVector& b) const GL_HOT_FLATTEN {

    size_t intersection_size = 0;

    SparseVector::InnerIterator it_a(a, 0);
    SparseVector::InnerIterator it_b(b, 0);

    while(it_a && it_b) {
      if(it_a.index() < it_b.index()) {
        ++it_a;
      } else if(it_a.index() > it_b.index()) {
        ++it_b;
      } else {
        ++intersection_size;
        ++it_a, ++it_b;
      }
    }

    size_t d = a.nonZeros() + b.nonZeros() - intersection_size;
    
    return 1.0 - double(intersection_size) / d;
  }

  double distance(std::vector<size_t>& av,
                  std::vector<size_t>& bv) const {
    DASSERT_TRUE(av.size() > 0);
    DASSERT_TRUE(bv.size() > 0);

    std::sort(av.begin(), av.end());
    std::sort(bv.begin(), bv.end());

    // Use an efficient accumulate function
    size_t n = count_intersection(av.begin(), av.end(),
                                  bv.begin(), bv.end());
    size_t d = av.size() + bv.size() - n;
    DASSERT_TRUE(d > 0);

    return 1 - double(n) / d;
  }

};

/* weighted jaccard distance 
 */
struct weighted_jaccard final : public distance_metric {

  double distance(const SparseVector& a, const SparseVector& b) const {

    SparseVector::InnerIterator it_a(a, 0);
    SparseVector::InnerIterator it_b(b, 0);

    double cwise_min_sum = 0; 
    double cwise_max_sum = 0; 

    while(it_a && it_b) {
      if(it_a.index() < it_b.index()) {
        cwise_max_sum += it_a.value();
        ++it_a;
      } else if(it_a.index() > it_b.index()) {
        cwise_max_sum += it_b.value();
        ++it_b;
      } else {
        cwise_min_sum += std::min(it_a.value(), it_b.value());
        cwise_max_sum += std::max(it_a.value(), it_b.value());
        ++it_a, ++it_b;
      }
    }

    while(it_a) {
      cwise_max_sum += it_a.value();
      ++it_a; 
    }

    while(it_b) {
      cwise_max_sum += it_b.value();
      ++it_b; 
    }
    
    double similarity = cwise_min_sum / cwise_max_sum; 
    return 1 - similarity;
  }

};


/* levenshtein distance
 */
struct levenshtein final : public distance_metric {

  double distance(const std::string& a, const std::string& b) const {

    std::string s;
    std::string t;
    size_t len_s;
    size_t len_t;

    // if 't' is not the longer string, switch them so 't' is the longer string
    if (a.length() > b.length()) {
      t = a;
      s = b;
    } else {
      s = a;
      t = b;
    }

    // trim common prefix - these cannot add anything to the distance
    size_t idx_start = 0;
    while ((s[idx_start] == t[idx_start]) && (idx_start < s.length())) {
      idx_start++;
    }

    if (idx_start == t.length()) {  // if the entire strings match, distance is 0
      return 0;
    }

    s = s.substr(idx_start);
    t = t.substr(idx_start);

    len_s = s.length();
    len_t = t.length();

    // if either trimmed string has length 0, the distance is the length of the
    // other string. Since 's' is the shorter string, this should capture all
    // cases.
    if (len_s == 0)
      return len_t;

    // initialize the rows
    std::vector<size_t> v0(len_t + 1, 0);
    std::vector<size_t> v1(len_t + 1, 0);

    for (size_t i = 0; i < v0.size(); i++) {
      v0[i] = i;
    }

    size_t cost;

    // For each letter in the shorter string...
    for (size_t i = 0; i < len_s; i++) {
      v1[0] = i + 1;
      
      // Fill in the second row
      for (size_t j = 0; j < len_t; j++) {
        cost = (s[i] == t[j]) ? 0 : 1;
        v1[j+1] = std::min({v0[j] + cost, v0[j+1] + 1, v1[j] + 1});
      }

      // Copy the second row into the first row
      for (size_t j = 0; j < v0.size(); j++) {
        v0[j] = v1[j];
      }
    }

    return v1[t.length()];
  }
};

struct custom_distance final : public distance_metric {

  // std::function<double(const std::vector<double>, const std::vector<double>)> fn;
  std::function<double(const flexible_type, const flexible_type)> fn;

  double distance(const std::vector<double>& a, const std::vector<double>& b) const {
    return fn(a, b);
  }

  double distance(const std::string& a, const std::string& b) const {
    return fn(a, b);
  }
};

std::shared_ptr<distance_metric> inline distance_metric::make_distance_metric(
    function_closure_info fn) {
  auto fn_name = fn.native_fn_name;
  distance_metric d;
  if (boost::algorithm::ends_with(fn_name, ".euclidean")) {
    return  distance_metric::make_dist_instance("euclidean");
  } else if (boost::algorithm::ends_with(fn_name, ".squared_euclidean")) {
    return distance_metric::make_dist_instance("squared_euclidean");
  } else if (boost::algorithm::ends_with(fn_name, ".gaussian_kernel")) {
    return distance_metric::make_dist_instance("gaussian_kernel");
  } else if (boost::algorithm::ends_with(fn_name, ".manhattan")) {
    return distance_metric::make_dist_instance("manhattan");
  } else if (boost::algorithm::ends_with(fn_name, ".cosine")) {
    return distance_metric::make_dist_instance("cosine");
  } else if (boost::algorithm::ends_with(fn_name, ".dot_product")) {
    return distance_metric::make_dist_instance("dot_product");
  } else if (boost::algorithm::ends_with(fn_name, ".transformed_dot_product")) {
    return distance_metric::make_dist_instance("transformed_dot_product");
  } else if (boost::algorithm::ends_with(fn_name, ".jaccard")) {
    return distance_metric::make_dist_instance("jaccard");
  } else if (boost::algorithm::ends_with(fn_name, ".weighted_jaccard")) {
    return distance_metric::make_dist_instance("weighted_jaccard");
  } else if (boost::algorithm::ends_with(fn_name, ".levenshtein")) {
    return distance_metric::make_dist_instance("levenshtein");
  } else {
    // Create a distance metric that uses the user-provided function.
    // Only functions that take dense vectors are currently supported.
    auto actual_fn = variant_get_value< std::function<double(const std::vector<double>, const std::vector<double>)> >(fn);
    auto d = nearest_neighbors::custom_distance();
    d.fn = actual_fn;
    auto sp = std::make_shared<distance_metric>(d);
    return sp;
  }
}

std::shared_ptr<distance_metric> inline distance_metric::make_dist_instance(
  const std::string& dist_name) {
  
  std::shared_ptr<distance_metric> dist_ptr;

  if (dist_name == "euclidean") 
    dist_ptr.reset(new euclidean); 
  else if(dist_name == "squared_euclidean")
    dist_ptr.reset(new squared_euclidean);
  else if(dist_name == "gaussian_kernel")
    dist_ptr.reset(new gaussian_kernel);
  else if (dist_name == "manhattan")
    dist_ptr.reset(new manhattan);
  else if (dist_name == "cosine")
    dist_ptr.reset(new cosine);
  else if (dist_name == "dot_product")
    dist_ptr.reset(new dot_product);
  else if (dist_name == "transformed_dot_product")
    dist_ptr.reset(new transformed_dot_product);
  else if (dist_name == "jaccard")
    dist_ptr.reset(new jaccard);
  else if (dist_name == "weighted_jaccard")
    dist_ptr.reset(new weighted_jaccard);
  else if (dist_name == "levenshtein")
    dist_ptr.reset(new levenshtein);
  else
    log_and_throw("Unrecognized distance: " + dist_name);

  return dist_ptr;
}


}}

#endif  // TURI_NN_DISTANCE_FUNCTIONS_H_
