/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/dml/distributed_graph.hpp>
#include <unity/dml/load_balancing.hpp>

#include <rpc/dc.hpp>
#include <rpc/dc_global.hpp>
#include <parallel/pthread_tools.hpp>
#include <fileio/sanitize_url.hpp>

#include <sframe/sframe.hpp>
#include <sframe/sframe_saving.hpp>
#include <sgraph/sgraph.hpp>
#include <sgraph/sgraph_compute.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>
#include <sgraph/hilbert_curve.hpp>

#include <unity/lib/gl_sgraph.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/unity_sarray.hpp>

#include <boost/filesystem.hpp>
#include <numerics/armadillo.hpp>
#include <numerics/armadillo.hpp>

namespace turi {
namespace distributed_sgraph_compute {


/**************************************************************************/
/*                                                                        */
/*                            Helper Functions                            */
/*                                                                        */
/**************************************************************************/
/**
 * This function creates a local copy of the sgraph which contains only
 * a subset of the the edges of the original graph.
 */
sgraph make_local_sgraph(sgraph& global_sgraph,
                         std::vector<std::pair<size_t, size_t>> edge_coordinates,
                         std::vector<size_t> vertex_coordinates) {

  sgraph ret(global_sgraph.get_num_partitions());

  size_t sframe_default_segments = SFRAME_DEFAULT_NUM_SEGMENTS;
  SFRAME_DEFAULT_NUM_SEGMENTS = 1;

  sframe empty_vertices;
  empty_vertices.open_for_write(global_sgraph.get_vertex_fields(),
                                global_sgraph.get_vertex_field_types());
  empty_vertices.close();

  sframe empty_edges;
  empty_edges.open_for_write(global_sgraph.get_edge_fields(),
                             global_sgraph.get_edge_field_types());
  empty_edges.close();

  for (size_t i = 0; i < ret.get_num_partitions(); ++i) {
    ret.vertex_partition(i) = empty_vertices;
    for (size_t j = 0; j < ret.get_num_partitions(); ++j) {
      ret.edge_partition(i, j) = empty_edges;
    }
  }

  for (auto edge_coord: edge_coordinates) {
    auto tempname = get_temp_name() + ".frame_idx";
    sframe_save_weak_reference(global_sgraph.edge_partition(edge_coord.first, edge_coord.second), tempname);
    ret.edge_partition(edge_coord.first, edge_coord.second) = sframe(tempname);
  }
  for (auto vertex_coord: vertex_coordinates) {
    auto tempname = get_temp_name() + ".frame_idx";
    sframe_save_weak_reference(global_sgraph.vertex_partition(vertex_coord), tempname);
    ret.vertex_partition(vertex_coord) = sframe(tempname);
  }

  SFRAME_DEFAULT_NUM_SEGMENTS = sframe_default_segments;
  return ret;
}


/**************************************************************************/
/*                                                                        */
/*                           Distributed Graph                            */
/*                                                                        */
/**************************************************************************/
distributed_graph::distributed_graph(std::string path,
                                     distributed_control* dc,
                                     std::vector<std::string> vdata_fields,
                                     std::vector<std::string> edata_fields)
  : procid(dc->procid()), numprocs(dc->numprocs()), dc(dc) {

    m_global_graph = std::make_shared<sgraph>();
    m_local_graph = std::make_shared<sgraph>();

    std::shared_ptr<unity_sgraph> ug = std::make_shared<unity_sgraph>();
    ug->load_graph(path);
    ug = std::static_pointer_cast<unity_sgraph>(ug->select_vertex_fields(vdata_fields, 0)->select_edge_fields(edata_fields, 0, 0));
    *m_global_graph = ug->get_graph();

    partition_graph();

    logstream(LOG_EMPH) << "Constructing local sgraph" << std::endl;
    *m_local_graph = make_local_sgraph(*m_global_graph,
                                       edge_coords[procid],
                                       m_my_master_vertex_partitions);
    logstream(LOG_EMPH) << "Done constructing local sgraph" << std::endl;
}

size_t distributed_graph::num_partitions() const {
  return m_global_graph->get_num_partitions();
}

size_t distributed_graph::num_vertices(size_t partition_id) const {
  if( partition_id == (size_t)-1 ) {
    return m_global_graph->num_vertices();
  } else {
    return m_global_graph->vertex_partition(partition_id).num_rows();
  }
}

size_t distributed_graph::num_edges(size_t src_partition, size_t dst_partition) const {
  if( src_partition == (size_t)-1 || dst_partition == (size_t)-1 ) {
    return m_global_graph->num_edges();
  } else {
    return m_global_graph->edge_partition(src_partition, dst_partition).num_rows();
  }
}

void distributed_graph::print_partition_summary() {
  std::cout << "[Proc " << procid << "] Vertex partition assignment: \n";
  for (size_t i = 0; i < vertex_partition_to_worker.size(); ++i) {
    std::cout << "[Proc " << procid << "] Partition " << i << ":";
    for (size_t j = 0; j < vertex_partition_to_worker[i].size(); ++j) {
      std::cout << vertex_partition_to_worker[i][j] << " ";
    }
    std::cout << "\n";
  }
  std::cout << std::endl;;

  size_t num_edges_local = 0;
  for (auto coord : my_edge_partitions())
    num_edges_local += m_global_graph->edge_partition(coord.first, coord.second).num_rows();

  size_t num_vertices_local = 0;
  for (auto coord : my_master_vertex_partitions())
    num_vertices_local += m_global_graph->vertex_partition(coord).num_rows();

  std::cout << "[Proc " << procid << "] Number edge partitions: "
            << my_edge_partitions().size() << "\n"
            << "[Proc " << procid << "] Number edges: "
            << num_edges_local << "\n"
            << "[Proc " << procid << "] Number owning vertices: "
            << num_vertices_local << "\n"
            << "[Proc " << procid << "] Number vertex partitions: "
            << my_vertex_partitions().size() << "\n"
            << "[Proc " << procid << "] Master of vertex partitions: \n";
  for (auto i : my_master_vertex_partitions()) {
    std::cout << i << "\t";
  }
  std::cout << std::endl;
}

/**
 * Graph partitioning
 * Assign edge partitions to machines evenly using hilbert curve ordering
 * to minimize the span of vertex partitions.
 */
void distributed_graph::partition_graph() {
  edge_coords.resize(numprocs);
  vertex_coords.resize(numprocs);

  // tmp data structures
  std::vector<std::set<size_t>> vertex_coords_set(numprocs);
  EIGTODO::SparseMatrix<size_t> constraint_matrix(numprocs, num_partitions());
  std::set<size_t> my_src_vertex_coords_set, my_dst_vertex_coords_set;

  // Fill in edge_coords
  size_t num_edge_partitions = num_partitions() * num_partitions();
  for (size_t i = 0; i < numprocs; ++i) {
    size_t low = (num_edge_partitions * i) / numprocs;
    size_t high = (num_edge_partitions * (i + 1)) / numprocs;
    for (size_t j = low; j < high; ++j) {
      auto coord = hilbert_index_to_coordinate(j, num_edge_partitions);
      edge_coords[i].push_back(coord);
      vertex_coords_set[i].insert(coord.first);
      vertex_coords_set[i].insert(coord.second);
      if (i == procid) {
        my_src_vertex_coords_set.insert(coord.first);
        my_dst_vertex_coords_set.insert(coord.second);
      }
    }
  }

  // Fill in vertex_coords, assign masters
  for (size_t i = 0; i < numprocs; ++i) {
    vertex_coords[i].assign(vertex_coords_set[i].begin(),
                            vertex_coords_set[i].end());
    for (size_t j : vertex_coords[i]) {
      constraint_matrix.coeffRef(i, j) = 1;
    }
  }
  std::vector<size_t> master_assignment;
  double max_load = 0.0;
  std::tie(master_assignment, max_load) = solve_genearlized_load_balancing(constraint_matrix);
  logstream(LOG_INFO) << "Max load " << max_load << std::endl;

  m_src_vertex_coords.assign(my_src_vertex_coords_set.begin(),
                             my_src_vertex_coords_set.end());
  m_dst_vertex_coords.assign(my_dst_vertex_coords_set.begin(),
                             my_dst_vertex_coords_set.end());

  // Fill in veretx_partition_to_worker
  // A reverse lookup table: vertex partition to worker
  vertex_partition_to_worker.resize(num_partitions());
  for (size_t i = 0; i < vertex_coords.size(); ++i) {
    for (auto j: vertex_coords[i]) {
      vertex_partition_to_worker[j].push_back(i);
    }
  }
  // First worker in each partition is the master
  for (size_t j = 0; j < vertex_partition_to_worker.size(); ++j) {
    size_t master = master_assignment[j];
    for (size_t i = 0; i < vertex_partition_to_worker[j].size(); ++i) {
      if (vertex_partition_to_worker[j][i] == master)
        std::swap(vertex_partition_to_worker[j][0], vertex_partition_to_worker[j][i]);
    }
  }

  // Fill in m_my_master_vertex_partitions
  for (size_t coord = 0; coord < vertex_partition_to_worker.size(); ++coord) {
    // if I am master for this partition
    if (vertex_partition_to_worker[coord][0] == procid) {
      m_my_master_vertex_partitions.push_back(coord);
    }
  }

  // Done, print some debug information
  print_partition_summary();
}

void distributed_graph::add_vertex_field(
    const std::vector<std::shared_ptr<sarray<flexible_type>>>& column_data,
    const std::string& column_name,
    flex_type_enum dtype) {

  ASSERT_EQ(column_data.size(), num_partitions());
  for (size_t i = 0; i < column_data.size(); ++i) {

    auto column = column_data[i];
    ASSERT_EQ((int)column->get_type(), (int)dtype);
    auto& sf = m_local_graph->vertex_partition(i);
    size_t expected_column_size = m_local_graph->vertex_partition(i).size();

    if (is_master_of_partition(i)) {
      ASSERT_EQ(column->size(), expected_column_size);
      sf = sf.add_column(column, column_name);
    } else {
      // make dummy column
      auto sa = std::make_shared<sarray<flexible_type>>();
      sa->open_for_write(1);
      sa->set_type(dtype);
      flexible_type v(dtype);
      std::vector<flexible_type> dummy_values(expected_column_size, v);
      turi::copy(dummy_values.begin(), dummy_values.end(), *sa);
      sa->close();
      sf = sf.add_column(sa, column_name);
    }
  }
}

void distributed_graph::save_as_sgraph(const std::string& path) {
  auto dc = distributed_control::get_instance();
  size_t procid = dc->procid();

  namespace fs = boost::filesystem;
  std::string subgraph_prefix("subgraphs");

  // Create directory for subgraph
  dir_archive dirarc;
  if (procid == 0) {
    dirarc.open_directory_for_write(path);

    fs::path subgraph_dir(path);
    subgraph_dir /= subgraph_prefix;
    auto fstat = fileio::get_file_status(subgraph_dir.string());
    if (fstat == fileio::file_status::MISSING) {
      fileio::create_directory(subgraph_dir.string());
    } else if(fstat == fileio::file_status::DIRECTORY) {
      // do nothing, let it be overwritting
      logstream(LOG_WARNING) << "Subgraph directory " << subgraph_dir.string()
                             << " already exists. Overwriting." << std::endl;
    } else {
      // regular file
      log_and_throw(std::string("Cannot create directory at regular file ") + subgraph_dir.string());
    }
  }
  dc->barrier();

  // Each machine saves its own vertex partition
  auto vertex_partition_outname = [=](size_t partition_id) {
    std::string vertex_prefix = std::string("vertex-part-") + std::to_string(partition_id);
    fs::path outpath(path);
    outpath /= subgraph_prefix;
    outpath /= vertex_prefix;
    return outpath.string() + ".frame_idx";
  };

  for (auto partition_id: my_master_vertex_partitions()) {
    std::string outname = vertex_partition_outname(partition_id);
    auto& sf = m_local_graph->vertex_partition(partition_id);
    logstream(LOG_INFO) << "Saving partition " << partition_id << std::endl;
    sframe_save_weak_reference(sf, outname);
    logstream(LOG_INFO) << "Done saving partition " << partition_id << std::endl;
  }

  // Ignore edge partitions, since no algorithm changes it, yet.

  dc->barrier();
  // For root machine, lets' collect the pieces
  if (procid == 0) {
    sgraph ret(*m_global_graph);
    for (size_t i = 0; i < num_partitions(); ++i) {
      ret.vertex_partition(i) = sframe(vertex_partition_outname(i));
    }
    // Save reference
    dirarc.set_metadata("contents", "graph");
    oarchive oarc(dirarc);
    unity_sgraph ug(std::make_shared<sgraph>(ret));
    ug.save_reference(oarc);
  }
}


} // end of distributed_sgraph_compute
} // end of turicreate
