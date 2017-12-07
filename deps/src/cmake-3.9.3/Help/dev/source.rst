CMake Source Code Guide
***********************

The following is a guide to the CMake source code for developers.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: README.rst

C++ Code Style
==============

We use `clang-format`_ version **3.8** to define our style for C++ code in
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

CMake supports compiling as C++98 in addition to C++11 and C++14.
In order to support building on older toolchains some constructs
need to be handled with care:

* Use ``CM_AUTO_PTR`` instead of ``std::auto_ptr``.

  The ``std::auto_ptr`` template is deprecated in C++11.  We want to use it
  so we can build on C++98 compilers but we do not want to turn off compiler
  warnings about deprecated interfaces in general.  Use the ``CM_AUTO_PTR``
  macro instead.

* Use ``CM_EQ_DELETE;`` instead of ``= delete;``.

  Defining functions as *deleted* is not supported in C++98.  Using
  ``CM_EQ_DELETE`` will delete the functions if the compiler supports it and
  give them no implementation otherwise.  Calling such a function will lead
  to compiler errors if the compiler supports *deleted* functions and linker
  errors otherwise.

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
