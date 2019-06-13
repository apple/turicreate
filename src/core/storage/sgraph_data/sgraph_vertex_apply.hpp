/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_VERTEX_APPLY_HPP
#define TURI_SGRAPH_SGRAPH_VERTEX_APPLY_HPP

#include <vector>
#include <tuple>
#include <type_traits>
#include <core/data/flexible_type/flexible_type.hpp>
#include <functional>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sgraph_data/hilbert_parallel_for.hpp>
#include <core/storage/sgraph_data/sgraph_compute_vertex_block.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi {


/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute SGraph Compute
 * \{
 */

/**
 * Graph Computation Functions
 */
namespace sgraph_compute {

/**
 * Performs a map operation combining one external array (other) with the
 * graph vertex data. other must be the same length as the vertex data.
 * Abstractly performs the following computation:
 * \code
 *  for each vertex i:
 *    out[i] = fn(vertex_data[i], other[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the first argument, a vector<flexible_type>
 * and the second argument, T, and must return an object castable to a
 * flexible_type of the result_type specified.
 *
 * For instance, if I am going to compute a change in pagerank value between
 * the existing pagerank column, and a new computed column:
 *
 * \code
 *  const size_t pr_idx = g.get_vertex_field_id("pagerank");
 *
 *  // compute the change in pagerank
 *  auto delta = sgraph_compute::vertex_apply(
 *      g,
 *      new_pagerank,      // a vector<shared_ptr<sarray<flexible_type>>>
 *      flex_type_enum::FLOAT,
 *      [&](const std::vector<flexible_type>& vdata, const flexible_type& y) {
 *        return std::abs((double)(vdata[pr_idx]) - (double)(y));
 *      });
 * \endcode
 *
 * Note that if the apply is only going to access one column, the alternative
 * overload will be more efficient.
 */
template <typename Fn, typename T>
std::vector<std::shared_ptr<sarray<flexible_type>>>
vertex_apply(sgraph& g,
             std::vector<std::shared_ptr<sarray<T>>> & other,
             flex_type_enum result_type,
             Fn fn) {
  ASSERT_EQ(g.get_num_partitions(), other.size());
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(g.get_num_partitions());
  // get all the vertex partitions.
  const std::vector<sframe>& vdata = g.vertex_group();
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    binary_transform(vdata[i], *other[i], *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}

/**
 * Performs a map operation on graph vertex_data.
 * Abstractly performs the following computation:
 * \code
 *  for each vertex i:
 *    out[i] = fn(vertex_data[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the only argument, a vector<flexible_type>.
 * and must return an object castable to a flexible_type of the result_type
 * specified.
 *
 * For instance, if I am going to compute a "normalized" pagerank.
 *
 * \code
 *  double pagerank_sum = [...from reduce function below...]
 *
 *  const size_t pr_idx = g.get_vertex_field_id("pagerank");
 *
 *  auto normalized = sgraph_compute::vertex_apply(
 *      g,
 *      flex_type_enum::FLOAT,
 *      [&](const std::vector<flexible_type>& vdata) {
 *        return vdata[pr_idx] / pagerank_sum;
 *      });
 * \endcode
 *
 * Note that if the apply is only going to access one column, the alternative
 * overload will be more efficient.
 */
template <typename Fn>
std::vector<std::shared_ptr<sarray<flexible_type>>>
vertex_apply(sgraph& g,
             flex_type_enum result_type,
             Fn fn) {
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(g.get_num_partitions());
  // get all the vertex partitions.
  const std::vector<sframe>& vdata = g.vertex_group();
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    transform(vdata[i], *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}


/**
 * Performs a map operation combining one external array (other) with one
 * column of the graph vertex data. other must be the same length as the vertex data.
 * Abstractly performs the following computation:
 * \code
 *  for each vertex i:
 *    out[i] = fn(vertex_data[column_name][i], other[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the first argument, a flexible_type
 * and the second argument, T, and must return an object castable to a
 * flexible_type of the result_type specified.
 *
 * For instance, if I am going to compute a change in pagerank value between
 * the existing pagerank column, and a new computed column:
 *
 * \code
 *  // compute the change in pagerank
 *  auto delta = sgraph_compute::vertex_apply(
 *      g,
 *      "pagerank",
 *      new_pagerank,      // a vector<shared_ptr<sarray<flexible_type>>>
 *      flex_type_enum::FLOAT,
 *      [&](const flexible_type& vdata, const flexible_type& y) {
 *        return std::abs((double)(vdata) - (double)(y));
 *      });
 * \endcode
 */
template <typename Fn, typename T>
std::vector<std::shared_ptr<sarray<flexible_type>>>
vertex_apply(sgraph& g,
             std::string column_name,
             std::vector<std::shared_ptr<sarray<T>>> & other,
             flex_type_enum result_type,
             Fn fn) {
  ASSERT_EQ(g.get_num_partitions(), other.size());
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(g.get_num_partitions());
  // get all the vertex partitions.
  const std::vector<sframe>& vdata = g.vertex_group();
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = vdata[i].select_column(column_name);
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    binary_transform(*graph_field, *other[i], *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}

/**
 * Performs a map operation on one column of the graph vertex_data.
 * Abstractly performs the following computation:
 * \code
 *  for each vertex i:
 *    out[i] = fn(vertex_data[column_name][i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the only argument, a flexible_type.
 * and must return an object castable to a flexible_type of the result_type
 * specified.
 *
 * For instance, if I am going to compute a "normalized" pagerank.
 *
 * \code
 *  double pagerank_sum = [...from reduce function below...]
 *
 *  auto normalized = sgraph_compute::vertex_apply(
 *      g,
 *      "pagerank",
 *      flex_type_enum::FLOAT,
 *      [&](const flexible_type& y) {
 *        return y / pagerank_sum;
 *      });
 * \endcode
 */
template <typename Fn>
std::vector<std::shared_ptr<sarray<flexible_type>>>
vertex_apply(sgraph& g,
             std::string column_name,
             flex_type_enum result_type,
             Fn fn) {
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(g.get_num_partitions());
  // get all the vertex partitions.
  const std::vector<sframe>& vdata = g.vertex_group();
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = vdata[i].select_column(column_name);
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    transform(*graph_field, *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}


/**
 * Performs a reduction over the graph data. If you are only reducing
 * over one column, see the alternative overload.
 *
 * The vertex data is partitioned into small chunks. Within each chunk,
 * the reducer function is called on every element using init as the initial
 * value. This accomplishes a collection of partial reductions.
 * Finally, the combine function is used to merge all the partial reductions
 * which is then returned.
 *
 * Abstractly performs the following computation:
 * \code
 *  total_reduction = init
 *  for each partition:
 *     partial_reduction[partition] = init
 *     for each vertex i in partition:
 *       reducer(vertex_data[i], partial_reduction[partition])
 *     combiner(partial_reduction[partition], total_reduction)
 *  return total_reduction
 * \endcode
 *
 * Example. Here were compute the sum of the pagerank field of every vertex.
 * \code
 *  const size_t pr_idx = g.get_vertex_field_id("pagerank");
 *  total_pagerank =
 *        sgraph_compute::reduce<double>(g,
 *                               [](const std::vector<flexible_type>& vdata, double& acc) {
 *                                 // ran on each vertex data
 *                                 acc += (flex_float)vdata[pr_idx];
 *                               },
 *                               [](const double& v, double& acc) {
 *                                 // partial combiner.
 *                                 acc += v;
 *                               });
 * \endcode
 *
 * Note that if the apply is only going to access one column, the alternative
 * overload will be more efficient.
 */
template <typename ResultType, typename Reducer, typename Combiner>
typename std::enable_if<!std::is_convertible<Reducer, std::string>::value, ResultType>::type
/*ResultType*/ vertex_reduce(sgraph& g,
                             Reducer fn,
                             Combiner combine,
                             ResultType init = ResultType()) {
  const std::vector<sframe>& vdata = g.vertex_group();
  mutex lock;
  ResultType ret = init;
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::vector<ResultType> result =
        turi::reduce(vdata[i],
                         [&](const std::vector<flexible_type>& left, ResultType& right) {
                           fn(left, right);
                           return true;
                         }, init);

    std::unique_lock<mutex> result_lock(lock);
    for (ResultType& res: result) {
      combine(res, ret);
    }
  });
  return ret;
}


/**
 * Performs a reduction over a single column of the graph data.
 *
 * The vertex data is partitioned into small chunks. Within each chunk,
 * the reducer function is called on every element using init as the initial
 * value. This accomplishes a collection of partial reductions.
 * Finally, the combine function is used to merge all the partial reductions
 * which is then returned.
 *
 *
 * Abstractly performs the following computation:
 * \code
 *  total_reduction = init
 *  for each partition:
 *     partial_reduction[partition] = init
 *     for each vertex i in partition:
 *       reducer(vertex_data[columnname][i], partial_reduction[partition])
 *     combiner(partial_reduction[partition], total_reduction)
 *  return total_reduction
 * \endcode
 *
 * Example. Here were compute the sum of the pagerank field of every vertex.
 * \code
 *  total_pagerank =
 *        sgraph_compute::reduce<double>(g,
 *                               "pagerank",
 *                               [](const flexible_type& pr, double& acc) {
 *                                 // ran on each vertex data
 *                                 acc += (flex_float)pr;
 *                               },
 *                               [](const double& v, double& acc) {
 *                                 // partial combiner.
 *                                 acc += v;
 *                               });
 * \endcode
 */
template <typename ResultType, typename Reducer, typename Combiner>
ResultType vertex_reduce(sgraph& g,
                         std::string column_name,
                         Reducer fn,
                         Combiner combine,
                         ResultType init = ResultType()) {
  const std::vector<sframe>& vdata = g.vertex_group();
  mutex lock;
  ResultType ret = init;
  parallel_for((size_t)(0), (size_t)g.get_num_partitions(), [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = vdata[i].select_column(column_name);
    std::vector<ResultType> result =
        turi::reduce(*graph_field,
                         [&](const flexible_type& left, ResultType& right) {
                           fn(left, right);
                           return true;
                         }, init);
    std::unique_lock<mutex> result_lock(lock);
    for (ResultType& res: result) {
      combine(res, ret);
    }
  });
  return ret;
}

} // end of sgraph

/// \}
} // end of turicreate
#endif
