/*!
 *  Copyright (c) 2016 by Contributors
 * \file FunctionBase.h
 * \brief The function reference data structure to hold the function without defining it.
 *
 *  This is used because Function is a high level object that can contain schedule,
 *  which could have many variations. Removing FunctionContent dep from IR makes IR minimum.
 */
#ifndef HALIDE_IR_FUNCTION_BASE_H_
#define HALIDE_IR_FUNCTION_BASE_H_

#include <memory>
#include "Expr.h"

namespace Halide {
namespace IR {

// Internal node container of Range
class FunctionBaseNode;

/*! \brief reference to a function */
class FunctionRef : public NodeRef {
 public:
  /*! \brief constructor */
  FunctionRef() {}
  FunctionRef(std::shared_ptr<Node> n) : NodeRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const FunctionBaseNode* operator->() const;
};

/*! \brief range over one dimension */
class FunctionBaseNode : public Node {
 public:
  /*! \return the name of the function */
  virtual const std::string& func_name() const = 0;
  /*! \return the number of outputs of this function */
  virtual int num_outputs() const = 0;
};

// implements of inline functions
inline const FunctionBaseNode* FunctionRef::operator->() const {
  return static_cast<const FunctionBaseNode*>(node_.get());
}

}  // namespace IR
}  // namespace Halide

#endif  // HALIDE_IR_FUNCTION_BASE_H_
