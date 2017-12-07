#include "Deinterleave.h"
#include "base/Debug.h"
#include "ir/IRMutator.h"
#include "ir/IROperator.h"
#include "ir/IREquality.h"
#include "ir/IRPrinter.h"
#include "ModulusRemainder.h"
#include "Scope.h"
#include "Simplify.h"

namespace Halide {
namespace Internal {

using std::pair;

class Deinterleaver : public IRMutator {
public:
    int starting_lane;
    int new_lanes;
    int lane_stride;

    Deinterleaver() {}

private:
    Scope<Expr> internal;

    using IRMutator::visit;

    void visit(const Broadcast *op, const Expr &self) {
        if (new_lanes == 1) {
            expr = op->value;
        } else {
            expr = Broadcast::make(op->value, new_lanes);
        }
    }

    void visit(const Load *op, const Expr &self) {
        if (op->type.is_scalar()) {
            expr = self;
        } else {
            Type t = op->type.with_lanes(new_lanes);
            expr = Load::make(t, op->buffer_var, mutate(op->index), mutate(op->predicate));
        }
    }

    void visit(const Ramp *op, const Expr &self) {
        expr = op->base + starting_lane * op->stride;
        internal_assert(expr.type() == op->base.type());
        if (new_lanes > 1) {
            expr = Ramp::make(expr, op->stride * lane_stride, new_lanes);
        }
    }

    void visit(const Variable *op, const Expr &self) {
        if (op->type.is_scalar()) {
            expr = self;
        } else {
            if (internal.contains(op)) {
                expr = internal.get(op);
            } else {
                // Uh-oh, we don't know how to deinterleave this vector expression
                // Make llvm do it
                Array<Expr> indices;
                for (int i = 0; i < new_lanes; i++) {
                  indices.push_back(IntImm::make(Int(32), starting_lane + lane_stride * i));
                }
                expr = Shuffle::make({self}, indices);
            }
        }
    }

    void visit(const Cast *op, const Expr &self) {
        if (op->type.is_scalar()) {
            expr = self;
        } else {
            Type t = op->type.with_lanes(new_lanes);
            expr = Cast::make(t, mutate(op->value));
        }
    }

    void visit(const Call *op, const Expr &self) {
        Type t = op->type.with_lanes(new_lanes);

        // Don't mutate scalars
        if (op->type.is_scalar()) {
            expr = self;
        } else if (op->is_intrinsic(Call::glsl_texture_load)) {
            // glsl_texture_load returns a <uint x 4> result. Deinterleave by
            // wrapping the call in a shuffle_vector
            Array<Expr> indices;
            for (int i = 0; i < new_lanes; i++) {
                indices.push_back(i*lane_stride + starting_lane);
            }
            expr = Shuffle::make({self}, indices);
        } else {

            // Vector calls are always parallel across the lanes, so we
            // can just deinterleave the args.

            // Beware of other intrinsics for which this is not true!
            // Currently there's only interleave_vectors and
            // shuffle_vector.

            std::vector<Expr> args(op->args.size());
            for (size_t i = 0; i < args.size(); i++) {
                args[i] = mutate(op->args[i]);
            }

            expr = Call::make(t, op->name, args, op->call_type,
                              op->func, op->value_index);
        }
    }

    void visit(const Let *op, const Expr& self) {
        if (op->type.is_vector()) {
            Expr new_value = mutate(op->value);
            Type new_type = new_value.type();
            VarExpr new_var = Variable::make(new_type, "t");
            internal.push(op->var.get(), new_var);
            Expr body = mutate(op->body);
            internal.pop(op->var.get());

            // Define the new name.
            expr = Let::make(new_var, new_value, body);

            // Someone might still use the old name.
            expr = Let::make(op->var, op->value, expr);
        } else {
           IRMutator::visit(op, self);
        }
    }

    void visit(const Shuffle *op, const Expr &self) {
        if (op->is_interleave()) {
            internal_assert(starting_lane >= 0 && starting_lane < lane_stride);
            if ((int)op->vectors.size() == lane_stride) {
                expr = op->vectors[starting_lane];
            } else if ((int)op->vectors.size() % lane_stride == 0) {
                // Pick up every lane-stride vector.
                std::vector<Expr> new_vectors(op->vectors.size() / lane_stride);
                for (size_t i = 0; i < new_vectors.size(); i++) {
                    new_vectors[i] = op->vectors[i*lane_stride + starting_lane];
                }
                expr = Shuffle::make_interleave(new_vectors);
            } else {
                // Interleave some vectors then deinterleave by some other factor...
                // Brute force!
                Array<Expr> indices;
                for (int i = 0; i < new_lanes; i++) {
                   indices.push_back(IntImm::make(Int(32), i*lane_stride + starting_lane));
                }
                expr = Shuffle::make({self}, indices);
            }
        } else {
            // Extract every nth numeric arg to the shuffle.
            Array<Expr> indices;
            for (int i = 0; i < new_lanes; i++) {
                int idx = i * lane_stride + starting_lane;
                indices.push_back(op->indices[idx]);
            }
            expr = Shuffle::make({self}, indices);
        }
    }
};

Expr extract_odd_lanes(Expr e) {
    internal_assert(e.type().lanes() % 2 == 0);
    Deinterleaver d;
    d.starting_lane = 1;
    d.lane_stride = 2;
    d.new_lanes = e.type().lanes()/2;
    e = d.mutate(e);
    return simplify(e);
}

Expr extract_even_lanes(Expr e) {
    internal_assert(e.type().lanes() % 2 == 0);
    Deinterleaver d;
    d.starting_lane = 0;
    d.lane_stride = 2;
    d.new_lanes = (e.type().lanes()+1)/2;
    e = d.mutate(e);
    return simplify(e);
}

Expr extract_lane(Expr e, int lane) {
    Deinterleaver d;
    d.starting_lane = lane;
    d.lane_stride = e.type().lanes();
    d.new_lanes = 1;
    e = d.mutate(e);
    return simplify(e);
}
}
}
