#include "cmParseDelphiCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include <stdio.h>
#include <stdlib.h>

class cmParseDelphiCoverage::HTMLParser
{
public:
  typedef cmCTestCoverageHandlerContainer::SingleFileCoverageVector
    FileLinesType;
  HTMLParser(cmCTest* ctest, cmCTestCoverageHandlerContainer& cont)
    : CTest(ctest)
    , Coverage(cont)
  {
  }

  virtual ~HTMLParser() {}

  bool initializeDelphiFile(
    std::string const& filename,
    cmParseDelphiCoverage::HTMLParser::FileLinesType& coverageVector)
  {
    std::string line;
    size_t comPos;
    size_t semiPos;
    bool blockComFlag = false;
    bool lineComFlag = false;
    std::vector<std::string> beginSet;
    cmsys::ifstream in(filename.c_str());
    if (!in) {
      return false;
    }
    while (cmSystemTools::GetLineFromStream(in, line)) {
      lineComFlag = false;
      // Unique cases found in lines.
      size_t beginPos = line.find("begin");

      // Check that the begin is the first non-space string on the line
      if ((beginPos == line.find_first_not_of(' ')) &&
          beginPos != std::string::npos) {
        beginSet.push_back("begin");
        coverageVector.push_back(-1);
        continue;
      }
      if (line.find('{') != std::string::npos) {
        blockComFlag = true;
      } else if (line.find('}') != std::string::npos) {
        blockComFlag = false;
        coverageVector.push_back(-1);
        continue;
      } else if ((line.find("end;") != std::string::npos) &&
                 !beginSet.empty()) {
        beginSet.pop_back();
        coverageVector.push_back(-1);
        continue;
      }

      //  This checks for comments after lines of code, finding the
      //  comment symbol after the ending semicolon.
      comPos = line.find("//");
      if (comPos != std::string::npos) {
        semiPos = line.find(';');
        if (comPos < semiPos) {
          lineComFlag = true;
        }
      }
      // Based up what was found, add a line to the coverageVector
      if (!beginSet.empty() && line != "" && !blockComFlag && !lineComFlag) {
        coverageVector.push_back(0);
      } else {
        coverageVector.push_back(-1);
      }
    }
    return true;
  }
  bool ParseFile(const char* file)
  {
    std::string line = file;
    std::string lineresult;
    std::string lastroutine;
    std::string filename;
    std::string filelineoffset;
    size_t afterLineNum = 0;
    size_t lastoffset = 0;
    size_t endcovpos = 0;
    size_t endnamepos = 0;
    size_t pos = 0;

    /*
     *  This first 'while' section goes through the found HTML
     *  file name and attempts to capture the source file name
     *   which is set as part of the HTML file name: the name of
     *   the file is found in parenthesis '()'
     *
     *   See test HTML file name: UTCovTest(UTCovTest.pas).html.
     *
     *   Find the text inside each pair of parenthesis and check
     *   to see if it ends in '.pas'.  If it can't be found,
     *   exit the function.
     */
    while (true) {
      lastoffset = line.find('(', pos);
      if (lastoffset == std::string::npos) {
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, endnamepos
                             << "File not found  " << lastoffset << std::endl,
                           this->Coverage.Quiet);
        return false;
      }
      endnamepos = line.find(')', lastoffset);
      filename = line.substr(lastoffset + 1, (endnamepos - 1) - lastoffset);
      if (filename.find(".pas") != std::string::npos) {
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "Coverage found for file:  " << filename
                                                        << std::endl,
                           this->Coverage.Quiet);
        break;
      }
      pos = lastoffset + 1;
    }
    /*
     *  Glob through the source directory for the
     *  file found above
     */
    cmsys::Glob gl;
    gl.RecurseOn();
    gl.RecurseThroughSymlinksOff();
    std::string glob = Coverage.SourceDir + "*/" + filename;
    gl.FindFiles(glob);
    std::vector<std::string> const& files = gl.GetFiles();
    if (files.empty()) {
      /*
       *  If that doesn't find any matching files
       *  return a failure.
       */
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Unable to find file matching" << glob << std::endl,
                         this->Coverage.Quiet);
      return false;
    }
    FileLinesType& coverageVector = this->Coverage.TotalCoverage[files[0]];

    /*
     *  Initialize the file to have all code between 'begin' and
     *  'end' tags marked as executable
     */

    this->initializeDelphiFile(files[0], coverageVector);

    cmsys::ifstream in(file);
    if (!in) {
      return false;
    }

    /*
     *  Now read the HTML file, looking for the lines that have an
     *  "inline" in it. Then parse out the "class" value of that
     *  line to determine if the line is executed or not.
     *
     *  Sample HTML line:
     *
     *  <tr class="covered"><td>47</td><td><pre style="display:inline;">
     *    &nbsp;CheckEquals(1,2-1);</pre></td></tr>
     *
     */

    while (cmSystemTools::GetLineFromStream(in, line)) {
      if (line.find("inline") == std::string::npos) {
        continue;
      }

      lastoffset = line.find("class=");
      endcovpos = line.find('>', lastoffset);
      lineresult = line.substr(lastoffset + 7, (endcovpos - 8) - lastoffset);

      if (lineresult == "covered") {
        afterLineNum = line.find('<', endcovpos + 5);
        filelineoffset =
          line.substr(endcovpos + 5, afterLineNum - (endcovpos + 5));
        coverageVector[atoi(filelineoffset.c_str()) - 1] = 1;
      }
    }
    return true;
  }

private:
  cmCTest* CTest;
  cmCTestCoverageHandlerContainer& Coverage;
};

cmParseDelphiCoverage::cmParseDelphiCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

bool cmParseDelphiCoverage::LoadCoverageData(
  std::vector<std::string> const& files)
{
  size_t i;
  std::string path;
  size_t numf = files.size();
  for (i = 0; i < numf; i++) {
    path = files[i];

    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Reading HTML File " << path << std::endl,
                       this->Coverage.Quiet);
    if (cmSystemTools::GetFilenameLastExtension(path) == ".html") {
      if (!this->ReadDelphiHTML(path.c_str())) {
        return false;
      }
    }
  }
  return true;
}

bool cmParseDelphiCoverage::ReadDelphiHTML(const char* file)
{
  cmParseDelphiCoverage::HTMLParser parser(this->CTest, this->Coverage);
  parser.ParseFile(file);
  return true;
}
