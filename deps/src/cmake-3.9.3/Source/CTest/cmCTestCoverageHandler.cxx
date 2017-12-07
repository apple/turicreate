/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestCoverageHandler.h"

#include "cmCTest.h"
#include "cmGeneratedFileStream.h"
#include "cmParseBlanketJSCoverage.h"
#include "cmParseCacheCoverage.h"
#include "cmParseCoberturaCoverage.h"
#include "cmParseDelphiCoverage.h"
#include "cmParseGTMCoverage.h"
#include "cmParseJacocoCoverage.h"
#include "cmParsePHPCoverage.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"
#include "cmXMLWriter.h"
#include "cmake.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/Process.h"
#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <utility>

class cmMakefile;

#define SAFEDIV(x, y) (((y) != 0) ? ((x) / (y)) : (0))

class cmCTestRunProcess
{
public:
  cmCTestRunProcess()
  {
    this->Process = cmsysProcess_New();
    this->PipeState = -1;
    this->TimeOut = -1;
  }
  ~cmCTestRunProcess()
  {
    if (!(this->PipeState == -1) &&
        !(this->PipeState == cmsysProcess_Pipe_None) &&
        !(this->PipeState == cmsysProcess_Pipe_Timeout)) {
      this->WaitForExit();
    }
    cmsysProcess_Delete(this->Process);
  }
  void SetCommand(const char* command)
  {
    this->CommandLineStrings.clear();
    this->CommandLineStrings.push_back(command);
    ;
  }
  void AddArgument(const char* arg)
  {
    if (arg) {
      this->CommandLineStrings.push_back(arg);
    }
  }
  void SetWorkingDirectory(const char* dir) { this->WorkingDirectory = dir; }
  void SetTimeout(double t) { this->TimeOut = t; }
  bool StartProcess()
  {
    std::vector<const char*> args;
    for (std::vector<std::string>::iterator i =
           this->CommandLineStrings.begin();
         i != this->CommandLineStrings.end(); ++i) {
      args.push_back(i->c_str());
    }
    args.push_back(CM_NULLPTR); // null terminate
    cmsysProcess_SetCommand(this->Process, &*args.begin());
    if (!this->WorkingDirectory.empty()) {
      cmsysProcess_SetWorkingDirectory(this->Process,
                                       this->WorkingDirectory.c_str());
    }

    cmsysProcess_SetOption(this->Process, cmsysProcess_Option_HideWindow, 1);
    if (this->TimeOut != -1) {
      cmsysProcess_SetTimeout(this->Process, this->TimeOut);
    }
    cmsysProcess_Execute(this->Process);
    this->PipeState = cmsysProcess_GetState(this->Process);
    // if the process is running or exited return true
    return this->PipeState == cmsysProcess_State_Executing ||
      this->PipeState == cmsysProcess_State_Exited;
  }
  void SetStdoutFile(const char* fname)
  {
    cmsysProcess_SetPipeFile(this->Process, cmsysProcess_Pipe_STDOUT, fname);
  }
  void SetStderrFile(const char* fname)
  {
    cmsysProcess_SetPipeFile(this->Process, cmsysProcess_Pipe_STDERR, fname);
  }
  int WaitForExit(double* timeout = CM_NULLPTR)
  {
    this->PipeState = cmsysProcess_WaitForExit(this->Process, timeout);
    return this->PipeState;
  }
  int GetProcessState() { return this->PipeState; }

private:
  int PipeState;
  cmsysProcess* Process;
  std::vector<std::string> CommandLineStrings;
  std::string WorkingDirectory;
  double TimeOut;
};

cmCTestCoverageHandler::cmCTestCoverageHandler()
{
}

void cmCTestCoverageHandler::Initialize()
{
  this->Superclass::Initialize();
  this->CustomCoverageExclude.clear();
  this->SourceLabels.clear();
  this->TargetDirs.clear();
  this->LabelIdMap.clear();
  this->Labels.clear();
  this->LabelFilter.clear();
}

void cmCTestCoverageHandler::CleanCoverageLogFiles(std::ostream& log)
{
  std::string logGlob = this->CTest->GetCTestConfiguration("BuildDirectory");
  logGlob += "/Testing/";
  logGlob += this->CTest->GetCurrentTag();
  logGlob += "/CoverageLog*";
  cmsys::Glob gl;
  gl.FindFiles(logGlob);
  std::vector<std::string> const& files = gl.GetFiles();
  for (std::vector<std::string>::const_iterator fi = files.begin();
       fi != files.end(); ++fi) {
    log << "Removing old coverage log: " << *fi << "\n";
    cmSystemTools::RemoveFile(*fi);
  }
}

bool cmCTestCoverageHandler::StartCoverageLogFile(
  cmGeneratedFileStream& covLogFile, int logFileCount)
{
  char covLogFilename[1024];
  sprintf(covLogFilename, "CoverageLog-%d", logFileCount);
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Open file: " << covLogFilename << std::endl,
                     this->Quiet);
  if (!this->StartResultingXML(cmCTest::PartCoverage, covLogFilename,
                               covLogFile)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot open log file: " << covLogFilename << std::endl);
    return false;
  }
  return true;
}

void cmCTestCoverageHandler::EndCoverageLogFile(cmGeneratedFileStream& ostr,
                                                int logFileCount)
{
  char covLogFilename[1024];
  sprintf(covLogFilename, "CoverageLog-%d.xml", logFileCount);
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Close file: " << covLogFilename << std::endl,
                     this->Quiet);
  ostr.Close();
}

void cmCTestCoverageHandler::StartCoverageLogXML(cmXMLWriter& xml)
{
  this->CTest->StartXML(xml, this->AppendXML);
  xml.StartElement("CoverageLog");
  xml.Element("StartDateTime", this->CTest->CurrentTime());
  xml.Element("StartTime",
              static_cast<unsigned int>(cmSystemTools::GetTime()));
}

void cmCTestCoverageHandler::EndCoverageLogXML(cmXMLWriter& xml)
{
  xml.Element("EndDateTime", this->CTest->CurrentTime());
  xml.Element("EndTime", static_cast<unsigned int>(cmSystemTools::GetTime()));
  xml.EndElement(); // CoverageLog
  this->CTest->EndXML(xml);
}

bool cmCTestCoverageHandler::ShouldIDoCoverage(const char* file,
                                               const char* srcDir,
                                               const char* binDir)
{
  if (this->IsFilteredOut(file)) {
    return false;
  }

  std::vector<cmsys::RegularExpression>::iterator sit;
  for (sit = this->CustomCoverageExcludeRegex.begin();
       sit != this->CustomCoverageExcludeRegex.end(); ++sit) {
    if (sit->find(file)) {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "  File "
                           << file << " is excluded in CTestCustom.ctest"
                           << std::endl;
                         , this->Quiet);
      return false;
    }
  }

  std::string fSrcDir = cmSystemTools::CollapseFullPath(srcDir);
  std::string fBinDir = cmSystemTools::CollapseFullPath(binDir);
  std::string fFile = cmSystemTools::CollapseFullPath(file);
  bool sourceSubDir = cmSystemTools::IsSubDirectory(fFile, fSrcDir);
  bool buildSubDir = cmSystemTools::IsSubDirectory(fFile, fBinDir);
  // Always check parent directory of the file.
  std::string fileDir = cmSystemTools::GetFilenamePath(fFile);
  std::string checkDir;

  // We also need to check the binary/source directory pair.
  if (sourceSubDir && buildSubDir) {
    if (fSrcDir.size() > fBinDir.size()) {
      checkDir = fSrcDir;
    } else {
      checkDir = fBinDir;
    }
  } else if (sourceSubDir) {
    checkDir = fSrcDir;
  } else if (buildSubDir) {
    checkDir = fBinDir;
  }
  std::string ndc = cmSystemTools::FileExistsInParentDirectories(
    ".NoDartCoverage", fFile.c_str(), checkDir.c_str());
  if (!ndc.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Found: " << ndc << " so skip coverage of " << file
                                 << std::endl,
                       this->Quiet);
    return false;
  }

  // By now checkDir should be set to parent directory of the file.
  // Get the relative path to the file an apply it to the opposite directory.
  // If it is the same as fileDir, then ignore, otherwise check.
  std::string relPath;
  if (!checkDir.empty()) {
    relPath = cmSystemTools::RelativePath(checkDir.c_str(), fFile.c_str());
  } else {
    relPath = fFile;
  }
  if (checkDir == fSrcDir) {
    checkDir = fBinDir;
  } else {
    checkDir = fSrcDir;
  }
  fFile = checkDir + "/" + relPath;
  fFile = cmSystemTools::GetFilenamePath(fFile);

  if (fileDir == fFile) {
    // This is in-source build, so we trust the previous check.
    return true;
  }

  ndc = cmSystemTools::FileExistsInParentDirectories(
    ".NoDartCoverage", fFile.c_str(), checkDir.c_str());
  if (!ndc.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Found: " << ndc << " so skip coverage of: " << file
                                 << std::endl,
                       this->Quiet);
    return false;
  }
  // Ok, nothing in source tree, nothing in binary tree
  return true;
}

// clearly it would be nice if this were broken up into a few smaller
// functions and commented...
int cmCTestCoverageHandler::ProcessHandler()
{
  this->CTest->ClearSubmitFiles(cmCTest::PartCoverage);
  int error = 0;
  // do we have time for this
  if (this->CTest->GetRemainingTimeAllowed() < 120) {
    return error;
  }

  std::string coverage_start_time = this->CTest->CurrentTime();
  unsigned int coverage_start_time_time =
    static_cast<unsigned int>(cmSystemTools::GetTime());
  std::string sourceDir =
    this->CTest->GetCTestConfiguration("SourceDirectory");
  std::string binaryDir = this->CTest->GetCTestConfiguration("BuildDirectory");

  this->LoadLabels();

  cmGeneratedFileStream ofs;
  double elapsed_time_start = cmSystemTools::GetTime();
  if (!this->StartLogFile("Coverage", ofs)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot create LastCoverage.log file" << std::endl);
  }

  ofs << "Performing coverage: " << elapsed_time_start << std::endl;
  this->CleanCoverageLogFiles(ofs);

  cmSystemTools::ConvertToUnixSlashes(sourceDir);
  cmSystemTools::ConvertToUnixSlashes(binaryDir);

  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                     "Performing coverage" << std::endl, this->Quiet);

  cmCTestCoverageHandlerContainer cont;
  cont.Error = error;
  cont.SourceDir = sourceDir;
  cont.BinaryDir = binaryDir;
  cont.OFS = &ofs;
  cont.Quiet = this->Quiet;

  // setup the regex exclude stuff
  this->CustomCoverageExcludeRegex.clear();
  std::vector<std::string>::iterator rexIt;
  for (rexIt = this->CustomCoverageExclude.begin();
       rexIt != this->CustomCoverageExclude.end(); ++rexIt) {
    this->CustomCoverageExcludeRegex.push_back(
      cmsys::RegularExpression(rexIt->c_str()));
  }

  if (this->HandleBullseyeCoverage(&cont)) {
    return cont.Error;
  }
  int file_count = 0;
  file_count += this->HandleGCovCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }
  file_count += this->HandleLCovCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }
  file_count += this->HandleTracePyCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }
  file_count += this->HandlePHPCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }
  file_count += this->HandleCoberturaCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }

  file_count += this->HandleMumpsCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }

  file_count += this->HandleJacocoCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }

  file_count += this->HandleBlanketJSCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }

  file_count += this->HandleDelphiCoverage(&cont);
  error = cont.Error;
  if (file_count < 0) {
    return error;
  }
  std::set<std::string> uncovered = this->FindUncoveredFiles(&cont);

  if (file_count == 0 && this->ExtraCoverageGlobs.empty()) {
    cmCTestOptionalLog(
      this->CTest, WARNING,
      " Cannot find any coverage files. Ignoring Coverage request."
        << std::endl,
      this->Quiet);
    return error;
  }
  cmGeneratedFileStream covSumFile;
  cmGeneratedFileStream covLogFile;
  cmXMLWriter covSumXML(covSumFile);
  cmXMLWriter covLogXML(covLogFile);

  if (!this->StartResultingXML(cmCTest::PartCoverage, "Coverage",
                               covSumFile)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot open coverage summary file."
                 << std::endl);
    return -1;
  }
  covSumFile.setf(std::ios::fixed, std::ios::floatfield);
  covSumFile.precision(2);

  this->CTest->StartXML(covSumXML, this->AppendXML);
  // Produce output xml files

  covSumXML.StartElement("Coverage");
  covSumXML.Element("StartDateTime", coverage_start_time);
  covSumXML.Element("StartTime", coverage_start_time_time);
  int logFileCount = 0;
  if (!this->StartCoverageLogFile(covLogFile, logFileCount)) {
    return -1;
  }
  this->StartCoverageLogXML(covLogXML);
  cmCTestCoverageHandlerContainer::TotalCoverageMap::iterator fileIterator;
  int cnt = 0;
  long total_tested = 0;
  long total_untested = 0;
  // std::string fullSourceDir = sourceDir + "/";
  // std::string fullBinaryDir = binaryDir + "/";
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, std::endl, this->Quiet);
  cmCTestOptionalLog(
    this->CTest, HANDLER_OUTPUT,
    "   Accumulating results (each . represents one file):" << std::endl,
    this->Quiet);
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);

  std::vector<std::string> errorsWhileAccumulating;

  file_count = 0;
  for (fileIterator = cont.TotalCoverage.begin();
       fileIterator != cont.TotalCoverage.end(); ++fileIterator) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "." << std::flush,
                       this->Quiet);
    file_count++;
    if (file_count % 50 == 0) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, " processed: "
                           << file_count << " out of "
                           << cont.TotalCoverage.size() << std::endl,
                         this->Quiet);
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);
    }

    const std::string fullFileName = fileIterator->first;
    bool shouldIDoCoverage = this->ShouldIDoCoverage(
      fullFileName.c_str(), sourceDir.c_str(), binaryDir.c_str());
    if (!shouldIDoCoverage) {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         ".NoDartCoverage found, so skip coverage check for: "
                           << fullFileName << std::endl,
                         this->Quiet);
      continue;
    }

    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Process file: " << fullFileName << std::endl,
                       this->Quiet);

    if (!cmSystemTools::FileExists(fullFileName.c_str())) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Cannot find file: " << fullFileName << std::endl);
      continue;
    }

    if (++cnt % 100 == 0) {
      this->EndCoverageLogXML(covLogXML);
      this->EndCoverageLogFile(covLogFile, logFileCount);
      logFileCount++;
      if (!this->StartCoverageLogFile(covLogFile, logFileCount)) {
        return -1;
      }
      this->StartCoverageLogXML(covLogXML);
    }

    const std::string fileName = cmSystemTools::GetFilenameName(fullFileName);
    std::string shortFileName =
      this->CTest->GetShortPathToFile(fullFileName.c_str());
    const cmCTestCoverageHandlerContainer::SingleFileCoverageVector& fcov =
      fileIterator->second;
    covLogXML.StartElement("File");
    covLogXML.Attribute("Name", fileName);
    covLogXML.Attribute("FullPath", shortFileName);
    covLogXML.StartElement("Report");

    cmsys::ifstream ifs(fullFileName.c_str());
    if (!ifs) {
      std::ostringstream ostr;
      ostr << "Cannot open source file: " << fullFileName;
      errorsWhileAccumulating.push_back(ostr.str());
      error++;
      continue;
    }

    int tested = 0;
    int untested = 0;

    cmCTestCoverageHandlerContainer::SingleFileCoverageVector::size_type cc;
    std::string line;
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Actually performing coverage for: " << fullFileName
                                                            << std::endl,
                       this->Quiet);
    for (cc = 0; cc < fcov.size(); cc++) {
      if (!cmSystemTools::GetLineFromStream(ifs, line) &&
          cc != fcov.size() - 1) {
        std::ostringstream ostr;
        ostr << "Problem reading source file: " << fullFileName
             << " line:" << cc << "  out total: " << fcov.size() - 1;
        errorsWhileAccumulating.push_back(ostr.str());
        error++;
        break;
      }
      covLogXML.StartElement("Line");
      covLogXML.Attribute("Number", cc);
      covLogXML.Attribute("Count", fcov[cc]);
      covLogXML.Content(line);
      covLogXML.EndElement(); // Line
      if (fcov[cc] == 0) {
        untested++;
      } else if (fcov[cc] > 0) {
        tested++;
      }
    }
    if (cmSystemTools::GetLineFromStream(ifs, line)) {
      std::ostringstream ostr;
      ostr << "Looks like there are more lines in the file: " << fullFileName;
      errorsWhileAccumulating.push_back(ostr.str());
    }
    float cper = 0;
    float cmet = 0;
    if (tested + untested > 0) {
      cper = (100 * SAFEDIV(static_cast<float>(tested),
                            static_cast<float>(tested + untested)));
      cmet = (SAFEDIV(static_cast<float>(tested + 10),
                      static_cast<float>(tested + untested + 10)));
    }
    total_tested += tested;
    total_untested += untested;
    covLogXML.EndElement(); // Report
    covLogXML.EndElement(); // File
    covSumXML.StartElement("File");
    covSumXML.Attribute("Name", fileName);
    covSumXML.Attribute("FullPath",
                        this->CTest->GetShortPathToFile(fullFileName.c_str()));
    covSumXML.Attribute("Covered", tested + untested > 0 ? "true" : "false");
    covSumXML.Element("LOCTested", tested);
    covSumXML.Element("LOCUnTested", untested);
    covSumXML.Element("PercentCoverage", cper);
    covSumXML.Element("CoverageMetric", cmet);
    this->WriteXMLLabels(covSumXML, shortFileName);
    covSumXML.EndElement(); // File
  }

  // Handle all the files in the extra coverage globs that have no cov data
  for (std::set<std::string>::iterator i = uncovered.begin();
       i != uncovered.end(); ++i) {
    std::string fileName = cmSystemTools::GetFilenameName(*i);
    std::string fullPath = cont.SourceDir + "/" + *i;

    covLogXML.StartElement("File");
    covLogXML.Attribute("Name", fileName);
    covLogXML.Attribute("FullPath", *i);
    covLogXML.StartElement("Report");

    cmsys::ifstream ifs(fullPath.c_str());
    if (!ifs) {
      std::ostringstream ostr;
      ostr << "Cannot open source file: " << fullPath;
      errorsWhileAccumulating.push_back(ostr.str());
      error++;
      covLogXML.EndElement(); // Report
      covLogXML.EndElement(); // File
      continue;
    }
    int untested = 0;
    std::string line;
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Actually performing coverage for: " << *i << std::endl,
                       this->Quiet);
    while (cmSystemTools::GetLineFromStream(ifs, line)) {
      covLogXML.StartElement("Line");
      covLogXML.Attribute("Number", untested);
      covLogXML.Attribute("Count", 0);
      covLogXML.Content(line);
      covLogXML.EndElement(); // Line
      untested++;
    }
    covLogXML.EndElement(); // Report
    covLogXML.EndElement(); // File

    total_untested += untested;
    covSumXML.StartElement("File");
    covSumXML.Attribute("Name", fileName);
    covSumXML.Attribute("FullPath", *i);
    covSumXML.Attribute("Covered", "true");
    covSumXML.Element("LOCTested", 0);
    covSumXML.Element("LOCUnTested", untested);
    covSumXML.Element("PercentCoverage", 0);
    covSumXML.Element("CoverageMetric", 0);
    this->WriteXMLLabels(covSumXML, *i);
    covSumXML.EndElement(); // File
  }

  this->EndCoverageLogXML(covLogXML);
  this->EndCoverageLogFile(covLogFile, logFileCount);

  if (!errorsWhileAccumulating.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, std::endl);
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error(s) while accumulating results:" << std::endl);
    std::vector<std::string>::iterator erIt;
    for (erIt = errorsWhileAccumulating.begin();
         erIt != errorsWhileAccumulating.end(); ++erIt) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "  " << *erIt << std::endl);
    }
  }

  long total_lines = total_tested + total_untested;
  float percent_coverage = 100 *
    SAFEDIV(static_cast<float>(total_tested), static_cast<float>(total_lines));
  if (total_lines == 0) {
    percent_coverage = 0;
  }

  std::string end_time = this->CTest->CurrentTime();

  covSumXML.Element("LOCTested", total_tested);
  covSumXML.Element("LOCUntested", total_untested);
  covSumXML.Element("LOC", total_lines);
  covSumXML.Element("PercentCoverage", percent_coverage);
  covSumXML.Element("EndDateTime", end_time);
  covSumXML.Element("EndTime",
                    static_cast<unsigned int>(cmSystemTools::GetTime()));
  covSumXML.Element(
    "ElapsedMinutes",
    static_cast<int>((cmSystemTools::GetTime() - elapsed_time_start) / 6) /
      10.0);
  covSumXML.EndElement(); // Coverage
  this->CTest->EndXML(covSumXML);

  cmCTestLog(this->CTest, HANDLER_OUTPUT, ""
               << std::endl
               << "\tCovered LOC:         " << total_tested << std::endl
               << "\tNot covered LOC:     " << total_untested << std::endl
               << "\tTotal LOC:           " << total_lines << std::endl
               << "\tPercentage Coverage: "
               << std::setiosflags(std::ios::fixed) << std::setprecision(2)
               << (percent_coverage) << "%" << std::endl);

  ofs << "\tCovered LOC:         " << total_tested << std::endl
      << "\tNot covered LOC:     " << total_untested << std::endl
      << "\tTotal LOC:           " << total_lines << std::endl
      << "\tPercentage Coverage: " << std::setiosflags(std::ios::fixed)
      << std::setprecision(2) << (percent_coverage) << "%" << std::endl;

  if (error) {
    return -1;
  }
  return 0;
}

void cmCTestCoverageHandler::PopulateCustomVectors(cmMakefile* mf)
{
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " Add coverage exclude regular expressions." << std::endl,
                     this->Quiet);
  this->CTest->PopulateCustomVector(mf, "CTEST_CUSTOM_COVERAGE_EXCLUDE",
                                    this->CustomCoverageExclude);
  this->CTest->PopulateCustomVector(mf, "CTEST_EXTRA_COVERAGE_GLOB",
                                    this->ExtraCoverageGlobs);
  std::vector<std::string>::iterator it;
  for (it = this->CustomCoverageExclude.begin();
       it != this->CustomCoverageExclude.end(); ++it) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Add coverage exclude: " << *it << std::endl,
                       this->Quiet);
  }
  for (it = this->ExtraCoverageGlobs.begin();
       it != this->ExtraCoverageGlobs.end(); ++it) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Add coverage glob: " << *it << std::endl,
                       this->Quiet);
  }
}

// Fix for issue #4971 where the case of the drive letter component of
// the filenames might be different when analyzing gcov output.
//
// Compare file names: fnc(fn1) == fnc(fn2) // fnc == file name compare
//
#ifdef _WIN32
#define fnc(s) cmSystemTools::LowerCase(s)
#else
#define fnc(s) s
#endif

bool IsFileInDir(const std::string& infile, const std::string& indir)
{
  std::string file = cmSystemTools::CollapseFullPath(infile);
  std::string dir = cmSystemTools::CollapseFullPath(indir);

  return file.size() > dir.size() &&
    fnc(file.substr(0, dir.size())) == fnc(dir) && file[dir.size()] == '/';
}

int cmCTestCoverageHandler::HandlePHPCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmParsePHPCoverage cov(*cont, this->CTest);
  std::string coverageDir = this->CTest->GetBinaryDir() + "/xdebugCoverage";
  if (cmSystemTools::FileIsDirectory(coverageDir)) {
    cov.ReadPHPCoverageDirectory(coverageDir.c_str());
  }
  return static_cast<int>(cont->TotalCoverage.size());
}

int cmCTestCoverageHandler::HandleCoberturaCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmParseCoberturaCoverage cov(*cont, this->CTest);

  // Assume the coverage.xml is in the binary directory
  // check for the COBERTURADIR environment variable,
  // if it doesn't exist or is empty, assume the
  // binary directory is used.
  std::string coverageXMLFile;
  if (!cmSystemTools::GetEnv("COBERTURADIR", coverageXMLFile) ||
      coverageXMLFile.empty()) {
    coverageXMLFile = this->CTest->GetBinaryDir();
  }
  // build the find file string with the directory from above
  coverageXMLFile += "/coverage.xml";

  if (cmSystemTools::FileExists(coverageXMLFile.c_str())) {
    // If file exists, parse it
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Parsing Cobertura XML file: " << coverageXMLFile
                                                      << std::endl,
                       this->Quiet);
    cov.ReadCoverageXML(coverageXMLFile.c_str());
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find Cobertura XML file: " << coverageXMLFile
                                                           << std::endl,
                       this->Quiet);
  }
  return static_cast<int>(cont->TotalCoverage.size());
}

int cmCTestCoverageHandler::HandleMumpsCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  // try gtm coverage
  cmParseGTMCoverage cov(*cont, this->CTest);
  std::string coverageFile =
    this->CTest->GetBinaryDir() + "/gtm_coverage.mcov";
  if (cmSystemTools::FileExists(coverageFile.c_str())) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Parsing Cache Coverage: " << coverageFile << std::endl,
                       this->Quiet);
    cov.ReadCoverageFile(coverageFile.c_str());
    return static_cast<int>(cont->TotalCoverage.size());
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " Cannot find GTM coverage file: " << coverageFile
                                                        << std::endl,
                     this->Quiet);
  cmParseCacheCoverage ccov(*cont, this->CTest);
  coverageFile = this->CTest->GetBinaryDir() + "/cache_coverage.cmcov";
  if (cmSystemTools::FileExists(coverageFile.c_str())) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Parsing Cache Coverage: " << coverageFile << std::endl,
                       this->Quiet);
    ccov.ReadCoverageFile(coverageFile.c_str());
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find Cache coverage file: " << coverageFile
                                                            << std::endl,
                       this->Quiet);
  }
  return static_cast<int>(cont->TotalCoverage.size());
}

struct cmCTestCoverageHandlerLocale
{
  cmCTestCoverageHandlerLocale()
  {
    std::string l;
    if (cmSystemTools::GetEnv("LC_ALL", l)) {
      lc_all = l;
    }
    if (lc_all != "C") {
      cmSystemTools::PutEnv("LC_ALL=C");
    }
  }
  ~cmCTestCoverageHandlerLocale()
  {
    if (!lc_all.empty()) {
      cmSystemTools::PutEnv("LC_ALL=" + lc_all);
    } else {
      cmSystemTools::UnsetEnv("LC_ALL");
    }
  }
  std::string lc_all;
};

int cmCTestCoverageHandler::HandleJacocoCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmParseJacocoCoverage cov = cmParseJacocoCoverage(*cont, this->CTest);

  // Search in the source directory.
  cmsys::Glob g1;
  std::vector<std::string> files;
  g1.SetRecurse(true);

  std::string SourceDir =
    this->CTest->GetCTestConfiguration("SourceDirectory");
  std::string coverageFile = SourceDir + "/*jacoco.xml";

  g1.FindFiles(coverageFile);
  files = g1.GetFiles();

  // ...and in the binary directory.
  cmsys::Glob g2;
  std::vector<std::string> binFiles;
  g2.SetRecurse(true);
  std::string binaryDir = this->CTest->GetCTestConfiguration("BuildDirectory");
  std::string binCoverageFile = binaryDir + "/*jacoco.xml";
  g2.FindFiles(binCoverageFile);
  binFiles = g2.GetFiles();
  if (!binFiles.empty()) {
    files.insert(files.end(), binFiles.begin(), binFiles.end());
  }

  if (!files.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Found Jacoco Files, Performing Coverage" << std::endl,
                       this->Quiet);
    cov.LoadCoverageData(files);
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find Jacoco coverage files: " << coverageFile
                                                              << std::endl,
                       this->Quiet);
  }
  return static_cast<int>(cont->TotalCoverage.size());
}

int cmCTestCoverageHandler::HandleDelphiCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmParseDelphiCoverage cov = cmParseDelphiCoverage(*cont, this->CTest);
  cmsys::Glob g;
  std::vector<std::string> files;
  g.SetRecurse(true);

  std::string BinDir = this->CTest->GetBinaryDir();
  std::string coverageFile = BinDir + "/*(*.pas).html";

  g.FindFiles(coverageFile);
  files = g.GetFiles();
  if (!files.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Found Delphi HTML Files, Performing Coverage"
                         << std::endl,
                       this->Quiet);
    cov.LoadCoverageData(files);
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find Delphi coverage files: " << coverageFile
                                                              << std::endl,
                       this->Quiet);
  }
  return static_cast<int>(cont->TotalCoverage.size());
}

int cmCTestCoverageHandler::HandleBlanketJSCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmParseBlanketJSCoverage cov = cmParseBlanketJSCoverage(*cont, this->CTest);
  std::string SourceDir =
    this->CTest->GetCTestConfiguration("SourceDirectory");

  // Look for something other than output.json, still JSON extension.
  std::string coverageFile = SourceDir + "/*.json";
  cmsys::Glob g;
  std::vector<std::string> files;
  std::vector<std::string> blanketFiles;
  g.FindFiles(coverageFile);
  files = g.GetFiles();
  // Ensure that the JSON files found are the result of the
  // Blanket.js output. Check for the "node-jscoverage"
  // string on the second line
  std::string line;
  for (unsigned int fileEntry = 0; fileEntry < files.size(); fileEntry++) {
    cmsys::ifstream in(files[fileEntry].c_str());
    cmSystemTools::GetLineFromStream(in, line);
    cmSystemTools::GetLineFromStream(in, line);
    if (line.find("node-jscoverage") != std::string::npos) {
      blanketFiles.push_back(files[fileEntry]);
    }
  }
  //  Take all files with the node-jscoverage string and parse those
  if (!blanketFiles.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Found BlanketJS output JSON, Performing Coverage"
                         << std::endl,
                       this->Quiet);
    cov.LoadCoverageData(files);
  } else {
    cmCTestOptionalLog(
      this->CTest, HANDLER_VERBOSE_OUTPUT,
      " Cannot find BlanketJS coverage files: " << coverageFile << std::endl,
      this->Quiet);
  }
  return static_cast<int>(cont->TotalCoverage.size());
}
int cmCTestCoverageHandler::HandleGCovCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  std::string gcovCommand =
    this->CTest->GetCTestConfiguration("CoverageCommand");
  if (gcovCommand.empty()) {
    cmCTestLog(this->CTest, WARNING, "Could not find gcov." << std::endl);
    return 0;
  }
  std::string gcovExtraFlags =
    this->CTest->GetCTestConfiguration("CoverageExtraFlags");

  // Immediately skip to next coverage option since codecov is only for Intel
  // compiler
  if (gcovCommand == "codecov") {
    return 0;
  }

  // Style 1
  std::string st1gcovOutputRex1 =
    "[0-9]+\\.[0-9]+% of [0-9]+ (source |)lines executed in file (.*)$";
  std::string st1gcovOutputRex2 = "^Creating (.*\\.gcov)\\.";
  cmsys::RegularExpression st1re1(st1gcovOutputRex1.c_str());
  cmsys::RegularExpression st1re2(st1gcovOutputRex2.c_str());

  // Style 2
  std::string st2gcovOutputRex1 = "^File *[`'](.*)'$";
  std::string st2gcovOutputRex2 =
    "Lines executed: *[0-9]+\\.[0-9]+% of [0-9]+$";
  std::string st2gcovOutputRex3 = "^(.*)reating [`'](.*\\.gcov)'";
  std::string st2gcovOutputRex4 = "^(.*):unexpected EOF *$";
  std::string st2gcovOutputRex5 = "^(.*):cannot open source file*$";
  std::string st2gcovOutputRex6 =
    "^(.*):source file is newer than graph file `(.*)'$";
  cmsys::RegularExpression st2re1(st2gcovOutputRex1.c_str());
  cmsys::RegularExpression st2re2(st2gcovOutputRex2.c_str());
  cmsys::RegularExpression st2re3(st2gcovOutputRex3.c_str());
  cmsys::RegularExpression st2re4(st2gcovOutputRex4.c_str());
  cmsys::RegularExpression st2re5(st2gcovOutputRex5.c_str());
  cmsys::RegularExpression st2re6(st2gcovOutputRex6.c_str());

  std::vector<std::string> files;
  this->FindGCovFiles(files);
  std::vector<std::string>::iterator it;

  if (files.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find any GCov coverage files." << std::endl,
                       this->Quiet);
    // No coverage files is a valid thing, so the exit code is 0
    return 0;
  }

  std::string testingDir = this->CTest->GetBinaryDir() + "/Testing";
  std::string tempDir = testingDir + "/CoverageInfo";
  cmSystemTools::MakeDirectory(tempDir.c_str());
  cmWorkingDirectory workdir(tempDir);

  int gcovStyle = 0;

  std::set<std::string> missingFiles;

  std::string actualSourceFile;
  cmCTestOptionalLog(
    this->CTest, HANDLER_OUTPUT,
    "   Processing coverage (each . represents one file):" << std::endl,
    this->Quiet);
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);
  int file_count = 0;

  // make sure output from gcov is in English!
  cmCTestCoverageHandlerLocale locale_C;
  static_cast<void>(locale_C);

  // files is a list of *.da and *.gcda files with coverage data in them.
  // These are binary files that you give as input to gcov so that it will
  // give us text output we can analyze to summarize coverage.
  //
  for (it = files.begin(); it != files.end(); ++it) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "." << std::flush,
                       this->Quiet);

    // Call gcov to get coverage data for this *.gcda file:
    //
    std::string fileDir = cmSystemTools::GetFilenamePath(*it);
    std::string command = "\"" + gcovCommand + "\" " + gcovExtraFlags + " " +
      "-o \"" + fileDir + "\" " + "\"" + *it + "\"";

    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       command << std::endl, this->Quiet);

    std::string output;
    std::string errors;
    int retVal = 0;
    *cont->OFS << "* Run coverage for: " << fileDir << std::endl;
    *cont->OFS << "  Command: " << command << std::endl;
    int res =
      this->CTest->RunCommand(command.c_str(), &output, &errors, &retVal,
                              tempDir.c_str(), 0 /*this->TimeOut*/);

    *cont->OFS << "  Output: " << output << std::endl;
    *cont->OFS << "  Errors: " << errors << std::endl;
    if (!res) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Problem running coverage on file: " << *it << std::endl);
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Command produced error: " << errors << std::endl);
      cont->Error++;
      continue;
    }
    if (retVal != 0) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "Coverage command returned: "
                   << retVal << " while processing: " << *it << std::endl);
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Command produced error: " << cont->Error << std::endl);
    }
    cmCTestOptionalLog(
      this->CTest, HANDLER_VERBOSE_OUTPUT,
      "--------------------------------------------------------------"
        << std::endl
        << output << std::endl
        << "--------------------------------------------------------------"
        << std::endl,
      this->Quiet);

    std::vector<std::string> lines;
    std::vector<std::string>::iterator line;

    cmSystemTools::Split(output.c_str(), lines);

    for (line = lines.begin(); line != lines.end(); ++line) {
      std::string sourceFile;
      std::string gcovFile;

      cmCTestOptionalLog(this->CTest, DEBUG,
                         "Line: [" << *line << "]" << std::endl, this->Quiet);

      if (line->empty()) {
        // Ignore empty line; probably style 2
      } else if (st1re1.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 1;
        }
        if (gcovStyle != 1) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e1"
                       << std::endl);
          cont->Error++;
          break;
        }

        actualSourceFile = "";
        sourceFile = st1re1.match(2);
      } else if (st1re2.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 1;
        }
        if (gcovStyle != 1) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e2"
                       << std::endl);
          cont->Error++;
          break;
        }

        gcovFile = st1re2.match(1);
      } else if (st2re1.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e3"
                       << std::endl);
          cont->Error++;
          break;
        }

        actualSourceFile = "";
        sourceFile = st2re1.match(1);
      } else if (st2re2.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e4"
                       << std::endl);
          cont->Error++;
          break;
        }
      } else if (st2re3.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e5"
                       << std::endl);
          cont->Error++;
          break;
        }

        gcovFile = st2re3.match(2);
      } else if (st2re4.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e6"
                       << std::endl);
          cont->Error++;
          break;
        }

        cmCTestOptionalLog(this->CTest, WARNING,
                           "Warning: " << st2re4.match(1)
                                       << " had unexpected EOF" << std::endl,
                           this->Quiet);
      } else if (st2re5.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e7"
                       << std::endl);
          cont->Error++;
          break;
        }

        cmCTestOptionalLog(this->CTest, WARNING, "Warning: Cannot open file: "
                             << st2re5.match(1) << std::endl,
                           this->Quiet);
      } else if (st2re6.find(line->c_str())) {
        if (gcovStyle == 0) {
          gcovStyle = 2;
        }
        if (gcovStyle != 2) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output style e8"
                       << std::endl);
          cont->Error++;
          break;
        }

        cmCTestOptionalLog(this->CTest, WARNING, "Warning: File: "
                             << st2re6.match(1) << " is newer than "
                             << st2re6.match(2) << std::endl,
                           this->Quiet);
      } else {
        // gcov 4.7 can have output lines saying "No executable lines" and
        // "Removing 'filename.gcov'"... Don't log those as "errors."
        if (*line != "No executable lines" &&
            !cmSystemTools::StringStartsWith(line->c_str(), "Removing ")) {
          cmCTestLog(this->CTest, ERROR_MESSAGE, "Unknown gcov output line: ["
                       << *line << "]" << std::endl);
          cont->Error++;
          // abort();
        }
      }

      // If the last line of gcov output gave us a valid value for gcovFile,
      // and we have an actualSourceFile, then insert a (or add to existing)
      // SingleFileCoverageVector for actualSourceFile:
      //
      if (!gcovFile.empty() && !actualSourceFile.empty()) {
        cmCTestCoverageHandlerContainer::SingleFileCoverageVector& vec =
          cont->TotalCoverage[actualSourceFile];

        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "   in gcovFile: " << gcovFile << std::endl,
                           this->Quiet);

        cmsys::ifstream ifile(gcovFile.c_str());
        if (!ifile) {
          cmCTestLog(this->CTest, ERROR_MESSAGE,
                     "Cannot open file: " << gcovFile << std::endl);
        } else {
          long cnt = -1;
          std::string nl;
          while (cmSystemTools::GetLineFromStream(ifile, nl)) {
            cnt++;

            // TODO: Handle gcov 3.0 non-coverage lines

            // Skip empty lines
            if (nl.empty()) {
              continue;
            }

            // Skip unused lines
            if (nl.size() < 12) {
              continue;
            }

            // Read the coverage count from the beginning of the gcov output
            // line
            std::string prefix = nl.substr(0, 12);
            int cov = atoi(prefix.c_str());

            // Read the line number starting at the 10th character of the gcov
            // output line
            std::string lineNumber = nl.substr(10, 5);

            int lineIdx = atoi(lineNumber.c_str()) - 1;
            if (lineIdx >= 0) {
              while (vec.size() <= static_cast<size_t>(lineIdx)) {
                vec.push_back(-1);
              }

              // Initially all entries are -1 (not used). If we get coverage
              // information, increment it to 0 first.
              if (vec[lineIdx] < 0) {
                if (cov > 0 || prefix.find('#') != std::string::npos) {
                  vec[lineIdx] = 0;
                }
              }

              vec[lineIdx] += cov;
            }
          }
        }

        actualSourceFile = "";
      }

      if (!sourceFile.empty() && actualSourceFile.empty()) {
        gcovFile = "";

        // Is it in the source dir or the binary dir?
        //
        if (IsFileInDir(sourceFile, cont->SourceDir)) {
          cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                             "   produced s: " << sourceFile << std::endl,
                             this->Quiet);
          *cont->OFS << "  produced in source dir: " << sourceFile
                     << std::endl;
          actualSourceFile = cmSystemTools::CollapseFullPath(sourceFile);
        } else if (IsFileInDir(sourceFile, cont->BinaryDir)) {
          cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                             "   produced b: " << sourceFile << std::endl,
                             this->Quiet);
          *cont->OFS << "  produced in binary dir: " << sourceFile
                     << std::endl;
          actualSourceFile = cmSystemTools::CollapseFullPath(sourceFile);
        }

        if (actualSourceFile.empty()) {
          if (missingFiles.find(sourceFile) == missingFiles.end()) {
            cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                               "Something went wrong" << std::endl,
                               this->Quiet);
            cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                               "Cannot find file: [" << sourceFile << "]"
                                                     << std::endl,
                               this->Quiet);
            cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                               " in source dir: [" << cont->SourceDir << "]"
                                                   << std::endl,
                               this->Quiet);
            cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                               " or binary dir: [" << cont->BinaryDir.size()
                                                   << "]" << std::endl,
                               this->Quiet);
            *cont->OFS << "  Something went wrong. Cannot find file: "
                       << sourceFile << " in source dir: " << cont->SourceDir
                       << " or binary dir: " << cont->BinaryDir << std::endl;

            missingFiles.insert(sourceFile);
          }
        }
      }
    }

    file_count++;

    if (file_count % 50 == 0) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                         " processed: " << file_count << " out of "
                                        << files.size() << std::endl,
                         this->Quiet);
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);
    }
  }

  return file_count;
}

int cmCTestCoverageHandler::HandleLCovCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  std::string lcovCommand =
    this->CTest->GetCTestConfiguration("CoverageCommand");
  std::string lcovExtraFlags =
    this->CTest->GetCTestConfiguration("CoverageExtraFlags");
  if (lcovCommand != "codecov") {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Not a valid Intel Coverage command." << std::endl,
                       this->Quiet);
    return 0;
  }
  // There is only percentage completed output from LCOV
  std::string st2lcovOutputRex3 = "[0-9]+%";
  cmsys::RegularExpression st2re3(st2lcovOutputRex3.c_str());

  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " This is coverage command: " << lcovCommand << std::endl,
                     this->Quiet);

  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " These are coverage command flags: " << lcovExtraFlags
                                                           << std::endl,
                     this->Quiet);

  std::vector<std::string> files;
  if (!this->FindLCovFiles(files)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error while finding LCov files.\n");
    return 0;
  }
  std::vector<std::string>::iterator it;

  if (files.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find any LCov coverage files." << std::endl,
                       this->Quiet);
    // No coverage files is a valid thing, so the exit code is 0
    return 0;
  }
  std::string testingDir = this->CTest->GetBinaryDir();

  std::set<std::string> missingFiles;

  std::string actualSourceFile;
  cmCTestOptionalLog(
    this->CTest, HANDLER_OUTPUT,
    "   Processing coverage (each . represents one file):" << std::endl,
    this->Quiet);
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);
  int file_count = 0;

  // make sure output from lcov is in English!
  cmCTestCoverageHandlerLocale locale_C;
  static_cast<void>(locale_C);

  // In intel compiler we have to call codecov only once in each executable
  // directory. It collects all *.dyn files to generate .dpi file.
  for (it = files.begin(); it != files.end(); ++it) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "." << std::flush,
                       this->Quiet);
    std::string fileDir = cmSystemTools::GetFilenamePath(*it);
    cmWorkingDirectory workdir(fileDir);
    std::string command = "\"" + lcovCommand + "\" " + lcovExtraFlags + " ";

    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Current coverage dir: " << fileDir << std::endl,
                       this->Quiet);
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       command << std::endl, this->Quiet);

    std::string output;
    std::string errors;
    int retVal = 0;
    *cont->OFS << "* Run coverage for: " << fileDir << std::endl;
    *cont->OFS << "  Command: " << command << std::endl;
    int res =
      this->CTest->RunCommand(command.c_str(), &output, &errors, &retVal,
                              fileDir.c_str(), 0 /*this->TimeOut*/);

    *cont->OFS << "  Output: " << output << std::endl;
    *cont->OFS << "  Errors: " << errors << std::endl;
    if (!res) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Problem running coverage on file: " << *it << std::endl);
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Command produced error: " << errors << std::endl);
      cont->Error++;
      continue;
    }
    if (retVal != 0) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "Coverage command returned: "
                   << retVal << " while processing: " << *it << std::endl);
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Command produced error: " << cont->Error << std::endl);
    }
    cmCTestOptionalLog(
      this->CTest, HANDLER_VERBOSE_OUTPUT,
      "--------------------------------------------------------------"
        << std::endl
        << output << std::endl
        << "--------------------------------------------------------------"
        << std::endl,
      this->Quiet);

    std::vector<std::string> lines;
    std::vector<std::string>::iterator line;

    cmSystemTools::Split(output.c_str(), lines);

    for (line = lines.begin(); line != lines.end(); ++line) {
      std::string sourceFile;
      std::string lcovFile;

      if (line->empty()) {
        // Ignore empty line
      }
      // Look for LCOV files in binary directory
      // Intel Compiler creates a CodeCoverage dir for each subfolder and
      // each subfolder has LCOV files
      cmsys::Glob gl;
      gl.RecurseOn();
      gl.RecurseThroughSymlinksOff();
      std::string dir;
      std::vector<std::string> lcovFiles;
      dir = this->CTest->GetBinaryDir();
      std::string daGlob;
      daGlob = dir;
      daGlob += "/*.LCOV";
      cmCTestOptionalLog(
        this->CTest, HANDLER_VERBOSE_OUTPUT,
        "   looking for LCOV files in: " << daGlob << std::endl, this->Quiet);
      gl.FindFiles(daGlob);
      // Keep a list of all LCOV files
      lcovFiles.insert(lcovFiles.end(), gl.GetFiles().begin(),
                       gl.GetFiles().end());

      for (std::vector<std::string>::iterator a = lcovFiles.begin();
           a != lcovFiles.end(); ++a) {
        lcovFile = *a;
        cmsys::ifstream srcead(lcovFile.c_str());
        if (!srcead) {
          cmCTestLog(this->CTest, ERROR_MESSAGE,
                     "Cannot open file: " << lcovFile << std::endl);
        }
        std::string srcname;

        int success = cmSystemTools::GetLineFromStream(srcead, srcname);
        if (!success) {
          cmCTestLog(this->CTest, ERROR_MESSAGE,
                     "Error while parsing lcov file '"
                       << lcovFile << "':"
                       << " No source file name found!" << std::endl);
          return 0;
        }
        srcname = srcname.substr(18);
        // We can directly read found LCOV files to determine the source
        // files
        sourceFile = srcname;
        actualSourceFile = srcname;

        for (std::vector<std::string>::iterator t = lcovFiles.begin();
             t != lcovFiles.end(); ++t) {
          cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                             "Found LCOV File: " << *t << std::endl,
                             this->Quiet);
        }
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "SourceFile: " << sourceFile << std::endl,
                           this->Quiet);
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "lCovFile: " << lcovFile << std::endl, this->Quiet);

        // If we have some LCOV files to process
        if (!lcovFile.empty() && !actualSourceFile.empty()) {
          cmCTestCoverageHandlerContainer::SingleFileCoverageVector& vec =
            cont->TotalCoverage[actualSourceFile];

          cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                             "   in lcovFile: " << lcovFile << std::endl,
                             this->Quiet);

          cmsys::ifstream ifile(lcovFile.c_str());
          if (!ifile) {
            cmCTestLog(this->CTest, ERROR_MESSAGE,
                       "Cannot open file: " << lcovFile << std::endl);
          } else {
            long cnt = -1;
            std::string nl;

            // Skip the first line
            cmSystemTools::GetLineFromStream(ifile, nl);
            cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                               "File is ready, start reading." << std::endl,
                               this->Quiet);
            while (cmSystemTools::GetLineFromStream(ifile, nl)) {
              cnt++;

              // Skip empty lines
              if (nl.empty()) {
                continue;
              }

              // Skip unused lines
              if (nl.size() < 12) {
                continue;
              }

              // Read the coverage count from the beginning of the lcov
              // output line
              std::string prefix = nl.substr(0, 17);
              int cov = atoi(prefix.c_str());

              // Read the line number starting at the 17th character of the
              // lcov output line
              std::string lineNumber = nl.substr(17, 7);

              int lineIdx = atoi(lineNumber.c_str()) - 1;
              if (lineIdx >= 0) {
                while (vec.size() <= static_cast<size_t>(lineIdx)) {
                  vec.push_back(-1);
                }

                // Initially all entries are -1 (not used). If we get coverage
                // information, increment it to 0 first.
                if (vec[lineIdx] < 0) {
                  if (cov > 0 || prefix.find('#') != std::string::npos) {
                    vec[lineIdx] = 0;
                  }
                }

                vec[lineIdx] += cov;
              }
            }
          }

          actualSourceFile = "";
        }
      }
    }

    file_count++;

    if (file_count % 50 == 0) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                         " processed: " << file_count << " out of "
                                        << files.size() << std::endl,
                         this->Quiet);
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "    ", this->Quiet);
    }
  }

  return file_count;
}

void cmCTestCoverageHandler::FindGCovFiles(std::vector<std::string>& files)
{
  cmsys::Glob gl;
  gl.RecurseOn();
  gl.RecurseThroughSymlinksOff();

  for (LabelMapType::const_iterator lmi = this->TargetDirs.begin();
       lmi != this->TargetDirs.end(); ++lmi) {
    // Skip targets containing no interesting labels.
    if (!this->IntersectsFilter(lmi->second)) {
      continue;
    }

    // Coverage files appear next to their object files in the target
    // support directory.
    cmCTestOptionalLog(
      this->CTest, HANDLER_VERBOSE_OUTPUT,
      "   globbing for coverage in: " << lmi->first << std::endl, this->Quiet);
    std::string daGlob = lmi->first;
    daGlob += "/*.da";
    gl.FindFiles(daGlob);
    files.insert(files.end(), gl.GetFiles().begin(), gl.GetFiles().end());
    daGlob = lmi->first;
    daGlob += "/*.gcda";
    gl.FindFiles(daGlob);
    files.insert(files.end(), gl.GetFiles().begin(), gl.GetFiles().end());
  }
}

bool cmCTestCoverageHandler::FindLCovFiles(std::vector<std::string>& files)
{
  cmsys::Glob gl;
  gl.RecurseOff(); // No need of recurse if -prof_dir${BUILD_DIR} flag is
                   // used while compiling.
  gl.RecurseThroughSymlinksOff();
  std::string buildDir = this->CTest->GetCTestConfiguration("BuildDirectory");
  cmWorkingDirectory workdir(buildDir);

  // Run profmerge to merge all *.dyn files into dpi files
  if (!cmSystemTools::RunSingleCommand("profmerge")) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Error while running profmerge.\n");
    return false;
  }

  // DPI file should appear in build directory
  std::string daGlob;
  daGlob = buildDir;
  daGlob += "/*.dpi";
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "   looking for dpi files in: " << daGlob << std::endl,
                     this->Quiet);
  if (!gl.FindFiles(daGlob)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error while finding files matching " << daGlob << std::endl);
    return false;
  }
  files.insert(files.end(), gl.GetFiles().begin(), gl.GetFiles().end());
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Now searching in: " << daGlob << std::endl, this->Quiet);
  return true;
}

int cmCTestCoverageHandler::HandleTracePyCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  cmsys::Glob gl;
  gl.RecurseOn();
  gl.RecurseThroughSymlinksOff();
  std::string daGlob = cont->BinaryDir + "/*.cover";
  gl.FindFiles(daGlob);
  std::vector<std::string> files = gl.GetFiles();

  if (files.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " Cannot find any Python Trace.py coverage files."
                         << std::endl,
                       this->Quiet);
    // No coverage files is a valid thing, so the exit code is 0
    return 0;
  }

  std::string testingDir = this->CTest->GetBinaryDir() + "/Testing";
  std::string tempDir = testingDir + "/CoverageInfo";
  cmSystemTools::MakeDirectory(tempDir.c_str());

  std::vector<std::string>::iterator fileIt;
  int file_count = 0;
  for (fileIt = files.begin(); fileIt != files.end(); ++fileIt) {
    std::string fileName = this->FindFile(cont, *fileIt);
    if (fileName.empty()) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Cannot find source Python file corresponding to: "
                   << *fileIt << std::endl);
      continue;
    }

    std::string actualSourceFile = cmSystemTools::CollapseFullPath(fileName);
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "   Check coverage for file: " << actualSourceFile
                                                      << std::endl,
                       this->Quiet);
    cmCTestCoverageHandlerContainer::SingleFileCoverageVector* vec =
      &cont->TotalCoverage[actualSourceFile];
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "   in file: " << *fileIt << std::endl, this->Quiet);
    cmsys::ifstream ifile(fileIt->c_str());
    if (!ifile) {
      cmCTestLog(this->CTest, ERROR_MESSAGE,
                 "Cannot open file: " << *fileIt << std::endl);
    } else {
      long cnt = -1;
      std::string nl;
      while (cmSystemTools::GetLineFromStream(ifile, nl)) {
        cnt++;

        // Skip empty lines
        if (nl.empty()) {
          continue;
        }

        // Skip unused lines
        if (nl.size() < 12) {
          continue;
        }

        // Read the coverage count from the beginning of the Trace.py output
        // line
        std::string prefix = nl.substr(0, 6);
        if (prefix[5] != ' ' && prefix[5] != ':') {
          // This is a hack. We should really do something more elaborate
          prefix = nl.substr(0, 7);
          if (prefix[6] != ' ' && prefix[6] != ':') {
            prefix = nl.substr(0, 8);
            if (prefix[7] != ' ' && prefix[7] != ':') {
              cmCTestLog(this->CTest, ERROR_MESSAGE,
                         "Currently the limit is maximum coverage of 999999"
                           << std::endl);
            }
          }
        }
        int cov = atoi(prefix.c_str());
        if (prefix[prefix.size() - 1] != ':') {
          // This line does not have ':' so no coverage here. That said,
          // Trace.py does not handle not covered lines versus comments etc.
          // So, this will be set to 0.
          cov = 0;
        }
        cmCTestOptionalLog(
          this->CTest, DEBUG,
          "Prefix: " << prefix << " cov: " << cov << std::endl, this->Quiet);
        // Read the line number starting at the 10th character of the gcov
        // output line
        long lineIdx = cnt;
        if (lineIdx >= 0) {
          while (vec->size() <= static_cast<size_t>(lineIdx)) {
            vec->push_back(-1);
          }
          // Initially all entries are -1 (not used). If we get coverage
          // information, increment it to 0 first.
          if ((*vec)[lineIdx] < 0) {
            if (cov >= 0) {
              (*vec)[lineIdx] = 0;
            }
          }
          (*vec)[lineIdx] += cov;
        }
      }
    }
    ++file_count;
  }
  return file_count;
}

std::string cmCTestCoverageHandler::FindFile(
  cmCTestCoverageHandlerContainer* cont, std::string const& fileName)
{
  std::string fileNameNoE =
    cmSystemTools::GetFilenameWithoutLastExtension(fileName);
  // First check in source and binary directory
  std::string fullName = cont->SourceDir + "/" + fileNameNoE + ".py";
  if (cmSystemTools::FileExists(fullName.c_str())) {
    return fullName;
  }
  fullName = cont->BinaryDir + "/" + fileNameNoE + ".py";
  if (cmSystemTools::FileExists(fullName.c_str())) {
    return fullName;
  }
  return "";
}

// This is a header put on each marked up source file
namespace {
const char* bullseyeHelp[] = {
  "    Coverage produced by bullseye covbr tool: ",
  "      www.bullseye.com/help/ref_covbr.html",
  "    * An arrow --> indicates incomplete coverage.",
  "    * An X indicates a function that was invoked, a switch label that ",
  "      was exercised, a try-block that finished, or an exception handler ",
  "      that was invoked.",
  "    * A T or F indicates a boolean decision that evaluated true or false,",
  "      respectively.",
  "    * A t or f indicates a boolean condition within a decision if the ",
  "      condition evaluated true or false, respectively.",
  "    * A k indicates a constant decision or condition.",
  "    * The slash / means this probe is excluded from summary results. ",
  CM_NULLPTR
};
}

int cmCTestCoverageHandler::RunBullseyeCoverageBranch(
  cmCTestCoverageHandlerContainer* cont,
  std::set<std::string>& coveredFileNames, std::vector<std::string>& files,
  std::vector<std::string>& filesFullPath)
{
  if (files.size() != filesFullPath.size()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Files and full path files not the same size?:\n");
    return 0;
  }
  // create the output stream for the CoverageLog-N.xml file
  cmGeneratedFileStream covLogFile;
  cmXMLWriter covLogXML(covLogFile);
  int logFileCount = 0;
  if (!this->StartCoverageLogFile(covLogFile, logFileCount)) {
    return -1;
  }
  this->StartCoverageLogXML(covLogXML);
  // for each file run covbr on that file to get the coverage
  // information for that file
  std::string outputFile;
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "run covbr: " << std::endl, this->Quiet);

  if (!this->RunBullseyeCommand(cont, "covbr", CM_NULLPTR, outputFile)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "error running covbr for."
                 << "\n");
    return -1;
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "covbr output in  " << outputFile << std::endl,
                     this->Quiet);
  // open the output file
  cmsys::ifstream fin(outputFile.c_str());
  if (!fin) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot open coverage file: " << outputFile << std::endl);
    return 0;
  }
  std::map<std::string, std::string> fileMap;
  std::vector<std::string>::iterator fp = filesFullPath.begin();
  for (std::vector<std::string>::iterator f = files.begin(); f != files.end();
       ++f, ++fp) {
    fileMap[*f] = *fp;
  }

  int count = 0; // keep count of the number of files
  // Now parse each line from the bullseye cov log file
  std::string lineIn;
  bool valid = false; // are we in a valid output file
  int line = 0;       // line of the current file
  std::string file;
  while (cmSystemTools::GetLineFromStream(fin, lineIn)) {
    bool startFile = false;
    if (lineIn.size() > 1 && lineIn[lineIn.size() - 1] == ':') {
      file = lineIn.substr(0, lineIn.size() - 1);
      if (coveredFileNames.find(file) != coveredFileNames.end()) {
        startFile = true;
      }
    }
    if (startFile) {
      // if we are in a valid file close it because a new one started
      if (valid) {
        covLogXML.EndElement(); // Report
        covLogXML.EndElement(); // File
      }
      // only allow 100 files in each log file
      if (count != 0 && count % 100 == 0) {
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "start a new log file: " << count << std::endl,
                           this->Quiet);
        this->EndCoverageLogXML(covLogXML);
        this->EndCoverageLogFile(covLogFile, logFileCount);
        logFileCount++;
        if (!this->StartCoverageLogFile(covLogFile, logFileCount)) {
          return -1;
        }
        this->StartCoverageLogXML(covLogXML);
        count++; // move on one
      }
      std::map<std::string, std::string>::iterator i = fileMap.find(file);
      // if the file should be covered write out the header for that file
      if (i != fileMap.end()) {
        // we have a new file so count it in the output
        count++;
        cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                           "Produce coverage for file: " << file << " "
                                                         << count << std::endl,
                           this->Quiet);
        // start the file output
        covLogXML.StartElement("File");
        covLogXML.Attribute("Name", i->first);
        covLogXML.Attribute(
          "FullPath", this->CTest->GetShortPathToFile(i->second.c_str()));
        covLogXML.StartElement("Report");
        // write the bullseye header
        line = 0;
        for (int k = 0; bullseyeHelp[k] != CM_NULLPTR; ++k) {
          covLogXML.StartElement("Line");
          covLogXML.Attribute("Number", line);
          covLogXML.Attribute("Count", -1);
          covLogXML.Content(bullseyeHelp[k]);
          covLogXML.EndElement(); // Line
          line++;
        }
        valid = true; // we are in a valid file section
      } else {
        // this is not a file that we want coverage for
        valid = false;
      }
    }
    // we are not at a start file, and we are in a valid file output the line
    else if (valid) {
      covLogXML.StartElement("Line");
      covLogXML.Attribute("Number", line);
      covLogXML.Attribute("Count", -1);
      covLogXML.Content(lineIn);
      covLogXML.EndElement(); // Line
      line++;
    }
  }
  // if we ran out of lines a valid file then close that file
  if (valid) {
    covLogXML.EndElement(); // Report
    covLogXML.EndElement(); // File
  }
  this->EndCoverageLogXML(covLogXML);
  this->EndCoverageLogFile(covLogFile, logFileCount);
  return 1;
}

int cmCTestCoverageHandler::RunBullseyeCommand(
  cmCTestCoverageHandlerContainer* cont, const char* cmd, const char* arg,
  std::string& outputFile)
{
  std::string program = cmSystemTools::FindProgram(cmd);
  if (program.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot find :" << cmd << "\n");
    return 0;
  }
  if (arg) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Run : " << program << " " << arg << "\n", this->Quiet);
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Run : " << program << "\n", this->Quiet);
  }
  // create a process object and start it
  cmCTestRunProcess runCoverageSrc;
  runCoverageSrc.SetCommand(program.c_str());
  runCoverageSrc.AddArgument(arg);
  std::string stdoutFile = cont->BinaryDir + "/Testing/Temporary/";
  stdoutFile += this->GetCTestInstance()->GetCurrentTag();
  stdoutFile += "-";
  stdoutFile += cmd;
  std::string stderrFile = stdoutFile;
  stdoutFile += ".stdout";
  stderrFile += ".stderr";
  runCoverageSrc.SetStdoutFile(stdoutFile.c_str());
  runCoverageSrc.SetStderrFile(stderrFile.c_str());
  if (!runCoverageSrc.StartProcess()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Could not run : " << program << " " << arg << "\n"
                                  << "kwsys process state : "
                                  << runCoverageSrc.GetProcessState());
    return 0;
  }
  // since we set the output file names wait for it to end
  runCoverageSrc.WaitForExit();
  outputFile = stdoutFile;
  return 1;
}

int cmCTestCoverageHandler::RunBullseyeSourceSummary(
  cmCTestCoverageHandlerContainer* cont)
{
  // Run the covsrc command and create a temp outputfile
  std::string outputFile;
  if (!this->RunBullseyeCommand(cont, "covsrc", "-c", outputFile)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "error running covsrc:\n");
    return 0;
  }

  std::ostream& tmpLog = *cont->OFS;
  // copen the Coverage.xml file in the Testing directory
  cmGeneratedFileStream covSumFile;
  cmXMLWriter xml(covSumFile);
  if (!this->StartResultingXML(cmCTest::PartCoverage, "Coverage",
                               covSumFile)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot open coverage summary file."
                 << std::endl);
    return 0;
  }
  this->CTest->StartXML(xml, this->AppendXML);
  double elapsed_time_start = cmSystemTools::GetTime();
  std::string coverage_start_time = this->CTest->CurrentTime();
  xml.StartElement("Coverage");
  xml.Element("StartDateTime", coverage_start_time);
  xml.Element("StartTime",
              static_cast<unsigned int>(cmSystemTools::GetTime()));
  std::string stdline;
  std::string errline;
  // expected output:
  // first line is:
  // "Source","Function Coverage","out of","%","C/D Coverage","out of","%"
  // after that data follows in that format
  std::string sourceFile;
  int functionsCalled = 0;
  int totalFunctions = 0;
  int percentFunction = 0;
  int branchCovered = 0;
  int totalBranches = 0;
  int percentBranch = 0;
  double total_tested = 0;
  double total_untested = 0;
  double total_functions = 0;
  double percent_coverage = 0;
  double number_files = 0;
  std::vector<std::string> coveredFiles;
  std::vector<std::string> coveredFilesFullPath;
  // Read and parse the summary output file
  cmsys::ifstream fin(outputFile.c_str());
  if (!fin) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot open coverage summary file: " << outputFile
                                                     << std::endl);
    return 0;
  }
  std::set<std::string> coveredFileNames;
  while (cmSystemTools::GetLineFromStream(fin, stdline)) {
    // if we have a line of output from stdout
    if (!stdline.empty()) {
      // parse the comma separated output
      this->ParseBullsEyeCovsrcLine(
        stdline, sourceFile, functionsCalled, totalFunctions, percentFunction,
        branchCovered, totalBranches, percentBranch);
      // The first line is the header
      if (sourceFile == "Source" || sourceFile == "Total") {
        continue;
      }
      std::string file = sourceFile;
      coveredFileNames.insert(file);
      if (!cmSystemTools::FileIsFullPath(sourceFile.c_str())) {
        // file will be relative to the binary dir
        file = cont->BinaryDir;
        file += "/";
        file += sourceFile;
      }
      file = cmSystemTools::CollapseFullPath(file);
      bool shouldIDoCoverage = this->ShouldIDoCoverage(
        file.c_str(), cont->SourceDir.c_str(), cont->BinaryDir.c_str());
      if (!shouldIDoCoverage) {
        cmCTestOptionalLog(
          this->CTest, HANDLER_VERBOSE_OUTPUT,
          ".NoDartCoverage found, so skip coverage check for: " << file
                                                                << std::endl,
          this->Quiet);
        continue;
      }

      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Doing coverage for: " << file << std::endl,
                         this->Quiet);

      coveredFiles.push_back(sourceFile);
      coveredFilesFullPath.push_back(file);

      number_files++;
      total_functions += totalFunctions;
      total_tested += functionsCalled;
      total_untested += (totalFunctions - functionsCalled);

      std::string fileName = cmSystemTools::GetFilenameName(file);
      std::string shortFileName =
        this->CTest->GetShortPathToFile(file.c_str());

      float cper = static_cast<float>(percentBranch + percentFunction);
      if (totalBranches > 0) {
        cper /= 2.0f;
      }
      percent_coverage += static_cast<double>(cper);
      float cmet = static_cast<float>(percentFunction + percentBranch);
      if (totalBranches > 0) {
        cmet /= 2.0f;
      }
      cmet /= 100.0f;
      tmpLog << stdline << "\n";
      tmpLog << fileName << "\n";
      tmpLog << "functionsCalled: " << functionsCalled / 100 << "\n";
      tmpLog << "totalFunctions: " << totalFunctions / 100 << "\n";
      tmpLog << "percentFunction: " << percentFunction << "\n";
      tmpLog << "branchCovered: " << branchCovered << "\n";
      tmpLog << "totalBranches: " << totalBranches << "\n";
      tmpLog << "percentBranch: " << percentBranch << "\n";
      tmpLog << "percentCoverage: " << percent_coverage << "\n";
      tmpLog << "coverage metric: " << cmet << "\n";
      xml.StartElement("File");
      xml.Attribute("Name", sourceFile);
      xml.Attribute("FullPath", shortFileName);
      xml.Attribute("Covered", cmet > 0 ? "true" : "false");
      xml.Element("BranchesTested", branchCovered);
      xml.Element("BranchesUnTested", totalBranches - branchCovered);
      xml.Element("FunctionsTested", functionsCalled);
      xml.Element("FunctionsUnTested", totalFunctions - functionsCalled);
      // Hack for conversion of function to loc assume a function
      // has 100 lines of code
      xml.Element("LOCTested", functionsCalled * 100);
      xml.Element("LOCUnTested", (totalFunctions - functionsCalled) * 100);
      xml.Element("PercentCoverage", cper);
      xml.Element("CoverageMetric", cmet);
      this->WriteXMLLabels(xml, shortFileName);
      xml.EndElement(); // File
    }
  }
  std::string end_time = this->CTest->CurrentTime();
  xml.Element("LOCTested", total_tested);
  xml.Element("LOCUntested", total_untested);
  xml.Element("LOC", total_functions);
  xml.Element("PercentCoverage", SAFEDIV(percent_coverage, number_files));
  xml.Element("EndDateTime", end_time);
  xml.Element("EndTime", static_cast<unsigned int>(cmSystemTools::GetTime()));
  xml.Element(
    "ElapsedMinutes",
    static_cast<int>((cmSystemTools::GetTime() - elapsed_time_start) / 6) /
      10.0);
  xml.EndElement(); // Coverage
  this->CTest->EndXML(xml);

  // Now create the coverage information for each file
  return this->RunBullseyeCoverageBranch(cont, coveredFileNames, coveredFiles,
                                         coveredFilesFullPath);
}

int cmCTestCoverageHandler::HandleBullseyeCoverage(
  cmCTestCoverageHandlerContainer* cont)
{
  std::string covfile;
  if (!cmSystemTools::GetEnv("COVFILE", covfile) || covfile.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " COVFILE environment variable not found, not running "
                       " bullseye\n",
                       this->Quiet);
    return 0;
  }
  cmCTestOptionalLog(
    this->CTest, HANDLER_VERBOSE_OUTPUT,
    " run covsrc with COVFILE=[" << covfile << "]" << std::endl, this->Quiet);
  if (!this->RunBullseyeSourceSummary(cont)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error running bullseye summary.\n");
    return 0;
  }
  cmCTestOptionalLog(this->CTest, DEBUG,
                     "HandleBullseyeCoverage return 1 " << std::endl,
                     this->Quiet);
  return 1;
}

bool cmCTestCoverageHandler::GetNextInt(std::string const& inputLine,
                                        std::string::size_type& pos,
                                        int& value)
{
  std::string::size_type start = pos;
  pos = inputLine.find(',', start);
  value = atoi(inputLine.substr(start, pos).c_str());
  if (pos == std::string::npos) {
    return true;
  }
  pos++;
  return true;
}

bool cmCTestCoverageHandler::ParseBullsEyeCovsrcLine(
  std::string const& inputLine, std::string& sourceFile, int& functionsCalled,
  int& totalFunctions, int& percentFunction, int& branchCovered,
  int& totalBranches, int& percentBranch)
{
  // find the first comma
  std::string::size_type pos = inputLine.find(',');
  if (pos == std::string::npos) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error parsing string : " << inputLine << "\n");
    return false;
  }
  // the source file has "" around it so extract out the file name
  sourceFile = inputLine.substr(1, pos - 2);
  pos++;
  if (!this->GetNextInt(inputLine, pos, functionsCalled)) {
    return false;
  }
  if (!this->GetNextInt(inputLine, pos, totalFunctions)) {
    return false;
  }
  if (!this->GetNextInt(inputLine, pos, percentFunction)) {
    return false;
  }
  if (!this->GetNextInt(inputLine, pos, branchCovered)) {
    return false;
  }
  if (!this->GetNextInt(inputLine, pos, totalBranches)) {
    return false;
  }
  if (!this->GetNextInt(inputLine, pos, percentBranch)) {
    return false;
  }
  // should be at the end now
  if (pos != std::string::npos) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Error parsing input : "
                 << inputLine << " last pos not npos =  " << pos << "\n");
  }
  return true;
}

int cmCTestCoverageHandler::GetLabelId(std::string const& label)
{
  LabelIdMapType::iterator i = this->LabelIdMap.find(label);
  if (i == this->LabelIdMap.end()) {
    int n = int(this->Labels.size());
    this->Labels.push_back(label);
    LabelIdMapType::value_type entry(label, n);
    i = this->LabelIdMap.insert(entry).first;
  }
  return i->second;
}

void cmCTestCoverageHandler::LoadLabels()
{
  std::string fileList = this->CTest->GetBinaryDir();
  fileList += cmake::GetCMakeFilesDirectory();
  fileList += "/TargetDirectories.txt";
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " target directory list [" << fileList << "]\n",
                     this->Quiet);
  cmsys::ifstream finList(fileList.c_str());
  std::string line;
  while (cmSystemTools::GetLineFromStream(finList, line)) {
    this->LoadLabels(line.c_str());
  }
}

void cmCTestCoverageHandler::LoadLabels(const char* dir)
{
  LabelSet& dirLabels = this->TargetDirs[dir];
  std::string fname = dir;
  fname += "/Labels.txt";
  cmsys::ifstream fin(fname.c_str());
  if (!fin) {
    return;
  }

  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     " loading labels from [" << fname << "]\n", this->Quiet);
  bool inTarget = true;
  std::string source;
  std::string line;
  std::vector<int> targetLabels;
  while (cmSystemTools::GetLineFromStream(fin, line)) {
    if (line.empty() || line[0] == '#') {
      // Ignore blank and comment lines.
      continue;
    }
    if (line[0] == ' ') {
      // Label lines appear indented by one space.
      std::string label = line.substr(1);
      int id = this->GetLabelId(label);
      dirLabels.insert(id);
      if (inTarget) {
        targetLabels.push_back(id);
      } else {
        this->SourceLabels[source].insert(id);
      }
    } else {
      // Non-indented lines specify a source file name.  The first one
      // is the end of the target-wide labels.
      inTarget = false;

      source = this->CTest->GetShortPathToFile(line.c_str());

      // Label the source with the target labels.
      LabelSet& labelSet = this->SourceLabels[source];
      labelSet.insert(targetLabels.begin(), targetLabels.end());
    }
  }
}

void cmCTestCoverageHandler::WriteXMLLabels(cmXMLWriter& xml,
                                            std::string const& source)
{
  LabelMapType::const_iterator li = this->SourceLabels.find(source);
  if (li != this->SourceLabels.end() && !li->second.empty()) {
    xml.StartElement("Labels");
    for (LabelSet::const_iterator lsi = li->second.begin();
         lsi != li->second.end(); ++lsi) {
      xml.Element("Label", this->Labels[*lsi]);
    }
    xml.EndElement(); // Labels
  }
}

void cmCTestCoverageHandler::SetLabelFilter(
  std::set<std::string> const& labels)
{
  this->LabelFilter.clear();
  for (std::set<std::string>::const_iterator li = labels.begin();
       li != labels.end(); ++li) {
    this->LabelFilter.insert(this->GetLabelId(*li));
  }
}

bool cmCTestCoverageHandler::IntersectsFilter(LabelSet const& labels)
{
  // If there is no label filter then nothing is filtered out.
  if (this->LabelFilter.empty()) {
    return true;
  }

  std::vector<int> ids;
  std::set_intersection(labels.begin(), labels.end(),
                        this->LabelFilter.begin(), this->LabelFilter.end(),
                        std::back_inserter(ids));
  return !ids.empty();
}

bool cmCTestCoverageHandler::IsFilteredOut(std::string const& source)
{
  // If there is no label filter then nothing is filtered out.
  if (this->LabelFilter.empty()) {
    return false;
  }

  // The source is filtered out if it does not have any labels in
  // common with the filter set.
  std::string shortSrc = this->CTest->GetShortPathToFile(source.c_str());
  LabelMapType::const_iterator li = this->SourceLabels.find(shortSrc);
  if (li != this->SourceLabels.end()) {
    return !this->IntersectsFilter(li->second);
  }
  return true;
}

std::set<std::string> cmCTestCoverageHandler::FindUncoveredFiles(
  cmCTestCoverageHandlerContainer* cont)
{
  std::set<std::string> extraMatches;

  for (std::vector<std::string>::iterator i = this->ExtraCoverageGlobs.begin();
       i != this->ExtraCoverageGlobs.end(); ++i) {
    cmsys::Glob gl;
    gl.RecurseOn();
    gl.RecurseThroughSymlinksOff();
    std::string glob = cont->SourceDir + "/" + *i;
    gl.FindFiles(glob);
    std::vector<std::string> files = gl.GetFiles();
    for (std::vector<std::string>::iterator f = files.begin();
         f != files.end(); ++f) {
      if (this->ShouldIDoCoverage(f->c_str(), cont->SourceDir.c_str(),
                                  cont->BinaryDir.c_str())) {
        extraMatches.insert(this->CTest->GetShortPathToFile(f->c_str()));
      }
    }
  }

  if (!extraMatches.empty()) {
    for (cmCTestCoverageHandlerContainer::TotalCoverageMap::iterator i =
           cont->TotalCoverage.begin();
         i != cont->TotalCoverage.end(); ++i) {
      std::string shortPath =
        this->CTest->GetShortPathToFile(i->first.c_str());
      extraMatches.erase(shortPath);
    }
  }
  return extraMatches;
}
