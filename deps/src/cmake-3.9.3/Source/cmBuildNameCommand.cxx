/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmBuildNameCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>

#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmBuildNameCommand
bool cmBuildNameCommand::InitialPass(std::vector<std::string> const& args,
                                     cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  const char* cacheValue = this->Makefile->GetDefinition(args[0]);
  if (cacheValue) {
    // do we need to correct the value?
    cmsys::RegularExpression reg("[()/]");
    if (reg.find(cacheValue)) {
      std::string cv = cacheValue;
      std::replace(cv.begin(), cv.end(), '/', '_');
      std::replace(cv.begin(), cv.end(), '(', '_');
      std::replace(cv.begin(), cv.end(), ')', '_');
      this->Makefile->AddCacheDefinition(args[0], cv.c_str(), "Name of build.",
                                         cmStateEnums::STRING);
    }
    return true;
  }

  std::string buildname = "WinNT";
  if (this->Makefile->GetDefinition("UNIX")) {
    buildname = "";
    cmSystemTools::RunSingleCommand("uname -a", &buildname, &buildname);
    if (!buildname.empty()) {
      std::string RegExp = "([^ ]*) [^ ]* ([^ ]*) ";
      cmsys::RegularExpression reg(RegExp.c_str());
      if (reg.find(buildname.c_str())) {
        buildname = reg.match(1) + "-" + reg.match(2);
      }
    }
  }
  std::string compiler = "${CMAKE_CXX_COMPILER}";
  this->Makefile->ExpandVariablesInString(compiler);
  buildname += "-";
  buildname += cmSystemTools::GetFilenameName(compiler);
  std::replace(buildname.begin(), buildname.end(), '/', '_');
  std::replace(buildname.begin(), buildname.end(), '(', '_');
  std::replace(buildname.begin(), buildname.end(), ')', '_');

  this->Makefile->AddCacheDefinition(args[0], buildname.c_str(),
                                     "Name of build.", cmStateEnums::STRING);
  return true;
}
