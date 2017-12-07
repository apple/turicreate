/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestTestHandler_h
#define cmCTestTestHandler_h

#include "cmConfigure.h"

#include "cmCTestGenericHandler.h"

#include "cmsys/RegularExpression.hxx"
#include <iosfwd>
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

class cmCTest;
class cmMakefile;
class cmXMLWriter;

/** \class cmCTestTestHandler
 * \brief A class that handles ctest -S invocations
 *
 */
class cmCTestTestHandler : public cmCTestGenericHandler
{
  friend class cmCTestRunTest;
  friend class cmCTestMultiProcessHandler;
  friend class cmCTestBatchTestHandler;

public:
  typedef cmCTestGenericHandler Superclass;

  /**
   * The main entry point for this class
   */
  int ProcessHandler() CM_OVERRIDE;

  /**
   * When both -R and -I are used should te resulting test list be the
   * intersection or the union of the lists. By default it is the
   * intersection.
   */
  void SetUseUnion(bool val) { this->UseUnion = val; }

  /**
   * Set whether or not CTest should only execute the tests that failed
   * on the previous run.  By default this is false.
   */
  void SetRerunFailed(bool val) { this->RerunFailed = val; }

  /**
   * This method is called when reading CTest custom file
   */
  void PopulateCustomVectors(cmMakefile* mf) CM_OVERRIDE;

  ///! Control the use of the regular expresisons, call these methods to turn
  /// them on
  void UseIncludeRegExp();
  void UseExcludeRegExp();
  void SetIncludeRegExp(const char*);
  void SetExcludeRegExp(const char*);

  void SetMaxIndex(int n) { this->MaxIndex = n; }
  int GetMaxIndex() { return this->MaxIndex; }

  void SetTestOutputSizePassed(int n)
  {
    this->CustomMaximumPassedTestOutputSize = n;
  }
  void SetTestOutputSizeFailed(int n)
  {
    this->CustomMaximumFailedTestOutputSize = n;
  }

  ///! pass the -I argument down
  void SetTestsToRunInformation(const char*);

  cmCTestTestHandler();

  /*
   * Add the test to the list of tests to be executed
   */
  bool AddTest(const std::vector<std::string>& args);

  /*
   * Set tests properties
   */
  bool SetTestsProperties(const std::vector<std::string>& args);

  void Initialize() CM_OVERRIDE;

  // NOTE: This struct is Saved/Restored
  // in cmCTestTestHandler, if you add to this class
  // then you must add the new members to that code or
  // ctest -j N will break for that feature
  struct cmCTestTestProperties
  {
    std::string Name;
    std::string Directory;
    std::vector<std::string> Args;
    std::vector<std::string> RequiredFiles;
    std::vector<std::string> Depends;
    std::vector<std::string> AttachedFiles;
    std::vector<std::string> AttachOnFail;
    std::vector<std::pair<cmsys::RegularExpression, std::string> >
      ErrorRegularExpressions;
    std::vector<std::pair<cmsys::RegularExpression, std::string> >
      RequiredRegularExpressions;
    std::vector<std::pair<cmsys::RegularExpression, std::string> >
      TimeoutRegularExpressions;
    std::map<std::string, std::string> Measurements;
    bool IsInBasedOnREOptions;
    bool WillFail;
    bool Disabled;
    float Cost;
    int PreviousRuns;
    bool RunSerial;
    double Timeout;
    bool ExplicitTimeout;
    double AlternateTimeout;
    int Index;
    // Requested number of process slots
    int Processors;
    // return code of test which will mark test as "not run"
    int SkipReturnCode;
    std::vector<std::string> Environment;
    std::vector<std::string> Labels;
    std::set<std::string> LockedResources;
    std::set<std::string> FixturesSetup;
    std::set<std::string> FixturesCleanup;
    std::set<std::string> FixturesRequired;
    std::set<std::string> RequireSuccessDepends;
  };

  struct cmCTestTestResult
  {
    std::string Name;
    std::string Path;
    std::string Reason;
    std::string FullCommandLine;
    double ExecutionTime;
    int ReturnValue;
    int Status;
    bool CompressOutput;
    std::string CompletionStatus;
    std::string Output;
    std::string DartString;
    int TestCount;
    cmCTestTestProperties* Properties;
  };

  struct cmCTestTestResultLess
  {
    bool operator()(const cmCTestTestResult& lhs,
                    const cmCTestTestResult& rhs) const
    {
      return lhs.TestCount < rhs.TestCount;
    }
  };

  // add configurations to a search path for an executable
  static void AddConfigurations(cmCTest* ctest,
                                std::vector<std::string>& attempted,
                                std::vector<std::string>& attemptedConfigs,
                                std::string filepath, std::string& filename);

  // full signature static method to find an executable
  static std::string FindExecutable(cmCTest* ctest, const char* testCommand,
                                    std::string& resultingConfig,
                                    std::vector<std::string>& extraPaths,
                                    std::vector<std::string>& failed);

  typedef std::vector<cmCTestTestProperties> ListOfTests;

protected:
  // compute a final test list
  virtual int PreProcessHandler();
  virtual int PostProcessHandler();
  virtual void GenerateTestCommand(std::vector<std::string>& args, int test);
  int ExecuteCommands(std::vector<std::string>& vec);

  void WriteTestResultHeader(cmXMLWriter& xml, cmCTestTestResult* result);
  void WriteTestResultFooter(cmXMLWriter& xml, cmCTestTestResult* result);
  // Write attached test files into the xml
  void AttachFiles(cmXMLWriter& xml, cmCTestTestResult* result);

  //! Clean test output to specified length
  bool CleanTestOutput(std::string& output, size_t length);

  double ElapsedTestingTime;

  typedef std::vector<cmCTestTestResult> TestResultsVector;
  TestResultsVector TestResults;

  std::vector<std::string> CustomTestsIgnore;
  std::string StartTest;
  std::string EndTest;
  unsigned int StartTestTime;
  unsigned int EndTestTime;
  bool MemCheck;
  int CustomMaximumPassedTestOutputSize;
  int CustomMaximumFailedTestOutputSize;
  int MaxIndex;

public:
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

private:
  /**
   * Generate the Dart compatible output
   */
  virtual void GenerateDartOutput(cmXMLWriter& xml);

  void PrintLabelSummary();
  /**
   * Run the tests for a directory and any subdirectories
   */
  void ProcessDirectory(std::vector<std::string>& passed,
                        std::vector<std::string>& failed);

  /**
   * Get the list of tests in directory and subdirectories.
   */
  void GetListOfTests();
  // compute the lists of tests that will actually run
  // based on union regex and -I stuff
  void ComputeTestList();

  // compute the lists of tests that will actually run
  // based on LastTestFailed.log
  void ComputeTestListForRerunFailed();

  // add required setup/cleanup tests not already in the
  // list of tests to be run and update dependencies between
  // tests to account for fixture setup/cleanup
  void UpdateForFixtures(ListOfTests& tests) const;

  void UpdateMaxTestNameWidth();

  bool GetValue(const char* tag, std::string& value, std::istream& fin);
  bool GetValue(const char* tag, int& value, std::istream& fin);
  bool GetValue(const char* tag, size_t& value, std::istream& fin);
  bool GetValue(const char* tag, bool& value, std::istream& fin);
  bool GetValue(const char* tag, double& value, std::istream& fin);
  /**
   * Find the executable for a test
   */
  std::string FindTheExecutable(const char* exe);

  const char* GetTestStatus(int status);
  void ExpandTestsToRunInformation(size_t numPossibleTests);
  void ExpandTestsToRunInformationForRerunFailed();

  std::vector<std::string> CustomPreTest;
  std::vector<std::string> CustomPostTest;

  std::vector<int> TestsToRun;

  bool UseIncludeLabelRegExpFlag;
  bool UseExcludeLabelRegExpFlag;
  bool UseIncludeRegExpFlag;
  bool UseExcludeRegExpFlag;
  bool UseExcludeRegExpFirst;
  std::string IncludeLabelRegExp;
  std::string ExcludeLabelRegExp;
  std::string IncludeRegExp;
  std::string ExcludeRegExp;
  std::string ExcludeFixtureRegExp;
  std::string ExcludeFixtureSetupRegExp;
  std::string ExcludeFixtureCleanupRegExp;
  cmsys::RegularExpression IncludeLabelRegularExpression;
  cmsys::RegularExpression ExcludeLabelRegularExpression;
  cmsys::RegularExpression IncludeTestsRegularExpression;
  cmsys::RegularExpression ExcludeTestsRegularExpression;

  void GenerateRegressionImages(cmXMLWriter& xml, const std::string& dart);
  cmsys::RegularExpression DartStuff1;
  void CheckLabelFilter(cmCTestTestProperties& it);
  void CheckLabelFilterExclude(cmCTestTestProperties& it);
  void CheckLabelFilterInclude(cmCTestTestProperties& it);

  std::string TestsToRunString;
  bool UseUnion;
  ListOfTests TestList;
  size_t TotalNumberOfTests;
  cmsys::RegularExpression DartStuff;

  std::ostream* LogFile;

  bool RerunFailed;
};

#endif
