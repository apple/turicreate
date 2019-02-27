#include "cmUVHandlePtr.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "cm_uv.h"

static void signal_reset_fn(uv_async_t* handle)
{
  auto ptr = static_cast<cm::uv_async_ptr*>(handle->data);
  ptr->reset();
}

// A common pattern is to use an async signal to shutdown the server.
static bool testAsyncShutdown()
{
  uv_loop_t Loop;
  auto err = uv_loop_init(&Loop);
  if (err != 0) {
    std::cerr << "Could not init loop" << std::endl;
    return false;
  }

  {
    cm::uv_async_ptr signal;
    signal.init(Loop, &signal_reset_fn, &signal);

    std::thread([&] {
      std::this_thread::sleep_for(std::chrono::seconds(2));
      signal.send();
    })
      .detach();

    if (uv_run(&Loop, UV_RUN_DEFAULT) != 0) {
      std::cerr << "Unclean exit state in testAsyncDtor" << std::endl;
      return false;
    }

    if (signal.get()) {
      std::cerr << "Loop exited with signal not being cleaned up" << std::endl;
      return false;
    }
  }

  uv_loop_close(&Loop);

  return true;
}

static void signal_fn(uv_async_t*)
{
}

// Async dtor is sort of a pain; since it locks a mutex we must be sure its
// dtor always calls reset otherwise the mutex is deleted then locked.
static bool testAsyncDtor()
{
  uv_loop_t Loop;
  auto err = uv_loop_init(&Loop);
  if (err != 0) {
    std::cerr << "Could not init loop" << std::endl;
    return false;
  }

  {
    cm::uv_async_ptr signal;
    signal.init(Loop, signal_fn);
  }

  if (uv_run(&Loop, UV_RUN_DEFAULT) != 0) {
    std::cerr << "Unclean exit state in testAsyncDtor" << std::endl;
    return false;
  }

  uv_loop_close(&Loop);

  return true;
}

// Async needs a relatively stateful deleter; make sure that is properly
// accounted for and doesn't try to hold on to invalid state when it is
// moved
static bool testAsyncMove()
{
  uv_loop_t Loop;
  auto err = uv_loop_init(&Loop);
  if (err != 0) {
    std::cerr << "Could not init loop" << std::endl;
    return false;
  }

  {
    cm::uv_async_ptr signal;
    {
      cm::uv_async_ptr signalTmp;
      signalTmp.init(Loop, signal_fn);
      signal = std::move(signalTmp);
    }
  }

  if (uv_run(&Loop, UV_RUN_DEFAULT) != 0) {
    std::cerr << "Unclean exit state in testAsyncDtor" << std::endl;
    return false;
  }

  uv_loop_close(&Loop);
  return true;
}

// When a type is castable to another uv type (pipe -> stream) here,
// and the deleter is convertible as well, we should allow moves from
// one type to the other.
static bool testCrossAssignment()
{
  uv_loop_t Loop;
  auto err = uv_loop_init(&Loop);
  if (err != 0) {
    std::cerr << "Could not init loop" << std::endl;
    return false;
  }

  {
    cm::uv_pipe_ptr pipe;
    pipe.init(Loop, 0);

    cm::uv_stream_ptr stream = std::move(pipe);
    if (pipe.get()) {
      std::cerr << "Move should be sure to invalidate the previous ptr"
                << std::endl;
      return false;
    }
    cm::uv_handle_ptr handle = std::move(stream);
    if (stream.get()) {
      std::cerr << "Move should be sure to invalidate the previous ptr"
                << std::endl;
      return false;
    }
  }

  if (uv_run(&Loop, UV_RUN_DEFAULT) != 0) {
    std::cerr << "Unclean exit state in testCrossAssignment" << std::endl;
    return false;
  }

  uv_loop_close(&Loop);
  return true;
}

// This test can't fail at run time; but this makes sure we have all our move
// ctors created correctly.
static bool testAllMoves()
{
  using namespace cm;
  struct allTypes
  {
    uv_stream_ptr _7;
    uv_timer_ptr _8;
    uv_tty_ptr _9;
    uv_process_ptr _11;
    uv_pipe_ptr _12;
    uv_async_ptr _13;
    uv_signal_ptr _14;
    uv_handle_ptr _15;
  };

  allTypes a;
  allTypes b(std::move(a));
  allTypes c = std::move(b);
  return true;
};

int testUVRAII(int, char** const)
{
  if ((testAsyncShutdown() &&
       testAsyncDtor() & testAsyncMove() & testCrossAssignment() &
         testAllMoves()) == 0) {
    return -1;
  }
  return 0;
}
