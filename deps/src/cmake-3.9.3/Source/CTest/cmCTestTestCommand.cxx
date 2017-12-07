/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestTestCommand.h"

#include "cmCTest.h"
#include "cmCTestGenericHandler.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

#include <sstream>
#include <stdlib.h>
#include <vector>

cmCTestTestCommand::cmCTestTestCommand()
{
  this->Arguments[ctt_START] = "START";
  this->Arguments[ctt_END] = "END";
  this->Arguments[ctt_STRIDE] = "STRIDE";
  this->Arguments[ctt_EXCLUDE] = "EXCLUDE";
  this->Arguments[ctt_INCLUDE] = "INCLUDE";
  this->Arguments[ctt_EXCLUDE_LABEL] = "EXCLUDE_LABEL";
  this->Arguments[ctt_INCLUDE_LABEL] = "INCLUDE_LABEL";
  this->Arguments[ctt_EXCLUDE_FIXTURE] = "EXCLUDE_FIXTURE";
  this->Arguments[ctt_EXCLUDE_FIXTURE_SETUP] = "EXCLUDE_FIXTURE_SETUP";
  this->Arguments[ctt_EXCLUDE_FIXTURE_CLEANUP] = "EXCLUDE_FIXTURE_CLEANUP";
  this->Arguments[ctt_PARALLEL_LEVEL] = "PARALLEL_LEVEL";
  this->Arguments[ctt_SCHEDULE_RANDOM] = "SCHEDULE_RANDOM";
  this->Arguments[ctt_STOP_TIME] = "STOP_TIME";
  this->Arguments[ctt_TEST_LOAD] = "TEST_LOAD";
  this->Arguments[ctt_LAST] = CM_NULLPTR;
  this->Last = ctt_LAST;
}

cmCTestGenericHandler* cmCTestTestCommand::InitializeHandler()
{
  const char* ctestTimeout =
    this->Makefile->GetDefinition("CTEST_TEST_TIMEOUT");

  double timeout;
  if (ctestTimeout) {
    timeout = atof(ctestTimeout);
  } else {
    timeout = this->CTest->GetTimeOut();
    if (timeout <= 0) {
      // By default use timeout of 10 minutes
      timeout = 600;
    }
  }
  this->CTest->SetTimeOut(timeout);
  cmCTestGenericHandler* handler = this->InitializeActualHandler();
  if (this->Values[ctt_START] || this->Values[ctt_END] ||
      this->Values[ctt_STRIDE]) {
    std::ostringstream testsToRunString;
    if (this->Values[ctt_START]) {
      testsToRunString << this->Values[ctt_START];
    }
    testsToRunString << ",";
    if (this->Values[ctt_END]) {
      testsToRunString << this->Values[ctt_END];
    }
    testsToRunString << ",";
    if (this->Values[ctt_STRIDE]) {
      testsToRunString << this->Values[ctt_STRIDE];
    }
    handler->SetOption("TestsToRunInformation",
                       testsToRunString.str().c_str());
  }
  if (this->Values[ctt_EXCLUDE]) {
    handler->SetOption("ExcludeRegularExpression", this->Values[ctt_EXCLUDE]);
  }
  if (this->Values[ctt_INCLUDE]) {
    handler->SetOption("IncludeRegularExpression", this->Values[ctt_INCLUDE]);
  }
  if (this->Values[ctt_EXCLUDE_LABEL]) {
    handler->SetOption("ExcludeLabelRegularExpression",
                       this->Values[ctt_EXCLUDE_LABEL]);
  }
  if (this->Values[ctt_INCLUDE_LABEL]) {
    handler->SetOption("LabelRegularExpression",
                       this->Values[ctt_INCLUDE_LABEL]);
  }
  if (this->Values[ctt_EXCLUDE_FIXTURE]) {
    handler->SetOption("ExcludeFixtureRegularExpression",
                       this->Values[ctt_EXCLUDE_FIXTURE]);
  }
  if (this->Values[ctt_EXCLUDE_FIXTURE_SETUP]) {
    handler->SetOption("ExcludeFixtureSetupRegularExpression",
                       this->Values[ctt_EXCLUDE_FIXTURE_SETUP]);
  }
  if (this->Values[ctt_EXCLUDE_FIXTURE_CLEANUP]) {
    handler->SetOption("ExcludeFixtureCleanupRegularExpression",
                       this->Values[ctt_EXCLUDE_FIXTURE_CLEANUP]);
  }
  if (this->Values[ctt_PARALLEL_LEVEL]) {
    handler->SetOption("ParallelLevel", this->Values[ctt_PARALLEL_LEVEL]);
  }
  if (this->Values[ctt_SCHEDULE_RANDOM]) {
    handler->SetOption("ScheduleRandom", this->Values[ctt_SCHEDULE_RANDOM]);
  }
  if (this->Values[ctt_STOP_TIME]) {
    this->CTest->SetStopTime(this->Values[ctt_STOP_TIME]);
  }

  // Test load is determined by: TEST_LOAD argument,
  // or CTEST_TEST_LOAD script variable, or ctest --test-load
  // command line argument... in that order.
  unsigned long testLoad;
  const char* ctestTestLoad = this->Makefile->GetDefinition("CTEST_TEST_LOAD");
  if (this->Values[ctt_TEST_LOAD] && *this->Values[ctt_TEST_LOAD]) {
    if (!cmSystemTools::StringToULong(this->Values[ctt_TEST_LOAD],
                                      &testLoad)) {
      testLoad = 0;
      cmCTestLog(this->CTest, WARNING, "Invalid value for 'TEST_LOAD' : "
                   << this->Values[ctt_TEST_LOAD] << std::endl);
    }
  } else if (ctestTestLoad && *ctestTestLoad) {
    if (!cmSystemTools::StringToULong(ctestTestLoad, &testLoad)) {
      testLoad = 0;
      cmCTestLog(this->CTest, WARNING, "Invalid value for 'CTEST_TEST_LOAD' : "
                   << ctestTestLoad << std::endl);
    }
  } else {
    testLoad = this->CTest->GetTestLoad();
  }
  handler->SetTestLoad(testLoad);

  handler->SetQuiet(this->Quiet);
  return handler;
}

cmCTestGenericHandler* cmCTestTestCommand::InitializeActualHandler()
{
  return this->CTest->GetInitializedHandler("test");
}
