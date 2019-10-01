add_link_options
----------------

Adds options to the link of shared library, module and executable targets.

::

  add_link_options(<option> ...)

Adds options to the link step for targets in the current directory and below
that are added after this command is invoked. See documentation of the
:prop_dir:`directory <LINK_OPTIONS>` and
:prop_tgt:`target <LINK_OPTIONS>` ``LINK_OPTIONS`` properties.

This command can be used to add any options, but alternative commands
exist to add libraries (:command:`target_link_libraries` or
:command:`link_libraries`).

Arguments to ``add_link_options`` may use "generator expressions" with
the syntax ``$<...>``.  See the :manual:`cmake-generator-expressions(7)`
manual for available expressions.  See the :manual:`cmake-buildsystem(7)`
manual for more on defining buildsystem properties.

.. include:: OPTIONS_SHELL.txt

.. include:: LINK_OPTIONS_LINKER.txt
