/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCustomCommandLines_h
#define cmCustomCommandLines_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

/** Data structure to represent a single command line.  */
class cmCustomCommandLine : public std::vector<std::string>
{
public:
  typedef std::vector<std::string> Superclass;
  typedef Superclass::iterator iterator;
  typedef Superclass::const_iterator const_iterator;
};

/** Data structure to represent a list of command lines.  */
class cmCustomCommandLines : public std::vector<cmCustomCommandLine>
{
public:
  typedef std::vector<cmCustomCommandLine> Superclass;
  typedef Superclass::iterator iterator;
  typedef Superclass::const_iterator const_iterator;
};

#endif
