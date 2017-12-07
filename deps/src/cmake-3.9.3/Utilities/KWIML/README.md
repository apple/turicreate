Kitware Information Macro Library (KWIML)
=========================================

KWIML provides header files that use preprocessor tests to detect and
provide information about the compiler and its target architecture.
The headers contain no configuration-time test results and thus may
be installed into an architecture-independent include directory.
This makes them suitable for use in the public interface of any package.

The following headers are provided.  See header comments for details:

* [kwiml/abi.h][]: Fundamental type size and representation.

* [kwiml/int.h][]: Fixed-size integer types and format specifiers.

* [kwiml/version.h][]: Information about this version of KWIML.

The [test][] subdirectory builds tests that verify correctness of the
information provided by each header.

License
=======

KWIML is distributed under the OSI-approved 3-clause BSD License.

Files used only for build and test purposes contain a copyright notice and
reference [Copyright.txt][] for details.  Headers meant for installation and
distribution outside the source tree come with full inlined copies of the
copyright notice and license text.  This makes them suitable for distribution
with any package under compatible license terms.

[Copyright.txt]: Copyright.txt
[kwiml/abi.h]: include/kwiml/abi.h
[kwiml/int.h]: include/kwiml/int.h
[kwiml/version.h]: src/version.h.in
[test]: test/
