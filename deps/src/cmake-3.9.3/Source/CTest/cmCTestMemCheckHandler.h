/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestMemCheckHandler_h
#define cmCTestMemCheckHandler_h

#include "cmConfigure.h"

#include "cmCTestTestHandler.h"

#include <string>
#include <vector>

class cmMakefile;
class cmXMLWriter;

/** \class cmCTestMemCheckHandler
 * \brief A class that handles ctest -S invocations
 *
 */
class cmCTestMemCheckHandler : public cmCTestTestHandler
{
  friend class cmCTestRunTest;

public:
  typedef cmCTestTestHandler Superclass;

  void PopulateCustomVectors(cmMakefile* mf) CM_OVERRIDE;

  cmCTestMemCheckHandler();

  void Initialize() CM_OVERRIDE;

  int GetDefectCount();

protected:
  int PreProcessHandler() CM_OVERRIDE;
  int PostProcessHandler() CM_OVERRIDE;
  void GenerateTestCommand(std::vector<std::string>& args,
                           int test) CM_OVERRIDE;

private:
  enum
  { // Memory checkers
    UNKNOWN = 0,
    VALGRIND,
    PURIFY,
    BOUNDS_CHECKER,
    // checkers after here do not use the standard error list
    ADDRESS_SANITIZER,
    LEAK_SANITIZER,
    THREAD_SANITIZER,
    MEMORY_SANITIZER,
    UB_SANITIZER
  };

public:
  enum
  { // Memory faults
    ABR = 0,
    ABW,
    ABWL,
    COR,
    EXU,
    FFM,
    FIM,
    FMM,
    FMR,
    FMW,
    FUM,
    IPR,
    IPW,
    MAF,
    MLK,
    MPK,
    NPR,
    ODS,
    PAR,
    PLK,
    UMC,
    UMR,
    NO_MEMORY_FAULT
  };

private:
  enum
  { // Program statuses
    NOT_RUN = 0,
    TIMEOUT,
    SEGFAULT,
    ILLEGAL,
    INTERRUPT,
    NUMERICAL,
    OTHER_FAULT,
    FAILED,
    BAD_COMMAND,
    COMPLETED
  };
  std::string BoundsCheckerDPBDFile;
  std::string BoundsCheckerXMLFile;
  std::string MemoryTester;
  std::vector<std::string> MemoryTesterDynamicOptions;
  std::vector<std::string> MemoryTesterOptions;
  int MemoryTesterStyle;
  std::string MemoryTesterOutputFile;
  std::string MemoryTesterEnvironmentVariable;
  // these are used to store the types of errors that can show up
  std::vector<std::string> ResultStrings;
  std::vector<std::string> ResultStringsLong;
  std::vector<int> GlobalResults;
  bool LogWithPID; // does log file add pid
  int DefectCount;

  std::vector<int>::size_type FindOrAddWarning(const std::string& warning);
  // initialize the ResultStrings and ResultStringsLong for
  // this type of checker
  void InitializeResultsVectors();

  ///! Initialize memory checking subsystem.
  bool InitializeMemoryChecking();

  /**
   * Generate the Dart compatible output
   */
  void GenerateDartOutput(cmXMLWriter& xml) CM_OVERRIDE;

  std::vector<std::string> CustomPreMemCheck;
  std::vector<std::string> CustomPostMemCheck;

  //! Parse Valgrind/Purify/Bounds Checker result out of the output
  // string. After running, log holds the output and results hold the
  // different memmory errors.
  bool ProcessMemCheckOutput(const std::string& str, std::string& log,
                             std::vector<int>& results);
  bool ProcessMemCheckValgrindOutput(const std::string& str, std::string& log,
                                     std::vector<int>& results);
  bool ProcessMemCheckPurifyOutput(const std::string& str, std::string& log,
                                   std::vector<int>& results);
  bool ProcessMemCheckSanitizerOutput(const std::string& str, std::string& log,
                                      std::vector<int>& results);
  bool ProcessMemCheckBoundsCheckerOutput(const std::string& str,
                                          std::string& log,
                                          std::vector<int>& results);

  void PostProcessTest(cmCTestTestResult& res, int test);
  void PostProcessBoundsCheckerTest(cmCTestTestResult& res, int test);

  ///! append MemoryTesterOutputFile to the test log
  void AppendMemTesterOutput(cmCTestTestHandler::cmCTestTestResult& res,
                             std::string const& filename);

  ///! generate the output filename for the given test index
  void TestOutputFileNames(int test, std::vector<std::string>& files);
};

#endif
