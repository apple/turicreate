/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_BASIC_TYPES_H_
#define TURI_BASIC_TYPES_H_

#include <logger/assertions.hpp>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

using std::shared_ptr;

using boost::optional;
using boost::variant;

template<typename T> optional<T> NONE() {
  return optional<T>();
}

template<typename T> optional<T> SOME(const T& t) {
  return optional<T>(t);
}

template<typename T> T extract(const optional<T>& x, T default_value) {
  if (!!x) {
    return *x;
  } else {
    return default_value;
  }
}

template<typename T> std::ostream& operator<<(std::ostream& os, const optional<T>& x) {
  if (!!x) {
    os << "SOME(" << (*x) << ")";
  } else {
    os << "NONE";
  }
  return os;
}

template<typename Enum> void vget_fail_internal(Enum x) {
  std::ostringstream os;
  os << "Failed extraction of variant; found: " << static_cast<Enum>(x)
     << std::endl;
  log_and_throw(os.str());
}

template<typename Enum, typename T, typename... Ts>
T& vget(boost::variant<Ts...>& v) {
  T* ret = boost::relaxed_get<T, Ts...>(&v);
  if (!!ret) {
    return *ret;
  }
  vget_fail_internal(v.which());
  AU();
}

template<typename Enum, typename T, typename... Ts>
const T& vget(const boost::variant<Ts...>& v) {
  const T* ret = boost::relaxed_get<T, Ts...>(&v);
  if (!!ret) {
    return *ret;
  }
  vget_fail_internal(v.which());
  AU();
}

template<typename Target, typename Source, int UnusedParam> struct truncate_check_impl {
  Target operator()(Source x);
};

template<typename Target, typename Source> Target truncate_check(Source x) {
  return truncate_check_impl<Target, Source, 0>()(x);
}

template<typename Target, typename Source> struct truncate_check_impl<Target, Source, 0> {
  Target operator()(Source x) {
    static_assert(!std::is_same<Source, Source>::value, "Unknown instantiation of truncate_check");
    return 0;
  }
};

template<> struct truncate_check_impl<int64_t, size_t, 0> {
  int64_t operator()(size_t x) {
    ASSERT_LT(x, (1ULL << 63));
    return static_cast<int64_t>(x);
  }
};

// Convenience macro for declaring types allocated via shared_ptr.
#define DECL_STRUCT(X) \
  struct X; \
  using X ## _p = shared_ptr<X>; \
  bool operator==(shared_ptr<X>, shared_ptr<X>); \

// For integer values, returns ceil(n / m);
template<typename T> T ceil_divide(T n, T m) {
  return (n + (m-1)) / m;
}

template<typename T> int64_t len(const std::vector<T>& x) {
  return static_cast<int64_t>(x.size());
}

inline int64_t len(const std::string& x) {
  return static_cast<int64_t>(x.length());
}

template<typename T> int64_t len(const std::unordered_set<T>& x) {
  return static_cast<int64_t>(x.size());
}

template<typename T> int64_t len(const std::set<T>& x) {
  return static_cast<int64_t>(x.size());
}

template<typename T> bool all_distinct(const std::vector<T>& v) {
  std::unordered_set<T> vs;
  for (const auto& vi : v) {
    vs.insert(vi);
  }
  return (len(vs) == len(v));
}

template<typename T> optional<T> vector_max(const std::vector<T>& v) {
  if (v.size() == 0) {
    return NONE<T>();
  }

  T ret = v[0];
  for (int64_t i = 1; i < len(v); i++) {
    ret = std::max<T>(ret, v[i]);
  }

  return SOME(ret);
}

template<typename T> T product(const std::vector<T>& x) {
  T ret = 1;
  for (int64_t i = 0; i < len(x); i++) {
    ret *= x[i];
  }
  return ret;
}

std::vector<int64_t> contiguous_strides(const std::vector<int64_t>& shape);

template<typename T> T& at(std::vector<T>& v, int64_t i) {
  ASSERT_LT(i, len(v));
  return v[i];
}

template<typename T> const T& at(const std::vector<T>& v, int64_t i) {
  ASSERT_LT(i, len(v));
  return v[i];
}

template<typename T, typename U> std::ostream& operator<<(
  std::ostream& os, const std::pair<T, U>& x) {

  os << "(" << x.first << ", " << x.second << ")";
  return os;
}

template<typename T, typename U> struct std_pair_hash {
  int64_t operator()(const std::pair<T, U>& d) const {
    int64_t ret0 = static_cast<int64_t>(std::hash<T>()(d.first)) % ((1LL << 31) - 1);
    int64_t ret1 = static_cast<int64_t>(std::hash<U>()(d.second)) % ((1LL << 31) - 1);
    return int64_t((ret0 << 32) + ret1);
  }
};

#pragma push_macro("check")
#undef check
int64_t check(const char* desc, int64_t ret);
#pragma pop_macro("check")

void* check_ptr(const char* desc, void* ptr);

#endif
