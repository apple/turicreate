/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmParseBlanketJSCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include <stdio.h>
#include <stdlib.h>

class cmParseBlanketJSCoverage::JSONParser
{
public:
  typedef cmCTestCoverageHandlerContainer::SingleFileCoverageVector
    FileLinesType;
  JSONParser(cmCTestCoverageHandlerContainer& cont)
    : Coverage(cont)
  {
  }

  virtual ~JSONParser() {}

  std::string getValue(std::string const& line, int type)
  {
    size_t begIndex;
    size_t endIndex;
    endIndex = line.rfind(',');
    begIndex = line.find_first_of(':');
    if (type == 0) {
      //  A unique substring to remove the extra characters
      //  around the files name in the JSON (extra " and ,)
      std::string foundFileName =
        line.substr(begIndex + 3, endIndex - (begIndex + 4));
      return foundFileName;
    }
    return line.substr(begIndex);
  }
  bool ParseFile(std::string const& file)
  {
    FileLinesType localCoverageVector;
    std::string filename;
    bool foundFile = false;
    bool inSource = false;
    std::string covResult;
    std::string line;

    cmsys::ifstream in(file.c_str());
    if (!in) {
      return false;
    }
    while (cmSystemTools::GetLineFromStream(in, line)) {
      if (line.find("filename") != std::string::npos) {
        if (foundFile) {
          /*
           * Upon finding a second file name, generate a
           * vector within the total coverage to capture the
           * information in the local vector
           */
          FileLinesType& CoverageVector =
            this->Coverage.TotalCoverage[filename];
          CoverageVector = localCoverageVector;
          localCoverageVector.clear();
        }
        foundFile = true;
        inSource = false;
        filename = getValue(line, 0);
      } else if ((line.find("coverage") != std::string::npos) && foundFile &&
                 inSource) {
        /*
         *  two types of "coverage" in the JSON structure
         *
         *  The coverage result over the file or set of files
         *  and the coverage for each individual line
         *
         *  FoundFile and foundSource ensure that
         *  only the value of the line coverage is captured
         */
        std::string result = getValue(line, 1);
        result = result.substr(2);
        if (result == "\"\"") {
          // Empty quotation marks indicate that the
          // line is not executable
          localCoverageVector.push_back(-1);
        } else {
          // Else, it contains the number of time executed
          localCoverageVector.push_back(atoi(result.c_str()));
        }
      } else if (line.find("source") != std::string::npos) {
        inSource = true;
      }
    }

    // On exit, capture end of last file covered.
    FileLinesType& CoverageVector = this->Coverage.TotalCoverage[filename];
    CoverageVector = localCoverageVector;
    localCoverageVector.clear();
    return true;
  }

private:
  cmCTestCoverageHandlerContainer& Coverage;
};

cmParseBlanketJSCoverage::cmParseBlanketJSCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

bool cmParseBlanketJSCoverage::LoadCoverageData(std::vector<std::string> files)
{
  size_t i = 0;
  std::string path;
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Found " << files.size() << " Files" << std::endl,
                     this->Coverage.Quiet);
  for (i = 0; i < files.size(); i++) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Reading JSON File " << files[i] << std::endl,
                       this->Coverage.Quiet);

    if (!this->ReadJSONFile(files[i])) {
      return false;
    }
  }
  return true;
}

bool cmParseBlanketJSCoverage::ReadJSONFile(std::string const& file)
{
  cmParseBlanketJSCoverage::JSONParser parser(this->Coverage);
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Parsing " << file << std::endl, this->Coverage.Quiet);
  parser.ParseFile(file);
  return true;
}
