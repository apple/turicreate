/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestBuildHandler.h"

#include "cmAlgorithms.h"
#include "cmCTest.h"
#include "cmFileTimeComparison.h"
#include "cmGeneratedFileStream.h"
#include "cmMakefile.h"
#include "cmProcessOutput.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/Process.h"
#include <set>
#include <stdlib.h>
#include <string.h>

static const char* cmCTestErrorMatches[] = {
  "^[Bb]us [Ee]rror",
  "^[Ss]egmentation [Vv]iolation",
  "^[Ss]egmentation [Ff]ault",
  ":.*[Pp]ermission [Dd]enied",
  "([^ :]+):([0-9]+): ([^ \\t])",
  "([^:]+): error[ \\t]*[0-9]+[ \\t]*:",
  "^Error ([0-9]+):",
  "^Fatal",
  "^Error: ",
  "^Error ",
  "[0-9] ERROR: ",
  "^\"[^\"]+\", line [0-9]+: [^Ww]",
  "^cc[^C]*CC: ERROR File = ([^,]+), Line = ([0-9]+)",
  "^ld([^:])*:([ \\t])*ERROR([^:])*:",
  "^ild:([ \\t])*\\(undefined symbol\\)",
  "([^ :]+) : (error|fatal error|catastrophic error)",
  "([^:]+): (Error:|error|undefined reference|multiply defined)",
  "([^:]+)\\(([^\\)]+)\\) ?: (error|fatal error|catastrophic error)",
  "^fatal error C[0-9]+:",
  ": syntax error ",
  "^collect2: ld returned 1 exit status",
  "ld terminated with signal",
  "Unsatisfied symbol",
  "^Unresolved:",
  "Undefined symbol",
  "^Undefined[ \\t]+first referenced",
  "^CMake Error.*:",
  ":[ \\t]cannot find",
  ":[ \\t]can't find",
  ": \\*\\*\\* No rule to make target [`'].*\\'.  Stop",
  ": \\*\\*\\* No targets specified and no makefile found",
  ": Invalid loader fixup for symbol",
  ": Invalid fixups exist",
  ": Can't find library for",
  ": internal link edit command failed",
  ": Unrecognized option [`'].*\\'",
  "\", line [0-9]+\\.[0-9]+: [0-9]+-[0-9]+ \\([^WI]\\)",
  "ld: 0706-006 Cannot find or open library file: -l ",
  "ild: \\(argument error\\) can't find library argument ::",
  "^could not be found and will not be loaded.",
  "s:616 string too big",
  "make: Fatal error: ",
  "ld: 0711-993 Error occurred while writing to the output file:",
  "ld: fatal: ",
  "final link failed:",
  "make: \\*\\*\\*.*Error",
  "make\\[.*\\]: \\*\\*\\*.*Error",
  "\\*\\*\\* Error code",
  "nternal error:",
  "Makefile:[0-9]+: \\*\\*\\* .*  Stop\\.",
  ": No such file or directory",
  ": Invalid argument",
  "^The project cannot be built\\.",
  "^\\[ERROR\\]",
  "^Command .* failed with exit code",
  CM_NULLPTR
};

static const char* cmCTestErrorExceptions[] = {
  "instantiated from ",
  "candidates are:",
  ": warning",
  ": \\(Warning\\)",
  ": note",
  "Note:",
  "makefile:",
  "Makefile:",
  ":[ \\t]+Where:",
  "([^ :]+):([0-9]+): Warning",
  "------ Build started: .* ------",
  CM_NULLPTR
};

static const char* cmCTestWarningMatches[] = {
  "([^ :]+):([0-9]+): warning:",
  "([^ :]+):([0-9]+): note:",
  "^cc[^C]*CC: WARNING File = ([^,]+), Line = ([0-9]+)",
  "^ld([^:])*:([ \\t])*WARNING([^:])*:",
  "([^:]+): warning ([0-9]+):",
  "^\"[^\"]+\", line [0-9]+: [Ww](arning|arnung)",
  "([^:]+): warning[ \\t]*[0-9]+[ \\t]*:",
  "^(Warning|Warnung) ([0-9]+):",
  "^(Warning|Warnung)[ :]",
  "WARNING: ",
  "([^ :]+) : warning",
  "([^:]+): warning",
  "\", line [0-9]+\\.[0-9]+: [0-9]+-[0-9]+ \\([WI]\\)",
  "^cxx: Warning:",
  ".*file: .* has no symbols",
  "([^ :]+):([0-9]+): (Warning|Warnung)",
  "\\([0-9]*\\): remark #[0-9]*",
  "\".*\", line [0-9]+: remark\\([0-9]*\\):",
  "cc-[0-9]* CC: REMARK File = .*, Line = [0-9]*",
  "^CMake Warning.*:",
  "^\\[WARNING\\]",
  CM_NULLPTR
};

static const char* cmCTestWarningExceptions[] = {
  "/usr/.*/X11/Xlib\\.h:[0-9]+: war.*: ANSI C\\+\\+ forbids declaration",
  "/usr/.*/X11/Xutil\\.h:[0-9]+: war.*: ANSI C\\+\\+ forbids declaration",
  "/usr/.*/X11/XResource\\.h:[0-9]+: war.*: ANSI C\\+\\+ forbids declaration",
  "WARNING 84 :",
  "WARNING 47 :",
  "makefile:",
  "Makefile:",
  "warning:  Clock skew detected.  Your build may be incomplete.",
  "/usr/openwin/include/GL/[^:]+:",
  "bind_at_load",
  "XrmQGetResource",
  "IceFlush",
  "warning LNK4089: all references to [^ \\t]+ discarded by .OPT:REF",
  "ld32: WARNING 85: definition of dataKey in",
  "cc: warning 422: Unknown option \"\\+b",
  "_with_warning_C",
  CM_NULLPTR
};

struct cmCTestBuildCompileErrorWarningRex
{
  const char* RegularExpressionString;
  int FileIndex;
  int LineIndex;
};

static cmCTestBuildCompileErrorWarningRex cmCTestWarningErrorFileLine[] = {
  { "^Warning W[0-9]+ ([a-zA-Z.\\:/0-9_+ ~-]+) ([0-9]+):", 1, 2 },
  { "^([a-zA-Z./0-9_+ ~-]+):([0-9]+):", 1, 2 },
  { "^([a-zA-Z.\\:/0-9_+ ~-]+)\\(([0-9]+)\\)", 1, 2 },
  { "^[0-9]+>([a-zA-Z.\\:/0-9_+ ~-]+)\\(([0-9]+)\\)", 1, 2 },
  { "^([a-zA-Z./0-9_+ ~-]+)\\(([0-9]+)\\)", 1, 2 },
  { "\"([a-zA-Z./0-9_+ ~-]+)\", line ([0-9]+)", 1, 2 },
  { "File = ([a-zA-Z./0-9_+ ~-]+), Line = ([0-9]+)", 1, 2 },
  { CM_NULLPTR, 0, 0 }
};

cmCTestBuildHandler::cmCTestBuildHandler()
{
  this->MaxPreContext = 10;
  this->MaxPostContext = 10;

  this->MaxErrors = 50;
  this->MaxWarnings = 50;

  this->LastErrorOrWarning = this->ErrorsAndWarnings.end();

  this->UseCTestLaunch = false;
}

void cmCTestBuildHandler::Initialize()
{
  this->Superclass::Initialize();
  this->StartBuild = "";
  this->EndBuild = "";
  this->CustomErrorMatches.clear();
  this->CustomErrorExceptions.clear();
  this->CustomWarningMatches.clear();
  this->CustomWarningExceptions.clear();
  this->ReallyCustomWarningMatches.clear();
  this->ReallyCustomWarningExceptions.clear();
  this->ErrorWarningFileLineRegex.clear();

  this->ErrorMatchRegex.clear();
  this->ErrorExceptionRegex.clear();
  this->WarningMatchRegex.clear();
  this->WarningExceptionRegex.clear();
  this->BuildProcessingQueue.clear();
  this->BuildProcessingErrorQueue.clear();
  this->BuildOutputLogSize = 0;
  this->CurrentProcessingLine.clear();

  this->SimplifySourceDir = "";
  this->SimplifyBuildDir = "";
  this->OutputLineCounter = 0;
  this->ErrorsAndWarnings.clear();
  this->LastErrorOrWarning = this->ErrorsAndWarnings.end();
  this->PostContextCount = 0;
  this->MaxPreContext = 10;
  this->MaxPostContext = 10;
  this->PreContext.clear();

  this->TotalErrors = 0;
  this->TotalWarnings = 0;
  this->LastTickChar = 0;

  this->ErrorQuotaReached = false;
  this->WarningQuotaReached = false;

  this->MaxErrors = 50;
  this->MaxWarnings = 50;

  this->UseCTestLaunch = false;
}

void cmCTestBuildHandler::PopulateCustomVectors(cmMakefile* mf)
{
  this->CTest->PopulateCustomVector(mf, "CTEST_CUSTOM_ERROR_MATCH",
                                    this->CustomErrorMatches);
  this->CTest->PopulateCustomVector(mf, "CTEST_CUSTOM_ERROR_EXCEPTION",
                                    this->CustomErrorExceptions);
  this->CTest->PopulateCustomVector(mf, "CTEST_CUSTOM_WARNING_MATCH",
                                    this->CustomWarningMatches);
  this->CTest->PopulateCustomVector(mf, "CTEST_CUSTOM_WARNING_EXCEPTION",
                                    this->CustomWarningExceptions);
  this->CTest->PopulateCustomInteger(
    mf, "CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS", this->MaxErrors);
  this->CTest->PopulateCustomInteger(
    mf, "CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS", this->MaxWarnings);

  int n = -1;
  this->CTest->PopulateCustomInteger(mf, "CTEST_CUSTOM_ERROR_PRE_CONTEXT", n);
  if (n != -1) {
    this->MaxPreContext = static_cast<size_t>(n);
  }

  n = -1;
  this->CTest->PopulateCustomInteger(mf, "CTEST_CUSTOM_ERROR_POST_CONTEXT", n);
  if (n != -1) {
    this->MaxPostContext = static_cast<size_t>(n);
  }

  // Record the user-specified custom warning rules.
  if (const char* customWarningMatchers =
        mf->GetDefinition("CTEST_CUSTOM_WARNING_MATCH")) {
    cmSystemTools::ExpandListArgument(customWarningMatchers,
                                      this->ReallyCustomWarningMatches);
  }
  if (const char* customWarningExceptions =
        mf->GetDefinition("CTEST_CUSTOM_WARNING_EXCEPTION")) {
    cmSystemTools::ExpandListArgument(customWarningExceptions,
                                      this->ReallyCustomWarningExceptions);
  }
}

std::string cmCTestBuildHandler::GetMakeCommand()
{
  std::string makeCommand = this->CTest->GetCTestConfiguration("MakeCommand");
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "MakeCommand:" << makeCommand << "\n", this->Quiet);

  std::string configType = this->CTest->GetConfigType();
  if (configType == "") {
    configType =
      this->CTest->GetCTestConfiguration("DefaultCTestConfigurationType");
  }
  if (configType == "") {
    configType = "Release";
  }

  cmSystemTools::ReplaceString(makeCommand, "${CTEST_CONFIGURATION_TYPE}",
                               configType.c_str());

  return makeCommand;
}

// clearly it would be nice if this were broken up into a few smaller
// functions and commented...
int cmCTestBuildHandler::ProcessHandler()
{
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "Build project" << std::endl,
                     this->Quiet);

  // do we have time for this
  if (this->CTest->GetRemainingTimeAllowed() < 120) {
    return 0;
  }

  int entry;
  for (entry = 0; cmCTestWarningErrorFileLine[entry].RegularExpressionString;
       ++entry) {
    cmCTestBuildHandler::cmCTestCompileErrorWarningRex r;
    if (r.RegularExpression.compile(
          cmCTestWarningErrorFileLine[entry].RegularExpressionString)) {
      r.FileIndex = cmCTestWarningErrorFileLine[entry].FileIndex;
      r.LineIndex = cmCTestWarningErrorFileLine[entry].LineIndex;
      this->ErrorWarningFileLineRegex.push_back(r);
    } else {
      cmCTestLog(
        this->CTest, ERROR_MESSAGE, "Problem Compiling regular expression: "
          << cmCTestWarningErrorFileLine[entry].RegularExpressionString
          << std::endl);
    }
  }

  // Determine build command and build directory
  std::string makeCommand = this->GetMakeCommand();
  if (makeCommand.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot find MakeCommand key in the DartConfiguration.tcl"
                 << std::endl);
    return -1;
  }

  const std::string& buildDirectory =
    this->CTest->GetCTestConfiguration("BuildDirectory");
  if (buildDirectory.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot find BuildDirectory  key in the DartConfiguration.tcl"
                 << std::endl);
    return -1;
  }

  std::string const& useLaunchers =
    this->CTest->GetCTestConfiguration("UseLaunchers");
  this->UseCTestLaunch = cmSystemTools::IsOn(useLaunchers.c_str());

  // Create a last build log
  cmGeneratedFileStream ofs;
  double elapsed_time_start = cmSystemTools::GetTime();
  if (!this->StartLogFile("Build", ofs)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot create build log file"
                 << std::endl);
  }

  // Create lists of regular expression strings for errors, error exceptions,
  // warnings and warning exceptions.
  std::vector<std::string>::size_type cc;
  for (cc = 0; cmCTestErrorMatches[cc]; cc++) {
    this->CustomErrorMatches.push_back(cmCTestErrorMatches[cc]);
  }
  for (cc = 0; cmCTestErrorExceptions[cc]; cc++) {
    this->CustomErrorExceptions.push_back(cmCTestErrorExceptions[cc]);
  }
  for (cc = 0; cmCTestWarningMatches[cc]; cc++) {
    this->CustomWarningMatches.push_back(cmCTestWarningMatches[cc]);
  }

  for (cc = 0; cmCTestWarningExceptions[cc]; cc++) {
    this->CustomWarningExceptions.push_back(cmCTestWarningExceptions[cc]);
  }

  // Pre-compile regular expressions objects for all regular expressions
  std::vector<std::string>::iterator it;

#define cmCTestBuildHandlerPopulateRegexVector(strings, regexes)              \
  regexes.clear();                                                            \
  cmCTestOptionalLog(this->CTest, DEBUG,                                      \
                     this << "Add " #regexes << std::endl, this->Quiet);      \
  for (it = (strings).begin(); it != (strings).end(); ++it) {                 \
    cmCTestOptionalLog(this->CTest, DEBUG,                                    \
                       "Add " #strings ": " << *it << std::endl,              \
                       this->Quiet);                                          \
    (regexes).push_back(it->c_str());                                         \
  }
  cmCTestBuildHandlerPopulateRegexVector(this->CustomErrorMatches,
                                         this->ErrorMatchRegex);
  cmCTestBuildHandlerPopulateRegexVector(this->CustomErrorExceptions,
                                         this->ErrorExceptionRegex);
  cmCTestBuildHandlerPopulateRegexVector(this->CustomWarningMatches,
                                         this->WarningMatchRegex);
  cmCTestBuildHandlerPopulateRegexVector(this->CustomWarningExceptions,
                                         this->WarningExceptionRegex);

  // Determine source and binary tree substitutions to simplify the output.
  this->SimplifySourceDir = "";
  this->SimplifyBuildDir = "";
  if (this->CTest->GetCTestConfiguration("SourceDirectory").size() > 20) {
    std::string srcdir =
      this->CTest->GetCTestConfiguration("SourceDirectory") + "/";
    std::string srcdirrep;
    for (cc = srcdir.size() - 2; cc > 0; cc--) {
      if (srcdir[cc] == '/') {
        srcdirrep = srcdir.c_str() + cc;
        srcdirrep = "/..." + srcdirrep;
        srcdir = srcdir.substr(0, cc + 1);
        break;
      }
    }
    this->SimplifySourceDir = srcdir;
  }
  if (this->CTest->GetCTestConfiguration("BuildDirectory").size() > 20) {
    std::string bindir =
      this->CTest->GetCTestConfiguration("BuildDirectory") + "/";
    std::string bindirrep;
    for (cc = bindir.size() - 2; cc > 0; cc--) {
      if (bindir[cc] == '/') {
        bindirrep = bindir.c_str() + cc;
        bindirrep = "/..." + bindirrep;
        bindir = bindir.substr(0, cc + 1);
        break;
      }
    }
    this->SimplifyBuildDir = bindir;
  }

  // Ok, let's do the build

  // Remember start build time
  this->StartBuild = this->CTest->CurrentTime();
  this->StartBuildTime = cmSystemTools::GetTime();
  int retVal = 0;
  int res = cmsysProcess_State_Exited;
  if (!this->CTest->GetShowOnly()) {
    res = this->RunMakeCommand(makeCommand.c_str(), &retVal,
                               buildDirectory.c_str(), 0, ofs);
  } else {
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Build with command: " << makeCommand << std::endl,
                       this->Quiet);
  }

  // Remember end build time and calculate elapsed time
  this->EndBuild = this->CTest->CurrentTime();
  this->EndBuildTime = cmSystemTools::GetTime();
  double elapsed_build_time = cmSystemTools::GetTime() - elapsed_time_start;

  // Cleanups strings in the errors and warnings list.
  t_ErrorsAndWarningsVector::iterator evit;
  if (!this->SimplifySourceDir.empty()) {
    for (evit = this->ErrorsAndWarnings.begin();
         evit != this->ErrorsAndWarnings.end(); ++evit) {
      cmSystemTools::ReplaceString(evit->Text, this->SimplifySourceDir.c_str(),
                                   "/.../");
      cmSystemTools::ReplaceString(evit->PreContext,
                                   this->SimplifySourceDir.c_str(), "/.../");
      cmSystemTools::ReplaceString(evit->PostContext,
                                   this->SimplifySourceDir.c_str(), "/.../");
    }
  }

  if (!this->SimplifyBuildDir.empty()) {
    for (evit = this->ErrorsAndWarnings.begin();
         evit != this->ErrorsAndWarnings.end(); ++evit) {
      cmSystemTools::ReplaceString(evit->Text, this->SimplifyBuildDir.c_str(),
                                   "/.../");
      cmSystemTools::ReplaceString(evit->PreContext,
                                   this->SimplifyBuildDir.c_str(), "/.../");
      cmSystemTools::ReplaceString(evit->PostContext,
                                   this->SimplifyBuildDir.c_str(), "/.../");
    }
  }

  // Generate XML output
  cmGeneratedFileStream xofs;
  if (!this->StartResultingXML(cmCTest::PartBuild, "Build", xofs)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot create build XML file"
                 << std::endl);
    return -1;
  }
  cmXMLWriter xml(xofs);
  this->GenerateXMLHeader(xml);
  if (this->UseCTestLaunch) {
    this->GenerateXMLLaunched(xml);
  } else {
    this->GenerateXMLLogScraped(xml);
  }
  this->GenerateXMLFooter(xml, elapsed_build_time);

  if (res != cmsysProcess_State_Exited || retVal || this->TotalErrors > 0) {
    cmCTestLog(this->CTest, ERROR_MESSAGE, "Error(s) when building project"
                 << std::endl);
  }

  // Display message about number of errors and warnings
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   "
               << this->TotalErrors
               << (this->TotalErrors >= this->MaxErrors ? " or more" : "")
               << " Compiler errors" << std::endl);
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   "
               << this->TotalWarnings
               << (this->TotalWarnings >= this->MaxWarnings ? " or more" : "")
               << " Compiler warnings" << std::endl);

  return retVal;
}

void cmCTestBuildHandler::GenerateXMLHeader(cmXMLWriter& xml)
{
  this->CTest->StartXML(xml, this->AppendXML);
  xml.StartElement("Build");
  xml.Element("StartDateTime", this->StartBuild);
  xml.Element("StartBuildTime",
              static_cast<unsigned int>(this->StartBuildTime));
  xml.Element("BuildCommand", this->GetMakeCommand());
}

class cmCTestBuildHandler::FragmentCompare
{
public:
  FragmentCompare(cmFileTimeComparison* ftc)
    : FTC(ftc)
  {
  }
  FragmentCompare()
    : FTC(CM_NULLPTR)
  {
  }
  bool operator()(std::string const& l, std::string const& r) const
  {
    // Order files by modification time.  Use lexicographic order
    // among files with the same time.
    int result;
    if (this->FTC->FileTimeCompare(l.c_str(), r.c_str(), &result) &&
        result != 0) {
      return result < 0;
    }
    return l < r;
  }

private:
  cmFileTimeComparison* FTC;
};

void cmCTestBuildHandler::GenerateXMLLaunched(cmXMLWriter& xml)
{
  if (this->CTestLaunchDir.empty()) {
    return;
  }

  // Sort XML fragments in chronological order.
  cmFileTimeComparison ftc;
  FragmentCompare fragmentCompare(&ftc);
  typedef std::set<std::string, FragmentCompare> Fragments;
  Fragments fragments(fragmentCompare);

  // only report the first 50 warnings and first 50 errors
  int numErrorsAllowed = this->MaxErrors;
  int numWarningsAllowed = this->MaxWarnings;
  // Identify fragments on disk.
  cmsys::Directory launchDir;
  launchDir.Load(this->CTestLaunchDir);
  unsigned long n = launchDir.GetNumberOfFiles();
  for (unsigned long i = 0; i < n; ++i) {
    const char* fname = launchDir.GetFile(i);
    if (this->IsLaunchedErrorFile(fname) && numErrorsAllowed) {
      numErrorsAllowed--;
      fragments.insert(this->CTestLaunchDir + "/" + fname);
      ++this->TotalErrors;
    } else if (this->IsLaunchedWarningFile(fname) && numWarningsAllowed) {
      numWarningsAllowed--;
      fragments.insert(this->CTestLaunchDir + "/" + fname);
      ++this->TotalWarnings;
    }
  }

  // Copy the fragments into the final XML file.
  for (Fragments::const_iterator fi = fragments.begin(); fi != fragments.end();
       ++fi) {
    xml.FragmentFile(fi->c_str());
  }
}

void cmCTestBuildHandler::GenerateXMLLogScraped(cmXMLWriter& xml)
{
  std::vector<cmCTestBuildErrorWarning>& ew = this->ErrorsAndWarnings;
  std::vector<cmCTestBuildErrorWarning>::iterator it;

  // only report the first 50 warnings and first 50 errors
  int numErrorsAllowed = this->MaxErrors;
  int numWarningsAllowed = this->MaxWarnings;
  std::string srcdir = this->CTest->GetCTestConfiguration("SourceDirectory");
  // make sure the source dir is in the correct case on windows
  // via a call to collapse full path.
  srcdir = cmSystemTools::CollapseFullPath(srcdir);
  srcdir += "/";
  for (it = ew.begin();
       it != ew.end() && (numErrorsAllowed || numWarningsAllowed); it++) {
    cmCTestBuildErrorWarning* cm = &(*it);
    if ((cm->Error && numErrorsAllowed) ||
        (!cm->Error && numWarningsAllowed)) {
      if (cm->Error) {
        numErrorsAllowed--;
      } else {
        numWarningsAllowed--;
      }
      xml.StartElement(cm->Error ? "Error" : "Warning");
      xml.Element("BuildLogLine", cm->LogLine);
      xml.Element("Text", cm->Text);
      std::vector<cmCTestCompileErrorWarningRex>::iterator rit;
      for (rit = this->ErrorWarningFileLineRegex.begin();
           rit != this->ErrorWarningFileLineRegex.end(); ++rit) {
        cmsys::RegularExpression* re = &rit->RegularExpression;
        if (re->find(cm->Text.c_str())) {
          cm->SourceFile = re->match(rit->FileIndex);
          // At this point we need to make this->SourceFile relative to
          // the source root of the project, so cvs links will work
          cmSystemTools::ConvertToUnixSlashes(cm->SourceFile);
          if (cm->SourceFile.find("/.../") != std::string::npos) {
            cmSystemTools::ReplaceString(cm->SourceFile, "/.../", "");
            std::string::size_type p = cm->SourceFile.find('/');
            if (p != std::string::npos) {
              cm->SourceFile =
                cm->SourceFile.substr(p + 1, cm->SourceFile.size() - p);
            }
          } else {
            // make sure it is a full path with the correct case
            cm->SourceFile = cmSystemTools::CollapseFullPath(cm->SourceFile);
            cmSystemTools::ReplaceString(cm->SourceFile, srcdir.c_str(), "");
          }
          cm->LineNumber = atoi(re->match(rit->LineIndex).c_str());
          break;
        }
      }
      if (!cm->SourceFile.empty() && cm->LineNumber >= 0) {
        if (!cm->SourceFile.empty()) {
          xml.Element("SourceFile", cm->SourceFile);
        }
        if (!cm->SourceFileTail.empty()) {
          xml.Element("SourceFileTail", cm->SourceFileTail);
        }
        if (cm->LineNumber >= 0) {
          xml.Element("SourceLineNumber", cm->LineNumber);
        }
      }
      xml.Element("PreContext", cm->PreContext);
      xml.StartElement("PostContext");
      xml.Content(cm->PostContext);
      // is this the last warning or error, if so notify
      if ((cm->Error && !numErrorsAllowed) ||
          (!cm->Error && !numWarningsAllowed)) {
        xml.Content("\nThe maximum number of reported warnings or errors "
                    "has been reached!!!\n");
      }
      xml.EndElement(); // PostContext
      xml.Element("RepeatCount", "0");
      xml.EndElement(); // "Error" / "Warning"
    }
  }
}

void cmCTestBuildHandler::GenerateXMLFooter(cmXMLWriter& xml,
                                            double elapsed_build_time)
{
  xml.StartElement("Log");
  xml.Attribute("Encoding", "base64");
  xml.Attribute("Compression", "bin/gzip");
  xml.EndElement(); // Log

  xml.Element("EndDateTime", this->EndBuild);
  xml.Element("EndBuildTime", static_cast<unsigned int>(this->EndBuildTime));
  xml.Element("ElapsedMinutes",
              static_cast<int>(elapsed_build_time / 6) / 10.0);
  xml.EndElement(); // Build
  this->CTest->EndXML(xml);
}

bool cmCTestBuildHandler::IsLaunchedErrorFile(const char* fname)
{
  // error-{hash}.xml
  return (cmHasLiteralPrefix(fname, "error-") &&
          strcmp(fname + strlen(fname) - 4, ".xml") == 0);
}

bool cmCTestBuildHandler::IsLaunchedWarningFile(const char* fname)
{
  // warning-{hash}.xml
  return (cmHasLiteralPrefix(fname, "warning-") &&
          strcmp(fname + strlen(fname) - 4, ".xml") == 0);
}

//######################################################################
//######################################################################
//######################################################################
//######################################################################

class cmCTestBuildHandler::LaunchHelper
{
public:
  LaunchHelper(cmCTestBuildHandler* handler);
  ~LaunchHelper();

private:
  cmCTestBuildHandler* Handler;
  cmCTest* CTest;

  void WriteLauncherConfig();
  void WriteScrapeMatchers(const char* purpose,
                           std::vector<std::string> const& matchers);
};

cmCTestBuildHandler::LaunchHelper::LaunchHelper(cmCTestBuildHandler* handler)
  : Handler(handler)
  , CTest(handler->CTest)
{
  std::string tag = this->CTest->GetCurrentTag();
  if (tag.empty()) {
    // This is not for a dashboard submission, so there is no XML.
    // Skip enabling the launchers.
    this->Handler->UseCTestLaunch = false;
  } else {
    // Compute a directory in which to store launcher fragments.
    std::string& launchDir = this->Handler->CTestLaunchDir;
    launchDir = this->CTest->GetBinaryDir();
    launchDir += "/Testing/";
    launchDir += tag;
    launchDir += "/Build";

    // Clean out any existing launcher fragments.
    cmSystemTools::RemoveADirectory(launchDir);

    if (this->Handler->UseCTestLaunch) {
      // Enable launcher fragments.
      cmSystemTools::MakeDirectory(launchDir.c_str());
      this->WriteLauncherConfig();
      std::string launchEnv = "CTEST_LAUNCH_LOGS=";
      launchEnv += launchDir;
      cmSystemTools::PutEnv(launchEnv);
    }
  }

  // If not using launchers, make sure they passthru.
  if (!this->Handler->UseCTestLaunch) {
    cmSystemTools::UnsetEnv("CTEST_LAUNCH_LOGS");
  }
}

cmCTestBuildHandler::LaunchHelper::~LaunchHelper()
{
  if (this->Handler->UseCTestLaunch) {
    cmSystemTools::UnsetEnv("CTEST_LAUNCH_LOGS");
  }
}

void cmCTestBuildHandler::LaunchHelper::WriteLauncherConfig()
{
  this->WriteScrapeMatchers("Warning",
                            this->Handler->ReallyCustomWarningMatches);
  this->WriteScrapeMatchers("WarningSuppress",
                            this->Handler->ReallyCustomWarningExceptions);

  // Give some testing configuration information to the launcher.
  std::string fname = this->Handler->CTestLaunchDir;
  fname += "/CTestLaunchConfig.cmake";
  cmGeneratedFileStream fout(fname.c_str());
  std::string srcdir = this->CTest->GetCTestConfiguration("SourceDirectory");
  fout << "set(CTEST_SOURCE_DIRECTORY \"" << srcdir << "\")\n";
}

void cmCTestBuildHandler::LaunchHelper::WriteScrapeMatchers(
  const char* purpose, std::vector<std::string> const& matchers)
{
  if (matchers.empty()) {
    return;
  }
  std::string fname = this->Handler->CTestLaunchDir;
  fname += "/Custom";
  fname += purpose;
  fname += ".txt";
  cmGeneratedFileStream fout(fname.c_str());
  for (std::vector<std::string>::const_iterator mi = matchers.begin();
       mi != matchers.end(); ++mi) {
    fout << *mi << "\n";
  }
}

int cmCTestBuildHandler::RunMakeCommand(const char* command, int* retVal,
                                        const char* dir, int timeout,
                                        std::ostream& ofs, Encoding encoding)
{
  // First generate the command and arguments
  std::vector<std::string> args = cmSystemTools::ParseArguments(command);

  if (args.empty()) {
    return false;
  }

  std::vector<const char*> argv;
  for (std::vector<std::string>::const_iterator a = args.begin();
       a != args.end(); ++a) {
    argv.push_back(a->c_str());
  }
  argv.push_back(CM_NULLPTR);

  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Run command:",
                     this->Quiet);
  std::vector<const char*>::iterator ait;
  for (ait = argv.begin(); ait != argv.end() && *ait; ++ait) {
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       " \"" << *ait << "\"", this->Quiet);
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, std::endl,
                     this->Quiet);

  // Optionally use make rule launchers to record errors and warnings.
  LaunchHelper launchHelper(this);
  static_cast<void>(launchHelper);

  // Now create process object
  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmsysProcess_SetWorkingDirectory(cp, dir);
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  // Initialize tick's
  std::string::size_type tick = 0;
  const std::string::size_type tick_len = 1024;

  char* data;
  int length;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  cmCTestOptionalLog(
    this->CTest, HANDLER_PROGRESS_OUTPUT, "   Each symbol represents "
      << tick_len << " bytes of output." << std::endl
      << (this->UseCTestLaunch
            ? ""
            : "   '!' represents an error and '*' a warning.\n")
      << "    " << std::flush,
    this->Quiet);

  // Initialize building structures
  this->BuildProcessingQueue.clear();
  this->OutputLineCounter = 0;
  this->ErrorsAndWarnings.clear();
  this->TotalErrors = 0;
  this->TotalWarnings = 0;
  this->BuildOutputLogSize = 0;
  this->LastTickChar = '.';
  this->WarningQuotaReached = false;
  this->ErrorQuotaReached = false;

  // For every chunk of data
  int res;
  while ((res = cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR))) {
    // Replace '\0' with '\n', since '\0' does not really make sense. This is
    // for Visual Studio output
    for (int cc = 0; cc < length; ++cc) {
      if (data[cc] == 0) {
        data[cc] = '\n';
      }
    }

    // Process the chunk of data
    if (res == cmsysProcess_Pipe_STDERR) {
      processOutput.DecodeText(data, length, strdata, 1);
      this->ProcessBuffer(strdata.c_str(), strdata.size(), tick, tick_len, ofs,
                          &this->BuildProcessingErrorQueue);
    } else {
      processOutput.DecodeText(data, length, strdata, 2);
      this->ProcessBuffer(strdata.c_str(), strdata.size(), tick, tick_len, ofs,
                          &this->BuildProcessingQueue);
    }
  }
  processOutput.DecodeText(std::string(), strdata, 1);
  if (!strdata.empty()) {
    this->ProcessBuffer(strdata.c_str(), strdata.size(), tick, tick_len, ofs,
                        &this->BuildProcessingErrorQueue);
  }
  processOutput.DecodeText(std::string(), strdata, 2);
  if (!strdata.empty()) {
    this->ProcessBuffer(strdata.c_str(), strdata.size(), tick, tick_len, ofs,
                        &this->BuildProcessingQueue);
  }

  this->ProcessBuffer(CM_NULLPTR, 0, tick, tick_len, ofs,
                      &this->BuildProcessingQueue);
  this->ProcessBuffer(CM_NULLPTR, 0, tick, tick_len, ofs,
                      &this->BuildProcessingErrorQueue);
  cmCTestOptionalLog(this->CTest, HANDLER_PROGRESS_OUTPUT, " Size of output: "
                       << ((this->BuildOutputLogSize + 512) / 1024) << "K"
                       << std::endl,
                     this->Quiet);

  // Properly handle output of the build command
  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  int result = cmsysProcess_GetState(cp);

  if (result == cmsysProcess_State_Exited) {
    if (retVal) {
      *retVal = cmsysProcess_GetExitValue(cp);
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Command exited with the value: " << *retVal
                                                           << std::endl,
                         this->Quiet);
      // if a non zero return value
      if (*retVal) {
        // If there was an error running command, report that on the
        // dashboard.
        cmCTestBuildErrorWarning errorwarning;
        errorwarning.LogLine = 1;
        errorwarning.Text =
          "*** WARNING non-zero return value in ctest from: ";
        errorwarning.Text += argv[0];
        errorwarning.PreContext = "";
        errorwarning.PostContext = "";
        errorwarning.Error = false;
        this->ErrorsAndWarnings.push_back(errorwarning);
        this->TotalWarnings++;
      }
    }
  } else if (result == cmsysProcess_State_Exception) {
    if (retVal) {
      *retVal = cmsysProcess_GetExitException(cp);
      cmCTestOptionalLog(this->CTest, WARNING,
                         "There was an exception: " << *retVal << std::endl,
                         this->Quiet);
    }
  } else if (result == cmsysProcess_State_Expired) {
    cmCTestOptionalLog(this->CTest, WARNING,
                       "There was a timeout" << std::endl, this->Quiet);
  } else if (result == cmsysProcess_State_Error) {
    // If there was an error running command, report that on the dashboard.
    cmCTestBuildErrorWarning errorwarning;
    errorwarning.LogLine = 1;
    errorwarning.Text = "*** ERROR executing: ";
    errorwarning.Text += cmsysProcess_GetErrorString(cp);
    errorwarning.PreContext = "";
    errorwarning.PostContext = "";
    errorwarning.Error = true;
    this->ErrorsAndWarnings.push_back(errorwarning);
    this->TotalErrors++;
    cmCTestLog(this->CTest, ERROR_MESSAGE, "There was an error: "
                 << cmsysProcess_GetErrorString(cp) << std::endl);
  }

  cmsysProcess_Delete(cp);
  return result;
}

//######################################################################
//######################################################################
//######################################################################
//######################################################################

void cmCTestBuildHandler::ProcessBuffer(const char* data, size_t length,
                                        size_t& tick, size_t tick_len,
                                        std::ostream& ofs,
                                        t_BuildProcessingQueueType* queue)
{
  const std::string::size_type tick_line_len = 50;
  const char* ptr;
  for (ptr = data; ptr < data + length; ptr++) {
    queue->push_back(*ptr);
  }
  this->BuildOutputLogSize += length;

  // until there are any lines left in the buffer
  while (true) {
    // Find the end of line
    t_BuildProcessingQueueType::iterator it;
    for (it = queue->begin(); it != queue->end(); ++it) {
      if (*it == '\n') {
        break;
      }
    }

    // Once certain number of errors or warnings reached, ignore future errors
    // or warnings.
    if (this->TotalWarnings >= this->MaxWarnings) {
      this->WarningQuotaReached = true;
    }
    if (this->TotalErrors >= this->MaxErrors) {
      this->ErrorQuotaReached = true;
    }

    // If the end of line was found
    if (it != queue->end()) {
      // Create a contiguous array for the line
      this->CurrentProcessingLine.clear();
      this->CurrentProcessingLine.insert(this->CurrentProcessingLine.end(),
                                         queue->begin(), it);
      this->CurrentProcessingLine.push_back(0);
      const char* line = &*this->CurrentProcessingLine.begin();

      // Process the line
      int lineType = this->ProcessSingleLine(line);

      // Erase the line from the queue
      queue->erase(queue->begin(), it + 1);

      // Depending on the line type, produce error or warning, or nothing
      cmCTestBuildErrorWarning errorwarning;
      bool found = false;
      switch (lineType) {
        case b_WARNING_LINE:
          this->LastTickChar = '*';
          errorwarning.Error = false;
          found = true;
          this->TotalWarnings++;
          break;
        case b_ERROR_LINE:
          this->LastTickChar = '!';
          errorwarning.Error = true;
          found = true;
          this->TotalErrors++;
          break;
      }
      if (found) {
        // This is an error or warning, so generate report
        errorwarning.LogLine = static_cast<int>(this->OutputLineCounter + 1);
        errorwarning.Text = line;
        errorwarning.PreContext = "";
        errorwarning.PostContext = "";

        // Copy pre-context to report
        std::deque<std::string>::iterator pcit;
        for (pcit = this->PreContext.begin(); pcit != this->PreContext.end();
             ++pcit) {
          errorwarning.PreContext += *pcit + "\n";
        }
        this->PreContext.clear();

        // Store report
        this->ErrorsAndWarnings.push_back(errorwarning);
        this->LastErrorOrWarning = this->ErrorsAndWarnings.end() - 1;
        this->PostContextCount = 0;
      } else {
        // This is not an error or warning.
        // So, figure out if this is a post-context line
        if (!this->ErrorsAndWarnings.empty() &&
            this->LastErrorOrWarning != this->ErrorsAndWarnings.end() &&
            this->PostContextCount < this->MaxPostContext) {
          this->PostContextCount++;
          this->LastErrorOrWarning->PostContext += line;
          if (this->PostContextCount < this->MaxPostContext) {
            this->LastErrorOrWarning->PostContext += "\n";
          }
        } else {
          // Otherwise store pre-context for the next error
          this->PreContext.push_back(line);
          if (this->PreContext.size() > this->MaxPreContext) {
            this->PreContext.erase(this->PreContext.begin(),
                                   this->PreContext.end() -
                                     this->MaxPreContext);
          }
        }
      }
      this->OutputLineCounter++;
    } else {
      break;
    }
  }

  // Now that the buffer is processed, display missing ticks
  int tickDisplayed = false;
  while (this->BuildOutputLogSize > (tick * tick_len)) {
    tick++;
    cmCTestOptionalLog(this->CTest, HANDLER_PROGRESS_OUTPUT,
                       this->LastTickChar, this->Quiet);
    tickDisplayed = true;
    if (tick % tick_line_len == 0 && tick > 0) {
      cmCTestOptionalLog(this->CTest, HANDLER_PROGRESS_OUTPUT, "  Size: "
                           << ((this->BuildOutputLogSize + 512) / 1024) << "K"
                           << std::endl
                           << "    ",
                         this->Quiet);
    }
  }
  if (tickDisplayed) {
    this->LastTickChar = '.';
  }

  // And if this is verbose output, display the content of the chunk
  cmCTestLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
             cmCTestLogWrite(data, length));

  // Always store the chunk to the file
  ofs << cmCTestLogWrite(data, length);
}

int cmCTestBuildHandler::ProcessSingleLine(const char* data)
{
  if (this->UseCTestLaunch) {
    // No log scraping when using launchers.
    return b_REGULAR_LINE;
  }

  cmCTestOptionalLog(this->CTest, DEBUG, "Line: [" << data << "]" << std::endl,
                     this->Quiet);

  std::vector<cmsys::RegularExpression>::iterator it;

  int warningLine = 0;
  int errorLine = 0;

  // Check for regular expressions

  if (!this->ErrorQuotaReached) {
    // Errors
    int wrxCnt = 0;
    for (it = this->ErrorMatchRegex.begin(); it != this->ErrorMatchRegex.end();
         ++it) {
      if (it->find(data)) {
        errorLine = 1;
        cmCTestOptionalLog(this->CTest, DEBUG,
                           "  Error Line: " << data << " (matches: "
                                            << this->CustomErrorMatches[wrxCnt]
                                            << ")" << std::endl,
                           this->Quiet);
        break;
      }
      wrxCnt++;
    }
    // Error exceptions
    wrxCnt = 0;
    for (it = this->ErrorExceptionRegex.begin();
         it != this->ErrorExceptionRegex.end(); ++it) {
      if (it->find(data)) {
        errorLine = 0;
        cmCTestOptionalLog(this->CTest, DEBUG, "  Not an error Line: "
                             << data << " (matches: "
                             << this->CustomErrorExceptions[wrxCnt] << ")"
                             << std::endl,
                           this->Quiet);
        break;
      }
      wrxCnt++;
    }
  }
  if (!this->WarningQuotaReached) {
    // Warnings
    int wrxCnt = 0;
    for (it = this->WarningMatchRegex.begin();
         it != this->WarningMatchRegex.end(); ++it) {
      if (it->find(data)) {
        warningLine = 1;
        cmCTestOptionalLog(this->CTest, DEBUG, "  Warning Line: "
                             << data << " (matches: "
                             << this->CustomWarningMatches[wrxCnt] << ")"
                             << std::endl,
                           this->Quiet);
        break;
      }
      wrxCnt++;
    }

    wrxCnt = 0;
    // Warning exceptions
    for (it = this->WarningExceptionRegex.begin();
         it != this->WarningExceptionRegex.end(); ++it) {
      if (it->find(data)) {
        warningLine = 0;
        cmCTestOptionalLog(this->CTest, DEBUG, "  Not a warning Line: "
                             << data << " (matches: "
                             << this->CustomWarningExceptions[wrxCnt] << ")"
                             << std::endl,
                           this->Quiet);
        break;
      }
      wrxCnt++;
    }
  }
  if (errorLine) {
    return b_ERROR_LINE;
  }
  if (warningLine) {
    return b_WARNING_LINE;
  }
  return b_REGULAR_LINE;
}
