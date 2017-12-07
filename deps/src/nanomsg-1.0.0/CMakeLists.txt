#
#   Copyright (c) 2012 Martin Sustrik  All rights reserved.
#   Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
#   Copyright (c) 2015-2016 Jack R. Dunaway. All rights reserved.
#   Copyright 2016 Garrett D'Amore <garrett@damore.org>
#   Copyright 2016 Franklin "Snaipe" Mathieu <franklinmathieu@gmail.com>
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

cmake_minimum_required (VERSION 2.8.7)

project (nanomsg C)
include (CheckFunctionExists)
include (CheckSymbolExists)
include (CheckStructHasMember)
include (CheckLibraryExists)
include (CheckCSourceCompiles)
include (GNUInstallDirs)

if (POLICY CMP0042)
    # Newer cmake on MacOS should use @rpath
    cmake_policy (SET CMP0042 NEW)
endif ()

set (NN_DESCRIPTION "High-Performance Scalability Protocols")
set (ISSUE_REPORT_MSG "Please consider opening an issue at https://github.com/nanomsg/nanomsg")

# Determine library versions.

file (READ src/nn.h NN_HDR_STR)
string (REGEX REPLACE ".*#define +NN_VERSION_CURRENT +([0-9]+).*" "\\1" NN_VERSION_CURRENT "${NN_HDR_STR}")
string (REGEX REPLACE ".*#define +NN_VERSION_REVISION +([0-9]+).*" "\\1" NN_VERSION_REVISION "${NN_HDR_STR}")
string (REGEX REPLACE ".*#define +NN_VERSION_AGE +([0-9]+).*" "\\1" NN_VERSION_AGE "${NN_HDR_STR}")

if ((NN_VERSION_CURRENT STREQUAL "") OR (NN_VERSION_REVISION STREQUAL "") OR (NN_VERSION_AGE STREQUAL ""))
    message (FATAL_ERROR "Could not read ABI version from nn.h")
else ()
    set (NN_ABI_VERSION "${NN_VERSION_CURRENT}.${NN_VERSION_REVISION}.${NN_VERSION_AGE}")
    message (STATUS "Detected nanomsg ABI v${NN_ABI_VERSION}")
endif ()

# Determine package version.
find_package (Git QUIET)
if (DEFINED ENV{TRAVIS_TAG})
    set (NN_PACKAGE_VERSION "$ENV{TRAVIS_TAG}")
elseif (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Working off a git repo, using git versioning

    # Get version from last tag
    execute_process (
        COMMAND             "${GIT_EXECUTABLE}" describe --always# | sed -e "s:v::"
        WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE     NN_PACKAGE_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    # If the sources have been changed locally, add -dirty to the version.
    execute_process (
        COMMAND             "${GIT_EXECUTABLE}" diff --quiet
        WORKING_DIRECTORY   "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE     res)
    if (res EQUAL 1)
        set (NN_PACKAGE_VERSION "${NN_PACKAGE_VERSION}-dirty")
    endif()

elseif (EXISTS ${PROJECT_SOURCE_DIR}/.version)
    #  If git is not available (e.g. when building from source package)
    #  we can extract the package version from .version file.
    file (STRINGS .version NN_PACKAGE_VERSION)
else ()
    set (NN_PACKAGE_VERSION "Unknown")
endif()

# User-defined options.

option (NN_STATIC_LIB "Build static library instead of shared library." OFF)
option (NN_ENABLE_DOC "Enable building documentation." ON)
option (NN_ENABLE_GETADDRINFO_A "Enable/disable use of getaddrinfo_a in place of getaddrinfo." ON)
option (NN_TESTS "Build and run nanomsg tests" ON)
option (NN_TOOLS "Build nanomsg tools" ON)
option (NN_ENABLE_NANOCAT "Enable building nanocat utility." ${NN_TOOLS})

#  Platform checks.

find_package (Threads REQUIRED)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions (-DNN_HAVE_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions (-DNN_HAVE_OSX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set (NN_HAVE_WINSOCK 1)
    add_definitions (-DNN_HAVE_WINDOWS)
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)

    # Target Windows Vista and later
    add_definitions (-D_WIN32_WINNT=0x0600)
    list (APPEND CMAKE_REQUIRED_DEFINITIONS -D_WIN32_WINNT=0x0600)
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    add_definitions (-DNN_HAVE_FREEBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    add_definitions (-DNN_HAVE_NETBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    add_definitions (-DNN_HAVE_OPENBSD)
elseif (CMAKE_SYSTEM_NAME MATCHES "Solaris|SunOS")
    add_definitions (-DNN_HAVE_SOLARIS)
elseif (CMAKE_SYSTEM_NAME MATCHES "HP-UX")
    add_definitions (-DNN_HAVE_HPUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "QNX")
    add_definitions (-DNN_HAVE_QNX)
else ()
    message (AUTHOR_WARNING "WARNING: This platform may or may not be supported: ${CMAKE_SYSTEM_NAME}")
    message (AUTHOR_WARNING "${ISSUE_REPORT_MSG}")
endif ()

if (NN_STATIC_LIB)
    add_definitions (-DNN_STATIC_LIB)
endif ()

macro (nn_check_func SYM DEF)
    check_function_exists (${SYM} ${DEF})
    if (${DEF})
        add_definitions (-D${DEF}=1)
    endif ()
endmacro (nn_check_func)

macro (nn_check_sym SYM HDR DEF)
    check_symbol_exists (${SYM} ${HDR} ${DEF})
    if (${DEF})
        add_definitions (-D${DEF}=1)
    endif ()
endmacro (nn_check_sym)

macro (nn_check_lib LIB SYM DEF)
    check_library_exists (${LIB} ${SYM} "" ${DEF})
    if (${DEF})
        add_definitions (-D${DEF}=1)
        set(NN_REQUIRED_LIBRARIES ${NN_REQUIRED_LIBRARIES} ${LIB})
    endif ()
endmacro (nn_check_lib)

macro (nn_check_struct_member STR MEM HDR DEF)
    check_struct_has_member ("struct ${STR}" ${MEM} ${HDR} ${DEF})
    if (${DEF})
        add_definitions (-D${DEF}=1)
    endif ()
endmacro (nn_check_struct_member)

if (WIN32)
    # Windows is a special snowflake.
    set(NN_REQUIRED_LIBRARIES ${NN_REQUIRED_LIBRARIES} ws2_32)
    set(NN_REQUIRED_LIBRARIES ${NN_REQUIRED_LIBRARIES} mswsock)
    set(NN_REQUIRED_LIBRARIES ${NN_REQUIRED_LIBRARIES} advapi32)
    nn_check_sym (InitializeConditionVariable windows.h NN_HAVE_CONDVAR)
    if (NOT NN_HAVE_CONDVAR)
        message (FATAL_ERROR
	    "Modern Windows API support is missing. "
	    "Versions of Windows prior to Vista are not supported.  "
	    "Further, the 32-bit MinGW environment is not supported. "
	    "Ensure you have at least Windows Vista or newer, and are "
	    "using either Visual Studio 2010 or newer or MinGW-W64.")
    endif()
else ()
    # Unconditionally declare the following feature test macros.  These are
    # needed for some platforms (glibc and SunOS/illumos) and should be harmless
    # on the others.
    add_definitions (-D_GNU_SOURCE)
    add_definitions (-D_REENTRANT)
    add_definitions (-D_THREAD_SAFE)
    add_definitions (-D_POSIX_PTHREAD_SEMANTICS)

    nn_check_func (gethrtime NN_HAVE_GETHRTIME)
    nn_check_func (socketpair NN_HAVE_SOCKETPAIR)
    nn_check_func (eventfd NN_HAVE_EVENTFD)
    nn_check_func (pipe NN_HAVE_PIPE)
    nn_check_func (pipe2 NN_HAVE_PIPE2)
    nn_check_func (accept4 NN_HAVE_ACCEPT4)
    nn_check_func (epoll_create NN_HAVE_EPOLL)
    nn_check_func (kqueue NN_HAVE_KQUEUE)
    nn_check_func (poll NN_HAVE_POLL)

    nn_check_lib (anl getaddrinfo_a NN_HAVE_GETADDRINFO_A)
    nn_check_lib (rt clock_gettime  NN_HAVE_CLOCK_GETTIME)
    nn_check_lib (rt sem_wait NN_HAVE_SEMAPHORE_RT)
    nn_check_lib (pthread sem_wait  NN_HAVE_SEMAPHORE_PTHREAD)
    nn_check_lib (nsl gethostbyname NN_HAVE_LIBNSL)
    nn_check_lib (socket socket NN_HAVE_LIBSOCKET)

    nn_check_sym (CLOCK_MONOTONIC time.h NN_HAVE_CLOCK_MONOTONIC)
    nn_check_sym (atomic_cas_32 atomic.h NN_HAVE_ATOMIC_SOLARIS)
    nn_check_sym (AF_UNIX sys/socket.h NN_HAVE_UNIX_SOCKETS)
    nn_check_sym (backtrace_symbols_fd execinfo.h NN_HAVE_BACKTRACE)
    nn_check_struct_member(msghdr msg_control sys/socket.h NN_HAVE_MSG_CONTROL)
    if (NN_HAVE_SEMAPHORE_RT OR NN_HAVE_SEMAPHORE_PTHREAD)
        add_definitions (-DNN_HAVE_SEMAPHORE)
    endif ()
endif ()


if (NOT NN_ENABLE_GETADDRINFO_A)
    add_definitions (-DNN_DISABLE_GETADDRINFO_A)
endif ()

check_c_source_compiles ("
    #include <stdint.h>
    int main()
    {
        volatile uint32_t n = 0;
        __sync_fetch_and_add (&n, 1);
        __sync_fetch_and_sub (&n, 1);
        return 0;
    }
" NN_HAVE_GCC_ATOMIC_BUILTINS)
if (NN_HAVE_GCC_ATOMIC_BUILTINS)
    add_definitions (-DNN_HAVE_GCC_ATOMIC_BUILTINS)
endif ()

add_subdirectory (src)

#  Build the tools

if (NN_ENABLE_NANOCAT)
    add_executable (nanocat tools/nanocat.c tools/options.c)
    target_link_libraries (nanocat ${PROJECT_NAME})
endif ()

if (NN_ENABLE_DOC)
    find_program (ASCIIDOCTOR_EXE asciidoctor)
    if (NOT ASCIIDOCTOR_EXE)
        message (WARNING "Could not find asciidoctor: skipping docs")
        set (NN_ENABLE_DOC OFF)
    else ()
        message (STATUS "Using asciidoctor at ${ASCIIDOCTOR_EXE}")
    endif ()
endif ()

# Build the documenation
if (NN_ENABLE_DOC)

    set (NN_DOCDIR ${CMAKE_CURRENT_SOURCE_DIR}/doc)
    set (NN_STYLESHEET ${NN_DOCDIR}/stylesheet.css)
    set (NN_TITLE ${PROJECT_NAME} ${NN_PACKAGE_VERSION})
    set (NN_A2M ${ASCIIDOCTOR_EXE} -b manpage -amanmanual='${NN_TITLE}')
    set (NN_A2H ${ASCIIDOCTOR_EXE} -d manpage -b html5 -a stylesheeet=${NN_STYLESHEET} -aversion-label=${PROJECT_NAME} -arevnumber=${NN_PACKAGE_VERSION})

    macro (add_libnanomsg_man NAME SECT)
        add_custom_command (
            OUTPUT ${NAME}.${SECT}
            COMMAND ${NN_A2M} -o ${NAME}.${SECT} ${NN_DOCDIR}/${NAME}.txt
            MAIN_DEPENDENCY ${NN_DOCDIR}/${NAME}.txt
        )

        add_custom_command (
            OUTPUT ${NAME}.${SECT}.html
            COMMAND ${NN_A2H} -o ${NAME}.${SECT}.html ${NN_DOCDIR}/${NAME}.txt
            DEPENDS ${NN_STYLESHEET}
            MAIN_DEPENDENCY ${NN_DOCDIR}/${NAME}.txt
        )

        set(NN_MANS ${NN_MANS} ${NAME}.${SECT})
        set(NN_HTMLS ${NN_HTMLS} ${NAME}.${SECT}.html)

        install (
            FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${SECT}.html
            DESTINATION ${CMAKE_INSTALL_DOCDIR}
        )
        install (
            FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.${SECT}
            DESTINATION ${CMAKE_INSTALL_MANDIR}/man${SECT}
        )

    endmacro (add_libnanomsg_man)

    if (NN_ENABLE_NANOCAT)
        add_libnanomsg_man (nanocat 1)
    endif ()

    add_libnanomsg_man (nn_errno 3)
    add_libnanomsg_man (nn_strerror 3)
    add_libnanomsg_man (nn_symbol 3)
    add_libnanomsg_man (nn_symbol_info 3)
    add_libnanomsg_man (nn_allocmsg 3)
    add_libnanomsg_man (nn_reallocmsg 3)
    add_libnanomsg_man (nn_freemsg 3)
    add_libnanomsg_man (nn_socket 3)
    add_libnanomsg_man (nn_close 3)
    add_libnanomsg_man (nn_get_statistic 3)
    add_libnanomsg_man (nn_getsockopt 3)
    add_libnanomsg_man (nn_setsockopt 3)
    add_libnanomsg_man (nn_bind 3)
    add_libnanomsg_man (nn_connect 3)
    add_libnanomsg_man (nn_shutdown 3)
    add_libnanomsg_man (nn_send 3)
    add_libnanomsg_man (nn_recv 3)
    add_libnanomsg_man (nn_sendmsg 3)
    add_libnanomsg_man (nn_recvmsg 3)
    add_libnanomsg_man (nn_device 3)
    add_libnanomsg_man (nn_cmsg 3)
    add_libnanomsg_man (nn_poll 3)

    add_libnanomsg_man (nanomsg 7)
    add_libnanomsg_man (nn_pair 7)
    add_libnanomsg_man (nn_reqrep 7)
    add_libnanomsg_man (nn_pubsub 7)
    add_libnanomsg_man (nn_survey 7)
    add_libnanomsg_man (nn_pipeline 7)
    add_libnanomsg_man (nn_bus 7)
    add_libnanomsg_man (nn_inproc 7)
    add_libnanomsg_man (nn_ipc 7)
    add_libnanomsg_man (nn_tcp 7)
    add_libnanomsg_man (nn_ws 7)
    add_libnanomsg_man (nn_env 7)

    add_custom_target (man ALL DEPENDS ${NN_MANS})
    add_custom_target (html ALL DEPENDS ${NN_HTMLS})

endif ()

#  Build unit tests.

if (NN_TESTS)

    enable_testing ()
    set (all_tests "")

    set (TEST_PORT 5500)
    macro (add_libnanomsg_test NAME TIMEOUT)
        list (APPEND all_tests ${NAME})
        add_executable (${NAME} tests/${NAME}.c)
        target_link_libraries (${NAME} ${PROJECT_NAME})
        add_test (NAME ${NAME} COMMAND ${NAME} ${TEST_PORT})
        set_tests_properties (${NAME} PROPERTIES TIMEOUT ${TIMEOUT})
        math (EXPR TEST_PORT "${TEST_PORT}+10")
    endmacro (add_libnanomsg_test)

    #  Transport tests.
    add_libnanomsg_test (inproc 5)
    add_libnanomsg_test (inproc_shutdown 5)
    add_libnanomsg_test (ipc 5)
    add_libnanomsg_test (ipc_shutdown 30)
    add_libnanomsg_test (ipc_stress 5)
    add_libnanomsg_test (tcp 5)
    add_libnanomsg_test (tcp_shutdown 120)
    add_libnanomsg_test (ws 5)

    #  Protocol tests.
    add_libnanomsg_test (pair 5)
    add_libnanomsg_test (pubsub 5)
    add_libnanomsg_test (reqrep 5)
    add_libnanomsg_test (pipeline 5)
    add_libnanomsg_test (survey 5)
    add_libnanomsg_test (bus 5)

    #  Feature tests.
    add_libnanomsg_test (async_shutdown 30)
    add_libnanomsg_test (block 5)
    add_libnanomsg_test (term 5)
    add_libnanomsg_test (timeo 5)
    add_libnanomsg_test (iovec 5)
    add_libnanomsg_test (msg 5)
    add_libnanomsg_test (prio 5)
    add_libnanomsg_test (poll 5)
    add_libnanomsg_test (device 5)
    add_libnanomsg_test (device4 5)
    add_libnanomsg_test (device5 5)
    add_libnanomsg_test (device6 5)
    add_libnanomsg_test (device7 30)
    add_libnanomsg_test (emfile 5)
    add_libnanomsg_test (domain 5)
    add_libnanomsg_test (trie 5)
    add_libnanomsg_test (list 5)
    add_libnanomsg_test (hash 5)
    add_libnanomsg_test (stats 5)
    add_libnanomsg_test (symbol 5)
    add_libnanomsg_test (separation 5)
    add_libnanomsg_test (zerocopy 5)
    add_libnanomsg_test (shutdown 5)
    add_libnanomsg_test (cmsg 5)
    add_libnanomsg_test (bug328 5)
    add_libnanomsg_test (ws_async_shutdown 5)
    add_libnanomsg_test (reqttl 10)
    add_libnanomsg_test (surveyttl 10)

    # Platform-specific tests
    if (WIN32)
        add_libnanomsg_test (win_sec_attr 5)
    endif()

    #  Build the performance tests.

    macro (add_libnanomsg_perf NAME)
        add_executable (${NAME} perf/${NAME}.c)
        target_link_libraries (${NAME} ${PROJECT_NAME})
    endmacro (add_libnanomsg_perf)

    add_libnanomsg_perf (inproc_lat)
    add_libnanomsg_perf (inproc_thr)
    add_libnanomsg_perf (local_lat)
    add_libnanomsg_perf (remote_lat)
    add_libnanomsg_perf (local_thr)
    add_libnanomsg_perf (remote_thr)

endif ()

install (TARGETS LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (TARGETS ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES src/nn.h DESTINATION include/nanomsg)
install (FILES src/inproc.h DESTINATION include/nanomsg)
install (FILES src/ipc.h DESTINATION include/nanomsg)
install (FILES src/tcp.h DESTINATION include/nanomsg)
install (FILES src/ws.h DESTINATION include/nanomsg)
install (FILES src/pair.h DESTINATION include/nanomsg)
install (FILES src/pubsub.h DESTINATION include/nanomsg)
install (FILES src/reqrep.h DESTINATION include/nanomsg)
install (FILES src/pipeline.h DESTINATION include/nanomsg)
install (FILES src/survey.h DESTINATION include/nanomsg)
install (FILES src/bus.h DESTINATION include/nanomsg)

if (NN_ENABLE_NANOCAT)
    install (TARGETS nanocat RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

set (CPACK_PACKAGE_NAME ${PROJECT_NAME})
set (CPACK_PACKAGE_VERSION ${NN_PACKAGE_VERSION})
set (CPACK_SOURCE_GENERATOR "TBZ2;TGZ;ZIP")
set (CPACK_SOURCE_IGNORE_FILES "/build/;/.git/;~$;${CPACK_SOURCE_IGNORE_FILES}")
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${NN_PACKAGE_VERSION}")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
include (CPack)
