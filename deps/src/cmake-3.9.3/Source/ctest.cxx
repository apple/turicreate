/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmConfigure.h"

#include "CTest/cmCTestLaunch.h"
#include "CTest/cmCTestScriptHandler.h"
#include "cmCTest.h"
#include "cmDocumentation.h"
#include "cmSystemTools.h"

#include "cmsys/Encoding.hxx"
#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmsys/ConsoleBuf.hxx"
#endif
#include <iostream>
#include <string.h>
#include <string>
#include <vector>

static const char* cmDocumentationName[][2] = {
  { CM_NULLPTR, "  ctest - Testing driver provided by CMake." },
  { CM_NULLPTR, CM_NULLPTR }
};

static const char* cmDocumentationUsage[][2] = { { CM_NULLPTR,
                                                   "  ctest [options]" },
                                                 { CM_NULLPTR, CM_NULLPTR } };

static const char* cmDocumentationOptions[][2] = {
  { "-C <cfg>, --build-config <cfg>", "Choose configuration to test." },
  { "-V,--verbose", "Enable verbose output from tests." },
  { "-VV,--extra-verbose", "Enable more verbose output from tests." },
  { "--debug", "Displaying more verbose internals of CTest." },
  { "--output-on-failure", "Output anything outputted by the test program "
                           "if the test should fail." },
  { "--test-output-size-passed <size>", "Limit the output for passed tests "
                                        "to <size> bytes" },
  { "--test-output-size-failed <size>", "Limit the output for failed tests "
                                        "to <size> bytes" },
  { "-F", "Enable failover." },
  { "-j <jobs>, --parallel <jobs>", "Run the tests in parallel using the "
                                    "given number of jobs." },
  { "-Q,--quiet", "Make ctest quiet." },
  { "-O <file>, --output-log <file>", "Output to log file" },
  { "-N,--show-only", "Disable actual execution of tests." },
  { "-L <regex>, --label-regex <regex>", "Run tests with labels matching "
                                         "regular expression." },
  { "-R <regex>, --tests-regex <regex>", "Run tests matching regular "
                                         "expression." },
  { "-E <regex>, --exclude-regex <regex>", "Exclude tests matching regular "
                                           "expression." },
  { "-LE <regex>, --label-exclude <regex>", "Exclude tests with labels "
                                            "matching regular expression." },
  { "-FA <regex>, --fixture-exclude-any <regex>", "Do not automatically "
                                                  "add any tests for "
                                                  "fixtures matching "
                                                  "regular expression." },
  { "-FS <regex>, --fixture-exclude-setup <regex>", "Do not automatically "
                                                    "add setup tests for "
                                                    "fixtures matching "
                                                    "regular expression." },
  { "-FC <regex>, --fixture-exclude-cleanup <regex>", "Do not automatically "
                                                      "add cleanup tests for "
                                                      "fixtures matching "
                                                      "regular expression." },
  { "-D <dashboard>, --dashboard <dashboard>", "Execute dashboard test" },
  { "-D <var>:<type>=<value>", "Define a variable for script mode" },
  { "-M <model>, --test-model <model>", "Sets the model for a dashboard" },
  { "-T <action>, --test-action <action>", "Sets the dashboard action to "
                                           "perform" },
  { "--track <track>", "Specify the track to submit dashboard to" },
  { "-S <script>, --script <script>", "Execute a dashboard for a "
                                      "configuration" },
  { "-SP <script>, --script-new-process <script>", "Execute a dashboard for a "
                                                   "configuration" },
  { "-A <file>, --add-notes <file>", "Add a notes file with submission" },
  { "-I [Start,End,Stride,test#,test#|Test file], --tests-information",
    "Run a specific number of tests by number." },
  { "-U, --union", "Take the Union of -I and -R" },
  { "--rerun-failed", "Run only the tests that failed previously" },
  { "--repeat-until-fail <n>", "Require each test to run <n> "
                               "times without failing in order to pass" },
  { "--max-width <width>", "Set the max width for a test name to output" },
  { "--interactive-debug-mode [0|1]", "Set the interactive mode to 0 or 1." },
  { "--no-label-summary", "Disable timing summary information for labels." },
  { "--build-and-test", "Configure, build and run a test." },
  { "--build-target", "Specify a specific target to build." },
  { "--build-nocmake", "Run the build without running cmake first." },
  { "--build-run-dir", "Specify directory to run programs from." },
  { "--build-two-config", "Run CMake twice" },
  { "--build-exe-dir", "Specify the directory for the executable." },
  { "--build-generator", "Specify the generator to use." },
  { "--build-generator-platform", "Specify the generator-specific platform." },
  { "--build-generator-toolset", "Specify the generator-specific toolset." },
  { "--build-project", "Specify the name of the project to build." },
  { "--build-makeprogram", "Specify the make program to use." },
  { "--build-noclean", "Skip the make clean step." },
  { "--build-config-sample",
    "A sample executable to use to determine the configuration" },
  { "--build-options", "Add extra options to the build step." },

  { "--test-command", "The test to run with the --build-and-test option." },
  { "--test-timeout", "The time limit in seconds, internal use only." },
  { "--test-load", "CPU load threshold for starting new parallel tests." },
  { "--tomorrow-tag", "Nightly or experimental starts with next day tag." },
  { "--ctest-config", "The configuration file used to initialize CTest state "
                      "when submitting dashboards." },
  { "--overwrite", "Overwrite CTest configuration option." },
  { "--extra-submit <file>[;<file>]", "Submit extra files to the dashboard." },
  { "--force-new-ctest-process",
    "Run child CTest instances as new processes" },
  { "--schedule-random", "Use a random order for scheduling tests" },
  { "--submit-index",
    "Submit individual dashboard tests with specific index" },
  { "--timeout <seconds>", "Set a global timeout on all tests." },
  { "--stop-time <time>",
    "Set a time at which all tests should stop running." },
  { "--http1.0", "Submit using HTTP 1.0." },
  { "--no-compress-output", "Do not compress test output when submitting." },
  { "--print-labels", "Print all available test labels." },
  { CM_NULLPTR, CM_NULLPTR }
};

// this is a test driver program for cmCTest.
int main(int argc, char const* const* argv)
{
#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
  // Replace streambuf so we can output Unicode to console
  cmsys::ConsoleBuf::Manager consoleOut(std::cout);
  consoleOut.SetUTF8Pipes();
  cmsys::ConsoleBuf::Manager consoleErr(std::cerr, true);
  consoleErr.SetUTF8Pipes();
#endif
  cmsys::Encoding::CommandLineArguments encoding_args =
    cmsys::Encoding::CommandLineArguments::Main(argc, argv);
  argc = encoding_args.argc();
  argv = encoding_args.argv();

  cmSystemTools::DoNotInheritStdPipes();
  cmSystemTools::EnableMSVCDebugHook();
  cmSystemTools::FindCMakeResources(argv[0]);

  // Dispatch 'ctest --launch' mode directly.
  if (argc >= 2 && strcmp(argv[1], "--launch") == 0) {
    return cmCTestLaunch::Main(argc, argv);
  }

  cmCTest inst;

  if (cmSystemTools::GetCurrentWorkingDirectory().empty()) {
    cmCTestLog(&inst, ERROR_MESSAGE,
               "Current working directory cannot be established."
                 << std::endl);
    return 1;
  }

  // If there is a testing input file, check for documentation options
  // only if there are actually arguments.  We want running without
  // arguments to run tests.
  if (argc > 1 ||
      !(cmSystemTools::FileExists("CTestTestfile.cmake") ||
        cmSystemTools::FileExists("DartTestfile.txt"))) {
    if (argc == 1) {
      cmCTestLog(&inst, ERROR_MESSAGE, "*********************************"
                   << std::endl
                   << "No test configuration file found!" << std::endl
                   << "*********************************" << std::endl);
    }
    cmDocumentation doc;
    doc.addCTestStandardDocSections();
    if (doc.CheckOptions(argc, argv)) {
      // Construct and print requested documentation.
      cmCTestScriptHandler* ch =
        static_cast<cmCTestScriptHandler*>(inst.GetHandler("script"));
      ch->CreateCMake();

      doc.SetShowGenerators(false);
      doc.SetName("ctest");
      doc.SetSection("Name", cmDocumentationName);
      doc.SetSection("Usage", cmDocumentationUsage);
      doc.PrependSection("Options", cmDocumentationOptions);
      return doc.PrintRequestedDocumentation(std::cout) ? 0 : 1;
    }
  }

  // copy the args to a vector
  std::vector<std::string> args;
  for (int i = 0; i < argc; ++i) {
    args.push_back(argv[i]);
  }
  // run ctest
  std::string output;
  int res = inst.Run(args, &output);
  cmCTestLog(&inst, OUTPUT, output);

  return res;
}
