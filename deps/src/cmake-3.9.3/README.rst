CMake
*****

Introduction
============

CMake is a cross-platform, open-source build system generator.
For full documentation visit the `CMake Home Page`_ and the
`CMake Documentation Page`_.

.. _`CMake Home Page`: https://cmake.org
.. _`CMake Documentation Page`: https://cmake.org/cmake/help/documentation.html

CMake is maintained and supported by `Kitware`_ and developed in
collaboration with a productive community of contributors.

.. _`Kitware`: http://www.kitware.com/cmake

License
=======

CMake is distributed under the OSI-approved BSD 3-clause License.
See `Copyright.txt`_ for details.

.. _`Copyright.txt`: Copyright.txt

Building CMake
==============

Supported Platforms
-------------------

* Microsoft Windows
* Apple macOS
* Linux
* FreeBSD
* OpenBSD
* Solaris
* HP-UX

Other UNIX-like operating systems may work too out of the box, if not
it should not be a major problem to port CMake to this platform.
Subscribe and post to the `CMake Users List`_ to ask if others have
had experience with the platform.

.. _`CMake Users List`: https://cmake.org/mailman/listinfo/cmake

Building CMake from Scratch
---------------------------

UNIX/Mac OSX/MinGW/MSYS/Cygwin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You need to have a compiler and a make installed.
Run the ``bootstrap`` script you find in the source directory of CMake.
You can use the ``--help`` option to see the supported options.
You may use the ``--prefix=<install_prefix>`` option to specify a custom
installation directory for CMake. You can run the ``bootstrap`` script from
within the CMake source directory or any other build directory of your
choice. Once this has finished successfully, run ``make`` and
``make install``.  In summary::

 $ ./bootstrap && make && make install

Windows
^^^^^^^

You need to download and install a binary release of CMake in order to build
CMake.  You can get these releases from the `CMake Download Page`_ .  Then
proceed with the instructions below.

.. _`CMake Download Page`: https://cmake.org/cmake/resources/software.html

Building CMake with CMake
-------------------------

You can build CMake as any other project with a CMake-based build system:
run the installed CMake on the sources of this CMake with your preferred
options and generators. Then build it and install it.
For instructions how to do this, see documentation on `Running CMake`_.

.. _`Running CMake`: https://cmake.org/cmake/help/runningcmake.html

Reporting Bugs
==============

If you have found a bug:

1. If you have a patch, please read the `CONTRIBUTING.rst`_ document.

2. Otherwise, please join the `CMake Users List`_ and ask about
   the expected and observed behaviors to determine if it is really
   a bug.

3. Finally, if the issue is not resolved by the above steps, open
   an entry in the `CMake Issue Tracker`_.

.. _`CMake Issue Tracker`: https://gitlab.kitware.com/cmake/cmake/issues

Contributing
============

See `CONTRIBUTING.rst`_ for instructions to contribute.

.. _`CONTRIBUTING.rst`: CONTRIBUTING.rst
