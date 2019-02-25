/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once
#include "cmConfigure.h" // IWYU pragma: keep

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "cm_uv.h"

#define CM_PERFECT_FWD_CTOR(Class, FwdTo)                                     \
  template <typename... Args>                                                 \
  Class(Args&&... args)                                                       \
    : FwdTo(std::forward<Args>(args)...)                                      \
  {                                                                           \
  }

namespace cm {

/***
 * RAII class to simplify and insure the safe usage of uv_*_t types. This
 * includes making sure resources are properly freed and contains casting
 * operators which allow for passing into relevant uv_* functions.
 *
 *@tparam T actual uv_*_t type represented.
 */
template <typename T>
class uv_handle_ptr_base_
{
protected:
  template <typename _T>
  friend class uv_handle_ptr_base_;

  /**
   * This must be a pointer type since the handle can outlive this class.
   * When uv_close is eventually called on the handle, the memory the
   * handle inhabits must be valid until the close callback is called
   * which can be later on in the loop.
   */
  std::shared_ptr<T> handle;

  /**
   * Allocate memory for the type and optionally set it's 'data' pointer.
   * Protected since this should only be called for an appropriate 'init'
   * call.
   *
   * @param data data pointer to set
   */
  void allocate(void* data = nullptr);

public:
  CM_DISABLE_COPY(uv_handle_ptr_base_)
  uv_handle_ptr_base_(uv_handle_ptr_base_&&) noexcept;
  uv_handle_ptr_base_& operator=(uv_handle_ptr_base_&&) noexcept;

  /**
   * This move constructor allows us to move out of a more specialized
   * uv type into a less specialized one. The only constraint is that
   * the right hand side is castable to T.
   *
   * This allows you to return uv_handle_ptr or uv_stream_ptr from a function
   * that initializes something like uv_pipe_ptr or uv_tcp_ptr and interact
   * and clean up after it without caring about the exact type.
   */
  template <typename S,
            typename = typename std::enable_if<
              std::is_rvalue_reference<S&&>::value>::type>
  uv_handle_ptr_base_(S&& rhs)
  {
    // This will force a compiler error if rhs doesn't have a casting
    // operator to get T*
    this->handle = std::shared_ptr<T>(rhs.handle, rhs);
    rhs.handle.reset();
  }

  // Dtor and ctor need to be inline defined like this for default ctors and
  // dtors to work.
  uv_handle_ptr_base_() {}
  uv_handle_ptr_base_(std::nullptr_t) {}
  ~uv_handle_ptr_base_() { reset(); }

  /**
   * Properly close the handle if needed and sets the inner handle to nullptr
   */
  void reset();

  /**
   * Allow less verbose calling of uv_handle_* functions
   * @return reinterpreted handle
   */
  operator uv_handle_t*();

  T* get() const;
  T* operator->() const noexcept;
};

template <typename T>
inline uv_handle_ptr_base_<T>::uv_handle_ptr_base_(
  uv_handle_ptr_base_<T>&&) noexcept = default;
template <typename T>
inline uv_handle_ptr_base_<T>& uv_handle_ptr_base_<T>::operator=(
  uv_handle_ptr_base_<T>&&) noexcept = default;

/**
 * While uv_handle_ptr_base_ only exposes uv_handle_t*, this exposes uv_T_t*
 * too. It is broken out like this so we can reuse most of the code for the
 * uv_handle_ptr class.
 */
template <typename T>
class uv_handle_ptr_ : public uv_handle_ptr_base_<T>
{
  template <typename _T>
  friend class uv_handle_ptr_;

public:
  CM_PERFECT_FWD_CTOR(uv_handle_ptr_, uv_handle_ptr_base_<T>);

  /***
   * Allow less verbose calling of uv_<T> functions
   * @return reinterpreted handle
   */
  operator T*() const;
};

/***
 * This specialization is required to avoid duplicate 'operator uv_handle_t*()'
 * declarations
 */
template <>
class uv_handle_ptr_<uv_handle_t> : public uv_handle_ptr_base_<uv_handle_t>
{
public:
  CM_PERFECT_FWD_CTOR(uv_handle_ptr_, uv_handle_ptr_base_<uv_handle_t>);
};

class uv_async_ptr : public uv_handle_ptr_<uv_async_t>
{
public:
  CM_PERFECT_FWD_CTOR(uv_async_ptr, uv_handle_ptr_<uv_async_t>);

  int init(uv_loop_t& loop, uv_async_cb async_cb, void* data = nullptr);

  void send();
};

struct uv_signal_ptr : public uv_handle_ptr_<uv_signal_t>
{
  CM_PERFECT_FWD_CTOR(uv_signal_ptr, uv_handle_ptr_<uv_signal_t>);

  int init(uv_loop_t& loop, void* data = nullptr);

  int start(uv_signal_cb cb, int signum);

  void stop();
};

struct uv_pipe_ptr : public uv_handle_ptr_<uv_pipe_t>
{
  CM_PERFECT_FWD_CTOR(uv_pipe_ptr, uv_handle_ptr_<uv_pipe_t>);

  operator uv_stream_t*() const;

  int init(uv_loop_t& loop, int ipc, void* data = nullptr);
};

struct uv_process_ptr : public uv_handle_ptr_<uv_process_t>
{
  CM_PERFECT_FWD_CTOR(uv_process_ptr, uv_handle_ptr_<uv_process_t>);

  int spawn(uv_loop_t& loop, uv_process_options_t const& options,
            void* data = nullptr);
};

struct uv_timer_ptr : public uv_handle_ptr_<uv_timer_t>
{
  CM_PERFECT_FWD_CTOR(uv_timer_ptr, uv_handle_ptr_<uv_timer_t>);

  int init(uv_loop_t& loop, void* data = nullptr);

  int start(uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
};

struct uv_tty_ptr : public uv_handle_ptr_<uv_tty_t>
{
  CM_PERFECT_FWD_CTOR(uv_tty_ptr, uv_handle_ptr_<uv_tty_t>);

  operator uv_stream_t*() const;

  int init(uv_loop_t& loop, int fd, int readable, void* data = nullptr);
};

typedef uv_handle_ptr_<uv_stream_t> uv_stream_ptr;
typedef uv_handle_ptr_<uv_handle_t> uv_handle_ptr;

#ifndef cmUVHandlePtr_cxx

extern template class uv_handle_ptr_base_<uv_handle_t>;

#  define UV_HANDLE_PTR_INSTANTIATE_EXTERN(NAME)                              \
    extern template class uv_handle_ptr_base_<uv_##NAME##_t>;                 \
    extern template class uv_handle_ptr_<uv_##NAME##_t>;

UV_HANDLE_PTR_INSTANTIATE_EXTERN(async)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(signal)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(pipe)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(process)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(stream)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(timer)

UV_HANDLE_PTR_INSTANTIATE_EXTERN(tty)

#  undef UV_HANDLE_PTR_INSTANTIATE_EXTERN

#endif
}
