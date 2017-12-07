#
#   Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
#   Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
#   Copyright (c) 2015-2016 Jack R. Dunaway. All rights reserved.
#
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom
#   the Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#   IN THE SOFTWARE.
#

set (NN_SOURCES
    nn.h
    inproc.h
    ipc.h
    tcp.h
    ws.h
    pair.h
    pubsub.h
    reqrep.h
    pipeline.h
    survey.h
    bus.h

    core/ep.h
    core/ep.c
    core/epbase.c
    core/global.h
    core/global.c
    core/pipe.c
    core/poll.c
    core/sock.h
    core/sock.c
    core/sockbase.c
    core/symbol.c

    aio/ctx.h
    aio/ctx.c
    aio/fsm.h
    aio/fsm.c
    aio/pool.h
    aio/pool.c
    aio/timer.h
    aio/timer.c
    aio/timerset.h
    aio/timerset.c
    aio/usock.h
    aio/usock.c
    aio/worker.h
    aio/worker.c

    utils/alloc.h
    utils/alloc.c
    utils/atomic.h
    utils/atomic.c
    utils/attr.h
    utils/chunk.h
    utils/chunk.c
    utils/chunkref.h
    utils/chunkref.c
    utils/clock.h
    utils/clock.c
    utils/closefd.h
    utils/closefd.c
    utils/cont.h
    utils/efd.h
    utils/efd.c
    utils/err.h
    utils/err.c
    utils/fast.h
    utils/fd.h
    utils/hash.h
    utils/hash.c
    utils/list.h
    utils/list.c
    utils/msg.h
    utils/msg.c
    utils/condvar.h
    utils/condvar.c
    utils/mutex.h
    utils/mutex.c
    utils/once.h
    utils/once.c
    utils/queue.h
    utils/queue.c
    utils/random.h
    utils/random.c
    utils/sem.h
    utils/sem.c
    utils/sleep.h
    utils/sleep.c
    utils/thread.h
    utils/thread.c
    utils/wire.h
    utils/wire.c

    devices/device.h
    devices/device.c

    protocols/utils/dist.h
    protocols/utils/dist.c
    protocols/utils/excl.h
    protocols/utils/excl.c
    protocols/utils/fq.h
    protocols/utils/fq.c
    protocols/utils/lb.h
    protocols/utils/lb.c
    protocols/utils/priolist.h
    protocols/utils/priolist.c

    protocols/bus/bus.h
    protocols/bus/bus.c
    protocols/bus/xbus.h
    protocols/bus/xbus.c

    protocols/pipeline/push.h
    protocols/pipeline/push.c
    protocols/pipeline/pull.h
    protocols/pipeline/pull.c
    protocols/pipeline/xpull.h
    protocols/pipeline/xpull.c
    protocols/pipeline/xpush.h
    protocols/pipeline/xpush.c

    protocols/pair/pair.h
    protocols/pair/pair.c
    protocols/pair/xpair.h
    protocols/pair/xpair.c

    protocols/pubsub/pub.h
    protocols/pubsub/pub.c
    protocols/pubsub/sub.h
    protocols/pubsub/sub.c
    protocols/pubsub/trie.h
    protocols/pubsub/trie.c
    protocols/pubsub/xpub.h
    protocols/pubsub/xpub.c
    protocols/pubsub/xsub.h
    protocols/pubsub/xsub.c

    protocols/reqrep/req.h
    protocols/reqrep/req.c
    protocols/reqrep/rep.h
    protocols/reqrep/rep.c
    protocols/reqrep/task.h
    protocols/reqrep/task.c
    protocols/reqrep/xrep.h
    protocols/reqrep/xrep.c
    protocols/reqrep/xreq.h
    protocols/reqrep/xreq.c

    protocols/survey/respondent.h
    protocols/survey/respondent.c
    protocols/survey/surveyor.h
    protocols/survey/surveyor.c
    protocols/survey/xrespondent.h
    protocols/survey/xrespondent.c
    protocols/survey/xsurveyor.h
    protocols/survey/xsurveyor.c

    transports/utils/backoff.h
    transports/utils/backoff.c
    transports/utils/dns.h
    transports/utils/dns.c
    transports/utils/dns_getaddrinfo.h
    transports/utils/dns_getaddrinfo.inc
    transports/utils/dns_getaddrinfo_a.h
    transports/utils/dns_getaddrinfo_a.inc
    transports/utils/iface.h
    transports/utils/iface.c
    transports/utils/literal.h
    transports/utils/literal.c
    transports/utils/port.h
    transports/utils/port.c
    transports/utils/streamhdr.h
    transports/utils/streamhdr.c
    transports/utils/base64.h
    transports/utils/base64.c

    transports/inproc/binproc.h
    transports/inproc/binproc.c
    transports/inproc/cinproc.h
    transports/inproc/cinproc.c
    transports/inproc/inproc.h
    transports/inproc/inproc.c
    transports/inproc/ins.h
    transports/inproc/ins.c
    transports/inproc/msgqueue.h
    transports/inproc/msgqueue.c
    transports/inproc/sinproc.h
    transports/inproc/sinproc.c

    transports/ipc/aipc.h
    transports/ipc/aipc.c
    transports/ipc/bipc.h
    transports/ipc/bipc.c
    transports/ipc/cipc.h
    transports/ipc/cipc.c
    transports/ipc/ipc.h
    transports/ipc/ipc.c
    transports/ipc/sipc.h
    transports/ipc/sipc.c

    transports/tcp/atcp.h
    transports/tcp/atcp.c
    transports/tcp/btcp.h
    transports/tcp/btcp.c
    transports/tcp/ctcp.h
    transports/tcp/ctcp.c
    transports/tcp/stcp.h
    transports/tcp/stcp.c
    transports/tcp/tcp.h
    transports/tcp/tcp.c

    transports/ws/aws.h
    transports/ws/aws.c
    transports/ws/bws.h
    transports/ws/bws.c
    transports/ws/cws.h
    transports/ws/cws.c
    transports/ws/sws.h
    transports/ws/sws.c
    transports/ws/ws.h
    transports/ws/ws.c
    transports/ws/ws_handshake.h
    transports/ws/ws_handshake.c
    transports/ws/sha1.h
    transports/ws/sha1.c
)

if (WIN32)
    list (APPEND NN_SOURCES
        aio/usock_win.h
        aio/usock_win.inc
        aio/worker_win.h
        aio/worker_win.inc
        utils/thread_win.h
        utils/thread_win.inc
        utils/win.h
    )
elseif (UNIX)
    list (APPEND NN_SOURCES
        aio/usock_posix.h
        aio/usock_posix.inc
        aio/worker_posix.h
        aio/worker_posix.inc
        utils/thread_posix.h
        utils/thread_posix.inc
    )
else ()
    message (FATAL_ERROR "Assertion failed; this path is unreachable.")
endif ()

if (NN_HAVE_EPOLL)
    add_definitions (-DNN_USE_EPOLL)
    list (APPEND NN_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_epoll.h
        aio/poller_epoll.inc
    )
elseif (NN_HAVE_KQUEUE)
    add_definitions (-DNN_USE_KQUEUE)
    list (APPEND NN_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_kqueue.h
        aio/poller_kqueue.inc
    )
elseif (NN_HAVE_POLL)
    add_definitions (-DNN_USE_POLL)
    list (APPEND NN_SOURCES
        aio/poller.h
        aio/poller.c
        aio/poller_poll.h
        aio/poller_poll.inc
    )
elseif (NN_HAVE_WINSOCK)
    # No operation
else ()
    message (SEND_ERROR "ERROR: could not determine socket polling method.")
    message (SEND_ERROR "${ISSUE_REPORT_MSG}" )
endif ()

if (NN_HAVE_EVENTFD)
    add_definitions (-DNN_USE_EVENTFD)
    list (APPEND NN_SOURCES
        utils/efd_eventfd.h
        utils/efd_eventfd.inc
    )
elseif (NN_HAVE_PIPE)
    add_definitions (-DNN_USE_PIPE)
    list (APPEND NN_SOURCES
        utils/efd_pipe.h
        utils/efd_pipe.inc
    )
elseif (NN_HAVE_SOCKETPAIR)
    add_definitions (-DNN_USE_SOCKETPAIR)
    list (APPEND NN_SOURCES
        utils/efd_socketpair.h
        utils/efd_socketpair.inc
    )
elseif (NN_HAVE_WINSOCK)
    add_definitions (-DNN_USE_WINSOCK)
    list (APPEND NN_SOURCES
        utils/efd_win.h
        utils/efd_win.inc
    )
else ()
    message (SEND_ERROR "ERROR: could not determine socket signaling method.")
    message (SEND_ERROR "${ISSUE_REPORT_MSG}" )
endif ()

# Provide same folder structure in IDE as on disk
foreach (f ${NN_SOURCES})
    # Get the path of the file relative to source directory
    if (IS_ABSOLUTE "${f}")
        file (RELATIVE_PATH f ${CMAKE_CURRENT_SOURCE_DIR} ${f})
    endif ()
    set (SRC_GROUP "${f}")
    set (f "${CMAKE_CURRENT_SOURCE_DIR}/${f}")

    # Remove the filename part
    string (REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRC_GROUP ${SRC_GROUP})

    # CMake source_group expects \\, not /
    string (REPLACE / \\ SRC_GROUP ${SRC_GROUP})
    source_group ("${SRC_GROUP}" FILES ${f})
endforeach ()

if (NN_STATIC_LIB)
    add_library (${PROJECT_NAME} STATIC ${NN_SOURCES})
else ()
    add_library (${PROJECT_NAME} SHARED ${NN_SOURCES})
    add_definitions (-DNN_SHARED_LIB)
    set_target_properties (${PROJECT_NAME} PROPERTIES
        VERSION "${NN_PACKAGE_VERSION}"
        SOVERSION "${NN_ABI_VERSION}")
endif ()

# Set library outputs same as top-level project binary outputs
set_target_properties (${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties (${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set_target_properties (${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_link_libraries (${PROJECT_NAME} ${NN_REQUIRED_LIBRARIES})
if(THREADS_HAVE_PTHREAD_ARG)
    add_definitions (-pthread)
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries (${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif()

# pkg-config file
if(NN_REQUIRED_LIBRARIES)
    foreach (lib ${NN_REQUIRED_LIBRARIES})
        set (NN_REQUIRED_LFLAGS "${NN_REQUIRED_LFLAGS} -l${lib}")
    endforeach()
endif()
configure_file (pkgconfig.in ${PROJECT_NAME}.pc @ONLY)
install (
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install (TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
