Compiling Turi Create from Source
=================================

Repository Layout
-----------------

Note: Turi Create uses a [cmake out-of-source](https://cmake.org/Wiki/CMake_FAQ#Out-of-source_build_trees)
build. This means that the build itself, and the products of the build, take place outside of the source tree
(in our case in a directory structure parallel to `src/`, etc., but nested one level deeper underneath `debug/` or `release/`).
*Do not edit* any of the files underneath the build output directories, unless you want your changes to get
overwritten on the next build. Make changes in the `src/` directory, and run build commands to produce output.

* `src/`: source code of Turi Create
* `src/unity/python`: the Python module source code
* `src/unity/python/turicreate/test`: Python unit tests for Turi Create
* `src/external`: source drops of 3rd party source dependencies
* `deps/`: build dependencies and environment
* `debug/`, `release/`: build output directories for debug and release builds respectively
* `test/`: C++ unit tests for Turi Create

Build Dependencies
------------------

You will need:

* On macOS, [Xcode](https://itunes.apple.com/us/app/xcode/id497799835) with command line tools (tested with Xcode 9)
* On Linux, a C++ compiler toolchain with C++11 support
* [Node.js](https://nodejs.org) 6.x or later with `node` and `npm` in `$PATH`
* The python `virtualenv` package.

Turi Create automatically satisfies other dependencies in the `deps/` directory,
which includes compiler support and dependent libraries.

Compiling
---------

    pip install virtualenv

We use virtualenv to install required python packages into the build directory,
be sure to install virtualenv using your system pip (conda pip is not supported)

    ./configure

Running configure will create two sub-directories, `release/` and
`debug/` . cd into `src/unity` under either of these directories and running make will build the
release or the debug versions respectively.

We recommend using make’s parallel build feature to accelerate the compilation
process. For instance:

    cd debug/src/unity
    make -j 4

will perform up to 4 build tasks in parallel. When building in release mode,
Turi Create does require a large amount of memory to compile with the
heaviest toolkit requiring 1GB of RAM. Where K is the amount of memory you
have on your machine in GB, we recommend not exceeding `make -j K`

To use your dev build export these environment variables:

    source <repo root>/scripts/python_env.sh debug

or 

    source <repo root>/scripts/python_env.sh release

Running Unit Tests
------------------

### Running Python unit tests
From the repo root:

    cd debug/src/unity/python/turicreate/test
    pytest


### Running C++ units
From the repo root:

    cd debug/test
    make
    ctest .


