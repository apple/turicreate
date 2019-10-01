LINK_OPTIONS
------------

List of options to use for the link step of shared library, module
and executable targets. Targets that are static libraries need to use
the :prop_tgt:`STATIC_LIBRARY_OPTIONS` target property.


This property holds a :ref:`;-list <CMake Language Lists>` of options
specified so far for its target.  Use the :command:`target_link_options`
command to append more options.

This property is initialized by the :prop_dir:`LINK_OPTIONS` directory
property when a target is created, and is used by the generators to set
the options for the compiler.

Contents of ``LINK_OPTIONS`` may use "generator expressions" with the
syntax ``$<...>``.  See the :manual:`cmake-generator-expressions(7)` manual
for available expressions.  See the :manual:`cmake-buildsystem(7)` manual
for more on defining buildsystem properties.

.. note::

  This property must be used in preference to :prop_tgt:`LINK_FLAGS` property.
