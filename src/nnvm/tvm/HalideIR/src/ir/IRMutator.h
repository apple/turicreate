#ifndef HALIDE_IR_MUTATOR_H
#define HALIDE_IR_MUTATOR_H

/** \file
 * Defines a base class for passes over the IR that modify it
 */

#include "IRVisitor.h"
#include <unordered_map>

namespace Halide {
namespace Internal {

/** A base class for passes over the IR which modify it
 * (e.g. replacing a variable with a value (Substitute.h), or
 * constant-folding).
 *
 * Your mutate should override the visit methods you care about. Return
 * the new expression by assigning to expr or stmt. The default ones
 * recursively mutate their children. To mutate sub-expressions and
 * sub-statements you should the mutate method, which will dispatch to
 * the appropriate visit method and then return the value of expr or
 * stmt after the call to visit.
 */
class IRMutator : public IRVisitor {
public:

    /** This is the main interface for using a mutator. Also call
     * these in your subclass to mutate sub-expressions and
     * sub-statements.
     */
    virtual EXPORT Expr mutate(Expr expr);
    virtual EXPORT Stmt mutate(Stmt stmt);

protected:
    /** visit methods that take Exprs assign to this to return their
     * new value */
    Expr expr;

    /** visit methods that take Stmts assign to this to return their
     * new value */
    Stmt stmt;

protected:
    EXPORT virtual void visit(const IntImm *, const Expr &);
    EXPORT virtual void visit(const UIntImm *, const Expr &);
    EXPORT virtual void visit(const FloatImm *, const Expr &);
    EXPORT virtual void visit(const StringImm *, const Expr &);
    EXPORT virtual void visit(const Cast *, const Expr &);
    EXPORT virtual void visit(const Variable *, const Expr &);
    EXPORT virtual void visit(const Add *, const Expr &);
    EXPORT virtual void visit(const Sub *, const Expr &);
    EXPORT virtual void visit(const Mul *, const Expr &);
    EXPORT virtual void visit(const Div *, const Expr &);
    EXPORT virtual void visit(const Mod *, const Expr &);
    EXPORT virtual void visit(const Min *, const Expr &);
    EXPORT virtual void visit(const Max *, const Expr &);
    EXPORT virtual void visit(const EQ *, const Expr &);
    EXPORT virtual void visit(const NE *, const Expr &);
    EXPORT virtual void visit(const LT *, const Expr &);
    EXPORT virtual void visit(const LE *, const Expr &);
    EXPORT virtual void visit(const GT *, const Expr &);
    EXPORT virtual void visit(const GE *, const Expr &);
    EXPORT virtual void visit(const And *, const Expr &);
    EXPORT virtual void visit(const Or *, const Expr &);
    EXPORT virtual void visit(const Not *, const Expr &);
    EXPORT virtual void visit(const Select *, const Expr &);
    EXPORT virtual void visit(const Load *, const Expr &);
    EXPORT virtual void visit(const Ramp *, const Expr &);
    EXPORT virtual void visit(const Broadcast *, const Expr &);
    EXPORT virtual void visit(const Call *, const Expr &);
    EXPORT virtual void visit(const Let *, const Expr &);
    EXPORT virtual void visit(const LetStmt *, const Stmt &);
    EXPORT virtual void visit(const AttrStmt *, const Stmt &);
    EXPORT virtual void visit(const AssertStmt *, const Stmt &);
    EXPORT virtual void visit(const ProducerConsumer *, const Stmt &);
    EXPORT virtual void visit(const For *, const Stmt &);
    EXPORT virtual void visit(const Store *, const Stmt &);
    EXPORT virtual void visit(const Provide *, const Stmt &);
    EXPORT virtual void visit(const Allocate *, const Stmt &);
    EXPORT virtual void visit(const Free *, const Stmt &);
    EXPORT virtual void visit(const Realize *, const Stmt &);
    EXPORT virtual void visit(const Prefetch *, const Stmt &);
    EXPORT virtual void visit(const Block *, const Stmt &);
    EXPORT virtual void visit(const IfThenElse *, const Stmt &);
    EXPORT virtual void visit(const Evaluate *, const Stmt &);
    EXPORT virtual void visit(const Shuffle *, const Expr &);
};


/** A mutator that caches and reapplies previously-done mutations, so
 * that it can handle graphs of IR that have not had CSE done to
 * them. */
class IRGraphMutator : public IRMutator {
protected:
    std::unordered_map<Expr, Expr, ExprHash, ExprEqual> expr_replacements;
    std::unordered_map<Stmt, Stmt> stmt_replacements;

public:
    EXPORT Stmt mutate(Stmt s);
    EXPORT Expr mutate(Expr e);
};


}
}

#endif
