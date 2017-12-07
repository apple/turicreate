project
-------

Set a name, version, and enable languages for the entire project.

.. code-block:: cmake

 project(<PROJECT-NAME> [LANGUAGES] [<language-name>...])
 project(<PROJECT-NAME>
         [VERSION <major>[.<minor>[.<patch>[.<tweak>]]]]
         [DESCRIPTION <project-description-string>]
         [LANGUAGES <language-name>...])

Sets the name of the project and stores the name in the
:variable:`PROJECT_NAME` variable.  Additionally this sets variables

* :variable:`PROJECT_SOURCE_DIR`,
  :variable:`<PROJECT-NAME>_SOURCE_DIR`
* :variable:`PROJECT_BINARY_DIR`,
  :variable:`<PROJECT-NAME>_BINARY_DIR`

If ``VERSION`` is specified, given components must be non-negative integers.
If ``VERSION`` is not specified, the default version is the empty string.
The ``VERSION`` option may not be used unless policy :policy:`CMP0048` is
set to ``NEW``.

The :command:`project()` command stores the version number and its components
in variables

* :variable:`PROJECT_VERSION`,
  :variable:`<PROJECT-NAME>_VERSION`
* :variable:`PROJECT_VERSION_MAJOR`,
  :variable:`<PROJECT-NAME>_VERSION_MAJOR`
* :variable:`PROJECT_VERSION_MINOR`,
  :variable:`<PROJECT-NAME>_VERSION_MINOR`
* :variable:`PROJECT_VERSION_PATCH`,
  :variable:`<PROJECT-NAME>_VERSION_PATCH`
* :variable:`PROJECT_VERSION_TWEAK`,
  :variable:`<PROJECT-NAME>_VERSION_TWEAK`

Variables corresponding to unspecified versions are set to the empty string
(if policy :policy:`CMP0048` is set to ``NEW``).

If optional ``DESCRIPTION`` is given, then additional :variable:`PROJECT_DESCRIPTION`
variable will be set to its argument. The argument must be a string with short
description of the project (only a few words).

Optionally you can specify which languages your project supports.
Example languages are ``C``, ``CXX`` (i.e.  C++), ``Fortran``, etc.
By default ``C`` and ``CXX`` are enabled if no language options are
given.  Specify language ``NONE``, or use the ``LANGUAGES`` keyword
and list no languages, to skip enabling any languages.

If a variable exists called :variable:`CMAKE_PROJECT_<PROJECT-NAME>_INCLUDE`,
the file pointed to by that variable will be included as the last step of the
project command.

The top-level ``CMakeLists.txt`` file for a project must contain a
literal, direct call to the :command:`project` command; loading one
through the :command:`include` command is not sufficient.  If no such
call exists CMake will implicitly add one to the top that enables the
default languages (``C`` and ``CXX``).

.. note::
  Call the :command:`cmake_minimum_required` command at the beginning
  of the top-level ``CMakeLists.txt`` file even before calling the
  ``project()`` command.  It is important to establish version and
  policy settings before invoking other commands whose behavior they
  may affect.  See also policy :policy:`CMP0000`.
