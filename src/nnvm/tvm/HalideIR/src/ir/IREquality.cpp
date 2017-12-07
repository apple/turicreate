#include "IREquality.h"
#include "IRVisitor.h"
#include "IROperator.h"

namespace Halide {
namespace Internal {

using std::string;
using std::vector;

namespace {

/** The class that does the work of comparing two IR nodes. */
class IRComparer : public IRVisitor {
public:

    /** Different possible results of a comparison. Unknown should
     * only occur internally due to a cache miss. */
    enum CmpResult {Unknown, Equal, LessThan, GreaterThan};

    /** The result of the comparison. Should be Equal, LessThan, or GreaterThan. */
    CmpResult result;

    /** Compare two expressions or statements and return the
     * result. Returns the result immediately if it is already
     * non-zero. */
    // @{
    CmpResult compare_expr(const Expr &a, const Expr &b);
    CmpResult compare_stmt(const Stmt &a, const Stmt &b);
    // @}

    /** If the expressions you're comparing may contain many repeated
     * subexpressions, it's worth passing in a cache to use.
     * Currently this is only done in common-subexpression
     * elimination. */
    IRComparer(IRCompareCache *c = nullptr) : result(Equal), cache(c) {}

private:
    Expr expr_;
    Stmt stmt_;
    IRCompareCache *cache;

    CmpResult compare_names(const std::string &a, const std::string &b);
    CmpResult compare_ptrs(const Node* a, const Node* b);
    CmpResult compare_node_refs(const NodeRef& a, const NodeRef& b);
    CmpResult compare_types(Type a, Type b);
    CmpResult compare_expr_vector(const Array<Expr> &a, const Array<Expr> &b);

    // Compare two things that already have a well-defined operator<
    template<typename T>
    CmpResult compare_scalar(T a, T b);


    void visit(const IntImm *, const Expr &);
    void visit(const UIntImm *, const Expr &);
    void visit(const FloatImm *, const Expr &);
    void visit(const StringImm *, const Expr &);
    void visit(const Cast *, const Expr &);
    void visit(const Variable *, const Expr &);
    void visit(const Add *, const Expr &);
    void visit(const Sub *, const Expr &);
    void visit(const Mul *, const Expr &);
    void visit(const Div *, const Expr &);
    void visit(const Mod *, const Expr &);
    void visit(const Min *, const Expr &);
    void visit(const Max *, const Expr &);
    void visit(const EQ *, const Expr &);
    void visit(const NE *, const Expr &);
    void visit(const LT *, const Expr &);
    void visit(const LE *, const Expr &);
    void visit(const GT *, const Expr &);
    void visit(const GE *, const Expr &);
    void visit(const And *, const Expr &);
    void visit(const Or *, const Expr &);
    void visit(const Not *, const Expr &);
    void visit(const Select *, const Expr &);
    void visit(const Load *, const Expr &);
    void visit(const Ramp *, const Expr &);
    void visit(const Broadcast *, const Expr &);
    void visit(const Call *, const Expr &);
    void visit(const Let *, const Expr &);
    void visit(const Shuffle *, const Expr &);
    void visit(const LetStmt *, const Stmt &);
    void visit(const AttrStmt *, const Stmt &);
    void visit(const AssertStmt *, const Stmt &);
    void visit(const ProducerConsumer *, const Stmt &);
    void visit(const For *, const Stmt &);
    void visit(const Store *, const Stmt &);
    void visit(const Provide *, const Stmt &);
    void visit(const Allocate *, const Stmt &);
    void visit(const Free *, const Stmt &);
    void visit(const Realize *, const Stmt &);
    void visit(const Prefetch *, const Stmt &);
    void visit(const Block *, const Stmt &);
    void visit(const IfThenElse *, const Stmt &);
    void visit(const Evaluate *, const Stmt &);
};

template<typename T>
IRComparer::CmpResult IRComparer::compare_scalar(T a, T b) {
    if (result != Equal) return result;

    if (a < b) {
        result = LessThan;
    } else if (a > b) {
        result = GreaterThan;
    }

    return result;
}

IRComparer::CmpResult IRComparer::compare_expr(const Expr &a, const Expr &b) {
    if (result != Equal) {
        return result;
    }

    if (a.same_as(b)) {
        result = Equal;
        return result;
    }

    if (!a.defined() && !b.defined()) {
        result = Equal;
        return result;
    }

    if (!a.defined()) {
        result = LessThan;
        return result;
    }

    if (!b.defined()) {
        result = GreaterThan;
        return result;
    }

    // If in the future we have hashes for Exprs, this is a good place
    // to compare the hashes:
    // if (compare_scalar(a.hash(), b.hash()) != Equal) {
    //   return result;
    // }

    if (compare_scalar(a->type_info(), b->type_info()) != Equal) {
        return result;
    }

    if (compare_types(a.type(), b.type()) != Equal) {
        return result;
    }

    // Check the cache - perhaps these exprs have already been compared and found equal.
    if (cache && cache->contains(a, b)) {
        result = Equal;
        return result;
    }

    expr_ = a;
    b.accept(this);

    if (cache && result == Equal) {
        cache->insert(a, b);
    }

    return result;
}

IRComparer::CmpResult IRComparer::compare_stmt(const Stmt &a, const Stmt &b) {
    if (result != Equal) {
        return result;
    }

    if (a.same_as(b)) {
        result = Equal;
        return result;
    }

    if (!a.defined() && !b.defined()) {
        result = Equal;
        return result;
    }

    if (!a.defined()) {
        result = LessThan;
        return result;
    }

    if (!b.defined()) {
        result = GreaterThan;
        return result;
    }

    if (compare_scalar(a->type_info(), b->type_info()) != Equal) {
        return result;
    }

    stmt_ = a;
    b.accept(this);

    return result;
}

IRComparer::CmpResult IRComparer::compare_types(Type a, Type b) {
    if (result != Equal) return result;

    compare_scalar(a.code(), b.code());
    compare_scalar(a.bits(), b.bits());
    compare_scalar(a.lanes(), b.lanes());
    compare_scalar((uintptr_t)a.handle_type, (uintptr_t)b.handle_type);

    return result;
}

IRComparer::CmpResult IRComparer::compare_names(const string &a, const string &b) {
    if (result != Equal) return result;

    int string_cmp = a.compare(b);
    if (string_cmp < 0) {
        result = LessThan;
    } else if (string_cmp > 0) {
        result = GreaterThan;
    }

    return result;
}

IRComparer::CmpResult IRComparer::compare_ptrs(const Node* a, const Node* b) {
    if (result != Equal) return result;
    if (a < b) {
      result = LessThan;
    } else if (a > b) {
      result = GreaterThan;
    }
    return result;
}

IRComparer::CmpResult IRComparer::compare_node_refs(const NodeRef& a, const NodeRef& b) {
  return compare_ptrs(a.get(), b.get());
}

IRComparer::CmpResult IRComparer::compare_expr_vector(const Array<Expr> &a, const Array<Expr> &b) {
    if (result != Equal) return result;

    compare_scalar(a.size(), b.size());
    for (size_t i = 0; (i < a.size()) && result == Equal; i++) {
        compare_expr(a[i], b[i]);
    }

    return result;
}

void IRComparer::visit(const IntImm *op, const Expr &e) {
    const IntImm *node = expr_.as<IntImm>();
    compare_scalar(node->value, op->value);
}

void IRComparer::visit(const UIntImm *op, const Expr &e) {
    const UIntImm *node = expr_.as<UIntImm>();
    compare_scalar(node->value, op->value);
}

void IRComparer::visit(const FloatImm *op, const Expr &e) {
    const FloatImm *node = expr_.as<FloatImm>();
    compare_scalar(node->value, op->value);
}

void IRComparer::visit(const StringImm *op, const Expr &e) {
    const StringImm *node = expr_.as<StringImm>();
    compare_names(node->value, op->value);
}

void IRComparer::visit(const Cast *op, const Expr &e) {
    compare_expr(expr_.as<Cast>()->value, op->value);
}

void IRComparer::visit(const Variable *op, const Expr &e) {
    const Variable *node = expr_.as<Variable>();
    compare_ptrs(node, op);
}

namespace {
template<typename T>
void visit_binary_operator(IRComparer *cmp, const T *op, Expr expr) {
    const T *node = expr.as<T>();
    cmp->compare_expr(node->a, op->a);
    cmp->compare_expr(node->b, op->b);
}
}

void IRComparer::visit(const Add *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Sub *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Mul *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Div *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Mod *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Min *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Max *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const EQ *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const NE *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const LT *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const LE *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const GT *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const GE *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const And *op, const Expr &e) {visit_binary_operator(this, op, expr_);}
void IRComparer::visit(const Or *op, const Expr &e) {visit_binary_operator(this, op, expr_);}

void IRComparer::visit(const Not *op, const Expr &e) {
    const Not *node = expr_.as<Not>();
    compare_expr(node->a, op->a);
}

void IRComparer::visit(const Select *op, const Expr &e) {
    const Select *node = expr_.as<Select>();
    compare_expr(node->condition, op->condition);
    compare_expr(node->true_value, op->true_value);
    compare_expr(node->false_value, op->false_value);

}

void IRComparer::visit(const Load *op, const Expr &e) {
    const Load *node = expr_.as<Load>();
    compare_node_refs(op->buffer_var, node->buffer_var);
    compare_expr(node->index, op->index);
    compare_expr(node->predicate, op->predicate);
}

void IRComparer::visit(const Ramp *op, const Expr &e) {
    const Ramp *node = expr_.as<Ramp>();
    // No need to compare width because we already compared types
    compare_expr(node->base, op->base);
    compare_expr(node->stride, op->stride);
}

void IRComparer::visit(const Broadcast *op, const Expr &e) {
    const Broadcast *node = expr_.as<Broadcast>();
    compare_expr(node->value, op->value);
}

void IRComparer::visit(const Call *op, const Expr &e) {
    const Call *node = expr_.as<Call>();

    compare_names(node->name, op->name);
    compare_scalar(node->call_type, op->call_type);
    compare_scalar(node->value_index, op->value_index);
    compare_expr_vector(node->args, op->args);
    compare_node_refs(node->func, op->func);
}

void IRComparer::visit(const Let *op, const Expr &e) {
    const Let *node = expr_.as<Let>();

    compare_node_refs(node->var, op->var);
    compare_expr(node->value, op->value);
    compare_expr(node->body, op->body);
}

void IRComparer::visit(const LetStmt *op, const Stmt &s) {
    const LetStmt *node = stmt_.as<LetStmt>();

    compare_node_refs(node->var, op->var);
    compare_expr(node->value, op->value);
    compare_stmt(node->body, op->body);
}


void IRComparer::visit(const AttrStmt *op, const Stmt &s) {
    const AttrStmt *node = stmt_.as<AttrStmt>();

    compare_node_refs(node->node, op->node);
    compare_names(node->attr_key, op->attr_key);
    compare_expr(node->value, op->value);
    compare_stmt(node->body, op->body);
}

void IRComparer::visit(const AssertStmt *op, const Stmt &s) {
    const AssertStmt *node = stmt_.as<AssertStmt>();

    compare_expr(node->condition, op->condition);
    compare_expr(node->message, op->message);
    compare_stmt(node->body, op->body);
}

void IRComparer::visit(const ProducerConsumer *op, const Stmt &s) {
    const ProducerConsumer *node = stmt_.as<ProducerConsumer>();

    compare_node_refs(node->func, op->func);
    compare_scalar(node->is_producer, op->is_producer);
    compare_stmt(node->body, op->body);
}

void IRComparer::visit(const For *op, const Stmt &s) {
    const For *node = stmt_.as<For>();

    compare_node_refs(node->loop_var, op->loop_var);
    compare_scalar(node->for_type, op->for_type);
    compare_expr(node->min, op->min);
    compare_expr(node->extent, op->extent);
    compare_stmt(node->body, op->body);
}

void IRComparer::visit(const Store *op, const Stmt &s) {
    const Store *node = stmt_.as<Store>();

    compare_node_refs(node->buffer_var, op->buffer_var);

    compare_expr(node->value, op->value);
    compare_expr(node->index, op->index);
    compare_expr(node->predicate, op->predicate);
}

void IRComparer::visit(const Provide *op, const Stmt &s) {
    const Provide *node = stmt_.as<Provide>();

    compare_node_refs(node->func, op->func);
    compare_scalar(node->value_index, op->value_index);
    compare_expr_vector(node->args, op->args);
    compare_expr(node->value, op->value);
}

void IRComparer::visit(const Allocate *op, const Stmt &s) {
    const Allocate *node = stmt_.as<Allocate>();

    compare_node_refs(node->buffer_var, op->buffer_var);
    compare_expr_vector(node->extents, op->extents);
    compare_stmt(node->body, op->body);
    compare_expr(node->condition, op->condition);
    compare_expr(node->new_expr, op->new_expr);
    compare_names(node->free_function, op->free_function);
}

void IRComparer::visit(const Realize *op, const Stmt &s) {
    const Realize *node = stmt_.as<Realize>();

    compare_node_refs(node->func, op->func);
    compare_scalar(node->value_index, op->value_index);
    compare_types(node->type, op->type);
    compare_scalar(node->bounds.size(), op->bounds.size());

    for (size_t i = 0; (result == Equal) && (i < node->bounds.size()); i++) {
        compare_expr(node->bounds[i]->min, op->bounds[i]->min);
        compare_expr(node->bounds[i]->extent, op->bounds[i]->extent);
    }
    compare_stmt(node->body, op->body);
    compare_expr(node->condition, op->condition);
}

void IRComparer::visit(const Prefetch *op, const Stmt& stmt) {
    const Prefetch *node = stmt_.as<Prefetch>();
    compare_node_refs(node->func, op->func);
    compare_scalar(node->value_index, op->value_index);
    compare_types(node->type, op->type);
    compare_scalar(node->bounds.size(), op->bounds.size());
    for (size_t i = 0; (result == Equal) && (i < node->bounds.size()); i++) {
        compare_expr(node->bounds[i]->min, op->bounds[i]->min);
        compare_expr(node->bounds[i]->extent, op->bounds[i]->extent);
    }
}

void IRComparer::visit(const Block *op, const Stmt &s) {
    const Block *node = stmt_.as<Block>();

    compare_stmt(node->first, op->first);
    compare_stmt(node->rest, op->rest);
}

void IRComparer::visit(const Free *op, const Stmt &s) {
    const Free *node = stmt_.as<Free>();

    compare_node_refs(node->buffer_var, op->buffer_var);
}

void IRComparer::visit(const IfThenElse *op, const Stmt &s) {
    const IfThenElse *node = stmt_.as<IfThenElse>();

    compare_expr(node->condition, op->condition);
    compare_stmt(node->then_case, op->then_case);
    compare_stmt(node->else_case, op->else_case);
}

void IRComparer::visit(const Evaluate *op, const Stmt &s) {
    const Evaluate *node = stmt_.as<Evaluate>();

    compare_expr(node->value, op->value);
}

void IRComparer::visit(const Shuffle *op, const Expr &expr) {
    const Shuffle *e = expr.as<Shuffle>();

    compare_expr_vector(e->vectors, op->vectors);
    compare_expr_vector(e->indices, op->indices);
}

} // namespace


// Now the methods exposed in the header.
bool equal(const Expr &a, const Expr &b) {
    return IRComparer().compare_expr(a, b) == IRComparer::Equal;
}

bool graph_equal(const Expr &a, const Expr &b) {
    IRCompareCache cache(8);
    return IRComparer(&cache).compare_expr(a, b) == IRComparer::Equal;
}

bool equal(const Stmt &a, const Stmt &b) {
    return IRComparer().compare_stmt(a, b) == IRComparer::Equal;
}

bool graph_equal(const Stmt &a, const Stmt &b) {
    IRCompareCache cache(8);
    return IRComparer(&cache).compare_stmt(a, b) == IRComparer::Equal;
}

bool IRDeepCompare::operator()(const Expr &a, const Expr &b) const {
    IRComparer cmp;
    cmp.compare_expr(a, b);
    return cmp.result == IRComparer::LessThan;
}

bool IRDeepCompare::operator()(const Stmt &a, const Stmt &b) const {
    IRComparer cmp;
    cmp.compare_stmt(a, b);
    return cmp.result == IRComparer::LessThan;
}

bool ExprWithCompareCache::operator<(const ExprWithCompareCache &other) const {
    IRComparer cmp(cache);
    cmp.compare_expr(expr, other.expr);
    return cmp.result == IRComparer::LessThan;
}

// Testing code
namespace {

IRComparer::CmpResult flip_result(IRComparer::CmpResult r) {
    switch(r) {
    case IRComparer::LessThan: return IRComparer::GreaterThan;
    case IRComparer::Equal: return IRComparer::Equal;
    case IRComparer::GreaterThan: return IRComparer::LessThan;
    case IRComparer::Unknown: return IRComparer::Unknown;
    }
    return IRComparer::Unknown;
}

void check_equal(Expr a, Expr b) {
    IRCompareCache cache(5);
    IRComparer::CmpResult r = IRComparer(&cache).compare_expr(a, b);
    internal_assert(r == IRComparer::Equal)
        << "Error in ir_equality_test: " << r
        << " instead of " << IRComparer::Equal
        << " when comparing:\n" << a
        << "\nand\n" << b << "\n";
}

void check_not_equal(Expr a, Expr b) {
    IRCompareCache cache(5);
    IRComparer::CmpResult r1 = IRComparer(&cache).compare_expr(a, b);
    IRComparer::CmpResult r2 = IRComparer(&cache).compare_expr(b, a);
    internal_assert(r1 != IRComparer::Equal &&
                    r1 != IRComparer::Unknown &&
                    flip_result(r1) == r2)
        << "Error in ir_equality_test: " << r1
        << " is not the opposite of " << r2
        << " when comparing:\n" << a
        << "\nand\n" << b << "\n";
}

} // namespace

void ir_equality_test() {
    Expr x = Variable::make(Int(32), "x");
    check_equal(Ramp::make(x, 4, 3), Ramp::make(x, 4, 3));
    check_not_equal(Ramp::make(x, 2, 3), Ramp::make(x, 4, 3));

    check_equal(x, Variable::make(Int(32), "x"));
    check_not_equal(x, Variable::make(Int(32), "y"));

    // Something that will hang if IREquality has poor computational
    // complexity.
    Expr e1 = x, e2 = x;
    for (int i = 0; i < 100; i++) {
        e1 = e1*e1 + e1;
        e2 = e2*e2 + e2;
    }
    check_equal(e1, e2);
    // These are only discovered to be not equal way down the tree:
    e2 = e2*e2 + e2;
    check_not_equal(e1, e2);

    debug(0) << "ir_equality_test passed\n";
}

}}
