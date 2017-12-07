ctest_start
-----------

Starts the testing for a given model

::

  ctest_start(Model [TRACK <track>] [APPEND] [source [binary]] [QUIET])

Starts the testing for a given model.  The command should be called
after the binary directory is initialized.  If the 'source' and
'binary' directory are not specified, it reads the
:variable:`CTEST_SOURCE_DIRECTORY` and :variable:`CTEST_BINARY_DIRECTORY`.
If the track is
specified, the submissions will go to the specified track.  If APPEND
is used, the existing TAG is used rather than creating a new one based
on the current time stamp.  If ``QUIET`` is used, CTest will suppress any
non-error messages that it otherwise would have printed to the console.

If the :variable:`CTEST_CHECKOUT_COMMAND` variable
(or the :variable:`CTEST_CVS_CHECKOUT` variable)
is set, its content is treated as command-line.  The command is
invoked with the current working directory set to the parent of the source
directory, even if the source directory already exists.  This can be used
to create the source tree from a version control repository.
