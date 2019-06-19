/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_PARALLEL_EXECUTE_TASK_IN_NATIVE_THREAD_HPP
#define TURI_PARALLEL_EXECUTE_TASK_IN_NATIVE_THREAD_HPP
#include <functional>

namespace turi {

/**
 * Takes a function and executes it in a native stack space.
 * Used to get by some libjvm oddities when using coroutines.
 *
 * Returns an exception if an exception was thrown while executing the inner task.
 * \ingroup threading
 */
std::exception_ptr execute_task_in_native_thread(const std::function<void(void)>& fn);

namespace native_exec_task_impl {
template <typename T>
struct value_type {
  T ret;

  T get_result() {
    return ret;
  }

  template <typename F, typename A1>
  void run_as_native(F f, A1 a1) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2>
  void run_as_native(F f, A1 a1, A2 a2) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2, a3);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2, a3, a4);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2, a3, a4, a5);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2, a3, a4, a5, a6);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
    auto except = execute_task_in_native_thread([&](void)->void {
      ret = f(a1, a2, a3, a4, a5, a6, a7);
    });
    if (except) std::rethrow_exception(except);
  }
};

template <>
struct value_type<void> {

  void get_result() { }

  template <typename F, typename A1>
  void run_as_native(F f, A1 a1) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2>
  void run_as_native(F f, A1 a1, A2 a2) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2, a3);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2, a3, a4);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2, a3, a4, a5);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2, a3, a4, a5, a6);
    });
    if (except) std::rethrow_exception(except);
  }

  template <typename F, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
  void run_as_native(F f, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7) {
    auto except = execute_task_in_native_thread([&](void)->void {
      f(a1, a2, a3, a4, a5, a6, a7);
    });
    if (except) std::rethrow_exception(except);
  }
};
} // namespace native_exec_task_impl
/**
 * Takes a function call and runs it in a native stack space.
 * Used to get by some libjvm oddities when using coroutines.
 *
 * Returns an exception if an exception was thrown while executing the inner task.
 */
template <typename F, typename ... Args>
typename std::result_of<F(Args...)>::type run_as_native(F f, Args... args) {
  native_exec_task_impl::value_type<typename std::result_of<F(Args...)>::type> result;

  result.template run_as_native<F, Args...>(f, args...);

  return result.get_result();
}

} // turicreate
#endif
