#include "cmParseCoberturaCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"

#include "cmConfigure.h"
#include "cmsys/FStream.hxx"
#include <stdlib.h>
#include <string.h>

class cmParseCoberturaCoverage::XMLParser : public cmXMLParser
{
public:
  XMLParser(cmCTest* ctest, cmCTestCoverageHandlerContainer& cont)
    : CTest(ctest)
    , Coverage(cont)
  {
    this->InSources = false;
    this->InSource = false;
    this->SkipThisClass = false;
    this->FilePaths.push_back(this->Coverage.SourceDir);
    this->FilePaths.push_back(this->Coverage.BinaryDir);
    this->CurFileName = "";
  }

  ~XMLParser() CM_OVERRIDE {}

protected:
  void EndElement(const std::string& name) CM_OVERRIDE
  {
    if (name == "source") {
      this->InSource = false;
    } else if (name == "sources") {
      this->InSources = false;
    } else if (name == "class") {
      this->SkipThisClass = false;
    }
  }

  void CharacterDataHandler(const char* data, int length) CM_OVERRIDE
  {
    std::string tmp;
    tmp.insert(0, data, length);
    if (this->InSources && this->InSource) {
      this->FilePaths.push_back(tmp);
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Adding Source: " << tmp << std::endl,
                         this->Coverage.Quiet);
    }
  }

  void StartElement(const std::string& name, const char** atts) CM_OVERRIDE
  {
    std::string FoundSource;
    std::string finalpath;
    if (name == "source") {
      this->InSource = true;
    } else if (name == "sources") {
      this->InSources = true;
    } else if (name == "class") {
      int tagCount = 0;
      while (true) {
        if (strcmp(atts[tagCount], "filename") == 0) {
          cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                             "Reading file: " << atts[tagCount + 1]
                                              << std::endl,
                             this->Coverage.Quiet);
          std::string filename = atts[tagCount + 1];
          this->CurFileName = "";

          // Check if this is an absolute path that falls within our
          // source or binary directories.
          for (size_t i = 0; i < FilePaths.size(); i++) {
            if (filename.find(FilePaths[i]) == 0) {
              this->CurFileName = filename;
              break;
            }
          }

          if (this->CurFileName == "") {
            // Check if this is a path that is relative to our source or
            // binary directories.
            for (size_t i = 0; i < FilePaths.size(); i++) {
              finalpath = FilePaths[i] + "/" + filename;
              if (cmSystemTools::FileExists(finalpath.c_str())) {
                this->CurFileName = finalpath;
                break;
              }
            }
          }

          cmsys::ifstream fin(this->CurFileName.c_str());
          if (this->CurFileName == "" || !fin) {
            this->CurFileName =
              this->Coverage.BinaryDir + "/" + atts[tagCount + 1];
            fin.open(this->CurFileName.c_str());
            if (!fin) {
              cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                                 "Skipping system file " << filename
                                                         << std::endl,
                                 this->Coverage.Quiet);

              this->SkipThisClass = true;
              break;
            }
          }
          std::string line;
          FileLinesType& curFileLines =
            this->Coverage.TotalCoverage[this->CurFileName];
          while (cmSystemTools::GetLineFromStream(fin, line)) {
            curFileLines.push_back(-1);
          }

          break;
        }
        ++tagCount;
      }
    } else if (name == "line") {
      int tagCount = 0;
      int curNumber = -1;
      int curHits = -1;
      while (true) {
        if (this->SkipThisClass) {
          break;
        }
        if (strcmp(atts[tagCount], "hits") == 0) {
          curHits = atoi(atts[tagCount + 1]);
        } else if (strcmp(atts[tagCount], "number") == 0) {
          curNumber = atoi(atts[tagCount + 1]);
        }

        if (curHits > -1 && curNumber > 0) {
          FileLinesType& curFileLines =
            this->Coverage.TotalCoverage[this->CurFileName];
          {
            curFileLines[curNumber - 1] = curHits;
          }
          break;
        }
        ++tagCount;
      }
    }
  }

private:
  bool InSources;
  bool InSource;
  bool SkipThisClass;
  std::vector<std::string> FilePaths;
  typedef cmCTestCoverageHandlerContainer::SingleFileCoverageVector
    FileLinesType;
  cmCTest* CTest;
  cmCTestCoverageHandlerContainer& Coverage;
  std::string CurFileName;
};

cmParseCoberturaCoverage::cmParseCoberturaCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

bool cmParseCoberturaCoverage::ReadCoverageXML(const char* xmlFile)
{
  cmParseCoberturaCoverage::XMLParser parser(this->CTest, this->Coverage);
  parser.ParseFile(xmlFile);
  return true;
}
