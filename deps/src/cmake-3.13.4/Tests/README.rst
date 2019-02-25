CMake Tests Directory
*********************

This directory contains the CMake test suite.
See also the `CMake Source Code Guide`_.

.. _`CMake Source Code Guide`: ../Help/dev/source.rst

Many tests exist as immediate subdirectories, but some tests
are organized as follows.

* ``CMakeLib/``:
  Source code, used for tests, that links to the ``CMakeLib`` library
  defined over in ``Source/``.

* ``CMakeOnly/``:
  Deprecated.  Tests that run CMake to generate a project but not build it.
  Superseded by ``Tests/RunCMake/``.

* ``Find*/``:
  Tests for specific find modules that can only be run on machines with
  the corresponding packages installed.  They are enabled in
  ``CMakeLists.txt`` by undocumented options used on CI builds.

* ``Module/``:
  Tests for specific CMake modules.

* ``RunCMake/``:
  Tests that run CMake and/or other tools while precisely checking
  their return code and stdout/stderr content.  Useful for testing
  error cases and diagnostic output.
