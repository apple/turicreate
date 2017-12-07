#ifndef HALIDE_IR_EQUALITY_H
#define HALIDE_IR_EQUALITY_H

/** \file
 * Methods to test Exprs and Stmts for equality of value
 */

#include "IR.h"

namespace Halide {
namespace Internal {

/** A compare struct suitable for use in std::map and std::set that
 * computes a lexical ordering on IR nodes. */
struct IRDeepCompare {
    EXPORT bool operator()(const Expr &a, const Expr &b) const;
    EXPORT bool operator()(const Stmt &a, const Stmt &b) const;
};

/** Lossily track known equal exprs with a cache. On collision, the
 * old pair is evicted. Used below by ExprWithCompareCache. */
class IRCompareCache {
private:
    struct Entry {
        Expr a, b;
    };

    int bits;

    uint32_t hash(const Expr &a, const Expr &b) const {
        // Note this hash is symmetric in a and b, so that a
        // comparison in a and b hashes to the same bucket as
        // a comparison on b and a.
        uint64_t pa = (uint64_t)(a.get());
        uint64_t pb = (uint64_t)(b.get());
        uint64_t mix = (pa + pb) + (pa ^ pb);
        mix ^= (mix >> bits);
        mix ^= (mix >> (bits*2));
        uint32_t bottom = mix & ((1 << bits) - 1);
        return bottom;
    }

    std::vector<Entry> entries;

public:
    void insert(const Expr &a, const Expr &b) {
        uint32_t h = hash(a, b);
        entries[h].a = a;
        entries[h].b = b;
    }

    bool contains(const Expr &a, const Expr &b) const {
        uint32_t h = hash(a, b);
        const Entry &e = entries[h];
        return ((a.same_as(e.a) && b.same_as(e.b)) ||
                (a.same_as(e.b) && b.same_as(e.a)));
    }

    void clear() {
        for (size_t i = 0; i < entries.size(); i++) {
            entries[i].a = Expr();
            entries[i].b = Expr();
        }
    }

    IRCompareCache() {}
    IRCompareCache(int b) : bits(b), entries(static_cast<size_t>(1) << bits) {}
};

/** A wrapper about Exprs so that they can be deeply compared with a
 * cache for known-equal subexpressions. Useful for unsanitized Exprs
 * coming in from the front-end, which may be horrible graphs with
 * sub-expressions that are equal by value but not by identity. This
 * isn't a comparison object like IRDeepCompare above, because libc++
 * requires that comparison objects be stateless (and constructs a new
 * one for each comparison!), so they can't have a cache associated
 * with them. However, by sneakily making the cache a mutable member
 * of the objects being compared, we can dodge this issue.
 *
 * Clunky example usage:
 *
\code
Expr a, b, c, query;
std::set<ExprWithCompareCache> s;
IRCompareCache cache(8);
s.insert(ExprWithCompareCache(a, &cache));
s.insert(ExprWithCompareCache(b, &cache));
s.insert(ExprWithCompareCache(c, &cache));
if (m.contains(ExprWithCompareCache(query, &cache))) {...}
\endcode
 *
 */
struct ExprWithCompareCache {
    Expr expr;
    mutable IRCompareCache *cache;

    ExprWithCompareCache() : cache(nullptr) {}
    ExprWithCompareCache(const Expr &e, IRCompareCache *c) : expr(e), cache(c) {}

    /** The comparison uses (and updates) the cache */
    EXPORT bool operator<(const ExprWithCompareCache &other) const;
};

/** Compare IR nodes for equality of value. Traverses entire IR
 * tree. For equality of reference, use Expr::same_as. If you're
 * comparing non-CSE'd Exprs, use graph_equal, which is safe for nasty
 * graphs of IR nodes. */
// @{
EXPORT bool equal(const Expr &a, const Expr &b);
EXPORT bool equal(const Stmt &a, const Stmt &b);
EXPORT bool graph_equal(const Expr &a, const Expr &b);
EXPORT bool graph_equal(const Stmt &a, const Stmt &b);
// @}



EXPORT void ir_equality_test();

}
}

#endif
