.. cmake-manual-description: CPack Command-Line Reference

cpack(1)
********

Synopsis
========

.. parsed-literal::

 cpack -G <generator> [<options>]

Description
===========

The "cpack" executable is the CMake packaging program.
CMake-generated build trees created for projects that use the
INSTALL_* commands have packaging support.  This program will generate
the package.

CMake is a cross-platform build system generator.  Projects specify
their build process with platform-independent CMake listfiles included
in each directory of a source tree with the name CMakeLists.txt.
Users build a project by using CMake to generate a build system for a
native tool on their platform.

Options
=======

``-G <generator>``
 Use the specified generator to generate package.

 CPack may support multiple native packaging systems on certain
 platforms.  A generator is responsible for generating input files
 for particular system and invoking that systems.  Possible generator
 names are specified in the Generators section.

``-C <Configuration>``
 Specify the project configuration

 This option specifies the configuration that the project was build
 with, for example 'Debug', 'Release'.

``-D <var>=<value>``
 Set a CPack variable.

 Set a variable that can be used by the generator.

``--config <config file>``
 Specify the config file.

 Specify the config file to use to create the package.  By default
 CPackConfig.cmake in the current directory will be used.

``--verbose,-V``
 enable verbose output

 Run cpack with verbose output.

``--debug``
 enable debug output (for CPack developers)

 Run cpack with debug output (for CPack developers).

``-P <package name>``
 override/define CPACK_PACKAGE_NAME

 If the package name is not specified on cpack command line
 thenCPack.cmake defines it as CMAKE_PROJECT_NAME

``-R <package version>``
 override/define CPACK_PACKAGE_VERSION

 If version is not specified on cpack command line thenCPack.cmake
 defines it from CPACK_PACKAGE_VERSION_[MAJOR|MINOR|PATCH]look into
 CPack.cmake for detail

``-B <package directory>``
 override/define CPACK_PACKAGE_DIRECTORY

 The directory where CPack will be doing its packaging work.The
 resulting package will be found there.  Inside this directoryCPack
 creates '_CPack_Packages' sub-directory which is theCPack temporary
 directory.

``--vendor <vendor name>``
 override/define CPACK_PACKAGE_VENDOR

 If vendor is not specified on cpack command line (or inside
 CMakeLists.txt) thenCPack.cmake defines it with a default value

.. include:: OPTIONS_HELP.txt

See Also
========

.. include:: LINKS.txt
