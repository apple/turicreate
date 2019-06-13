/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GRAPH_OPERATION_DAG_HPP
#define TURI_UNITY_GRAPH_OPERATION_DAG_HPP
#include <functional>
#include <queue>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <core/util/mutable_queue.hpp>
#include <core/storage/lazy_eval/lazy_eval_operation.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
namespace turi {

template <typename T>
class lazy_eval_operation_dag;

/**
 * \defgroup lazy_eval Lazy Evaluation DAG
 */

/**
 * \ingroup lazy_eval
 * The lazy_eval_future<T> is the key object managed by the
 * lazy_eval_operation_dag. All operations issued to the dag returns a
 * future object. Evaluating the future (using operator()) will evaluate
 * the DAG allowing the object to be accessed efficiently.
 */
template <typename T>
class lazy_eval_future {
 public:
  typedef T value_type;
  typedef lazy_eval_operation_base<value_type> operation_type;
  typedef lazy_eval_operation_dag<value_type> dag_type;

  /// deleted default constructor
  lazy_eval_future() = delete;
  /// default copy constructor
  lazy_eval_future(const lazy_eval_future& other) = default;
  /// default assignment operator
  lazy_eval_future& operator=(const lazy_eval_future& other) = default;

  /**
   * Clears the contents of the future. All shared pointers obtained from
   * the object are still valid.
   */
  void reset() {
    log_func_entry();
    object.reset();
  }

  /**
   * Evaluates the dependencies of this object, caches the result, and returns a
   * reference to the computed value. If the value has already been cached,
   * no further computation is performed.
   */
  value_type& operator()() {
    if (!object) make_eager();
    return (*object);
  }


  /**
   * Evaluates the dependencies of this object, caches the result, and returns a
   * a shared pointer to the computed value. If the value has already been
   * cached, no further computation is performed.
   */
  std::shared_ptr<value_type> get_ptr() {
    if (!object) make_eager();
    return object;
  }

  /// Destructor
  ~lazy_eval_future() {
    log_func_entry();
    owner->mark_for_deletion(vertex_idx);
  }

  /// Gets the vertex ID in the lazy eval DAG datastructure
  size_t get_vertex_id() const {
    return vertex_idx;
  }

  /**
   * Returns true if the object has been computed.
   */
  bool is_available() {
    return object;
  }

 private:
  dag_type* owner;
  size_t vertex_idx;
  std::shared_ptr<value_type> object;

  /// private constructor
  lazy_eval_future(dag_type* owner, size_t vertex_idx):
      owner(owner), vertex_idx(vertex_idx) { log_func_entry(); }

  /// Forces the object to be fully instantiated
  void make_eager() {
    log_func_entry();
    object = owner->make_eager(vertex_idx);
  }

  friend class lazy_eval_operation_dag<value_type>;
};

/**
 * \ingroup lazy_eval
 * The Lazy Evaluation Operation DAG is a directed acyclic graph
 * connecting immutable objects of type T with operations, and also provide
 * lazy evaluation primitives to provide object computation at any point
 * in the tree.
 *
 * Using the lazy_eval_operation_dag system simply requires the user to
 * implement a collection of operations, each inheriting from
 * lazy_eval_operation_base<T>. For instance, we can define the following
 * multiply, increment, and set_val lazy operations on integers.
 *
 * \code
 *  struct multiplier: lazy_eval_operation_base<int> {
 *    virtual size_t num_arguments() { return 2; }
 *    virtual void execute(int& output,
 *                         const std::vector<int*>& parents) {
 *      output *= *(parents[0]);
 *    }
 *  };
 *
 *  struct increment: lazy_eval_operation_base<int> {
 *    virtual size_t num_arguments() { return 1; }
 *    virtual void execute(int& output,
 *                         const std::vector<int*>& parents) {
 *      output++;
 *    }
 *  };
 *
 *  struct set_val: lazy_eval_operation_base<int> {
 *    set_val(int i): val(i) { }
 *    size_t val;
 *    virtual size_t num_arguments() { return 0; }
 *    virtual void execute(int& output,
 *                         const std::vector<int*>& parents) {
 *      output = val;
 *    }
 *  };
 * \endcode
 *
 * To create a sequence of lazy operations simply involves the use of the
 * add_operation() function. Each call to add_operation is lazy, and
 * returns a future object.
 * \code
 *  lazy_eval_operation_dag<int> dag;
 *  lazy_eval_future<int>* five = dag.add_operation(new set_val(5), {});
 *  lazy_eval_future<int>* two = dag.add_operation(new set_val(2), {});
 *  lazy_eval_future<int>* seven = dag.add_operation(new adder, {five, two});
 *  lazy_eval_future<int>* nine = dag.add_operation(new adder, {seven, two});
 * \endcode
 *
 * We can then evaluate values with the operator() to compute the DAG.
 * \code
 *  int val = (*nine)();
 * \endcode
 */
template <typename T>
class lazy_eval_operation_dag {
 public:
  typedef T value_type;
  typedef lazy_eval_operation_base<value_type> operation_type;
  typedef lazy_eval_future<value_type> future_type;

  lazy_eval_operation_dag(std::function<value_type*()>
                            allocator = [](){return new value_type;},
                          std::function<void(value_type& dest, value_type& src)>
                            copier = [](value_type& dest, value_type& src){dest = src;})
      :next_vid(0), allocator(allocator), copier(copier) { log_func_entry(); }


  /**
   * Adds a fixed value to the DAG.
   * The returned future pointer will always be available efficiently,
   * Deleting the returned future pointer will mark the corresponding entry
   * in the DAG for deletion.
   */
  future_type* add_value(value_type* value) {
    return add_value(std::shared_ptr<value_type>(value));
  }

  future_type* add_value(std::shared_ptr<value_type> value) {
    vertex* vtx = new vertex(next_vid);
    vtx->operation = NULL;
    vtx->object_cache = value;
    vtx->object = value;
    vertices[next_vid] = vtx;
    future_type* ret = new future_type(this, next_vid);
    ++next_vid;
    // create and return a future
    return ret;
  }

  /**
   * Creates a new entry in the dependency tree by creating a future which
   * corresponds to calling the provided operation on the parents.
   *
   * Deleting the returned future pointer will mark the corresponding entry
   * in the DAG for deletion.
   */
  future_type* add_operation(operation_type* operation,
                            const std::vector<future_type*>& parents) {
    ASSERT_TRUE(operation != NULL);
    ASSERT_EQ(operation->num_arguments(), parents.size());
    // create a new vertex
    vertex* vtx = new vertex(next_vid);
    // no object yet. do not create one
    vtx->operation = operation;
    vtx->parents.resize(parents.size());
    for (size_t i = 0; i < parents.size(); ++i) {
      vtx->parents[i] = parents[i]->get_vertex_id();
      vertices[vtx->parents[i]]->children.push_back(next_vid);
    }
    vertices[next_vid] = vtx;
    future_type* ret = new future_type(this, next_vid);
    ++next_vid;
    // create and return a future
    return ret;
  }

  /**
   * Computes and caches a particular entry in the graph
   */
  std::shared_ptr<value_type> make_eager(size_t vertex_id) {
    log_func_entry();
    // check that we do have the vertex ID in question
    ASSERT_EQ(vertices.count(vertex_id), 1);
    // this is now the main evaluation logic
    // first check if we have a pointer to it
    vertex* vtx = vertices.at(vertex_id);
    // ok, the object is around! lets return that.
    if (!vtx->object.expired()) return vtx->object.lock();

    // first we need to backtrack the tree and find all ancestors which I depend
    // on in two or more paths and force construct them.
    // Lets keep those cached too.
    auto ancestors = list_ancestors(vertex_id);
    for(auto ancestor : ancestors) {
      if (ancestor.second.size() > 1) {
        // we only need to keep a shared to the objects to keep them alive.
        // The weak pointers in the vertex object will do the rest of the
        // work of tracking the existance of the objects.
        auto ancestor_object = make_eager(ancestor.first);
        vertices[ancestor.first]->object_cache = ancestor_object;
        vertices[ancestor.first]->object = ancestor_object;
      }
    }
    // ok. now we can do a recursive preorder traversal of the ancestor graph
    // to perform the evaluation
    auto ret = preorder_compute(vertex_id);
    vtx->object = ret;
    return ret;
  }

  /**
   * Marks the vertex for deletion. Deletion will only occur on a call to
   * cleanup()
   */
  void mark_for_deletion(size_t vertex_id) {
    log_func_entry();
    if (vertices.count(vertex_id) == 0) return;
    vertex* vtx = vertices.at(vertex_id);
    vtx->to_delete = true;
    // do some safe cleanup now
    cleanup(true);
  }


  /**
   * If the vertex value is cached, this will force it to be uncached.
   */
  void uncache(size_t vertex_id) {
    log_func_entry();
    if (vertices.count(vertex_id) == 0) return;
    vertex* vtx = vertices.at(vertex_id);
    // if operation is NULL, this vertex was created using add_value
     vtx->uncache();
  }

  /**
   * Attempts to delete all vertices that were marked for deletion
   * (see mark_for_deletion()). Note that not all marked vertices may be deleted
   * as some vertices (for instance, in the middle of a chain of operations)
   * cannot be deleted safely. This may result in the making eager of
   * certain vertices to ensure that referenced vertices can always be
   * constructed.
   *
   * \param avoid_instantiation Cancel the deletion if it involves instantiating
   *                            an as yet, uninstantiated vertex.
   */
  void cleanup(bool avoid_instantiation = false) {
    log_func_entry();
    // delete vertices from bottom up
    // sort the range of keys
    std::vector<size_t> ids_to_delete;
    for(auto vertex: vertices) {
      if (vertex.second->to_delete) ids_to_delete.push_back(vertex.first);
    }
    // reverse sort
    std::sort(ids_to_delete.rbegin(), ids_to_delete.rend());
    for(size_t id: ids_to_delete) {
      if (vertices.at(id)->to_delete) {
        delete_vertex(id, avoid_instantiation);
      }
    }
  }

  void print(std::ostream& out) const {
     out << "digraph G {\n";
     for(auto vertex: vertices) {
       out << "\t\"" << vertex.second->vertex_id << "\" ";
       // output the label
       out << "[label=\"" << vertex.second->vertex_id << ":";
       if (vertex.second->operation) {
         out<< vertex.second->operation->name();
       } else {
         out << "NULL";
       }
       if (!vertex.second->object.expired()) {
         value_type* v = vertex.second->object.lock().get();
         out << "\\nptr=" << v;
       }
       out << "\"";

       // print allocated objects in bold border
       // print deleted objects in red
       if (!vertex.second->object.expired()) {
         out << ",style=bold";
       }
       if (vertex.second->to_delete) {
         out << ",color=red";
       }
       out << "]\n";

       for (size_t children : vertex.second->children) {
         out << "\t\"" << vertex.second->vertex_id << "\" -> "
             << "\"" << children << "\"\n";
       }
     }
     out << "}\n";
  }


  /// destructor
  ~lazy_eval_operation_dag() {
    log_func_entry();
    for(auto vtx: vertices) {
      delete vtx.second;
    }
  }
 private:
  /// ID to assign to next vertex in the DAG
  size_t next_vid;

  /// A vertex in the DAG
  struct vertex {
    explicit vertex(size_t vertex_id)
        : operation(NULL), to_delete(false), vertex_id(vertex_id) { }
    ~vertex() { if (operation) delete operation; }
    /// The value of the vertex
    std::weak_ptr<value_type> object;
    /**
     * This is used to the store the value of the vertex when it is truly
     * necessary to do so. i.e. sometimes when vertices are deleted, their
     * children must be evaluated to keep the tree evaluatable.
     */
    std::shared_ptr<value_type> object_cache;
    /** The operation to evaluate on this vertex
     * If this is NULL, this is a value vertex
     */
    operation_type* operation;
    /// parent vertices
    std::vector<size_t> parents;
    /// child vertices
    std::vector<size_t> children;
    /// Marked for deletion
    bool to_delete;
    /// vertex ID
    size_t vertex_id;

    bool is_value_vertex() const {
      return operation == NULL;
    }
    void uncache() {
      if (!is_value_vertex()) {
        object_cache.reset();
      }
    }
  };

  /// A map from vertex ID to the vertex
  std::unordered_map<size_t, vertex*> vertices;
  /// Used to allocate values
  std::function<value_type*()> allocator;
  /// Used to copy values
  std::function<void(value_type& dest, value_type& src)> copier;

  /**
   * Returns a map of all ancestor vertices that have to be computed for this
   * vertex to be computed, as well as their children.
   *  - halts at non-expired nodes
   *  - halts at vertices with two or more children which the current vertex
   *    depends on
   */
  std::unordered_map<size_t, std::vector<size_t> > list_ancestors(size_t vertex) {
    std::unordered_map<size_t, std::vector<size_t> > ret;
    turi::mutable_queue<size_t, size_t> vqueue;
    // BFS implementation which does the following
    //  - halts at non-expired nodes
    //  - halts at vertices with two or more children which the current vertex
    //    depends on
    //
    // To accomplish the former is simply a bog standard BFS
    //
    // To accomplish the latter requires a simple trick: the vertex ordering
    // is a valid topological sort, and thus by always backtracking in the
    // reverse vertex ordering, when I evalute a vertex, I am guaranteed to
    // have all of its children listed
    vqueue.push(vertex, vertex);
    while(!vqueue.empty()) {
      size_t curvtx = vqueue.pop().first;
      if (ret.count(curvtx) && ret[curvtx].size() >= 2) continue;
      for (size_t parent: vertices.at(curvtx)->parents) {
        if (vertices.at(curvtx)->object.expired()) {
          ret[parent].push_back(curvtx);
          if (!vqueue.contains(parent)) vqueue.push(parent, parent);
        }
      }
    }
    return ret;
  }


  /**
   * Computes the value of the vertex, assuming certain preconditions
   * are satisfied: i.e. All dependent ancestors in the DAG with multiple
   * children are fully evaluated.
   */
  std::shared_ptr<value_type> preorder_compute(size_t vertex_id,
                                               bool make_copy = true) {
    // do a recursive backtrack through the vertices in ancestor_forward_edges
    vertex* vtx = vertices.at(vertex_id);

    if (!vtx->object.expired()) {
      // we hit a fully instantiated object. Return a copy of it.
      if (make_copy) {
        // if it is a value _vertex, we must always copy
        std::shared_ptr<value_type> ret;
        if (vtx->is_value_vertex()) {
          ret.reset(allocator());
          copier(*ret, *(vtx->object.lock()));
        } else if (vtx->object_cache.unique()) {
          // it is unique! i.e. we are the only people having a cache to it
          // there are no other external references.
          // lets take over it. Some care is needed here, since we mutating
          // what really is supposed to be immutable.
          // we need to make sure to reset the value pointer as well.
          ret = vtx->object_cache;
          vtx->object_cache.reset();
          vtx->object.reset();
          return ret;
        } else {
          // regular case. There is a still a reference to it, we need to
          // copy it.
          ret.reset(allocator());
          copier(*ret, *(vtx->object.lock()));
        }
        return ret;
      } else {
        return vtx->object.lock();
      }
    }

    if (vtx->parents.size() == 0) {
      // no parents.
      // Create a new object, pass it through the operation and return it.
      std::shared_ptr<value_type> ret;
      ret.reset(allocator());
      vtx->operation->execute(*ret, std::vector<value_type*>());
      // we are not making a copy, so it is safe to remember the weak pointer
      if (!make_copy) vtx->object = ret;
      return ret;
    } else {
      // compute all parents
      // also extract raw pointers from all parents except the first
      std::vector<std::shared_ptr<value_type> > parents_shared_ptr(vtx->parents.size());
      std::vector<value_type*> other_parents_raw_ptr;
      for (size_t i = 0;i < vtx->parents.size(); ++i) {
        // compute parent, we need to make a copy of only the left side of the
        // tree. This can be optimized.
        parents_shared_ptr[i] = preorder_compute(vtx->parents[i], i == 0);
        if (i > 0) other_parents_raw_ptr.push_back(parents_shared_ptr[i].get());
      }
      // set up the call to the operation
      std::shared_ptr<value_type> ret = parents_shared_ptr[0];
      vtx->operation->execute(*ret, other_parents_raw_ptr);
      // memory cleanup
      other_parents_raw_ptr.clear();
      parents_shared_ptr.clear();
      if (!make_copy) vtx->object = ret;
      return ret;
    }
  }




  /**
   * Tries to delete a given vertex. This may result in the making eager of
   * certain vertices to ensure that referenced vertices can always
   * be constructed. A vertex is only uncached if it can be deleted.
   *
   * \param vertex_id Vertex to delete
   * \param avoid_instantiation Cancel the deletion if it involves instantiating
   *                            an as yet, uninstantiated vertex.
   */
  void delete_vertex(size_t vertex_id, bool avoid_instantiation = false) {
    if (vertices.count(vertex_id) == 0) return;
    vertex* vtx = vertices.at(vertex_id);
    if (vtx->children.size() == 0) {
      // no children! no issue deleting
      // remove myself from parent's children listing
      for (size_t parentid: vtx->parents) {
        vertex* parent= vertices.at(parentid);
        // delete myself from the parent
        parent->children.erase(std::find(parent->children.begin(),
                                         parent->children.end(),
                                         vertex_id));
      }
      // now we can clear the current object
      delete vtx;
      vertices.erase(vertex_id);
    } else if (vtx->parents.size() == 0 && vtx->children.size() == 1) {
      // ok. we can actually delete this now.
      // But we would preferably like to avoid "splits" where I have to
      // instantiate many children.
      // keep going downwards until I find a split, or a not deleted vertex
      // nd make that eager.
      size_t deepest_child = vertex_id;
      vertex* deepest_child_vtx = vtx;
      while(deepest_child_vtx->to_delete &&
            deepest_child_vtx->children.size() == 1) {
        deepest_child = deepest_child_vtx->children[0];
        deepest_child_vtx= vertices.at(deepest_child);
      }
      // make eager that element
      if (avoid_instantiation && !deepest_child_vtx->object_cache) return;
      auto deepest_child_value = make_eager(deepest_child);
      deepest_child_vtx->object_cache = deepest_child_value;
      deepest_child_vtx->object = deepest_child_value;

      // now we can delete every vertex up to the deepest child
      size_t deletion_cur_id = vertex_id;
      vertex* deletion_cur_vtx = vtx;
      do {
        // detach this vertex
        size_t next_child = deletion_cur_vtx->children[0];
        vertex* next_child_vtx = vertices.at(next_child);

        next_child_vtx->parents.erase(std::find(next_child_vtx->parents.begin(),
                                                next_child_vtx->parents.end(),
                                                deletion_cur_id));
        delete deletion_cur_vtx;
        vertices.erase(deletion_cur_id);
        deletion_cur_id = next_child;
        deletion_cur_vtx = next_child_vtx;
      }while(deletion_cur_vtx->to_delete && deletion_cur_vtx->children.size() == 1);
    }
  }
};

} // turicreate

template <typename T>
std::ostream& operator<<(std::ostream& out,
                         const turi::lazy_eval_operation_dag<T>& dag) {
  dag.print(out);
  return out;
}

#endif
