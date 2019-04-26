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
* `scripts/`: Useful build, test, and repo maintenance scripts
* `test/`: C++ unit tests for Turi Create

Using Docker to build and test for Linux
----------------------------------------

Most of the commands in this repo, including `make_wheel.sh` and `test_wheel.sh`, have optional `--docker-python${VERSION}`
flags to run the build in Docker. This process is fully automated, and uses Docker images created on the fly from the following
docker files:

* `scripts/Dockerfile-Ubuntu-10.04`: used for building (see comments in that file re: GLIBC/GLIBCXX compatibility).
* `scripts/Dockerfile-Ubuntu-14.04`: used for testing Python 2.7 and Python 3.5 wheels.
* `scripts/Dockerfile-Ubuntu-18.04`: used for testing Python 3.6 wheels.

You can start with `./scripts/make_wheel.sh --help` to see available Docker build options.

Build Dependencies
------------------

You will need:

* On macOS, [Xcode](https://itunes.apple.com/us/app/xcode/id497799835) with command line tools (tested with Xcode 9 and later)
* On Linux:
  * A C++ compiler toolchain with C++11 support (provided by `build-essential` on Ubuntu)
  * blas and lapack development libraries (typically provided by the `liblapack-dev` package)
  * `pyfpe.h` (typically provided by the `libpython2.7-dev` package for Python 2.7, or `libpython3.6-dev` for Python 3.6)
  * `gif_lib.h` (typically provided by the `libgif-dev` package)
  * `xxd` (typically provided by the `vim-common` package)
  * For visualization support, X11 libraries (typically provided by `libx11-dev` or `libX11-devel` packages)
* On both macOS and Linux:
  * The python `virtualenv` package
* With Docker:
  * All dependencies are satisfied for you automatically (see "Using Docker" section above).

Optionally, you may want:

* `cmake` (if not present, one will be downloaded and bootstrapped locally).
* `ccache` (speeds up subsequent builds by caching intermediate outputs).

Turi Create automatically satisfies other dependencies in the `deps/` directory,
which includes compiler support and dependent libraries.

Compiling
---------

We use virtualenv to install required python packages into the build directory.
If you don't already have it, you can install it with:

    pip install virtualenv

Note that you may need to do a system-wide install with `sudo`; this depends on your Python environment and whether your `pip` binary requires sudo permissions. Alternately, you could try `pip install --user` to force a user-local installation if `pip install` gives permission denied errors.

Optionally, set a [generator](https://cmake.org/cmake/help/v3.0/manual/cmake-generators.7.html) for CMake before running `./configure`. Ninja can speed up incremental builds, but is not required.

    # Optional: set a generator
    # The default is "Unix Makefiles"
    export GENERATOR="-G Ninja"

Optionally, set an environment variable named `VIRTUALENV` if you want to use a different virtualenv binary than the default found in your `PATH`. This can allow you to build for multiple Python versions side-by-side on a single machine; note that the Python version that Turi Create is built for will correspond to the Python version of the `virtualenv` binary used.

    # Optional: override VIRTUALENV to point to a different binary
    # For instance, to build for Python 3.6 on a system where
    # /usr/local/bin/virtualenv is using Python 2.7
    export VIRTUALENV=/Library/Frameworks/Python.framework/Versions/3.6/bin/virtualenv

Then, run `./configure` (optionally with command line arguments to control what is built):

    ./configure

Running `./configure` will create two sub-directories, `release/` and
`debug/` . cd into `src/unity` under either of these directories and running make will build the
release or the debug versions respectively.

We recommend using make’s parallel build feature to accelerate the compilation
process. For instance:

    cd debug/src/unity
    make -j 4

will perform up to 4 build tasks in parallel. When building in release mode,
Turi Create does require a large amount of memory to compile with the
heaviest toolkit requiring 1GB of RAM. Where K is the amount of memory you
have on your machine in GB, we recommend not exceeding `make -j K`. Note that
if you are using ninja, it uses parallelism by default, and builds all targets
directly from the `debug/` or  `release/` directories.

To use your dev build after a successful build, enter the virtual environment
used for the build:

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
