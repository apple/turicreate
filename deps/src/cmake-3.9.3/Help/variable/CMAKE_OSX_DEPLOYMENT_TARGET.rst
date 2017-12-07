CMAKE_OSX_DEPLOYMENT_TARGET
---------------------------

Specify the minimum version of OS X on which the target binaries are
to be deployed.  CMake uses this value for the ``-mmacosx-version-min``
flag and to help choose the default SDK
(see :variable:`CMAKE_OSX_SYSROOT`).

If not set explicitly the value is initialized by the
``MACOSX_DEPLOYMENT_TARGET`` environment variable, if set,
and otherwise computed based on the host platform.

.. include:: CMAKE_OSX_VARIABLE.txt
