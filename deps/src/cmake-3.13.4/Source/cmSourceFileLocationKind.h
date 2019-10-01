/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSourceFileLocationKind_h
#define cmSourceFileLocationKind_h

enum class cmSourceFileLocationKind
{
  // The location is user-specified and may be ambiguous.
  Ambiguous,
  // The location is known to be at the given location; do not try to guess at
  // extensions or absolute path.
  Known
};

#endif
