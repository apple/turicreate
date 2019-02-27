/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once
#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_uv.h"

#if defined(CMAKE_USE_SYSTEM_LIBUV) && !defined(_WIN32) &&                    \
  UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR < 19
#  define CMAKE_UV_SIGNAL_HACK
#  include "cmUVHandlePtr.h"
/*
   libuv does not use SA_RESTART on its signal handler, but C++ streams
   depend on it for reliable i/o operations.  This RAII helper convinces
   libuv to install its handler, and then revises the handler to add the
   SA_RESTART flag.  We use a distinct uv loop that never runs to avoid
   ever really getting a callback.  libuv may fill the hack loop's signal
   pipe and then stop writing, but that won't break any real loops.
 */
class cmUVSignalHackRAII
{
  uv_loop_t HackLoop;
  cm::uv_signal_ptr HackSignal;
  static void HackCB(uv_signal_t*, int) {}

public:
  cmUVSignalHackRAII()
  {
    uv_loop_init(&this->HackLoop);
    this->HackSignal.init(this->HackLoop);
    this->HackSignal.start(HackCB, SIGCHLD);
    struct sigaction hack_sa;
    sigaction(SIGCHLD, nullptr, &hack_sa);
    if (!(hack_sa.sa_flags & SA_RESTART)) {
      hack_sa.sa_flags |= SA_RESTART;
      sigaction(SIGCHLD, &hack_sa, nullptr);
    }
  }
  ~cmUVSignalHackRAII()
  {
    this->HackSignal.stop();
    uv_loop_close(&this->HackLoop);
  }
};
#endif
