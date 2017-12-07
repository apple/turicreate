// Always use assert, even if llvm-config defines NDEBUG
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

#ifndef HALIDE_UTIL_H
#define HALIDE_UTIL_H

/** \file
 * Various utility functions used internally Halide. */

#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <cstring>

// by default, the symbol EXPORT does nothing. In windows dll builds we can define it to __declspec(dllexport)
#if defined(_WIN32) && defined(Halide_SHARED)
#ifdef Halide_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
#else
#define EXPORT
#endif

// If we're in user code, we don't want certain functions to be inlined.
#if defined(COMPILING_HALIDE) || defined(BUILDING_PYTHON)
#define NO_INLINE
#else
#ifdef _WIN32
#define NO_INLINE __declspec(noinline)
#else
#define NO_INLINE __attribute__((noinline))
#endif
#endif

// On windows, Halide needs a larger stack than the default MSVC provides
#ifdef _MSC_VER
#pragma comment(linker, "/STACK:8388608,1048576")
#endif

namespace Halide {
namespace Internal {

/** An aggressive form of reinterpret cast used for correct type-punning. */
template<typename DstType, typename SrcType>
DstType reinterpret_bits(const SrcType &src) {
    static_assert(sizeof(SrcType) == sizeof(DstType), "Types must be same size");
    DstType dst;
    memcpy(&dst, &src, sizeof(SrcType));
    return dst;
}

/** Perform a left fold of a vector. Returns a default-constructed
 * vector element if the vector is empty. Similar to std::accumulate
 * but with a less clunky syntax. */
template<typename T, typename Fn>
T fold_left(const std::vector<T> &vec, Fn f) {
    T result;
    if (vec.empty()) {
        return result;
    }
    result = vec[0];
    for (size_t i = 1; i < vec.size(); i++) {
        result = f(result, vec[i]);
    }
    return result;
}

/** Returns a right fold of a vector. Returns a default-constructed
 * vector element if the vector is empty. */
template<typename T, typename Fn>
T fold_right(const std::vector<T> &vec, Fn f) {
    T result;
    if (vec.empty()) {
        return result;
    }
    result = vec.back();
    for (size_t i = vec.size()-1; i > 0; i--) {
        result = f(vec[i-1], result);
    }
    return result;
}

template <typename T1, typename T2, typename T3, typename T4 >
inline NO_INLINE void collect_paired_args(std::vector<std::pair<T1, T2>> &collected_args,
                                     const T3 &a1, const T4 &a2) {
    collected_args.push_back(std::pair<T1, T2>(a1, a2));
}

template <typename T1, typename T2, typename T3, typename T4, typename ...Args>
inline NO_INLINE void collect_paired_args(std::vector<std::pair<T1, T2>> &collected_args,
                                   const T3 &a1, const T4 &a2, Args&&... args) {
    collected_args.push_back(std::pair<T1, T2>(a1, a2));
    collect_paired_args(collected_args, std::forward<Args>(args)...);
}

template<typename... T>
struct meta_and : std::true_type {};

template<typename T1, typename... Args>
struct meta_and<T1, Args...> : std::integral_constant<bool, T1::value && meta_and<Args...>::value> {};

template<typename... T>
struct meta_or : std::false_type {};

template<typename T1, typename... Args>
struct meta_or<T1, Args...> : std::integral_constant<bool, T1::value || meta_or<Args...>::value> {};

template<typename To, typename... Args>
struct all_are_convertible : meta_and<std::is_convertible<Args, To>...> {};

/** Returns base name and fills in namespaces, outermost one first in vector. */
EXPORT std::string extract_namespaces(const std::string &name, std::vector<std::string> &namespaces);


/** Routines to test if math would overflow for signed integers with
 * the given number of bits. */
// @{
bool add_would_overflow(int bits, int64_t a, int64_t b);
bool sub_would_overflow(int bits, int64_t a, int64_t b);
bool mul_would_overflow(int bits, int64_t a, int64_t b);
// @}

// Wrappers for some C++14-isms that are useful and trivially implementable
// in C++11; these are defined in the Halide::Internal namespace. If we
// are compiling under C++14 or later, we just use the standard implementations
// rather than our own.
#if __cplusplus >= 201402L

// C++14: Use the standard implementations
using std::integer_sequence;
using std::make_integer_sequence;
using std::index_sequence;
using std::make_index_sequence;

#else

// C++11: std::integer_sequence (etc) is standard in C++14 but not C++11, but
// is easily written in C++11. This is a simple version that could
// probably be improved.

template<typename T, T... Ints>
struct integer_sequence {
    static constexpr size_t size() { return sizeof...(Ints); }
};

template<typename T>
struct next_integer_sequence;

template<typename T, T... Ints>
struct next_integer_sequence<integer_sequence<T, Ints...>> {
    using type = integer_sequence<T, Ints..., sizeof...(Ints)>;
};

template<typename T, T I, T N>
struct make_integer_sequence_helper {
    using type = typename next_integer_sequence<
        typename make_integer_sequence_helper<T, I+1, N>::type
    >::type;
};

template<typename T, T N>
struct make_integer_sequence_helper<T, N, N> {
    using type = integer_sequence<T>;
};

template<typename T, T N>
using make_integer_sequence = typename make_integer_sequence_helper<T, 0, N>::type;

template<size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

template<size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

#endif

}  // namespace Internal
}  // namespace Halide

#endif
