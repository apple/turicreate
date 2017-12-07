/*!
 *  Copyright (c) 2016 by Contributors
 * \file Range.h
 * \brief The Range data structure
 */
#ifndef HALIDE_IR_RANGE_H_
#define HALIDE_IR_RANGE_H_

#include <memory>
#include "Expr.h"

namespace Halide {
namespace IR {

// Internal node container of Range
class RangeNode;

/*! \brief Node range */
class Range : public NodeRef {
 public:
  /*! \brief constructor */
  Range() {}
  Range(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const RangeNode* operator->() const;
  /*! \brief specify container node */
  using ContainerType = RangeNode;
  /*!
   * \brief construct a new range with min and extent
   *  The corresponding constructor is removed,
   *  because that is counter convention of tradition meaning
   *  of range(begin, end)
   *
   * \param min The minimum range.
   * \param extent The extent of the range.
   */
  static inline Range make_by_min_extent(Expr min, Expr extent);
};

/*! \brief range over one dimension */
class RangeNode : public Node {
 public:
  /*! \brief beginning of the node */
  Expr min;
  /*! \brief the extend of range */
  Expr extent;
  /*! \brief constructor */
  RangeNode() {}
  RangeNode(Expr min, Expr extent) : min(min), extent(extent) {}

  void VisitAttrs(IR::AttrVisitor* v) final {
    v->Visit("min", &min);
    v->Visit("extent", &extent);
  }

  static constexpr const char* _type_key = "Range";
  TVM_DECLARE_NODE_TYPE_INFO(RangeNode, Node);
};

// implements of inline functions
inline const RangeNode* Range::operator->() const {
  return static_cast<const RangeNode*>(node_.get());
}

inline Range Range::make_by_min_extent(Expr min, Expr extent) {
  internal_assert(min.type() == extent.type())
      << "Region min and extent must have same type\n";
  return Range(std::make_shared<RangeNode>(min, extent));
}

// overload print function
inline std::ostream& operator<<(std::ostream &os, const Range& r) {  // NOLINT(*)
  os << "Range(min=" << r->min << ", extent=" << r->extent <<')';
  return os;
}

}  // namespace IR
}  // namespace Halide

#endif  // HALIDE_IR_H_
