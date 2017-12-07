CMAKE_FIND_NO_INSTALL_PREFIX
----------------------------

Ignore the :variable:`CMAKE_INSTALL_PREFIX` when searching for assets.

CMake adds the :variable:`CMAKE_INSTALL_PREFIX` and the
:variable:`CMAKE_STAGING_PREFIX` variable to the
:variable:`CMAKE_SYSTEM_PREFIX_PATH` by default. This variable may be set
on the command line to control that behavior.

Set ``CMAKE_FIND_NO_INSTALL_PREFIX`` to ``TRUE`` to tell
:command:`find_package` not to search in the :variable:`CMAKE_INSTALL_PREFIX`
or :variable:`CMAKE_STAGING_PREFIX` by default.  Note that the
prefix may still be searched for other reasons, such as being the same prefix
as the CMake installation, or for being a built-in system prefix.
