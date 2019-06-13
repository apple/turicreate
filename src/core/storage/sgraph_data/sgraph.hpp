/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_HPP
#define TURI_SGRAPH_SGRAPH_HPP
#include <memory>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sgraph_data/sgraph_constants.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <sparsehash/sparse_hash_map>

namespace turi {

/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_main Main SGraph Objects
 * \{
 */


/**
 * An On disk representation of a graph.
 *
 * The actual on disk representation looks like the following:
 *
 * Where the partition size (\ref partition_size) is 4, we shuffle all the
 * vertices into 4 SFrames, each of 1 segment. The shuffling is performed by
 * simply hashing the vertex ID into one of the buckets.
 *
 * The edges however, are placed into 4*4=16 SFrames, each of 1 segment.
 * Each edge (src,dst) is placed into the (hash(src) % 4) * 4 + hash(dst) % 4.
 * Essentially the Edge SFrame can be thought of as cutting the adjacency matrix
 * into a 4x4 grid.
 *
 * The result is that the edges in the block (0,0) is adjacent only to the
 * vertices in the first block (0), the edges in block (0,1) is adjacent only
 * to the vertices in block 0 and 1 and so on.
 *
 * \verbatim
 *
 * Vertices                 Edges
 *  +---+      +-------+-------+-------+-------+
 *  |   |      |       |       |       |       |
 *  | 0 |      | (0,0) | (0,1) | (0,2) | (0,3) |
 *  +---+      +-------+-------+-------+-------+
 *  |   |      |       |       |       |       |
 *  | 1 |      | (1,0) | (1,1) | (1,2) | (1,3) |
 *  +---+      +-------+-------+-------+-------+
 *  |   |      |       |       |       |       |
 *  | 2 |      | (2,0) | (2,1) | (2,2) | (2,3) |
 *  +---+      +-------+-------+-------+-------+
 *  |   |      |       |       |       |       |
 *  | 3 |      | (3,0) | (3,1) | (3,2) | (3,3) |
 *  +---+      +-------+-------+-------+-------+
 *
 * \endverbatim
 *
 *  Vertices are partitioned into user-defined semantic groups.  Each vertex
 *  can only show up in one group, and each vertex is uniquely identified by
 *  the combination of the group ID and the Vertex ID. The vertex ID type MUST
 *  be consistent and identical across all groups.
 *
 *  Vertex grouping is implemented by having multiple of the vertex blocks, one
 *  for each group. Thus m_vertex_groups[0] contains a vector of SFrames for
 *  vertex group 0 and so on.
 *
 *  Edges are not grouped and they may span any collection of vertices.
 *  However, to be able to efficiently slice vertices and edges across groups,
 *  there are g*g edges groups, where m_edge_groups[{a,b}]
 *  contain all the edges between group a and group b.
 */
class sgraph {
 public:

  static const char* DEFAULT_GROUP_NAME;
  static const char* VID_COLUMN_NAME;
  static const char* SRC_COLUMN_NAME;
  static const char* DST_COLUMN_NAME;
  static const flex_type_enum INTERNAL_ID_TYPE;

  explicit sgraph(size_t num_partitions = SGRAPH_DEFAULT_NUM_PARTITIONS);

  typedef std::map<std::string, flexible_type> options_map_t;


  enum class edge_direction {
    IN_EDGE = 1, OUT_EDGE = 2, ANY_EDGE = 3
  };

  struct vertex_partition_address {
    size_t group = 0, partition = 0;
    vertex_partition_address() = default;
    vertex_partition_address(size_t group, size_t partition):
        group(group),partition(partition) { }

    bool operator==(const vertex_partition_address& other) const {
      return group == other.group && partition == other.partition;
    }
    bool operator<(const vertex_partition_address& other) const {
      return group < other.group ||
          (group == other.group && partition < other.partition);
    }
  };

  class edge_partition_address {
   public:
    size_t src_group = 0, dst_group = 0, partition1 = 0, partition2 = 0;

    edge_partition_address() = default;
    edge_partition_address(size_t src_group, size_t dst_group,
                           size_t partition1, size_t partition2) :
        src_group(src_group), dst_group(dst_group),
        partition1(partition1), partition2(partition2) { }

    vertex_partition_address get_src_vertex_partition() {
      vertex_partition_address ret;
      ret.group = src_group; ret.partition = partition1;
      return ret;
    }

    vertex_partition_address get_dst_vertex_partition() {
      vertex_partition_address ret;
      ret.group = dst_group; ret.partition = partition2;
      return ret;
    }
  };


 public:
  sgraph(const sgraph& other) = default;

  sgraph& operator=(const sgraph& other) = default;

  /**
   * Returns a sframe of vertices satisfying the id and field constraints.
   */
  sframe get_vertices(const std::vector<flexible_type>& vid_vec = {},
                      const options_map_t& field_constraint = options_map_t(),
                      size_t groupid = 0) const;

  /**
   * Returns a dataframe of vertices satisfying the id
   * and field constraints. source_vids may contain UNDEFINED as wildcards
   * and target_vids may contain UNDEFINED as wildcards.
   * Each edge will only be represented once in the output.
   *
   * If source_vids and target_vids are empty, a universal
   * "UNDEFINED-->UNDEFINED" query is assumed
   */
   sframe get_edges(const std::vector<flexible_type>& source_vids = {},
                    const std::vector<flexible_type>& target_vids = {},
                    const options_map_t& field_constraint = options_map_t(),
                    size_t groupa = 0, size_t groupb = 0) const;

  /**
   * Returns a list of fields for given vertex group in the graph.
   */
  std::vector<std::string> get_vertex_fields(size_t groupid = 0) const;

  /**
   * Returns a list of field types for given vertex group in the graph.
   */
  std::vector<flex_type_enum> get_vertex_field_types(size_t groupid = 0) const;

  /**
   * Returns a list of fields for given edge group in the graph.
   */
  std::vector<std::string> get_edge_fields(size_t groupa = 0, size_t groupb = 0) const;

  /**
   * Returns a list of field types for given edge group in the graph.
   */
  std::vector<flex_type_enum> get_edge_field_types(size_t groupa = 0, size_t groupb = 0) const;

  /**
   * Adds vertices to the graph.
   *
   * Note: The dataframe must contain the id_field_name
   */
   bool add_vertices(const dataframe_t& vertices,
                     const std::string& id_field_name,
                     size_t group = 0);
  /**
   * \overload
   */
   bool add_vertices(sframe vertices,
                     const std::string& id_field_name,
                     size_t group = 0);

  /**
   * Adds edges to the graph.
   *
   * Note: The dataframe must contain the {source, target}_field_name
   */
   bool add_edges(const dataframe_t& edges,
                  const std::string& source_field_name,
                  const std::string& target_field_name,
                  size_t groupa = 0, size_t groupb = 0);
  /**
   * \overload
   */
   bool add_edges(sframe edges,
                  const std::string& source_field_name,
                  const std::string& target_field_name,
                  size_t groupa = 0, size_t groupb = 0);

  /**
   * Copies data from "field" to a new field with name "new_field" for a vertex group.
   * If the new_field already exists, it will be replaced.
   */
  bool copy_vertex_field(const std::string& field, const std::string& new_field,
                         size_t group = 0);

  /**
   * Similar to copy_vertex_field but work on edge data.
   * If the new_field already exists, it will be replaced.
   */
  bool copy_edge_field(const std::string& field, const std::string& new_field,
                       size_t groupa = 0, size_t groupb = 0);

  /**
   * Initialize a vertex field of group with const value.
   * Creates a new column if the field does not exist.
   */
  bool init_vertex_field(const std::string& field, const flexible_type& init_value, size_t group = 0);

  /**
   * Deletes a field from vertex data. Returns true on success.
   * False on failure.
   */
  bool remove_vertex_field(const std::string& field, size_t group = 0);

  /**
   * Subselect fields in the vertex sframe.
   */
  bool select_vertex_fields(const std::vector<std::string>& fields, size_t group = 0);

  /**
   * Deletes a field from edge data. Returns true on success.
   * False on failure.
   */
  bool remove_edge_field(const std::string& field, size_t groupa = 0, size_t groupb = 0);

  /**
   * Initialize an edge field of group with const value.
   * Creates a new column if the field does not exist.
   */
  bool init_edge_field(const std::string& field, const flexible_type& init_value, size_t groupa = 0, size_t groupb = 0);

  /**
   * Subselect fields in the edge sframe.
   */
  bool select_edge_fields(const std::vector<std::string>& fields, size_t groupa = 0, size_t groupb = 0);

  /**
   * Resets the graph
   */
  bool clear();

  /**
   * Returns the collection of SFrames containing all the vertices
   * in group groupid.
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * vertex_group(group) = blah
   * \endcode
   * The caller must guarantee that blah is of the right size. (i.e.
   * blah.size() == get_num_partitions() )
   */
  inline std::vector<sframe>& vertex_group(size_t groupid = 0) {
    ASSERT_LT(groupid, m_num_groups);
    return m_vertex_groups[groupid];
  }

  /**
   * Returns the collection of SFrames containing all the vertices
   * in group groupid
   */
  inline const std::vector<sframe>& vertex_group(size_t groupid = 0) const {
    ASSERT_LT(groupid, m_num_groups);
    return m_vertex_groups[groupid];
  }


  /**
   * Returns the collection of SFrames containing all the edges
   * between vertex group groupa and vertex group groupb.
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * edge_group(group) = blah
   * \endcode
   * The caller must guarantee that blah is of the right size. (i.e.
   * blah.size() == get_num_partitions() * get_num_partitions() )
   */
  inline std::vector<sframe>& edge_group(size_t groupa = 0,
                                         size_t groupb = 0) {
    ASSERT_LT(groupa, m_num_groups);
    ASSERT_LT(groupb, m_num_groups);
    return m_edge_groups.at({groupa, groupb});
  }

  /**
   * Returns the collection of SFrames containing all the edges
   * between vertex group groupa and vertex group groupb.
   */
  inline const std::vector<sframe>& edge_group(size_t groupa,
                                               size_t groupb) const {
    ASSERT_LT(groupa, m_num_groups);
    ASSERT_LT(groupb, m_num_groups);
    return m_edge_groups.at({groupa, groupb});
  }


  /**
   * Returns the SFrame containing all the vertices in a given partition
   * of a group groupid
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * vertex_partition(part, group) = sframe
   * \endcode
   */
  inline sframe& vertex_partition(size_t partition,
                                  size_t groupid = 0) {
    ASSERT_LT(partition, m_num_partitions);
    return vertex_group(groupid)[partition];
  }


  /**
   * Returns the SFrame containing all the vertices in a given partition
   * of a group groupid
   */
  inline const sframe& vertex_partition(size_t partition,
                                        size_t groupid = 0) const {
    ASSERT_LT(partition, m_num_partitions);
    return vertex_group(groupid)[partition];
  }


  /**
   * Returns the SFrame containing all the vertices in a given partition
   * of a group groupid
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * vertex_partition(part, group) = sframe
   * \endcode
   */
  inline sframe& vertex_partition(vertex_partition_address part) {
    return vertex_partition(part.partition, part.group);
  }


  /**
   * Returns the SFrame containing all the vertices in a given partition
   * of a group groupid
   */
  inline const sframe& vertex_partition(vertex_partition_address part) const {
    return vertex_partition(part.partition, part.group);
  }

  /**
   * Returns the SFrame containing all edges in the partition (partition1,
   * partition2) between vertex group groupa and vertex group groupb.
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * edge_partition(part1, part2) = sframe
   * \endcode
   */
  inline sframe& edge_partition(size_t partition1,
                                size_t partition2,
                                size_t groupa = 0,
                                size_t groupb = 0) {
    ASSERT_LT(partition1, m_num_partitions);
    ASSERT_LT(partition2, m_num_partitions);
    return edge_group(groupa, groupb)[partition1 * m_num_partitions + partition2];
  }


  /**
   * Returns the SFrame containing all edges in the partition (partition1,
   * partition2) between vertex group groupa and vertex group groupb.
   */
  inline const sframe& edge_partition(size_t partition1,
                                      size_t partition2,
                                      size_t groupa = 0,
                                      size_t groupb = 0) const {
    ASSERT_LT(partition1, m_num_partitions);
    ASSERT_LT(partition2, m_num_partitions);
    return edge_group(groupa, groupb)[partition1 * m_num_partitions + partition2];
  }


  /**
   * Returns the SFrame containing all edges in the partition (partition1,
   * partition2) between vertex group groupa and vertex group groupb.
   *
   * This function can be used as the left hand side of an assignment.
   * i.e.
   * \code
   * edge_partition(part1, part2) = sframe
   * \endcode
   */
  inline sframe& edge_partition(edge_partition_address address) {
    return edge_partition(address.partition1, address.partition2,
                          address.src_group, address.dst_group);
  }


  /**
   * Returns the SFrame containing all edges in the partition (partition1,
   * partition2) between vertex group groupa and vertex group groupb.
   */
  inline const sframe& edge_partition(edge_partition_address address) const {
    return edge_partition(address.partition1, address.partition2,
                          address.src_group, address.dst_group);
  }


  /**
   * Returns the name of the vertex group given the group id. Assertion failure
   * if the group does not exist.
   */
  inline std::string get_vertex_group_name(size_t idx) const {
    ASSERT_LT(idx, m_vertex_group_names.size());
    return m_vertex_group_names[idx];
  }

  /**
   * Returns the id of the vertex group given the group name. Returns
   * (size_t)(-1) on failure.
   */
  inline size_t get_vertex_group_id(std::string name) const {
    auto iter = std::find(m_vertex_group_names.begin(), m_vertex_group_names.end(), name);
    if (iter != m_vertex_group_names.end()) return std::distance(m_vertex_group_names.begin(), iter);
    else return (size_t)(-1);
  }

  /**
   * Returns number of edges from groupa to groupb.
   */
  inline size_t num_edges(size_t groupa, size_t groupb) const {
    size_t ret = 0;
    auto& egroup = edge_group(groupa, groupb);
    for (auto& sf : egroup) {
      ret += sf.size();
    }
    return ret;
  };

  /**
   * Returns the total number of edges.
   */
  inline size_t num_edges() const { return m_num_edges; };

  /**
   * Returns the number of vertices in the group.
   */
  inline size_t num_vertices(size_t group) const {
    size_t ret = 0;
    auto& vgroup = vertex_group(group);
    for (auto& sf : vgroup) {
      ret += sf.size();
    }
    return ret;
  };

  /**
   * Returns the total number of vertices.
   */
  inline size_t num_vertices() const { return m_num_vertices; }

  /**
   * Returns true if the graph is empty.
   */
  inline bool empty() const { return (m_num_vertices == 0) && (m_num_edges == 0);}

  /**
   * Returns the number of vertex partitions
   */
  inline size_t get_num_partitions() const {
    return m_num_partitions;
  }

  /**
   * Returns the number of vertex groups
   */
  inline size_t get_num_groups() const {
    return m_num_groups;
  }

  inline flex_type_enum vertex_id_type() const { return m_vid_type; }

/**************************************************************************/
/*                                                                        */
/*                             Serialization                              */
/*                                                                        */
/**************************************************************************/
  /**
   * \Internal
   *
   * Save to a directory oarchive.
   */
  void save(oarchive& oarc) const;

  /**
   * \Internal
   *
   * Save to directory oarchive using sframe save reference.
   *
   * \see sframe_save_weak_reference
   */
  void save_reference(oarchive& oarc) const;

  /**
   * \Internal
   *
   * Load from a directory oarchive.
   */
  void load(iarchive& iarc);

/**************************************************************************/
/*                                                                        */
/*                       Unity Related Operations                         */
/*                                                                        */
/**************************************************************************/
  void add_vertex_field(std::shared_ptr<sarray<flexible_type>> data, std::string field);

  void add_edge_field(std::shared_ptr<sarray<flexible_type>> data, std::string field);

  void swap_vertex_fields(const std::string& field1, const std::string& field2);

  void swap_edge_fields(const std::string& field1, const std::string& field2);

  void rename_vertex_fields(const std::vector<std::string>& oldnames,
                            const std::vector<std::string>& newnames);

  void rename_edge_fields(const std::vector<std::string>& oldnames,
                         const std::vector<std::string>& newnames);

/**************************************************************************/
/*                                                                        */
/*                       Compute Related Operations                       */
/*                                                                        */
/**************************************************************************/
  /**
   * Replaces a particular column in all partitions of a particular group of
   * vertices. The column must exist. Returns true on success.
   */
  bool replace_vertex_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                            std::string column_name,
                            size_t group = 0);

  /**
   * Same as \ref replace_vertex_field, but all values are in memory
   * The column must exist. Assertion failure otherwise.
   */
  template<typename T, typename FLEX_TYPE=T>
  bool replace_vertex_field(std::vector<std::vector<T>>& column,
                            std::string column_name,
                            size_t group = 0);

  /**
   * Replaces a particular edge in all partitions of a particular group of
   * edges. The column must exist. Returns true on success.
   */
  bool replace_edge_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                          std::string column_name,
                          size_t groupa = 0, size_t groupb = 0);

  /**
   * Add a particular column in all partitions of a particular group of
   * vertices. The column must not exist. Returns true on success.
   */
  bool add_vertex_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                        std::string column_name,
                        size_t group = 0);

  /**
   * Same as \ref add_vertex_field, but all values are in memory
   * The column must exist. Assertion failure otherwise.
   */
  template<typename T, typename FLEX_TYPE=T>
  bool add_vertex_field(std::vector<std::vector<T>>& column,
                        std::string column_name,
                        flex_type_enum column_type,
                        size_t group = 0);

  /**
   * Add a particular edge in all partitions of a particular group of
   * edges. The column must not exist. Returns true on success.
   */
  bool add_edge_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                      std::string column_name,
                      size_t groupa = 0, size_t groupb = 0);


  /**
   * Extracts the data for a particular field of a group of vertices.
   * The column must exist. Assertion failure otherwise.
   */
  std::vector<std::shared_ptr<sarray<flexible_type>>>
      fetch_vertex_data_field(std::string column_name, size_t group = 0) const;


  /**
   * Same as \ref fetch_vertex_data_field, but store all values in memory
   * and return std::vector<std::vector<flexible_type>>
   * The column must exist. Assertion failure otherwise.
   */
  std::vector<std::vector<flexible_type>>
      fetch_vertex_data_field_in_memory(std::string column_name, size_t groupid = 0) const;

  /**
   * Extracts the data for a particular field of a group of edges.
   * The column must exist. Assertion failure otherwise.
   */
  std::vector<std::shared_ptr<sarray<flexible_type>>>
      fetch_edge_data_field(std::string column_name, size_t groupa = 0, size_t groupb = 0) const;


  /**
   * Gets the offset of the vertex field. Throws an exception on failure.
   */
  size_t get_vertex_field_id(std::string column_name, size_t group = 0) const;


  /**
   * Gets the offset of the edge field. Throws an exception on failure.
   */
  size_t get_edge_field_id(std::string column_name, size_t groupa = 0, size_t groupb = 0);

 private:

  /**
   * Clears the graph, and reinitializes it with a given number of partitions
   * and only 1 group (the default group)
   */
  void init(size_t num_partitions);

  /**
   * Initialize the vertex id type.
   * Reset vertex/edge sframes to have the proper id column types.
   */
  void bootstrap_vertex_id_type(flex_type_enum id_type);

  /**
   * Add new groups up to num_groups - 1. Initialize the vertex partitions
   * and edge partitions.
   * num_groups must be strictly greater than the current number of groups.
   */
  void increase_number_of_groups(size_t num_groups);

  /**
   * Merges the insertion buffer for a vertex partition of a particular group
   * into the main vertex store.
   *
   * \param edge_buffer vector of length m_num_partition^2.
   */
  void commit_edge_buffer(size_t groupa,
                          size_t groupb,
                          sframe edge_buffer);

  /**
   * Merges the vertex data of the group  with the vertex data buffer.
   *
   * \param vertex_buffer vector of length m_num_partition.
   */
  void commit_vertex_buffer(size_t group,
                            std::vector<sframe>& vertex_buffer);

  /**
   * Helper function to merge a single vertex partition.
   */
  sframe merge_vertex_partition(sframe& current_vdata, sframe& new_vdata);

  /**
   * Return the vertex partition number for given vertex id.
   */
  inline size_t get_vertex_partition(const flexible_type& vid) { return vid.hash() % m_num_partitions; }

  /**
   * Return the edge partition number for an edge.
   */
  inline size_t get_edge_partition(const flexible_type& src, const flexible_type& dst) {
    return get_vertex_partition(src) * m_num_partitions + get_vertex_partition(dst);
  }

  /**
   * Returns a vector of vertex ids in the given partition and vertex group.
   */
  inline std::vector<flexible_type> get_vertex_ids(size_t partition, size_t group) const {
    std::vector<flexible_type> ret;
    const sframe& sf = vertex_partition(partition, group);
    auto id_column = sf.select_column(VID_COLUMN_NAME);
    ret.reserve(id_column->size());
    copy(*id_column, std::inserter(ret, ret.begin()));
    return ret;
  }

  /**
   * Returns true if this field name begins with __
   */
  static inline bool is_private_field(std::string s) {
    return s.length() > 2 && s[0] == '_' && s[1] == '_';
  }

  /**
   * A list of all the group names. The 0th group (the default group)
   * is always the group name "default".
   */
  std::vector<std::string> m_vertex_group_names;

  /**
   * Number of SFrames each vertex group is cut up into.
   */
  size_t m_num_partitions = 0;

  /**
   * The number of groups
   */
  size_t m_num_groups = 1;

  /**
   * Cached graph statistics: number of vertices.
   */
  size_t m_num_vertices = 0;

  /**
   * Cached graph statistics: number of edges.
   */
  size_t m_num_edges = 0;

  /**
   * The vertex id type.
   */
  flex_type_enum m_vid_type = flex_type_enum::INTEGER;

  /**
   * An array of the same length as vertex_group_names.
   * Each vertex group is represented as an array of sframes.
   */
  std::vector<std::vector<sframe> > m_vertex_groups;

  /**
   * A map from (group, group) to an edge group.
   * Each edge group is represented as an array of sframes.
   * Only the "upper triangle" of the group pair is defined. i.e.
   * to find all edges between group a and group b, it is in
   * m_edge_groups({min(a,b),max(a,b)})
   */
  std::map<std::pair<size_t, size_t>, std::vector<sframe> > m_edge_groups;

  /**************************************************************************/
  /*                                                                        */
  /*                            Helper Functions                            */
  /*                                                                        */
  /**************************************************************************/
 private:
  /**
   * Adjust the columns in sf to be the order of column_names.
   * All columns in sf must exist in column_names, and the types must match column_types.
   * For columns in column_names that are not in sf, add a dummy column filled
   * with flexible_undefined values.
   */
  static bool reorder_and_add_new_columns(sframe& sf,
                                          const std::vector<std::string>& column_names,
                                          const std::vector<flex_type_enum>& column_types);

  /**
   * Union the column name and column types of two sframes.
   * By the end of the function, two sframes should have
   * the same column names and types, in the same order.
   *
   * The intersection of the column set must have the same type.
   *
   * \code
   * (column_names, column_types)= (sframe_a.column_names(), sframe_a.column_types())
   * for (col in sframe_b.columns() but not in sframe_a.columns()) {
   *   column_names.push_back(col.name);
   *   column_types.push_back(col.type);
   * }
   * reorder_and_add_new_columns(sframe_a, column_names, column_types);
   * reorder_and_add_new_columns(sframe_b, column_names, column_types);
   * \endcode
   *
   * Return true if union succeeds.
   */
  static bool union_columns(sframe& sframe_a, sframe& sframe_b);

  typedef google::sparse_hash_map<flexible_type, size_t, std::hash<flexible_type> > vid_hash_map_type;

  /**
   * Fetch the vertex id to row id lookup table.
   */
  std::shared_ptr<vid_hash_map_type> fetch_vid_hash_map(size_t partition, size_t group);

  /**
   * Initialize an empty sframe with column names and types.
   */
  static inline void init_empty_sframe(sframe& sf,
      std::vector<std::string> column_names = {},
      std::vector<flex_type_enum> column_types = {}) {
    sframe new_sf;
    new_sf.open_for_write(column_names, column_types, "", 1 /*one segment*/);
    new_sf.close();
    sf = new_sf;
  }

  /**
   * Do a quick validation of the add vertices operation
   * Throws a string on failure.
   */
   void fast_validate_add_vertices(const sframe& vertices,
                                   size_t group);

  /**
   * Do a quick validation of the add edges operation
   * Throws a string on failure.
   */
   void fast_validate_add_edges(const sframe& edges,
                                size_t groupa, size_t groupb);
};


template<typename T, typename FLEX_TYPE>
bool sgraph::add_vertex_field(std::vector<std::vector<T>>& column,
                              std::string column_name,
                              flex_type_enum column_type,
                              size_t groupid) {
  auto vfields = get_vertex_fields();
  if (std::count(vfields.begin(), vfields.end(), column_name) != 0) {
    logstream(LOG_ERROR) << "Vertex field already exists." << std::endl;
    return false;
  }
  auto& vgroups = vertex_group(groupid);
  if (vgroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }
  parallel_for (0, vgroups.size(), [&](size_t i) {
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write(1);
    sa->set_type(column_type);
    auto writer = sa->get_output_iterator(0);
    for (auto& v: column[i])
      *writer++ = std::move((FLEX_TYPE)(v));
    sa->close();
    vgroups[i] = vgroups[i].add_column(sa, column_name);
  });
  return true;
}

template<typename T, typename FLEX_TYPE>
bool sgraph::replace_vertex_field(std::vector<std::vector<T>>& column,
                                  std::string column_name,
                                  size_t groupid) {

  auto vfields = get_vertex_fields();
  if (std::count(vfields.begin(), vfields.end(), column_name) == 0) {
    logstream(LOG_ERROR) << "Vertex field not found." << std::endl;
    return false;
  }
  auto& vgroups = vertex_group(groupid);
  if (vgroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }

  auto column_type = get_vertex_field_types()[get_vertex_field_id(column_name)];

  parallel_for (0, vgroups.size(), [&](size_t i) {
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write(1);
    sa->set_type(column_type);
    auto writer = sa->get_output_iterator(0);
    for (auto& v: column[i])
      *writer++ = std::move((FLEX_TYPE)(v));
    sa->close();
    vgroups[i] = vgroups[i].replace_column(sa, column_name);
  });
  return true;
}

/// \}
} // namespace turi
#endif
