CMAKE_VS_WINRT_BY_DEFAULT
-------------------------

Tell :ref:`Visual Studio Generators` for VS 2010 and above that the
target platform compiles as WinRT by default (compiles with ``/ZW``).

This variable is meant to be set by a
:variable:`toolchain file <CMAKE_TOOLCHAIN_FILE>` for such platforms.
