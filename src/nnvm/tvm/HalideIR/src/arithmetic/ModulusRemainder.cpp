#include "ModulusRemainder.h"
#include "Simplify.h"
#include "ir/IROperator.h"
#include "ir/IRPrinter.h"
#include "ir/IR.h"

// This file is largely a port of parts of src/analysis.ml
namespace Halide {
namespace Internal {

class ComputeModulusRemainder : public IRVisitor {
public:
    ModulusRemainder analyze(Expr e);

    int modulus, remainder;
    Scope<ModulusRemainder> scope;

    ComputeModulusRemainder(const Scope<ModulusRemainder> *s) {
        scope.set_containing_scope(s);
    }

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
    void visit(const LetStmt *, const Stmt &);
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
    void visit(const Shuffle *, const Expr &);
};

ModulusRemainder modulus_remainder(Expr e) {
    ComputeModulusRemainder mr(nullptr);
    return mr.analyze(e);
}

ModulusRemainder modulus_remainder(Expr e, const Scope<ModulusRemainder> &scope) {
    ComputeModulusRemainder mr(&scope);
    return mr.analyze(e);
}



bool reduce_expr_modulo(Expr expr, int modulus, int *remainder) {
    ModulusRemainder result = modulus_remainder(expr);

    /* As an example: If we asked for expr mod 8, and the analysis
     * said that expr = 16*k + 13, then because 16 % 8 == 0, the
     * result is 13 % 8 == 5. But if the analysis says that expr =
     * 6*k + 3, then expr mod 8 could be 1, 3, 5, or 7, so we just
     * return false.
     */

    if (result.modulus % modulus == 0) {
        *remainder = result.remainder % modulus;
        return true;
    } else {
        return false;
    }
}
bool reduce_expr_modulo(Expr expr, int modulus, int *remainder, const Scope<ModulusRemainder> &scope) {
    ModulusRemainder result = modulus_remainder(expr, scope);

    if (result.modulus % modulus == 0) {
        *remainder = result.remainder % modulus;
        return true;
    } else {
        return false;
    }
}

ModulusRemainder ComputeModulusRemainder::analyze(Expr e) {
    e.accept(this);
    return ModulusRemainder(modulus, remainder);
}

namespace {
void check(Expr e, int m, int r) {
    ModulusRemainder result = modulus_remainder(e);
    if (result.modulus != m || result.remainder != r) {
        std::cerr << "Test failed for modulus_remainder:\n";
        std::cerr << "Expression: " << e << "\n";
        std::cerr << "Correct modulus, remainder  = " << m << ", " << r << "\n";
        std::cerr << "Computed modulus, remainder = "
                  << result.modulus << ", "
                  << result.remainder << "\n";
        exit(-1);
    }
}
}

void modulus_remainder_test() {
    VarExpr x = Variable::make(Int(32), "x");
    VarExpr y = Variable::make(Int(32), "y");

    check((30*x + 3) + (40*y + 2), 10, 5);
    check((6*x + 3) * (4*y + 1), 2, 1);
    check(max(30*x - 24, 40*y + 31), 5, 1);
    check(10*x - 33*y, 1, 0);
    check(10*x - 35*y, 5, 0);
    check(123, 0, 123);
    check(Let::make(y, x*3 + 4, y*3 + 4), 9, 7);

    std::cout << "modulus_remainder test passed\n";
}


void ComputeModulusRemainder::visit(const IntImm *op, const Expr &) {
    // Equal to op->value modulo anything. We'll use zero as the
    // modulus to mark this special case. We'd better be able to
    // handle zero in the rest of the code...
    remainder = op->value;
    modulus = 0;
}

void ComputeModulusRemainder::visit(const UIntImm *op, const Expr &) {
    internal_error << "modulus_remainder of uint\n";
}

void ComputeModulusRemainder::visit(const FloatImm *, const Expr &) {
    internal_error << "modulus_remainder of float\n";
}

void ComputeModulusRemainder::visit(const StringImm *, const Expr &) {
    internal_error << "modulus_remainder of string\n";
}

void ComputeModulusRemainder::visit(const Cast *, const Expr &) {
    modulus = 1;
    remainder = 0;
}

void ComputeModulusRemainder::visit(const Variable *op, const Expr &) {
    if (scope.contains(op)) {
        ModulusRemainder mod_rem = scope.get(op);
        modulus = mod_rem.modulus;
        remainder = mod_rem.remainder;
    } else {
        modulus = 1;
        remainder = 0;
    }
}

int gcd(int a, int b) {
    if (a < b) std::swap(a, b);
    while (b != 0) {
        int64_t tmp = b;
        b = a % b;
        a = tmp;
    }
    return a;
}

int lcm(int a, int b) {
    return (a*b)/gcd(a, b);
}

int mod(int a, int m) {
    if (m == 0) return a;
    return mod_imp(a, m);
}

void ComputeModulusRemainder::visit(const Add *op, const Expr &) {
    ModulusRemainder a = analyze(op->a);
    ModulusRemainder b = analyze(op->b);
    modulus = gcd(a.modulus, b.modulus);
    remainder = mod(a.remainder + b.remainder, modulus);
}

void ComputeModulusRemainder::visit(const Sub *op, const Expr &) {
    ModulusRemainder a = analyze(op->a);
    ModulusRemainder b = analyze(op->b);
    modulus = gcd(a.modulus, b.modulus);
    remainder = mod(a.remainder - b.remainder, modulus);
}

void ComputeModulusRemainder::visit(const Mul *op, const Expr &) {
    ModulusRemainder a = analyze(op->a);
    ModulusRemainder b = analyze(op->b);

    if (a.modulus == 0) {
        // a is constant
        modulus = a.remainder * b.modulus;
        remainder = a.remainder * b.remainder;
    } else if (b.modulus == 0) {
        // b is constant
        modulus = b.remainder * a.modulus;
        remainder = a.remainder * b.remainder;
    } else if (a.remainder == 0 && b.remainder == 0) {
        // multiple times multiple
        modulus = a.modulus * b.modulus;
        remainder = 0;
    } else if (a.remainder == 0) {
        modulus = a.modulus * gcd(b.modulus, b.remainder);
        remainder = 0;
    } else if (b.remainder == 0) {
        modulus = b.modulus * gcd(a.modulus, a.remainder);
        remainder = 0;
    } else {
        // All our tricks failed. Convert them to the same modulus and multiply
        modulus = gcd(a.modulus, b.modulus);
        a.remainder = mod(a.remainder * b.remainder, modulus);
    }
}

void ComputeModulusRemainder::visit(const Div *, const Expr &) {
    // We might be able to say something about this if the numerator
    // modulus is provably a multiple of a constant denominator, but
    // in this case we should have simplified away the division.
    remainder = 0;
    modulus = 1;
}

namespace {
ModulusRemainder unify_alternatives(ModulusRemainder a, ModulusRemainder b) {
    // We don't know if we're going to get a or b, so we'd better find
    // a single modulus remainder that works for both.

    // For example:
    // max(30*_ + 13, 40*_ + 27) ->
    // max(10*_ + 3, 10*_ + 7) ->
    // max(2*_ + 1, 2*_ + 1) ->
    // 2*_ + 1

    // Reduce them to the same modulus and the same remainder
    int modulus = gcd(a.modulus, b.modulus);
    int64_t diff = (int64_t)a.remainder - (int64_t)b.remainder;
    if (!Int(32).can_represent(diff)) {
        // The difference overflows.
        return ModulusRemainder(0, 1);
    }
    if (diff < 0) diff = -diff;
    modulus = gcd((int)diff, modulus);

    int ra = mod(a.remainder, modulus);

    internal_assert(ra == mod(b.remainder, modulus))
        << "There's a bug inside ModulusRemainder in unify_alternatives:\n"
        << "a.modulus         = " << a.modulus << "\n"
        << "a.remainder       = " << a.remainder << "\n"
        << "b.modulus         = " << b.modulus << "\n"
        << "b.remainder       = " << b.remainder << "\n"
        << "diff              = " << diff << "\n"
        << "unified modulus   = " << modulus << "\n"
        << "unified remainder = " << ra << "\n";


    return ModulusRemainder(modulus, ra);
}
}

void ComputeModulusRemainder::visit(const Mod *op, const Expr &) {
    // We can treat x mod y as x + z*y, where we know nothing about z.
    // (ax + b) + z (cx + d) ->
    // ax + b + zcx + dz ->
    // gcd(a, c, d) * w + b

    // E.g:
    // (8x + 5) mod (6x + 2) ->
    // (8x + 5) + z (6x + 2) ->
    // (8x + 6zx + 2x) + 5 ->
    // 2(4x + 3zx + x) + 5 ->
    // 2w + 1
    ModulusRemainder a = analyze(op->a);
    ModulusRemainder b = analyze(op->b);
    modulus = gcd(a.modulus, b.modulus);
    modulus = gcd(modulus, b.remainder);
    remainder = mod(a.remainder, modulus);
}

void ComputeModulusRemainder::visit(const Min *op, const Expr &) {
    ModulusRemainder r = unify_alternatives(analyze(op->a), analyze(op->b));
    modulus = r.modulus;
    remainder = r.remainder;
}

void ComputeModulusRemainder::visit(const Max *op, const Expr &) {
    ModulusRemainder r = unify_alternatives(analyze(op->a), analyze(op->b));
    modulus = r.modulus;
    remainder = r.remainder;
}

void ComputeModulusRemainder::visit(const EQ *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const NE *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const LT *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const LE *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const GT *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const GE *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const And *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const Or *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const Not *, const Expr &) {
    internal_assert(false) << "modulus_remainder of bool\n";
}

void ComputeModulusRemainder::visit(const Select *op, const Expr &) {
    ModulusRemainder r = unify_alternatives(analyze(op->true_value),
                                            analyze(op->false_value));
    modulus = r.modulus;
    remainder = r.remainder;
}

void ComputeModulusRemainder::visit(const Load *, const Expr &) {
    modulus = 1;
    remainder = 0;
}

void ComputeModulusRemainder::visit(const Ramp *, const Expr &) {
    internal_assert(false) << "modulus_remainder of vector\n";
}

void ComputeModulusRemainder::visit(const Broadcast *, const Expr &) {
    internal_assert(false) << "modulus_remainder of vector\n";
}

void ComputeModulusRemainder::visit(const Call *, const Expr &) {
    modulus = 1;
    remainder = 0;
}

void ComputeModulusRemainder::visit(const Let *op, const Expr &) {
    bool value_interesting = op->value.type().is_int();

    if (value_interesting) {
        ModulusRemainder val = analyze(op->value);
        scope.push(op->var.get(), val);
    }
    ModulusRemainder val = analyze(op->body);
    if (value_interesting) {
      scope.pop(op->var.get());
    }
    modulus = val.modulus;
    remainder = val.remainder;
}

void ComputeModulusRemainder::visit(const Shuffle *op, const Expr&) {
    // It's possible that scalar expressions are extracting a lane of a vector - don't fail in this case, but stop
    internal_assert(op->indices.size() == 1) << "modulus_remainder of vector\n";
    modulus = 1;
    remainder = 0;
}

void ComputeModulusRemainder::visit(const LetStmt *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const AssertStmt *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const ProducerConsumer *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const For *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Store *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Provide *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Allocate *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Realize *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Prefetch *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Block *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Free *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const IfThenElse *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

void ComputeModulusRemainder::visit(const Evaluate *, const Stmt &) {
    internal_assert(false) << "modulus_remainder of statement\n";
}

}
}
