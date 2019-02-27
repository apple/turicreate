/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestMultiProcessHandler.h"

#include "cmAffinity.h"
#include "cmCTest.h"
#include "cmCTestRunTest.h"
#include "cmCTestTestHandler.h"
#include "cmSystemTools.h"
#include "cmWorkingDirectory.h"

#include "cm_uv.h"

#include "cmUVSignalHackRAII.h" // IWYU pragma: keep

#include "cmsys/FStream.hxx"
#include "cmsys/SystemInformation.hxx"

#include <algorithm>
#include <chrono>
#include <cstring>
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
  this->FakeLoadForTesting = 0;
  this->Completed = 0;
  this->RunningCount = 0;
  this->ProcessorsAvailable = cmAffinity::GetProcessorsAvailable();
  this->HaveAffinity = this->ProcessorsAvailable.size();
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
  for (auto const& t : this->Tests) {
    this->TestRunningMap[t.first] = false;
    this->TestFinishMap[t.first] = false;
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

  std::string fake_load_value;
  if (cmSystemTools::GetEnv("__CTEST_FAKE_LOAD_AVERAGE_FOR_TESTING",
                            fake_load_value)) {
    if (!cmSystemTools::StringToULong(fake_load_value.c_str(),
                                      &this->FakeLoadForTesting)) {
      cmSystemTools::Error("Failed to parse fake load value: ",
                           fake_load_value.c_str());
    }
  }
}

void cmCTestMultiProcessHandler::RunTests()
{
  this->CheckResume();
  if (this->HasCycles) {
    return;
  }
#ifdef CMAKE_UV_SIGNAL_HACK
  cmUVSignalHackRAII hackRAII;
#endif
  this->TestHandler->SetMaxIndex(this->FindMaxIndex());

  uv_loop_init(&this->Loop);
  this->StartNextTests();
  uv_run(&this->Loop, UV_RUN_DEFAULT);
  uv_loop_close(&this->Loop);

  this->MarkFinished();
  this->UpdateCostData();
}

bool cmCTestMultiProcessHandler::StartTestProcess(int test)
{
  if (this->HaveAffinity && this->Properties[test]->WantAffinity) {
    size_t needProcessors = this->GetProcessorsUsed(test);
    if (needProcessors > this->ProcessorsAvailable.size()) {
      return false;
    }
    std::vector<size_t> affinity;
    affinity.reserve(needProcessors);
    for (size_t i = 0; i < needProcessors; ++i) {
      auto p = this->ProcessorsAvailable.begin();
      affinity.push_back(*p);
      this->ProcessorsAvailable.erase(p);
    }
    this->Properties[test]->Affinity = std::move(affinity);
  }

  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "test " << test << "\n", this->Quiet);
  this->TestRunningMap[test] = true; // mark the test as running
  // now remove the test itself
  this->EraseTest(test);
  this->RunningCount += GetProcessorsUsed(test);

  cmCTestRunTest* testRun = new cmCTestRunTest(*this);
  if (this->CTest->GetRepeatUntilFail()) {
    testRun->SetRunUntilFailOn();
    testRun->SetNumberOfRuns(this->CTest->GetTestRepeat());
  }
  testRun->SetIndex(test);
  testRun->SetTestProperties(this->Properties[test]);

  // Find any failed dependencies for this test. We assume the more common
  // scenario has no failed tests, so make it the outer loop.
  for (std::string const& f : *this->Failed) {
    if (this->Properties[test]->RequireSuccessDepends.find(f) !=
        this->Properties[test]->RequireSuccessDepends.end()) {
      testRun->AddFailedDependency(f);
    }
  }

  // Always lock the resources we'll be using, even if we fail to set the
  // working directory because FinishTestProcess() will try to unlock them
  this->LockResources(test);

  cmWorkingDirectory workdir(this->Properties[test]->Directory);
  if (workdir.Failed()) {
    testRun->StartFailure("Failed to change working directory to " +
                          this->Properties[test]->Directory + " : " +
                          std::strerror(workdir.GetLastResult()));
  } else {
    if (testRun->StartTest(this->Completed, this->Total)) {
      return true;
    }
  }

  this->FinishTestProcess(testRun, false);
  return false;
}

bool cmCTestMultiProcessHandler::CheckStopTimePassed()
{
  if (!this->StopTimePassed) {
    std::chrono::system_clock::time_point stop_time =
      this->CTest->GetStopTime();
    if (stop_time != std::chrono::system_clock::time_point() &&
        stop_time <= std::chrono::system_clock::now()) {
      this->SetStopTimePassed();
    }
  }
  return this->StopTimePassed;
}

void cmCTestMultiProcessHandler::SetStopTimePassed()
{
  if (!this->StopTimePassed) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "The stop time has been passed. "
               "Stopping all tests."
                 << std::endl);
    this->StopTimePassed = true;
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
  for (std::string const& i : this->Properties[index]->LockedResources) {
    this->LockedResources.erase(i);
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
  // Cap tests that want affinity to the maximum affinity available.
  if (this->HaveAffinity && processors > this->HaveAffinity &&
      this->Properties[test]->WantAffinity) {
    processors = this->HaveAffinity;
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
  for (std::string const& i : this->Properties[test]->LockedResources) {
    if (this->LockedResources.find(i) != this->LockedResources.end()) {
      return false;
    }
  }

  // if there are no depends left then run this test
  if (this->Tests[test].empty()) {
    return this->StartTestProcess(test);
  }
  // This test was not able to start because it is waiting
  // on depends to run
  return false;
}

void cmCTestMultiProcessHandler::StartNextTests()
{
  if (this->TestLoadRetryTimer.get() != nullptr) {
    // This timer may be waiting to call StartNextTests again.
    // Since we have been called it is no longer needed.
    uv_timer_stop(this->TestLoadRetryTimer);
  }

  if (this->Tests.empty()) {
    this->TestLoadRetryTimer.reset();
    return;
  }

  if (this->CheckStopTimePassed()) {
    return;
  }

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
  size_t minProcessorsRequired = this->ParallelLevel;
  std::string testWithMinProcessors;

  cmsys::SystemInformation info;

  unsigned long systemLoad = 0;
  size_t spareLoad = 0;
  if (this->TestLoad > 0) {
    // Activate possible wait.
    allTestsFailedTestLoadCheck = true;

    // Check for a fake load average value used in testing.
    if (this->FakeLoadForTesting > 0) {
      systemLoad = this->FakeLoadForTesting;
      // Drop the fake load for the next iteration to a value low enough
      // that the next iteration will start tests.
      this->FakeLoadForTesting = 1;
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
  for (auto const& test : copy) {
    // Take a nap if we're currently performing a RUN_SERIAL test.
    if (this->SerialTestRunning) {
      break;
    }
    // We can only start a RUN_SERIAL test if no other tests are also running.
    if (this->Properties[test]->RunSerial && this->RunningCount > 0) {
      continue;
    }

    size_t processors = GetProcessorsUsed(test);
    bool testLoadOk = true;
    if (this->TestLoad > 0) {
      if (processors <= spareLoad) {
        cmCTestLog(this->CTest, DEBUG,
                   "OK to run " << GetName(test) << ", it requires "
                                << processors << " procs & system load is: "
                                << systemLoad << std::endl);
        allTestsFailedTestLoadCheck = false;
      } else {
        testLoadOk = false;
      }
    }

    if (processors <= minProcessorsRequired) {
      minProcessorsRequired = processors;
      testWithMinProcessors = GetName(test);
    }

    if (testLoadOk && processors <= numToStart && this->StartTest(test)) {
      numToStart -= processors;
    } else if (numToStart == 0) {
      break;
    }
  }

  if (allTestsFailedTestLoadCheck) {
    // Find out whether there are any non RUN_SERIAL tests left, so that the
    // correct warning may be displayed.
    bool onlyRunSerialTestsLeft = true;
    for (auto const& test : copy) {
      if (!this->Properties[test]->RunSerial) {
        onlyRunSerialTestsLeft = false;
      }
    }
    cmCTestLog(this->CTest, HANDLER_OUTPUT, "***** WAITING, ");

    if (this->SerialTestRunning) {
      cmCTestLog(this->CTest, HANDLER_OUTPUT,
                 "Waiting for RUN_SERIAL test to finish.");
    } else if (onlyRunSerialTestsLeft) {
      cmCTestLog(this->CTest, HANDLER_OUTPUT,
                 "Only RUN_SERIAL tests remain, awaiting available slot.");
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

    // Wait between 1 and 5 seconds before trying again.
    unsigned int milliseconds = (cmSystemTools::RandomSeed() % 5 + 1) * 1000;
    if (this->FakeLoadForTesting) {
      milliseconds = 10;
    }
    if (this->TestLoadRetryTimer.get() == nullptr) {
      this->TestLoadRetryTimer.init(this->Loop, this);
    }
    this->TestLoadRetryTimer.start(
      &cmCTestMultiProcessHandler::OnTestLoadRetryCB, milliseconds, 0);
  }
}

void cmCTestMultiProcessHandler::OnTestLoadRetryCB(uv_timer_t* timer)
{
  auto self = static_cast<cmCTestMultiProcessHandler*>(timer->data);
  self->StartNextTests();
}

void cmCTestMultiProcessHandler::FinishTestProcess(cmCTestRunTest* runner,
                                                   bool started)
{
  this->Completed++;

  int test = runner->GetIndex();
  auto properties = runner->GetTestProperties();

  bool testResult = runner->EndTest(this->Completed, this->Total, started);
  if (runner->TimedOutForStopTime()) {
    this->SetStopTimePassed();
  }
  if (started) {
    if (!this->StopTimePassed && runner->StartAgain(this->Completed)) {
      this->Completed--; // remove the completed test because run again
      return;
    }
  }

  if (testResult) {
    this->Passed->push_back(properties->Name);
  } else if (!properties->Disabled) {
    this->Failed->push_back(properties->Name);
  }

  for (auto& t : this->Tests) {
    t.second.erase(test);
  }

  this->TestFinishMap[test] = true;
  this->TestRunningMap[test] = false;
  this->WriteCheckpoint(test);
  this->UnlockResources(test);
  this->RunningCount -= GetProcessorsUsed(test);

  for (auto p : properties->Affinity) {
    this->ProcessorsAvailable.insert(p);
  }
  properties->Affinity.clear();

  delete runner;
  if (started) {
    this->StartNextTests();
  }
}

void cmCTestMultiProcessHandler::UpdateCostData()
{
  std::string fname = this->CTest->GetCostDataFile();
  std::string tmpout = fname + ".tmp";
  cmsys::ofstream fout;
  fout.open(tmpout.c_str());

  PropertiesMap temp = this->Properties;

  if (cmSystemTools::FileExists(fname)) {
    cmsys::ifstream fin;
    fin.open(fname.c_str());

    std::string line;
    while (std::getline(fin, line)) {
      if (line == "---") {
        break;
      }
      std::vector<std::string> parts = cmSystemTools::SplitString(line, ' ');
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
  for (auto const& i : temp) {
    fout << i.second->Name << " " << i.second->PreviousRuns << " "
         << i.second->Cost << "\n";
  }

  // Write list of failed tests
  fout << "---\n";
  for (std::string const& f : *this->Failed) {
    fout << f << "\n";
  }
  fout.close();
  cmSystemTools::RenameFile(tmpout.c_str(), fname.c_str());
}

void cmCTestMultiProcessHandler::ReadCostData()
{
  std::string fname = this->CTest->GetCostDataFile();

  if (cmSystemTools::FileExists(fname, true)) {
    cmsys::ifstream fin;
    fin.open(fname.c_str());
    std::string line;
    while (std::getline(fin, line)) {
      if (line == "---") {
        break;
      }

      std::vector<std::string> parts = cmSystemTools::SplitString(line, ' ');

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
      if (!line.empty()) {
        this->LastTestsFailed.push_back(line);
      }
    }
    fin.close();
  }
}

int cmCTestMultiProcessHandler::SearchByName(std::string const& name)
{
  int index = -1;

  for (auto const& p : this->Properties) {
    if (p.second->Name == name) {
      index = p.first;
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
  priorityStack.emplace_back();
  TestSet& topLevel = priorityStack.back();

  // In parallel test runs add previously failed tests to the front
  // of the cost list and queue other tests for further sorting
  for (auto const& t : this->Tests) {
    if (std::find(this->LastTestsFailed.begin(), this->LastTestsFailed.end(),
                  this->Properties[t.first]->Name) !=
        this->LastTestsFailed.end()) {
      // If the test failed last time, it should be run first.
      this->SortedTests.push_back(t.first);
      alreadySortedTests.insert(t.first);
    } else {
      topLevel.insert(t.first);
    }
  }

  // In parallel test runs repeatedly move dependencies of the tests on
  // the current dependency level to the next level until no
  // further dependencies exist.
  while (!priorityStack.back().empty()) {
    TestSet& previousSet = priorityStack.back();
    priorityStack.emplace_back();
    TestSet& currentSet = priorityStack.back();

    for (auto const& i : previousSet) {
      TestSet const& dependencies = this->Tests[i];
      currentSet.insert(dependencies.begin(), dependencies.end());
    }

    for (auto const& i : currentSet) {
      previousSet.erase(i);
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

    for (auto const& j : sortedCopy) {
      if (alreadySortedTests.find(j) == alreadySortedTests.end()) {
        this->SortedTests.push_back(j);
        alreadySortedTests.insert(j);
      }
    }
  }
}

void cmCTestMultiProcessHandler::GetAllTestDependencies(int test,
                                                        TestList& dependencies)
{
  TestSet const& dependencySet = this->Tests[test];
  for (int i : dependencySet) {
    GetAllTestDependencies(i, dependencies);
    dependencies.push_back(i);
  }
}

void cmCTestMultiProcessHandler::CreateSerialTestCostList()
{
  TestList presortedList;

  for (auto const& i : this->Tests) {
    presortedList.push_back(i.first);
  }

  TestComparator comp(this);
  std::stable_sort(presortedList.begin(), presortedList.end(), comp);

  TestSet alreadySortedTests;

  for (int test : presortedList) {
    if (alreadySortedTests.find(test) != alreadySortedTests.end()) {
      continue;
    }

    TestList dependencies;
    GetAllTestDependencies(test, dependencies);

    for (int testDependency : dependencies) {
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

  for (auto& it : this->Properties) {
    count++;
    cmCTestTestHandler::cmCTestTestProperties& p = *it.second;

    // Don't worry if this fails, we are only showing the test list, not
    // running the tests
    cmWorkingDirectory workdir(p.Directory);

    cmCTestRunTest testRun(*this);
    testRun.SetIndex(p.Index);
    testRun.SetTestProperties(&p);
    testRun.ComputeArguments(); // logs the command in verbose mode

    if (!p.Labels.empty()) // print the labels
    {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                         "Labels:", this->Quiet);
    }
    for (std::string const& label : p.Labels) {
      cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT, " " << label,
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

  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                     std::endl
                       << "Total Tests: " << this->Total << std::endl,
                     this->Quiet);
}

void cmCTestMultiProcessHandler::PrintLabels()
{
  std::set<std::string> allLabels;
  for (auto& it : this->Properties) {
    cmCTestTestHandler::cmCTestTestProperties& p = *it.second;
    allLabels.insert(p.Labels.begin(), p.Labels.end());
  }

  if (!allLabels.empty()) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "All Labels:" << std::endl,
                       this->Quiet);
  } else {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "No Labels Exist" << std::endl, this->Quiet);
  }
  for (std::string const& label : allLabels) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT, "  " << label << std::endl,
                       this->Quiet);
  }
}

void cmCTestMultiProcessHandler::CheckResume()
{
  std::string fname =
    this->CTest->GetBinaryDir() + "/Testing/Temporary/CTestCheckpoint.txt";
  if (this->CTest->GetFailover()) {
    if (cmSystemTools::FileExists(fname, true)) {
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
  } else if (cmSystemTools::FileExists(fname, true)) {
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
  for (auto const& i : this->Tests) {
    if (i.first > max) {
      max = i.first;
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
  for (auto const& it : this->Tests) {
    // DFS from each element to itself
    int root = it.first;
    std::set<int> visited;
    std::stack<int> s;
    s.push(root);
    while (!s.empty()) {
      int test = s.top();
      s.pop();
      if (visited.insert(test).second) {
        for (auto const& d : this->Tests[test]) {
          if (d == root) {
            // cycle exists
            cmCTestLog(
              this->CTest, ERROR_MESSAGE,
              "Error: a cycle exists in the test dependency graph "
              "for the test \""
                << this->Properties[root]->Name
                << "\".\nPlease fix the cycle and run ctest again.\n");
            return false;
          }
          s.push(d);
        }
      }
    }
  }
  cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                     "Checking test dependency graph end" << std::endl,
                     this->Quiet);
  return true;
}
