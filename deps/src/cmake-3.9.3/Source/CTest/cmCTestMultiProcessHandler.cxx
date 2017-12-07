/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestMultiProcessHandler.h"

#include "cmCTest.h"
#include "cmCTestRunTest.h"
#include "cmCTestScriptHandler.h"
#include "cmCTestTestHandler.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"

#include "cmsys/FStream.hxx"
#include "cmsys/String.hxx"
#include "cmsys/SystemInformation.hxx"
#include <algorithm>
#include <iomanip>
#include <list>
#include <math.h>
#include <sstream>
#include <stack>
#include <stdlib.h>
#include <utility>

class TestComparator
{
public:
  TestComparator(cmCTestMultiProcessHandler* handler)
    : Handler(handler)
  {
  }
  ~TestComparator() {}

  // Sorts tests in descending order of cost
  bool operator()(int index1, int index2) const
  {
    return Handler->Properties[index1]->Cost >
      Handler->Properties[index2]->Cost;
  }

private:
  cmCTestMultiProcessHandler* Handler;
};

cmCTestMultiProcessHandler::cmCTestMultiProcessHandler()
{
  this->ParallelLevel = 1;
  this->TestLoad = 0;
  this->Completed = 0;
  this->RunningCount = 0;
  this->StopTimePassed = false;
  this->HasCycles = false;
  this->SerialTestRunning = false;
}

cmCTestMultiProcessHandler::~cmCTestMultiProcessHandler()
{
}

// Set the tests
void cmCTestMultiProcessHandler::SetTests(TestMap& tests,
                                          PropertiesMap& properties)
{
  this->Tests = tests;
  this->Properties = properties;
  this->Total = this->Tests.size();
  // set test run map to false for all
  for (TestMap::iterator i = this->Tests.begin(); i != this->Tests.end();
       ++i) {
    this->TestRunningMap[i->first] = false;
    this->TestFinishMap[i->first] = false;
  }
  if (!this->CTest->GetShowOnly()) {
    this->ReadCostData();
    this->HasCycles = !this->CheckCycles();
    if (this->HasCycles) {
      return;
    }
    this->CreateTestCostList();
  }
}

// Set the max number of tests that can be run at the same time.
void cmCTestMultiProcessHandler::SetParallelLevel(size_t level)
{
  this->ParallelLevel = level < 1 ? 1 : level;
}

void cmCTestMultiProcessHandler::SetTestLoad(unsigned long load)
{
  this->TestLoad = load;
}

void cmCTestMultiProcessHandler::RunTests()
{
  this->CheckResume();
  if (this->HasCycles) {
    return;
  }
  this->TestHandler->SetMaxIndex(this->FindMaxIndex());
  this->StartNextTests();
  while (!this->Tests.empty()) {
    if (this->StopTimePassed) {
      return;
    }
    this->CheckOutput();
    this->StartNextTests();
  }
  // let all running tests finish
  while (this->CheckOutput()) {
  }
  this->MarkFinished();
  this->UpdateCostData();
}

void cmCTestMultiProcessHandler::StartTestProcess(int test)
{
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "test " << test << "\n", this->Quiet);
  this->TestRunningMap[test] = true; // mark the test as running
  // now remove the test itself
  this->EraseTest(test);
  this->RunningCount += GetProcessorsUsed(test);

  cmCTestRunTest* testRun = new cmCTestRunTest(this->TestHandler);
  if (this->CTest->GetRepeatUntilFail()) {
    testRun->SetRunUntilFailOn();
    testRun->SetNumberOfRuns(this->CTest->GetTestRepeat());
  }
  testRun->SetIndex(test);
  testRun->SetTestProperties(this->Properties[test]);

  // Find any failed dependencies for this test. We assume the more common
  // scenario has no failed tests, so make it the outer loop.
  for (std::vector<std::string>::const_iterator it = this->Failed->begin();
       it != this->Failed->end(); ++it) {
    if (this->Properties[test]->RequireSuccessDepends.find(*it) !=
        this->Properties[test]->RequireSuccessDepends.end()) {
      testRun->AddFailedDependency(*it);
    }
  }

  cmWorkingDirectory workdir(this->Properties[test]->Directory);

  // Lock the resources we'll be using
  this->LockResources(test);

  if (testRun->StartTest(this->Total)) {
    this->RunningTests.insert(testRun);
  } else if (testRun->IsStopTimePassed()) {
    this->StopTimePassed = true;
    delete testRun;
    return;
  } else {

    for (TestMap::iterator j = this->Tests.begin(); j != this->Tests.end();
         ++j) {
      j->second.erase(test);
    }

    this->UnlockResources(test);
    this->Completed++;
    this->TestFinishMap[test] = true;
    this->TestRunningMap[test] = false;
    this->RunningCount -= GetProcessorsUsed(test);
    testRun->EndTest(this->Completed, this->Total, false);
    if (!this->Properties[test]->Disabled) {
      this->Failed->push_back(this->Properties[test]->Name);
    }
    delete testRun;
  }
}

void cmCTestMultiProcessHandler::LockResources(int index)
{
  this->LockedResources.insert(
    this->Properties[index]->LockedResources.begin(),
    this->Properties[index]->LockedResources.end());

  if (this->Properties[index]->RunSerial) {
    this->SerialTestRunning = true;
  }
}

void cmCTestMultiProcessHandler::UnlockResources(int index)
{
  for (std::set<std::string>::iterator i =
         this->Properties[index]->LockedResources.begin();
       i != this->Properties[index]->LockedResources.end(); ++i) {
    this->LockedResources.erase(*i);
  }
  if (this->Properties[index]->RunSerial) {
    this->SerialTestRunning = false;
  }
}

void cmCTestMultiProcessHandler::EraseTest(int test)
{
  this->Tests.erase(test);
  this->SortedTests.erase(
    std::find(this->SortedTests.begin(), this->SortedTests.end(), test));
}

inline size_t cmCTestMultiProcessHandler::GetProcessorsUsed(int test)
{
  size_t processors = static_cast<int>(this->Properties[test]->Processors);
  // If processors setting is set higher than the -j
  // setting, we default to using all of the process slots.
  if (processors > this->ParallelLevel) {
    processors = this->ParallelLevel;
  }
  return processors;
}

std::string cmCTestMultiProcessHandler::GetName(int test)
{
  return this->Properties[test]->Name;
}

bool cmCTestMultiProcessHandler::StartTest(int test)
{
  // Check for locked resources
  for (std::set<std::string>::iterator i =
         this->Properties[test]->LockedResources.begin();
       i != this->Properties[test]->LockedResources.end(); ++i) {
    if (this->LockedResources.find(*i) != this->LockedResources.end()) {
      return false;
    }
  }

  // if there are no depends left then run this test
  if (this->Tests[test].empty()) {
    this->StartTestProcess(test);
    return true;
  }
  // This test was not able to start because it is waiting
  // on depends to run
  return false;
}

void cmCTestMultiProcessHandler::StartNextTests()
{
  size_t numToStart = 0;
  if (this->RunningCount < this->ParallelLevel) {
    numToStart = this->ParallelLevel - this->RunningCount;
  }

  if (numToStart == 0) {
    return;
  }

  // Don't start any new tests if one with the RUN_SERIAL property
  // is already running.
  if (this->SerialTestRunning) {
    return;
  }

  bool allTestsFailedTestLoadCheck = false;
  bool usedFakeLoadForTesting = false;
  size_t minProcessorsRequired = this->ParallelLevel;
  std::string testWithMinProcessors;

  cmsys::SystemInformation info;

  unsigned long systemLoad = 0;
  size_t spareLoad = 0;
  if (this->TestLoad > 0) {
    // Activate possible wait.
    allTestsFailedTestLoadCheck = true;

    // Check for a fake load average value used in testing.
    std::string fake_load_value;
    if (cmSystemTools::GetEnv("__CTEST_FAKE_LOAD_AVERAGE_FOR_TESTING",
                              fake_load_value)) {
      usedFakeLoadForTesting = true;
      if (!cmSystemTools::StringToULong(fake_load_value.c_str(),
                                        &systemLoad)) {
        cmSystemTools::Error("Failed to parse fake load value: ",
                             fake_load_value.c_str());
      }
    }
    // If it's not set, look up the true load average.
    else {
      systemLoad = static_cast<unsigned long>(ceil(info.GetLoadAverage()));
    }
    spareLoad =
      (this->TestLoad > systemLoad ? this->TestLoad - systemLoad : 0);

    // Don't start more tests than the spare load can support.
    if (numToStart > spareLoad) {
      numToStart = spareLoad;
    }
  }

  TestList copy = this->SortedTests;
  for (TestList::iterator test = copy.begin(); test != copy.end(); ++test) {
    // Take a nap if we're currently performing a RUN_SERIAL test.
    if (this->SerialTestRunning) {
      break;
    }
    // We can only start a RUN_SERIAL test if no other tests are also running.
    if (this->Properties[*test]->RunSerial && this->RunningCount > 0) {
      continue;
    }

    size_t processors = GetProcessorsUsed(*test);
    bool testLoadOk = true;
    if (this->TestLoad > 0) {
      if (processors <= spareLoad) {
        cmCTestLog(this->CTest, DEBUG, "OK to run "
                     << GetName(*test) << ", it requires " << processors
                     << " procs & system load is: " << systemLoad
                     << std::endl);
        allTestsFailedTestLoadCheck = false;
      } else {
        testLoadOk = false;
      }
    }

    if (processors <= minProcessorsRequired) {
      minProcessorsRequired = processors;
      testWithMinProcessors = GetName(*test);
    }

    if (testLoadOk && processors <= numToStart && this->StartTest(*test)) {
      if (this->StopTimePassed) {
        return;
      }

      numToStart -= processors;
    } else if (numToStart == 0) {
      break;
    }
  }

  if (allTestsFailedTestLoadCheck) {
    cmCTestLog(this->CTest, HANDLER_OUTPUT, "***** WAITING, ");
    if (this->SerialTestRunning) {
      cmCTestLog(this->CTest, HANDLER_OUTPUT,
                 "Waiting for RUN_SERIAL test to finish.");
    } else {
      /* clang-format off */
      cmCTestLog(this->CTest, HANDLER_OUTPUT,
                 "System Load: " << systemLoad << ", "
                 "Max Allowed Load: " << this->TestLoad << ", "
                 "Smallest test " << testWithMinProcessors <<
                 " requires " << minProcessorsRequired);
      /* clang-format on */
    }
    cmCTestLog(this->CTest, HANDLER_OUTPUT, "*****" << std::endl);

    if (usedFakeLoadForTesting) {
      // Break out of the infinite loop of waiting for our fake load
      // to come down.
      this->StopTimePassed = true;
    } else {
      // Wait between 1 and 5 seconds before trying again.
      cmCTestScriptHandler::SleepInSeconds(cmSystemTools::RandomSeed() % 5 +
                                           1);
    }
  }
}

bool cmCTestMultiProcessHandler::CheckOutput()
{
  // no more output we are done
  if (this->RunningTests.empty()) {
    return false;
  }
  std::vector<cmCTestRunTest*> finished;
  std::string out, err;
  for (std::set<cmCTestRunTest*>::const_iterator i =
         this->RunningTests.begin();
       i != this->RunningTests.end(); ++i) {
    cmCTestRunTest* p = *i;
    if (!p->CheckOutput()) {
      finished.push_back(p);
    }
  }
  for (std::vector<cmCTestRunTest*>::iterator i = finished.begin();
       i != finished.end(); ++i) {
    this->Completed++;
    cmCTestRunTest* p = *i;
    int test = p->GetIndex();

    bool testResult = p->EndTest(this->Completed, this->Total, true);
    if (p->StartAgain()) {
      this->Completed--; // remove the completed test because run again
      continue;
    }
    if (testResult) {
      this->Passed->push_back(p->GetTestProperties()->Name);
    } else {
      this->Failed->push_back(p->GetTestProperties()->Name);
    }
    for (TestMap::iterator j = this->Tests.begin(); j != this->Tests.end();
         ++j) {
      j->second.erase(test);
    }
    this->TestFinishMap[test] = true;
    this->TestRunningMap[test] = false;
    this->RunningTests.erase(p);
    this->WriteCheckpoint(test);
    this->UnlockResources(test);
    this->RunningCount -= GetProcessorsUsed(test);
    delete p;
  }
  return true;
}

void cmCTestMultiProcessHandler::UpdateCostData()
{
  std::string fname = this->CTest->GetCostDataFile();
  std::string tmpout = fname + ".tmp";
  cmsys::ofstream fout;
  fout.open(tmpout.c_str());

  PropertiesMap temp = this->Properties;

  if (cmSystemTools::FileExists(fname.c_str())) {
    cmsys::ifstream fin;
    fin.open(fname.c_str());

    std::string line;
    while (std::getline(fin, line)) {
      if (line == "---") {
        break;
      }
      std::vector<cmsys::String> parts = cmSystemTools::SplitString(line, ' ');
      // Format: <name> <previous_runs> <avg_cost>
      if (parts.size() < 3) {
        break;
      }

      std::string name = parts[0];
      int prev = atoi(parts[1].c_str());
      float cost = static_cast<float>(atof(parts[2].c_str()));

      int index = this->SearchByName(name);
      if (index == -1) {
        // This test is not in memory. We just rewrite the entry
        fout << name << " " << prev << " " << cost << "\n";
      } else {
        // Update with our new average cost
        fout << name << " " << this->Properties[index]->PreviousRuns << " "
             << this->Properties[index]->Cost << "\n";
        temp.erase(index);
      }
    }
    fin.close();
    cmSystemTools::RemoveFile(fname);
  }

  // Add all tests not previously listed in the file
  for (PropertiesMap::iterator i = temp.begin(); i != temp.end(); ++i) {
    fout << i->second->Name << " " << i->second->PreviousRuns << " "
         << i->second->Cost << "\n";
  }

  // Write list of failed tests
  fout << "---\n";
  for (std::vector<std::string>::iterator i = this->Failed->begin();
       i != this->Failed->end(); ++i) {
    fout << *i << "\n";
  }
  fout.close();
  cmSystemTools::RenameFile(tmpout.c_str(), fname.c_str());
}

void cmCTestMultiProcessHandler::ReadCostData()
{
  std::string fname = this->CTest->GetCostDataFile();

  if (cmSystemTools::FileExists(fname.c_str(), true)) {
    cmsys::ifstream fin;
    fin.open(fname.c_str());
    std::string line;
    while (std::getline(fin, line)) {
      if (line == "---") {
        break;
      }

      std::vector<cmsys::String> parts = cmSystemTools::SplitString(line, ' ');

      // Probably an older version of the file, will be fixed next run
      if (parts.size() < 3) {
        fin.close();
        return;
      }

      std::string name = parts[0];
      int prev = atoi(parts[1].c_str());
      float cost = static_cast<float>(atof(parts[2].c_str()));

      int index = this->SearchByName(name);
      if (index == -1) {
        continue;
      }

      this->Properties[index]->PreviousRuns = prev;
      // When not running in parallel mode, don't use cost data
      if (this->ParallelLevel > 1 && this->Properties[index] &&
          this->Properties[index]->Cost == 0) {
        this->Properties[index]->Cost = cost;
      }
    }
    // Next part of the file is the failed tests
    while (std::getline(fin, line)) {
      if (line != "") {
        this->LastTestsFailed.push_back(line);
      }
    }
    fin.close();
  }
}

int cmCTestMultiProcessHandler::SearchByName(std::string const& name)
{
  int index = -1;

  for (PropertiesMap::iterator i = this->Properties.begin();
       i != this->Properties.end(); ++i) {
    if (i->second->Name == name) {
      index = i->first;
    }
  }
  return index;
}

void cmCTestMultiProcessHandler::CreateTestCostList()
{
  if (this->ParallelLevel > 1) {
    CreateParallelTestCostList();
  } else {
    CreateSerialTestCostList();
  }
}

void cmCTestMultiProcessHandler::CreateParallelTestCostList()
{
  TestSet alreadySortedTests;

  std::list<TestSet> priorityStack;
  priorityStack.push_back(TestSet());
  TestSet& topLevel = priorityStack.back();

  // In parallel test runs add previously failed tests to the front
  // of the cost list and queue other tests for further sorting
  for (TestMap::const_iterator i = this->Tests.begin(); i != this->Tests.end();
       ++i) {
    if (std::find(this->LastTestsFailed.begin(), this->LastTestsFailed.end(),
                  this->Properties[i->first]->Name) !=
        this->LastTestsFailed.end()) {
      // If the test failed last time, it should be run first.
      this->SortedTests.push_back(i->first);
      alreadySortedTests.insert(i->first);
    } else {
      topLevel.insert(i->first);
    }
  }

  // In parallel test runs repeatedly move dependencies of the tests on
  // the current dependency level to the next level until no
  // further dependencies exist.
  while (!priorityStack.back().empty()) {
    TestSet& previousSet = priorityStack.back();
    priorityStack.push_back(TestSet());
    TestSet& currentSet = priorityStack.back();

    for (TestSet::const_iterator i = previousSet.begin();
         i != previousSet.end(); ++i) {
      TestSet const& dependencies = this->Tests[*i];
      currentSet.insert(dependencies.begin(), dependencies.end());
    }

    for (TestSet::const_iterator i = currentSet.begin(); i != currentSet.end();
         ++i) {
      previousSet.erase(*i);
    }
  }

  // Remove the empty dependency level
  priorityStack.pop_back();

  // Reverse iterate over the different dependency levels (deepest first).
  // Sort tests within each level by COST and append them to the cost list.
  for (std::list<TestSet>::reverse_iterator i = priorityStack.rbegin();
       i != priorityStack.rend(); ++i) {
    TestSet const& currentSet = *i;
    TestComparator comp(this);

    TestList sortedCopy;

    sortedCopy.insert(sortedCopy.end(), currentSet.begin(), currentSet.end());

    std::stable_sort(sortedCopy.begin(), sortedCopy.end(), comp);

    for (TestList::const_iterator j = sortedCopy.begin();
         j != sortedCopy.end(); ++j) {
      if (alreadySortedTests.find(*j) == alreadySortedTests.end()) {
        this->SortedTests.push_back(*j);
        alreadySortedTests.insert(*j);
      }
    }
  }
}

void cmCTestMultiProcessHandler::GetAllTestDependencies(int test,
                                                        TestList& dependencies)
{
  TestSet const& dependencySet = this->Tests[test];
  for (TestSet::const_iterator i = dependencySet.begin();
       i != dependencySet.end(); ++i) {
    GetAllTestDependencies(*i, dependencies);
    dependencies.push_back(*i);
  }
}

void cmCTestMultiProcessHandler::CreateSerialTestCostList()
{
  TestList presortedList;

  for (TestMap::iterator i = this->Tests.begin(); i != this->Tests.end();
       ++i) {
    presortedList.push_back(i->first);
  }

  TestComparator comp(this);
  std::stable_sort(presortedList.begin(), presortedList.end(), comp);

  TestSet alreadySortedTests;

  for (TestList::const_iterator i = presortedList.begin();
       i != presortedList.end(); ++i) {
    int test = *i;

    if (alreadySortedTests.find(test) != alreadySortedTests.end()) {
      continue;
    }

    TestList dependencies;
    GetAllTestDependencies(test, dependencies);

    for (TestList::const_iterator j = dependencies.begin();
         j != dependencies.end(); ++j) {
      int testDependency = *j;

      if (alreadySortedTests.find(testDependency) ==
          alreadySortedTests.end()) {
        alreadySortedTests.insert(testDependency);
        this->SortedTests.push_back(testDependency);
      }
    }

    alreadySortedTests.insert(test);
    this->SortedTests.push_back(test);
  }
}

void cmCTestMultiProcessHandler::WriteCheckpoint(int index)
{
  std::string fname =
    this->CTest->GetBinaryDir() + "/Testing/Temporary/CTestCheckpoint.txt";
  cmsys::ofstream fout;
  fout.open(fname.c_str(), std::ios::app);
  fout << index << "\n";
  fout.close();
}

void cmCTestMultiProcessHandler::MarkFinished()
{
  std::string fname =
    this->CTest->GetBinaryDir() + "/Testing/Temporary/CTestCheckpoint.txt";
  cmSystemTools::RemoveFile(fname);
}

// For ShowOnly mode
void cmCTestMultiProcessHandler::PrintTestList()
{
  this->TestHandler->SetMaxIndex(this->FindMaxIndex());
  int count = 0;

  for (PropertiesMap::iterator it = this->Properties.begin();
       it != this->Properties.end(); ++it) {
    count++;
    cmCTestTestHandler::cmCTestTestProperties& p = *it->second;

    cmWorkingDirectory workdir(p.Directory);

    cmCTestRunTest testRun(this->TestHandler);
    testRun.SetIndex(p.Index);
    testRun.SetTestProperties(&p);
    testRun.ComputeArguments(); // logs the command in verbose mode

    if (!p.Labels.empty()) // print the labels
    {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, "Labels:",
                         this->Quiet);
    }
    for (std::vector<std::string>::iterator label = p.Labels.begin();
         label != p.Labels.end(); ++label) {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, " " << *label,
                         this->Quiet);
    }
    if (!p.Labels.empty()) // print the labels
    {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, std::endl,
                         this->Quiet);
    }

    if (this->TestHandler->MemCheck) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "  Memory Check",
                         this->Quiet);
    } else {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "  Test", this->Quiet);
    }
    std::ostringstream indexStr;
    indexStr << " #" << p.Index << ":";
    cmCTestOptionalLog(
      this->CTest, HANDLER_OUTPUT,
      std::setw(3 + getNumWidth(this->TestHandler->GetMaxIndex()))
        << indexStr.str(),
      this->Quiet);
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, " " << p.Name,
                       this->Quiet);
    if (p.Disabled) {
      cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, " (Disabled)",
                         this->Quiet);
    }
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, std::endl, this->Quiet);
  }

  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, std::endl
                       << "Total Tests: " << this->Total << std::endl,
                     this->Quiet);
}

void cmCTestMultiProcessHandler::PrintLabels()
{
  std::set<std::string> allLabels;
  for (PropertiesMap::iterator it = this->Properties.begin();
       it != this->Properties.end(); ++it) {
    cmCTestTestHandler::cmCTestTestProperties& p = *it->second;
    allLabels.insert(p.Labels.begin(), p.Labels.end());
  }

  if (!allLabels.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "All Labels:" << std::endl,
                       this->Quiet);
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "No Labels Exist" << std::endl, this->Quiet);
  }
  for (std::set<std::string>::iterator label = allLabels.begin();
       label != allLabels.end(); ++label) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "  " << *label << std::endl, this->Quiet);
  }
}

void cmCTestMultiProcessHandler::CheckResume()
{
  std::string fname =
    this->CTest->GetBinaryDir() + "/Testing/Temporary/CTestCheckpoint.txt";
  if (this->CTest->GetFailover()) {
    if (cmSystemTools::FileExists(fname.c_str(), true)) {
      *this->TestHandler->LogFile
        << "Resuming previously interrupted test set" << std::endl
        << "----------------------------------------------------------"
        << std::endl;

      cmsys::ifstream fin;
      fin.open(fname.c_str());
      std::string line;
      while (std::getline(fin, line)) {
        int index = atoi(line.c_str());
        this->RemoveTest(index);
      }
      fin.close();
    }
  } else if (cmSystemTools::FileExists(fname.c_str(), true)) {
    cmSystemTools::RemoveFile(fname);
  }
}

void cmCTestMultiProcessHandler::RemoveTest(int index)
{
  this->EraseTest(index);
  this->Properties.erase(index);
  this->TestRunningMap[index] = false;
  this->TestFinishMap[index] = true;
  this->Completed++;
}

int cmCTestMultiProcessHandler::FindMaxIndex()
{
  int max = 0;
  cmCTestMultiProcessHandler::TestMap::iterator i = this->Tests.begin();
  for (; i != this->Tests.end(); ++i) {
    if (i->first > max) {
      max = i->first;
    }
  }
  return max;
}

// Returns true if no cycles exist in the dependency graph
bool cmCTestMultiProcessHandler::CheckCycles()
{
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Checking test dependency graph..." << std::endl,
                     this->Quiet);
  for (TestMap::iterator it = this->Tests.begin(); it != this->Tests.end();
       ++it) {
    // DFS from each element to itself
    int root = it->first;
    std::set<int> visited;
    std::stack<int> s;
    s.push(root);
    while (!s.empty()) {
      int test = s.top();
      s.pop();
      if (visited.insert(test).second) {
        for (TestSet::iterator d = this->Tests[test].begin();
             d != this->Tests[test].end(); ++d) {
          if (*d == root) {
            // cycle exists
            cmCTestLog(
              this->CTest, ERROR_MESSAGE,
              "Error: a cycle exists in the test dependency graph "
              "for the test \""
                << this->Properties[root]->Name
                << "\".\nPlease fix the cycle and run ctest again.\n");
            return false;
          }
          s.push(*d);
        }
      }
    }
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Checking test dependency graph end" << std::endl,
                     this->Quiet);
  return true;
}
