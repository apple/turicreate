#include "cmParsePHPCoverage.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"
#include "cmSystemTools.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include <stdlib.h>
#include <string.h>

/*
  To setup coverage for php.

  - edit php.ini to add auto prepend and append php files from phpunit
  auto_prepend_file =
  auto_append_file =
  - run the tests
  - run this program on all the files in c:/tmp

*/

cmParsePHPCoverage::cmParsePHPCoverage(cmCTestCoverageHandlerContainer& cont,
                                       cmCTest* ctest)
  : Coverage(cont)
  , CTest(ctest)
{
}

bool cmParsePHPCoverage::ReadUntil(std::istream& in, char until)
{
  char c = 0;
  while (in.get(c) && c != until) {
  }
  return c == until;
}
bool cmParsePHPCoverage::ReadCoverageArray(std::istream& in,
                                           std::string const& fileName)
{
  cmCTestCoverageHandlerContainer::SingleFileCoverageVector& coverageVector =
    this->Coverage.TotalCoverage[fileName];

  char c;
  char buf[4];
  in.read(buf, 3);
  buf[3] = 0;
  if (strcmp(buf, ";a:") != 0) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "failed to read start of coverage array, found : " << buf
                                                                  << "\n");
    return false;
  }
  int size = 0;
  if (!this->ReadInt(in, size)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "failed to read size ");
    return false;
  }
  if (!in.get(c) && c == '{') {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "failed to read open {\n");
    return false;
  }
  for (int i = 0; i < size; i++) {
    this->ReadUntil(in, ':');
    int line = 0;
    this->ReadInt(in, line);
    // ok xdebug may have a bug here
    // it seems to be 1 based but often times
    // seems to have a 0'th line.
    line--;
    if (line < 0) {
      line = 0;
    }
    this->ReadUntil(in, ':');
    int value = 0;
    this->ReadInt(in, value);
    // make sure the vector is the right size and is
    // initialized with -1 for each line
    while (coverageVector.size() <= static_cast<size_t>(line)) {
      coverageVector.push_back(-1);
    }
    // if value is less than 0, set it to zero
    // TODO figure out the difference between
    // -1 and -2 in xdebug coverage??  For now
    // assume less than 0 is just not covered
    // CDash expects -1 for non executable code (like comments)
    // and 0 for uncovered code, and a positive value
    // for number of times a line was executed
    if (value < 0) {
      value = 0;
    }
    // if unset then set it to value
    if (coverageVector[line] == -1) {
      coverageVector[line] = value;
    }
    // otherwise increment by value
    else {
      coverageVector[line] += value;
    }
  }
  return true;
}

bool cmParsePHPCoverage::ReadInt(std::istream& in, int& v)
{
  std::string s;
  char c = 0;
  while (in.get(c) && c != ':' && c != ';') {
    s += c;
  }
  v = atoi(s.c_str());
  return true;
}

bool cmParsePHPCoverage::ReadArraySize(std::istream& in, int& size)
{
  char c = 0;
  in.get(c);
  if (c != 'a') {
    return false;
  }
  if (in.get(c) && c == ':') {
    if (this->ReadInt(in, size)) {
      return true;
    }
  }
  return false;
}

bool cmParsePHPCoverage::ReadFileInformation(std::istream& in)
{
  char buf[4];
  in.read(buf, 2);
  buf[2] = 0;
  if (strcmp(buf, "s:") != 0) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "failed to read start of file info found: [" << buf << "]\n");
    return false;
  }
  char c;
  int size = 0;
  if (this->ReadInt(in, size)) {
    size++; // add one for null termination
    char* s = new char[size + 1];
    // read open quote
    if (in.get(c) && c != '"') {
      delete[] s;
      return false;
    }
    // read the string data
    in.read(s, size - 1);
    s[size - 1] = 0;
    std::string fileName = s;
    delete[] s;
    // read close quote
    if (in.get(c) && c != '"') {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "failed to read close quote\n"
                   << "read [" << c << "]\n");
      return false;
    }
    if (!this->ReadCoverageArray(in, fileName)) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "failed to read coverage array for file: " << fileName
                                                            << "\n");
      return false;
    }
    return true;
  }
  return false;
}

bool cmParsePHPCoverage::ReadPHPData(const char* file)
{
  cmsys::ifstream in(file);
  if (!in) {
    return false;
  }
  int size = 0;
  this->ReadArraySize(in, size);
  char c = 0;
  in.get(c);
  if (c != '{') {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "failed to read open array\n");
    return false;
  }
  for (int i = 0; i < size; i++) {
    if (!this->ReadFileInformation(in)) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "Failed to read file #" << i
                                                                     << "\n");
      return false;
    }
    in.get(c);
    if (c != '}') {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "failed to read close array\n");
      return false;
    }
  }
  return true;
}

bool cmParsePHPCoverage::ReadPHPCoverageDirectory(const char* d)
{
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
      if (!this->ReadPHPData(path.c_str())) {
        return false;
      }
    }
  }
  return true;
}
