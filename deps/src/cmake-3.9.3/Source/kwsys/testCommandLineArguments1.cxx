/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(CommandLineArguments.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "CommandLineArguments.hxx.in"
#endif

#include <iostream>
#include <vector>

#include <assert.h> /* assert */
#include <string.h> /* strcmp */

int testCommandLineArguments1(int argc, char* argv[])
{
  kwsys::CommandLineArguments arg;
  arg.Initialize(argc, argv);

  int n = 0;
  char* m = 0;
  std::string p;
  int res = 0;

  typedef kwsys::CommandLineArguments argT;
  arg.AddArgument("-n", argT::SPACE_ARGUMENT, &n, "Argument N");
  arg.AddArgument("-m", argT::EQUAL_ARGUMENT, &m, "Argument M");
  arg.AddBooleanArgument("-p", &p, "Argument P");

  arg.StoreUnusedArguments(true);

  if (!arg.Parse()) {
    std::cerr << "Problem parsing arguments" << std::endl;
    res = 1;
  }
  if (n != 24) {
    std::cout << "Problem setting N. Value of N: " << n << std::endl;
    res = 1;
  }
  if (!m || strcmp(m, "test value") != 0) {
    std::cout << "Problem setting M. Value of M: " << m << std::endl;
    res = 1;
  }
  if (p != "1") {
    std::cout << "Problem setting P. Value of P: " << p << std::endl;
    res = 1;
  }
  std::cout << "Value of N: " << n << std::endl;
  std::cout << "Value of M: " << m << std::endl;
  std::cout << "Value of P: " << p << std::endl;
  if (m) {
    delete[] m;
  }

  char** newArgv = 0;
  int newArgc = 0;
  arg.GetUnusedArguments(&newArgc, &newArgv);
  int cc;
  const char* valid_unused_args[9] = { 0,
                                       "--ignored",
                                       "--second-ignored",
                                       "third-ignored",
                                       "some",
                                       "junk",
                                       "at",
                                       "the",
                                       "end" };
  if (newArgc != 9) {
    std::cerr << "Bad number of unused arguments: " << newArgc << std::endl;
    res = 1;
  }
  for (cc = 0; cc < newArgc; ++cc) {
    assert(newArgv[cc]); /* Quiet Clang scan-build. */
    std::cout << "Unused argument[" << cc << "] = [" << newArgv[cc] << "]"
              << std::endl;
    if (cc >= 9) {
      std::cerr << "Too many unused arguments: " << cc << std::endl;
      res = 1;
    } else if (valid_unused_args[cc] &&
               strcmp(valid_unused_args[cc], newArgv[cc]) != 0) {
      std::cerr << "Bad unused argument [" << cc << "] \"" << newArgv[cc]
                << "\" should be: \"" << valid_unused_args[cc] << "\""
                << std::endl;
      res = 1;
    }
  }
  arg.DeleteRemainingArguments(newArgc, &newArgv);

  return res;
}
