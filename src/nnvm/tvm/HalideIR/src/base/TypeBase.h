#ifndef HALIDE_TYPEBASE_H
#define HALIDE_TYPEBASE_H

// type handling code stripped from Halide runtime

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
// Forward declare type to allow naming typed handles.
// See Type.h for documentation.
template<typename T> struct halide_handle_traits;

/** Types in the halide type system. They can be ints, unsigned ints,
 * or floats (of various bit-widths), or a handle (which is always 64-bits).
 * Note that the int/uint/float values do not imply a specific bit width
 * (the bit width is expected to be encoded in a separate value).
 */
typedef enum halide_type_code_t
#if __cplusplus >= 201103L
: uint8_t
#endif
{
    halide_type_int = 0,   //!< signed integers
    halide_type_uint = 1,  //!< unsigned integers
    halide_type_float = 2, //!< floating point numbers
    halide_type_handle = 3 //!< opaque pointer type (void *)
} halide_type_code_t;

// Note that while __attribute__ can go before or after the declaration,
// __declspec apparently is only allowed before.
#ifndef HALIDE_ATTRIBUTE_ALIGN
    #ifdef _MSC_VER
        #define HALIDE_ATTRIBUTE_ALIGN(x) __declspec(align(x))
    #else
        #define HALIDE_ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
    #endif
#endif

/** A runtime tag for a type in the halide type system. Can be ints,
 * unsigned ints, or floats of various bit-widths (the 'bits'
 * field). Can also be vectors of the same (by setting the 'lanes'
 * field to something larger than one). This struct should be
 * exactly 32-bits in size. */
struct halide_type_t {
    /** The basic type code: signed integer, unsigned integer, or floating point. */
#if __cplusplus >= 201103L
    HALIDE_ATTRIBUTE_ALIGN(1) halide_type_code_t code; // halide_type_code_t
#else
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t code; // halide_type_code_t
#endif

    /** The number of bits of precision of a single scalar value of this type. */
    HALIDE_ATTRIBUTE_ALIGN(1) uint8_t bits;

    /** How many elements in a vector. This is 1 for scalar types. */
    HALIDE_ATTRIBUTE_ALIGN(2) uint16_t lanes;

#ifdef __cplusplus
    /** Construct a runtime representation of a Halide type from:
     * code: The fundamental type from an enum.
     * bits: The bit size of one element.
     * lanes: The number of vector elements in the type. */
    halide_type_t(halide_type_code_t code, uint8_t bits, uint16_t lanes = 1)
        : code(code), bits(bits), lanes(lanes) {
    }

    /** Default constructor is required e.g. to declare halide_trace_event
     * instances. */
    halide_type_t() : code((halide_type_code_t)0), bits(0), lanes(0) {}

    /** Compare two types for equality. */
    bool operator==(const halide_type_t &other) const {
        return (code == other.code &&
                bits == other.bits &&
                lanes == other.lanes);
    }

    /** Size in bytes for a single element, even if width is not 1, of this type. */
    size_t bytes() const { return (bits + 7) / 8; }
#endif
};

namespace {

template<typename T>
struct halide_type_of_helper;

template<typename T>
struct halide_type_of_helper<T *> {
    operator halide_type_t() {
        return halide_type_t(halide_type_handle, 64);
    }
};

template<typename T>
struct halide_type_of_helper<T &> {
    operator halide_type_t() {
        return halide_type_t(halide_type_handle, 64);
    }
};

// Halide runtime does not require C++11
#if __cplusplus > 199711L
template<typename T>
struct halide_type_of_helper<T &&> {
    operator halide_type_t() {
        return halide_type_t(halide_type_handle, 64);
    }
};
#endif

template<>
struct halide_type_of_helper<float> {
    operator halide_type_t() { return halide_type_t(halide_type_float, 32); }
};

template<>
struct halide_type_of_helper<double> {
    operator halide_type_t() { return halide_type_t(halide_type_float, 64); }
};

template<>
struct halide_type_of_helper<uint8_t> {
    operator halide_type_t() { return halide_type_t(halide_type_uint, 8); }
};

template<>
struct halide_type_of_helper<uint16_t> {
    operator halide_type_t() { return halide_type_t(halide_type_uint, 16); }
};

template<>
struct halide_type_of_helper<uint32_t> {
    operator halide_type_t() { return halide_type_t(halide_type_uint, 32); }
};

template<>
struct halide_type_of_helper<uint64_t> {
    operator halide_type_t() { return halide_type_t(halide_type_uint, 64); }
};

template<>
struct halide_type_of_helper<int8_t> {
    operator halide_type_t() { return halide_type_t(halide_type_int, 8); }
};

template<>
struct halide_type_of_helper<int16_t> {
    operator halide_type_t() { return halide_type_t(halide_type_int, 16); }
};

template<>
struct halide_type_of_helper<int32_t> {
    operator halide_type_t() { return halide_type_t(halide_type_int, 32); }
};

template<>
struct halide_type_of_helper<int64_t> {
    operator halide_type_t() { return halide_type_t(halide_type_int, 64); }
};

template<>
struct halide_type_of_helper<bool> {
    operator halide_type_t() { return halide_type_t(halide_type_uint, 1); }
};

}

/** Construct the halide equivalent of a C type */
template<typename T> halide_type_t halide_type_of() {
    return halide_type_of_helper<T>();
}

// it is not necessary, and may produce warnings for some build configurations.
#ifdef _MSC_VER
#define HALIDE_ALWAYS_INLINE __forceinline
#else
#define HALIDE_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

#endif // HALIDE_HALIDERUNTIME_H
