Visual Studio 8 2005
--------------------

Deprecated.  Generates Visual Studio 8 2005 project files.

.. note::
  This generator is deprecated and will be removed in a future version
  of CMake.  It will still be possible to build with VS 8 2005 tools
  using the :generator:`Visual Studio 10 2010` (or above) generator
  with :variable:`CMAKE_GENERATOR_TOOLSET` set to ``v80``, or by
  using the :generator:`NMake Makefiles` generator.

The :variable:`CMAKE_GENERATOR_PLATFORM` variable may be set
to specify a target platform name.

For compatibility with CMake versions prior to 3.1, one may specify
a target platform name optionally at the end of this generator name:

``Visual Studio 8 2005 Win64``
  Specify target platform ``x64``.

``Visual Studio 8 2005 <WinCE-SDK>``
  Specify target platform matching a Windows CE SDK name.
