#include "cmParseJacocoCoverage.h"

#include "cmConfigure.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include <stdlib.h>
#include <string.h>

class cmParseJacocoCoverage::XMLParser : public cmXMLParser
{
public:
  XMLParser(cmCTest* ctest, cmCTestCoverageHandlerContainer& cont)
    : CTest(ctest)
    , Coverage(cont)
  {
    this->FilePath = "";
    this->PackagePath = "";
    this->PackageName = "";
  }

  ~XMLParser() CM_OVERRIDE {}

protected:
  void EndElement(const std::string& /*name*/) CM_OVERRIDE {}

  void StartElement(const std::string& name, const char** atts) CM_OVERRIDE
  {
    if (name == "package") {
      this->PackageName = atts[1];
      this->PackagePath = "";
    } else if (name == "sourcefile") {
      std::string fileName = atts[1];

      if (this->PackagePath == "") {
        if (!this->FindPackagePath(fileName)) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot find file: "
                       << this->PackageName << "/" << fileName << std::endl);
          this->Coverage.Error++;
          return;
        }
      }

      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Reading file: " << fileName << std::endl,
                         this->Coverage.Quiet);

      this->FilePath = this->PackagePath + "/" + fileName;
      cmsys::ifstream fin(this->FilePath.c_str());
      if (!fin) {
        cmCTestLog(this->CTest, ERROR_MESSAGE,
                   "Jacoco Coverage: Error opening " << this->FilePath
                                                     << std::endl);
      }
      std::string line;
      FileLinesType& curFileLines =
        this->Coverage.TotalCoverage[this->FilePath];
      if (fin) {
        curFileLines.push_back(-1);
      }
      while (cmSystemTools::GetLineFromStream(fin, line)) {
        curFileLines.push_back(-1);
      }
    } else if (name == "line") {
      int tagCount = 0;
      int nr = -1;
      int ci = -1;
      while (true) {
        if (strcmp(atts[tagCount], "ci") == 0) {
          ci = atoi(atts[tagCount + 1]);
        } else if (strcmp(atts[tagCount], "nr") == 0) {
          nr = atoi(atts[tagCount + 1]);
        }
        if (ci > -1 && nr > 0) {
          FileLinesType& curFileLines =
            this->Coverage.TotalCoverage[this->FilePath];
          if (!curFileLines.empty()) {
            curFileLines[nr - 1] = ci;
          }
          break;
        }
        ++tagCount;
      }
    }
  }

  virtual bool FindPackagePath(std::string const& fileName)
  {
    // Search for the source file in the source directory.
    if (this->PackagePathFound(fileName, this->Coverage.SourceDir)) {
      return true;
    }

    // If not found there, check the binary directory.
    if (this->PackagePathFound(fileName, this->Coverage.BinaryDir)) {
      return true;
    }
    return false;
  }

  virtual bool PackagePathFound(std::string const& fileName,
                                std::string const& baseDir)
  {
    // Search for the file in the baseDir and its subdirectories.
    std::string packageGlob = baseDir;
    packageGlob += "/";
    packageGlob += fileName;
    cmsys::Glob gl;
    gl.RecurseOn();
    gl.RecurseThroughSymlinksOn();
    gl.FindFiles(packageGlob);
    std::vector<std::string> const& files = gl.GetFiles();
    if (files.empty()) {
      return false;
    }

    // Check if any of the locations found match our package.
    for (std::vector<std::string>::const_iterator fi = files.begin();
         fi != files.end(); ++fi) {
      std::string dir = cmsys::SystemTools::GetParentDirectory(*fi);
      if (cmsys::SystemTools::StringEndsWith(dir, this->PackageName.c_str())) {
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "Found package directory for " << fileName << ": "
                                                          << dir << std::endl,
                           this->Coverage.Quiet);
        this->PackagePath = dir;
        return true;
      }
    }
    return false;
  }

private:
  std::string FilePath;
  std::string PackagePath;
  std::string PackageName;
  typedef cmCTestCoverageHandlerContainer::SingleFileCoverageVector
    FileLinesType;
  cmCTest* CTest;
  cmCTestCoverageHandlerContainer& Coverage;
};

cmParseJacocoCoverage::cmParseJacocoCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

bool cmParseJacocoCoverage::LoadCoverageData(
  std::vector<std::string> const& files)
{
  // load all the jacoco.xml files in the source directory
  cmsys::Directory dir;
  size_t i;
  std::string path;
  size_t numf = files.size();
  for (i = 0; i < numf; i++) {
    path = files[i];

    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Reading XML File " << path << std::endl,
                       this->Coverage.Quiet);
    if (cmSystemTools::GetFilenameLastExtension(path) == ".xml") {
      if (!this->ReadJacocoXML(path.c_str())) {
        return false;
      }
    }
  }
  return true;
}

bool cmParseJacocoCoverage::ReadJacocoXML(const char* file)
{
  cmParseJacocoCoverage::XMLParser parser(this->CTest, this->Coverage);
  parser.ParseFile(file);
  return true;
}
