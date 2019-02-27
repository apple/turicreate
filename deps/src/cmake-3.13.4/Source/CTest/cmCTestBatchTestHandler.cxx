/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestBatchTestHandler.h"

#include "cmCTest.h"
#include "cmCTestMultiProcessHandler.h"
#include "cmCTestTestHandler.h"
#include "cmProcess.h"
#include "cmSystemTools.h"

#include <map>
#include <utility>
#include <vector>

cmCTestBatchTestHandler::~cmCTestBatchTestHandler()
{
}

void cmCTestBatchTestHandler::RunTests()
{
  this->WriteBatchScript();
  this->SubmitBatchScript();
}

void cmCTestBatchTestHandler::WriteBatchScript()
{
  this->Script = this->CTest->GetBinaryDir() + "/Testing/CTestBatch.txt";
  cmsys::ofstream fout;
  fout.open(this->Script.c_str());
  fout << "#!/bin/sh\n";

  for (TestMap::iterator i = this->Tests.begin(); i != this->Tests.end();
       ++i) {
    this->WriteSrunArgs(i->first, fout);
    this->WriteTestCommand(i->first, fout);
    fout << "\n";
  }
  fout.flush();
  fout.close();
}

void cmCTestBatchTestHandler::WriteSrunArgs(int test, std::ostream& fout)
{
  cmCTestTestHandler::cmCTestTestProperties* properties =
    this->Properties[test];

  fout << "srun ";
  // fout << "--jobid=" << test << " ";
  fout << "-J=" << properties->Name << " ";

  // Write dependency information
  /*if(!this->Tests[test].empty())
    {
      fout << "-P=afterany";
      for(TestSet::iterator i = this->Tests[test].begin();
          i != this->Tests[test].end(); ++i)
        {
          fout << ":" << *i;
        }
      fout << " ";
    }*/
  if (properties->RunSerial) {
    fout << "--exclusive ";
  }
  if (properties->Processors > 1) {
    fout << "-n" << properties->Processors << " ";
  }
}

void cmCTestBatchTestHandler::WriteTestCommand(int test, std::ostream& fout)
{
  std::vector<std::string> args = this->Properties[test]->Args;
  std::vector<std::string> processArgs;
  std::string command;

  command = this->TestHandler->FindTheExecutable(args[1].c_str());
  command = cmSystemTools::ConvertToOutputPath(command.c_str());

  // Prepends memcheck args to our command string if this is a memcheck
  this->TestHandler->GenerateTestCommand(processArgs, test);
  processArgs.push_back(command);

  for (std::vector<std::string>::iterator arg = processArgs.begin();
       arg != processArgs.end(); ++arg) {
    fout << *arg << " ";
  }

  std::vector<std::string>::iterator i = args.begin();
  ++i; // the test name
  ++i; // the executable (command)
  if (args.size() > 2) {
    fout << "'";
  }
  while (i != args.end()) {
    fout << "\"" << *i << "\""; // args to the test executable
    ++i;

    if (i == args.end() && args.size() > 2) {
      fout << "'";
    }
    fout << " ";
  }
  // TODO ZACH build TestResult.FullCommandLine
  // this->TestResult.FullCommandLine = this->TestCommand;
}

void cmCTestBatchTestHandler::SubmitBatchScript()
{
  cmProcess sbatch;
  std::vector<std::string> args;
  args.push_back(this->Script);
  args.push_back("-o");
  args.push_back(this->CTest->GetBinaryDir() + "/Testing/CTestBatch.txt");

  sbatch.SetCommand("sbatch");
  sbatch.SetCommandArguments(args);
  /*if(sbatch.StartProcess())
    {
      //success condition
    }
  else
    {
      //fail condition
    }*/
}
