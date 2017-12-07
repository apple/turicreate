CMAKE_<LANG>_IMPLICIT_LINK_DIRECTORIES
--------------------------------------

Implicit linker search path detected for language ``<LANG>``.

Compilers typically pass directories containing language runtime
libraries and default library search paths when they invoke a linker.
These paths are implicit linker search directories for the compiler's
language.  CMake automatically detects these directories for each
language and reports the results in this variable.

When a library in one of these directories is given by full path to
:command:`target_link_libraries` CMake will generate the ``-l<name>`` form on
link lines to ensure the linker searches its implicit directories for the
library.  Note that some toolchains read implicit directories from an
environment variable such as ``LIBRARY_PATH`` so keep its value consistent
when operating in a given build tree.
