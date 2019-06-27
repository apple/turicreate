/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sgraph_data/sgraph_fast_triple_apply.hpp>
#include <core/storage/sgraph_data/sgraph_constants.hpp>
#include <core/storage/sgraph_data/hilbert_parallel_for.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi {
namespace sgraph_compute {

namespace {
  /**
   * Field information about a vertex or edge data field.
   */
  struct field_info {
    size_t field_id; // the index of field in the original sgraph
    size_t local_field_id; // the index of field in the pruned sgraph
    std::string name;
    flex_type_enum type;
    bool is_mutable;
  };

  /**
   * This class impelements the main control flow for iterating edges in
   * paratitions in parallel using triple_apply objects inheriting from
   * \ref edge_visitor_interfaces.
   */
  class fast_triple_apply_impl {
   public:
    fast_triple_apply_impl(sgraph& g,
                      const std::vector<std::string>& edge_fields,
                      const std::vector<std::string>& mutated_edge_fields)
        : m_graph(g) {
          init(edge_fields, mutated_edge_fields);
        }

    template<typename EdgeVisitor>
    void run(EdgeVisitor visitor);

   private:
    /**
     * Initialize edge field information
     */
    void init(const std::vector<std::string>& edge_fields,
              const std::vector<std::string>& mutated_edge_fields);

    /**
     * Perform the triple apply function on one partition. If \ref muateted_edge_fields
     * is not empty, the edge data sframe will be updated at the end of the call.
     */
    template<typename EdgeVisitor>
    void do_work_on_edge_partition(sframe edge_sframe_compute,
                                   sgraph::edge_partition_address partition_address,
                                   EdgeVisitor visitor);

   private:
    sgraph& m_graph;

    std::vector<field_info> m_edge_fields_info;
  };

  void fast_triple_apply_impl::init(
      const std::vector<std::string>& edge_fields,
      const std::vector<std::string>& mutated_edge_fields) {

    const auto& all_edge_fields = m_graph.get_edge_fields();

    // validate input edge fields
    for (auto& f: edge_fields) {
      if (std::find(all_edge_fields.begin(),
                    all_edge_fields.end(), f) == all_edge_fields.end()) {
        log_and_throw(std::string("Cannot find edge field: ") + f);
      }
    }

    // validate mutated edge fields
    for (auto& f: mutated_edge_fields) {
      if (std::find(edge_fields.begin(),
                    edge_fields.end(), f) == edge_fields.end()) {
        log_and_throw(std::string("Mutated edge field \"" + f + "\" must be inlucded in all edge fields."));
      }
      if (f == sgraph::SRC_COLUMN_NAME || f == sgraph::DST_COLUMN_NAME) {
        log_and_throw(std::string("Id column cannot be mutable: ") + f);
      }
    }

    const auto& all_edge_field_types = m_graph.get_edge_field_types();

    m_edge_fields_info.clear();

    // Store the required edge fields internally as
    // {field_id, local_field_id, field_name, field_type, and mutable} tuples.
    //
    // Begin with SRC_ID_COLUMN, and DST_ID_COLUMN
    flex_type_enum id_type = m_graph.vertex_id_type();
    size_t srcid_column = m_graph.get_edge_field_id(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = m_graph.get_edge_field_id(sgraph::DST_COLUMN_NAME);
    m_edge_fields_info.push_back({srcid_column, 0, sgraph::SRC_COLUMN_NAME, id_type, false});
    m_edge_fields_info.push_back({dstid_column, 1, sgraph::DST_COLUMN_NAME, id_type, false});

    // Fill in the rest of the fields
    size_t local_field_id = 2;
    for (auto& field : edge_fields) {
      size_t fid = m_graph.get_edge_field_id(field);

      // skip the id columns since we've already done that
      if (fid == srcid_column || fid == dstid_column)
        continue;

      flex_type_enum ftype = all_edge_field_types[fid];
      if (std::find(mutated_edge_fields.begin(),
                    mutated_edge_fields.end(), field) != mutated_edge_fields.end()) {
        m_edge_fields_info.push_back({fid, local_field_id, field, ftype, true});
      } else {
        m_edge_fields_info.push_back({fid, local_field_id, field, ftype, false});
      }
      ++local_field_id;
    }
  }

  /**
   * Perform the triple apply function on one partition.
   */
  template<typename EdgeVisitor>
  void fast_triple_apply_impl::do_work_on_edge_partition(sframe edgeframe_compute,
                                                    sgraph::edge_partition_address partition_address,
                                                    EdgeVisitor visitor) {
    timer mytimer;
    // load source and target vertex partition in memory.
    size_t src_partition = partition_address.get_src_vertex_partition().partition;
    size_t dst_partition = partition_address.get_dst_vertex_partition().partition;

    logstream(LOG_INFO) << "Do work on partition "
                        << partition_address.partition1
                        << ", " << partition_address.partition2
                        << "\nNumber of vertices: " << m_graph.vertex_partition(src_partition).size()
                        << ", " << m_graph.vertex_partition(dst_partition).size()
                        << "\nNumber of edges: " << edgeframe_compute.size() << std::endl;

    // initialize the edge visitor
    mytimer.start();
    visitor.init(m_graph, m_edge_fields_info, src_partition, dst_partition);

    auto reader = edgeframe_compute.get_reader();

    in_parallel([&](size_t threadid, size_t nthreads) {
      size_t row_start = reader->num_rows() * threadid / nthreads;
      size_t row_end = reader->num_rows() * (threadid + 1) / nthreads;
      if (threadid == nthreads - 1) row_end = reader->num_rows();
      sframe_rows batch_edgedata;
      while (row_start < row_end) {
        size_t nrows = 0;
        nrows = std::min<size_t>(SGRAPH_TRIPLE_APPLY_EDGE_BATCH_SIZE, row_end - row_start);
        reader->read_rows(row_start, row_start + nrows, batch_edgedata);
        visitor.visit_edges(batch_edgedata, threadid);
        row_start += nrows;
      }
    });

    // finalize the visitor
    visitor.finalize();

    logstream(LOG_INFO) << "Finish working on partition "
                        << partition_address.partition1
                        << ", " << partition_address.partition2
                        << " in " << mytimer.current_time() <<  " secs" << std::endl;
  }

  template<typename EdgeVisitor>
  void fast_triple_apply_impl::run(EdgeVisitor edge_visitor) {

    std::function<void(std::vector<std::pair<size_t, size_t>>)>
      preamble_fn = [](std::vector<std::pair<size_t, size_t>> coordinates){ };

    std::vector<std::string> edge_columns_compute;
    for (const auto& field: m_edge_fields_info)
      edge_columns_compute.push_back(field.name);

    size_t nparts = m_graph.get_num_partitions() * m_graph.get_num_partitions();
    for (size_t i = 0;i < nparts; ++i) {
      std::pair<size_t, size_t> coordinate = hilbert_index_to_coordinate(i, nparts);
      sgraph::edge_partition_address partition_address(0, 0, coordinate.first, coordinate.second);
      sframe sf = m_graph.edge_partition(partition_address);
      if (sf.num_rows() > 0) {
        sf = sf.select_columns(edge_columns_compute);
        do_work_on_edge_partition(sf, partition_address, edge_visitor);
      }
    }
  }


/**************************************************************************/
/*                                                                        */
/*                     Implementation of triple apply                     */
/*                                                                        */
/**************************************************************************/

  /**
   * Visit the edges one at a time,
   * creating an \ref edge_scope object and apply a user defined function
   * on the scope. The user function can modify vertex or edge
   * data inplace, locking on vertex data is done by the user defined function.
   */
  class single_edge_triple_apply_visitor {
   public:
    /**
     * Constructor.
     * \param apply_fn the user defined apply function of type void(edge_scope&).
     * \param lock_array array of locks to protect con-current access to vertex data.
     * \param srcid_column the column id of the source id field in edge data.
     * \param dstid_column  the column id of the target id field in edge data.
     */
    single_edge_triple_apply_visitor(fast_triple_apply_fn_type apply_fn) :
      apply_fn(apply_fn) { }

    /**
     * Set the source and target vertex partition.
     * Prepare an sframe to store the modfied edge data.
     */
    void init(sgraph& g,
        const std::vector<field_info>& edge_fields_info,
        size_t _src_partition, size_t _dst_partition) {

      // Store the edge sframe pointer.
      src_partition = _src_partition;
      dst_partition = _dst_partition;
      edge_data_ptr = &(g.edge_partition(src_partition, dst_partition));

      // Initialize the sframe storing updated edge data.
      m_mutating_edge_data =
          std::find_if(edge_fields_info.begin(),
                       edge_fields_info.end(),
                       [](const field_info& f) {
                         return f.is_mutable;
                       }) != edge_fields_info.end();

      if (m_mutating_edge_data) {
        std::vector<std::string> field_names;
        std::vector<flex_type_enum> field_types;
        for (auto& finfo : edge_fields_info) {
          if (finfo.is_mutable) {
            field_names.push_back(finfo.name);
            field_types.push_back(finfo.type);
            m_mutated_edge_field_ids.push_back(finfo.local_field_id);
          }
        }
        m_mutated_edges.open_for_write(field_names, field_types, "", thread::cpu_count());
        for (size_t i = 0; i < thread::cpu_count(); ++i) {
          m_mutated_edge_data_writer.push_back(m_mutated_edges.get_output_iterator(i));
        }
      }
    }

    void visit_edges(sframe_rows& edgedata, size_t thread_id) {
      // preallocate vector to store the mutated edge data.
      size_t num_mutated_fields = m_mutated_edge_field_ids.size();

      std::vector<flexible_type> mutated_edge_data(num_mutated_fields);

      std::vector<flexible_type> edata(edgedata.num_columns());
      for (auto& row : edgedata) {

        for (size_t i = 0;i < row.size(); ++i) edata[i] = row[i];

        size_t srcid = edata[0];
        size_t dstid = edata[1];

        // the edge scope contains reference to source, target vertex data,
        // edge data, and the locks associcated to the vertex data.
        fast_edge_scope scope({src_partition, srcid},
                              {dst_partition, dstid},
                              &edata);

        // apply the user defined triple_apply_fn
        apply_fn(scope);

        // write the mutated edge data to the a sframe, whose columns will
        // replace the sframe in the edge partition on finalize call.
        if (m_mutating_edge_data) {
          for (size_t i = 0; i < num_mutated_fields; ++i) {
            mutated_edge_data[i] = edata[m_mutated_edge_field_ids[i]];
          }
          *m_mutated_edge_data_writer[thread_id] = mutated_edge_data;
        }
      }
    }

    /**
     * Replace the edge partition sframe with the modified edge data.
     * Modified vertex data is taken care by the \ref triple_apply_impl
     * on \ref unload_vertex_partitions.
     */
    void finalize() {
      if (m_mutating_edge_data) {
        m_mutated_edges.close();
        for (size_t i  = 0; i < m_mutated_edge_field_ids.size(); ++i) {
          *edge_data_ptr = edge_data_ptr->replace_column(
              m_mutated_edges.select_column(i),
              m_mutated_edges.column_name(i));
        }
      }
    }

  private:
    sframe* edge_data_ptr;

    bool m_mutating_edge_data;

    // sframe storing the mutated edge data.
    sframe m_mutated_edges;

    // output iterator of m_mutated_edges
    std::vector<sframe::iterator> m_mutated_edge_data_writer;

    // The field id of the mutated edge fields
    std::vector<size_t> m_mutated_edge_field_ids;

    // source vertex partition id
    size_t src_partition;
    // target vertex partition id
    size_t dst_partition;

    // user defined triple apply function
    fast_triple_apply_fn_type apply_fn;
  };
} // end of empty namespace


  /**
   * The actual triple apply API.
   */
  void fast_triple_apply(sgraph& g, fast_triple_apply_fn_type apply_fn,
                    const std::vector<std::string>& edge_fields,
                    const std::vector<std::string>& mutated_edge_fields) {

    fast_triple_apply_impl compute(g, edge_fields, mutated_edge_fields);
    single_edge_triple_apply_visitor visitor(apply_fn);
    compute.run(visitor);
  }
} // end of sgraph_compute
} // end of grahlab
