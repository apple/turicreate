/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestMultiProcessHandler_h
#define cmCTestMultiProcessHandler_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestTestHandler.h"
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <vector>

class cmCTest;
class cmCTestRunTest;

/** \class cmCTestMultiProcessHandler
 * \brief run parallel ctest
 *
 * cmCTestMultiProcessHandler
 */
class cmCTestMultiProcessHandler
{
  friend class TestComparator;

public:
  struct TestSet : public std::set<int>
  {
  };
  struct TestMap : public std::map<int, TestSet>
  {
  };
  struct TestList : public std::vector<int>
  {
  };
  struct PropertiesMap
    : public std::map<int, cmCTestTestHandler::cmCTestTestProperties*>
  {
  };

  cmCTestMultiProcessHandler();
  virtual ~cmCTestMultiProcessHandler();
  // Set the tests
  void SetTests(TestMap& tests, PropertiesMap& properties);
  // Set the max number of tests that can be run at the same time.
  void SetParallelLevel(size_t);
  void SetTestLoad(unsigned long load);
  virtual void RunTests();
  void PrintTestList();
  void PrintLabels();

  void SetPassFailVectors(std::vector<std::string>* passed,
                          std::vector<std::string>* failed)
  {
    this->Passed = passed;
    this->Failed = failed;
  }
  void SetTestResults(std::vector<cmCTestTestHandler::cmCTestTestResult>* r)
  {
    this->TestResults = r;
  }

  void SetCTest(cmCTest* ctest) { this->CTest = ctest; }

  void SetTestHandler(cmCTestTestHandler* handler)
  {
    this->TestHandler = handler;
  }

  cmCTestTestHandler* GetTestHandler() { return this->TestHandler; }

  void SetQuiet(bool b) { this->Quiet = b; }
protected:
  // Start the next test or tests as many as are allowed by
  // ParallelLevel
  void StartNextTests();
  void StartTestProcess(int test);
  bool StartTest(int test);
  // Mark the checkpoint for the given test
  void WriteCheckpoint(int index);

  void UpdateCostData();
  void ReadCostData();
  // Return index of a test based on its name
  int SearchByName(std::string const& name);

  void CreateTestCostList();

  void GetAllTestDependencies(int test, TestList& dependencies);
  void CreateSerialTestCostList();

  void CreateParallelTestCostList();

  // Removes the checkpoint file
  void MarkFinished();
  void EraseTest(int index);
  // Return true if there are still tests running
  // check all running processes for output and exit case
  bool CheckOutput();
  void RemoveTest(int index);
  // Check if we need to resume an interrupted test set
  void CheckResume();
  // Check if there are any circular dependencies
  bool CheckCycles();
  int FindMaxIndex();
  inline size_t GetProcessorsUsed(int index);
  std::string GetName(int index);

  void LockResources(int index);
  void UnlockResources(int index);
  // map from test number to set of depend tests
  TestMap Tests;
  TestList SortedTests;
  // Total number of tests we'll be running
  size_t Total;
  // Number of tests that are complete
  size_t Completed;
  size_t RunningCount;
  bool StopTimePassed;
  // list of test properties (indices concurrent to the test map)
  PropertiesMap Properties;
  std::map<int, bool> TestRunningMap;
  std::map<int, bool> TestFinishMap;
  std::map<int, std::string> TestOutput;
  std::vector<std::string>* Passed;
  std::vector<std::string>* Failed;
  std::vector<std::string> LastTestsFailed;
  std::set<std::string> LockedResources;
  std::vector<cmCTestTestHandler::cmCTestTestResult>* TestResults;
  size_t ParallelLevel; // max number of process that can be run at once
  unsigned long TestLoad;
  std::set<cmCTestRunTest*> RunningTests; // current running tests
  cmCTestTestHandler* TestHandler;
  cmCTest* CTest;
  bool HasCycles;
  bool Quiet;
  bool SerialTestRunning;
};

#endif
