/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackBundleGenerator.h"

#include <sstream>
#include <vector>

#include "cmCPackLog.h"
#include "cmSystemTools.h"

cmCPackBundleGenerator::cmCPackBundleGenerator()
{
}

cmCPackBundleGenerator::~cmCPackBundleGenerator()
{
}

int cmCPackBundleGenerator::InitializeInternal()
{
  const char* name = this->GetOption("CPACK_BUNDLE_NAME");
  if (0 == name) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "CPACK_BUNDLE_NAME must be set to use the Bundle generator."
                    << std::endl);

    return 0;
  }

  if (this->GetOption("CPACK_BUNDLE_APPLE_CERT_APP")) {
    const std::string codesign_path = cmSystemTools::FindProgram(
      "codesign", std::vector<std::string>(), false);

    if (codesign_path.empty()) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot locate codesign command"
                      << std::endl);
      return 0;
    }
    this->SetOptionIfNotSet("CPACK_COMMAND_CODESIGN", codesign_path.c_str());
  }

  return this->Superclass::InitializeInternal();
}

const char* cmCPackBundleGenerator::GetPackagingInstallPrefix()
{
  this->InstallPrefix = "/";
  this->InstallPrefix += this->GetOption("CPACK_BUNDLE_NAME");
  this->InstallPrefix += ".app/Contents/Resources";

  return this->InstallPrefix.c_str();
}

int cmCPackBundleGenerator::ConstructBundle()
{

  // Get required arguments ...
  const std::string cpack_bundle_name = this->GetOption("CPACK_BUNDLE_NAME")
    ? this->GetOption("CPACK_BUNDLE_NAME")
    : "";
  if (cpack_bundle_name.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_BUNDLE_NAME must be set."
                    << std::endl);

    return 0;
  }

  const std::string cpack_bundle_plist = this->GetOption("CPACK_BUNDLE_PLIST")
    ? this->GetOption("CPACK_BUNDLE_PLIST")
    : "";
  if (cpack_bundle_plist.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_BUNDLE_PLIST must be set."
                    << std::endl);

    return 0;
  }

  const std::string cpack_bundle_icon = this->GetOption("CPACK_BUNDLE_ICON")
    ? this->GetOption("CPACK_BUNDLE_ICON")
    : "";
  if (cpack_bundle_icon.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_BUNDLE_ICON must be set."
                    << std::endl);

    return 0;
  }

  // Get optional arguments ...
  const std::string cpack_bundle_startup_command =
    this->GetOption("CPACK_BUNDLE_STARTUP_COMMAND")
    ? this->GetOption("CPACK_BUNDLE_STARTUP_COMMAND")
    : "";

  // The staging directory contains everything that will end-up inside the
  // final disk image ...
  std::ostringstream staging;
  staging << toplevel;

  std::ostringstream contents;
  contents << staging.str() << "/" << cpack_bundle_name << ".app/"
           << "Contents";

  std::ostringstream application;
  application << contents.str() << "/"
              << "MacOS";

  std::ostringstream resources;
  resources << contents.str() << "/"
            << "Resources";

  // Install a required, user-provided bundle metadata file ...
  std::ostringstream plist_source;
  plist_source << cpack_bundle_plist;

  std::ostringstream plist_target;
  plist_target << contents.str() << "/"
               << "Info.plist";

  if (!this->CopyFile(plist_source, plist_target)) {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR,
      "Error copying plist.  Check the value of CPACK_BUNDLE_PLIST."
        << std::endl);

    return 0;
  }

  // Install a user-provided bundle icon ...
  std::ostringstream icon_source;
  icon_source << cpack_bundle_icon;

  std::ostringstream icon_target;
  icon_target << resources.str() << "/" << cpack_bundle_name << ".icns";

  if (!this->CopyFile(icon_source, icon_target)) {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR,
      "Error copying bundle icon.  Check the value of CPACK_BUNDLE_ICON."
        << std::endl);

    return 0;
  }

  // Optionally a user-provided startup command (could be an
  // executable or a script) ...
  if (!cpack_bundle_startup_command.empty()) {
    std::ostringstream command_source;
    command_source << cpack_bundle_startup_command;

    std::ostringstream command_target;
    command_target << application.str() << "/" << cpack_bundle_name;

    if (!this->CopyFile(command_source, command_target)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error copying startup command. "
                    " Check the value of CPACK_BUNDLE_STARTUP_COMMAND."
                      << std::endl);

      return 0;
    }

    cmSystemTools::SetPermissions(command_target.str().c_str(), 0777);
  }

  return 1;
}

int cmCPackBundleGenerator::PackageFiles()
{
  if (!this->ConstructBundle()) {
    return 0;
  }

  if (!this->SignBundle(toplevel)) {
    return 0;
  }

  return this->CreateDMG(toplevel, packageFileNames[0]);
}

bool cmCPackBundleGenerator::SupportsComponentInstallation() const
{
  return false;
}

int cmCPackBundleGenerator::SignBundle(const std::string& src_dir)
{
  const std::string cpack_apple_cert_app =
    this->GetOption("CPACK_BUNDLE_APPLE_CERT_APP")
    ? this->GetOption("CPACK_BUNDLE_APPLE_CERT_APP")
    : "";

  // codesign the application.
  if (!cpack_apple_cert_app.empty()) {
    std::string output;
    std::string bundle_path;
    bundle_path = src_dir + "/";
    bundle_path += this->GetOption("CPACK_BUNDLE_NAME");
    bundle_path += ".app";

    // A list of additional files to sign, ie. frameworks and plugins.
    const std::string sign_parameter =
      this->GetOption("CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER")
      ? this->GetOption("CPACK_BUNDLE_APPLE_CODESIGN_PARAMETER")
      : "--deep -f";

    const std::string sign_files =
      this->GetOption("CPACK_BUNDLE_APPLE_CODESIGN_FILES")
      ? this->GetOption("CPACK_BUNDLE_APPLE_CODESIGN_FILES")
      : "";

    std::vector<std::string> relFiles;
    cmSystemTools::ExpandListArgument(sign_files, relFiles);

    // sign the files supplied by the user, ie. frameworks.
    for (std::vector<std::string>::iterator it = relFiles.begin();
         it != relFiles.end(); ++it) {
      std::ostringstream temp_sign_file_cmd;
      temp_sign_file_cmd << this->GetOption("CPACK_COMMAND_CODESIGN");
      temp_sign_file_cmd << " " << sign_parameter << " -s \""
                         << cpack_apple_cert_app;
      temp_sign_file_cmd << "\" -i ";
      temp_sign_file_cmd << this->GetOption("CPACK_APPLE_BUNDLE_ID");
      temp_sign_file_cmd << " \"";
      temp_sign_file_cmd << bundle_path;
      temp_sign_file_cmd << *it << "\"";

      if (!this->RunCommand(temp_sign_file_cmd, &output)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Error signing file:" << bundle_path << *it << std::endl
                                            << output << std::endl);

        return 0;
      }
    }

    // sign main binary
    std::ostringstream temp_sign_binary_cmd;
    temp_sign_binary_cmd << this->GetOption("CPACK_COMMAND_CODESIGN");
    temp_sign_binary_cmd << " " << sign_parameter << " -s \""
                         << cpack_apple_cert_app;
    temp_sign_binary_cmd << "\" \"" << bundle_path << "\"";

    if (!this->RunCommand(temp_sign_binary_cmd, &output)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error signing the application binary." << std::endl
                                                            << output
                                                            << std::endl);

      return 0;
    }

    // sign app bundle
    std::ostringstream temp_codesign_cmd;
    temp_codesign_cmd << this->GetOption("CPACK_COMMAND_CODESIGN");
    temp_codesign_cmd << " " << sign_parameter << " -s \""
                      << cpack_apple_cert_app << "\"";
    if (this->GetOption("CPACK_BUNDLE_APPLE_ENTITLEMENTS")) {
      temp_codesign_cmd << " --entitlements ";
      temp_codesign_cmd << this->GetOption("CPACK_BUNDLE_APPLE_ENTITLEMENTS");
    }
    temp_codesign_cmd << " \"" << bundle_path << "\"";

    if (!this->RunCommand(temp_codesign_cmd, &output)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error signing the application package." << std::endl
                                                             << output
                                                             << std::endl);

      return 0;
    }

    cmCPackLogger(cmCPackLog::LOG_OUTPUT, "- Application has been codesigned"
                    << std::endl);
    cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                  (this->GetOption("CPACK_BUNDLE_APPLE_ENTITLEMENTS")
                     ? "with entitlement sandboxing"
                     : "without entitlement sandboxing")
                    << std::endl);
  }

  return 1;
}
