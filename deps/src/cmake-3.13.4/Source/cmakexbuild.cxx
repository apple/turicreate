/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmsys/Process.h"
#include <iostream>
#include <string>
#include <vector>

#include "cmDuration.h"
#include "cmSystemTools.h"

// This is a wrapper program for xcodebuild
// it calls xcodebuild, and does two things
// it removes much of the output, all the setenv
// stuff.  Also, it checks for the text file busy
// error, and re-runs xcodebuild until that error does
// not show up.

int RunXCode(std::vector<const char*>& argv, bool& hitbug)
{
  hitbug = false;
  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmsysProcess_SetTimeout(cp, 0);
  cmsysProcess_Execute(cp);
  std::vector<char> out;
  std::vector<char> err;
  std::string line;
  int pipe =
    cmSystemTools::WaitForLine(cp, line, std::chrono::seconds(100), out, err);
  while (pipe != cmsysProcess_Pipe_None) {
    if (line.find("/bin/sh: bad interpreter: Text file busy") !=
        std::string::npos) {
      hitbug = true;
      std::cerr << "Hit xcodebuild bug : " << line << "\n";
    }
    // if the bug is hit, no more output should be generated
    // because it may contain bogus errors
    // also remove all output with setenv in it to tone down
    // the verbosity of xcodebuild
    if (!hitbug && (line.find("setenv") == std::string::npos)) {
      if (pipe == cmsysProcess_Pipe_STDERR) {
        std::cerr << line << "\n";
      } else if (pipe == cmsysProcess_Pipe_STDOUT) {
        std::cout << line << "\n";
      }
    }
    pipe = cmSystemTools::WaitForLine(cp, line, std::chrono::seconds(100), out,
                                      err);
  }
  cmsysProcess_WaitForExit(cp, nullptr);
  if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exited) {
    return cmsysProcess_GetExitValue(cp);
  }
  if (cmsysProcess_GetState(cp) == cmsysProcess_State_Error) {
    return -1;
  }
  return -1;
}

int main(int ac, char* av[])
{
  std::vector<const char*> argv;
  argv.push_back("xcodebuild");
  for (int i = 1; i < ac; i++) {
    argv.push_back(av[i]);
  }
  argv.push_back(nullptr);
  bool hitbug = true;
  int ret = 0;
  while (hitbug) {
    ret = RunXCode(argv, hitbug);
  }
  if (ret < 0) {
    return 255;
  }
  return ret;
}
