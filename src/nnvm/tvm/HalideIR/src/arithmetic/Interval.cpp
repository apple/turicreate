#include "Interval.h"
#include "ir/IROperator.h"
#include "ir/IREquality.h"

namespace Halide {
namespace Internal {

// This is called repeatedly by bounds inference and the solver to
// build large expressions, so we want to simplify eagerly to avoid
// monster expressions.
Expr Interval::make_max(Expr a, Expr b) {
    if (a.same_as(b)) return a;

    // Deal with infinities
    if (a.same_as(Interval::pos_inf)) return a;
    if (b.same_as(Interval::pos_inf)) return b;
    if (a.same_as(Interval::neg_inf)) return b;
    if (b.same_as(Interval::neg_inf)) return a;

    // Deep equality
    if (equal(a, b)) return a;

    // Constant fold
    const int64_t *ia = as_const_int(a);
    const int64_t *ib = as_const_int(b);
    const uint64_t *ua = as_const_uint(a);
    const uint64_t *ub = as_const_uint(b);
    const double *fa = as_const_float(a);
    const double *fb = as_const_float(b);
    if (ia && ib) return (*ia > *ib) ? a : b;
    if (ua && ub) return (*ua > *ub) ? a : b;
    if (fa && fb) return (*fa > *fb) ? a : b;

    // Balance trees to the left, with constants pushed rightwards
    const Max *ma = a.as<Max>();
    const Max *mb = b.as<Max>();
    if (mb && !ma && !(is_const(mb->a) && is_const(mb->b))) {
        std::swap(ma, mb);
        std::swap(a, b);
    }
    if (ma && is_const(ma->b) && is_const(b)) {
        return Interval::make_max(ma->a, Interval::make_max(ma->b, b));
    }
    if (ma && (ma->a.same_as(b) || ma->b.same_as(b))) {
        // b is already represented in a
        return a;
    }

    return Max::make(a, b);
}

Expr Interval::make_min(Expr a, Expr b) {
    if (a.same_as(b)) return a;

    // Deal with infinities
    if (a.same_as(Interval::pos_inf)) return b;
    if (b.same_as(Interval::pos_inf)) return a;
    if (a.same_as(Interval::neg_inf)) return a;
    if (b.same_as(Interval::neg_inf)) return b;

    // Deep equality
    if (equal(a, b)) return a;

    // Constant fold
    const int64_t *ia = as_const_int(a);
    const int64_t *ib = as_const_int(b);
    const uint64_t *ua = as_const_uint(a);
    const uint64_t *ub = as_const_uint(b);
    const double *fa = as_const_float(a);
    const double *fb = as_const_float(b);
    if (ia && ib) return (*ia > *ib) ? b : a;
    if (ua && ub) return (*ua > *ub) ? b : a;
    if (fa && fb) return (*fa > *fb) ? b : a;

    // Balance trees to the left, with constants pushed rightwards
    const Min *ma = a.as<Min>();
    const Min *mb = b.as<Min>();
    if (mb && !ma && !(is_const(mb->a) && is_const(mb->b))) {
        std::swap(ma, mb);
        std::swap(a, b);
    }
    if (ma && is_const(ma->b) && is_const(b)) {
        return Interval::make_min(ma->a, Interval::make_min(ma->b, b));
    }
    if (ma && (ma->a.same_as(b) || ma->b.same_as(b))) {
        // b is already represented in a
        return a;
    }

    return Min::make(a, b);
}

void Interval::include(const Interval &i) {
    max = Interval::make_max(max, i.max);
    min = Interval::make_min(min, i.min);
}

void Interval::include(Expr e) {
    max = Interval::make_max(max, e);
    min = Interval::make_min(min, e);
}

Interval Interval::make_union(const Interval &a, const Interval &b) {
    Interval result = a;
    result.include(b);
    return result;
}

Interval Interval::make_intersection(const Interval &a, const Interval &b) {
    return Interval(Interval::make_max(a.min, b.min),
                    Interval::make_min(a.max, b.max));
}

// Use Handle types for positive and negative infinity, to prevent
// accidentally doing arithmetic on them.
Expr Interval::pos_inf = Variable::make(Handle(), "pos_inf");
Expr Interval::neg_inf = Variable::make(Handle(), "neg_inf");


namespace {
void check(Interval result, Interval expected, int line) {
    internal_assert(equal(result.min, expected.min) &&
                    equal(result.max, expected.max))
        << "Interval test on line " << line << " failed\n"
        << "  Expected [" << expected.min << ", " << expected.max << "]\n"
        << "  Got      [" << result.min << ", " << result.max << "]\n";
}
}

void interval_test() {
    Interval e = Interval::everything();
    Interval n = Interval::nothing();
    Expr x = Variable::make(Int(32), "x");
    Interval xp{x, Interval::pos_inf};
    Interval xn{Interval::neg_inf, x};
    Interval xx{x, x};

    internal_assert(e.is_everything());
    internal_assert(!e.has_upper_bound());
    internal_assert(!e.has_lower_bound());
    internal_assert(!e.is_empty());
    internal_assert(!e.is_bounded());
    internal_assert(!e.is_single_point());

    internal_assert(!n.is_everything());
    internal_assert(!n.has_upper_bound());
    internal_assert(!n.has_lower_bound());
    internal_assert(n.is_empty());
    internal_assert(!n.is_bounded());
    internal_assert(!n.is_single_point());

    internal_assert(!xp.is_everything());
    internal_assert(!xp.has_upper_bound());
    internal_assert(xp.has_lower_bound());
    internal_assert(!xp.is_empty());
    internal_assert(!xp.is_bounded());
    internal_assert(!xp.is_single_point());

    internal_assert(!xn.is_everything());
    internal_assert(xn.has_upper_bound());
    internal_assert(!xn.has_lower_bound());
    internal_assert(!xn.is_empty());
    internal_assert(!xn.is_bounded());
    internal_assert(!xn.is_single_point());

    internal_assert(!xx.is_everything());
    internal_assert(xx.has_upper_bound());
    internal_assert(xx.has_lower_bound());
    internal_assert(!xx.is_empty());
    internal_assert(xx.is_bounded());
    internal_assert(xx.is_single_point());

    check(Interval::make_union(xp, xn), e, __LINE__);
    check(Interval::make_union(e, xn), e, __LINE__);
    check(Interval::make_union(xn, e), e, __LINE__);
    check(Interval::make_union(xn, n), xn, __LINE__);
    check(Interval::make_union(n, xp), xp, __LINE__);
    check(Interval::make_union(xp, xp), xp, __LINE__);

    check(Interval::make_intersection(xp, xn), Interval::single_point(x), __LINE__);
    check(Interval::make_intersection(e, xn), xn, __LINE__);
    check(Interval::make_intersection(xn, e), xn, __LINE__);
    check(Interval::make_intersection(xn, n), n, __LINE__);
    check(Interval::make_intersection(n, xp), n, __LINE__);
    check(Interval::make_intersection(xp, xp), xp, __LINE__);

    check(Interval::make_union({3, Interval::pos_inf}, {5, Interval::pos_inf}), {3, Interval::pos_inf}, __LINE__);
    check(Interval::make_intersection({3, Interval::pos_inf}, {5, Interval::pos_inf}), {5, Interval::pos_inf}, __LINE__);

    check(Interval::make_union({Interval::neg_inf, 3}, {Interval::neg_inf, 5}), {Interval::neg_inf, 5}, __LINE__);
    check(Interval::make_intersection({Interval::neg_inf, 3}, {Interval::neg_inf, 5}), {Interval::neg_inf, 3}, __LINE__);

    check(Interval::make_union({3, 4}, {9, 10}), {3, 10}, __LINE__);
    check(Interval::make_intersection({3, 4}, {9, 10}), {9, 4}, __LINE__);

    check(Interval::make_union({3, 9}, {4, 10}), {3, 10}, __LINE__);
    check(Interval::make_intersection({3, 9}, {4, 10}), {4, 9}, __LINE__);

    std::cout << "Interval test passed" << std::endl;
}


}
}
