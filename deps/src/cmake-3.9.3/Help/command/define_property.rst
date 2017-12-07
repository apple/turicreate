define_property
---------------

Define and document custom properties.

::

  define_property(<GLOBAL | DIRECTORY | TARGET | SOURCE |
                   TEST | VARIABLE | CACHED_VARIABLE>
                   PROPERTY <name> [INHERITED]
                   BRIEF_DOCS <brief-doc> [docs...]
                   FULL_DOCS <full-doc> [docs...])

Define one property in a scope for use with the :command:`set_property` and
:command:`get_property` commands.  This is primarily useful to associate
documentation with property names that may be retrieved with the
:command:`get_property` command. The first argument determines the kind of
scope in which the property should be used.  It must be one of the
following:

::

  GLOBAL    = associated with the global namespace
  DIRECTORY = associated with one directory
  TARGET    = associated with one target
  SOURCE    = associated with one source file
  TEST      = associated with a test named with add_test
  VARIABLE  = documents a CMake language variable
  CACHED_VARIABLE = documents a CMake cache variable

Note that unlike :command:`set_property` and :command:`get_property` no
actual scope needs to be given; only the kind of scope is important.

The required ``PROPERTY`` option is immediately followed by the name of
the property being defined.

If the ``INHERITED`` option then the :command:`get_property` command will
chain up to the next higher scope when the requested property is not set
in the scope given to the command. ``DIRECTORY`` scope chains to
``GLOBAL``. ``TARGET``, ``SOURCE``, and ``TEST`` chain to ``DIRECTORY``.

The ``BRIEF_DOCS`` and ``FULL_DOCS`` options are followed by strings to be
associated with the property as its brief and full documentation.
Corresponding options to the :command:`get_property` command will retrieve
the documentation.
