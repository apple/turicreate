/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallGenerator.h"

#include "cmMakefile.h"
#include "cmSystemTools.h"

#include <ostream>

cmInstallGenerator::cmInstallGenerator(
  const char* destination, std::vector<std::string> const& configurations,
  const char* component, MessageLevel message, bool exclude_from_all)
  : cmScriptGenerator("CMAKE_INSTALL_CONFIG_NAME", configurations)
  , Destination(destination ? destination : "")
  , Component(component ? component : "")
  , Message(message)
  , ExcludeFromAll(exclude_from_all)
{
}

cmInstallGenerator::~cmInstallGenerator()
{
}

void cmInstallGenerator::AddInstallRule(
  std::ostream& os, std::string const& dest, cmInstallType type,
  std::vector<std::string> const& files, bool optional /* = false */,
  const char* permissions_file /* = 0 */,
  const char* permissions_dir /* = 0 */, const char* rename /* = 0 */,
  const char* literal_args /* = 0 */, Indent indent)
{
  // Use the FILE command to install the file.
  std::string stype;
  switch (type) {
    case cmInstallType_DIRECTORY:
      stype = "DIRECTORY";
      break;
    case cmInstallType_PROGRAMS:
      stype = "PROGRAM";
      break;
    case cmInstallType_EXECUTABLE:
      stype = "EXECUTABLE";
      break;
    case cmInstallType_STATIC_LIBRARY:
      stype = "STATIC_LIBRARY";
      break;
    case cmInstallType_SHARED_LIBRARY:
      stype = "SHARED_LIBRARY";
      break;
    case cmInstallType_MODULE_LIBRARY:
      stype = "MODULE";
      break;
    case cmInstallType_FILES:
      stype = "FILE";
      break;
  }
  os << indent;
  if (cmSystemTools::FileIsFullPath(dest.c_str())) {
    os << "list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES\n";
    os << indent << " \"";
    for (std::vector<std::string>::const_iterator fi = files.begin();
         fi != files.end(); ++fi) {
      if (fi != files.begin()) {
        os << ";";
      }
      os << dest << "/";
      if (rename && *rename) {
        os << rename;
      } else {
        os << cmSystemTools::GetFilenameName(*fi);
      }
    }
    os << "\")\n";
    os << indent << "if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)\n";
    os << indent << indent << "message(WARNING \"ABSOLUTE path INSTALL "
       << "DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}\")\n";
    os << indent << "endif()\n";

    os << indent << "if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)\n";
    os << indent << indent << "message(FATAL_ERROR \"ABSOLUTE path INSTALL "
       << "DESTINATION forbidden (by caller): "
       << "${CMAKE_ABSOLUTE_DESTINATION_FILES}\")\n";
    os << indent << "endif()\n";
  }
  std::string absDest = this->ConvertToAbsoluteDestination(dest);
  os << "file(INSTALL DESTINATION \"" << absDest << "\" TYPE " << stype;
  if (optional) {
    os << " OPTIONAL";
  }
  switch (this->Message) {
    case MessageDefault:
      break;
    case MessageAlways:
      os << " MESSAGE_ALWAYS";
      break;
    case MessageLazy:
      os << " MESSAGE_LAZY";
      break;
    case MessageNever:
      os << " MESSAGE_NEVER";
      break;
  }
  if (permissions_file && *permissions_file) {
    os << " PERMISSIONS" << permissions_file;
  }
  if (permissions_dir && *permissions_dir) {
    os << " DIR_PERMISSIONS" << permissions_dir;
  }
  if (rename && *rename) {
    os << " RENAME \"" << rename << "\"";
  }
  os << " FILES";
  if (files.size() == 1) {
    os << " \"" << files[0] << "\"";
  } else {
    for (std::vector<std::string>::const_iterator fi = files.begin();
         fi != files.end(); ++fi) {
      os << "\n" << indent << "  \"" << *fi << "\"";
    }
    os << "\n" << indent << " ";
    if (!(literal_args && *literal_args)) {
      os << " ";
    }
  }
  if (literal_args && *literal_args) {
    os << literal_args;
  }
  os << ")\n";
}

std::string cmInstallGenerator::CreateComponentTest(const char* component,
                                                    bool exclude_from_all)
{
  std::string result = "\"${CMAKE_INSTALL_COMPONENT}\" STREQUAL \"";
  result += component;
  result += "\"";
  if (!exclude_from_all) {
    result += " OR NOT CMAKE_INSTALL_COMPONENT";
  }
  return result;
}

void cmInstallGenerator::GenerateScript(std::ostream& os)
{
  // Track indentation.
  Indent indent;

  // Begin this block of installation.
  std::string component_test =
    this->CreateComponentTest(this->Component.c_str(), this->ExcludeFromAll);
  os << indent << "if(" << component_test << ")\n";

  // Generate the script possibly with per-configuration code.
  this->GenerateScriptConfigs(os, indent.Next());

  // End this block of installation.
  os << indent << "endif()\n\n";
}

bool cmInstallGenerator::InstallsForConfig(const std::string& config)
{
  return this->GeneratesForConfig(config);
}

std::string cmInstallGenerator::ConvertToAbsoluteDestination(
  std::string const& dest) const
{
  std::string result;
  if (!dest.empty() && !cmSystemTools::FileIsFullPath(dest.c_str())) {
    result = "${CMAKE_INSTALL_PREFIX}/";
  }
  result += dest;
  return result;
}

cmInstallGenerator::MessageLevel cmInstallGenerator::SelectMessageLevel(
  cmMakefile* mf, bool never)
{
  if (never) {
    return MessageNever;
  }
  std::string m = mf->GetSafeDefinition("CMAKE_INSTALL_MESSAGE");
  if (m == "ALWAYS") {
    return MessageAlways;
  }
  if (m == "LAZY") {
    return MessageLazy;
  }
  if (m == "NEVER") {
    return MessageNever;
  }
  return MessageDefault;
}
