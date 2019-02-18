CMake Source Code Guide
***********************

The following is a guide to the CMake source code for developers.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: README.rst

C++ Code Style
==============

We use `clang-format`_ version **6.0** to define our style for C++ code in
the CMake source tree.  See the `.clang-format`_ configuration file for our
style settings.  Use the `Utilities/Scripts/clang-format.bash`_ script to
format source code.  It automatically runs ``clang-format`` on the set of
source files for which we enforce style.  The script also has options to
format only a subset of files, such as those that are locally modified.

.. _`clang-format`: http://clang.llvm.org/docs/ClangFormat.html
.. _`.clang-format`: ../../.clang-format
.. _`Utilities/Scripts/clang-format.bash`: ../../Utilities/Scripts/clang-format.bash

C++ Subset Permitted
====================

CMake requires compiling as C++11 or above.  However, in order to support
building on older toolchains some constructs need to be handled with care:

* Do not use ``std::auto_ptr``.

  The ``std::auto_ptr`` template is deprecated in C++11. Use ``std::unique_ptr``.

* Use ``CM_DISABLE_COPY(Class)`` to mark classes as non-copyable.

  The ``CM_DISABLE_COPY`` macro should be used in the private section of a
  class to make sure that attempts to copy or assign an instance of the class
  lead to compiler errors even if the compiler does not support *deleted*
  functions.  As a guideline, all polymorphic classes should be made
  non-copyable in order to avoid slicing.  Classes that are composed of or
  derived from non-copyable classes must also be made non-copyable explicitly
  with ``CM_DISABLE_COPY``.

* Use ``size_t`` instead of ``std::size_t``.

  Various implementations have differing implementation of ``size_t``.
  When assigning the result of ``.size()`` on a container for example,
  the result should be assigned to ``size_t`` not to ``std::size_t``,
  ``unsigned int`` or similar types.

Source Tree Layout
==================

The CMake source tree is organized as follows.

* ``Auxiliary/``:
  Shell and editor integration files.

* ``Help/``:
  Documentation.

  * ``Help/dev/``:
    Developer documentation.

  * ``Help/release/dev/``:
    Release note snippets for development since last release.

* ``Licenses/``:
  License files for third-party libraries in binary distributions.

* ``Modules/``:
  CMake language modules installed with CMake.

* ``Packaging/``:
  Files used for packaging CMake itself for distribution.

* ``Source/``:
  Source code of CMake itself.

* ``Templates/``:
  Files distributed with CMake as implementation details for generators,
  packagers, etc.

* ``Tests/``:
  The test suite.  See `Tests/README.rst`_.

* ``Utilities/``:
  Scripts, third-party source code.

  * ``Utilities/Sphinx/``:
    Sphinx configuration to build CMake user documentation.

  * ``Utilities/Release/``:
    Scripts used to package CMake itself for distribution on ``cmake.org``.

.. _`Tests/README.rst`: ../../Tests/README.rst
