#include "cmParseCacheCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <utility>

cmParseCacheCoverage::cmParseCacheCoverage(
  cmCTestCoverageHandlerContainer& cont, cmCTest* ctest)
  : cmParseMumpsCoverage(cont, ctest)
{
}

bool cmParseCacheCoverage::LoadCoverageData(const char* d)
{
  // load all the .mcov files in the specified directory
  cmsys::Directory dir;
  if (!dir.Load(d)) {
    return false;
  }
  size_t numf;
  unsigned int i;
  numf = dir.GetNumberOfFiles();
  for (i = 0; i < numf; i++) {
    std::string file = dir.GetFile(i);
    if (file != "." && file != ".." && !cmSystemTools::FileIsDirectory(file)) {
      std::string path = d;
      path += "/";
      path += file;
      if (cmSystemTools::GetFilenameLastExtension(path) == ".cmcov") {
        if (!this->ReadCMCovFile(path.c_str())) {
          return false;
        }
      }
    }
  }
  return true;
}

// not currently used, but leave it in case we want it in the future
void cmParseCacheCoverage::RemoveUnCoveredFiles()
{
  // loop over the coverage data computed and remove all files
  // that only have -1 or 0 for the lines.
  cmCTestCoverageHandlerContainer::TotalCoverageMap::iterator ci =
    this->Coverage.TotalCoverage.begin();
  while (ci != this->Coverage.TotalCoverage.end()) {
    cmCTestCoverageHandlerContainer::SingleFileCoverageVector& v = ci->second;
    bool nothing = true;
    for (cmCTestCoverageHandlerContainer::SingleFileCoverageVector::iterator
           i = v.begin();
         i != v.end(); ++i) {
      if (*i > 0) {
        nothing = false;
        break;
      }
    }
    if (nothing) {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "No coverage found in: " << ci->first << std::endl,
                         this->Coverage.Quiet);
      this->Coverage.TotalCoverage.erase(ci++);
    } else {
      ++ci;
    }
  }
}

bool cmParseCacheCoverage::SplitString(std::vector<std::string>& args,
                                       std::string const& line)
{
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = line.find(',', 0);
  if (pos2 == std::string::npos) {
    return false;
  }
  std::string arg;
  while (pos2 != std::string::npos) {
    arg = line.substr(pos1, pos2 - pos1);
    args.push_back(arg);
    pos1 = pos2 + 1;
    pos2 = line.find(',', pos1);
  }
  arg = line.substr(pos1);
  args.push_back(arg);
  return true;
}

bool cmParseCacheCoverage::ReadCMCovFile(const char* file)
{
  cmsys::ifstream in(file);
  if (!in) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Can not open : " << file << "\n");
    return false;
  }
  std::string line;
  std::vector<std::string> separateLine;
  if (!cmSystemTools::GetLineFromStream(in, line)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Empty file : "
                 << file << "  referenced in this line of cmcov data:\n"
                            "["
                 << line << "]\n");
    return false;
  }
  separateLine.clear();
  this->SplitString(separateLine, line);
  if (separateLine.size() != 4 || separateLine[0] != "Routine" ||
      separateLine[1] != "Line" || separateLine[2] != "RtnLine" ||
      separateLine[3] != "Code") {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Bad first line of cmcov file : " << file << "  line:\n"
                                                            "["
                                                 << line << "]\n");
  }
  std::string routine;
  std::string filepath;
  while (cmSystemTools::GetLineFromStream(in, line)) {
    // clear out line argument vector
    separateLine.clear();
    // parse the comma separated line
    this->SplitString(separateLine, line);
    // might have more because code could have a quoted , in it
    // but we only care about the first 3 args anyway
    if (separateLine.size() < 4) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Bad line of cmcov file expected at least 4 found: "
                   << separateLine.size() << " " << file << "  line:\n"
                                                            "["
                   << line << "]\n");
      for (std::string::size_type i = 0; i < separateLine.size(); ++i) {
        cmCTestLog(this->CTest, ERROR_MESSAGE, "" << separateLine[1] << " ");
      }
      cmCTestLog(this->CTest, ERROR_MESSAGE, "\n");
      return false;
    }
    // if we do not have a routine yet, then it should be
    // the first argument in the vector
    if (routine.empty()) {
      routine = separateLine[0];
      // Find the full path to the file
      if (!this->FindMumpsFile(routine, filepath)) {
        cmCTestLog(this->CTest, ERROR_MESSAGE,
                   "Could not find mumps file for routine: " << routine
                                                             << "\n");
        filepath = "";
        continue; // move to next line
      }
    }
    // if we have a routine name, check for end of routine
    else {
      // Totals in arg 0 marks the end of a routine
      if (separateLine[0].substr(0, 6) == "Totals") {
        routine = ""; // at the end of this routine
        filepath = "";
        continue; // move to next line
      }
    }
    // if the file path was not found for the routine
    // move to next line. We should have already warned
    // after the call to FindMumpsFile that we did not find
    // it, so don't report again to cut down on output
    if (filepath.empty()) {
      continue;
    }
    // now we are ready to set the coverage from the line of data
    cmCTestCoverageHandlerContainer::SingleFileCoverageVector& coverageVector =
      this->Coverage.TotalCoverage[filepath];
    std::string::size_type linenumber = atoi(separateLine[1].c_str()) - 1;
    int count = atoi(separateLine[2].c_str());
    if (linenumber > coverageVector.size()) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Parse error line is greater than number of lines in file: "
                   << linenumber << " " << filepath << "\n");
      continue; // skip setting count to avoid crash
    }
    // now add to count for linenumber
    // for some reason the cache coverage adds extra lines to the
    // end of the file in some cases. Since they do not exist, we will
    // mark them as non executable
    while (linenumber >= coverageVector.size()) {
      coverageVector.push_back(-1);
    }
    // Accounts for lines that were previously marked
    // as non-executable code (-1). if the parser comes back with
    // a non-zero count, increase the count by 1 to push the line
    // into the executable code set in addition to the count found.
    if (coverageVector[linenumber] == -1 && count > 0) {
      coverageVector[linenumber] += count + 1;
    } else {
      coverageVector[linenumber] += count;
    }
  }
  return true;
}
