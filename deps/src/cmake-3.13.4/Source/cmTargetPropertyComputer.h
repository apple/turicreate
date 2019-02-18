/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetPropertyComputer_h
#define cmTargetPropertyComputer_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmAlgorithms.h"
#include "cmListFileCache.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmMessenger;

class cmTargetPropertyComputer
{
public:
  template <typename Target>
  static const char* GetProperty(Target const* tgt, const std::string& prop,
                                 cmMessenger* messenger,
                                 cmListFileBacktrace const& context)
  {
    if (const char* loc = GetLocation(tgt, prop, messenger, context)) {
      return loc;
    }
    if (cmSystemTools::GetFatalErrorOccured()) {
      return nullptr;
    }
    if (prop == "SOURCES") {
      return GetSources(tgt, messenger, context);
    }
    return nullptr;
  }

  static bool WhiteListedInterfaceProperty(const std::string& prop);

  static bool PassesWhitelist(cmStateEnums::TargetType tgtType,
                              std::string const& prop, cmMessenger* messenger,
                              cmListFileBacktrace const& context);

private:
  static bool HandleLocationPropertyPolicy(std::string const& tgtName,
                                           cmMessenger* messenger,
                                           cmListFileBacktrace const& context);

  template <typename Target>
  static const char* ComputeLocationForBuild(Target const* tgt);
  template <typename Target>
  static const char* ComputeLocation(Target const* tgt,
                                     std::string const& config);

  template <typename Target>
  static const char* GetLocation(Target const* tgt, std::string const& prop,
                                 cmMessenger* messenger,
                                 cmListFileBacktrace const& context)

  {
    // Watch for special "computed" properties that are dependent on
    // other properties or variables.  Always recompute them.
    if (tgt->GetType() == cmStateEnums::EXECUTABLE ||
        tgt->GetType() == cmStateEnums::STATIC_LIBRARY ||
        tgt->GetType() == cmStateEnums::SHARED_LIBRARY ||
        tgt->GetType() == cmStateEnums::MODULE_LIBRARY ||
        tgt->GetType() == cmStateEnums::UNKNOWN_LIBRARY) {
      static const std::string propLOCATION = "LOCATION";
      if (prop == propLOCATION) {
        if (!tgt->IsImported() &&
            !HandleLocationPropertyPolicy(tgt->GetName(), messenger,
                                          context)) {
          return nullptr;
        }
        return ComputeLocationForBuild(tgt);
      }

      // Support "LOCATION_<CONFIG>".
      if (cmHasLiteralPrefix(prop, "LOCATION_")) {
        if (!tgt->IsImported() &&
            !HandleLocationPropertyPolicy(tgt->GetName(), messenger,
                                          context)) {
          return nullptr;
        }
        const char* configName = prop.c_str() + 9;
        return ComputeLocation(tgt, configName);
      }

      // Support "<CONFIG>_LOCATION".
      if (cmHasLiteralSuffix(prop, "_LOCATION") &&
          !cmHasLiteralPrefix(prop, "XCODE_ATTRIBUTE_")) {
        std::string configName(prop.c_str(), prop.size() - 9);
        if (configName != "IMPORTED") {
          if (!tgt->IsImported() &&
              !HandleLocationPropertyPolicy(tgt->GetName(), messenger,
                                            context)) {
            return nullptr;
          }
          return ComputeLocation(tgt, configName);
        }
      }
    }
    return nullptr;
  }

  template <typename Target>
  static const char* GetSources(Target const* tgt, cmMessenger* messenger,
                                cmListFileBacktrace const& context);
};

#endif
