/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmParseCacheCoverage_h
#define cmParseCacheCoverage_h

#include "cmConfigure.h"

#include "cmParseMumpsCoverage.h"

#include <string>
#include <vector>

class cmCTest;
class cmCTestCoverageHandlerContainer;

/** \class cmParseCacheCoverage
 * \brief Parse Cache coverage information
 *
 * This class is used to parse Cache coverage information for
 * mumps.
 */
class cmParseCacheCoverage : public cmParseMumpsCoverage
{
public:
  cmParseCacheCoverage(cmCTestCoverageHandlerContainer& cont, cmCTest* ctest);

protected:
  // implement virtual from parent
  bool LoadCoverageData(const char* dir) CM_OVERRIDE;
  // remove files with no coverage
  void RemoveUnCoveredFiles();
  // Read a single mcov file
  bool ReadCMCovFile(const char* f);
  // split a string based on ,
  bool SplitString(std::vector<std::string>& args, std::string const& line);
};

#endif
