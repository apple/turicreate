/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DISTANCES_H_
#define TURI_DISTANCES_H_

#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/util/algorithmic_utils.hpp>
#include <toolkits/nearest_neighbors/distance_functions.hpp>
#include <core/data/sframe/gl_sarray.hpp>

namespace turi { namespace distances {

typedef Eigen::VectorXd dense_vector;
typedef Eigen::SparseVector<double> sparse_vector;

/**
 * Utility function for taking a pair of dictionaries whose keys are
 * flexible_type and returning a sparse vector representation of each.
 * The index value of each key is determined by the order in which it appears.
 */
std::pair<sparse_vector, sparse_vector>
    convert_dict_pair_to_sparse(const flex_dict& a, const flex_dict& b) {
  sparse_vector av(a.size() + b.size());
  sparse_vector bv(a.size() + b.size());
  av.reserve(a.size());
  bv.reserve(b.size());

  // Initialize the map from value to the assigned index
  size_t current_index = 0;
  auto value_to_index = std::unordered_map<flexible_type, size_t>();

  // Iterate through the first dictionary, filling in the sparse_vector
  for (const auto& kv : a) {
    if (value_to_index.count(kv.first) == 0) {
      value_to_index[kv.first] = current_index;
      current_index++;
    }
    size_t index = value_to_index.at(kv.first);
    av.coeffRef(index) = kv.second;
  }

  // Iterate through the second dictionary, filling in the sparse_vector
  for (const auto& kv : b) {
    if (value_to_index.count(kv.first) == 0) {
      value_to_index[kv.first] = current_index;
      current_index++;
    }
    size_t index = value_to_index.at(kv.first);
    bv.coeffRef(index) = kv.second;
  }
  return std::make_pair(av, bv);
}

/**
 * Utility function for taking a pair of lists whose values are
 * flexible_type and returning a sparse vector representation of each.
 */
std::pair<sparse_vector, sparse_vector>
    convert_list_pair_to_sparse(const flex_list& a, const flex_list& b) {
  sparse_vector av(a.size() + b.size());
  sparse_vector bv(a.size() + b.size());
  av.reserve(a.size());
  bv.reserve(b.size());

  // Initialize the map from value to the assigned index
  size_t current_index = 0;
  auto value_to_index = std::unordered_map<flexible_type, size_t>();

  // Iterate through the first list, filling in the sparse_vector
  for (const auto& v : a) {
    if (value_to_index.count(v) == 0) {
      value_to_index[v] = current_index;
      av.coeffRef(current_index) = 0;
      current_index++;
    }
    size_t index = value_to_index.at(v);
    av.coeffRef(index) += 1;
  }

  // Iterate through the second list, filling in the sparse_vector
  for (const auto& v : b) {
    if (value_to_index.count(v) == 0) {
      value_to_index[v] = current_index;
      bv.coeffRef(current_index) = 0;
      current_index++;
    }
    size_t index = value_to_index.at(v);
    bv.coeffRef(index) += 1;
  }
  return std::make_pair(av, bv);
}



/**
 * For each of the following distances, we take a pair of flexible_type
 * objects and dispatch to one of two implementations:
 * - VECTOR: for several of the distances, it is sensible to compute the
 *   distance between two vectors of equal length
 * - DICT: for many distances we expose an implementation that can compute
 *   a distance between two dictionaries.
 * - LIST: treated like a dict containing the counts of each unique item.
 */

double compute_distance(std::string distance_name, const flexible_type& a, const flexible_type& b) {

  // Check that types match
  auto a_t = a.get_type();
  auto b_t = b.get_type();
  if (a_t != b_t) log_and_throw("Argument types must match.");

  // Make a distance metric struct that contains both sparse and dense
  // implementations
  auto d = nearest_neighbors::distance_metric::make_dist_instance(distance_name);

  if (a_t == flex_type_enum::VECTOR) {
    DASSERT_TRUE(a.size() == b.size());
    DASSERT_TRUE(a.size() > 0);
    const auto& a_vec = a.get<std::vector<double>>();
    const auto& b_vec = b.get<std::vector<double>>();
    Eigen::Map<const dense_vector> av(a_vec.data(), a.size());
    Eigen::Map<const dense_vector> bv(b_vec.data(), b.size());

    return d->distance(av, bv);

  } else if (a_t == flex_type_enum::DICT) {

    // Convert into sparse vectors
    auto ab = convert_dict_pair_to_sparse(a, b);

    // If both sparse vectors are empty, then return 0
    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;

    // Compute the distance
    return d->distance(ab.first, ab.second);

  } else if (a_t == flex_type_enum::LIST) {

    // Convert into sparse vectors
    auto ab = convert_list_pair_to_sparse(a, b);

    // If both sparse vectors are empty, then return 0
    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;

    // Compute the distance
    return d->distance(ab.first, ab.second);

  } else {
    log_and_throw("This distance does not support the provided type.");
  }
}

double gaussian_kernel(const flexible_type& a, const flexible_type& b) {
  return compute_distance("gaussian_kernel", a, b);
}

double euclidean(const flexible_type& a, const flexible_type& b) {
  return compute_distance("euclidean", a, b);
}

double squared_euclidean(const flexible_type& a, const flexible_type& b) {
  return compute_distance("squared_euclidean", a, b);
}

double manhattan(const flexible_type& a, const flexible_type& b) {
  return compute_distance("manhattan", a, b);
}

double cosine(const flexible_type& a, const flexible_type& b) {
  return compute_distance("cosine", a, b);
}

double dot_product(const flexible_type& a, const flexible_type& b) {
  return compute_distance("dot_product", a, b);
}

double transformed_dot_product(const flexible_type& a, const flexible_type& b) {
  return compute_distance("transformed_dot_product", a, b);
}

double levenshtein(const std::string& a, std::string& b) {
  return nearest_neighbors::levenshtein().distance(a, b);
}

double jaccard(const flexible_type& a, const flexible_type& b) {
  auto a_t = a.get_type();
  auto b_t = b.get_type();
  if (a_t != b_t) log_and_throw("Argument types must match.");
  if (a_t == flex_type_enum::DICT) {
    auto ab = convert_dict_pair_to_sparse(a, b);

    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;

    return nearest_neighbors::jaccard().distance(ab.first, ab.second);

  } else if (a_t == flex_type_enum::LIST) {
    auto ab = convert_list_pair_to_sparse(a, b);

    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;

    return nearest_neighbors::jaccard().distance(ab.first, ab.second);

  } else {

    log_and_throw("This distance does not support the provided type.");
  }
}


double weighted_jaccard(const flexible_type& a, const flexible_type& b) {
  auto a_t = a.get_type();
  auto b_t = b.get_type();
  if (a_t != b_t) log_and_throw("Argument types must match.");
  if (a_t == flex_type_enum::DICT) {
    auto ab = convert_dict_pair_to_sparse(a, b);
    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;
    return nearest_neighbors::weighted_jaccard().distance(ab.first, ab.second);

  } else if (a_t == flex_type_enum::LIST) {
    auto ab = convert_list_pair_to_sparse(a, b);

    if ((ab.first.size() == 0) && (ab.second.size() == 0))
      return 0.0;

    return nearest_neighbors::weighted_jaccard().distance(ab.first, ab.second);

 } else {
    log_and_throw("This distance does not support the provided type.");
  }
}

double apply_w_custom(function_closure_info fn,
                      const std::vector<double>& a,
                      const std::vector<double>& b) {
  auto actual_fn = variant_get_value< std::function<double(const std::vector<double>, const std::vector<double>)> >(fn);
  auto d = nearest_neighbors::custom_distance();
  d.fn = actual_fn;
  return d.distance(a, b);
}

gl_sarray apply(gl_sarray a, gl_sarray b, function_closure_info fn) {
  if (a.dtype() != b.dtype())
    log_and_throw("Types of both SArrays must match.");

  auto actual_fn = variant_get_value< std::function<double(const flexible_type, const flexible_type)> >(fn);

  gl_sarray_writer writer(flex_type_enum::FLOAT, 1);
  auto ar = a.range_iterator();
  auto br = b.range_iterator();
  for (auto ita = ar.begin(), itb = br.begin(); ita != ar.end(); ++ita, ++itb) {
    writer.write(actual_fn(*ita, *itb), 0);
  }
  return writer.close();
}


BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(euclidean, "x", "y")
REGISTER_DOCSTRING(euclidean, "Compute the Euclidean distance between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(squared_euclidean, "x", "y")
REGISTER_DOCSTRING(squared_euclidean, "Compute the squared Euclidean distance between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(cosine, "x", "y")
REGISTER_DOCSTRING(cosine, "Compute the cosine distance between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(dot_product, "x", "y")
REGISTER_DOCSTRING(dot_product, "Compute the dot_product distance between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(transformed_dot_product, "x", "y")
REGISTER_DOCSTRING(transformed_dot_product, "Compute the dot product between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(manhattan, "x", "y")
REGISTER_DOCSTRING(manhattan, "Compute the Manhattan distance between two dictionaries or two lists of equal length.");
REGISTER_FUNCTION(levenshtein, "x", "y")
REGISTER_DOCSTRING(levenshtein, "Compute the Levenshtein distance between two strings.");
REGISTER_FUNCTION(jaccard, "x", "y")
REGISTER_DOCSTRING(jaccard, "Compute the Jaccard distance between two dictionaries.");
REGISTER_FUNCTION(gaussian_kernel, "x", "y")
REGISTER_DOCSTRING(gaussian_kernel, "Compute the Gaussian distance between two dictionaries.");
REGISTER_FUNCTION(weighted_jaccard, "x", "y")
REGISTER_DOCSTRING(weighted_jaccard, "Compute the weighted Jaccard distance between two dictionaries.");

REGISTER_FUNCTION(apply_w_custom, "f", "x", "y")

REGISTER_FUNCTION(apply, "a", "b", "fn")

END_FUNCTION_REGISTRATION

std::vector<turi::toolkit_function_specification> get_toolkit_function_registration();
}}




#endif /* TURI_DISTANCES_H_ */
