/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sgraph_data/sgraph_triple_apply.hpp>
#include <core/storage/sgraph_data/hilbert_parallel_for.hpp>
#include <core/storage/sgraph_data/sgraph_constants.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/system/lambda/graph_lambda_interface.hpp>
#include <core/system/lambda/graph_pylambda_master.hpp>
#include <core/system/lambda/lambda_utils.hpp>

namespace turi {
namespace sgraph_compute {

namespace {
  /**
   * Field information about a vertex or edge data field.
   */
  struct field_info {
    size_t id;
    std::string name;
    flex_type_enum type;
  };

  /**
   * An edge_visitor defines the procedure of how to process
   * all the edges (and the associated vertices) in a partition of a \ref sgraph.
   * The edges are processed in batch mode.
   *
   * The state of an edge_visitor transit from:
   * load_partition() -> [while(has_edge) visit_edges(next_K_edges)] -> finalize()
   *
   * \ref load_partition initailze the visitor with the graph,
   * the edge parittion ids and the associated in-memory vertex data blocks.
   *
   * The in-memory vertex block is writable, and therefore, locking
   * must be taken care of by the visitor as well.
   *
   * load_partition() in general should only read vertices, and a barrier
   * is followed to make sure initial reads are completed.
   *
   * \ref visit_edges will peform actions on a stream of edges, which involves
   * reading and writing vertex data. Again, care must be taken to lock
   * the in-memory vertices.
   *
   * \ref finalized will be called after all the edges in the partition
   * is consumed.
   */
  class edge_visitor_interface {
   public:
    /**
     * Initialize the visitor state with the graph, edge partition ids,
     * and the associated in-memory vertex blocks.
     *
     * \param g the target sgraph to be visited
     * \param source_vertex_block source vertex data loaded in memory.
     * \param source_vertex_block target vertex data loaded in memory.
     * \param muated_vertex_fields  vertex fields to be mutated.
     * \param muated_vertex_fields  edge fields to be mutated.
     * \param src_partition the edge source partition id.
     * \param src_partition the edge target partition id.
     */
    virtual void load_partition(
        sgraph& g,
        vertex_block<sframe>& source_vertex_block,
        vertex_block<sframe>& target_vertex_block,
        const std::vector<field_info>& mutated_vertex_fields,
        const std::vector<field_info>& mutated_edge_fields,
        size_t src_partition, size_t dst_partition) = 0;

    /**
     * Called on a stream of edges data vectors of possibly variable sizes,
     * until all edges in the partition have been exhausted. Each edge will be
     * visit exactly once.
     */
    virtual void visit_edges(std::vector<edge_data>& edgedata) = 0;

    /**
     * Called when all edges from the partition have been visited.
     */
    virtual void finalize () = 0;

    virtual ~edge_visitor_interface() {};
  };


  /**
   * This class impelements the main control flow for iterating edges in
   * paratitions in parallel using triple_apply objects inheriting from
   * \ref edge_visitor_interfaces.
   */
  class triple_apply_impl {
   public:
    triple_apply_impl(sgraph& g,
                      const std::vector<std::string>& mutated_vertex_fields,
                      const std::vector<std::string>& mutated_edge_fields,
                      bool requires_vertex_id = true)
        : m_graph(g), m_requires_vertex_id(requires_vertex_id) {
          init_data_structures(mutated_vertex_fields, mutated_edge_fields);
        }

    template<typename EdgeVisitor>
    void run(EdgeVisitor edge_visitor);

   private:
    void init_data_structures(const std::vector<std::string>& mutated_vertex_fields,
                              const std::vector<std::string>& mutated_edge_fields);

    /**
     * This function will load all graph vertex blocks with the input_vetex_fields.
     */
    void load_graph_vertex_blocks(const std::set<vertex_partition_address>& vertex_address);

    /**
     * This function will unload all graph vertex blocks and update the columns in the
     * mutated_vertex_fields.
     */
    void unload_graph_vertex_blocks(const std::set<vertex_partition_address>& vertex_address);

    /**
     * Perform the triple apply function on one partition. If \ref muateted_edge_fields
     * is not empty, the edge data sframe will be updated at the end of the call.
     */
    template<typename EdgeVisitor>
    void do_work_on_edge_partition(sframe& edge_partition,
                                   edge_partition_address partition_address,
                                   EdgeVisitor visitor);

   private:
    sgraph& m_graph;

    // storing vertex blocks associated with the working edge partitions.
    std::vector<vertex_block<sframe>> m_vertex_data;

    // storing vertex partition addresses that are currently loaded in memory.
    std::set<vertex_partition_address> m_loaded_vertex_block_address;

    std::vector<field_info> m_mutated_vertex_fields;
    std::vector<field_info> m_mutated_edge_fields;

    bool m_requires_vertex_id = true;
  };

  template<typename EdgeVisitor>
  void triple_apply_impl::run(EdgeVisitor edge_visitor) {
    // preamble function that load the vertex blocks associated with the
    // edge partitions to be visited.
    std::function<void(std::vector<std::pair<size_t, size_t>>)>
      preamble_fn = [&](std::vector<std::pair<size_t, size_t>> coordinates) {
        std::set<vertex_partition_address> vertex_partition_to_load;
        std::set<vertex_partition_address> vertex_partition_to_unload;

        for (const auto& coordinate: coordinates) {
          size_t srcid = coordinate.first;
          size_t dstid = coordinate.second;
          edge_partition_address edge_partition(0, 0, srcid, dstid);
          vertex_partition_to_load.insert(vertex_partition_address(0, srcid));
          vertex_partition_to_load.insert(vertex_partition_address(0, dstid));
        }

        for (const auto& address: m_loaded_vertex_block_address) {
          if (vertex_partition_to_load.find(address) == vertex_partition_to_load.end()) {
            vertex_partition_to_unload.insert(address);
          }
        }

        unload_graph_vertex_blocks(vertex_partition_to_unload);
        load_graph_vertex_blocks(vertex_partition_to_load);

        m_loaded_vertex_block_address = vertex_partition_to_load;

        std::stringstream message_ss;
        message_ss << "Vertex partitions in memory: ";
        for (const auto& coord: vertex_partition_to_load)
          message_ss << coord.partition << " ";
        logstream(LOG_INFO) << message_ss.str() << std::endl;
      };

    hilbert_blocked_parallel_for(
        m_graph.get_num_partitions(),
        preamble_fn,
        [&](std::pair<size_t, size_t> coordinate) {
          edge_partition_address partition_address(0, 0, coordinate.first, coordinate.second);
          sframe& sf = m_graph.edge_partition(partition_address);
          do_work_on_edge_partition(sf, partition_address, edge_visitor);
        }
    );
    // unload and commit the remaining vertex block in the memory.
    preamble_fn({});
  }

  void triple_apply_impl::init_data_structures(
      const std::vector<std::string>& mutated_vertex_fields,
      const std::vector<std::string>& mutated_edge_fields) {

    // validate the mutated fields
    const auto& all_vertex_fields = m_graph.get_vertex_fields();
    const auto& all_edge_fields = m_graph.get_edge_fields();

    std::set<std::string> vertex_field_set(all_vertex_fields.begin(),
                                           all_vertex_fields.end());
    std::set<std::string> edge_field_set(all_edge_fields.begin(),
                                         all_edge_fields.end());
    for (auto& f: mutated_vertex_fields) {
      ASSERT_MSG(vertex_field_set.count(f) > 0, ("Cannot find vertex field: " + f).c_str());
    }
    for (auto& f: mutated_edge_fields) {
      ASSERT_MSG(edge_field_set.count(f) > 0, ("Cannot find edge field: " + f).c_str());
    }

    m_vertex_data.resize(m_graph.get_num_partitions());

    const auto& all_vertex_field_types = m_graph.get_vertex_field_types();
    const auto& all_edge_field_types = m_graph.get_edge_field_types();

    m_mutated_vertex_fields.clear();
    m_mutated_edge_fields.clear();

    // store the mutated fields internally as {field_id, field_name, and field_type} tuples.
    for (auto& field : mutated_vertex_fields) {
      size_t fid = m_graph.get_vertex_field_id(field);
      flex_type_enum ftype = all_vertex_field_types[fid];
      m_mutated_vertex_fields.push_back({fid, field, ftype});
    }

    for (auto& field : mutated_edge_fields) {
      size_t fid = m_graph.get_edge_field_id(field);
      flex_type_enum ftype = all_edge_field_types[fid];
      m_mutated_edge_fields.push_back({fid, field, ftype});
    }
  }

  /**
   * This function will load all graph vertex blocks with the input_vetex_fields.
   */
  void triple_apply_impl::load_graph_vertex_blocks(const std::set<vertex_partition_address>& vertex_address) {
    std::vector<vertex_partition_address> address_vec(vertex_address.begin(), vertex_address.end());
    parallel_for (0, address_vec.size(), [&](size_t i) {
      vertex_partition_address address = address_vec[i];
      if (!m_vertex_data[address.partition].is_loaded()) {
        auto vertex_data_sf = m_graph.vertex_partition(address);
        if (m_requires_vertex_id) {
          m_vertex_data[address.partition].load(vertex_data_sf);
        } else {
          // Exclude the vid column from loading, and fill the id column
          // in the loaded vertex data with undefined values.
          size_t id_column_index = vertex_data_sf.column_index(sgraph::VID_COLUMN_NAME);
          vertex_data_sf = vertex_data_sf.remove_column(id_column_index);
          m_vertex_data[address.partition].load(vertex_data_sf);
          for (auto& entry: m_vertex_data[address.partition].m_vertices) {
            entry.insert(entry.begin() + id_column_index, FLEX_UNDEFINED);
          }
        }
      }
    });
  }

  /**
   * This function will unload all graph vertex blocks and update the columns in the
   * mutated_vertex_fields.
   */
  void triple_apply_impl::unload_graph_vertex_blocks(const std::set<vertex_partition_address>& vertex_address) {
    std::vector<vertex_partition_address> address_vec(vertex_address.begin(), vertex_address.end());

    if (!m_mutated_vertex_fields.empty()) {
      std::vector<std::string> mutated_field_names;
      std::vector<flex_type_enum> mutated_field_types;
      std::vector<size_t> mutated_field_index;
      for (auto& finfo : m_mutated_vertex_fields) {
        mutated_field_index.push_back(finfo.id);
        mutated_field_names.push_back(finfo.name);
        mutated_field_types.push_back(finfo.type);
      }
      parallel_for (0, address_vec.size(), [&](size_t i) {
      // for (size_t i = 0; i < address_vec.size(); ++i) {
        vertex_partition_address address = address_vec[i];
        sframe& old_vertex_data = m_graph.vertex_partition(address);
        // save the updated vertex fields
        sframe updated_vertex_data;
        updated_vertex_data.open_for_write(mutated_field_names, mutated_field_types, "", 1);
        m_vertex_data[address.partition].flush(updated_vertex_data, mutated_field_index);
        // update the vertex sframe
        for (size_t i = 0; i < m_mutated_vertex_fields.size(); ++i) {
          std::string column_name = updated_vertex_data.column_name(i);
          old_vertex_data = old_vertex_data.replace_column(updated_vertex_data.select_column(i), column_name);
        }
        m_vertex_data[address.partition].set_modified_flag();
      // }
      });
    }

    // Unload the vertex blocks
    parallel_for(0, address_vec.size(), [&](size_t i) {
      m_vertex_data[address_vec[i].partition].unload();
    });
  }

  /**
   * Perform the triple apply function on one partition.
   */
  template<typename EdgeVisitor>
  void triple_apply_impl::do_work_on_edge_partition(sframe& edgeframe,
                                                    edge_partition_address partition_address,
                                                    EdgeVisitor visitor) {
    timer mytimer;
    // load source and target vertex partition in memory.
    size_t src_partition = partition_address.get_src_vertex_partition().partition;
    size_t dst_partition = partition_address.get_dst_vertex_partition().partition;
    vertex_block<sframe>& source_block = m_vertex_data[src_partition];
    vertex_block<sframe>& target_block = m_vertex_data[dst_partition];
    DASSERT_TRUE(source_block.is_loaded());
    DASSERT_TRUE(target_block.is_loaded());

    logstream(LOG_INFO) << "Do work on partition "
                        << partition_address.partition1
                        << ", " << partition_address.partition2
                        << "\nNumber of vertices: " << m_graph.vertex_partition(src_partition).size()
                        << ", " << m_graph.vertex_partition(dst_partition).size()
                        << "\nNumber of edges: " << edgeframe.size() << std::endl;

    // initialize the edge visitor
    mytimer.start();
    visitor.load_partition(m_graph, source_block, target_block,
                           m_mutated_vertex_fields,
                           m_mutated_edge_fields,
                           src_partition, dst_partition);
    logstream(LOG_INFO) << "Setup visitor in " << mytimer.current_time() <<  " secs" << std::endl;

    // barrier wait for all visitors complete the loading phase
    cancellable_barrier barrier(thread::cpu_count());

    mytimer.start();
    auto reader = edgeframe.get_reader();
    size_t row_start = 0;
    size_t row_end = reader->num_rows();
    // feed batch of edges to the edge visitor
    std::vector<std::vector<flexible_type> > all_edgedata;
    while (row_start < row_end) {
      size_t nrows = std::min<size_t>(SGRAPH_TRIPLE_APPLY_EDGE_BATCH_SIZE, row_end - row_start);
      reader->read_rows(row_start, row_start + nrows, all_edgedata);
      visitor.visit_edges(all_edgedata);
      row_start += nrows;
    }
    logstream(LOG_INFO) << "Finish working on partition "
                        << partition_address.partition1
                        << ", " << partition_address.partition2
                        << " in " << mytimer.current_time() <<  " secs" << std::endl;

    mytimer.start();
    // finalize the edge visitor
    visitor.finalize();
    logstream(LOG_INFO) << "Finalize working on partition "
                        << partition_address.partition1
                        << ", " << partition_address.partition2
                        << " in " << mytimer.current_time() <<  " secs" << std::endl;
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
  class single_edge_triple_apply_visitor : public edge_visitor_interface {
   public:
    typedef turi::mutex mutex_type;

    /**
     * Constructor.
     * \param apply_fn the user defined apply function of type void(edge_scope&).
     * \param lock_array array of locks to protect con-current access to vertex data.
     * \param srcid_column the column id of the source id field in edge data.
     * \param dstid_column  the column id of the target id field in edge data.
     */
    single_edge_triple_apply_visitor(
        triple_apply_fn_type apply_fn,
        std::vector<mutex_type>& lock_array,
        size_t srcid_column, size_t dstid_column) :
      apply_fn(apply_fn), lock_array(lock_array),
      srcid_column(srcid_column), dstid_column(dstid_column) { }

    /**
     * Set the source and target vertex partition.
     * Prepare an sframe to store the modfied edge data.
     */
    void load_partition(
        sgraph& g,
        vertex_block<sframe>& source_vertex_block,
        vertex_block<sframe>& target_vertex_block,
        const std::vector<field_info>& _mutated_vertex_fields,
        const std::vector<field_info>& _mutated_edge_fields,
        size_t _src_partition, size_t _dst_partition) {

      // Store the vertex block pointers.
      source_vertex_data = &source_vertex_block;
      target_vertex_data = &target_vertex_block;

      // Store the edge sframe pointer.
      src_partition = _src_partition;
      dst_partition = _dst_partition;
      edge_data_ptr = &(g.edge_partition(src_partition, dst_partition));

      // Initialize the sframe storing updated edge data.
      m_mutating_edge_data = !_mutated_edge_fields.empty();
      if (m_mutating_edge_data) {
        std::vector<std::string> field_names;
        std::vector<flex_type_enum> field_types;
        for (auto& finfo : _mutated_edge_fields) {
          field_names.push_back(finfo.name);
          field_types.push_back(finfo.type);
          m_mutated_edge_field_ids.push_back(finfo.id);
        }
        m_mutated_edges.open_for_write(field_names, field_types, "", 1);
        m_mutated_edge_data_writer = m_mutated_edges.get_output_iterator(0);
      }
    }

    void visit_edges(std::vector<edge_data>& edgedata) {
      // preallocate vector to store the mutated edge data.
      std::vector<flexible_type> edge_data_buffer;
      size_t num_mutated_fields = m_mutated_edge_field_ids.size();

      for (auto& edata: edgedata) {
        size_t srcid = edata[srcid_column];
        size_t dstid = edata[dstid_column];

        // preparing the locks to the source and target vertices.
        // To prevent deadlocking, lock ordering is determined by the hash value
        // Always lock lock_0 before lock_1 (the user defined triple_apply_fn is repsonsible).
        size_t src_hash = hash64_combine(hash64(src_partition), hash64(srcid)) % lock_array.size();
        size_t dst_hash = hash64_combine(hash64(dst_partition), hash64(dstid)) % lock_array.size();
        mutex_type *lock_0, *lock_1;
        if (src_hash == dst_hash) {
          lock_0 = lock_1 = &(lock_array[src_hash]);
        } else if (src_hash < dst_hash) {
          lock_0 = &lock_array[src_hash];
          lock_1 = &lock_array[dst_hash];
        } else {
          lock_0 = &lock_array[dst_hash];
          lock_1 = &lock_array[src_hash];
        }

        // the edge scope contains reference to source, target vertex data,
        // edge data, and the locks associcated to the vertex data.
        edge_scope scope(&(*source_vertex_data)[srcid], &(*target_vertex_data)[dstid],
            &edata, lock_0, lock_1);

        // apply the user defined triple_apply_fn
        apply_fn(scope);

        // write the mutated edge data to the a sframe, whose columns will
        // replace the sframe in the edge partition on finalize call.
        if (m_mutating_edge_data) {
          edge_data_buffer.resize(num_mutated_fields);
          for (size_t i = 0; i < num_mutated_fields; ++i) {
            edge_data_buffer[i] = edata[m_mutated_edge_field_ids[i]];
          }
          *m_mutated_edge_data_writer = std::move(edge_data_buffer);
          ++m_mutated_edge_data_writer;
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
    vertex_block<sframe>* source_vertex_data;
    vertex_block<sframe>* target_vertex_data;
    sframe* edge_data_ptr;

    bool m_mutating_edge_data;
    // sframe storing the mutated edge data.
    sframe m_mutated_edges;
    // output iterator of m_mutated_edges
    sframe::iterator m_mutated_edge_data_writer;
    // id of the edge fields to be modified.
    std::vector<size_t> m_mutated_edge_field_ids;

    // source vertex partition id
    size_t src_partition;
    // target vertex partition id
    size_t dst_partition;

    // user defined triple apply function
    triple_apply_fn_type apply_fn;

    // array of locks for guarding concurrent access to vertex data.
    std::vector<mutex_type>& lock_array;
    size_t srcid_column;
    size_t dstid_column;
  };



/**************************************************************************/
/*                                                                        */
/*                     Implementation of batch triple apply               */
/*                                                                        */
/**************************************************************************/
  /**
   * Batch edge triple_apply visitor.
   *
   * Processing edge_data in batch mode.
   *
   * To protect concurrent access of vertex data, we try to lock as many edges
   * (really the vertices associated with the edges)
   * as we can, and apply the user defined function on the locked edges.
   *
   * The batch locking will be less inefficient for c++ lambda function than the
   * single edge visitor. However, for python lambda that is evaluated in separate processes,
   * batching the edge_data is useful to increases the throughput.
   *
   * Edges that are not locked succesfully will be postponed and merge with the next batch.
   *
   * \ref finalize() make sure all edges are processed.
   */
  class batch_edge_triple_apply_visitor: public edge_visitor_interface {
    typedef turi::recursive_mutex mutex_type;

   public:
    /**
     * Constructor
     */
    batch_edge_triple_apply_visitor(
      std::vector<mutex_type>& lock_array,
      size_t srcid_column, size_t dstid_column) :
      lock_array(lock_array),
      m_srcid_column(srcid_column),
      m_dstid_column(dstid_column) { }

    virtual ~batch_edge_triple_apply_visitor() override { }

    /**
     * Sets the batch apply function.
     */
    void set_apply_fn(batch_triple_apply_fn_type _apply_fn) {
      apply_fn = _apply_fn;
    }

    /**
     * Set the source and target vertex partition.
     * Prepare an sframe to store the modfied edge data.
     */
    virtual void load_partition(
        sgraph& g,
        vertex_block<sframe>& source_vertex_block,
        vertex_block<sframe>& target_vertex_block,
        const std::vector<field_info>& mutated_vertex_fields,
        const std::vector<field_info>& mutated_edge_fields,
        size_t _src_partition, size_t _dst_partition) override {
      source_vertex_data = &source_vertex_block;
      target_vertex_data = &target_vertex_block;
      edge_data_ptr = &(g.edge_partition(_src_partition, _dst_partition));

      m_src_partition = _src_partition;
      m_dst_partition = _dst_partition;

      // If edge data will be modified, prepare an sframe that stores
      // the modified edge data.
      m_mutating_edges = !mutated_edge_fields.empty();
      if (m_mutating_edges) {
        std::vector<std::string> field_names {sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME};
        std::vector<flex_type_enum> field_types {sgraph::INTERNAL_ID_TYPE, sgraph::INTERNAL_ID_TYPE};

        // The modified edge fields have to include local source and target ids.
        // This is because the ordering which modified is written to the new sframe
        // is different from the ordering of we read the original edges, due to the locking
        // and postponeing of unsucceesfully locked edges.
        m_mutated_edge_field_ids.push_back(g.get_edge_field_id(sgraph::SRC_COLUMN_NAME));
        m_mutated_edge_field_ids.push_back(g.get_edge_field_id(sgraph::DST_COLUMN_NAME));
        for (auto& finfo : mutated_edge_fields) {
          field_names.push_back(finfo.name);
          field_types.push_back(finfo.type);
          m_mutated_edge_field_ids.push_back(finfo.id);
        }
        m_mutated_edges.open_for_write(field_names, field_types, "", 1);
        m_mutated_edge_data_writer = m_mutated_edges.get_output_iterator(0);
      }
    }

    /**
     * Apply user defined batch_apply_function to the successfully locked edges.
     * The rest of the edges will be stored in a local buffer and postpone to
     * be processed in the next visit_edges call.
     */
    void visit_edges(std::vector<edge_data>& edgedata) override {
      add_edge_data(edgedata);
      // try lock as many edges as we can.
      try_optimistic_lock();
      // obtain the locked edges.
      std::vector<edge_scope>& locked_scopes = get_locked_scopes();
      try {
        apply_fn(locked_scopes);
        // writes the modified edges to sframe
        commit_edge_changes(locked_scopes);
      } catch (...) {
        unlock_and_release();
        throw;
      }
      // release the locked edges.
      unlock_and_release();
    }

    virtual void finalize() override {
      DASSERT_LE(m_all_edge_data.size(), 1);
      // finish processing the rest of the edges in the buffer.
      while (!m_all_edge_data.empty() && !m_all_edge_data[0].empty()) {
        try_optimistic_lock();
        std::vector<edge_scope>& locked_scopes = get_locked_scopes();
        try {
          apply_fn(locked_scopes);
          commit_edge_changes(locked_scopes);
        } catch (...) {
          unlock_and_release();
          throw;
        }
        unlock_and_release();
      }

      // replace the modified edge columns in the original edge partition sframe.
      if (m_mutating_edges) {
        m_mutated_edges.close();
        for (size_t i = 0; i < m_mutated_edge_field_ids.size(); ++i) {
          *edge_data_ptr = edge_data_ptr->replace_column(
              m_mutated_edges.select_column(i),
              m_mutated_edges.column_name(i));
        }
      }
    }

    /**
     * Write the changed edge data to the mutated edge data sframe.
     */
    void commit_edge_changes(std::vector<edge_scope>& locked_scopes) {
      if (!m_mutating_edges)
        return;
      DASSERT_TRUE(m_mutated_edges.is_opened_for_write());
      std::vector<flexible_type> edge_data_buffer;
      size_t num_mutated_fields = m_mutated_edge_field_ids.size();
      for (auto& scope: locked_scopes) {
        auto& edata = scope.edge();
        edge_data_buffer.resize(num_mutated_fields);
        for (size_t i = 0; i < num_mutated_fields; ++i) {
          edge_data_buffer[i] = edata[m_mutated_edge_field_ids[i]];
        }
        *m_mutated_edge_data_writer = std::move(edge_data_buffer);
        ++m_mutated_edge_data_writer;
      }
    }

  private:
    /**************************************************************************/
    /*                                                                        */
    /*                       Logic for Optimistic Lock                        */
    /*                                                                        */
    /**************************************************************************/
    // Add a list of edge data to the local buffer.
    void add_edge_data(std::vector<edge_data>& edge_data) {
      m_all_edge_data.push_back(std::move(edge_data));
    }

    // Returns a list of edge scopes from the local buffer
    std::vector<edge_scope>& get_locked_scopes() {
      m_locked_scopes.clear();
      for (const auto& idx_pair: m_locked_edge_idx) {
        edge_data& edata = m_all_edge_data[idx_pair.first][idx_pair.second];
        size_t srcid = edata[m_srcid_column];
        size_t dstid = edata[m_dstid_column];
        m_locked_scopes.push_back (edge_scope(&(*source_vertex_data)[srcid],
                                              &(*target_vertex_data)[dstid],
                                              &edata));
      }
      return m_locked_scopes;
    }

    // Returns a pair of locks associated with the edge data. The returned
    // lock pair has lock ordering of pair.first > pair.second. (lock the first before the second).
    std::pair<mutex_type&, mutex_type&> get_edge_mutex(const edge_data& edata) {
      size_t srcid = edata[m_srcid_column];
      size_t dstid = edata[m_dstid_column];
      size_t src_hash = hash64_combine(hash64(m_src_partition), hash64(srcid)) % lock_array.size();
      size_t dst_hash = hash64_combine(hash64(m_dst_partition), hash64(dstid)) % lock_array.size();
      if (src_hash >= dst_hash) {
        return {lock_array[dst_hash], lock_array[src_hash]};
      } else {
        return {lock_array[src_hash], lock_array[dst_hash]};
      }
    }

    // Try to lock an edge by locking both vertices.
    // Return true on success.
    bool try_lock_edge(const edge_data& edge) {
      auto lock_pair = get_edge_mutex(edge);
      if (lock_pair.first.try_lock()) {
        if (lock_pair.second.try_lock()) {
          return true;
        }
        lock_pair.first.unlock();
      }
      return false;
    }

    // Try to lock as many edges in the local buffer as possible.
    void try_optimistic_lock() {
      DASSERT_TRUE(m_locked_edge_idx.empty());
      DASSERT_TRUE(m_unlocked_edge_idx.empty());
      for (size_t i = 0; i < m_all_edge_data.size(); ++i) {
        for (size_t j = 0; j < m_all_edge_data[i].size(); ++j) {
          if (try_lock_edge(m_all_edge_data[i][j])) {
            m_locked_edge_idx.push_back({i,j}); // index of edges that are locked.
          } else {
            m_unlocked_edge_idx.push_back({i,j}); // index of edges that are not locked.
          }
        }
      }
    }

    // unlock the locked edges and remove from the local buffer.
    void unlock_and_release() {
      // unlock the locked edges
      for (const auto& idx_pair: m_locked_edge_idx) {
        auto lock_pair = get_edge_mutex(m_all_edge_data[idx_pair.first][idx_pair.second]);
        lock_pair.first.unlock();
        lock_pair.second.unlock();
      }

      // add the unlocked edges to swap
      std::vector<std::vector<edge_data>> m_all_edge_data2;
      m_all_edge_data2.push_back({});
      for (const auto& idx_pair: m_unlocked_edge_idx) {
        m_all_edge_data2[0].push_back(std::move(m_all_edge_data[idx_pair.first][idx_pair.second]));
      }
      m_all_edge_data.swap(m_all_edge_data2);

      m_locked_edge_idx.clear();
      m_unlocked_edge_idx.clear();
    };

    // local buffer of all edges to be processed
    std::vector<std::vector<edge_data>> m_all_edge_data;
    std::vector<edge_scope> m_locked_scopes;
    std::vector<std::pair<short, size_t>> m_locked_edge_idx;
    std::vector<std::pair<short, size_t>> m_unlocked_edge_idx;

    std::vector<mutex_type>& lock_array;

    vertex_block<sframe>* source_vertex_data;
    vertex_block<sframe>* target_vertex_data;
    sframe* edge_data_ptr;

    sframe m_mutated_edges;
    sframe::iterator m_mutated_edge_data_writer;

 protected:
    size_t m_src_partition;
    size_t m_dst_partition;
    size_t m_srcid_column;
    size_t m_dstid_column;
    batch_triple_apply_fn_type apply_fn;

    bool m_mutating_edges;
    std::vector<size_t> m_mutated_edge_field_ids;
  }; // end of batch edge triple apply visitor

#ifdef TC_HAS_PYTHON
  /**************************************************************************/
  /*                                                                        */
  /*                 Implementation of lambda triple apply                  */
  /*                                                                        */
  /**************************************************************************/

  /**
   * Batch edge triple_apply visitor with python lambda.
   */
  class lambda_triple_apply_visitor: public batch_edge_triple_apply_visitor {
    typedef turi::recursive_mutex mutex_type;

   public:
    lambda_triple_apply_visitor(const std::string& lambda_str,
                                std::vector<mutex_type>& lock_array,
                                size_t srcid_column, size_t dstid_column)
      : batch_edge_triple_apply_visitor(lock_array, srcid_column, dstid_column), m_lambda_str(lambda_str) {
        m_worker_pool = lambda::graph_pylambda_master::get_instance().get_worker_pool();
        m_evaluator = NULL;
        m_evaluator_guard = NULL;
      }

    // Copy constructor: only copies lambda_str and worker pool, default initialize the rest
    lambda_triple_apply_visitor(const lambda_triple_apply_visitor& other) : batch_edge_triple_apply_visitor(other) {
       m_lambda_str = other.m_lambda_str;
       m_worker_pool = other.m_worker_pool;
       m_evaluator = NULL;
       m_evaluator_guard = NULL;
    }

    ~lambda_triple_apply_visitor() override { }

    /**
     * Overwrite parent's method, because we need to initialize the lambda evaluator
     * with the block formation.
     */
    void load_partition(
        sgraph& g,
        vertex_block<sframe>& source_vertex_block,
        vertex_block<sframe>& target_vertex_block,
        const std::vector<field_info>& mutated_vertex_fields,
        const std::vector<field_info>& mutated_edge_fields,
        size_t _src_partition, size_t _dst_partition) override {

      // call parent function to initialize the internal data structrues.
      batch_edge_triple_apply_visitor::load_partition(
          g, source_vertex_block, target_vertex_block,
          mutated_vertex_fields, mutated_edge_fields,
          _src_partition, _dst_partition);

      for (auto& finfo: mutated_vertex_fields)
        m_mutated_vertex_field_ids.push_back(finfo.id);

      // Initialize the sgraph_synchronize object used for exchanging
      // graph vertex data with lambda workers.
      m_graph_sync.init(g.get_num_partitions());

      // Ask for an evaluator from pylambda_master
      m_evaluator = m_worker_pool->get_worker();
      m_evaluator_guard = m_worker_pool->get_worker_guard(m_evaluator);
      logstream(LOG_INFO) << "Acquire worker " << m_evaluator->id << " on partition " << _src_partition << ", " << _dst_partition << std::endl;

      // Initialize evaluator graph compute with the graph information.
      try {
        m_evaluator->proxy->init(m_lambda_str, g.get_num_partitions(),
                                 g.get_vertex_fields(),
                                 g.get_edge_fields(),
                                 m_srcid_column, m_dstid_column);
      } catch (cppipc::ipcexception e) {
        throw(lambda::reinterpret_comm_failure(e));
      }

      // Load the vertex partitions for remote sgraph sync.
      logstream(LOG_INFO) << "Lambda worker load partition "
                          << m_src_partition << ", " << m_dst_partition << std::endl;
      m_evaluator->proxy->load_vertex_partition(m_src_partition, source_vertex_block.m_vertices);
      if (m_src_partition != m_dst_partition)
        m_evaluator->proxy->load_vertex_partition(m_dst_partition, target_vertex_block.m_vertices);

      // Load the vertex partitions for local sgraph sync.
      m_graph_sync.load_vertex_partition(m_src_partition, source_vertex_block.m_vertices);
      if (m_src_partition != m_dst_partition)
        m_graph_sync.load_vertex_partition(m_dst_partition, target_vertex_block.m_vertices);

      // Set the parent batch apply function.
      DASSERT_TRUE(m_evaluator->proxy->is_loaded(m_src_partition));
      DASSERT_TRUE(m_evaluator->proxy->is_loaded(m_dst_partition));
      set_apply_fn(boost::bind(&lambda_triple_apply_visitor::apply_lambda, this, _1));
    }

    void finalize() override {
      batch_edge_triple_apply_visitor::finalize();
      m_evaluator->proxy->clear();
      m_evaluator_guard.reset();
      m_evaluator.reset();
    }

    /**
     * Call the lambda evaluator to evaluate the given edge scopes. This will be
     * bind to the `apply_fn` in the base class.
     *
     * To do that, we must first keep track of the vertex data that are affected
     * by the edge scopes, synchronize the most recent data with the evaluator.
     *
     * After the evaluator returns, we must also fetch the mutated data
     * from the evaluator and keep our vertex data up to date.
     */
    void apply_lambda(std::vector<edge_scope>& edge_scopes) {
      std::vector<edge_data> all_edge_data;
      // keep track of the vertices touched in this batch of edge_scopes
      std::map<size_t, std::unordered_set<size_t>> vid_set;
      std::unordered_set<size_t>& srcid_set = vid_set[m_src_partition];
      std::unordered_set<size_t>& dstid_set = vid_set[m_dst_partition];

      // extract the edge data from the scopes.
      all_edge_data.reserve(edge_scopes.size());
      for (auto& scope: edge_scopes) {
        edge_data& edata = scope.edge();
        size_t srcid = edata[m_srcid_column];
        size_t dstid = edata[m_dstid_column];
        srcid_set.insert(srcid);
        dstid_set.insert(dstid);
        all_edge_data.push_back(edata);
      }

      DASSERT_TRUE(m_evaluator->proxy->is_loaded(m_src_partition));
      DASSERT_TRUE(m_evaluator->proxy->is_loaded(m_dst_partition));

      // Update the evaluator with the latest vertex data.
      if (!m_mutated_vertex_field_ids.empty()) {
        {
          auto partition_exchange = m_graph_sync.get_vertex_partition_exchange(
              m_src_partition, srcid_set, m_mutated_vertex_field_ids);
          m_evaluator->proxy->update_vertex_partition(partition_exchange);
        }
        if (m_src_partition != m_dst_partition) {
          auto partition_exchange = m_graph_sync.get_vertex_partition_exchange(
              m_dst_partition, dstid_set, m_mutated_vertex_field_ids);
          m_evaluator->proxy->update_vertex_partition(partition_exchange);
        }
      }

      // Update the edge data.
      std::vector<size_t> mutated_edge_field_ids;
      if (m_mutating_edges) {
        // In batch apply setting, the first two mutated edge fields are local src/dst ids.
        // However, because the lambda evaluator use the same pace to visit the edges,
        // we don't need to serialize the src/dst ids.
        DASSERT_GT(m_mutated_edge_field_ids.size(), 2);
        std::copy(m_mutated_edge_field_ids.begin() + 2,
                  m_mutated_edge_field_ids.end(),
                  std::inserter(mutated_edge_field_ids, mutated_edge_field_ids.end()));
      }
      std::vector<sgraph_edge_data> mutated_edge_data;
      try {
        mutated_edge_data = m_evaluator->proxy->eval_triple_apply(all_edge_data, m_src_partition,
                                                                  m_dst_partition, mutated_edge_field_ids);
      } catch (cppipc::ipcexception e) {
        throw(lambda::reinterpret_comm_failure(e));
      }

      DASSERT_EQ(mutated_edge_data.size(), edge_scopes.size());
      for (size_t i = 0 ; i < mutated_edge_data.size(); ++i) {
        for (size_t j = 0; j < mutated_edge_field_ids.size(); ++j) {
          sgraph_edge_data& edata = edge_scopes[i].edge();
          edata[mutated_edge_field_ids[j]] = mutated_edge_data[i][j];
        }
      }

      // Update the graph_sync after m_evaluator has changed the vertices.
      if (!m_mutated_vertex_field_ids.empty()) {
        {
          auto partition_exchange = m_evaluator->proxy->get_vertex_partition_exchange(
              m_src_partition, srcid_set, m_mutated_vertex_field_ids);
          m_graph_sync.update_vertex_partition(partition_exchange);
        }
        if (m_src_partition != m_dst_partition) {
          auto partition_exchange = m_evaluator->proxy->get_vertex_partition_exchange(
              m_dst_partition, dstid_set, m_mutated_vertex_field_ids);
          m_graph_sync.update_vertex_partition(partition_exchange);
        }
      }
    }

   private:
    std::string m_lambda_str;
    std::vector<size_t> m_mutated_vertex_field_ids;

    std::shared_ptr<lambda::worker_pool<lambda::graph_lambda_evaluator_proxy>> m_worker_pool;
    std::unique_ptr<lambda::worker_process<lambda::graph_lambda_evaluator_proxy>> m_evaluator;
    std::shared_ptr<lambda::worker_guard<lambda::graph_lambda_evaluator_proxy>> m_evaluator_guard;

    sgraph_synchronize m_graph_sync;
  }; // end of lambda_triple_apply_edge_visitor

#endif

  }// end of empty namespace

  /**
   * The actual triple apply API.
   */
  void triple_apply(sgraph& g, triple_apply_fn_type apply_fn,
                    const std::vector<std::string>& mutated_vertex_fields,
                    const std::vector<std::string>& mutated_edge_fields,
                    bool requires_vertex_id) {

    triple_apply_impl compute(g, mutated_vertex_fields, mutated_edge_fields, requires_vertex_id);

    std::vector<turi::mutex> lock_array(SGRAPH_TRIPLE_APPLY_LOCK_ARRAY_SIZE);
    size_t srcid_column = g.get_edge_field_id(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = g.get_edge_field_id(sgraph::DST_COLUMN_NAME);

    single_edge_triple_apply_visitor visitor(apply_fn, lock_array, srcid_column, dstid_column);
    compute.run(visitor);
  }

  /**
   * The batch_triple_apply API.
   *
   * Mock the single triple apply using batch_triple_apply implementation.
   * Used for testing only.
   */
  void batch_triple_apply_mock(sgraph& g, triple_apply_fn_type apply_fn,
                               const std::vector<std::string>& mutated_vertex_fields,
                               const std::vector<std::string>& mutated_edge_fields) {
    triple_apply_impl compute(g, mutated_vertex_fields, mutated_edge_fields);
    std::vector<turi::recursive_mutex> lock_array(SGRAPH_BATCH_TRIPLE_APPLY_LOCK_ARRAY_SIZE);
    size_t srcid_column = g.get_edge_field_id(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = g.get_edge_field_id(sgraph::DST_COLUMN_NAME);

    batch_triple_apply_fn_type batch_apply_fn =
      [=] (std::vector<edge_scope>& all_scope) {
        for (auto& scope : all_scope) apply_fn(scope);
      };

    batch_edge_triple_apply_visitor visitor(lock_array, srcid_column, dstid_column);
    visitor.set_apply_fn(batch_apply_fn);
    compute.run(visitor);
  }

#ifdef TC_HAS_PYTHON
  /**
   * The actual triple apply API with python lambda.
   */
  void triple_apply(sgraph& g, const std::string& lambda_str,
                    const std::vector<std::string>& mutated_vertex_fields,
                    const std::vector<std::string>& mutated_edge_fields) {
    triple_apply_impl compute(g, mutated_vertex_fields, mutated_edge_fields);
    std::vector<turi::recursive_mutex> lock_array(SGRAPH_BATCH_TRIPLE_APPLY_LOCK_ARRAY_SIZE);
    size_t srcid_column = g.get_edge_field_id(sgraph::SRC_COLUMN_NAME);
    size_t dstid_column = g.get_edge_field_id(sgraph::DST_COLUMN_NAME);

    lambda_triple_apply_visitor visitor(lambda_str, lock_array, srcid_column, dstid_column);
    compute.run(visitor);
  }
#endif

} // end of sgraph_compute
} // end of grahlab
