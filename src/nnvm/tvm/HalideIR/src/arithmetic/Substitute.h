#ifndef HALIDE_SUBSTITUTE_H
#define HALIDE_SUBSTITUTE_H

/** \file
 *
 * Defines methods for substituting out variables in expressions and
 * statements. */

#include <map>

#include "ir/IR.h"

namespace Halide {
namespace Internal {

/** Substitute variables with the given pointer with the replacement
 * expression within expr. */
EXPORT Expr substitute(const Variable* var, Expr replacement, Expr expr);

/** Substitute variables with the given pointer with the replacement
 * expression within stmt. */
EXPORT Stmt substitute(const Variable* var, Expr replacement, Stmt stmt);

EXPORT inline Expr substitute(const VarExpr& var, Expr replacement, Expr expr) {
  return substitute(var.get(), replacement, expr);
}

EXPORT inline Stmt substitute(const VarExpr& var, Expr replacement, Stmt stmt) {
  return substitute(var.get(), replacement, stmt);
}

/** Substitute variables with pointers in the map. */
// @{
EXPORT Expr substitute(const std::map<const Variable*, Expr> &replacements, Expr expr);
EXPORT Stmt substitute(const std::map<const Variable*, Expr> &replacements, Stmt stmt);
// @}

/** Substitute expressions for other expressions. */
// @{
EXPORT Expr substitute(Expr find, Expr replacement, Expr expr);
EXPORT Stmt substitute(Expr find, Expr replacement, Stmt stmt);
// @}

/** Substitutions where the IR may be a general graph (and not just a
 * DAG). */
// @{
Expr graph_substitute(const Variable* var, Expr replacement, Expr expr);
Stmt graph_substitute(const Variable* var, Expr replacement, Stmt stmt);
Expr graph_substitute(Expr find, Expr replacement, Expr expr);
Stmt graph_substitute(Expr find, Expr replacement, Stmt stmt);
// @}

/** Substitute in all let Exprs in a piece of IR. Doesn't substitute
 * in let stmts, as this may change the meaning of the IR (e.g. by
 * moving a load after a store). Produces graphs of IR, so don't use
 * non-graph-aware visitors or mutators on it until you've CSE'd the
 * result. */
// @{
Expr substitute_in_all_lets(Expr expr);
Stmt substitute_in_all_lets(Stmt stmt);
// @}

}
}

#endif
