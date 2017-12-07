CMAKE_<LANG>_FLAGS_INIT
-----------------------

Value used to initialize the :variable:`CMAKE_<LANG>_FLAGS` cache entry
the first time a build tree is configured for language ``<LANG>``.
This variable is meant to be set by a :variable:`toolchain file
<CMAKE_TOOLCHAIN_FILE>`.  CMake may prepend or append content to
the value based on the environment and target platform.

See also the configuration-specific variables:

* :variable:`CMAKE_<LANG>_FLAGS_DEBUG_INIT`
* :variable:`CMAKE_<LANG>_FLAGS_RELEASE_INIT`
* :variable:`CMAKE_<LANG>_FLAGS_MINSIZEREL_INIT`
* :variable:`CMAKE_<LANG>_FLAGS_RELWITHDEBINFO_INIT`
