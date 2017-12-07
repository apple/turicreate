/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_FUNCTIONAL_HPP
#define BOOST_POLY_COLLECTION_DETAIL_FUNCTIONAL_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/poly_collection/detail/integer_sequence.hpp>
#include <tuple>
#include <utility>

/* Assorted functional utilities. Much of this would be almost trivial with
 * C++14 generic lambdas.
 */

#if BOOST_WORKAROUND(BOOST_MSVC,>=1910)
/* https://lists.boost.org/Archives/boost/2017/06/235687.php */

#define BOOST_POLY_COLLECTION_DEFINE_OVERLOAD_SET(name,f) \
struct name                                               \
{                                                         \
  template<typename... Args>                              \
  auto operator()(Args&&... args)const                    \
  {                                                       \
    return f(std::forward<Args>(args)...);                \
  }                                                       \
};
#else
#define BOOST_POLY_COLLECTION_DEFINE_OVERLOAD_SET(name,f) \
struct name                                               \
{                                                         \
  template<typename... Args>                              \
  auto operator()(Args&&... args)const->                  \
    decltype(f(std::forward<Args>(args)...))              \
  {                                                       \
    return f(std::forward<Args>(args)...);                \
  }                                                       \
};
#endif

namespace boost{

namespace poly_collection{

namespace detail{

template<typename F,typename Tuple>
struct tail_closure_class
{
  template<typename... Args,std::size_t... I>
  auto call(index_sequence<I...>,Args&&... args)
    ->decltype(std::declval<F>()(
      std::forward<Args>(args)...,
      std::get<I>(std::declval<Tuple>())...))
  {
    return f(std::forward<Args>(args)...,std::get<I>(t)...);
  }

  template<typename... Args>
  auto operator()(Args&&... args)
    ->decltype(this->call(
      make_index_sequence<std::tuple_size<Tuple>::value>{},
      std::forward<Args>(args)...))
  {
    return call(
      make_index_sequence<std::tuple_size<Tuple>::value>{},
      std::forward<Args>(args)...);
  }

  F     f;
  Tuple t;
};

template<typename F,typename... Args>
auto tail_closure(const F& f,Args&&... args)
  ->tail_closure_class<F,std::tuple<Args&&...>>
{
  return {f,std::forward_as_tuple(std::forward<Args>(args)...)};
}

template<typename F,typename Tuple>
struct head_closure_class
{
  template<typename... Args,std::size_t... I>
  auto call(index_sequence<I...>,Args&&... args)
    ->decltype(std::declval<F>()(
      std::get<I>(std::declval<Tuple>())...,std::forward<Args>(args)...))
  {
    return f(std::get<I>(t)...,std::forward<Args>(args)...);
  }

  template<typename... Args>
  auto operator()(Args&&... args)
    ->decltype(this->call(
      make_index_sequence<std::tuple_size<Tuple>::value>{},
      std::forward<Args>(args)...))
  {
    return call(
      make_index_sequence<std::tuple_size<Tuple>::value>{},
      std::forward<Args>(args)...);
  }

  F     f;
  Tuple t;
};

template<typename F,typename... Args>
auto head_closure(const F& f,Args&&... args)
  ->head_closure_class<F,std::tuple<Args&&...>>
{
  return {f,std::forward_as_tuple(std::forward<Args>(args)...)};
}

template<typename ReturnType,typename F>
struct cast_return_class
{
  template<typename... Args>
  ReturnType operator()(Args&&... args)const
  {
    return static_cast<ReturnType>(f(std::forward<Args>(args)...));
  }

  F f;
};

template<typename ReturnType,typename F>
cast_return_class<ReturnType,F> cast_return(const F& f)
{
  return {f};
}

template<typename F>
struct deref_to_class
{
  template<typename... Args>
  auto operator()(Args&&... args)->decltype(std::declval<F>()(*args...))
  {
    return f(*args...);
  }

  F f;
};

template<typename F>
deref_to_class<F> deref_to(const F& f)
{
  return {f};
}

template<typename F>
struct deref_1st_to_class
{
  template<typename Arg,typename... Args>
  auto operator()(Arg&& arg,Args&&... args)
    ->decltype(std::declval<F>()(*arg,std::forward<Args>(args)...))
  {
    return f(*arg,std::forward<Args>(args)...);
  }

  F f;
};

template<typename F>
deref_1st_to_class<F> deref_1st_to(const F& f)
{
  return {f};
}

struct transparent_equal_to
{
  template<typename T,typename U>
  auto operator()(T&& x,U&& y)const
    noexcept(noexcept(std::forward<T>(x)==std::forward<U>(y)))
    ->decltype(std::forward<T>(x)==std::forward<U>(y))
  {
    return std::forward<T>(x)==std::forward<U>(y);
  }
};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
