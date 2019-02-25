/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportBuildAndroidMKGenerator.h"

#include <algorithm>
#include <memory> // IWYU pragma: keep
#include <sstream>
#include <utility>

#include "cmAlgorithms.h"
#include "cmGeneratorTarget.h"
#include "cmLinkItem.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

cmExportBuildAndroidMKGenerator::cmExportBuildAndroidMKGenerator()
{
  this->LG = nullptr;
  this->ExportSet = nullptr;
}

void cmExportBuildAndroidMKGenerator::GenerateImportHeaderCode(
  std::ostream& os, const std::string&)
{
  os << "LOCAL_PATH := $(call my-dir)\n\n";
}

void cmExportBuildAndroidMKGenerator::GenerateImportFooterCode(std::ostream&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateExpectedTargetsCode(
  std::ostream&, const std::string&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateImportTargetCode(
  std::ostream& os, cmGeneratorTarget const* target,
  cmStateEnums::TargetType /*targetType*/)
{
  std::string targetName = this->Namespace;
  targetName += target->GetExportName();
  os << "include $(CLEAR_VARS)\n";
  os << "LOCAL_MODULE := ";
  os << targetName << "\n";
  os << "LOCAL_SRC_FILES := ";
  std::string path = cmSystemTools::ConvertToOutputPath(target->GetFullPath());
  os << path << "\n";
}

void cmExportBuildAndroidMKGenerator::GenerateImportPropertyCode(
  std::ostream&, const std::string&, cmGeneratorTarget const*,
  ImportPropertyMap const&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateMissingTargetsCheckCode(
  std::ostream&, const std::vector<std::string>&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties)
{
  std::string config;
  if (!this->Configurations.empty()) {
    config = this->Configurations[0];
  }
  cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
    target, os, properties, cmExportBuildAndroidMKGenerator::BUILD, config);
}

void cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties, GenerateType type,
  std::string const& config)
{
  const bool newCMP0022Behavior =
    target->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
    target->GetPolicyStatusCMP0022() != cmPolicies::OLD;
  if (!newCMP0022Behavior) {
    std::ostringstream w;
    if (type == cmExportBuildAndroidMKGenerator::BUILD) {
      w << "export(TARGETS ... ANDROID_MK) called with policy CMP0022";
    } else {
      w << "install( EXPORT_ANDROID_MK ...) called with policy CMP0022";
    }
    w << " set to OLD for target " << target->Target->GetName() << ". "
      << "The export will only work with CMP0022 set to NEW.";
    target->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w.str());
  }
  if (!properties.empty()) {
    os << "LOCAL_CPP_FEATURES := rtti exceptions\n";
    for (auto const& property : properties) {
      if (property.first == "INTERFACE_COMPILE_OPTIONS") {
        os << "LOCAL_CPP_FEATURES += ";
        os << (property.second) << "\n";
      } else if (property.first == "INTERFACE_LINK_LIBRARIES") {
        std::string staticLibs;
        std::string sharedLibs;
        std::string ldlibs;
        cmLinkInterfaceLibraries const* linkIFace =
          target->GetLinkInterfaceLibraries(config, target, false);
        for (cmLinkItem const& item : linkIFace->Libraries) {
          cmGeneratorTarget const* gt = item.Target;
          std::string const& lib = item.AsStr();
          if (gt) {

            if (gt->GetType() == cmStateEnums::SHARED_LIBRARY ||
                gt->GetType() == cmStateEnums::MODULE_LIBRARY) {
              sharedLibs += " " + lib;
            } else {
              staticLibs += " " + lib;
            }
          } else {
            bool relpath = false;
            if (type == cmExportBuildAndroidMKGenerator::INSTALL) {
              relpath = lib.substr(0, 3) == "../";
            }
            // check for full path or if it already has a -l, or
            // in the case of an install check for relative paths
            // if it is full or a link library then use string directly
            if (cmSystemTools::FileIsFullPath(lib) ||
                lib.substr(0, 2) == "-l" || relpath) {
              ldlibs += " " + lib;
              // if it is not a path and does not have a -l then add -l
            } else if (!lib.empty()) {
              ldlibs += " -l" + lib;
            }
          }
        }
        if (!sharedLibs.empty()) {
          os << "LOCAL_SHARED_LIBRARIES :=" << sharedLibs << "\n";
        }
        if (!staticLibs.empty()) {
          os << "LOCAL_STATIC_LIBRARIES :=" << staticLibs << "\n";
        }
        if (!ldlibs.empty()) {
          os << "LOCAL_EXPORT_LDLIBS :=" << ldlibs << "\n";
        }
      } else if (property.first == "INTERFACE_INCLUDE_DIRECTORIES") {
        std::string includes = property.second;
        std::vector<std::string> includeList;
        cmSystemTools::ExpandListArgument(includes, includeList);
        os << "LOCAL_EXPORT_C_INCLUDES := ";
        std::string end;
        for (std::string const& i : includeList) {
          os << end << i;
          end = "\\\n";
        }
        os << "\n";
      } else if (property.first == "INTERFACE_LINK_OPTIONS") {
        os << "LOCAL_EXPORT_LDFLAGS := ";
        std::vector<std::string> linkFlagsList;
        cmSystemTools::ExpandListArgument(property.second, linkFlagsList);
        os << cmJoin(linkFlagsList, " ") << "\n";
      } else {
        os << "# " << property.first << " " << (property.second) << "\n";
      }
    }
  }

  // Tell the NDK build system if prebuilt static libraries use C++.
  if (target->GetType() == cmStateEnums::STATIC_LIBRARY) {
    cmLinkImplementation const* li = target->GetLinkImplementation(config);
    if (std::find(li->Languages.begin(), li->Languages.end(), "CXX") !=
        li->Languages.end()) {
      os << "LOCAL_HAS_CPP := true\n";
    }
  }

  switch (target->GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
      os << "include $(PREBUILT_SHARED_LIBRARY)\n";
      break;
    case cmStateEnums::STATIC_LIBRARY:
      os << "include $(PREBUILT_STATIC_LIBRARY)\n";
      break;
    case cmStateEnums::EXECUTABLE:
    case cmStateEnums::UTILITY:
    case cmStateEnums::OBJECT_LIBRARY:
    case cmStateEnums::GLOBAL_TARGET:
    case cmStateEnums::INTERFACE_LIBRARY:
    case cmStateEnums::UNKNOWN_LIBRARY:
      break;
  }
  os << "\n";
}
