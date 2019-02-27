/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmParseGTMCoverage_h
#define cmParseGTMCoverage_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmParseMumpsCoverage.h"

#include <string>

class cmCTest;
class cmCTestCoverageHandlerContainer;

/** \class cmParseGTMCoverage
 * \brief Parse GTM coverage information
 *
 * This class is used to parse GTM coverage information for
 * mumps.
 */
class cmParseGTMCoverage : public cmParseMumpsCoverage
{
public:
  cmParseGTMCoverage(cmCTestCoverageHandlerContainer& cont, cmCTest* ctest);

protected:
  // implement virtual from parent
  bool LoadCoverageData(const char* dir) override;
  // Read a single mcov file
  bool ReadMCovFile(const char* f);
  // find out what line in a mumps file (filepath) the given entry point
  // or function is.  lineoffset is set by this method.
  bool FindFunctionInMumpsFile(std::string const& filepath,
                               std::string const& function, int& lineoffset);
  // parse a line from a .mcov file, and fill in the
  // routine, function, linenumber and coverage count
  bool ParseMCOVLine(std::string const& line, std::string& routine,
                     std::string& function, int& linenumber, int& count);
};

#endif
