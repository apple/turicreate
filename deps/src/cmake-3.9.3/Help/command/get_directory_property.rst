get_directory_property
----------------------

Get a property of ``DIRECTORY`` scope.

::

  get_directory_property(<variable> [DIRECTORY <dir>] <prop-name>)

Store a property of directory scope in the named variable.  If the
property is not defined the empty-string is returned.  The ``DIRECTORY``
argument specifies another directory from which to retrieve the
property value.  The specified directory must have already been
traversed by CMake.

::

  get_directory_property(<variable> [DIRECTORY <dir>]
                         DEFINITION <var-name>)

Get a variable definition from a directory.  This form is useful to
get a variable definition from another directory.

See also the more general :command:`get_property` command.
