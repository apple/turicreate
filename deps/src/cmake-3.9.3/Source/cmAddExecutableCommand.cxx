/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddExecutableCommand.h"

#include <sstream>

#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmTarget.h"
#include "cmake.h"

class cmExecutionStatus;

// cmExecutableCommand
bool cmAddExecutableCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::vector<std::string>::const_iterator s = args.begin();

  std::string const& exename = *s;

  ++s;
  bool use_win32 = false;
  bool use_macbundle = false;
  bool excludeFromAll = false;
  bool importTarget = false;
  bool importGlobal = false;
  bool isAlias = false;
  while (s != args.end()) {
    if (*s == "WIN32") {
      ++s;
      use_win32 = true;
    } else if (*s == "MACOSX_BUNDLE") {
      ++s;
      use_macbundle = true;
    } else if (*s == "EXCLUDE_FROM_ALL") {
      ++s;
      excludeFromAll = true;
    } else if (*s == "IMPORTED") {
      ++s;
      importTarget = true;
    } else if (importTarget && *s == "GLOBAL") {
      ++s;
      importGlobal = true;
    } else if (*s == "ALIAS") {
      ++s;
      isAlias = true;
    } else {
      break;
    }
  }

  bool nameOk = cmGeneratorExpression::IsValidTargetName(exename) &&
    !cmGlobalGenerator::IsReservedTarget(exename);

  if (nameOk && !importTarget && !isAlias) {
    nameOk = exename.find(':') == std::string::npos;
  }
  if (!nameOk) {
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    std::ostringstream e;
    bool issueMessage = false;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0037)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0037) << "\n";
        issueMessage = true;
      case cmPolicies::OLD:
        break;
      case cmPolicies::NEW:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
        issueMessage = true;
        messageType = cmake::FATAL_ERROR;
    }
    if (issueMessage) {
      /* clang-format off */
      e << "The target name \"" << exename <<
          "\" is reserved or not valid for certain "
          "CMake features, such as generator expressions, and may result "
          "in undefined behavior.";
      /* clang-format on */
      this->Makefile->IssueMessage(messageType, e.str());

      if (messageType == cmake::FATAL_ERROR) {
        return false;
      }
    }
  }

  // Special modifiers are not allowed with IMPORTED signature.
  if (importTarget && (use_win32 || use_macbundle || excludeFromAll)) {
    if (use_win32) {
      this->SetError("may not be given WIN32 for an IMPORTED target.");
    } else if (use_macbundle) {
      this->SetError("may not be given MACOSX_BUNDLE for an IMPORTED target.");
    } else // if(excludeFromAll)
    {
      this->SetError(
        "may not be given EXCLUDE_FROM_ALL for an IMPORTED target.");
    }
    return false;
  }
  if (isAlias) {
    if (!cmGeneratorExpression::IsValidTargetName(exename)) {
      this->SetError("Invalid name for ALIAS: " + exename);
      return false;
    }
    if (excludeFromAll) {
      this->SetError("EXCLUDE_FROM_ALL with ALIAS makes no sense.");
      return false;
    }
    if (importTarget || importGlobal) {
      this->SetError("IMPORTED with ALIAS is not allowed.");
      return false;
    }
    if (args.size() != 3) {
      std::ostringstream e;
      e << "ALIAS requires exactly one target argument.";
      this->SetError(e.str());
      return false;
    }

    const char* aliasedName = s->c_str();
    if (this->Makefile->IsAlias(aliasedName)) {
      std::ostringstream e;
      e << "cannot create ALIAS target \"" << exename << "\" because target \""
        << aliasedName << "\" is itself an ALIAS.";
      this->SetError(e.str());
      return false;
    }
    cmTarget* aliasedTarget =
      this->Makefile->FindTargetToUse(aliasedName, true);
    if (!aliasedTarget) {
      std::ostringstream e;
      e << "cannot create ALIAS target \"" << exename << "\" because target \""
        << aliasedName << "\" does not already "
                          "exist.";
      this->SetError(e.str());
      return false;
    }
    cmStateEnums::TargetType type = aliasedTarget->GetType();
    if (type != cmStateEnums::EXECUTABLE) {
      std::ostringstream e;
      e << "cannot create ALIAS target \"" << exename << "\" because target \""
        << aliasedName << "\" is not an "
                          "executable.";
      this->SetError(e.str());
      return false;
    }
    if (aliasedTarget->IsImported()) {
      std::ostringstream e;
      e << "cannot create ALIAS target \"" << exename << "\" because target \""
        << aliasedName << "\" is IMPORTED.";
      this->SetError(e.str());
      return false;
    }
    this->Makefile->AddAlias(exename, aliasedName);
    return true;
  }

  // Handle imported target creation.
  if (importTarget) {
    // Make sure the target does not already exist.
    if (this->Makefile->FindTargetToUse(exename)) {
      std::ostringstream e;
      e << "cannot create imported target \"" << exename
        << "\" because another target with the same name already exists.";
      this->SetError(e.str());
      return false;
    }

    // Create the imported target.
    this->Makefile->AddImportedTarget(exename, cmStateEnums::EXECUTABLE,
                                      importGlobal);
    return true;
  }

  // Enforce name uniqueness.
  {
    std::string msg;
    if (!this->Makefile->EnforceUniqueName(exename, msg)) {
      this->SetError(msg);
      return false;
    }
  }

  if (s == args.end()) {
    this->SetError(
      "called with incorrect number of arguments, no sources provided");
    return false;
  }

  std::vector<std::string> srclists(s, args.end());
  cmTarget* tgt =
    this->Makefile->AddExecutable(exename.c_str(), srclists, excludeFromAll);
  if (use_win32) {
    tgt->SetProperty("WIN32_EXECUTABLE", "ON");
  }
  if (use_macbundle) {
    tgt->SetProperty("MACOSX_BUNDLE", "ON");
  }

  return true;
}
