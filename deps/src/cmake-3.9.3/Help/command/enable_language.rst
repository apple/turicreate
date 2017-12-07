enable_language
---------------

Enable a language (CXX/C/Fortran/etc)

::

  enable_language(<lang> [OPTIONAL] )

This command enables support for the named language in CMake.  This is
the same as the project command but does not create any of the extra
variables that are created by the project command.  Example languages
are CXX, C, Fortran.

This command must be called in file scope, not in a function call.
Furthermore, it must be called in the highest directory common to all
targets using the named language directly for compiling sources or
indirectly through link dependencies.  It is simplest to enable all
needed languages in the top-level directory of a project.

The ``OPTIONAL`` keyword is a placeholder for future implementation and
does not currently work.
