/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#define cmUVHandlePtr_cxx
#include "cmUVHandlePtr.h"

#include <assert.h>
#include <mutex>
#include <stdlib.h>

#include "cm_uv.h"

namespace cm {

static void close_delete(uv_handle_t* h)
{
  free(h);
}

template <typename T>
static void default_delete(T* type_handle)
{
  auto handle = reinterpret_cast<uv_handle_t*>(type_handle);
  if (handle) {
    assert(!uv_is_closing(handle));
    if (!uv_is_closing(handle)) {
      uv_close(handle, &close_delete);
    }
  }
}

/**
 * Encapsulates delete logic for a given handle type T
 */
template <typename T>
struct uv_handle_deleter
{
  void operator()(T* type_handle) const { default_delete(type_handle); }
};

template <typename T>
void uv_handle_ptr_base_<T>::allocate(void* data)
{
  reset();

  /*
    We use calloc since we know all these types are c structs
    and we just want to 0 init them. New would do the same thing;
    but casting from uv_handle_t to certain other types -- namely
    uv_timer_t -- triggers a cast_align warning on certain systems.
  */
  handle.reset(static_cast<T*>(calloc(1, sizeof(T))), uv_handle_deleter<T>());
  handle->data = data;
}

template <typename T>
void uv_handle_ptr_base_<T>::reset()
{
  handle.reset();
}

template <typename T>
uv_handle_ptr_base_<T>::operator uv_handle_t*()
{
  return reinterpret_cast<uv_handle_t*>(handle.get());
}

template <typename T>
T* uv_handle_ptr_base_<T>::operator->() const noexcept
{
  return handle.get();
}

template <typename T>
T* uv_handle_ptr_base_<T>::get() const
{
  return handle.get();
}

template <typename T>
uv_handle_ptr_<T>::operator T*() const
{
  return this->handle.get();
}

#ifdef CMAKE_BUILD_WITH_CMAKE
template <>
struct uv_handle_deleter<uv_async_t>
{
  /***
   * Wile uv_async_send is itself thread-safe, there are
   * no strong guarantees that close hasn't already been
   * called on the handle; and that it might be deleted
   * as the send call goes through. This mutex guards
   * against that.
   *
   * The shared_ptr here is to allow for copy construction
   * which is mandated by the standard for Deleter on
   * shared_ptrs.
   */
  std::shared_ptr<std::mutex> handleMutex;

  uv_handle_deleter()
    : handleMutex(std::make_shared<std::mutex>())
  {
  }

  void operator()(uv_async_t* handle)
  {
    std::lock_guard<std::mutex> lock(*handleMutex);
    default_delete(handle);
  }
};

void uv_async_ptr::send()
{
  auto deleter = std::get_deleter<uv_handle_deleter<uv_async_t>>(this->handle);
  assert(deleter);

  std::lock_guard<std::mutex> lock(*deleter->handleMutex);
  if (this->handle) {
    uv_async_send(*this);
  }
}

int uv_async_ptr::init(uv_loop_t& loop, uv_async_cb async_cb, void* data)
{
  allocate(data);
  return uv_async_init(&loop, handle.get(), async_cb);
}
#endif

template <>
struct uv_handle_deleter<uv_signal_t>
{
  void operator()(uv_signal_t* handle) const
  {
    if (handle) {
      uv_signal_stop(handle);
      default_delete(handle);
    }
  }
};

int uv_signal_ptr::init(uv_loop_t& loop, void* data)
{
  allocate(data);
  return uv_signal_init(&loop, handle.get());
}

int uv_signal_ptr::start(uv_signal_cb cb, int signum)
{
  assert(handle);
  return uv_signal_start(*this, cb, signum);
}

void uv_signal_ptr::stop()
{
  if (handle) {
    uv_signal_stop(*this);
  }
}

int uv_pipe_ptr::init(uv_loop_t& loop, int ipc, void* data)
{
  allocate(data);
  return uv_pipe_init(&loop, *this, ipc);
}

uv_pipe_ptr::operator uv_stream_t*() const
{
  return reinterpret_cast<uv_stream_t*>(handle.get());
}

#ifdef CMAKE_BUILD_WITH_CMAKE
int uv_process_ptr::spawn(uv_loop_t& loop, uv_process_options_t const& options,
                          void* data)
{
  allocate(data);
  return uv_spawn(&loop, *this, &options);
}

int uv_timer_ptr::init(uv_loop_t& loop, void* data)
{
  allocate(data);
  return uv_timer_init(&loop, *this);
}

int uv_timer_ptr::start(uv_timer_cb cb, uint64_t timeout, uint64_t repeat)
{
  assert(handle);
  return uv_timer_start(*this, cb, timeout, repeat);
}

uv_tty_ptr::operator uv_stream_t*() const
{
  return reinterpret_cast<uv_stream_t*>(handle.get());
}

int uv_tty_ptr::init(uv_loop_t& loop, int fd, int readable, void* data)
{
  allocate(data);
  return uv_tty_init(&loop, *this, fd, readable);
}
#endif

template class uv_handle_ptr_base_<uv_handle_t>;

#define UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(NAME)                              \
  template class uv_handle_ptr_base_<uv_##NAME##_t>;                          \
  template class uv_handle_ptr_<uv_##NAME##_t>;

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(signal)

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(pipe)

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(stream)

#ifdef CMAKE_BUILD_WITH_CMAKE
UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(async)

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(process)

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(timer)

UV_HANDLE_PTR_INSTANTIATE_EXPLICIT(tty)
#endif
}
