/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmParsePHPCoverage_h
#define cmParsePHPCoverage_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

class cmCTest;
class cmCTestCoverageHandlerContainer;

/** \class cmParsePHPCoverage
 * \brief Parse xdebug PHP coverage information
 *
 * This class is used to parse php coverage information produced
 * by xdebug.  The data is stored as a php dump of the array
 * return by xdebug coverage.  It is an array of arrays.
 */
class cmParsePHPCoverage
{
public:
  cmParsePHPCoverage(cmCTestCoverageHandlerContainer& cont, cmCTest* ctest);
  bool ReadPHPCoverageDirectory(const char* dir);
  void PrintCoverage();

private:
  bool ReadPHPData(const char* file);
  bool ReadArraySize(std::istream& in, int& size);
  bool ReadFileInformation(std::istream& in);
  bool ReadInt(std::istream& in, int& v);
  bool ReadCoverageArray(std::istream& in, std::string const&);
  bool ReadUntil(std::istream& in, char until);
  cmCTestCoverageHandlerContainer& Coverage;
  cmCTest* CTest;
};

#endif
