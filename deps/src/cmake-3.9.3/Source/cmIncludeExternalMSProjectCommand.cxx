/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmIncludeExternalMSProjectCommand.h"

#ifdef _WIN32
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#endif

class cmExecutionStatus;

// cmIncludeExternalMSProjectCommand
bool cmIncludeExternalMSProjectCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("INCLUDE_EXTERNAL_MSPROJECT called with incorrect "
                   "number of arguments");
    return false;
  }
// only compile this for win32 to avoid coverage errors
#ifdef _WIN32
  if (this->Makefile->GetDefinition("WIN32")) {
    enum Doing
    {
      DoingNone,
      DoingType,
      DoingGuid,
      DoingPlatform
    };

    Doing doing = DoingNone;

    std::string customType;
    std::string customGuid;
    std::string platformMapping;

    std::vector<std::string> depends;
    for (unsigned int i = 2; i < args.size(); ++i) {
      if (args[i] == "TYPE") {
        doing = DoingType;
      } else if (args[i] == "GUID") {
        doing = DoingGuid;
      } else if (args[i] == "PLATFORM") {
        doing = DoingPlatform;
      } else {
        switch (doing) {
          case DoingNone:
            depends.push_back(args[i]);
            break;
          case DoingType:
            customType = args[i];
            break;
          case DoingGuid:
            customGuid = args[i];
            break;
          case DoingPlatform:
            platformMapping = args[i];
            break;
        }
        doing = DoingNone;
      }
    }

    // Hack together a utility target storing enough information
    // to reproduce the target inclusion.
    std::string utility_name = args[0];

    std::string path = args[1];
    cmSystemTools::ConvertToUnixSlashes(path);

    if (!customGuid.empty()) {
      std::string guidVariable = utility_name + "_GUID_CMAKE";
      this->Makefile->GetCMakeInstance()->AddCacheEntry(
        guidVariable.c_str(), customGuid.c_str(), "Stored GUID",
        cmStateEnums::INTERNAL);
    }

    // Create a target instance for this utility.
    cmTarget* target = this->Makefile->AddNewTarget(cmStateEnums::UTILITY,
                                                    utility_name.c_str());

    target->SetProperty("GENERATOR_FILE_NAME", utility_name.c_str());
    target->SetProperty("EXTERNAL_MSPROJECT", path.c_str());
    target->SetProperty("EXCLUDE_FROM_ALL", "FALSE");

    if (!customType.empty())
      target->SetProperty("VS_PROJECT_TYPE", customType.c_str());
    if (!platformMapping.empty())
      target->SetProperty("VS_PLATFORM_MAPPING", platformMapping.c_str());

    for (std::vector<std::string>::const_iterator it = depends.begin();
         it != depends.end(); ++it) {
      target->AddUtility(it->c_str());
    }
  }
#endif
  return true;
}
