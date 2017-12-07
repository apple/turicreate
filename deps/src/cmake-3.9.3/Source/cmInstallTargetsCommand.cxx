/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallTargetsCommand.h"

#include <utility>

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmTarget.h"
#include "cm_unordered_map.hxx"

class cmExecutionStatus;

// cmExecutableCommand
bool cmInstallTargetsCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Enable the install target.
  this->Makefile->GetGlobalGenerator()->EnableInstallTarget();

  cmTargets& tgts = this->Makefile->GetTargets();
  std::vector<std::string>::const_iterator s = args.begin();
  ++s;
  std::string runtime_dir = "/bin";
  for (; s != args.end(); ++s) {
    if (*s == "RUNTIME_DIRECTORY") {
      ++s;
      if (s == args.end()) {
        this->SetError("called with RUNTIME_DIRECTORY but no actual "
                       "directory");
        return false;
      }

      runtime_dir = *s;
    } else {
      cmTargets::iterator ti = tgts.find(*s);
      if (ti != tgts.end()) {
        ti->second.SetInstallPath(args[0].c_str());
        ti->second.SetRuntimeInstallPath(runtime_dir.c_str());
        ti->second.SetHaveInstallRule(true);
      } else {
        std::string str = "Cannot find target: \"" + *s + "\" to install.";
        this->SetError(str);
        return false;
      }
    }
  }

  this->Makefile->GetGlobalGenerator()->AddInstallComponent(
    this->Makefile->GetSafeDefinition("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME"));

  return true;
}
