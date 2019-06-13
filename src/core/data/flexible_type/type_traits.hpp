#ifndef TURI_TYPE_TRAITS_H_
#define TURI_TYPE_TRAITS_H_

#include <type_traits>
#include <map>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <core/generics/gl_vector.hpp>
#include <core/generics/gl_string.hpp>


namespace turi {

template <typename T> class gl_vector;

struct invalid_type {};
struct _invalid_type_base { typedef invalid_type type; };

////////////////////////////////////////////////////////////////////////////////
// Is it an std::vector?

template<class... T> struct is_std_vector : public std::false_type {};
template<typename... A> struct is_std_vector<std::vector<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it a gl_vector?

template<class... T> struct is_gl_vector : public std::false_type {};
template<typename... A> struct is_gl_vector<gl_vector<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it any sort of vector?

template<typename... A> struct is_vector {
  static constexpr bool value = (is_std_vector<A...>::value || is_gl_vector<A...>::value);
};

////////////////////////////////////////////////////////////////////////////////
// Is it any sort of deque?

template<typename... T> struct is_deque : public std::false_type {};
template<typename... A> struct is_deque<std::deque<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it any sort of list?

template<typename... T> struct is_list : public std::false_type {};
template<typename... A> struct is_list<std::list<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it any of the above sequence operators?

template<typename... A> struct is_sequence_container {
  static constexpr bool value =
      (is_vector<A...>::value || is_deque<A...>::value || is_list<A...>::value);
};

////////////////////////////////////////////////////////////////////////////////
// Is it an std::map?

template<class T> struct is_std_map : public std::false_type {};
template<typename... A> struct is_std_map<std::map<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it an std::unordered_map?

template<class T> struct is_std_unordered_map : public std::false_type {};
template<typename... A> struct is_std_unordered_map<std::unordered_map<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it an boost::unordered_map?

template<class T> struct is_boost_unordered_map : public std::false_type {};
template<typename... A> struct is_boost_unordered_map<boost::unordered_map<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it any type of map?

template<typename... A> struct is_map {
  static constexpr bool value = (is_std_map<A...>::value
                                 || is_std_unordered_map<A...>::value
                                 || is_boost_unordered_map<A...>::value);
};

////////////////////////////////////////////////////////////////////////////////
// Is it an std::pair?

template<class T> struct is_std_pair : public std::false_type {};
template<typename... A> struct is_std_pair<std::pair<A...> > : public std::true_type {};


////////////////////////////////////////////////////////////////////////////////
// Is it an std::string?

template<class... T> struct is_std_string : public std::false_type {};
template <> struct is_std_string<std::string> : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it a gl_string?

template<class... T> struct is_gl_string : public std::false_type {};
template <> struct is_gl_string<gl_string> : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////
// Is it any sort of string?

template<typename... A> struct is_string {
  static constexpr bool value = (is_std_string<A...>::value || is_gl_string<A...>::value);
};

////////////////////////////////////////////////////////////////////////////////
// Is tuple?

template<class T> struct is_tuple : public std::false_type {};
template<typename... A> struct is_tuple<std::tuple<A...> > : public std::true_type {};

////////////////////////////////////////////////////////////////////////////////

/** Extract the first nested type from a template parameterized type
 *  definition; return invalid_type on failure.
 *
 *  Example 1:  second_nested_type<std::map<K, V, ...> >::type  // Equal to K.
 *  Example 2:  second_nested_type<std::pair<K, V> >::type      // Equal to K.
 *  Example 3:  second_nested_type<std::vector<K, ...> >::type  // Equal to K.
 */
template<class T> struct first_nested_type {
  typedef invalid_type type;
};

template<class T, typename... A, template <typename...> class C>
struct first_nested_type<C<T, A...> > {
  typedef T type;
};

template<class T, template <typename> class C>
struct first_nested_type<C<T> > {
  typedef T type;
};

////////////////////////////////////////////////////////////////////////////////

/** Extract the second nested type from a template parameterized type
 *  definition; return invalid_type on failure.
 *
 *  Example 1:  second_nested_type<std::map<K, V, ...> >::type  // Equal to V.
 *  Example 2:  second_nested_type<std::pair<K, V> >::type      // Equal to V.
 */
template<class T> struct second_nested_type {
  typedef invalid_type type;
};

template<class T, class U, typename... A, template <typename...> class C>
struct second_nested_type<C<T, U, A...> > {
  typedef U type;
};

template<class T, class U, template <typename...> class C>
struct second_nested_type<C<T, U> > {
  typedef U type;
};

////////////////////////////////////////////////////////////////////////////////
/* All true.
 *
 * Use: all_nested_true<P, Args...>::value.  value is
 * true if P<A>::value is true for all A in Args...
 */
template <template <typename> class P, typename...>
struct all_true : public std::false_type {};

template <template <typename> class P, class T>
struct all_true<P, T> {
  static constexpr bool value = P<T>::value;
};

template<template <typename> class P, class T, typename ... Args>
struct all_true<P, T, Args...> {
  static constexpr bool value = (P<T>::value && all_true<P, Args...>::value);
};

////////////////////////////////////////////////////////////////////////////////

/* All nested true -- mainly useful for tuples.
 *
 * Use: all_nested_true<P, std::tuple<Args...> >::value.  value is
 * true if P<A>::value is true for all A in Args...
 */
template <template <typename> class P, typename...> struct all_nested_true : public std::false_type {};

template<template <typename> class P, template <typename...> class C, typename... Args>
struct all_nested_true<P, C<Args...> > {
  static constexpr bool value = all_true<P, Args...>::value;
};

////////////////////////////////////////////////////////////////////////////////

/** Conditional test -- the use case here is that the test is not even
 *  instantiated if the condition fails.  This has been useful to
 *  eliminate certain recursions.
 *
 *  Use: conditional_test<bool c, Cond, Args...>::value.  If c is
 *  true, then Cond<Args...> is instantiated and the value is equal to
 *  Cond<Args...>::value.  If c is false, the value is false.
 */
template <bool, template <typename...> class Cond, typename... Args> struct conditional_test
    : public std::false_type {};

template <template <typename...> class Cond, typename... Args>
struct conditional_test<true, Cond, Args...> {

  template <template <typename...> class _Cond, typename... _Args>
  static constexpr bool _value(typename std::enable_if<_Cond<_Args...>::value, int>::type = 0) {
    return true;
  }

  template <template <typename...> class _Cond, typename... _Args>
  static constexpr bool _value(typename std::enable_if<!_Cond<_Args...>::value, int>::type = 0) {
    return false;
  }

  static constexpr bool value = _value<Cond, Args...>();
};

////////////////////////////////////////////////////////////////////////////////
// Int that fits in double

template <typename T> struct fits_in_4bytes {
  static constexpr bool value = (sizeof(T) <= 4);
};

template <typename T> struct is_integer_in_4bytes {
  static constexpr bool value = conditional_test<std::is_integral<T>::value, fits_in_4bytes, T>::value;
};

////////////////////////////////////////////////////////////////////////////////
// Swallow to false.  Conditional instantiation for static_assert statements.

template<typename T> struct swallow_to_false : std::false_type {};

////////////////////////////////////////////////////////////////////////////////
// Remove all modifiers
//
template <typename T> struct base_type           { typedef T type; };
template <typename T> struct base_type<T&>       { typedef T type; };
template <typename T> struct base_type<const T&> { typedef T type; };
template <typename T> struct base_type<T&&>      { typedef T type; };
}


#endif /* TURI_TYPE_TRAITS_H_ */
