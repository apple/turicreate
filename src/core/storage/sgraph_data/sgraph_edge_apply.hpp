/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_EDGE_APPLY_HPP
#define TURI_SGRAPH_SGRAPH_EDGE_APPLY_HPP

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
 * graph edge data. other must be the same length as the edge data.
 * Abstractly performs the following computation:
 * \code
 *  for each edge i:
 *    out[i] = fn(edge[i], other[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the first argument, a vector<flexible_type>
 * and the second argument, T, and must return an object castable to a
 * flexible_type of the result_type specified.
 *
 * For instance, if I am going to compute a change in message value between
 * the existing column, and a new computed column:
 *
 * \code
 *  const size_t msg_idx = g.get_edge_field_id("message");
 *
 *  // compute the change in message
 *  auto delta = sgraph_compute::edge_apply(
 *      g,
 *      new_message,      // a vector<shared_ptr<sarray<flexible_type>>>
 *      flex_type_enum::FLOAT,
 *      [&](const std::vector<flexible_type>& edata, const flexible_type& y) {
 *        return std::abs((double)(edata[msg_idx]) - (double)(y));
 *      });
 * \endcode
 *
 * Note that if the apply is only going to access one column, the alternative
 * overload will be more efficient.
 */
template <typename Fn, typename T>
std::vector<std::shared_ptr<sarray<flexible_type>>>
edge_apply(sgraph& g,
           std::vector<std::shared_ptr<sarray<T>>> & other,
           flex_type_enum result_type,
           Fn fn) {
  size_t len = g.get_num_partitions() * g.get_num_partitions();
  ASSERT_EQ(len, other.size());
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(len);
  // get all the edge partitions.
  const std::vector<sframe>& edata = g.edge_group();
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    binary_transform(edata[i], *other[i], *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}

/**
 * Performs a map operation on graph edge_data.
 * Abstractly performs the following computation:
 * \code
 *  for each edge i:
 *    out[i] = fn(edge_data[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the only argument, a vector<flexible_type>.
 * and must return an object castable to a flexible_type of the result_type
 * specified.
 *
 * For instance, if I am going to compute a "normalized" sampling vector.
 *
 * \code
 *  auto normalized = sgraph_compute::edge_apply(
 *      g,
 *      flex_type_enum::FLOAT,
 *      [&](std::vector<flexible_type>& edata) {
 *        double sum = 0.0;
 *        for (const auto& v : edata) sum += v;
 *        for (auto& v : edata): v /= sum;
 *        return edata;
 *      });
 * \endcode
 *
 * Note that if the apply is only going to access one column, the alternative
 * overload will be more efficient.
 */
template <typename Fn>
std::vector<std::shared_ptr<sarray<flexible_type>>>
edge_apply(sgraph& g,
             flex_type_enum result_type,
             Fn fn) {
  size_t len = g.get_num_partitions() * g.get_num_partitions();
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(len);
  // get all the edge partitions.
  const std::vector<sframe>& edata = g.edge_group();
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> ret_partition = std::make_shared<sarray<flexible_type>>();
    ret_partition->open_for_write(1);
    ret_partition->set_type(result_type);
    transform(edata[i], *ret_partition, fn);
    ret_partition->close();
    ret[i] = ret_partition;
  });
  return ret;
}


/**
 * Performs a map operation combining one external array (other) with one
 * column of the graph edge data. other must be the same length as the edge data.
 * Abstractly performs the following computation:
 * \code
 *  for each edge i:
 *    out[i] = fn(edge_data[column_name][i], other[i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the first argument, a flexible_type
 * and the second argument, T, and must return an object castable to a
 * flexible_type of the result_type specified.
 *
 * For instance, if I am going to compute a change in message value between
 * the existing column, and a new computed column:
 *
 * \code
 *  // compute the change in message
 *  auto delta = sgraph_compute::edge_apply(
 *      g,
 *      "message",
 *      new_message,      // a vector<shared_ptr<sarray<flexible_type>>>
 *      flex_type_enum::FLOAT,
 *      [&](const flexible_type& edata, const flexible_type& y) {
 *        return std::abs((double)(edata) - (double)(y));
 *      });
 * \endcode
 */
template <typename Fn, typename T>
std::vector<std::shared_ptr<sarray<flexible_type>>>
edge_apply(sgraph& g,
             std::string column_name,
             std::vector<std::shared_ptr<sarray<T>>> & other,
             flex_type_enum result_type,
             Fn fn) {
  size_t len = g.get_num_partitions() * g.get_num_partitions();
  ASSERT_EQ(other.size(), len);
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(len);
  // get all the edge partitions.
  const std::vector<sframe>& edata = g.edge_group();
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = edata[i].select_column(column_name);
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
 * Performs a map operation on one column of the graph edge_data.
 * Abstractly performs the following computation:
 * \code
 *  for each edge i:
 *    out[i] = fn(edge_data[column_name][i])
 * \endcode
 * out must be of the result_type specified.
 *
 * The function must take as the only argument, a flexible_type.
 * and must return an object castable to a flexible_type of the result_type
 * specified.
 *
 * For instance, if I am going to compute the log of the message column.
 *
 * \code
 *  auto normalized = sgraph_compute::edge_apply(
 *      g,
 *      "message",
 *      flex_type_enum::FLOAT,
 *      [&](const flexible_type& y) {
 *        return log(y);
 *      });
 * \endcode
 */
template <typename Fn>
std::vector<std::shared_ptr<sarray<flexible_type>>>
edge_apply(sgraph& g,
             std::string column_name,
             flex_type_enum result_type,
             Fn fn) {
  size_t len = g.get_num_partitions()*g.get_num_partitions();
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret(len);
  // get all the edge partitions.
  const std::vector<sframe>& edata = g.edge_group();
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = edata[i].select_column(column_name);
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
 * The edge data is partitioned into small chunks. Within each chunk,
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
 *     for each edge i in partition:
 *       reducer(edge_data[i], partial_reduction[partition])
 *     combiner(partial_reduction[partition], total_reduction)
 *  return total_reduction
 * \endcode
 *
 * Example. Here were compute the sum of the triangle_count field of every edge.
 * \code
 *  const size_t triangle_idx = g.get_edge_field_id("triangle_counts");
 *  total_triangles =
 *        sgraph_compute::reduce<double>(g,
 *                               [](const std::vector<flexible_type>& edata, double& acc) {
 *                                 // ran on each edge data
 *                                 acc += (flex_int)edata[triangle_idx];
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
/*ResultType*/ edge_reduce(sgraph& g,
                             Reducer fn,
                             Combiner combine,
                             ResultType init = ResultType()) {
  const std::vector<sframe>& edata = g.edge_group();
  size_t len = g.get_num_partitions() * g.get_num_partitions();
  mutex lock;
  ResultType ret = init;
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::vector<ResultType> result =
        turi::reduce(edata[i],
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
 * The edge data is partitioned into small chunks. Within each chunk,
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
 *     for each edge i in partition:
 *       reducer(edge_data[columnname][i], partial_reduction[partition])
 *     combiner(partial_reduction[partition], total_reduction)
 *  return total_reduction
 * \endcode
 *
 * Example. Here were compute the sum of the triangle field of every edge.
 * \code
 *  total_triangles =
 *        sgraph_compute::reduce<double>(g,
 *                               "triangle",
 *                               [](const flexible_type& tr, double& acc) {
 *                                 // ran on each edge data
 *                                 acc += (flex_int)tr;
 *                               },
 *                               [](const double& v, double& acc) {
 *                                 // partial combiner.
 *                                 acc += v;
 *                               });
 * \endcode
 */
template <typename ResultType, typename Reducer, typename Combiner>
ResultType edge_reduce(sgraph& g,
                         std::string column_name,
                         Reducer fn,
                         Combiner combine,
                         ResultType init = ResultType()) {
  const std::vector<sframe>& edata = g.edge_group();
  size_t len = g.get_num_partitions() * g.get_num_partitions();
  mutex lock;
  ResultType ret = init;
  parallel_for((size_t)(0), len, [&](size_t i) {
    std::shared_ptr<sarray<flexible_type>> graph_field = edata[i].select_column(column_name);
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
