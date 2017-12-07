#include <sstream>
#include <cfloat>
// TODO(tqchen): remove recursive dep on IR?
#include "ir/IR.h"

namespace Halide {

using std::ostringstream;

namespace {
uint64_t max_uint(int bits) {
    uint64_t max_val = 0xffffffffffffffffULL;
    return max_val >> (64 - bits);
}

int64_t max_int(int bits) {
    int64_t  max_val = 0x7fffffffffffffffLL;
    return max_val >> (64 - bits);
}

int64_t min_int(int bits) {
    return -max_int(bits) - 1;
}

}

/** Return an expression which is the maximum value of this type */
Halide::Expr Type::max() const {
    if (is_vector()) {
        return Internal::Broadcast::make(element_of().max(), lanes());
    } else if (is_int()) {
        return Internal::IntImm::make(*this, max_int(bits()));
    } else if (is_uint()) {
        return Internal::UIntImm::make(*this, max_uint(bits()));
    } else {
        internal_assert(is_float());
        if (bits() == 16) {
            return Internal::FloatImm::make(*this, 65504.0);
        } else if (bits() == 32) {
            return Internal::FloatImm::make(*this, FLT_MAX);
        } else if (bits() == 64) {
            return Internal::FloatImm::make(*this, DBL_MAX);
        } else {
            internal_error
                << "Unknown float type: " << (*this) << "\n";
            return 0;
        }
    }
}

/** Return an expression which is the minimum value of this type */
Halide::Expr Type::min() const {
    if (is_vector()) {
        return Internal::Broadcast::make(element_of().min(), lanes());
    } else if (is_int()) {
        return Internal::IntImm::make(*this, min_int(bits()));
    } else if (is_uint()) {
        return Internal::UIntImm::make(*this, 0);
    } else {
        internal_assert(is_float());
        if (bits() == 16) {
            return Internal::FloatImm::make(*this, -65504.0);
        } else if (bits() == 32) {
            return Internal::FloatImm::make(*this, -FLT_MAX);
        } else if (bits() == 64) {
            return Internal::FloatImm::make(*this, -DBL_MAX);
        } else {
            internal_error
                << "Unknown float type: " << (*this) << "\n";
            return 0;
        }
    }
}

bool Type::is_max(int64_t x) const {
    return x > 0 && is_max((uint64_t)x);
}

bool Type::is_max(uint64_t x) const {
    if (is_int()) {
        return x == (uint64_t)max_int(bits());
    } else if (is_uint()) {
        return x == max_uint(bits());
    } else {
        return false;
    }
}

bool Type::is_min(int64_t x) const {
    if (is_int()) {
        return x == min_int(bits());
    } else if (is_uint()) {
        return x == 0;
    } else {
        return false;
    }
}

bool Type::is_min(uint64_t x) const {
    return false;
}

bool Type::can_represent(Type other) const {
    if (lanes() != other.lanes()) return false;
    if (is_int()) {
        return ((other.is_int() && other.bits() <= bits()) ||
                (other.is_uint() && other.bits() < bits()));
    } else if (is_uint()) {
        return other.is_uint() && other.bits() <= bits();
    } else if (is_float()) {
        return ((other.is_float() && other.bits() <= bits()) ||
                (bits() == 64 && other.bits() <= 32) ||
                (bits() == 32 && other.bits() <= 16));
    } else {
        return false;
    }
}

bool Type::can_represent(int64_t x) const {
    if (is_int()) {
        return x >= min_int(bits()) && x <= max_int(bits());
    } else if (is_uint()) {
        return x >= 0 && (uint64_t)x <= max_uint(bits());
    } else if (is_float()) {
        switch (bits()) {
        case 16:
            return (int64_t)(float)(float16_t)(float)x == x;
        case 32:
            return (int64_t)(float)x == x;
        case 64:
            return (int64_t)(double)x == x;
        default:
            return false;
        }
    } else {
        return false;
    }
}

bool Type::can_represent(uint64_t x) const {
    if (is_int()) {
        return x <= (uint64_t)(max_int(bits()));
    } else if (is_uint()) {
        return x <= max_uint(bits());
    } else if (is_float()) {
        switch (bits()) {
        case 16:
            return (uint64_t)(float)(float16_t)(float)x == x;
        case 32:
            return (uint64_t)(float)x == x;
        case 64:
            return (uint64_t)(double)x == x;
        default:
            return false;
        }
    } else {
        return false;
    }
}

bool Type::can_represent(double x) const {
    if (is_int()) {
        int64_t i = x;
        return (x >= min_int(bits())) && (x <= max_int(bits())) && (x == (double)i);
    } else if (is_uint()) {
        uint64_t u = x;
        return (x >= 0) && (x <= max_uint(bits())) && (x == (double)u);
    } else if (is_float()) {
        switch (bits()) {
        case 16:
            return (double)(float16_t)x == x;
        case 32:
            return (double)(float)x == x;
        case 64:
            return true;
        default:
            return false;
        }
    } else {
        return false;
    }
}

bool Type::same_handle_type(const Type &other) const {
    const halide_handle_cplusplus_type *first = handle_type;
    const halide_handle_cplusplus_type *second = other.handle_type;

    if (first == second) {
        return true;
    }

    if (first == nullptr) {
        first = halide_handle_traits<void*>::type_info();
    }
    if (second == nullptr) {
        second = halide_handle_traits<void*>::type_info();
    }

    return first->inner_name == second->inner_name &&
        first->namespaces == second->namespaces &&
        first->enclosing_types == second->enclosing_types &&
        first->cpp_type_modifiers == second->cpp_type_modifiers &&
        first->reference_type == second->reference_type;
}

}
