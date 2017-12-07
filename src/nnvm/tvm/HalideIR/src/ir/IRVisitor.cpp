#include "IRVisitor.h"

namespace Halide {
namespace Internal {

IRVisitor::~IRVisitor() {
}

void IRVisitor::visit(const IntImm *, const Expr &) {
}

void IRVisitor::visit(const UIntImm *, const Expr &) {
}

void IRVisitor::visit(const FloatImm *, const Expr &) {
}

void IRVisitor::visit(const StringImm *, const Expr &) {
}

void IRVisitor::visit(const Cast *op, const Expr &) {
    op->value.accept(this);
}

void IRVisitor::visit(const Variable *, const Expr &) {
}

void IRVisitor::visit(const Add *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Sub *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Mul *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Div *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Mod *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Min *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Max *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const EQ *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const NE *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const LT *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const LE *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const GT *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const GE *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const And *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Or *op, const Expr &) {
    op->a.accept(this);
    op->b.accept(this);
}

void IRVisitor::visit(const Not *op, const Expr &) {
    op->a.accept(this);
}

void IRVisitor::visit(const Select *op, const Expr &) {
    op->condition.accept(this);
    op->true_value.accept(this);
    op->false_value.accept(this);
}

void IRVisitor::visit(const Load *op, const Expr &) {
    op->index.accept(this);
    op->predicate.accept(this);
}

void IRVisitor::visit(const Ramp *op, const Expr &) {
    op->base.accept(this);
    op->stride.accept(this);
}

void IRVisitor::visit(const Broadcast *op, const Expr &) {
    op->value.accept(this);
}

void IRVisitor::visit(const Call *op, const Expr &) {
    for (size_t i = 0; i < op->args.size(); i++) {
        op->args[i].accept(this);
    }

    // removed: Consider extern call args
}

void IRVisitor::visit(const Let *op, const Expr &) {
    op->value.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const LetStmt *op, const Stmt &) {
    op->value.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const AttrStmt *op, const Stmt &) {
    op->value.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const AssertStmt *op, const Stmt &) {
    op->condition.accept(this);
    op->message.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const ProducerConsumer *op, const Stmt &) {
    op->body.accept(this);
}

void IRVisitor::visit(const For *op, const Stmt &) {
    op->min.accept(this);
    op->extent.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const Store *op, const Stmt &) {
    op->value.accept(this);
    op->index.accept(this);
    op->predicate.accept(this);
}

void IRVisitor::visit(const Provide *op, const Stmt &) {
    op->value.accept(this);
    for (size_t i = 0; i < op->args.size(); i++) {
        op->args[i].accept(this);
    }
}

void IRVisitor::visit(const Allocate *op, const Stmt &) {
    for (size_t i = 0; i < op->extents.size(); i++) {
      op->extents[i].accept(this);
    }
    op->condition.accept(this);
    if (op->new_expr.defined()) {
        op->new_expr.accept(this);
    }
    op->body.accept(this);
}

void IRVisitor::visit(const Free *op, const Stmt &) {
}

void IRVisitor::visit(const Realize *op, const Stmt &) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        op->bounds[i]->min.accept(this);
        op->bounds[i]->extent.accept(this);
    }
    op->condition.accept(this);
    op->body.accept(this);
}

void IRVisitor::visit(const Prefetch *op, const Stmt &) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        op->bounds[i]->min.accept(this);
        op->bounds[i]->extent.accept(this);
    }
}

void IRVisitor::visit(const Block *op, const Stmt &) {
    op->first.accept(this);
    if (op->rest.defined()) {
        op->rest.accept(this);
    }
}

void IRVisitor::visit(const IfThenElse *op, const Stmt &) {
    op->condition.accept(this);
    op->then_case.accept(this);
    if (op->else_case.defined()) {
        op->else_case.accept(this);
    }
}

void IRVisitor::visit(const Evaluate *op, const Stmt &) {
    op->value.accept(this);
}

void IRVisitor::visit(const Shuffle *op, const Expr &) {
    for (Expr i : op->vectors) {
      i.accept(this);
    }
}

void IRGraphVisitor::include(const Expr &e) {
    if (visited.count(e.get())) {
        return;
    } else {
        visited.insert(e.get());
        e.accept(this);
        return;
    }
}

void IRGraphVisitor::include(const Stmt &s) {
    if (visited.count(s.get())) {
        return;
    } else {
        visited.insert(s.get());
        s.accept(this);
        return;
    }
}

void IRGraphVisitor::visit(const IntImm *, const Expr &) {
}

void IRGraphVisitor::visit(const UIntImm *, const Expr &) {
}

void IRGraphVisitor::visit(const FloatImm *, const Expr &) {
}

void IRGraphVisitor::visit(const StringImm *, const Expr &) {
}

void IRGraphVisitor::visit(const Cast *op, const Expr &) {
    include(op->value);
}

void IRGraphVisitor::visit(const Variable *op, const Expr &) {
}

void IRGraphVisitor::visit(const Add *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Sub *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Mul *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Div *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Mod *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Min *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Max *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const EQ *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const NE *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const LT *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const LE *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const GT *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const GE *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const And *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Or *op, const Expr &) {
    include(op->a);
    include(op->b);
}

void IRGraphVisitor::visit(const Not *op, const Expr &) {
    include(op->a);
}

void IRGraphVisitor::visit(const Select *op, const Expr &) {
    include(op->condition);
    include(op->true_value);
    include(op->false_value);
}

void IRGraphVisitor::visit(const Load *op, const Expr &) {
    include(op->index);
    include(op->predicate);
}

void IRGraphVisitor::visit(const Ramp *op, const Expr &) {
    include(op->base);
    include(op->stride);
}

void IRGraphVisitor::visit(const Broadcast *op, const Expr &) {
    include(op->value);
}

void IRGraphVisitor::visit(const Call *op, const Expr &) {
    for (size_t i = 0; i < op->args.size(); i++) {
        include(op->args[i]);
    }
}

void IRGraphVisitor::visit(const Let *op, const Expr &) {
    include(op->value);
    include(op->body);
}

void IRGraphVisitor::visit(const LetStmt *op, const Stmt &) {
    include(op->value);
    include(op->body);
}

void IRGraphVisitor::visit(const AssertStmt *op, const Stmt &) {
    include(op->condition);
    include(op->message);
    include(op->body);
}

void IRGraphVisitor::visit(const ProducerConsumer *op, const Stmt &) {
    include(op->body);
}

void IRGraphVisitor::visit(const For *op, const Stmt &) {
    include(op->min);
    include(op->extent);
    include(op->body);
}

void IRGraphVisitor::visit(const Store *op, const Stmt &) {
    include(op->value);
    include(op->index);
    include(op->predicate);
}

void IRGraphVisitor::visit(const Provide *op, const Stmt &) {
    include(op->value);
    for (size_t i = 0; i < op->args.size(); i++) {
        include(op->args[i]);
    }
}

void IRGraphVisitor::visit(const Allocate *op, const Stmt &) {
    for (size_t i = 0; i < op->extents.size(); i++) {
        include(op->extents[i]);
    }
    include(op->condition);
    if (op->new_expr.defined()) {
        include(op->new_expr);
    }
    include(op->body);
}

void IRGraphVisitor::visit(const Free *op, const Stmt &) {
}

void IRGraphVisitor::visit(const Realize *op, const Stmt &) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        include(op->bounds[i]->min);
        include(op->bounds[i]->extent);
    }
    include(op->condition);
    include(op->body);
}

void IRGraphVisitor::visit(const Prefetch *op, const Stmt &) {
    for (size_t i = 0; i < op->bounds.size(); i++) {
        include(op->bounds[i]->min);
        include(op->bounds[i]->extent);
    }
}

void IRGraphVisitor::visit(const Block *op, const Stmt &) {
    include(op->first);
    if (op->rest.defined()) include(op->rest);
}

void IRGraphVisitor::visit(const IfThenElse *op, const Stmt &) {
    include(op->condition);
    include(op->then_case);
    if (op->else_case.defined()) {
        include(op->else_case);
    }
}

void IRGraphVisitor::visit(const Evaluate *op, const Stmt &) {
    include(op->value);
}

void IRGraphVisitor::visit(const Shuffle *op, const Expr &) {
    for (Expr i : op->vectors) {
      include(i);
    }
}

}
}
