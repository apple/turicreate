/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestBuildHandler_h
#define cmCTestBuildHandler_h

#include "cmConfigure.h"

#include "cmCTestGenericHandler.h"

#include "cmProcessOutput.h"
#include "cmsys/RegularExpression.hxx"
#include <deque>
#include <iosfwd>
#include <stddef.h>
#include <string>
#include <vector>

class cmMakefile;
class cmXMLWriter;

/** \class cmCTestBuildHandler
 * \brief A class that handles ctest -S invocations
 *
 */
class cmCTestBuildHandler : public cmCTestGenericHandler
{
public:
  typedef cmCTestGenericHandler Superclass;
  typedef cmProcessOutput::Encoding Encoding;

  /*
   * The main entry point for this class
   */
  int ProcessHandler() CM_OVERRIDE;

  cmCTestBuildHandler();

  void PopulateCustomVectors(cmMakefile* mf) CM_OVERRIDE;

  /**
   * Initialize handler
   */
  void Initialize() CM_OVERRIDE;

  int GetTotalErrors() { return this->TotalErrors; }
  int GetTotalWarnings() { return this->TotalWarnings; }

private:
  std::string GetMakeCommand();

  //! Run command specialized for make and configure. Returns process status
  // and retVal is return value or exception.
  int RunMakeCommand(const char* command, int* retVal, const char* dir,
                     int timeout, std::ostream& ofs,
                     Encoding encoding = cmProcessOutput::Auto);

  enum
  {
    b_REGULAR_LINE,
    b_WARNING_LINE,
    b_ERROR_LINE
  };

  class cmCTestCompileErrorWarningRex
  {
  public:
    cmCTestCompileErrorWarningRex() {}
    int FileIndex;
    int LineIndex;
    cmsys::RegularExpression RegularExpression;
  };

  struct cmCTestBuildErrorWarning
  {
    bool Error;
    int LogLine;
    std::string Text;
    std::string SourceFile;
    std::string SourceFileTail;
    int LineNumber;
    std::string PreContext;
    std::string PostContext;
  };

  // generate the XML output
  void GenerateXMLHeader(cmXMLWriter& xml);
  void GenerateXMLLaunched(cmXMLWriter& xml);
  void GenerateXMLLogScraped(cmXMLWriter& xml);
  void GenerateXMLFooter(cmXMLWriter& xml, double elapsed_build_time);
  bool IsLaunchedErrorFile(const char* fname);
  bool IsLaunchedWarningFile(const char* fname);

  std::string StartBuild;
  std::string EndBuild;
  double StartBuildTime;
  double EndBuildTime;

  std::vector<std::string> CustomErrorMatches;
  std::vector<std::string> CustomErrorExceptions;
  std::vector<std::string> CustomWarningMatches;
  std::vector<std::string> CustomWarningExceptions;
  std::vector<std::string> ReallyCustomWarningMatches;
  std::vector<std::string> ReallyCustomWarningExceptions;
  std::vector<cmCTestCompileErrorWarningRex> ErrorWarningFileLineRegex;

  std::vector<cmsys::RegularExpression> ErrorMatchRegex;
  std::vector<cmsys::RegularExpression> ErrorExceptionRegex;
  std::vector<cmsys::RegularExpression> WarningMatchRegex;
  std::vector<cmsys::RegularExpression> WarningExceptionRegex;

  typedef std::deque<char> t_BuildProcessingQueueType;

  void ProcessBuffer(const char* data, size_t length, size_t& tick,
                     size_t tick_len, std::ostream& ofs,
                     t_BuildProcessingQueueType* queue);
  int ProcessSingleLine(const char* data);

  t_BuildProcessingQueueType BuildProcessingQueue;
  t_BuildProcessingQueueType BuildProcessingErrorQueue;
  size_t BuildOutputLogSize;
  std::vector<char> CurrentProcessingLine;

  std::string SimplifySourceDir;
  std::string SimplifyBuildDir;
  size_t OutputLineCounter;
  typedef std::vector<cmCTestBuildErrorWarning> t_ErrorsAndWarningsVector;
  t_ErrorsAndWarningsVector ErrorsAndWarnings;
  t_ErrorsAndWarningsVector::iterator LastErrorOrWarning;
  size_t PostContextCount;
  size_t MaxPreContext;
  size_t MaxPostContext;
  std::deque<std::string> PreContext;

  int TotalErrors;
  int TotalWarnings;
  char LastTickChar;

  bool ErrorQuotaReached;
  bool WarningQuotaReached;

  int MaxErrors;
  int MaxWarnings;

  bool UseCTestLaunch;
  std::string CTestLaunchDir;
  class LaunchHelper;

  friend class LaunchHelper;
  class FragmentCompare;
};

#endif
