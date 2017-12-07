/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include <cmConfigure.h> // IWYU pragma: keep

#include <iostream>
#include <string>
#include <vector>

#include "cmFindPackageCommand.h"

#define cmPassed(m) std::cout << "Passed: " << (m) << "\n"
#define cmFailed(m)                                                           \
  std::cout << "FAILED: " << (m) << "\n";                                     \
  failed = 1

int testFindPackageCommand(int /*unused*/, char* /*unused*/ [])
{
  int failed = 0;

  // ----------------------------------------------------------------------
  // Test cmFindPackage::Sort
  std::vector<std::string> testString;
  testString.push_back("lib-0.0");
  testString.push_back("lib-1.2");
  testString.push_back("lib-2.0");
  testString.push_back("lib-19.0.1");
  testString.push_back("lib-20.01.1");
  testString.push_back("lib-20.2.2a");

  cmFindPackageCommand::Sort(testString.begin(), testString.end(),
                             cmFindPackageCommand::Natural,
                             cmFindPackageCommand::Asc);
  if (!(testString[0] == "lib-0.0" && testString[1] == "lib-1.2" &&
        testString[2] == "lib-2.0" && testString[3] == "lib-19.0.1" &&
        testString[4] == "lib-20.01.1" && testString[5] == "lib-20.2.2a")) {
    cmFailed("cmSystemTools::Sort fail with Natural ASC");
  }

  cmFindPackageCommand::Sort(testString.begin(), testString.end(),
                             cmFindPackageCommand::Natural,
                             cmFindPackageCommand::Dec);
  if (!(testString[5] == "lib-0.0" && testString[4] == "lib-1.2" &&
        testString[3] == "lib-2.0" && testString[2] == "lib-19.0.1" &&
        testString[1] == "lib-20.01.1" && testString[0] == "lib-20.2.2a")) {
    cmFailed("cmSystemTools::Sort fail with Natural ASC");
  }

  cmFindPackageCommand::Sort(testString.begin(), testString.end(),
                             cmFindPackageCommand::Name_order,
                             cmFindPackageCommand::Dec);
  if (!(testString[5] == "lib-0.0" && testString[4] == "lib-1.2" &&
        testString[3] == "lib-19.0.1" && testString[2] == "lib-2.0" &&
        testString[1] == "lib-20.01.1" && testString[0] == "lib-20.2.2a")) {
    cmFailed("cmSystemTools::Sort fail with Name DEC");
  }

  cmFindPackageCommand::Sort(testString.begin(), testString.end(),
                             cmFindPackageCommand::Name_order,
                             cmFindPackageCommand::Asc);
  if (!(testString[0] == "lib-0.0" && testString[1] == "lib-1.2" &&
        testString[2] == "lib-19.0.1" && testString[3] == "lib-2.0" &&
        testString[4] == "lib-20.01.1" && testString[5] == "lib-20.2.2a")) {
    cmFailed("cmSystemTools::Sort fail with Natural ASC");
  }

  if (!failed) {
    cmPassed("cmSystemTools::Sort working");
  }
  return failed;
}
