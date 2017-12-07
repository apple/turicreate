/*!
 *  Copyright (c) 2016 by Contributors
 * \file container.h
 * \brief Array/Map container in the DSL graph.
 */
#ifndef TVM_CONTAINER_H_
#define TVM_CONTAINER_H_

#include <type_traits>
#include <vector>
#include <initializer_list>
#include <unordered_map>
#include <utility>
#include "./node.h"

namespace tvm {

/*! \brief array node content in array */
class ArrayNode : public Node {
 public:
  /*! \brief the data content */
  std::vector<std::shared_ptr<Node> > data;

  void VisitAttrs(AttrVisitor* visitor) final {
     // Visitor to array have no effect.
  }

  static constexpr const char* _type_key = "Array";
  TVM_DECLARE_NODE_TYPE_INFO(ArrayNode, Node);
};

/*! \brief map node content in array */
class MapNode : public Node {
 public:
  void VisitAttrs(AttrVisitor* visitor) final {
     // Visitor to map have no effect.
  }
  // hash function
  struct Hash {
    size_t operator()(const std::shared_ptr<Node>& n) const {
      return std::hash<Node*>()(n.get());
    }
  };
  // comparator
  struct Equal {
    bool operator()(
        const std::shared_ptr<Node>& a,
        const std::shared_ptr<Node>& b) const {
      return a.get() == b.get();
    }
  };

  /*! \brief The corresponding conatiner type */
  using ContainerType = std::unordered_map<
   std::shared_ptr<Node>,
   std::shared_ptr<Node>,
   Hash, Equal>;

  /*! \brief the data content */
  ContainerType data;

  static constexpr const char* _type_key = "Map";
  TVM_DECLARE_NODE_TYPE_INFO(MapNode, Node);
};

/*!
 * \brief iterator adapter that adapts TIter to return another type.
 * \tparam Converter a struct that contains converting function
 * \tparam TIter the content iterator type.
 */
template<typename Converter,
         typename TIter>
class IterAdapter {
 public:
  explicit IterAdapter(TIter iter) : iter_(iter) {}
  inline IterAdapter& operator++() {  // NOLINT(*)
    ++iter_;
    return *this;
  }
  inline IterAdapter& operator++(int) {  // NOLINT(*)
    ++iter_;
    return *this;
  }
  inline IterAdapter operator+(int offset) const {  // NOLINT(*)
    return IterAdapter(iter_ + offset);
  }
  inline bool operator==(IterAdapter other) const {
    return iter_ == other.iter_;
  }
  inline bool operator!=(IterAdapter other) const {
    return !(*this == other);
  }
  inline const typename Converter::ResultType operator*() const {
    return Converter::convert(*iter_);
  }

 private:
  TIter iter_;
};

/*!
 * \brief Array container of NodeRef in DSL graph.
 *  Array implements copy on write semantics, which means array is mutable
 *  but copy will happen when array is referenced in more than two places.
 *
 * operator[] only provide const acces, use Set to mutate the content.
 * \tparam T The content NodeRef type.
 */
template<typename T,
         typename = typename std::enable_if<std::is_base_of<NodeRef, T>::value>::type >
class Array : public NodeRef {
 public:
  /*!
   * \brief default constructor
   */
  Array() {
    node_ = std::make_shared<ArrayNode>();
  }
  /*!
   * \brief move constructor
   * \param other source
   */
  Array(Array<T> && other) {  // NOLINT(*)
    node_ = std::move(other.node_);
  }
  /*!
   * \brief copy constructor
   * \param other source
   */
  Array(const Array<T> &other) { // NOLINT(*)
    node_ = other.node_;
  }
  /*!
   * \brief constructor from pointer
   * \param n the container pointer
   */
  explicit Array(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief constructor from iterator
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  Array(IterType begin, IterType end) {
    assign(begin, end);
  }
  /*!
   * \brief constructor from initializer list
   * \param init The initalizer list
   */
  Array(std::initializer_list<T> init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief constructor from vector
   * \param init The vector
   */
  Array(const std::vector<T>& init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief move assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(Array<T> && other) {
    node_ = std::move(other.node_);
    return *this;
  }
  /*!
   * \brief copy assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(const Array<T> & other) {
    node_ = other.node_;
    return *this;
  }
  /*!
   * \brief reset the array to content from iterator.
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  void assign(IterType begin, IterType end) {
    auto n = std::make_shared<ArrayNode>();
    for (IterType it = begin; it != end; ++it) {
      n->data.push_back((*it).node_);
    }
    node_ = std::move(n);
  }
  /*!
   * \brief Read i-th element from array.
   * \param i The index
   * \return the i-th element.
   */
  inline const T operator[](size_t i) const {
    return T(static_cast<const ArrayNode*>(node_.get())->data[i]);
  }
  /*! \return The size of the array */
  inline size_t size() const {
    if (node_.get() == nullptr) return 0;
    return static_cast<const ArrayNode*>(node_.get())->data.size();
  }
  /*!
   * \brief copy on write semantics
   *  Do nothing if current handle is the unique copy of the array.
   *  Otherwise make a new copy of the array to ensure the current handle
   *  hold a unique copy.
   *
   * \return Handle to the internal node container(which ganrantees to be unique)
   */
  inline ArrayNode* CopyOnWrite() {
    if (node_.get() == nullptr || !node_.unique())  {
      node_ = std::make_shared<ArrayNode>(
          *static_cast<const ArrayNode*>(node_.get()));
    }
    return static_cast<ArrayNode*>(node_.get());
  }
  /*!
   * \brief push a new item to the back of the list
   * \param item The item to be pushed.
   */
  inline void push_back(const T& item) {
    ArrayNode* n = this->CopyOnWrite();
    n->data.push_back(item.node_);
  }
  /*!
   * \brief set i-th element of the array.
   * \param i The index
   * \param value The value to be setted.
   */
  inline void Set(size_t i, const T& value) {
    ArrayNode* n = this->CopyOnWrite();
    n->data[i] = value.node_;
  }
  /*! \return whether array is empty */
  inline bool empty() const {
    return size() == 0;
  }
  /*! \brief specify container node */
  using ContainerType = ArrayNode;

  struct Ptr2NodeRef {
    using ResultType = T;
    static inline T convert(const std::shared_ptr<Node>& n) {
      return T(n);
    }
  };
  using iterator = IterAdapter<Ptr2NodeRef,
                               std::vector<std::shared_ptr<Node> >::const_iterator>;

  using reverse_iterator = IterAdapter<
    Ptr2NodeRef,
    std::vector<std::shared_ptr<Node> >::const_reverse_iterator>;

  /*! \return begin iterator */
  inline iterator begin() const {
    return iterator(static_cast<const ArrayNode*>(node_.get())->data.begin());
  }
  /*! \return end iterator */
  inline iterator end() const {
    return iterator(static_cast<const ArrayNode*>(node_.get())->data.end());
  }
  /*! \return rbegin iterator */
  inline reverse_iterator rbegin() const {
    return reverse_iterator(static_cast<const ArrayNode*>(node_.get())->data.rbegin());
  }
  /*! \return rend iterator */
  inline reverse_iterator rend() const {
    return reverse_iterator(static_cast<const ArrayNode*>(node_.get())->data.rend());
  }
};

/*!
 * \brief Map container of NodeRef->NodeRef in DSL graph.
 *  Map implements copy on write semantics, which means map is mutable
 *  but copy will happen when array is referenced in more than two places.
 *
 * operator[] only provide const acces, use Set to mutate the content.
 * \tparam K The key NodeRef type.
 * \tparam V The value NodeRef type.
 */
template<typename K,
         typename V,
         typename = typename std::enable_if<std::is_base_of<NodeRef, K>::value>::type,
         typename = typename std::enable_if<std::is_base_of<NodeRef, V>::value>::type>
class Map : public NodeRef {
 public:
  /*!
   * \brief default constructor
   */
  Map() {
    node_ = std::make_shared<MapNode>();
  }
  /*!
   * \brief move constructor
   * \param other source
   */
  Map(Map<K, V> && other) {  // NOLINT(*)
    node_ = std::move(other.node_);
  }
  /*!
   * \brief copy constructor
   * \param other source
   */
  Map(const Map<K, V> &other) { // NOLINT(*)
    node_ = other.node_;
  }
  /*!
   * \brief constructor from pointer
   * \param n the container pointer
   */
  explicit Map(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief constructor from iterator
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  Map(IterType begin, IterType end) {
    assign(begin, end);
  }
  /*!
   * \brief constructor from initializer list
   * \param init The initalizer list
   */
  Map(std::initializer_list<std::pair<K, V> > init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief constructor from vector
   * \param init The vector
   */
  template<typename Hash, typename Equal>
  Map(const std::unordered_map<K, V, Hash, Equal>& init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief move assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Map<K, V>& operator=(Map<K, V> && other) {
    node_ = std::move(other.node_);
    return *this;
  }
  /*!
   * \brief copy assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Map<K, V>& operator=(const Map<K, V> & other) {
    node_ = other.node_;
    return *this;
  }
  /*!
   * \brief reset the array to content from iterator.
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  void assign(IterType begin, IterType end) {
    auto n = std::make_shared<MapNode>();
    for (IterType i = begin; i != end; ++i) {
      n->data.emplace(std::make_pair(i->first.node_,
                                     i->second.node_));
    }
    node_ = std::move(n);
  }
  /*!
   * \brief Read element from map.
   * \param key The key
   * \return the corresonding element.
   */
  inline const V operator[](const K& key) const {
    return V(static_cast<const MapNode*>(node_.get())->data.at(key.node_));
  }
  /*!
   * \brief Read element from map.
   * \param key The key
   * \return the corresonding element.
   */
  inline const V at(const K& key) const {
    return V(static_cast<const MapNode*>(node_.get())->data.at(key.node_));
  }
  /*! \return The size of the array */
  inline size_t size() const {
    if (node_.get() == nullptr) return 0;
    return static_cast<const MapNode*>(node_.get())->data.size();
  }
  /*! \return The size of the array */
  inline size_t count(const K& key) const {
    if (node_.get() == nullptr) return 0;
    return static_cast<const MapNode*>(node_.get())->data.count(key.node_);
  }
  /*!
   * \brief copy on write semantics
   *  Do nothing if current handle is the unique copy of the array.
   *  Otherwise make a new copy of the array to ensure the current handle
   *  hold a unique copy.
   *
   * \return Handle to the internal node container(which ganrantees to be unique)
   */
  inline MapNode* CopyOnWrite() {
    if (node_.get() == nullptr || !node_.unique())  {
      node_ = std::make_shared<MapNode>(
          *static_cast<const MapNode*>(node_.get()));
    }
    return static_cast<MapNode*>(node_.get());
  }
  /*!
   * \brief set the Map.
   * \param key The index key.
   * \param value The value to be setted.
   */
  inline void Set(const K& key, const V& value) {
    MapNode* n = this->CopyOnWrite();
    n->data[key.node_] = value.node_;
  }

  /*! \return whether array is empty */
  inline bool empty() const {
    return size() == 0;
  }
  /*! \brief specify container node */
  using ContainerType = MapNode;

  struct Ptr2NodeRef {
    using ResultType = std::pair<K, V>;
    static inline ResultType convert(const std::pair<
                            std::shared_ptr<Node>,
                            std::shared_ptr<Node> >& n) {
      return std::make_pair(K(n.first), V(n.second));
    }
  };

  using iterator = IterAdapter<
    Ptr2NodeRef, MapNode::ContainerType::const_iterator>;

  /*! \return begin iterator */
  inline iterator begin() const {
    return iterator(static_cast<const MapNode*>(node_.get())->data.begin());
  }
  /*! \return end iterator */
  inline iterator end() const {
    return iterator(static_cast<const MapNode*>(node_.get())->data.end());
  }
  /*! \return begin iterator */
  inline iterator find(const K& key) const {
    return iterator(static_cast<const MapNode*>(node_.get())->data.find(key.node_));
  }
};

}  // namespace tvm

namespace Halide {
namespace IR {

using tvm::Array;
using tvm::Map;

}  // namespace IR
}  // namespace Halide

#endif  // TVM_CONTAINER_H_
