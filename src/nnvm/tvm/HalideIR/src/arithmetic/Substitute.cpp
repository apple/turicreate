#include "Substitute.h"
#include "Scope.h"
#include "ir/IRMutator.h"
#include "ir/IREquality.h"

namespace Halide {
namespace Internal {

using std::map;
using std::string;

class Substitute : public IRMutator {
    /* We don't need a Scope to check if variable inside let statements has
       same name as the first argument because we use variable pointer to
       match. */
    const map<const Variable*, Expr> &replace;

    Expr find_replacement(const Variable* s) {
        map<const Variable*, Expr>::const_iterator iter = replace.find(s);
        if (iter != replace.end()) {
            return iter->second;
        } else {
            return Expr();
        }
    }

public:
    Substitute(const map<const Variable*, Expr> &m) : replace(m) {}

    using IRMutator::visit;

    void visit(const Variable *v, const Expr &e) {
        Expr r = find_replacement(v);
        if (r.defined()) {
            expr = r;
        } else {
            expr = e;
        }
    }

    void visit(const Let *op, const Expr &e) {
        Expr new_value = mutate(op->value);
        Expr new_body = mutate(op->body);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
          expr = e;
        } else {
          expr = Let::make(op->var, new_value, new_body);
        }
    }

    void visit(const LetStmt *op, const Stmt &s) {
        Expr new_value = mutate(op->value);
        Stmt new_body = mutate(op->body);

        if (new_value.same_as(op->value) &&
            new_body.same_as(op->body)) {
          stmt = s;
        } else {
          stmt = LetStmt::make(op->var, new_value, new_body);
        }
    }

    void visit(const For *op, const Stmt &s) {
        Expr new_min = mutate(op->min);
        Expr new_extent = mutate(op->extent);
        Stmt new_body = mutate(op->body);

        if (new_min.same_as(op->min) &&
            new_extent.same_as(op->extent) &&
            new_body.same_as(op->body)) {
          stmt = s;
        } else {
          stmt = For::make(op->loop_var, new_min, new_extent, op->for_type, op->device_api, new_body);
        }
    }

};

Expr substitute(const Variable* var, Expr replacement, Expr expr) {
    map<const Variable*, Expr> m;
    m[var] = replacement;
    Substitute s(m);
    return s.mutate(expr);
}

Stmt substitute(const Variable* var, Expr replacement, Stmt stmt) {
    map<const Variable*, Expr> m;
    m[var] = replacement;
    Substitute s(m);
    return s.mutate(stmt);
}

Expr substitute(const map<const Variable*, Expr> &m, Expr expr) {
    Substitute s(m);
    return s.mutate(expr);
}

Stmt substitute(const map<const Variable*, Expr> &m, Stmt stmt) {
    Substitute s(m);
    return s.mutate(stmt);
}


class SubstituteExpr : public IRMutator {
public:
    Expr find, replacement;

    using IRMutator::mutate;

    Expr mutate(Expr e) {
        if (equal(e, find)) {
            return replacement;
        } else {
            return IRMutator::mutate(e);
        }
    }
};

Expr substitute(Expr find, Expr replacement, Expr expr) {
    SubstituteExpr s;
    s.find = find;
    s.replacement = replacement;
    return s.mutate(expr);
}

Stmt substitute(Expr find, Expr replacement, Stmt stmt) {
    SubstituteExpr s;
    s.find = find;
    s.replacement = replacement;
    return s.mutate(stmt);
}

/** Substitute an expr for a var in a graph. */
class GraphSubstitute : public IRGraphMutator {
    const Variable* var;
    Expr value;

    using IRGraphMutator::visit;

    void visit(const Variable *op, const Expr &e) {
        if (op == var) {
          expr = value;
        } else {
          expr = e;
        }
    }

public:

    GraphSubstitute(const Variable* var, Expr value) : var(var), value(value) {}
};

/** Substitute an Expr for another Expr in a graph. Unlike substitute,
 * this only checks for shallow equality. */
class GraphSubstituteExpr : public IRGraphMutator {
    Expr find, replace;
public:

    using IRGraphMutator::mutate;

    Expr mutate(Expr e) {
        if (e.same_as(find)) return replace;
        return IRGraphMutator::mutate(e);
    }

    GraphSubstituteExpr(Expr find, Expr replace) : find(find), replace(replace) {}
};

Expr graph_substitute(const Variable* var, Expr replacement, Expr expr) {
    return GraphSubstitute(var, replacement).mutate(expr);
}

Stmt graph_substitute(const Variable* var, Expr replacement, Stmt stmt) {
    return GraphSubstitute(var, replacement).mutate(stmt);
}

Expr graph_substitute(Expr find, Expr replacement, Expr expr) {
    return GraphSubstituteExpr(find, replacement).mutate(expr);
}

Stmt graph_substitute(Expr find, Expr replacement, Stmt stmt) {
    return GraphSubstituteExpr(find, replacement).mutate(stmt);
}

class SubstituteInAllLets : public IRGraphMutator {

    using IRGraphMutator::visit;

    void visit(const Let *op, const Expr &) {
        Expr value = mutate(op->value);
        Expr body = mutate(op->body);
        expr = graph_substitute(op->var, value, body);
    }
};

Expr substitute_in_all_lets(Expr expr) {
    return SubstituteInAllLets().mutate(expr);
}

Stmt substitute_in_all_lets(Stmt stmt) {
    return SubstituteInAllLets().mutate(stmt);
}

}
}
