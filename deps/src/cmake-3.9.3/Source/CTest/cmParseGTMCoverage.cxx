#include "cmParseGTMCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

cmParseGTMCoverage::cmParseGTMCoverage(cmCTestCoverageHandlerContainer& cont,
                                       cmCTest* ctest)
  : cmParseMumpsCoverage(cont, ctest)
{
}

bool cmParseGTMCoverage::LoadCoverageData(const char* d)
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
      if (cmSystemTools::GetFilenameLastExtension(path) == ".mcov") {
        if (!this->ReadMCovFile(path.c_str())) {
          return false;
        }
      }
    }
  }
  return true;
}

bool cmParseGTMCoverage::ReadMCovFile(const char* file)
{
  cmsys::ifstream in(file);
  if (!in) {
    return false;
  }
  std::string line;
  std::string lastfunction;
  std::string lastroutine;
  std::string lastpath;
  int lastoffset = 0;
  while (cmSystemTools::GetLineFromStream(in, line)) {
    // only look at lines that have coverage data
    if (line.find("^ZZCOVERAGE") == std::string::npos) {
      continue;
    }
    std::string filepath;
    std::string function;
    std::string routine;
    int linenumber = 0;
    int count = 0;
    this->ParseMCOVLine(line, routine, function, linenumber, count);
    // skip this one
    if (routine == "RSEL") {
      continue;
    }
    // no need to search the file if we just did it
    if (function == lastfunction && lastroutine == routine) {
      if (!lastpath.empty()) {
        this->Coverage.TotalCoverage[lastpath][lastoffset + linenumber] +=
          count;
      } else {
        cmCTestLog(this->CTest, ERROR_MESSAGE, "Can not find mumps file : "
                     << lastroutine
                     << "  referenced in this line of mcov data:\n"
                        "["
                     << line << "]\n");
      }
      continue;
    }
    // Find the full path to the file
    bool found = this->FindMumpsFile(routine, filepath);
    if (found) {
      int lineoffset = 0;
      if (this->FindFunctionInMumpsFile(filepath, function, lineoffset)) {
        cmCTestCoverageHandlerContainer::SingleFileCoverageVector&
          coverageVector = this->Coverage.TotalCoverage[filepath];
        // This section accounts for lines that were previously marked
        // as non-executable code (-1), if the parser comes back with
        // a non-zero count, increase the count by 1 to push the line
        // into the executable code set in addtion to the count found.
        if (coverageVector[lineoffset + linenumber] == -1 && count > 0) {
          coverageVector[lineoffset + linenumber] += count + 1;
        } else {
          coverageVector[lineoffset + linenumber] += count;
        }
        lastoffset = lineoffset;
      }
    } else {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "Can not find mumps file : "
                   << routine << "  referenced in this line of mcov data:\n"
                                 "["
                   << line << "]\n");
    }
    lastfunction = function;
    lastroutine = routine;
    lastpath = filepath;
  }
  return true;
}

bool cmParseGTMCoverage::FindFunctionInMumpsFile(std::string const& filepath,
                                                 std::string const& function,
                                                 int& lineoffset)
{
  cmsys::ifstream in(filepath.c_str());
  if (!in) {
    return false;
  }
  std::string line;
  int linenum = 0;
  while (cmSystemTools::GetLineFromStream(in, line)) {
    std::string::size_type pos = line.find(function);
    if (pos == 0) {
      char nextchar = line[function.size()];
      if (nextchar == ' ' || nextchar == '(' || nextchar == '\t') {
        lineoffset = linenum;
        return true;
      }
    }
    if (pos == 1) {
      char prevchar = line[0];
      char nextchar = line[function.size() + 1];
      if (prevchar == '%' && (nextchar == ' ' || nextchar == '(')) {
        lineoffset = linenum;
        return true;
      }
    }
    linenum++; // move to next line count
  }
  lineoffset = 0;
  cmCTestLog(this->CTest, ERROR_MESSAGE, "Could not find entry point : "
               << function << " in " << filepath << "\n");
  return false;
}

bool cmParseGTMCoverage::ParseMCOVLine(std::string const& line,
                                       std::string& routine,
                                       std::string& function, int& linenumber,
                                       int& count)
{
  // this method parses lines from the .mcov file
  // each line has ^COVERAGE(...) in it, and there
  // are several varients of coverage lines:
  //
  // ^COVERAGE("DIC11","PR1",0)="2:0:0:0"
  //          ( file  , entry, line ) = "number_executed:timing_info"
  // ^COVERAGE("%RSEL","SRC")="1:0:0:0"
  //          ( file  , entry ) = "number_executed:timing_info"
  // ^COVERAGE("%RSEL","init",8,"FOR_LOOP",1)=1
  //          ( file  , entry, line, IGNORE ) =number_executed
  std::vector<std::string> args;
  std::string::size_type pos = line.find('(', 0);
  // if no ( is found, then return line has no coverage
  if (pos == std::string::npos) {
    return false;
  }
  std::string arg;
  bool done = false;
  // separate out all of the comma separated arguments found
  // in the COVERAGE(...) line
  while (line[pos] && !done) {
    // save the char we are looking at
    char cur = line[pos];
    // , or ) means end of argument
    if (cur == ',' || cur == ')') {
      // save the argument into the argument vector
      args.push_back(arg);
      // start on a new argument
      arg = "";
      // if we are at the end of the ), then finish while loop
      if (cur == ')') {
        done = true;
      }
    } else {
      // all chars except ", (, and % get stored in the arg string
      if (cur != '\"' && cur != '(' && cur != '%') {
        arg.append(1, line[pos]);
      }
    }
    // move to next char
    pos++;
  }
  // now parse the right hand side of the =
  pos = line.find('=');
  // no = found, this is an error
  if (pos == std::string::npos) {
    return false;
  }
  pos++; // move past =

  // if the next positing is not a ", then this is a
  // COVERAGE(..)=count line and turn the rest of the string
  // past the = into an integer and set it to count
  if (line[pos] != '\"') {
    count = atoi(line.substr(pos).c_str());
  } else {
    // this means line[pos] is a ", and we have a
    // COVERAGE(...)="1:0:0:0" type of line
    pos++; // move past "
    // find the first : past the "
    std::string::size_type pos2 = line.find(':', pos);
    // turn the string between the " and the first : into an integer
    // and set it to count
    count = atoi(line.substr(pos, pos2 - pos).c_str());
  }
  // less then two arguments is an error
  if (args.size() < 2) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Error parsing mcov line: ["
                 << line << "]\n");
    return false;
  }
  routine = args[0];  // the routine is the first argument
  function = args[1]; // the function in the routine is the second
  // in the two argument only format
  // ^COVERAGE("%RSEL","SRC"), the line offset is 0
  if (args.size() == 2) {
    // To avoid double counting of line 0 of each entry point,
    // Don't count the lines that do not give an explicit line
    // number.
    routine = "";
    function = "";
  } else {
    // this is the format for this line
    // ^COVERAGE("%RSEL","SRC",count)
    linenumber = atoi(args[2].c_str());
  }
  return true;
}
