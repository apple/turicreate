/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GL_SGRAPH_HPP
#define TURI_UNITY_GL_SGRAPH_HPP
#include <cmath>
#include <memory>
#include <cstddef>
#include <string>
#include <iostream>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/sgraph_triple_apply_typedefs.hpp>
#include "gl_gframe.hpp"

namespace turi {
class gl_sarray;
class gl_sframe;
class gl_gframe;
class unity_sgraph;
class unity_sgraph_base;


/**
 * \ingroup group_glsdk
 * A scalable graph data structure backed by persistent storage (\ref gl_sframe).
 *
 * The SGraph (\ref gl_sgraph) data structure allows arbitrary
 * dictionary attributes on vertices and edges, provides flexible vertex and
 * edge query functions, and seamless transformation to and from SFrame.
 *
 * ### Construction
 *
 * There are several ways to create an SGraph. The simplest way is to make an
 * empty graph then add vertices and edges with the \ref add_vertices
 * and \ref add_edges methods.
 *
 * \code
 * gl_sframe vertices { {"vid", {1,2,3} };
 * gl_sframe edges { {"src", {1, 3}}, {"dst", {2, 2}} };
 * gl_sgraph g = gl_sgraph().add_vertices(vertices, "vid").add_edges(edges, "src", "dst");
 * \endcode
 *
 * Columns in the \ref gl_sframes that are not used as id fields are assumed
 * to be vertex or edge attributes.
 *
 * \ref gl_sgraph objects can also be created from vertex and edge lists stored
 * in \ref gl_sframe.
 *
 * \code
 * gl_sframe vertices { {"vid", {1,2,3}, {"vdata" : {"foo", "bar", "foobar"}} };
 * gl_sframe edges { {"src", {1, 3}}, {"dst", {2, 2}}, {"edata": {0., 0.}} };
 * gl_sgraph g = gl_sgraph(vertices, edges, "vid", "src", "dst");
 * \endcode
 *
 * ### Usage
 *
 * The most convenient way to access vertex and edge data is through
 * the \ref vertices and \ref edges.
 * Both functions return a GFrame (\ref gl_gframe) object. GFrame is
 * like SFrame but is bound to the host SGraph,
 * so that modification to GFrame is applied to SGraph, and vice versa.
 *
 * For instance, the following code shows how to add/remove columns
 * to/from the vertex data. The change is applied to SGraph.
 * \code
 * // add a new edge attribute with const value.
 * g.edges().add_column("likes", 0);
 *
 * // remove a vertex attribute.
 * g.vertices().remove_column("age");
 *
 * // transforms one attribute to the other
 * g.vertices()["likes_fish"] = g.vertices()["__id"] == "cat";
 * \endcode
 *
 * You can also query for specific vertices and edges using the
 * \ref get_vertices and \ref get_edges functionality.
 *
 * For instance,
 * \code
 * gl_sframe selected_edges = g.get_edges(
 * { {0, UNDEFINED}, {UNDEFINED, 1}, {2, 3} },
 * { {"likes_fish", 1} });
 * \endcode
 * selects out going edges of 0, incoming edges of 1, edge 2->3, such that
 * the edge attribute "like_fish" evaluates to 1.
 *
 * In addition, you can perform other non-mutating \ref gl_sframe operations like
 * groupby, join, logical_filter in the same way, and the returned object will
 * be \ref gl_sframe.
 *
 * In the case where you want to perform vertex-specified operations,
 * such as "gather"/"scatter" over the neighborhood of each vertex,
 * we provide \ref triple_apply which is essentially a
 * "parallel for" over (Vertex, Edge, Vertex) triplets.
 *
 * For instance, the following code shows how to implement the update function
 * for synchronous pagerank.
 *
 * \code
 *
 * const double RESET_PROB = 0.15;
 *
 * void pr_update(edge_triple& triple) {
 *   triple.target["pagerank"] += triple.source["pagerank_prev"] / triple.source["out_degree"];
 * }
 *
 * gl_sgraph pagerank(const gl_sgraph& g, size_t iters) {
 *
 *   // Count the out degree of each vertex into an gl_sframe.
 *   gl_sframe out_degree = g.get_edges().groupby("__src_id", {{"out_degree", aggregate::COUNT()}});
 *
 *   // Add the computed "out_degree" to the graph as vertex attribute.
 *   // We exploit that adding the same vertex will overwrite the vertex data.
 *   gl_sgraph g2 = g.add_vertices(out_degree, "__src_id");
 *
 *   // Initialize pagerank value
 *   g2.vertices()["pagerank"] = 0.0;
 *   g2.vertices()["pagerank_prev"] = 1.0;
 *
 *   for (size_t i = 0; i < iters; ++i) {
 *     g2.vertices()["pagerank"] = 0.0;
 *     g2 = g2.triple_apply(pr_update, {"pagerank"});
 *     g2.vertices()["pagerank"] = RESET_PROB + (1 - RESET_PROB) * g2.vertices()["pagerank"];
 *     g2.vertices()["pagerank_prev"] = g2.vertices()["pagerank"];
 *   }
 *
 *   g2.vertices().remove_column("pagerank_prev");
 *   g2.vertices().remove_column("out_degree");
 *   return g2;
 * }
 *
 * \endcode
 *
 * ### Mutability
 *
 * \ref gl_sgraph is structurally immutable but data (or field) Mutable.
 * You can add new vertices and edges, but the operation returns a new \ref
 Ï
 * \endcode
 *
 * ### Example
 *
 * Please checkout turicreate/sdk_example/sgraph_example.cpp for a concrete example.
 */
class gl_sgraph {

 public:
  gl_sgraph();
  gl_sgraph(const gl_sgraph&);
  gl_sgraph(gl_sgraph&&);
  gl_sgraph& operator=(const gl_sgraph&);
  gl_sgraph& operator=(gl_sgraph&&);

  /**
   * Construct gl_sgraph with given vertex data and edge data.
   *
   * \param vertices Vertex data. Must include an ID column with the name specified by
   *     "vid_field." Additional columns are treated as vertex attributes.
   *
   * \param edges Edge data. Must include source and destination ID columns as specified
   *     by "src_field" and "dst_field". Additional columns are treated as edge
   *     attributes.
   *
   * \param vid_field Optional. The name of vertex ID column in the "vertices" \ref gl_sframe.
   *
   * \param src_field Optional. The name of source ID column in the "edges" \ref gl_sframe.
   *
   * \param dst_field Optional. The name of destination ID column in the "edges" \ref gl_sframe.
   *
   * Example
   *
   * \code
   *
   * gl_sframe vertices { {"vid", {"cat", "dog", "fossa"}} };
   * gl_sframe edges { {"source", {"cat", "dog", "dog"}},
   *                   {"dest", {"dog", "cat", "foosa"}},
   *                   {"relationship", {"dislikes", "likes", "likes"}} };
   * gl_sgraph g = gl_sgraph(vertices, edges, "vid", "source", "dest");
   *
   * std::cout << g.vertices() << std::endl;
   * std::cout << g.edges() << std::endl;
   *
   * // This following code is equivalent.
   * // gl_sgraph g = gl_sgraph().add_vertices(vertices, "vid") \
   * //                          .add_edges(edges, "source", "dest");
   *
   * \endcode
   *
   * Produces output:
   *
   * \code{.txt}
   *
   * vertices of the gl_sgraph
   * +-------+
   * |  __id |
   * +-------+
   * |  cat  |
   * |  dog  |
   * | foosa |
   * +-------+
   *
   * edges of the gl_sgraph
   * +----------+----------+--------------+
   * | __src_id | __dst_id | relationship |
   * +----------+----------+--------------+
   * |   cat    |   dog    |   dislikes   |
   * |   dog    |   cat    |    likes     |
   * |   dog    |  foosa   |    likes     |
   * +----------+----------+--------------+
   *
   * \endcode
   *
   * \see \ref gl_sframe
   */
  gl_sgraph(const gl_sframe& vertices, const gl_sframe& edges,
            const std::string& vid_field="__id",
            const std::string& src_field="__src_id",
            const std::string& dst_field="__dst_id");

  /**
   * Constructs from a saved gl_sgraph.
   *
   * \see save
   */
  explicit gl_sgraph(const std::string& directory);

  /*
   * Implicit Type converters
   */
  gl_sgraph(std::shared_ptr<unity_sgraph>);
  gl_sgraph(std::shared_ptr<unity_sgraph_base>);
  operator std::shared_ptr<unity_sgraph>() const;
  operator std::shared_ptr<unity_sgraph_base>() const;

  /**************************************************************************/
  /*                                                                        */
  /*                              Graph query                               */
  /*                                                                        */
  /**************************************************************************/

  typedef std::pair<flexible_type, flexible_type> vid_pair;

  /**
   * Return a collection of edges and their attributes.
   *
   * This function is used to find edges by vertex IDs,
   * filter on edge attributes, or list in-out * neighbors of vertex sets.
   *
   * \param ids Optional. Array of pairs of source and target vertices, each
   *    corresponding to an edge to fetch. Only edges in this list are returned.
   *    FLEX_UNDEFINED can be used to designate a wild card. For instance, {{1,3},
   *    {2,FLEX_UNDEFINED}, {FLEX_UNDEFINED, 5}} will fetch the edge 1->3, all
   *    outgoing edges of 2 and all incoming edges of 5. ids may be left empty,
   *    which implies an array of all wild cards.
   *
   * \param fields Optional. Dictionary specifying equality constraints on
   *    field values. For example, { {"relationship", "following"} }, returns only
   *    edges whose 'relationship' field equals 'following'. FLEX_UNDEFINED can be
   *    used as a value to designate a wild card. e.g. { {"relationship",
   *    FLEX_UNDEFINED} } will find all edges with the field 'relationship'
   *    regardless of the value.
   *
   * Example:
   * \code
   * gl_sframe edges{ {"__src_id", {0, 0, 1}},
   *                  {"__dst_id", {1, 2, 2}},
   *                  {"rating", {5, 2, FLEX_UNDEFINED}} };
   * gl_sgraph g = gl_sgraph().add_edges(edges);
   *
   * std::cout << g.get_edges() << std::endl;
   * std::cout << g.get_edges({}, { {"rating": 5} }) << std::endl;
   * std::cout << g.get_edges({ {0, 1}, {1, 2} }) << std::endl;
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   * Return all edges in the graph.
   *
   * +----------+----------+--------+
   * | __src_id | __dst_id | rating |
   * +----------+----------+--------+
   * |    0     |    2     |   2    |
   * |    0     |    1     |   5    |
   * |    1     |    2     |  None  |
   * +----------+----------+--------+
   *
   * Return edges with the attribute "rating" of 5.
   *
   * +----------+----------+--------+
   * | __src_id | __dst_id | rating |
   * +----------+----------+--------+
   * |    0     |    1     |   5    |
   * +----------+----------+--------+
   *
   * Return edges 0 --> 1 and 1 --> 2 (if present in the graph).
   *
   * +----------+----------+--------+
   * | __src_id | __dst_id | rating |
   * +----------+----------+--------+
   * |    0     |    1     |   5    |
   * |    1     |    2     |  None  |
   * +----------+----------+--------+
   * \endcode
   *
   * \see edges
   * \see get_vertices
   */
  gl_sframe get_edges(const std::vector<vid_pair>& ids=std::vector<vid_pair>(),
                      const std::map<std::string, flexible_type>& fields=std::map<std::string, flexible_type>()) const;

  /**
   * Return a collection of vertices and their attributes.
   *
   * \param ids List of vertex IDs to retrieve. Only vertices in this list will be
   *     returned.
   *
   * \param fields Dictionary specifying equality constraint on field values. For
   *     example { {"gender", "M"} }, returns only vertices whose 'gender'
   *     field is 'M'. FLEX_UNDEFINED can be used to designate a wild card. For
   *     example, { {"relationship", FLEX_UNDEFINED } } will find all vertices with the
   *     field 'relationship' regardless of the value.
   *
   * Example:
   *
   * \code
   *
   * gl_sframe vertices  { {"__id", {0, 1, 2}},
   *                       {"gender", {"M", "F", "F"}} };
   * g = gl_sgraph().add_vertices(vertices);
   *
   * std::cout << g.get_vertices() << std::endl;
   * std::cout << g.get_vertices({0, 2}) << std::endl;
   * std::cout << g.get_vertices({}, { {"gender", "M"} }) << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   * Return all vertices in the graph.
   *
   * +------+--------+
   * | __id | gender |
   * +------+--------+
   * |  0   |   M    |
   * |  2   |   F    |
   * |  1   |   F    |
   * +------+--------+
   *
   * Return vertices 0 and 2.
   *
   * +------+--------+
   * | __id | gender |
   * +------+--------+
   * |  0   |   M    |
   * |  2   |   F    |
   * +------+--------+
   *
   * Return vertices with the vertex attribute "gender" equal to "M".
   *
   * +------+--------+
   * | __id | gender |
   * +------+--------+
   * |  0   |   M    |
   * +------+--------+
   * \endcode
   *
   * \see vertices
   * \see get_edges
   */
  gl_sframe get_vertices(const std::vector<flexible_type>& ids=std::vector<flexible_type>(),
                         const std::map<std::string, flexible_type>& fields=std::map<std::string, flexible_type>()) const;


  /**
   * Return the number of vertices and edges as a dictionary.
   *
   * Example:
   *
   * \code
   * g = gl_sgraph();
   * std::cout << g.summary()['num_vertices'] << "\n"
   *           << g.summary()['num_edges'] << std::endl;;
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   * 0
   * 0
   * \endcode
   *
   * \see num_vertices
   * \see num_edges
   */
  std::map<std::string, flexible_type> summary() const;

  /**
   * Return the number of vertices in the graph.
   */
  size_t num_vertices() const;

  /**
   * Return the number of edges in the graph.
   */
  size_t num_edges() const;

  /**
   * Return the names of both vertex fields and edge fields in the graph.
   */
  std::vector<std::string> get_fields() const;

  /**
   * Return the names of vertex fields in the graph.
   */
  std::vector<std::string> get_vertex_fields() const;

  /**
   * Return the names of edge fields in the graph.
   */
  std::vector<std::string> get_edge_fields() const;

  /**
   * Return the types of vertex fields in the graph.
   */
  std::vector<flex_type_enum> get_vertex_field_types() const;

  /**
   * Return the types of edge fields in the graph.
   */
  std::vector<flex_type_enum> get_edge_field_types() const;


  /**************************************************************************/
  /*                                                                        */
  /*                           Graph modification                           */
  /*                                                                        */
  /**************************************************************************/
  /**
   * Add vertices to the \ref gl_sgraph and return the new graph.
   *
   * Input vertices should be in the form of \ref gl_sframe and
   * "vid_field" specifies which column contains the vertex ID. Remaining
   * columns are assumed to hold additional vertex attributes. If these
   * attributes are not already present in the graph's vertex data, they are
   * added, with existing vertices acquiring the missing value FLEX_UNDEFINED.
   *
   * \param vertices Vertex data. An \ref gl_sframe whose
   *     "vid_field" column contains the vertex IDs.
   *     Additional columns are treated as vertex attributes.
   *
   * \param vid_field Optional. Specifies the vertex id column in the vertices
   *     gl_sframe.
   *
   * Example:
   *
   * \code
   *
   * // Add three vertices to an empty graph
   * gl_sframe vertices { {"vid": {0, 1, 2}},
   *                      {"breed": {"labrador", "labrador", "vizsla"}} };
   * gl_sgraph g = gl_sgraph().add_vertices( vertices, "vid" );
   *
   * // Overwrite existing vertex
   * gl_sgraph g2 = g.add_vertices ( gl_sframe { {"vid", {0}}, {"breed", "poodle"} },
   *                      "vid" );
   *
   * // Add vertices with new attributes
   * gl_sgraph g3 = g2.add_vertices (gl_sframe { {"vid", {3}},
   *                                             {"weight", "20 pounds"} },
   *                                 "vid" );
   *
   * std::cout << g.get_vertices() << std::endl;
   * std::cout << g2.get_vertices() << std::endl;
   * std::cout << g3.get_vertices() << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * vertices of g1
   * +------+----------+
   * | __id |  breed   |
   * +------+----------+
   * |  0   | labrador |
   * |  2   |  vizsla  |
   * |  1   | labrador |
   * +------+----------+
   *
   * vertices of g2
   * +------+----------+
   * | __id |  breed   |
   * +------+----------+
   * |  0   |  poodle  |
   * |  2   |  vizsla  |
   * |  1   | labrador |
   * +------+----------+
   *
   * vertices of g3
   * +------+----------+-----------+
   * | __id |  breed   |   weight  |
   * +------+----------+-----------+
   * |  0   |  poodle  |    None   |
   * |  2   |  vizsla  |    None   |
   * |  1   | labrador |    None   |
   * |  4   |   None   | 20 pounds |
   * +------+----------+-----------+
   *
   * \endcode
   *
   * \note The column specified by vid_field will be renamed to "__id"
   *    as the special vertex attribute.
   *
   * \note If a vertex id already exists in the graph, adding a new vertex
   *    with the same id will overwrite the entire vertex attributes.
   *
   * \note If adding vertices to a non-empty graph, the types of the
   *    existing columns must be the same as those of the existing vertices.
   *
   * \note This function returns a new graph, and does not modify the
   *    current graph.
   *
   * \see gl_sframe
   * \see vertices
   * \see get_vertices
   * \see add_edges
   */
  gl_sgraph add_vertices(const gl_sframe& vertices, const std::string& vid_field) const;

  /**
   * Add edges to the \ref gl_sgraph and return the new graph.
   *
   * Input edges should be in the form of \ref gl_sframe and
   * "src_field" and "dst_field" specifies which two columns contain the
   * id of source vertex IDs and target vertices. Remaining
   * columns are assumed to hold additional vertex attributes. If these
   * attributes are not already present in the graph's edge data, they are
   * added, with existing edges acquiring the missing value FLEX_UNDEFINED.
   *
   * \param edges Edge data. An \ref gl_sframe whose
   *     "src_field" and "dst_field" columns contain the source and target
   *     vertex IDs. Additional columns are treated as edge attributes.
   *
   * \param src_field Optional. Specifies the source id column in the edges
   *     gl_sframe.
   *
   * \param dst_field Optional. Specifies the target id column in the edges
   *     gl_sframe.
   *
   * Example:
   *
   * \code
   *
   * gl_sframe edges { {"source", {"cat", "fish"}},
   *                   {"dest", {"fish", "cat"}},
   *                   {"relation", {"eat", "eaten"}} };
   *
   * gl_sgraph g = gl_sgraph().add_edges(edges, "source", "dest");
   *
   * gl_sgraph g2 = g.add_edges(gl_sframe { {"source", {"cat"}},
   *                                        {"dest", {"fish"}},
   *                                        {"relation", {"like"}} },
   *                            "source", "dest");
   *
   * std::cout << g.get_edges() << std::endl;
   * std::cout << g2.get_edges() << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * edges of g
   * +----------+----------+----------+
   * | __src_id | __dst_id | relation |
   * +----------+----------+----------+
   * |   cat    |   fish   |   eat    |
   * |   fish   |   cat    |  eaten   |
   * +----------+----------+----------+
   *
   * edges of g2
   * +----------+----------+----------+
   * | __src_id | __dst_id | relation |
   * +----------+----------+----------+
   * |   cat    |   fish   |   eat    |
   * |   cat    |   fish   |   like   |
   * |   fish   |   cat    |  eaten   |
   * +----------+----------+----------+
   *
   * \endcode
   *
   * \note The columns specified by "src_id" and "dst_id" will be renamed to
   *    "__src_id", and "__dst_id" respectively as the special edge attributes.
   *
   * \note If an edge (identified by src and dst id) already exists in the graph,
   *    adding a new edge with the same src and dst will NOT overwrite the existing
   *    edge. The same edge is DUPLICATED.
   *
   * \note If an edge contains new vertices, the new vertices will be automatically
   *    added to the graph with all attributes default to FLEX_UNDEFINED.
   *
   * \note If adding edges to a non-empty graph, the types of the
   *    existing columns must be the same as those of the existing edges.
   *
   * \note This function returns a new graph, and does not modify the
   *    current graph.
   *
   * \see gl_sframe
   * \see edges
   * \see get_edges
   * \see add_vertices
   */
  gl_sgraph add_edges(const gl_sframe& edges,
                      const std::string& src_field,
                      const std::string& dst_field) const;

  /**
   * Return a new \ref gl_sgraph with only the selected vertex fields.
   * Other vertex fields are discarded, while fields that do not exist in the
   * \ref gl_sgraph are ignored. Edge fields remain the same in the new graph.
   *
   * \param fields A list of field names to select.
   *
   * Example:
   *
   * \code
   *
   * gl_sframe vertices { {"vid", {0, 1, 2}},
   *                      {"breed", {"labrador", "labrador", "vizsla"}},
   *                      {"age", {5, 3, 8}} };
   * g = SGraph().add_vertices(vertices, "vid");
   * g2 = g.select_vertex_fields({"breed"});
   *
   * std::cout << g.vertices() << std::endl;
   * std::cout << g2.vertices() << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * vertices of g
   * +------+----------+-----+
   * | __id |  breed   | age |
   * +------+----------+-----+
   * |  0   | labrador |  5  |
   * |  2   |  vizsla  |  3  |
   * |  1   | labrador |  8  |
   * +------+----------+-----+
   *
   * vertices of g2
   * +------+----------+
   * | __id |  breed   |
   * +------+----------+
   * |  0   | labrador |
   * |  2   |  vizsla  |
   * |  1   | labrador |
   * +------+----------+
   * \endcode
   *
   * \note "__id" will always be selected.
   *
   * \see get_fields
   * \see get_vertex_fields
   * \see get_edge_fields
   * \see select_fields
   * \see select_edge_fields
   */
  gl_sgraph select_vertex_fields(const std::vector<std::string>& fields) const;

  /**
   * Return a new \ref gl_sgraph with only the selected edge fields.
   * Other edge fields are discarded, while fields that do not exist in the
   * \ref gl_sgraph are ignored. Vertex fields remain the same in the new graph.
   *
   * \param fields A list of field names to select.
   *
   * Example:
   *
   * \code
   * gl_sframe edges { {"source", {"Alice", "Bob"}},
   *                   {"dest", {"Bob", "Alice"}},
   *                   {"follows", {0, 1}},
   *                   {"likes", {5, 3}} };
   *
   * g = SGraph().add_edges(edges, "source", "dest");
   * g2 = g.select_edge_fields({"follows"});
   *
   * std::cout << g.edges() << std::endl;
   * std::cout << g2.edges() << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * vertices of g
   *
   * vertices of g2
   * \endcode
   *
   * \note "__src_id" and "__dst_id" will always be selected.
   *
   * \see get_fields
   * \see get_vertex_fields
   * \see get_edge_fields
   * \see select_vertex_fields
   * \see select_fields
   */
  gl_sgraph select_edge_fields(const std::vector<std::string>& fields) const;

  /**
   * Return a new \ref gl_sgraph with only the selected fields (both vertex and
   * edge fields. Other fields are discarded, while fields that do not exist in the
   * \ref gl_sgraph are ignored.
   *
   * \param fields A list of field names to select.
   *
   * \note "__id", "__src_id" and "__dst_id" will always be selected.
   *
   * \see select_vertex_fields
   * \see select_edge_fields
   */
  gl_sgraph select_fields(const std::vector<std::string>& fields) const;


  /**************************************************************************/
  /*                                                                        */
  /*             SFrame handler for mutating vertices and edges             */
  /*                                                                        */
  /**************************************************************************/

  /**
   * Returns a convenient "SFrame like" handler for the vertices in this
   * \ref gl_sgraph.
   *
   * While a regular \ref gl_sframe is independent of any \ref gl_sgraph, a
   * \ref gl_gframe is bound (or points) to an \ref gl_sgraph. Modifying fields
   * of the returned \ref gl_gframe changes the vertex data of the \ref
   * gl_sgraph.  Also, modifications to the fields in the \ref gl_sgraph, will
   * be reflected in the \ref gl_gframe.
   *
   * Example:
   *
   * \code
   *
   * gl_sframe vertices { {"vid", {"cat", "dog", "hippo"}},
   *                      {"fluffy", {1, 1, FLEX_UNDEFINED}},
   *                      {"woof", {FLEX_UNDEFINED, 1, FLEX_UNDEFINED}} };
   * gl_sgraph g = gl_sgraph().add_vertices(vertices, "vid");
   *
   * // Let's modify the vertex data by operating on g.vertices():
   * // Copy the 'woof' vertex field into a new 'bark' vertex field.
   * g.vertices()["bark"] = g.vertices["woof"];
   * std::cout << g.vertices() << std::endl;
   *
   * // Remove the 'woof' field.
   * g.vertices().remove_column("woof");
   * std::cout << g.vertices() << std::endl;
   *
   * // Create a new field 'like_fish'.
   * g.vertices()['likes_fish'] = g.vertices()['__id'] == "cat";
   * std::cout << g.vertices() << std::endl;
   *
   * // Replace missing values with zeros.
   * for (const auto& col : g.vertices().column_names()) {
   *   if (col != "__id") {
   *     g.vertices().fillna(col, 0);
   *   }
   * }
   * std::cout << g.vertices() << std::endl;
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * Copy the 'woof' vertex attribute into a new 'bark' vertex attribute:
   * +-------+--------+------+------+
   * |  __id | fluffy | woof | bark |
   * +-------+--------+------+ -----+
   * |  dog  |  1.0   | 1.0  | 1.0  |
   * |  cat  |  1.0   | NA   | NA   |
   * | hippo |  NA    | NA   | NA   |
   * +-------+--------+------+ -----+
   *
   * Remove the 'woof' attribute:
   * +-------+--------+------+
   * |  __id | fluffy | bark |
   * +-------+--------+------+
   * |  dog  |  1.0   | 1.0  |
   * |  cat  |  1.0   | NA   |
   * | hippo |  NA    | NA   |
   * +-------+--------+------+
   *
   * Create a new field 'likes_fish':
   * +-------+--------+------+------------+
   * |  __id | fluffy | bark | likes_fish |
   * +-------+--------+------+------------+
   * |  dog  |  1.0   | 1.0  |     0      |
   * |  cat  |  1.0   | NA   |     1      |
   * | hippo |  NA    | NA   |     0      |
   * +-------+--------+------+------------+
   *
   * Replace missing values with zeros:
   * +-------+--------+------+------------+
   * |  __id | fluffy | bark | likes_fish |
   * +-------+--------+------+------------+
   * |  dog  |  1.0   | 1.0  |     0      |
   * |  cat  |  1.0   | 0.0  |     1      |
   * | hippo |  0.0   | 0.0  |     0      |
   * +-------+--------+------+------------+
   * \endcode
   *
   * \note To preserve the graph structure the "__id" column of this \ref
   *    gl_sframe is read-only.
   *
   * \see edges
   */
  gl_gframe vertices();

  /**
   * Returns a convenient "SFrame like" handler for the edges in this
   * \ref gl_sgraph.
   *
   * While a regular \ref gl_sframe is independent of any \ref gl_sgraph, a
   * \ref gl_gframe is bound (or points) to an \ref gl_sgraph. Modifying fields
   * of the returned \ref gl_gframe changes the edge data of the \ref
   * gl_sgraph. Also, modifications to the fields in the \ref gl_sgraph, will
   * be reflected in the \ref gl_gframe.
   *
   * Example:
   *
   * \code
   *
   * gl_sframe vertices { {"vid", {"cat", "dog", "fossa"}} };
   * gl_sframe edges { {"source", {"cat", "dog", "dog"}},
   *                   {"dest", {"dog", "cat", "foosa"}},
   *                   {"relationship", {"dislikes", "likes", "likes"}} };
   *
   * gl_sgraph g = gl_sgraph(vertices, edges, "vid", "source", "dest");
   *
   * // Add a new edge field "size":
   * g.edges()["size"] = { {"smaller than", "larger than", "equal to"} };
   * std::cout << g.edges() << std::endl;
   *
   * \endcode
   *
   * Produces output:
   * \code{.txt}
   *
   * Add a new edge field "size"
   * +----------+----------+--------------+--------------+
   * | __src_id | __dst_id | relationship |     size     |
   * +----------+----------+--------------+--------------+
   * |   cat    |   dog    |   dislikes   | smaller than |
   * |   dog    |   cat    |    likes     | larger than  |
   * |   dog    |  foosa   |    likes     |   equal to   |
   * +----------+----------+--------------+--------------+
   *
   * \endcode
   *
   * \note To preserve the graph structure the "__src_id"  and "__dst_id"
   *    column of this \ref gl_sframe is read-only.
   *
   * \see vertices
   */
  gl_gframe edges();

  /**************************************************************************/
  /*                                                                        */
  /*                              Triple Apply                              */
  /*                                                                        */
  /**************************************************************************/
  /**
   * Apply a user defined lambda function on each of the edge triples, and returns
   * the new graph.
   *
   * An \ref edge_triple is a simple struct containing source, edge and target
   * of type std::map<std::string, flexible_type>. The lambda function
   * is applied once on each of the edge_triple in parallel, with locking on
   * both source and target vertices to prevent race conditions. The following
   * pseudo code describes the effect of the function:
   *
   * \code
   * INPUT: G
   * OUTPUT: G'
   * G' = copy(G)
   * PARALLEL FOR (source, edge, target) in G':
   *   LOCK (source, target)
   *      edge_triple triple(source, edge, target)
   *      lambda(triple)
   *      FOR f in mutated_fields:
   *        source[f] = triple.source[f] // if f in source
   *        target[f] = triple.target[f] // if f in target
   *        edge[f] = triple.edge[f] // if f in edge
   *      END FOR
   *   UNLOCK (source, target)
   * END PARALLEL FOR
   * RETURN G'
   * \endcode
   *
   * This function enables super easy implementations of common graph
   * computations like degree_count, weighted_pagerank, connected_component, etc.
   *
   * Example
   *
   * \code
   * gl_sframe edges { {"source": {0,1,2,3,4}},
   *                   {"dest": {1,2,3,4,0}} };
   * g = turicreate.SGraph().add_edges(edges, "source", "dest");
   * g.vertices()['degree'] = 0
   * std::cout << g.vertices() << std::endl;
   *
   * auto degree_count_fn = [](edge_triple& triple)->void {
   *  triple.source["degree"] += 1;
   *  triple.target["degree"] += 1;
   * };
   *
   * g2 = g.triple_apply(degree_count_fn, {"degree"});
   * std::cout << g2.vertices() << std::endl;
   * \endcode
   *
   * Produces output:
   *
   * \code{.txt}
   *
   * Vertices of g
   * +------+--------+
   * | __id | degree |
   * +------+--------+
   * |  0   |   0    |
   * |  2   |   0    |
   * |  3   |   0    |
   * |  1   |   0    |
   * |  4   |   0    |
   * +------+--------+
   *
   * Vertices of g2
   * +------+--------+
   * | __id | degree |
   * +------+--------+
   * |  0   |   2    |
   * |  2   |   2    |
   * |  3   |   2    |
   * |  1   |   2    |
   * |  4   |   2    |
   * +------+--------+
   *
   * \endcode
   *
   * \note mutated fields must be pre-allocated before triple_apply.
   *
   * \see edge_triple
   * \see lambda_triple_apply_fn
   */
  gl_sgraph triple_apply(const lambda_triple_apply_fn& lambda,
                         const std::vector<std::string>& mutated_fields) const;

  /**
   * Save the sgraph into a directory.
   */
  void save(const std::string& directory) const;

  /**
   * Save the sgraph using reference to other SFrames.
   *
   * \see gl_sframe::save_reference
   */
  void save_reference(const std::string& directory) const;

  friend class gl_gframe;

 public:
  /**************************************************************************/
  /*                                                                        */
  /*                 Lower level fields modifier primitives                 */
  /*                                                                        */
  /**************************************************************************/

  /**
   * Add a new vertex field with given field name and column_data.
   * Using \ref vertices() is preferred.
   *
   * \param column_data gl_sarray of size equals to num_vertices. The order of
   *    column_data is aligned with the order which vertices are stored.
   *
   * \param field name of the new vertex field.
   */
  void add_vertex_field(gl_sarray column_data, const std::string& field);

  /**
   * Add a new vertex field filled with constant data.
   * Using \ref vertices() is preferred.
   *
   * \param column_data the constant data to fill the new field column.
   *
   * \param field name of the new vertex field.
   */
  void add_vertex_field(const flexible_type& column_data, const std::string& field);

  /**
   * Removes the vertex field
   *
   * \param name of the field to be removed
   */
  void remove_vertex_field(const std::string& field);

  /**
   * Renames the vertex fields
   *
   * \param oldnames list of names of the fields to be renamed
   *
   * \param newnames list of new names of the fields, aligned with oldnames.
   */
  void rename_vertex_fields(const std::vector<std::string>& oldnames,
                            const std::vector<std::string>& newnames);

  /**
   * Add a new edge field with given field name and column_data.
   * Using \ref edges() is preferred.
   *
   * \param column_data gl_sarray of size equals to num_vertices. The order of
   *    column_data is aligned with the order which vertices are stored.
   *
   * \param field name of the new edge field.
   */
  void add_edge_field(gl_sarray column_data, const std::string& field);

  /**
   * Add a new edge field filled with constant data.
   * Using \ref edges() is preferred.
   *
   * \param column_data the constant data to fill the new field column.
   *
   * \param field name of the new edge field.
   */
  void add_edge_field(const flexible_type& column_data, const std::string& field);

  /**
   * Removes the edge field
   *
   * \param name of the field to be removed
   */
  void remove_edge_field(const std::string& field);

  /**
   * Renames the edge fields
   *
   * \param oldnames list of names of the fields to be renamed
   *
   * \param newnames list of new names of the fields, aligned with oldnames.
   */
  void rename_edge_fields(const std::vector<std::string>& oldnames,
                          const std::vector<std::string>& newnames);

  /**
   * Retrieves a pointer to the underlying unity_sgraph
   */
  virtual std::shared_ptr<unity_sgraph> get_proxy() const;

 private:
  void instantiate_new();
  void instantiate_from_proxy(std::shared_ptr<unity_sgraph>);
  void _swap_vertex_fields(const std::string& field1, const std::string& field2);
  void _swap_edge_fields(const std::string& field1, const std::string& field2);

 private:
  std::shared_ptr<unity_sgraph> m_sgraph;
};

} // namespace turi
#endif
