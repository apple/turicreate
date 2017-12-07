/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackProductBuildGenerator.h"

#include <map>
#include <sstream>
#include <stddef.h>

#include "cmCPackComponentGroup.h"
#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"

cmCPackProductBuildGenerator::cmCPackProductBuildGenerator()
{
  this->componentPackageMethod = ONE_PACKAGE;
}

cmCPackProductBuildGenerator::~cmCPackProductBuildGenerator()
{
}

int cmCPackProductBuildGenerator::PackageFiles()
{
  // TODO: Use toplevel
  //       It is used! Is this an obsolete comment?

  std::string packageDirFileName =
    this->GetOption("CPACK_TEMPORARY_DIRECTORY");

  // Create the directory where component packages will be built.
  std::string basePackageDir = packageDirFileName;
  basePackageDir += "/Contents/Packages";
  if (!cmsys::SystemTools::MakeDirectory(basePackageDir.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem creating component packages directory: "
                    << basePackageDir << std::endl);
    return 0;
  }

  if (!this->Components.empty()) {
    std::map<std::string, cmCPackComponent>::iterator compIt;
    for (compIt = this->Components.begin(); compIt != this->Components.end();
         ++compIt) {
      std::string packageDir = toplevel;
      packageDir += '/';
      packageDir += compIt->first;
      if (!this->GenerateComponentPackage(basePackageDir,
                                          GetPackageName(compIt->second),
                                          packageDir, &compIt->second)) {
        return 0;
      }
    }
  } else {
    if (!this->GenerateComponentPackage(basePackageDir,
                                        this->GetOption("CPACK_PACKAGE_NAME"),
                                        toplevel, NULL)) {
      return 0;
    }
  }

  std::string resDir = packageDirFileName + "/Contents";

  if (this->IsSet("CPACK_PRODUCTBUILD_RESOURCES_DIR")) {
    std::string userResDir =
      this->GetOption("CPACK_PRODUCTBUILD_RESOURCES_DIR");

    if (!cmSystemTools::CopyADirectory(userResDir, resDir)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem copying the resource files"
                      << std::endl);
      return 0;
    }
  }

  // Copy or create all of the resource files we need.
  if (!this->CopyCreateResourceFile("License", resDir) ||
      !this->CopyCreateResourceFile("ReadMe", resDir) ||
      !this->CopyCreateResourceFile("Welcome", resDir)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem copying the License, ReadMe and Welcome files"
                    << std::endl);
    return 0;
  }

  // combine package(s) into a distribution
  WriteDistributionFile(packageDirFileName.c_str());
  std::ostringstream pkgCmd;

  std::string version = this->GetOption("CPACK_PACKAGE_VERSION");
  std::string productbuild = this->GetOption("CPACK_COMMAND_PRODUCTBUILD");
  std::string identityName;
  if (const char* n = this->GetOption("CPACK_PRODUCTBUILD_IDENTITY_NAME")) {
    identityName = n;
  }
  std::string keychainPath;
  if (const char* p = this->GetOption("CPACK_PRODUCTBUILD_KEYCHAIN_PATH")) {
    keychainPath = p;
  }

  pkgCmd << productbuild << " --distribution \"" << packageDirFileName
         << "/Contents/distribution.dist\""
         << " --package-path \"" << packageDirFileName << "/Contents/Packages"
         << "\""
         << " --resources \"" << resDir << "\""
         << " --version \"" << version << "\""
         << (identityName.empty() ? "" : " --sign \"" + identityName + "\"")
         << (keychainPath.empty() ? ""
                                  : " --keychain \"" + keychainPath + "\"")
         << " \"" << packageFileNames[0] << "\"";

  // Run ProductBuild
  return RunProductBuild(pkgCmd.str());
}

int cmCPackProductBuildGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/Applications");

  std::vector<std::string> no_paths;
  std::string program =
    cmSystemTools::FindProgram("pkgbuild", no_paths, false);
  if (program.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find pkgbuild executable"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_COMMAND_PKGBUILD", program.c_str());

  program = cmSystemTools::FindProgram("productbuild", no_paths, false);
  if (program.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find productbuild executable"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_COMMAND_PRODUCTBUILD", program.c_str());

  return this->Superclass::InitializeInternal();
}

bool cmCPackProductBuildGenerator::RunProductBuild(const std::string& command)
{
  std::string tmpFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  tmpFile += "/ProductBuildOutput.log";

  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Execute: " << command << std::endl);
  std::string output, error_output;
  int retVal = 1;
  bool res =
    cmSystemTools::RunSingleCommand(command.c_str(), &output, &error_output,
                                    &retVal, 0, this->GeneratorVerbose, 0);
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Done running command" << std::endl);
  if (!res || retVal) {
    cmGeneratedFileStream ofs(tmpFile.c_str());
    ofs << "# Run command: " << command << std::endl
        << "# Output:" << std::endl
        << output << std::endl;
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem running command: " << command << std::endl
                                              << "Please check " << tmpFile
                                              << " for errors" << std::endl);
    return false;
  }
  return true;
}

bool cmCPackProductBuildGenerator::GenerateComponentPackage(
  const std::string& packageFileDir, const std::string& packageFileName,
  const std::string& packageDir, const cmCPackComponent* component)
{
  std::string packageFile = packageFileDir;
  packageFile += '/';
  packageFile += packageFileName;

  cmCPackLogger(cmCPackLog::LOG_OUTPUT, "-   Building component package: "
                  << packageFile << std::endl);

  const char* comp_name = component ? component->Name.c_str() : NULL;

  const char* preflight = this->GetComponentScript("PREFLIGHT", comp_name);
  const char* postflight = this->GetComponentScript("POSTFLIGHT", comp_name);

  std::string resDir = packageFileDir;
  if (component) {
    resDir += "/";
    resDir += component->Name;
  }
  std::string scriptDir = resDir + "/scripts";

  if (!cmsys::SystemTools::MakeDirectory(scriptDir.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem creating installer directory: " << scriptDir
                                                           << std::endl);
    return 0;
  }

  // if preflight, postflight, or postupgrade are set
  // then copy them into the script directory and make
  // them executable
  if (preflight) {
    this->CopyInstallScript(scriptDir, preflight, "preinstall");
  }
  if (postflight) {
    this->CopyInstallScript(scriptDir, postflight, "postinstall");
  }

  // The command that will be used to run ProductBuild
  std::ostringstream pkgCmd;

  std::string pkgId = "com.";
  pkgId += this->GetOption("CPACK_PACKAGE_VENDOR");
  pkgId += '.';
  pkgId += this->GetOption("CPACK_PACKAGE_NAME");
  if (component) {
    pkgId += '.';
    pkgId += component->Name;
  }

  std::string version = this->GetOption("CPACK_PACKAGE_VERSION");
  std::string pkgbuild = this->GetOption("CPACK_COMMAND_PKGBUILD");
  std::string identityName;
  if (const char* n = this->GetOption("CPACK_PKGBUILD_IDENTITY_NAME")) {
    identityName = n;
  }
  std::string keychainPath;
  if (const char* p = this->GetOption("CPACK_PKGBUILD_KEYCHAIN_PATH")) {
    keychainPath = p;
  }

  pkgCmd << pkgbuild << " --root \"" << packageDir << "\""
         << " --identifier \"" << pkgId << "\""
         << " --scripts \"" << scriptDir << "\""
         << " --version \"" << version << "\""
         << " --install-location \"/\""
         << (identityName.empty() ? "" : " --sign \"" + identityName + "\"")
         << (keychainPath.empty() ? ""
                                  : " --keychain \"" + keychainPath + "\"")
         << " \"" << packageFile << "\"";

  if (component && !component->Plist.empty()) {
    pkgCmd << " --component-plist \"" << component->Plist << "\"";
  }

  // Run ProductBuild
  return RunProductBuild(pkgCmd.str());
}

const char* cmCPackProductBuildGenerator::GetComponentScript(
  const char* script, const char* component_name)
{
  std::string scriptname = std::string("CPACK_") + script + "_";
  if (component_name) {
    scriptname += cmSystemTools::UpperCase(component_name);
    scriptname += "_";
  }
  scriptname += "SCRIPT";

  return this->GetOption(scriptname);
}
