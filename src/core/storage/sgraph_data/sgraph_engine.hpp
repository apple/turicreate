/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_sgraph_engine_HPP
#define TURI_SGRAPH_SGRAPH_sgraph_engine_HPP

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
 * PowerGraph computation.
 *
 * Two central graph computation operations are provided by this class:
 *  - Vertex Gather
 *  - Parallel For Edges
 *
 * ### Vertex Gather ###
 * Given a graph, this class computes for each vertex, a generalized "sum" over
 * neighborhood of each vertex. You need to define a function which is then
 * given
 *  - the data on the central vertex,
 *  - the data on the edge,
 *  - the data on the other vertex
 *  - The direction of the edge.
 *
 * The function then performs some computation and aggregates the result
 * into a combiner.
 *
 * Abstractly:
 * \code
 * for v \in vertices:
 *    output[v] = initial_value
 *
 * for v \in vertices:
 *    for u \in neighbor of v:
 *      gather(data[v], data[v,u], data[u], edge_direction_of_v_u, output[v])
 *
 * return output
 * \endcode
 *
 * At completion, a vector of shared_ptr to sarrays will be returned. Each sarray
 * corresponds to one vertex partition, and each sarray row corresponds to the
 * result of summing across the neighborhood of the vertex.
 *
 * This is easiest to explain with an example. Pagerank for instance:
 * \code
 *  const size_t degree_idx = g.get_vertex_field_id("out_degree");
 *  const size_t pr_idx = g.get_vertex_field_id("pagerank");
 *
 *  // declare a sgraph_engine computation object of a particular return type.
 *  sgraph_compute::sgraph_engine<flexible_type> ga;
 *
 *  ret = ga.gather(g,
 *      [=](const graph_data_type& center,
 *          const graph_data_type& edge,
 *          const graph_data_type& other,
 *          edge_direction edgedir,
 *          flexible_type& combiner) {
 *         combiner = combiner + (1-random_jump_prob) * (other[pr_idx] / other[degree_idx]);
 *      },
 *      flexible_type(random_jump_prob), // initial value
 *      edge_direction::IN_EDGE);        // edges to sum over
 * \endcode
 *
 *
 * ### Parallel For Edges###
 *
 * The parallel_for_edges() performs the following operations on the graph:
 *
 * Abstractly:
 * Given a function edge_map: (source_data, edge_data, target_data) -> T,
 * \code
 *  for e \in edges:
 *    output[e] = edge_map(e.source.data(), e.data(), e.target.data())
 *
 * \endcode
 *
 * At completion it returns a vector of shared_ptr to sarrays.
 * Each sarray corresponds to one edge partition, and each edge row corresponds
 * to the result of the map operation over the edge.
 *
 * \note
 * Refactoring is required in the furture for this class.
 * Essentially, this class should provide the mechanism to iterate over edges
 * with vertex data properly loaded. Then the user facing "gather" and "parallel for edges"
 * will be simple functions that uses this class.
 */
template <typename T>
class sgraph_engine {
 public:
  typedef sgraph::vertex_partition_address vertex_partition_address;
  typedef sgraph::edge_partition_address edge_partition_address;
  typedef sgraph::edge_direction edge_direction;
  typedef std::vector<flexible_type> graph_data_type;

  sgraph_engine() { }

  /**************************************************************************/
  /*                                                                        */
  /*                             Gather                                     */
  /*                                                                        */
  /**************************************************************************/
  using const_gather_function_type = std::function<void(const graph_data_type& center,
                                                        const graph_data_type& edge,
                                                        const graph_data_type& other,
                                                        edge_direction edgedir,
                                                        T& combiner)>;
  std::vector<std::shared_ptr<sarray<T>>> gather(sgraph& graph,
                                const_gather_function_type gather,
                                const T& initial_value,
                                edge_direction edgedir = edge_direction::ANY_EDGE,
                                size_t central_group = 0,
                                std::unordered_set<size_t> sgraph_compute_group = {0},
                                size_t parallel_limit = (size_t)(-1)) {
    if (parallel_limit == static_cast<size_t>(-1)) {
      parallel_limit = thread::cpu_count();
    }
    init_data_structures(graph, central_group, initial_value);
    hilbert_blocked_parallel_for(
         graph.get_num_partitions(),
         // The preamble to each parallel for block.
         // Thisis the collection of edges that will be executed in the next pass.
         [&](std::vector<std::pair<size_t, size_t> > edgeparts) {
           std::set<vertex_partition_address> vertex_partitions;
           std::set<size_t> combine_partitions;
           // for each partition requested, figure out exactly
           // which partition / group I need to load
           // That does depend on the edge direction I am
           // executing.
           for(auto edgepart: edgeparts) {
           logstream(LOG_INFO) << "Planning Execution on Edge Partition: "
                               << edgepart.first << " " << edgepart.second << std::endl;
             for(size_t gather_vgroup: sgraph_compute_group) {
               if (edgedir == edge_direction::ANY_EDGE ||
                   edgedir == edge_direction::IN_EDGE) {
                 // this is the edge partition I will read when I have to run
                 // this edge set. this is IN-edges. So src group is
                 // the gather_vgroup and dst group is the central group.
                 // partition is as defined by edgepart
                 edge_partition_address address(gather_vgroup, central_group,
                                                edgepart.first, edgepart.second);
                 combine_partitions.insert(address.get_dst_vertex_partition().partition);
                 vertex_partitions.insert(address.get_src_vertex_partition());
                 vertex_partitions.insert(address.get_dst_vertex_partition());
               }
               if (edgedir == edge_direction::ANY_EDGE ||
                          edgedir == edge_direction::OUT_EDGE) {
                 // this is the edge partition I will read when I have to run
                 // this edge set. this is OUT-edges. So dst group is
                 // the gather_vgroup and src group is the central group.
                 // partition is as defined by edgepart
                 edge_partition_address address(central_group, gather_vgroup,
                                                edgepart.first, edgepart.second);
                 combine_partitions.insert(address.get_src_vertex_partition().partition);
                 vertex_partitions.insert(address.get_src_vertex_partition());
                 vertex_partitions.insert(address.get_dst_vertex_partition());
               }
             }
           }
           // request loading of all the vertex partitions I need
           load_graph_vertex_blocks(graph, vertex_partitions);
           load_combine_blocks(combine_partitions);
         },
         // This is the actual parallel for, and this is the block I am to
         // be executing
         [&](std::pair<size_t, size_t> edgepart) {
           // at this stage we are in parallel. Also we we guaranteed to have all
           // the required vertices in memory. So we just need to sweep the edge set.
           // The trick is that edge partition (i,j) of group (a,b) only holds the
           // edges going from group a to group b. So depending on the requested
           // edge direction, we have to a little careful about which edge sets
           // we are actually loading.
           for(size_t gather_vgroup: sgraph_compute_group) {
             edge_partition_address address;
             // TODO: revisit the code when we actually have vertex groups
             address = edge_partition_address(gather_vgroup, central_group,
                                              edgepart.first, edgepart.second);
             sframe& edgeframe = graph.edge_partition(address);
             compute_const_gather(edgeframe, address, central_group, edgedir, gather);
           }
         });
    // flush the combine blocks
    load_combine_blocks(std::set<size_t>());
    return combine_sarrays;
  }


  /**************************************************************************/
  /*                                                                        */
  /*                           Parallel For Edges                           */
  /*                                                                        */
  /**************************************************************************/

  using const_edge_map_function_type = std::function<T(const graph_data_type& source,
                                                       graph_data_type& edge,
                                                       const graph_data_type& target)>;

  std::vector<std::shared_ptr<sarray<T>>> parallel_for_edges(sgraph& graph,
                                const_edge_map_function_type map_fn,
                                flex_type_enum ret_type,
                                size_t groupa = 0, size_t groupb = 0,
                                size_t parallel_limit = (size_t)(-1)) {
    if (parallel_limit == static_cast<size_t>(-1)) {
      parallel_limit = thread::cpu_count();
    }
    vertex_data.clear();
    vertex_data.resize(graph.get_num_groups());
    for(auto& v: vertex_data) v.resize(graph.get_num_partitions());

    size_t return_size = graph.get_num_partitions() * graph.get_num_partitions();
    std::vector<std::shared_ptr<sarray<T>>> return_edge_value(return_size);
    hilbert_blocked_parallel_for(
         graph.get_num_partitions(),
         // The preamble to each parallel for block.
         // Thisis the collection of edges that will be executed in the next pass.
         [&](std::vector<std::pair<size_t, size_t> > edgeparts) {
           std::set<vertex_partition_address> vertex_partitions;
           std::set<size_t> combine_partitions;
           // for each partition requested, figure out exactly
           // which partition / group I need to load
           // That does depend on the edge direction I am
           // executing.
           for(auto edgepart: edgeparts) {
           logstream(LOG_INFO) << "Planning Execution on Edge Partition: "
                               << edgepart.first << " " << edgepart.second << std::endl;
             // this is the edge partition I will read when I have to run
             // this edge set. this is IN-edges. So src group is
             // the gather_vgroup and dst group is the central group.
             // partition is as defined by edgepart
             edge_partition_address address(groupa, groupb, edgepart.first, edgepart.second);
             combine_partitions.insert(address.get_dst_vertex_partition().partition);
             vertex_partitions.insert(address.get_src_vertex_partition());
             vertex_partitions.insert(address.get_dst_vertex_partition());
           }
           // request loading of all the vertex partitions I need
           load_graph_vertex_blocks(graph, vertex_partitions);
         },
         // This is the actual parallel for, and this is the block I am to
         // be executing
         [&](std::pair<size_t, size_t> edgepart) {
           // at this stage we are in parallel. Also we we guaranteed to have all
           // the required vertices in memory. So we just need to sweep the edge set.
           // The trick is that edge partition (i,j) of group (a,b) only holds the
           // edges going from group a to group b. So depending on the requested
           // edge direction, we have to a little careful about which edge sets
           // we are actually loading.
           edge_partition_address address(groupa, groupb,
                                          edgepart.first, edgepart.second);
           sframe& edgeframe = graph.edge_partition(address);
           size_t partid = edgepart.first * graph.get_num_partitions() + edgepart.second;
           return_edge_value[partid] = compute_edge_map(edgeframe, address, map_fn, ret_type);
         });
    return return_edge_value;
  }

 private:
  // vertex_data[group][partition]
  std::vector<std::vector<vertex_block<sframe> > > vertex_data;
  // combine_data[partition]
  std::vector<vertex_block<sarray<T> > > combine_data;
  std::vector<std::shared_ptr<sarray<T> > > combine_sarrays;
  static constexpr size_t LOCK_ARRAY_SIZE = 1024;
  turi::mutex lock_array[LOCK_ARRAY_SIZE];
  flex_type_enum m_return_type = flex_type_enum::UNDEFINED;

  template <typename S>
  typename std::enable_if<std::is_same<S, flexible_type>::value>::type
  set_return_type(const S& t) {
    m_return_type = t.get_type();
  }

  template <typename S>
  typename std::enable_if<!std::is_same<S, flexible_type>::value>::type
  set_return_type(const S& t) { }

  /**
   * Initializes the temporary data structures, and the accumulation sarrays
   * we need to do the computation.
   */
  void init_data_structures(const sgraph& graph,
                            size_t sgraph_compute_group,
                            const T& initial_value) {
    vertex_data.clear();
    combine_data.clear();
    combine_sarrays.clear();
    set_return_type(initial_value);

    /*
     * A *very* sparsely populated buffer of vertex data.
     * vertex_data[group][partition][row]
     */
    vertex_data.resize(graph.get_num_groups());
    for(auto& v: vertex_data) v.resize(graph.get_num_partitions());
    // create the gather data
    combine_sarrays.resize(graph.get_num_partitions());
    combine_data.resize(graph.get_num_partitions());
    // shape up the combine_sarrays.
    // Create gather SArrays of the correct size
    parallel_for(size_t(0),
                 graph.get_num_partitions(),
                 [&](size_t i) {
                   const sframe& frame = graph.vertex_partition(i, sgraph_compute_group);
                   size_t rows = frame.num_rows();
                   combine_sarrays[i].reset(new sarray<T>());
                   combine_sarrays[i]->open_for_write(1);
                   auto iter = combine_sarrays[i]->get_output_iterator(0);
                   while(rows--) {
                     (*iter) = initial_value;
                     ++iter;
                   }
                   combine_sarrays[i]->close();
                 });

  }

  /**
   * This function will unload all graph vertex blocks which are not in the set
   * vertex_address and load all vertex blocks which are in the set.
   */
  void load_graph_vertex_blocks(sgraph& graph,
                                const std::set<vertex_partition_address>& vertex_address) {
    // look for all loaded blocks and if they are not in the vertex_address set, unload it
    for (size_t group = 0; group < vertex_data.size(); ++group) {
      for (size_t partition = 0; partition < vertex_data[group].size(); ++partition) {
        if (vertex_data[group][partition].is_loaded() &&
            vertex_address.count({group, partition}) == 0) {
          vertex_data[group][partition].unload();
        }
      }
    }
    std::vector<vertex_partition_address> vertex_address_vec;
    std::copy(vertex_address.begin(), vertex_address.end(),
              std::inserter(vertex_address_vec, vertex_address_vec.end()));

    // now, for each element in vertex_address, if it is not loaded, load it
    parallel_for(vertex_address_vec.begin(),
                 vertex_address_vec.end(),
                 // parallel for callback. called with each vertex address I need
                 // to load
                 [&](vertex_partition_address& part) {
                   // get the frame for the vertex partition
                   const sframe& frame = graph.vertex_partition(part.partition, part.group);
                   // load it into the vertex data.
                   logstream(LOG_INFO) << "Loading Vertex Partition: "
                                       << part.group << " " << part.partition << std::endl;
                   vertex_data[part.group][part.partition].load_if_not_loaded(frame);
                 });
  }


  /**
   * This function will unload all gather blocks not in the set partitions,
   * writing them back out to the sarray if modified. It will then load
   * all the gather blocks that are in the set.
   */
  void load_combine_blocks(const std::set<size_t> partitions) {
    for (size_t partition = 0; partition < combine_data.size(); ++partition) {
      if (combine_data[partition].is_loaded() && partitions.count(partition) == 0) {
        // reset the existing sarray and save the gather data to it.
        combine_sarrays[partition].reset(new sarray<T>());
        combine_sarrays[partition]->open_for_write(1);
        if (typeid(T) == typeid(flexible_type)) {
          combine_sarrays[partition]->set_type(m_return_type);
        }
        combine_data[partition].flush(*combine_sarrays[partition]);
        // we need to do a save here.
        combine_data[partition].unload();
      }
    }

    std::vector<size_t> partitions_vec;
    std::copy(partitions.begin(), partitions.end(),
              std::inserter(partitions_vec, partitions_vec.end()));

    // now, for each element in partitions, if it is not loaded, load it
    parallel_for(partitions_vec.begin(),
                 partitions_vec.end(),
                 [&](size_t part) {
                   logstream(LOG_INFO) << "Loading Combine Partition: " << part << std::endl;
                   combine_data[part].load_if_not_loaded(*combine_sarrays[part]);
                 });
  }

  void compute_const_gather(sframe& edgeframe,
                            edge_partition_address address,
                            size_t central_group,
                            edge_direction edgedir,
                            const_gather_function_type& gather) {
    auto reader = edgeframe.get_reader();
    size_t row_start = 0;
    size_t row_end = reader->num_rows();
    size_t srcid_column = edgeframe.column_index(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = edgeframe.column_index(sgraph::DST_COLUMN_NAME);

    vertex_partition_address src_address = address.get_src_vertex_partition();
    vertex_partition_address dst_address = address.get_dst_vertex_partition();
    while (row_start < row_end) {
      size_t nrows = std::min<size_t>(1024, row_end - row_start);
      std::vector<std::vector<flexible_type> > all_edgedata;
      reader->read_rows(row_start, row_start + nrows, all_edgedata);
      for (const auto& edgedata : all_edgedata) {
        // ok. edges here go from address.src_group to address.dst_group
        // we are gathering into target_central_group
        // it is an in edge if the dst group is the target vertex group
        size_t srcid = edgedata[srcid_column];
        size_t dstid = edgedata[dstid_column];

        if (edgedir == edge_direction::IN_EDGE ||
            edgedir == edge_direction::ANY_EDGE) {
          DASSERT_EQ(address.dst_group, central_group);
          // acquire lock on the combine target
          size_t vertexhash = hash64_combine(hash64(dst_address.partition), hash64(dstid));
          std::unique_lock<turi::mutex> guard(lock_array[vertexhash % LOCK_ARRAY_SIZE]);
          // perform the gather
          // recall vertex_data[group][partition][row]
          gather(vertex_data[dst_address.group][dst_address.partition][dstid],
                 edgedata,
                 vertex_data[src_address.group][src_address.partition][srcid],
                 edge_direction::IN_EDGE,
                 combine_data[dst_address.partition][dstid]);
        }
        if (edgedir == edge_direction::OUT_EDGE || edgedir == edge_direction::ANY_EDGE) {
          DASSERT_EQ(address.src_group, central_group);
          // acquire lock on the combine target
          size_t vertexhash = hash64_combine(hash64(src_address.partition), hash64(srcid));
          std::unique_lock<turi::mutex> guard(lock_array[vertexhash % LOCK_ARRAY_SIZE]);
          // perform the gather
          // recall vertex_data[group][partition][row]
          gather(vertex_data[src_address.group][src_address.partition][srcid],
                 edgedata,
                 vertex_data[dst_address.group][dst_address.partition][dstid],
                 edge_direction::OUT_EDGE,
                 combine_data[src_address.partition][srcid]);
        }
      }
      row_start += nrows;
    }
  }

  std::shared_ptr<sarray<T>> compute_edge_map(sframe& edgeframe,
                                              edge_partition_address address,
                                              const_edge_map_function_type map_fn,
                                              flex_type_enum ret_type) {
    std::shared_ptr<sarray<T>> ret = std::make_shared<sarray<T>>();
    ret->open_for_write(1);
    ret->set_type(ret_type);
    auto out = ret->get_output_iterator(0);

    auto reader = edgeframe.get_reader();
    size_t row_start = 0;
    size_t row_end = reader->num_rows();
    size_t srcid_column = edgeframe.column_index(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = edgeframe.column_index(sgraph::DST_COLUMN_NAME);

    vertex_partition_address src_address = address.get_src_vertex_partition();
    vertex_partition_address dst_address = address.get_dst_vertex_partition();
    while (row_start < row_end) {
      size_t nrows = std::min<size_t>(1024, row_end - row_start);
      std::vector<std::vector<flexible_type> > all_edgedata;
      reader->read_rows(row_start, row_start + nrows, all_edgedata);
      for (auto& edgedata : all_edgedata) {
        size_t srcid = edgedata[srcid_column];
        size_t dstid = edgedata[dstid_column];
        *out = map_fn(vertex_data[src_address.group][src_address.partition][srcid],
                      edgedata,
                      vertex_data[dst_address.group][dst_address.partition][dstid]);
        ++out;
      }
      row_start += nrows;
    }
    ret->close();
    return ret;
  }
}; // end sgraph_engine

} // end sgraph_compute

/// \}
} // end turicreate
#endif
