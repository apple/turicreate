Welcome to nanomsg
==================

[![Release](https://img.shields.io/github/release/nanomsg/nanomsg.svg)](https://github.com/nanomsg/nanomsg/releases/latest)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/nanomsg/nanomsg/blob/master/COPYING)
[![Linux Status](https://img.shields.io/circleci/project/github/nanomsg/nanomsg/master.svg?label=linux)](https://circleci.com/gh/nanomsg/nanomsg)
[![Windows Status](https://img.shields.io/appveyor/ci/nanomsg/nanomsg/master.svg?label=windows)](https://ci.appveyor.com/project/nanomsg/nanomsg)
[![Coverage](https://codecov.io/gh/nanomsg/nanomsg/branch/master/graph/badge.svg?label=coverage)](https://codecov.io/gh/nanomsg/nanomsg)
[![Gitter](https://img.shields.io/badge/gitter-join-brightgreen.svg)](https://gitter.im/nanomsg/nanomsg)

The nanomsg library is a simple high-performance implementation of several
"scalability protocols". These scalability protocols are light-weight messaging
protocols which can be used to solve a number of very common messaging
patterns, such as request/reply, publish/subscribe, surveyor/respondent,
and so forth.  These protocols can run over a variety of transports such
as TCP, UNIX sockets, and even WebSocket.

For more information check the [website](http://nanomsg.org).

Prerequisites
-------------

1. Windows.
   * Windows Vista or newer (Windows XP and 2003 are *NOT* supported)
   * Microsoft Visual Studio 2010 (including C++) or newer, or mingw-w64.
     (Specifically mingw and older Microsoft compilers are *NOT supported,
     and we do not test mingw-w64 at all, so YMMV.)
   * CMake 2.8.7 or newer, available in $PATH as `cmake`

2. POSIX (Linux, MacOS X, UNIX)
   * ANSI C compiler supporting C89
   * POSIX pthreads (should be present on all modern POSIX systems)
   * BSD sockets support for both TCP and UNIX domain sockets
   * CMake (http://cmake.org) 2.8.7 or newer, available in $PATH as `cmake`

3. Documentation (optional)
   * asciidoctor (http://asciidoctor.org/) available as `asciidoctor`
   * If not present, docs are not formatted, but left in readable ASCII
   * Also available on-line at http://nanomsg.org/documentation

Quick Build Instructions
------------------------

These steps here are the minimum steps to get a default Debug
build.  Using CMake you can do many other things, including
setting additional variables, setting up for static builds, or
generation project or solution files for different development
environments.  Please check the CMake website for all the various
options that CMake supports.

## POSIX

This assumes you have a shell in the project directory, and have
the cmake and suitable compilers (and any required supporting tools
like linkers or archivers) on your path.

1.  `% mkdir build`
2.  `% cd build`
3.  `% cmake ..`
4.  `% cmake --build .`
5.  `% ctest .`
6.  `% sudo cmake --build . --target install`
7.  `% sudo ldconfig` (if on Linux)

## Windows

This assumes you are in a command or powershell window and have
the appropriate variables setup to support Visual Studio, typically
by running `vcvarsall.bat` or similar with the appropriate argument(s).
It also assumes you are in the project directory.

1.  `md build`
2.  `cd build`
3.  `cmake ..`
4.  `cmake --build . --config Debug`
5.  `ctest -C Debug .`
6.  `cmake --build . --config Debug --target install`
    *NB:* This may have to be done using an Administrator account.

Static Library
--------------

We normally build a dynamic library (.so or .DLL) by default.

If you want a static library (.a or .LIB), configure by passing
`-DNN_STATIC_LIB=ON` to the first `cmake` command.

### POSIX

POSIX systems will need to link with the libraries normally used when building
network applications.  For some systems this might mean -lnsl or -lsocket.

### Windows

You will also need to define `NN_STATIC_LIB` in your compilation environment
when building programs that use this library.  This is required because of
the way Windows changes symbol names depending on whether the symbols should
be exported in a DLL or not.

When using the .LIB on Windows, you will also need to link with the
ws2_32, mswsock, and advapi32 libraries, as nanomsg depends on them.

Support
-------

This library is considered to be in "sustaining" mode, which means that new
feature development has ended, and bug fixes are made only when strictly
necessary for severe issues.

New development is now occuring in the [NNG](https://github.com/nanomsg/nng)
project, which offers both protocol and API compatibility with this project.
Please consider using NNG for new projects.

Please see the file SUPPORT for more details.

Resources
---------

Website: [http://nanomsg.org](http://nanomsg.org)

Source code: [https://github.com/nanomsg/nanomsg](http://github.com/nanomsg/nanomsg)

Documentation: [http://nanomsg.org/documentation.html](http://nanomsg.org/documentation.html)

Bug tracker: [https://github.com/nanomsg/nanomsg/issues](http://github.com/nanomsg/nanomsg/issues)

Mailing list: [nanomsg@freelists.org](http://www.freelists.org/list/nanomsg)

Gitter Chat: [https://gitter.im/nanomsg/nanomsg](https://gitter.im/nanomsg/nanomsg)

IRC chatroom: [#nanomsg at irc.freenode.net/8001](http://webchat.freenode.net?channels=%23nanomsg)
