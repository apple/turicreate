#include "cmParseMumpsCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include <map>
#include <string>
#include <utility>

cmParseMumpsCoverage::cmParseMumpsCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

cmParseMumpsCoverage::~cmParseMumpsCoverage()
{
}

bool cmParseMumpsCoverage::ReadCoverageFile(const char* file)
{
  // Read the gtm_coverage.mcov file, that has two lines of data:
  // packages:/full/path/to/Vista/Packages
  // coverage_dir:/full/path/to/dir/with/*.mcov
  cmsys::ifstream in(file);
  if (!in) {
    return false;
  }
  std::string line;
  while (cmSystemTools::GetLineFromStream(in, line)) {
    std::string::size_type pos = line.find(':', 0);
    std::string packages;
    if (pos != std::string::npos) {
      std::string type = line.substr(0, pos);
      std::string path = line.substr(pos + 1);
      if (type == "packages") {
        this->LoadPackages(path.c_str());
      } else if (type == "coverage_dir") {
        this->LoadCoverageData(path.c_str());
      } else {
        cmCTestLog(this->CTest, ERROR_MESSAGE,
                   "Parse Error in Mumps coverage file :\n"
                     << file << "\ntype: [" << type << "]\npath:[" << path
                     << "]\n"
                        "input line: ["
                     << line << "]\n");
      }
    }
  }
  return true;
}

void cmParseMumpsCoverage::InitializeMumpsFile(std::string& file)
{
  // initialize the coverage information for a given mumps file
  cmsys::ifstream in(file.c_str());
  if (!in) {
    return;
  }
  std::string line;
  cmCTestCoverageHandlerContainer::SingleFileCoverageVector& coverageVector =
    this->Coverage.TotalCoverage[file];
  if (!cmSystemTools::GetLineFromStream(in, line)) {
    return;
  }
  // first line of a .m file can never be run
  coverageVector.push_back(-1);
  while (cmSystemTools::GetLineFromStream(in, line)) {
    // putting in a 0 for a line means it is executable code
    // putting in a -1 for a line means it is not executable code
    int val = -1; // assume line is not executable
    bool found = false;
    std::string::size_type i = 0;
    // (1) Search for the first whitespace or semicolon character on a line.
    // This will skip over labels if the line starts with one, or will simply
    // be the first character on the line for non-label lines.
    for (; i < line.size(); ++i) {
      if (line[i] == ' ' || line[i] == '\t' || line[i] == ';') {
        found = true;
        break;
      }
    }
    if (found) {
      // (2) If the first character found above is whitespace or a period
      // then continue the search for the first following non-whitespace
      // character.
      if (line[i] == ' ' || line[i] == '\t') {
        while (i < line.size() &&
               (line[i] == ' ' || line[i] == '\t' || line[i] == '.')) {
          i++;
        }
      }
      // (3) If the character found is not a semicolon then the line counts for
      // coverage.
      if (i < line.size() && line[i] != ';') {
        val = 0;
      }
    }
    coverageVector.push_back(val);
  }
}

bool cmParseMumpsCoverage::LoadPackages(const char* d)
{
  cmsys::Glob glob;
  glob.RecurseOn();
  std::string pat = d;
  pat += "/*.m";
  glob.FindFiles(pat);
  for (std::string& file : glob.GetFiles()) {
    std::string name = cmSystemTools::GetFilenameName(file);
    this->RoutineToDirectory[name.substr(0, name.size() - 2)] = file;
    // initialize each file, this is left out until CDash is fixed
    // to handle large numbers of files
    this->InitializeMumpsFile(file);
  }
  return true;
}

bool cmParseMumpsCoverage::FindMumpsFile(std::string const& routine,
                                         std::string& filepath)
{
  std::map<std::string, std::string>::iterator i =
    this->RoutineToDirectory.find(routine);
  if (i != this->RoutineToDirectory.end()) {
    filepath = i->second;
    return true;
  }
  // try some alternate names
  const char* tryname[] = { "GUX", "GTM", "ONT", nullptr };
  for (int k = 0; tryname[k] != nullptr; k++) {
    std::string routine2 = routine + tryname[k];
    i = this->RoutineToDirectory.find(routine2);
    if (i != this->RoutineToDirectory.end()) {
      filepath = i->second;
      return true;
    }
  }
  return false;
}
