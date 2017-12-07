#ifndef HALIDE_IR_H
#define HALIDE_IR_H

/** \file
 * Subtypes for Halide expressions (\ref Halide::Expr) and statements (\ref Halide::Internal::Stmt)
 */

#include <string>
#include <vector>

#include "base/Debug.h"
#include "base/Error.h"
#include "base/Type.h"
#include "base/Util.h"
#include "Expr.h"
#include "Range.h"
#include "FunctionBase.h"

namespace Halide {
namespace Internal {

using IR::FunctionRef;
using IR::Range;

/** A multi-dimensional box. The outer product of the elements */
using Region = Array<Range>;


/** The actual IR nodes begin here. Remember that all the Expr
 *  nodes also have a public "type" property
 *
 *  These are exposed as dtype to DSL front-end to avoid confusion.
 */

/** Integer constants */
struct IntImm : public ExprNode<IntImm> {
    int64_t value;

    static Expr make(Type t, int64_t value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::IntImm;
    static constexpr const char* _type_key = "IntImm";
};

/** Unsigned integer constants */
struct UIntImm : public ExprNode<UIntImm> {
    uint64_t value;

    static Expr make(Type t, uint64_t value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::UIntImm;
    static constexpr const char* _type_key = "UIntImm";
};

/** Floating point constants */
struct FloatImm : public ExprNode<FloatImm> {
    double value;

    static Expr make(Type t, double value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::FloatImm;
    static constexpr const char* _type_key = "FloatImm";
};

/** String constants */
struct StringImm : public ExprNode<StringImm> {
    std::string value;

    Expr static make(const std::string &val);
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::StringImm;
    static constexpr const char* _type_key = "StringImm";
};

/** Cast a node from one type to another. Can't change vector widths. */
struct Cast : public ExprNode<Cast> {
    Expr value;

    EXPORT static Expr make(Type t, Expr v);
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::Cast;
    static constexpr const char* _type_key = "Cast";
};

/** base class of all Binary arithematic ops */
template<typename T>
struct BinaryOpNode : public ExprNode<T> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b) {
       internal_assert(a.defined()) << "BinaryOp of undefined\n";
       internal_assert(b.defined()) << "BinaryOp of undefined\n";
       internal_assert(a.type() == b.type()) << "BinaryOp of mismatched types\n";
       std::shared_ptr<T> node = std::make_shared<T>();
       node->type = a.type();
       node->a = std::move(a);
       node->b = std::move(b);
       return Expr(node);
    }
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
};

/** The sum of two expressions */
struct Add : public BinaryOpNode<Add> {
    static const IRNodeType _type_info = IRNodeType::Add;
    static constexpr const char* _type_key = "Add";
};

/** The difference of two expressions */
struct Sub : public BinaryOpNode<Sub> {
    static const IRNodeType _type_info = IRNodeType::Sub;
    static constexpr const char* _type_key = "Sub";
};

/** The product of two expressions */
struct Mul : public BinaryOpNode<Mul> {
    static const IRNodeType _type_info = IRNodeType::Mul;
    static constexpr const char* _type_key = "Mul";
};

/** The ratio of two expressions */
struct Div : public BinaryOpNode<Div> {
    static const IRNodeType _type_info = IRNodeType::Div;
    static constexpr const char* _type_key = "Div";
};

/** The remainder of a / b. Mostly equivalent to '%' in C, except that
 * the result here is always positive. For floats, this is equivalent
 * to calling fmod. */
struct Mod : public BinaryOpNode<Mod> {
    static const IRNodeType _type_info = IRNodeType::Mod;
    static constexpr const char* _type_key = "Mod";
};

/** The lesser of two values. */
struct Min : public BinaryOpNode<Min> {
    static const IRNodeType _type_info = IRNodeType::Min;
    static constexpr const char* _type_key = "Min";
};

/** The greater of two values */
struct Max : public BinaryOpNode<Max> {
    static const IRNodeType _type_info = IRNodeType::Max;
    static constexpr const char* _type_key = "Max";
};

/** base class of all comparison ops */
template<typename T>
struct CmpOpNode : public ExprNode<T> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b) {
        internal_assert(a.defined()) << "CmpOp of undefined\n";
        internal_assert(b.defined()) << "CmpOp of undefined\n";
        internal_assert(a.type() == b.type()) << "BinaryOp of mismatched types\n";
        std::shared_ptr<T> node = std::make_shared<T>();
        node->type = Bool(a.type().lanes());
        node->a = std::move(a);
        node->b = std::move(b);
        return Expr(node);
    }
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
};

/** Is the first expression equal to the second */
struct EQ : public CmpOpNode<EQ> {
    static const IRNodeType _type_info = IRNodeType::EQ;
    static constexpr const char* _type_key = "EQ";
};

/** Is the first expression not equal to the second */
struct NE : public CmpOpNode<NE> {
    static const IRNodeType _type_info = IRNodeType::NE;
    static constexpr const char* _type_key = "NE";
};

/** Is the first expression less than the second. */
struct LT : public CmpOpNode<LT> {
    static const IRNodeType _type_info = IRNodeType::LT;
    static constexpr const char* _type_key = "LT";
};

/** Is the first expression less than or equal to the second. */
struct LE : public CmpOpNode<LE> {
    static const IRNodeType _type_info = IRNodeType::LE;
    static constexpr const char* _type_key = "LE";
};

/** Is the first expression greater than the second. */
struct GT : public CmpOpNode<GT> {
    static const IRNodeType _type_info = IRNodeType::GT;
    static constexpr const char* _type_key = "GT";
};

/** Is the first expression greater than or equal to the second. */
struct GE : public CmpOpNode<GE> {
    static const IRNodeType _type_info = IRNodeType::GE;
    static constexpr const char* _type_key = "GE";
};

/** Logical and - are both expressions true */
struct And : public ExprNode<And> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
    static const IRNodeType _type_info = IRNodeType::And;
    static constexpr const char* _type_key = "And";
};

/** Logical or - is at least one of the expression true */
struct Or : public ExprNode<Or> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
    static const IRNodeType _type_info = IRNodeType::Or;
    static constexpr const char* _type_key = "Or";
};

/** Logical not - true if the expression false */
struct Not : public ExprNode<Not> {
    Expr a;

    EXPORT static Expr make(Expr a);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("a", &a);
    }
    static const IRNodeType _type_info = IRNodeType::Not;
    static constexpr const char* _type_key = "Not";
};

/** A ternary operator. Evalutes 'true_value' and 'false_value',
 * then selects between them based on 'condition'. Equivalent to
 * the ternary operator in C. */
struct Select : public ExprNode<Select> {
    Expr condition, true_value, false_value;

    EXPORT static Expr make(Expr condition, Expr true_value, Expr false_value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("condition", &condition);
        v->Visit("true_value", &true_value);
        v->Visit("false_value", &false_value);
    }
    static const IRNodeType _type_info = IRNodeType::Select;
    static constexpr const char* _type_key = "Select";
};

/** Load a value from a buffer. The buffer is treated as an
 * array of the 'type' of this Load node. That is, the buffer has
 * no inherent type. */
struct Load : public ExprNode<Load> {
    VarExpr buffer_var;
    Expr index, predicate;

    EXPORT static Expr make(Type type, VarExpr buffer_var, Expr index, Expr predicate);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("buffer_var", &buffer_var);
        v->Visit("index", &index);
        v->Visit("predicate", &predicate);
    }
    static const IRNodeType _type_info = IRNodeType::Load;
    static constexpr const char* _type_key = "Load";
};

/** A linear ramp vector node. This is vector with 'lanes' elements,
 * where element i is 'base' + i*'stride'. This is a convenient way to
 * pass around vectors without busting them up into individual
 * elements. E.g. a dense vector load from a buffer can use a ramp
 * node with stride 1 as the index. */
struct Ramp : public ExprNode<Ramp> {
    Expr base, stride;
    int lanes;

    EXPORT static Expr make(Expr base, Expr stride, int lanes);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("base", &base);
        v->Visit("stride", &stride);
        v->Visit("lanes", &lanes);
    }
    static const IRNodeType _type_info = IRNodeType::Ramp;
    static constexpr const char* _type_key = "Ramp";
};

/** A vector with 'lanes' elements, in which every element is
 * 'value'. This is a special case of the ramp node above, in which
 * the stride is zero. */
struct Broadcast : public ExprNode<Broadcast> {
    Expr value;
    int lanes;

    EXPORT static Expr make(Expr value, int lanes);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("value", &value);
        v->Visit("lanes", &lanes);
    }
    static const IRNodeType _type_info = IRNodeType::Broadcast;
    static constexpr const char* _type_key = "Broadcast";
};

/** A let expression, like you might find in a functional
 * language. Within the expression \ref Let::body, instances of the Var
 * node \ref Let::var refer to \ref Let::value. */
struct Let : public ExprNode<Let> {
    VarExpr var;
    Expr value, body;

    EXPORT static Expr make(VarExpr var, Expr value, Expr body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("var", &var);
        v->Visit("value", &value);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::Let;
    static constexpr const char* _type_key = "Let";
};

/** The statement form of a let node. Within the statement 'body',
 * instances of the Var refer to 'value'
 */
struct LetStmt : public StmtNode<LetStmt> {
    VarExpr var;
    Expr value;
    Stmt body;

    EXPORT static Stmt make(VarExpr var, Expr value, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("var", &var);
        v->Visit("value", &value);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::LetStmt;
    static constexpr const char* _type_key = "LetStmt";
};

/*!
 * \brief Define certain auxiliary attribute for the body to be a symbolic value.
 *  This provide auxiliary information for IR passes that transforms body.
 *
 *  In terms of effect, this is equivalent to Block(Evaluate(value), body).
 *
 *  Examples of possible usage:
 *    - Bound of function, variables.
 *    - Hint which block corresponds to a parallel region.
 */
struct AttrStmt : public StmtNode<AttrStmt> {
    /*! \brief this is attribute about certain node */
    NodeRef node;
    /*! \brief the type key of the attribute */
    std::string attr_key;
    /*! \brief The attribute value, value is well defined at current scope. */
    Expr value;
    /*! \brief The body statement to be executed */
    Stmt body;

    /*! \brief construct expr from name and rdom */
    EXPORT static Stmt make(NodeRef node, std::string type_key, Expr value, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("node", &node);
        v->Visit("attr_key", &attr_key);
        v->Visit("value", &value);
        v->Visit("body", &body);
    }

    static const IRNodeType _type_info = IRNodeType::AttrStmt;
    static constexpr const char* _type_key = "AttrStmt";
};

/** If the 'condition' is false, then evaluate and return the message,
 * which should be a call to an error function. */
struct AssertStmt : public StmtNode<AssertStmt> {
    // if condition then val else error out with message
    Expr condition;
    Expr message;
    // The statement which this assertion holds true.
    // body will get executed immediately after the assert check.
    // The introduction of makes it easy to write visitor pattern
    // that takes benefit of assertion invariant.
    // We can simply add the invariant when we see the assertion
    // and remove it after we visit body.
    Stmt body;

    EXPORT static Stmt make(Expr condition, Expr message, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("condition", &condition);
        v->Visit("message", &message);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::AssertStmt;
    static constexpr const char* _type_key = "AssertStmt";
};

/** This node is a helpful annotation to do with permissions. If 'is_produce' is
 * set to true, this represents a producer node which may also contain updates;
 * otherwise, this represents a consumer node. If the producer node contains
 * updates, the body of the node will be a block of 'produce' and 'update'
 * in that order. In a producer node, the access is read-write only (or write
 * only if it doesn't have updates). In a consumer node, the access is read-only.
 * None of this is actually enforced, the node is purely for informative purposes
 * to help out our analysis during lowering. For every unique ProducerConsumer,
 * there is an associated Realize node with the same name that creates the buffer
 * being read from or written to in the body of the ProducerConsumer.
 */
struct ProducerConsumer : public StmtNode<ProducerConsumer> {
    FunctionRef func;
    bool is_producer;
    Stmt body;

    EXPORT static Stmt make(FunctionRef func, bool is_producer, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("func", &func);
        v->Visit("is_producer", &is_producer);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::ProducerConsumer;
    static constexpr const char* _type_key = "ProducerConsumer";
};

/** Store a 'value' to the buffer with handle at a given
 * 'index'. The buffer is interpreted as an array of the same type as
 * 'value'. */
struct Store : public StmtNode<Store> {
    VarExpr buffer_var;
    Expr value, index, predicate;

    EXPORT static Stmt make(VarExpr buffer_var, Expr value, Expr index, Expr predicate);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("buffer_var", &buffer_var);
        v->Visit("value", &value);
        v->Visit("index", &index);
        v->Visit("predicate", &predicate);
    }
    static const IRNodeType _type_info = IRNodeType::Store;
    static constexpr const char* _type_key = "Store";
};

/** This defines the value of a function at a multi-dimensional
 * location. You should think of it as a store to a
 * multi-dimensional array. It gets lowered to a conventional
 * Store node. */
struct Provide : public StmtNode<Provide> {
    FunctionRef func;
    int value_index;
    Expr value;
    Array<Expr> args;

    EXPORT static Stmt make(FunctionRef func, int value_index,
                            Expr value, Array<Expr> args);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("func", &func);
        v->Visit("value_index", &value_index);
        v->Visit("value", &value);
        v->Visit("args", &args);
    }
    static const IRNodeType _type_info = IRNodeType::Provide;
    static constexpr const char* _type_key = "Provide";
};

/** Allocate a scratch area called with the given name, type, and
 * size. The buffer lives for at most the duration of the body
 * statement, within which it is freed. It is an error for an allocate
 * node not to contain a free node of the same buffer. Allocation only
 * occurs if the condition evaluates to true.
 *
 * Each allocate will create a new Variable of type handle,
 * that corresponds to the allocated space
 */
struct Allocate : public StmtNode<Allocate> {
    VarExpr buffer_var;
    Type type;
    Array<Expr> extents;
    Expr condition;

    // These override the code generator dependent malloc and free
    // equivalents if provided. If the new_expr succeeds, that is it
    // returns non-nullptr, the function named be free_function is
    // guaranteed to be called. The free function signature must match
    // that of the code generator dependent free (typically
    // halide_free). If free_function is left empty, code generator
    // default will be called.
    Expr new_expr;
    std::string free_function;
    Stmt body;

    EXPORT static Stmt make(VarExpr buffer_var,
                            Type type,
                            Array<Expr> extents,
                            Expr condition, Stmt body,
                            Expr new_expr = Expr(), std::string free_function = std::string());

    /** A routine to check if the extents are all constants, and if so verify
     * the total size is less than 2^31 - 1. If the result is constant, but
     * overflows, this routine asserts. This returns 0 if the extents are
     * not all constants; otherwise, it returns the total constant allocation
     * size. */
    EXPORT static int32_t constant_allocation_size(const Array<Expr> &extents, const std::string &name);
    EXPORT int32_t constant_allocation_size() const;

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("buffer_var", &buffer_var);
        v->Visit("dtype", &type);
        v->Visit("extents", &extents);
        v->Visit("condition", &condition);
        v->Visit("new_expr", &new_expr);
        v->Visit("free_function", &free_function);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::Allocate;
    static constexpr const char* _type_key = "Allocate";
};

/** Free the resources associated with the given buffer. */
struct Free : public StmtNode<Free> {
    VarExpr buffer_var;

    EXPORT static Stmt make(VarExpr handle);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("buffer_var", &buffer_var);
    }
    static const IRNodeType _type_info = IRNodeType::Free;
    static constexpr const char* _type_key = "Free";
};

/** Allocate a multi-dimensional buffer of the given type and
 * size. Create some scratch memory that will back the function 'name'
 * over the range specified in 'bounds'. The bounds are a vector of
 * (min, extent) pairs for each dimension. Allocation only occurs if
 * the condition evaluates to true. */
struct Realize : public StmtNode<Realize> {
    FunctionRef func;
    int value_index;
    Type type;
    Region bounds;
    Expr condition;
    Stmt body;

    EXPORT static Stmt make(FunctionRef func,
                            int value_index,
                            Type type,
                            Region bounds,
                            Expr condition, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("func", &func);
        v->Visit("value_index", &value_index);
        v->Visit("dtype", &type);
        v->Visit("bounds", &bounds);
        v->Visit("condition", &condition);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::Realize;
    static constexpr const char* _type_key = "Realize";
};

/** A sequence of statements to be executed in-order. 'rest' may be
 * undefined. Used rest.defined() to find out. */
struct Block : public StmtNode<Block> {
    Stmt first, rest;

    EXPORT static Stmt make(Stmt first, Stmt rest);
    EXPORT static Stmt make(const std::vector<Stmt> &stmts);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("first", &first);
        v->Visit("rest", &rest);
    }
    static const IRNodeType _type_info = IRNodeType::Block;
    static constexpr const char* _type_key = "Block";
};

/** An if-then-else block. 'else' may be undefined. */
struct IfThenElse : public StmtNode<IfThenElse> {
    Expr condition;
    Stmt then_case, else_case;

    EXPORT static Stmt make(Expr condition, Stmt then_case, Stmt else_case = Stmt());

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("condition", &condition);
        v->Visit("then_case", &then_case);
        v->Visit("else_case", &else_case);
    }
    static const IRNodeType _type_info = IRNodeType::IfThenElse;
    static constexpr const char* _type_key = "IfThenElse";
};

/** Evaluate and discard an expression, presumably because it has some side-effect. */
struct Evaluate : public StmtNode<Evaluate> {
    Expr value;

    EXPORT static Stmt make(Expr v);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::Evaluate;
    static constexpr const char* _type_key = "Evaluate";
};

/** A function call. This can represent a call to some extern function
 * (like sin), but it's also our multi-dimensional version of a Load,
 * so it can be a load from an input image, or a call to another
 * halide function. These two types of call nodes don't survive all
 * the way down to code generation - the lowering process converts
 * them to Load nodes. */
struct Call : public ExprNode<Call> {
    std::string name;
    Array<Expr> args;
    enum CallType : int {
      Extern = 0,       //< A call to an external C-ABI function, possibly with side-effects
      ExternCPlusPlus = 1, //< A call to an external C-ABI function, possibly with side-effects
      PureExtern = 2,   //< A call to a guaranteed-side-effect-free external function
      Halide = 3,       //< A call to a Func
      Intrinsic = 4,    //< A possibly-side-effecty compiler intrinsic, which has special handling during codegen
      PureIntrinsic = 5 //< A side-effect-free version of the above.
    };
    CallType call_type;

    // Halide uses calls internally to represent certain operations
    // (instead of IR nodes). These are matched by name. Note that
    // these are deliberately char* (rather than std::string) so that
    // they can be referenced at static-initialization time without
    // risking ambiguous initalization order; we use a typedef to simplify
    // declaration.
    typedef const char* const ConstString;

    EXPORT static ConstString debug_to_file,
        reinterpret,
        bitwise_and,
        bitwise_not,
        bitwise_xor,
        bitwise_or,
        shift_left,
        shift_right,
        abs,
        absd,
        rewrite_buffer,
        random,
        lerp,
        popcount,
        count_leading_zeros,
        count_trailing_zeros,
        undef,
        return_second,
        if_then_else,
        glsl_texture_load,
        glsl_texture_store,
        glsl_varying,
        image_load,
        image_store,
        make_struct,
        stringify,
        memoize_expr,
        alloca,
        likely,
        likely_if_innermost,
        register_destructor,
        div_round_to_zero,
        mod_round_to_zero,
        call_cached_indirect_function,
        prefetch,
        signed_integer_overflow,
        indeterminate_expression,
        bool_to_mask,
        cast_mask,
        select_mask,
        extract_mask_element,
        size_of_halide_buffer_t;
    // If it's a call to another halide function, this call node holds
    // onto a pointer to that function for the purposes of reference
    // counting only. Self-references in update definitions do not
    // have this set, to avoid cycles.
    FunctionRef func;

    // If that function has multiple values, which value does this
    // call node refer to?
    int value_index{0};

    EXPORT static Expr make(Type type,
                            std::string name,
                            Array<Expr> args,
                            CallType call_type,
                            FunctionRef func,
                            int value_index);

    static Expr make(Type type,
                     std::string name,
                     Array<Expr> args,
                     CallType call_type) {
        return make(type, name, args, call_type, FunctionRef(), 0);
    }

    /** Check if a call node is pure within a pipeline, meaning that
     * the same args always give the same result, and the calls can be
     * reordered, duplicated, unified, etc without changing the
     * meaning of anything. Not transitive - doesn't guarantee the
     * args themselves are pure. An example of a pure Call node is
     * sqrt. If in doubt, don't mark a Call node as pure. */
    bool is_pure() const {
        return (call_type == PureExtern ||
                call_type == PureIntrinsic);
    }

    bool is_intrinsic(ConstString intrin_name) const {
        return
            ((call_type == Intrinsic ||
              call_type == PureIntrinsic) &&
             name == intrin_name);
    }

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("name", &name);
        v->Visit("args", &args);
        v->Visit("call_type", &call_type);
        v->Visit("func", &func);
        v->Visit("value_index", &value_index);
    }
    static const IRNodeType _type_info = IRNodeType::Call;
    static constexpr const char* _type_key = "Call";
};

/** A named variable. Might be defined in
 * a loop variable, function argument,
 * parameter, reduction variable, or something defined by a Let or
 * LetStmt node.
 *
 * User should define each Variable at only one place(like SSA).
 * e.g do not let same var appear on two lets.
 *
 * IR nodes that defines a VarExpr
 * - Allocate
 * - For
 * - Let
 * - LetStmt
 */
struct Variable : public ExprNode<Variable> {
    /**
     * variable is uniquely identified by its address instead of name
     * This field is renamed to name_hint to make it different from
     * original ref by name convention
     */
    std::string name_hint;

    // BufferPtr and Parameter are removed from IR
    // They can be added back via passing in binding of Variable to specific values.
    // in the final stage of code generation.

    // refing back ReductionVariable from Variable can cause cyclic refs,
    // remove reference to reduction domain here,
    // instead,uses Reduction as a ExprNode

    EXPORT static VarExpr make(Type type, std::string name_hint);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("dtype", &type);
        v->Visit("name", &name_hint);
    }
    static const IRNodeType _type_info = IRNodeType::Variable;
    static constexpr const char* _type_key = "Variable";
};

/** A for loop. Execute the 'body' statement for all values of the
 * variable loop_var from 'min' to 'min + extent'. There are four
 * types of For nodes. A 'Serial' for loop is a conventional
 * one. In a 'Parallel' for loop, each iteration of the loop
 * happens in parallel or in some unspecified order. In a
 * 'Vectorized' for loop, each iteration maps to one SIMD lane,
 * and the whole loop is executed in one shot. For this case,
 * 'extent' must be some small integer constant (probably 4, 8, or
 * 16). An 'Unrolled' for loop compiles to a completely unrolled
 * version of the loop. Each iteration becomes its own
 * statement. Again in this case, 'extent' should be a small
 * integer constant. */
struct For : public StmtNode<For> {
    VarExpr loop_var;
    Expr min, extent;
    ForType for_type;
    DeviceAPI device_api;
    Stmt body;

    EXPORT static Stmt make(VarExpr loop_var,
                            Expr min, Expr extent,
                            ForType for_type,
                            DeviceAPI device_api, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("loop_var", &loop_var);
        v->Visit("min", &min);
        v->Visit("extent", &extent);
        v->Visit("for_type", &for_type);
        v->Visit("device_api", &device_api);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::For;
    static constexpr const char* _type_key = "For";
};

/** Construct a new vector by taking elements from another sequence of
 * vectors. */
struct Shuffle : public ExprNode<Shuffle> {
    Array<Expr> vectors;

    /** Indices indicating which vector element to place into the
     * result. The elements are numbered by their position in the
     * concatenation of the vector argumentss.
     *
     * These indices are guaranteed to be IntImm, use Expr so we can use
     * Array container from TVM
     */
    Array<Expr> indices;

    EXPORT static Expr make(Array<Expr> vectors, Array<Expr> indices);

    /** Convenience constructor for making a shuffle representing an
     * interleaving of vectors of the same length. */
    EXPORT static Expr make_interleave(Array<Expr> vectors);

    /** Convenience constructor for making a shuffle representing a
     * concatenation of the vectors. */
    EXPORT static Expr make_concat(Array<Expr> vectors);

    /** Convenience constructor for making a shuffle representing a
     * contiguous subset of a vector. */
    EXPORT static Expr make_slice(Expr vector, int begin, int stride, int size);

    /** Convenience constructor for making a shuffle representing
     * extracting a single element. */
    EXPORT static Expr make_extract_element(Expr vector, int i);

    /** Check if this shuffle is an interleaving of the vector
     * arguments. */
    EXPORT bool is_interleave() const;

    /** Check if this shuffle is a concatenation of the vector
     * arguments. */
    EXPORT bool is_concat() const;

    /** Check if this shuffle is a contiguous strict subset of the
     * vector arguments, and if so, the offset and stride of the
     * slice. */
    ///@{
    EXPORT bool is_slice() const;
    int slice_begin() const {
        return static_cast<int>(indices[0].as<IntImm>()->value);
    }
    int slice_stride() const {
        return indices.size() >= 2 ?
            static_cast<int>(indices[1].as<IntImm>()->value - indices[0].as<IntImm>()->value) : 1;
    }
    ///@}

    /** Check if this shuffle is extracting a scalar from the vector
     * arguments. */
    EXPORT bool is_extract_element() const;

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("vectors", &vectors);
        v->Visit("indices", &indices);
    }
    static const IRNodeType _type_info = IRNodeType::Shuffle;
    static constexpr const char* _type_key = "Shuffle";
};

/** Represent a multi-dimensional region of a Func or an ImageParam that
 * needs to be prefetched. */
struct Prefetch : public StmtNode<Prefetch> {
    FunctionRef func;
    int value_index;
    Type type;
    Region bounds;

    EXPORT static Stmt make(FunctionRef func,
                            int value_index,
                            Type type,
                            Region bounds);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("func", &func);
        v->Visit("value_index", &value_index);
        v->Visit("type", &type);
        v->Visit("bounds", &bounds);
    }
    static const IRNodeType _type_info = IRNodeType::Prefetch;
    static constexpr const char* _type_key = "Prefetch";
};

}

// inline functions
inline const Internal::Variable* VarExpr::get() const {
    return static_cast<const Internal::Variable*>(node_.get());
}
inline const Internal::Variable* VarExpr::operator->() const {
    return static_cast<const Internal::Variable*>(node_.get());
}
}
#endif
