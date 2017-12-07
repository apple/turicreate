project(libuv C)

# Disable warnings to avoid changing 3rd party code.
if(CMAKE_C_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
elseif(CMAKE_C_COMPILER_ID STREQUAL "PathScale")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -woffall")
endif()

find_package(Threads)

set(uv_libraries ${CMAKE_THREAD_LIBS_INIT})
set(uv_includes include src)
set(uv_headers
  include/uv.h
  include/uv-errno.h
  include/uv-threadpool.h
  include/uv-version.h
  )
set(uv_sources
  src/fs-poll.c
  src/heap-inl.h
  src/inet.c
  src/queue.h
  src/threadpool.c
  src/uv-common.c
  src/uv-common.h
  src/version.c
  )
if(WIN32)
  list(APPEND uv_libraries
    ws2_32
    psapi
    iphlpapi
    shell32
    userenv
    )
  list(APPEND uv_includes
    src/win
    )
  list(APPEND uv_defines
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0600
    )
  list(APPEND uv_headers
    include/uv-win.h
    include/tree.h
    )
  list(APPEND uv_sources
    src/win/async.c
    src/win/atomicops-inl.h
    src/win/core.c
    src/win/detect-wakeup.c
    src/win/dl.c
    src/win/error.c
    src/win/fs-event.c
    src/win/fs.c
    src/win/getaddrinfo.c
    src/win/getnameinfo.c
    src/win/handle.c
    src/win/handle-inl.h
    src/win/internal.h
    src/win/loop-watcher.c
    src/win/pipe.c
    src/win/poll.c
    src/win/process-stdio.c
    src/win/process.c
    src/win/req.c
    src/win/req-inl.h
    src/win/signal.c
    src/win/snprintf.c
    src/win/stream.c
    src/win/stream-inl.h
    src/win/tcp.c
    src/win/thread.c
    src/win/timer.c
    src/win/tty.c
    src/win/udp.c
    src/win/util.c
    src/win/winapi.c
    src/win/winapi.h
    src/win/winsock.c
    src/win/winsock.h
    )
else()
  list(APPEND uv_includes
    src/unix
    )
  list(APPEND uv_headers
    include/uv-unix.h
    )
  list(APPEND uv_sources
    src/unix/async.c
    src/unix/atomic-ops.h
    src/unix/core.c
    src/unix/dl.c
    src/unix/fs.c
    src/unix/getaddrinfo.c
    src/unix/getnameinfo.c
    src/unix/internal.h
    src/unix/loop-watcher.c
    src/unix/loop.c
    src/unix/pipe.c
    src/unix/poll.c
    src/unix/process.c
    src/unix/signal.c
    src/unix/spinlock.h
    src/unix/stream.c
    src/unix/tcp.c
    src/unix/thread.c
    src/unix/timer.c
    src/unix/tty.c
    src/unix/udp.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "AIX")
  list(APPEND uv_libraries
    perfstat
    )
  list(APPEND uv_headers
    include/uv-aix.h
    )
  list(APPEND uv_defines
    _ALL_SOURCE
    _XOPEN_SOURCE=500
    _LINUX_SOURCE_COMPAT
    _THREAD_SAFE
    )
  list(APPEND uv_sources
    src/unix/aix.c
    )
endif()

if(CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
  list(APPEND uv_libraries
    )
  list(APPEND uv_headers
    include/uv-posix.h
    )
  list(APPEND uv_defines
    )
  list(APPEND uv_sources
    src/unix/cygwin.c
    src/unix/bsd-ifaddrs.c
    src/unix/no-fsevents.c
    src/unix/no-proctitle.c
    src/unix/posix-hrtime.c
    src/unix/posix-poll.c
    src/unix/procfs-exepath.c
    src/unix/sysinfo-loadavg.c
    src/unix/sysinfo-memory.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  list(APPEND uv_headers
    include/uv-darwin.h
    include/pthread-barrier.h
    )
  list(APPEND uv_defines
    _DARWIN_USE_64_BIT_INODE=1
    _DARWIN_UNLIMITED_SELECT=1
    )
  list(APPEND uv_sources
    src/unix/bsd-ifaddrs.c
    src/unix/darwin.c
    src/unix/darwin-proctitle.c
    src/unix/fsevents.c
    src/unix/kqueue.c
    src/unix/proctitle.c
    src/unix/pthread-barrier.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND uv_libraries dl rt)
  list(APPEND uv_headers
    include/uv-linux.h
    )
  list(APPEND uv_defines _GNU_SOURCE)
  list(APPEND uv_sources
    src/unix/linux-core.c
    src/unix/linux-inotify.c
    src/unix/linux-syscalls.c
    src/unix/linux-syscalls.h
    src/unix/procfs-exepath.c
    src/unix/proctitle.c
    src/unix/sysinfo-loadavg.c
    src/unix/sysinfo-memory.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  list(APPEND uv_libraries
    kvm
    )
  list(APPEND uv_headers
    include/uv-bsd.h
    )
  list(APPEND uv_sources
    src/unix/bsd-ifaddrs.c
    src/unix/freebsd.c
    src/unix/kqueue.c
    src/unix/posix-hrtime.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  list(APPEND uv_libraries
    kvm
    )
  list(APPEND uv_headers
    include/uv-bsd.h
    )
  list(APPEND uv_sources
    src/unix/bsd-ifaddrs.c
    src/unix/netbsd.c
    src/unix/kqueue.c
    src/unix/posix-hrtime.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  list(APPEND uv_libraries
    kvm
    )
  list(APPEND uv_headers
    include/uv-bsd.h
    )
  list(APPEND uv_sources
    src/unix/bsd-ifaddrs.c
    src/unix/openbsd.c
    src/unix/kqueue.c
    src/unix/posix-hrtime.c
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  list(APPEND uv_libraries
    kstat
    nsl
    sendfile
    socket
    rt
    )
  list(APPEND uv_headers
    include/uv-sunos.h
    )
  list(APPEND uv_defines
    __EXTENSIONS__
    )
  if(CMAKE_SYSTEM_VERSION STREQUAL "5.10")
    set(CMAKE_C_STANDARD 90)
    if(CMAKE_VERSION VERSION_LESS 3.8.20170504 AND CMAKE_C_COMPILER_ID STREQUAL "SunPro" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.14)
      # The running version of CMake does not know how to add this flag.
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90")
    endif()
    list(APPEND uv_defines
      _XOPEN_SOURCE=500
      )
  else()
    if(NOT CMAKE_C_STANDARD OR CMAKE_C_STANDARD EQUAL 90)
      set(CMAKE_C_STANDARD 11)
    endif()
    if(CMAKE_VERSION VERSION_LESS 3.8.20170505 AND CMAKE_C_COMPILER_ID STREQUAL "SunPro" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 5.14)
      # The running version of CMake does not know how to add this flag.
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -xc99")
    endif()
    list(APPEND uv_defines
      _XOPEN_SOURCE=600
      )
  endif()
  list(APPEND uv_sources
    src/unix/no-proctitle.c
    src/unix/sunos.c
    )
endif()

include_directories(
  ${uv_includes}
  ${KWSYS_HEADER_ROOT}
  )
add_library(cmlibuv STATIC ${uv_sources})
target_link_libraries(cmlibuv ${uv_libraries})
set_property(TARGET cmlibuv PROPERTY COMPILE_DEFINITIONS ${uv_defines})

install(FILES LICENSE DESTINATION ${CMAKE_DOC_DIR}/cmlibuv)
