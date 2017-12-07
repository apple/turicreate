#ifndef HALIDE_EXPR_H
#define HALIDE_EXPR_H

/** \file
 * Base classes for Halide expressions (\ref Halide::Expr) and statements (\ref Halide::Internal::Stmt)
 */

#include <string>
#include <vector>

#include "base/Debug.h"
#include "base/Error.h"
#include "base/Float16.h"
#include "base/Type.h"
#include "base/Util.h"
#include "tvm/node.h"
#include "tvm/ir_functor.h"
#include "tvm/container.h"

namespace Halide {
namespace Internal {

using IR::Node;
using IR::NodeRef;
using IR::Array;

struct Variable;
class IRVisitor;

/** All our IR node types get unique IDs for the purposes of RTTI */
enum class IRNodeType : int {
    IntImm,
    UIntImm,
    FloatImm,
    StringImm,
    Cast,
    Variable,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Min,
    Max,
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,
    And,
    Or,
    Not,
    Select,
    Load,
    Ramp,
    Broadcast,
    Call,
    Let,
    LetStmt,
    AssertStmt,
    ProducerConsumer,
    For,
    Store,
    Provide,
    Allocate,
    Free,
    Realize,
    Block,
    IfThenElse,
    Evaluate,
    Shuffle,
    Prefetch,
    AttrStmt,
    ExtensionExpr
};

/** The abstract base classes for a node in the Halide IR. */
struct IRNode : public Node {
    /** Each IR node subclass should return some unique pointer. We
     * can compare these pointers to do runtime type
     * identification. We don't compile with rtti because that
     * injects run-time type identification stuff everywhere (and
     * often breaks when linking external libraries compiled
     * without it), and we only want it for IR nodes. */
    virtual IRNodeType type_info() const = 0;
};

/** IR nodes are split into expressions and statements. These are
   similar to expressions and statements in C - expressions
   represent some value and have some type (e.g. x + 3), and
   statements are side-effecting pieces of code that do not
   represent a value (e.g. assert(x > 3)) */

/** A base class for statement nodes. They have no properties or
   methods beyond base IR nodes for now */
struct BaseStmtNode : public IRNode {
    /** We use the visitor pattern to traverse IR nodes throughout the
     * compiler, so we have a virtual accept method which accepts
     * visitors.
     */
    virtual void accept(IRVisitor *v, const Stmt &s) const = 0;
    // friendly type message
    static constexpr const char* _type_key = "Stmt";

    TVM_DECLARE_BASE_NODE_INFO(BaseStmtNode, Node);
};

/** A base class for expression nodes. They all contain their types
 * (e.g. Int(32), Float(32)) */
struct BaseExprNode : public IRNode {
    Type type;
    /** We use the visitor pattern to traverse IR nodes throughout the
     * compiler, so we have a virtual accept method which accepts
     * visitors.
     */
    virtual void accept(IRVisitor *v, const Expr &e) const = 0;
    // friendly type message
    static constexpr const char* _type_key = "Expr";

    TVM_DECLARE_BASE_NODE_INFO(BaseExprNode, Node);
};

/** We use the "curiously recurring template pattern" to avoid
   duplicated code in the IR Nodes. These classes live between the
   abstract base classes and the actual IR Nodes in the
   inheritance hierarchy. It provides an implementation of the
   accept function necessary for the visitor pattern to work, and
   a concrete instantiation of a unique IRNodeType per class. */
template<typename T>
struct ExprNode : public BaseExprNode {
    EXPORT void accept(IRVisitor *v, const Expr &e) const;
    IRNodeType type_info() const final {return T::_type_info;}

    TVM_DECLARE_NODE_TYPE_INFO(T, BaseExprNode);
};

template<typename T>
struct StmtNode : public BaseStmtNode {
    EXPORT void accept(IRVisitor *v, const Stmt &s) const;
    IRNodeType type_info() const final {return T::_type_info;}

    TVM_DECLARE_NODE_TYPE_INFO(T, BaseStmtNode);
};

/** IR nodes are passed around opaque handles to them. This is a
   base class for those handles. It manages the reference count,
   and dispatches visitors. */
struct IRHandle : public NodeRef {
    IRHandle() {}
    IRHandle(std::shared_ptr<Node> p) : NodeRef(p) {}

    /** return internal content as IRNode */
    inline const IRNode* get() const {
        return static_cast<const IRNode*>(node_.get());
    }
    /** return internal content as IRNode */
    inline const IRNode* operator->() const {
        return static_cast<const IRNode*>(node_.get());
    }
};
}  // namespace Internal

/** A fragment of Halide syntax. It's implemented as reference-counted
 * handle to a concrete expression node, but it's immutable, so you
 * can treat it as a value type. */
struct Expr : public Internal::IRHandle {
    /** Make an undefined expression */
    Expr() : Internal::IRHandle() {}

    /** Make an expression from a concrete expression node pointer (e.g. Add) */
    explicit Expr(std::shared_ptr<IR::Node> n) : IRHandle(n) {}

    /** Make an expression representing numeric constants of various types. */
    // @{
    EXPORT explicit Expr(int8_t x);
    EXPORT explicit Expr(int16_t x);
    EXPORT          Expr(int32_t x);
    EXPORT explicit Expr(int64_t x);
    EXPORT explicit Expr(uint8_t x);
    EXPORT explicit Expr(uint16_t x);
    EXPORT explicit Expr(uint32_t x);
    EXPORT explicit Expr(uint64_t x);
    EXPORT          Expr(float16_t x);
    EXPORT          Expr(float x);
    EXPORT explicit Expr(double x);
    // @}

    /** Make an expression representing a const string (i.e. a StringImm) */
    // Ree
    EXPORT          Expr(const std::string &s);

    /** Dispatch to the correct visitor method for this node. E.g. if
     * this node is actually an Add node, then this will call
     * IRVisitor::visit(const Add *) */
    inline void accept(Internal::IRVisitor *v) const {
        static_cast<const Internal::BaseExprNode *>(node_.get())->accept(v, *this);
    }

    /** Get the type of this expression node */
    Type type() const {
      return (static_cast<const Internal::BaseExprNode *>(node_.get()))->type;
    }
    /*! \brief type indicate the container type */
    using ContainerType = Internal::BaseExprNode;
};

/** This lets you use an Expr as a key in a map of the form
 * map<Expr, Foo, ExprCompare> */
struct ExprCompare {
    bool operator()(const Expr& a, const Expr& b) const {
        return a.get() < b.get();
    }
};

/** This lets you use an Expr as a key in a unordered_map of the form
 * unordered_map<Expr, Foo, ExprHash, ExprEqual> */
struct ExprHash {
    size_t operator()(const Expr& a) const {
        return a.hash();
    }
};

/** This lets you use an Expr as a key in a unordered_map of the form
 * unordered_map<Expr, Foo, ExprHash, ExprEqual> */
struct ExprEqual {
    bool operator()(const Expr& a, const Expr& b) const {
        return a.get() == b.get();
    }
};

/**
 * A subclass of Expr that only refers to a Variable
 *
 * Avoid use the Var to confuse with Halide's Var in high level DSL.
 */
struct VarExpr : public Expr {
    VarExpr() : Expr() { }
    explicit VarExpr(std::shared_ptr<IR::Node> n) : Expr(n) {}
    /**
     * constructor from variable
     * Choose first have name then type, with default int32
     * because most VarExpr are used as looping variable.
     */
    explicit VarExpr(const std::string &name_hint, Type t = Int(32));
    /** return internal content as Variable */
    inline const Internal::Variable* get() const;
    /** return internal variable pointer */
    inline const Internal::Variable* operator->() const;
};

/** An enum describing a type of device API. Used by schedules, and in
 * the For loop IR node. */
enum class DeviceAPI : int {
    None = 0, /// Used to denote for loops that run on the same device as the containing code.
    Host,
    Default_GPU,
    CUDA,
    OpenCL,
    GLSL,
    OpenGLCompute,
    Metal,
    Hexagon
};

namespace Internal {

/** An enum describing a type of loop traversal. Used in schedules,
 * and in the For loop IR node. */
enum class ForType : int {
    Serial = 0,
    Parallel = 1,
    Vectorized = 2,
    Unrolled = 3
};


/** A reference-counted handle to a statement node. */
struct Stmt : public IRHandle {
    Stmt() : IRHandle() {}
    Stmt(std::shared_ptr<IR::Node> n) : IRHandle(n) {}

    /** Dispatch to the correct visitor method for this node. E.g. if
     * this node is actually an Add node, then this will call
     * IRVisitor::visit(const Add *) */
    inline void accept(Internal::IRVisitor *v) const {
        static_cast<const Internal::BaseStmtNode *>(node_.get())->accept(v, *this);
    }
    /*! \brief type indicate the container type */
    using ContainerType = Internal::BaseStmtNode;
};


}  // namespace Internal
}  // namespace Halide

namespace Halide {
namespace IR {
using ::Halide::Expr;
using Internal::Stmt;
}  // namespace IR
}  // namespace Stmt

namespace std {
template <>
struct hash<::Halide::Expr> {
  std::size_t operator()(const ::Halide::Expr& k) const {
    return k.hash();
  }
};
template <>
struct hash<::Halide::Internal::Stmt> {
  std::size_t operator()(const ::Halide::Internal::Stmt& k) const {
    return k.hash();
  }
};
}
#endif
