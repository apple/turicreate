Visual Studio 15 2017
---------------------

Generates Visual Studio 15 (VS 2017) project files.

The :variable:`CMAKE_GENERATOR_PLATFORM` variable may be set
to specify a target platform name (architecture).

For compatibility with CMake versions prior to 3.1, one may specify
a target platform name optionally at the end of this generator name:

``Visual Studio 15 2017 Win64``
  Specify target platform ``x64``.

``Visual Studio 15 2017 ARM``
  Specify target platform ``ARM``.

Instance Selection
^^^^^^^^^^^^^^^^^^

VS 2017 supports multiple installations on the same machine.
CMake queries the Visual Studio Installer to locate VS instances.
If more than one instance is installed we do not define which one
is chosen by default.  If the ``VS150COMNTOOLS`` environment variable
is set and points to the ``Common7/Tools`` directory within one of
the instances, that instance will be used.  The environment variable
must remain consistently set whenever CMake is re-run within a given
build tree.

Toolset Selection
^^^^^^^^^^^^^^^^^

The ``v141`` toolset that comes with Visual Studio 15 2017 is selected by
default.  The :variable:`CMAKE_GENERATOR_TOOLSET` option may be set, perhaps
via the :manual:`cmake(1)` ``-T`` option, to specify another toolset.

.. include:: VS_TOOLSET_HOST_ARCH.txt
