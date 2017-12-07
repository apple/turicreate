#include "IRMutator.h"

namespace Halide {
namespace Internal {

using std::vector;

Expr IRMutator::mutate(Expr e) {
    if (e.defined()) {
        e.accept(this);
    } else {
        expr = Expr();
    }
    stmt = Stmt();
    return expr;
}

Stmt IRMutator::mutate(Stmt s) {
    if (s.defined()) {
        s.accept(this);
    } else {
        stmt = Stmt();
    }
    expr = Expr();
    return stmt;
}

void IRMutator::visit(const IntImm *op, const Expr &e) { expr = e; }
void IRMutator::visit(const UIntImm *op, const Expr &e) { expr = e; }
void IRMutator::visit(const FloatImm *op, const Expr &e) { expr = e; }
void IRMutator::visit(const StringImm *op, const Expr &e) { expr = e; }
void IRMutator::visit(const Variable *op, const Expr &e) { expr = e; }

void IRMutator::visit(const Cast *op, const Expr &e) {
    Expr value = mutate(op->value);
    if (value.same_as(op->value)) {
        expr = e;
    } else {
        expr = Cast::make(op->type, value);
    }
}

// use macro to access private function.
#define MUTATE_BINARY_OP(op, e, T)                \
  Expr a = mutate(op->a);                         \
  Expr b = mutate(op->b);                         \
  if (a.same_as(op->a) &&                         \
      b.same_as(op->b)) {                         \
    expr = e;                                     \
  } else {                                        \
    expr = T::make(a, b);                         \
  }                                               \

void IRMutator::visit(const Add *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Add);
}
void IRMutator::visit(const Sub *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Sub);
}
void IRMutator::visit(const Mul *op, const Expr &e)  {
  MUTATE_BINARY_OP(op, e, Mul);
}
void IRMutator::visit(const Div *op, const Expr &e)  {
  MUTATE_BINARY_OP(op, e, Div);
}
void IRMutator::visit(const Mod *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Mod);
}
void IRMutator::visit(const Min *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Min);
}
void IRMutator::visit(const Max *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Max);
}
void IRMutator::visit(const EQ *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, EQ);
}
void IRMutator::visit(const NE *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, NE);
}
void IRMutator::visit(const LT *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, LT);
}
void IRMutator::visit(const LE *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, LE);
}
void IRMutator::visit(const GT *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, GT);
}
void IRMutator::visit(const GE *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, GE);
}
void IRMutator::visit(const And *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, And);
}
void IRMutator::visit(const Or *op, const Expr &e) {
  MUTATE_BINARY_OP(op, e, Or);
}

void IRMutator::visit(const Not *op, const Expr &e) {
    Expr a = mutate(op->a);
    if (a.same_as(op->a)) {
      expr = e;
    } else {
      expr = Not::make(a);
    }
}

void IRMutator::visit(const Select *op, const Expr &e)  {
    Expr cond = mutate(op->condition);
    Expr t = mutate(op->true_value);
    Expr f = mutate(op->false_value);
    if (cond.same_as(op->condition) &&
        t.same_as(op->true_value) &&
        f.same_as(op->false_value)) {
      expr = e;
    } else {
      expr = Select::make(cond, t, f);
    }
}

void IRMutator::visit(const Load *op, const Expr &e) {
    Expr index = mutate(op->index);
    Expr predicate = mutate(op->predicate);
    if (predicate.same_as(op->predicate) && index.same_as(op->index)) {
      expr = e;
    } else {
      expr = Load::make(op->type, op->buffer_var, index, predicate);
    }
}

void IRMutator::visit(const Ramp *op, const Expr &e) {
    Expr base = mutate(op->base);
    Expr stride = mutate(op->stride);
    if (base.same_as(op->base) &&
        stride.same_as(op->stride)) {
      expr = e;
    } else {
      expr = Ramp::make(base, stride, op->lanes);
    }
}

void IRMutator::visit(const Broadcast *op, const Expr &e) {
    Expr value = mutate(op->value);
    if (value.same_as(op->value)) {
      expr = e;
    } else {
      expr = Broadcast::make(value, op->lanes);
    }
}

void IRMutator::visit(const Call *op, const Expr &e) {
    vector<Expr > new_args(op->args.size());
    bool changed = false;

    // Mutate the args
    for (size_t i = 0; i < op->args.size(); i++) {
        Expr old_arg = op->args[i];
        Expr new_arg = mutate(old_arg);
        if (!new_arg.same_as(old_arg)) changed = true;
        new_args[i] = new_arg;
    }

    if (!changed) {
      expr = e;
    } else {
      expr = Call::make(op->type, op->name, new_args, op->call_type,
                        op->func, op->value_index);
    }
}

void IRMutator::visit(const Let *op, const Expr &e) {
    Expr value = mutate(op->value);
    Expr body = mutate(op->body);
    if (value.same_as(op->value) &&
        body.same_as(op->body)) {
      expr = e;
    } else {
      expr = Let::make(op->var, value, body);
    }
}

void IRMutator::visit(const LetStmt *op, const Stmt &s) {
    Expr value = mutate(op->value);
    Stmt body = mutate(op->body);
    if (value.same_as(op->value) &&
        body.same_as(op->body)) {
      stmt = s;
    } else {
      stmt = LetStmt::make(op->var, value, body);
    }
}

void IRMutator::visit(const AttrStmt *op, const Stmt &s) {
    Expr value = mutate(op->value);
    Stmt body = mutate(op->body);
    if (value.same_as(op->value) &&
        body.same_as(op->body)) {
      stmt = s;
    } else {
      stmt = AttrStmt::make(op->node, op->attr_key, value, body);
    }
}

void IRMutator::visit(const AssertStmt *op, const Stmt &s) {
    Expr condition = mutate(op->condition);
    Expr message = mutate(op->message);
    Stmt body = mutate(op->body);

    if (condition.same_as(op->condition) &&
        message.same_as(op->message) &&
        body.same_as(op->body)) {
      stmt = s;
    } else {
      stmt = AssertStmt::make(condition, message, body);
    }
}

void IRMutator::visit(const ProducerConsumer *op, const Stmt &s) {
    Stmt body = mutate(op->body);
    if (body.same_as(op->body)) {
      stmt = s;
    } else {
      stmt = ProducerConsumer::make(op->func, op->is_producer, body);
    }
}

void IRMutator::visit(const For *op, const Stmt &s) {
    Expr min = mutate(op->min);
    Expr extent = mutate(op->extent);
    Stmt body = mutate(op->body);
    if (min.same_as(op->min) &&
        extent.same_as(op->extent) &&
        body.same_as(op->body)) {
      stmt = s;
    } else {
      stmt = For::make(
          op->loop_var, min, extent, op->for_type, op->device_api, body);
    }
}

void IRMutator::visit(const Store *op, const Stmt &s) {
    Expr value = mutate(op->value);
    Expr index = mutate(op->index);
    Expr predicate = mutate(op->predicate);
    if (predicate.same_as(op->predicate) && value.same_as(op->value) && index.same_as(op->index)) {
      stmt = s;
    } else {
      stmt = Store::make(op->buffer_var, value, index, predicate);
    }
}

void IRMutator::visit(const Provide *op, const Stmt &s) {
    vector<Expr> new_args(op->args.size());

    bool changed = false;

    // Mutate the args
    for (size_t i = 0; i < op->args.size(); i++) {
        Expr old_arg = op->args[i];
        Expr new_arg = mutate(old_arg);
        if (!new_arg.same_as(old_arg)) changed = true;
        new_args[i] = new_arg;
    }
    Expr old_value = op->value;
    Expr new_value = mutate(old_value);
    if (!new_value.same_as(old_value)) changed = true;

    if (!changed) {
      stmt = s;
    } else {
      stmt = Provide::make(op->func, op->value_index, new_value, new_args);
    }
}

void IRMutator::visit(const Allocate *op, const Stmt &s) {
    std::vector<Expr> new_extents;
    bool all_extents_unmodified = true;
    for (size_t i = 0; i < op->extents.size(); i++) {
        new_extents.push_back(mutate(op->extents[i]));
        all_extents_unmodified &= new_extents[i].same_as(op->extents[i]);
    }
    Stmt body = mutate(op->body);
    Expr condition = mutate(op->condition);
    Expr new_expr;
    if (op->new_expr.defined()) {
        new_expr = mutate(op->new_expr);
    }
    if (all_extents_unmodified &&
        body.same_as(op->body) &&
        condition.same_as(op->condition) &&
        new_expr.same_as(op->new_expr)) {
      stmt = s;
    } else {
      stmt = Allocate::make(op->buffer_var, op->type, new_extents, condition, body, new_expr, op->free_function);
    }
}

void IRMutator::visit(const Free *op, const Stmt &s) {
  stmt = s;
}

void IRMutator::visit(const Realize *op, const Stmt &s) {
    Region new_bounds;
    bool bounds_changed = false;

    // Mutate the bounds
    for (size_t i = 0; i < op->bounds.size(); i++) {
        Expr old_min    = op->bounds[i]->min;
        Expr old_extent = op->bounds[i]->extent;
        Expr new_min    = mutate(old_min);
        Expr new_extent = mutate(old_extent);
        if (!new_min.same_as(old_min))       bounds_changed = true;
        if (!new_extent.same_as(old_extent)) bounds_changed = true;
        new_bounds.push_back(
            Range::make_by_min_extent(new_min, new_extent));
    }

    Stmt body = mutate(op->body);
    Expr condition = mutate(op->condition);
    if (!bounds_changed &&
        body.same_as(op->body) &&
        condition.same_as(op->condition)) {
      stmt = s;
    } else {
      stmt = Realize::make(op->func, op->value_index,
                           op->type, new_bounds,
                           condition, body);
    }
}

void IRMutator::visit(const Prefetch *op, const Stmt &s) {
    Region new_bounds;
    bool bounds_changed = false;

    // Mutate the bounds
    for (size_t i = 0; i < op->bounds.size(); i++) {
        Expr old_min    = op->bounds[i]->min;
        Expr old_extent = op->bounds[i]->extent;
        Expr new_min    = mutate(old_min);
        Expr new_extent = mutate(old_extent);
        if (!new_min.same_as(old_min))       bounds_changed = true;
        if (!new_extent.same_as(old_extent)) bounds_changed = true;
        new_bounds.push_back(
            Range::make_by_min_extent(new_min, new_extent));
    }

    if (!bounds_changed) {
      stmt = s;
    } else {
      stmt = Prefetch::make(op->func, op->value_index,
                           op->type, new_bounds);
    }
}

void IRMutator::visit(const Block *op, const Stmt &s) {
    Stmt first = mutate(op->first);
    Stmt rest = mutate(op->rest);
    if (first.same_as(op->first) &&
        rest.same_as(op->rest)) {
      stmt = s;
    } else {
      stmt = Block::make(first, rest);
    }
}

void IRMutator::visit(const IfThenElse *op, const Stmt &s) {
    Expr condition = mutate(op->condition);
    Stmt then_case = mutate(op->then_case);
    Stmt else_case = mutate(op->else_case);
    if (condition.same_as(op->condition) &&
        then_case.same_as(op->then_case) &&
        else_case.same_as(op->else_case)) {
      stmt = s;
    } else {
      stmt = IfThenElse::make(condition, then_case, else_case);
    }
}

void IRMutator::visit(const Evaluate *op, const Stmt &s) {
    Expr v = mutate(op->value);
    if (v.same_as(op->value)) {
      stmt = s;
    } else {
      stmt = Evaluate::make(v);
    }
}

void IRMutator::visit(const Shuffle *op, const Expr& e) {
    Array<Expr> new_vectors;
    bool changed = false;

    for (size_t i = 0; i < op->vectors.size(); i++) {
        Expr old_vector = op->vectors[i];
        Expr new_vector = mutate(old_vector);
        if (!new_vector.same_as(old_vector)) changed = true;
        new_vectors.push_back(new_vector);
    }

    if (!changed) {
        expr = e;
    } else {
        expr = Shuffle::make(new_vectors, op->indices);
    }
}

Stmt IRGraphMutator::mutate(Stmt s) {
    auto iter = stmt_replacements.find(s);
    if (iter != stmt_replacements.end()) {
        return iter->second;
    }
    Stmt new_s = IRMutator::mutate(s);
    stmt_replacements[s] = new_s;
    return new_s;
}

Expr IRGraphMutator::mutate(Expr e) {
    auto iter = expr_replacements.find(e);
    if (iter != expr_replacements.end()) {
        return iter->second;
    }
    Expr new_e = IRMutator::mutate(e);
    expr_replacements[e] = new_e;
    return new_e;
}

}
}
