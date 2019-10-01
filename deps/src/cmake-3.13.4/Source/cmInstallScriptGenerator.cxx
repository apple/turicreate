/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallScriptGenerator.h"

#include "cmScriptGenerator.h"

#include <ostream>
#include <vector>

cmInstallScriptGenerator::cmInstallScriptGenerator(const char* script,
                                                   bool code,
                                                   const char* component,
                                                   bool exclude_from_all)
  : cmInstallGenerator(nullptr, std::vector<std::string>(), component,
                       MessageDefault, exclude_from_all)
  , Script(script)
  , Code(code)
{
}

cmInstallScriptGenerator::~cmInstallScriptGenerator()
{
}

void cmInstallScriptGenerator::GenerateScript(std::ostream& os)
{
  Indent indent;
  std::string component_test =
    this->CreateComponentTest(this->Component.c_str(), this->ExcludeFromAll);
  os << indent << "if(" << component_test << ")\n";

  if (this->Code) {
    os << indent.Next() << this->Script << "\n";
  } else {
    os << indent.Next() << "include(\"" << this->Script << "\")\n";
  }

  os << indent << "endif()\n\n";
}
