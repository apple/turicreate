/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackGenerator.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <utility>

#include "cmCPackComponentGroup.h"
#include "cmCPackLog.h"
#include "cmCryptoHash.h"
#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmStateSnapshot.h"
#include "cmWorkingDirectory.h"
#include "cmXMLSafe.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

#if defined(__HAIKU__)
#include <FindDirectory.h>
#include <StorageDefs.h>
#endif

cmCPackGenerator::cmCPackGenerator()
{
  this->GeneratorVerbose = cmSystemTools::OUTPUT_NONE;
  this->MakefileMap = CM_NULLPTR;
  this->Logger = CM_NULLPTR;
  this->componentPackageMethod = ONE_PACKAGE_PER_GROUP;
}

cmCPackGenerator::~cmCPackGenerator()
{
  this->MakefileMap = CM_NULLPTR;
}

void cmCPackGeneratorProgress(const char* msg, float prog, void* ptr)
{
  cmCPackGenerator* self = static_cast<cmCPackGenerator*>(ptr);
  self->DisplayVerboseOutput(msg, prog);
}

void cmCPackGenerator::DisplayVerboseOutput(const char* msg, float progress)
{
  (void)progress;
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "" << msg << std::endl);
}

int cmCPackGenerator::PrepareNames()
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Create temp directory." << std::endl);

  // checks CPACK_SET_DESTDIR support
  if (IsOn("CPACK_SET_DESTDIR")) {
    if (SETDESTDIR_UNSUPPORTED == SupportsSetDestdir()) {
      cmCPackLogger(
        cmCPackLog::LOG_ERROR, "CPACK_SET_DESTDIR is set to ON but the '"
          << Name << "' generator does NOT support it." << std::endl);
      return 0;
    }
    if (SETDESTDIR_SHOULD_NOT_BE_USED == SupportsSetDestdir()) {
      cmCPackLogger(cmCPackLog::LOG_WARNING,
                    "CPACK_SET_DESTDIR is set to ON but it is "
                      << "usually a bad idea to do that with '" << Name
                      << "' generator. Use at your own risk." << std::endl);
    }
  }

  std::string tempDirectory = this->GetOption("CPACK_PACKAGE_DIRECTORY");
  tempDirectory += "/_CPack_Packages/";
  const char* toplevelTag = this->GetOption("CPACK_TOPLEVEL_TAG");
  if (toplevelTag) {
    tempDirectory += toplevelTag;
    tempDirectory += "/";
  }
  tempDirectory += this->GetOption("CPACK_GENERATOR");
  std::string topDirectory = tempDirectory;
  const char* pfname = this->GetOption("CPACK_PACKAGE_FILE_NAME");
  if (!pfname) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "CPACK_PACKAGE_FILE_NAME not specified" << std::endl);
    return 0;
  }
  std::string outName = pfname;
  tempDirectory += "/" + outName;
  if (!this->GetOutputExtension()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "No output extension specified"
                    << std::endl);
    return 0;
  }
  outName += this->GetOutputExtension();
  const char* pdir = this->GetOption("CPACK_PACKAGE_DIRECTORY");
  if (!pdir) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "CPACK_PACKAGE_DIRECTORY not specified" << std::endl);
    return 0;
  }

  std::string destFile = pdir;
  this->SetOptionIfNotSet("CPACK_OUTPUT_FILE_PREFIX", destFile.c_str());
  destFile += "/" + outName;
  std::string outFile = topDirectory + "/" + outName;
  this->SetOptionIfNotSet("CPACK_TOPLEVEL_DIRECTORY", topDirectory.c_str());
  this->SetOptionIfNotSet("CPACK_TEMPORARY_DIRECTORY", tempDirectory.c_str());
  this->SetOptionIfNotSet("CPACK_OUTPUT_FILE_NAME", outName.c_str());
  this->SetOptionIfNotSet("CPACK_OUTPUT_FILE_PATH", destFile.c_str());
  this->SetOptionIfNotSet("CPACK_TEMPORARY_PACKAGE_FILE_NAME",
                          outFile.c_str());
  this->SetOptionIfNotSet("CPACK_INSTALL_DIRECTORY", this->GetInstallPath());
  this->SetOptionIfNotSet(
    "CPACK_NATIVE_INSTALL_DIRECTORY",
    cmsys::SystemTools::ConvertToOutputPath(this->GetInstallPath()).c_str());
  this->SetOptionIfNotSet("CPACK_TEMPORARY_INSTALL_DIRECTORY",
                          tempDirectory.c_str());

  cmCPackLogger(cmCPackLog::LOG_DEBUG,
                "Look for: CPACK_PACKAGE_DESCRIPTION_FILE" << std::endl);
  const char* descFileName = this->GetOption("CPACK_PACKAGE_DESCRIPTION_FILE");
  if (descFileName) {
    cmCPackLogger(cmCPackLog::LOG_DEBUG, "Look for: " << descFileName
                                                      << std::endl);
    if (!cmSystemTools::FileExists(descFileName)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Cannot find description file name: ["
                      << descFileName << "]" << std::endl);
      return 0;
    }
    cmsys::ifstream ifs(descFileName);
    if (!ifs) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Cannot open description file name: " << descFileName
                                                          << std::endl);
      return 0;
    }
    std::ostringstream ostr;
    std::string line;

    cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                  "Read description file: " << descFileName << std::endl);
    while (ifs && cmSystemTools::GetLineFromStream(ifs, line)) {
      ostr << cmXMLSafe(line) << std::endl;
    }
    this->SetOptionIfNotSet("CPACK_PACKAGE_DESCRIPTION", ostr.str().c_str());
  }
  if (!this->GetOption("CPACK_PACKAGE_DESCRIPTION")) {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR,
      "Project description not specified. Please specify "
      "CPACK_PACKAGE_DESCRIPTION or CPACK_PACKAGE_DESCRIPTION_FILE."
        << std::endl);
    return 0;
  }
  const char* algoSignature = this->GetOption("CPACK_PACKAGE_CHECKSUM");
  if (algoSignature) {
    if (cmCryptoHash::New(algoSignature).get() == CM_NULLPTR) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot recognize algorithm: "
                      << algoSignature << std::endl);
      return 0;
    }
  }

  this->SetOptionIfNotSet("CPACK_REMOVE_TOPLEVEL_DIRECTORY", "1");

  return 1;
}

int cmCPackGenerator::InstallProject()
{
  cmCPackLogger(cmCPackLog::LOG_OUTPUT, "Install projects" << std::endl);
  this->CleanTemporaryDirectory();

  std::string bareTempInstallDirectory =
    this->GetOption("CPACK_TEMPORARY_INSTALL_DIRECTORY");
  std::string tempInstallDirectoryStr = bareTempInstallDirectory;
  bool setDestDir = cmSystemTools::IsOn(this->GetOption("CPACK_SET_DESTDIR")) |
    cmSystemTools::IsInternallyOn(this->GetOption("CPACK_SET_DESTDIR"));
  if (!setDestDir) {
    tempInstallDirectoryStr += this->GetPackagingInstallPrefix();
  }

  const char* tempInstallDirectory = tempInstallDirectoryStr.c_str();
  int res = 1;
  if (!cmsys::SystemTools::MakeDirectory(bareTempInstallDirectory.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem creating temporary directory: "
                    << (tempInstallDirectory ? tempInstallDirectory : "(NULL)")
                    << std::endl);
    return 0;
  }

  if (setDestDir) {
    std::string destDir = "DESTDIR=";
    destDir += tempInstallDirectory;
    cmSystemTools::PutEnv(destDir);
  } else {
    // Make sure there is no destdir
    cmSystemTools::PutEnv("DESTDIR=");
  }

  // If the CPackConfig file sets CPACK_INSTALL_COMMANDS then run them
  // as listed
  if (!this->InstallProjectViaInstallCommands(setDestDir,
                                              tempInstallDirectory)) {
    return 0;
  }

  // If the CPackConfig file sets CPACK_INSTALL_SCRIPT then run them
  // as listed
  if (!this->InstallProjectViaInstallScript(setDestDir,
                                            tempInstallDirectory)) {
    return 0;
  }

  // If the CPackConfig file sets CPACK_INSTALLED_DIRECTORIES
  // then glob it and copy it to CPACK_TEMPORARY_DIRECTORY
  // This is used in Source packaging
  if (!this->InstallProjectViaInstalledDirectories(setDestDir,
                                                   tempInstallDirectory)) {
    return 0;
  }

  // If the project is a CMAKE project then run pre-install
  // and then read the cmake_install script to run it
  if (!this->InstallProjectViaInstallCMakeProjects(setDestDir,
                                                   bareTempInstallDirectory)) {
    return 0;
  }

  if (setDestDir) {
    cmSystemTools::PutEnv("DESTDIR=");
  }

  return res;
}

int cmCPackGenerator::InstallProjectViaInstallCommands(
  bool setDestDir, const std::string& tempInstallDirectory)
{
  (void)setDestDir;
  const char* installCommands = this->GetOption("CPACK_INSTALL_COMMANDS");
  if (installCommands && *installCommands) {
    std::string tempInstallDirectoryEnv = "CMAKE_INSTALL_PREFIX=";
    tempInstallDirectoryEnv += tempInstallDirectory;
    cmSystemTools::PutEnv(tempInstallDirectoryEnv);
    std::vector<std::string> installCommandsVector;
    cmSystemTools::ExpandListArgument(installCommands, installCommandsVector);
    std::vector<std::string>::iterator it;
    for (it = installCommandsVector.begin(); it != installCommandsVector.end();
         ++it) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Execute: " << *it << std::endl);
      std::string output;
      int retVal = 1;
      bool resB =
        cmSystemTools::RunSingleCommand(it->c_str(), &output, &output, &retVal,
                                        CM_NULLPTR, this->GeneratorVerbose, 0);
      if (!resB || retVal) {
        std::string tmpFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
        tmpFile += "/InstallOutput.log";
        cmGeneratedFileStream ofs(tmpFile.c_str());
        ofs << "# Run command: " << *it << std::endl
            << "# Output:" << std::endl
            << output << std::endl;
        cmCPackLogger(
          cmCPackLog::LOG_ERROR, "Problem running install command: "
            << *it << std::endl
            << "Please check " << tmpFile << " for errors" << std::endl);
        return 0;
      }
    }
  }
  return 1;
}

int cmCPackGenerator::InstallProjectViaInstalledDirectories(
  bool setDestDir, const std::string& tempInstallDirectory)
{
  (void)setDestDir;
  (void)tempInstallDirectory;
  std::vector<cmsys::RegularExpression> ignoreFilesRegex;
  const char* cpackIgnoreFiles = this->GetOption("CPACK_IGNORE_FILES");
  if (cpackIgnoreFiles) {
    std::vector<std::string> ignoreFilesRegexString;
    cmSystemTools::ExpandListArgument(cpackIgnoreFiles,
                                      ignoreFilesRegexString);
    std::vector<std::string>::iterator it;
    for (it = ignoreFilesRegexString.begin();
         it != ignoreFilesRegexString.end(); ++it) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                    "Create ignore files regex for: " << *it << std::endl);
      ignoreFilesRegex.push_back(it->c_str());
    }
  }
  const char* installDirectories =
    this->GetOption("CPACK_INSTALLED_DIRECTORIES");
  if (installDirectories && *installDirectories) {
    std::vector<std::string> installDirectoriesVector;
    cmSystemTools::ExpandListArgument(installDirectories,
                                      installDirectoriesVector);
    if (installDirectoriesVector.size() % 2 != 0) {
      cmCPackLogger(
        cmCPackLog::LOG_ERROR,
        "CPACK_INSTALLED_DIRECTORIES should contain pairs of <directory> and "
        "<subdirectory>. The <subdirectory> can be '.' to be installed in "
        "the toplevel directory of installation."
          << std::endl);
      return 0;
    }
    std::vector<std::string>::iterator it;
    const std::string& tempDir = tempInstallDirectory;
    for (it = installDirectoriesVector.begin();
         it != installDirectoriesVector.end(); ++it) {
      std::vector<std::pair<std::string, std::string> > symlinkedFiles;
      cmCPackLogger(cmCPackLog::LOG_DEBUG, "Find files" << std::endl);
      cmsys::Glob gl;
      std::string top = *it;
      it++;
      std::string subdir = *it;
      std::string findExpr = top;
      findExpr += "/*";
      cmCPackLogger(cmCPackLog::LOG_OUTPUT,
                    "- Install directory: " << top << std::endl);
      gl.RecurseOn();
      gl.SetRecurseListDirs(true);
      if (!gl.FindFiles(findExpr)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Cannot find any files in the installed directory"
                        << std::endl);
        return 0;
      }
      files = gl.GetFiles();
      std::vector<std::string>::iterator gfit;
      std::vector<cmsys::RegularExpression>::iterator regIt;
      for (gfit = files.begin(); gfit != files.end(); ++gfit) {
        bool skip = false;
        std::string inFile = *gfit;
        if (cmSystemTools::FileIsDirectory(*gfit)) {
          inFile += '/';
        }
        for (regIt = ignoreFilesRegex.begin(); regIt != ignoreFilesRegex.end();
             ++regIt) {
          if (regIt->find(inFile.c_str())) {
            cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                          "Ignore file: " << inFile << std::endl);
            skip = true;
          }
        }
        if (skip) {
          continue;
        }
        std::string filePath = tempDir;
        filePath += "/" + subdir + "/" +
          cmSystemTools::RelativePath(top.c_str(), gfit->c_str());
        cmCPackLogger(cmCPackLog::LOG_DEBUG, "Copy file: "
                        << inFile << " -> " << filePath << std::endl);
        /* If the file is a symlink we will have to re-create it */
        if (cmSystemTools::FileIsSymlink(inFile)) {
          std::string targetFile;
          std::string inFileRelative =
            cmSystemTools::RelativePath(top.c_str(), inFile.c_str());
          cmSystemTools::ReadSymlink(inFile, targetFile);
          symlinkedFiles.push_back(
            std::pair<std::string, std::string>(targetFile, inFileRelative));
        }
        /* If it is not a symlink then do a plain copy */
        else if (!(cmSystemTools::CopyFileIfDifferent(inFile.c_str(),
                                                      filePath.c_str()) &&
                   cmSystemTools::CopyFileTime(inFile.c_str(),
                                               filePath.c_str()))) {
          cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem copying file: "
                          << inFile << " -> " << filePath << std::endl);
          return 0;
        }
      }
      /* rebuild symlinks in the installed tree */
      if (!symlinkedFiles.empty()) {
        std::vector<std::pair<std::string, std::string> >::iterator
          symlinkedIt;
        std::string curDir = cmSystemTools::GetCurrentWorkingDirectory();
        std::string goToDir = tempDir;
        goToDir += "/" + subdir;
        cmCPackLogger(cmCPackLog::LOG_DEBUG, "Change dir to: " << goToDir
                                                               << std::endl);
        cmWorkingDirectory workdir(goToDir);
        for (symlinkedIt = symlinkedFiles.begin();
             symlinkedIt != symlinkedFiles.end(); ++symlinkedIt) {
          cmCPackLogger(cmCPackLog::LOG_DEBUG, "Will create a symlink: "
                          << symlinkedIt->second << "--> "
                          << symlinkedIt->first << std::endl);
          // make sure directory exists for symlink
          std::string destDir =
            cmSystemTools::GetFilenamePath(symlinkedIt->second);
          if (!destDir.empty() && !cmSystemTools::MakeDirectory(destDir)) {
            cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot create dir: "
                            << destDir << "\nTrying to create symlink: "
                            << symlinkedIt->second << "--> "
                            << symlinkedIt->first << std::endl);
          }
          if (!cmSystemTools::CreateSymlink(symlinkedIt->first,
                                            symlinkedIt->second)) {
            cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot create symlink: "
                            << symlinkedIt->second << "--> "
                            << symlinkedIt->first << std::endl);
            return 0;
          }
        }
        cmCPackLogger(cmCPackLog::LOG_DEBUG, "Going back to: " << curDir
                                                               << std::endl);
      }
    }
  }
  return 1;
}

int cmCPackGenerator::InstallProjectViaInstallScript(
  bool setDestDir, const std::string& tempInstallDirectory)
{
  const char* cmakeScripts = this->GetOption("CPACK_INSTALL_SCRIPT");
  if (cmakeScripts && *cmakeScripts) {
    cmCPackLogger(cmCPackLog::LOG_OUTPUT, "- Install scripts: " << cmakeScripts
                                                                << std::endl);
    std::vector<std::string> cmakeScriptsVector;
    cmSystemTools::ExpandListArgument(cmakeScripts, cmakeScriptsVector);
    std::vector<std::string>::iterator it;
    for (it = cmakeScriptsVector.begin(); it != cmakeScriptsVector.end();
         ++it) {
      std::string installScript = *it;

      cmCPackLogger(cmCPackLog::LOG_OUTPUT,
                    "- Install script: " << installScript << std::endl);

      if (setDestDir) {
        // For DESTDIR based packaging, use the *project* CMAKE_INSTALL_PREFIX
        // underneath the tempInstallDirectory. The value of the project's
        // CMAKE_INSTALL_PREFIX is sent in here as the value of the
        // CPACK_INSTALL_PREFIX variable.

        std::string dir;
        if (this->GetOption("CPACK_INSTALL_PREFIX")) {
          dir += this->GetOption("CPACK_INSTALL_PREFIX");
        }
        this->SetOption("CMAKE_INSTALL_PREFIX", dir.c_str());
        cmCPackLogger(
          cmCPackLog::LOG_DEBUG,
          "- Using DESTDIR + CPACK_INSTALL_PREFIX... (this->SetOption)"
            << std::endl);
        cmCPackLogger(cmCPackLog::LOG_DEBUG,
                      "- Setting CMAKE_INSTALL_PREFIX to '" << dir << "'"
                                                            << std::endl);
      } else {
        this->SetOption("CMAKE_INSTALL_PREFIX", tempInstallDirectory.c_str());

        cmCPackLogger(cmCPackLog::LOG_DEBUG,
                      "- Using non-DESTDIR install... (this->SetOption)"
                        << std::endl);
        cmCPackLogger(cmCPackLog::LOG_DEBUG,
                      "- Setting CMAKE_INSTALL_PREFIX to '"
                        << tempInstallDirectory << "'" << std::endl);
      }

      this->SetOptionIfNotSet("CMAKE_CURRENT_BINARY_DIR",
                              tempInstallDirectory.c_str());
      this->SetOptionIfNotSet("CMAKE_CURRENT_SOURCE_DIR",
                              tempInstallDirectory.c_str());
      int res = this->MakefileMap->ReadListFile(installScript.c_str());
      if (cmSystemTools::GetErrorOccuredFlag() || !res) {
        return 0;
      }
    }
  }
  return 1;
}

int cmCPackGenerator::InstallProjectViaInstallCMakeProjects(
  bool setDestDir, const std::string& baseTempInstallDirectory)
{
  const char* cmakeProjects = this->GetOption("CPACK_INSTALL_CMAKE_PROJECTS");
  const char* cmakeGenerator = this->GetOption("CPACK_CMAKE_GENERATOR");
  std::string absoluteDestFiles;
  if (cmakeProjects && *cmakeProjects) {
    if (!cmakeGenerator) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "CPACK_INSTALL_CMAKE_PROJECTS is specified, but "
                    "CPACK_CMAKE_GENERATOR is not. CPACK_CMAKE_GENERATOR "
                    "is required to install the project."
                      << std::endl);
      return 0;
    }
    std::vector<std::string> cmakeProjectsVector;
    cmSystemTools::ExpandListArgument(cmakeProjects, cmakeProjectsVector);
    std::vector<std::string>::iterator it;
    for (it = cmakeProjectsVector.begin(); it != cmakeProjectsVector.end();
         ++it) {
      if (it + 1 == cmakeProjectsVector.end() ||
          it + 2 == cmakeProjectsVector.end() ||
          it + 3 == cmakeProjectsVector.end()) {
        cmCPackLogger(
          cmCPackLog::LOG_ERROR,
          "Not enough items on list: CPACK_INSTALL_CMAKE_PROJECTS. "
          "CPACK_INSTALL_CMAKE_PROJECTS should hold quadruplet of install "
          "directory, install project name, install component, and install "
          "subdirectory."
            << std::endl);
        return 0;
      }
      std::string installDirectory = *it;
      ++it;
      std::string installProjectName = *it;
      ++it;
      std::string installComponent = *it;
      ++it;
      std::string installSubDirectory = *it;
      std::string installFile = installDirectory + "/cmake_install.cmake";

      std::vector<std::string> componentsVector;

      bool componentInstall = false;
      /*
       * We do a component install iff
       *    - the CPack generator support component
       *    - the user did not request Monolithic install
       *      (this works at CPack time too)
       */
      if (this->SupportsComponentInstallation() &
          !(this->IsOn("CPACK_MONOLITHIC_INSTALL"))) {
        // Determine the installation types for this project (if provided).
        std::string installTypesVar = "CPACK_" +
          cmSystemTools::UpperCase(installComponent) + "_INSTALL_TYPES";
        const char* installTypes = this->GetOption(installTypesVar);
        if (installTypes && *installTypes) {
          std::vector<std::string> installTypesVector;
          cmSystemTools::ExpandListArgument(installTypes, installTypesVector);
          std::vector<std::string>::iterator installTypeIt;
          for (installTypeIt = installTypesVector.begin();
               installTypeIt != installTypesVector.end(); ++installTypeIt) {
            this->GetInstallationType(installProjectName, *installTypeIt);
          }
        }

        // Determine the set of components that will be used in this project
        std::string componentsVar =
          "CPACK_COMPONENTS_" + cmSystemTools::UpperCase(installComponent);
        const char* components = this->GetOption(componentsVar);
        if (components && *components) {
          cmSystemTools::ExpandListArgument(components, componentsVector);
          std::vector<std::string>::iterator compIt;
          for (compIt = componentsVector.begin();
               compIt != componentsVector.end(); ++compIt) {
            GetComponent(installProjectName, *compIt);
          }
          componentInstall = true;
        }
      }
      if (componentsVector.empty()) {
        componentsVector.push_back(installComponent);
      }

      const char* buildConfigCstr = this->GetOption("CPACK_BUILD_CONFIG");
      std::string buildConfig = buildConfigCstr ? buildConfigCstr : "";
      cmGlobalGenerator* globalGenerator =
        this->MakefileMap->GetCMakeInstance()->CreateGlobalGenerator(
          cmakeGenerator);
      if (!globalGenerator) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Specified package generator not found. "
                      "CPACK_CMAKE_GENERATOR value is invalid."
                        << std::endl);
        return 0;
      }
      // set the global flag for unix style paths on cmSystemTools as
      // soon as the generator is set.  This allows gmake to be used
      // on windows.
      cmSystemTools::SetForceUnixPaths(globalGenerator->GetForceUnixPaths());

      // Does this generator require pre-install?
      if (const char* preinstall =
            globalGenerator->GetPreinstallTargetName()) {
        std::string buildCommand = globalGenerator->GenerateCMakeBuildCommand(
          preinstall, buildConfig, "", false);
        cmCPackLogger(cmCPackLog::LOG_DEBUG,
                      "- Install command: " << buildCommand << std::endl);
        cmCPackLogger(cmCPackLog::LOG_OUTPUT, "- Run preinstall target for: "
                        << installProjectName << std::endl);
        std::string output;
        int retVal = 1;
        bool resB = cmSystemTools::RunSingleCommand(
          buildCommand.c_str(), &output, &output, &retVal,
          installDirectory.c_str(), this->GeneratorVerbose, 0);
        if (!resB || retVal) {
          std::string tmpFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
          tmpFile += "/PreinstallOutput.log";
          cmGeneratedFileStream ofs(tmpFile.c_str());
          ofs << "# Run command: " << buildCommand << std::endl
              << "# Directory: " << installDirectory << std::endl
              << "# Output:" << std::endl
              << output << std::endl;
          cmCPackLogger(
            cmCPackLog::LOG_ERROR, "Problem running install command: "
              << buildCommand << std::endl
              << "Please check " << tmpFile << " for errors" << std::endl);
          return 0;
        }
      }
      delete globalGenerator;

      cmCPackLogger(cmCPackLog::LOG_OUTPUT,
                    "- Install project: " << installProjectName << std::endl);

      // Run the installation for each component
      std::vector<std::string>::iterator componentIt;
      for (componentIt = componentsVector.begin();
           componentIt != componentsVector.end(); ++componentIt) {
        std::string tempInstallDirectory = baseTempInstallDirectory;
        installComponent = *componentIt;
        if (componentInstall) {
          cmCPackLogger(cmCPackLog::LOG_OUTPUT, "-   Install component: "
                          << installComponent << std::endl);
        }

        cmake cm(cmake::RoleScript);
        cm.SetHomeDirectory("");
        cm.SetHomeOutputDirectory("");
        cm.GetCurrentSnapshot().SetDefaultDefinitions();
        cm.AddCMakePaths();
        cm.SetProgressCallback(cmCPackGeneratorProgress, this);
        cmGlobalGenerator gg(&cm);
        CM_AUTO_PTR<cmMakefile> mf(
          new cmMakefile(&gg, cm.GetCurrentSnapshot()));
        if (!installSubDirectory.empty() && installSubDirectory != "/" &&
            installSubDirectory != ".") {
          tempInstallDirectory += installSubDirectory;
        }
        if (componentInstall) {
          tempInstallDirectory += "/";
          // Some CPack generators would rather chose
          // the local installation directory suffix.
          // Some (e.g. RPM) use
          //  one install directory for each component **GROUP**
          // instead of the default
          //  one install directory for each component.
          tempInstallDirectory +=
            GetComponentInstallDirNameSuffix(installComponent);
          if (this->IsOn("CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY")) {
            tempInstallDirectory += "/";
            tempInstallDirectory += this->GetOption("CPACK_PACKAGE_FILE_NAME");
          }
        }

        if (!setDestDir) {
          tempInstallDirectory += this->GetPackagingInstallPrefix();
        }

        if (setDestDir) {
          // For DESTDIR based packaging, use the *project*
          // CMAKE_INSTALL_PREFIX underneath the tempInstallDirectory. The
          // value of the project's CMAKE_INSTALL_PREFIX is sent in here as
          // the value of the CPACK_INSTALL_PREFIX variable.
          //
          // If DESTDIR has been 'internally set ON' this means that
          // the underlying CPack specific generator did ask for that
          // In this case we may override CPACK_INSTALL_PREFIX with
          // CPACK_PACKAGING_INSTALL_PREFIX
          // I know this is tricky and awkward but it's the price for
          // CPACK_SET_DESTDIR backward compatibility.
          if (cmSystemTools::IsInternallyOn(
                this->GetOption("CPACK_SET_DESTDIR"))) {
            this->SetOption("CPACK_INSTALL_PREFIX",
                            this->GetOption("CPACK_PACKAGING_INSTALL_PREFIX"));
          }
          std::string dir;
          if (this->GetOption("CPACK_INSTALL_PREFIX")) {
            dir += this->GetOption("CPACK_INSTALL_PREFIX");
          }
          mf->AddDefinition("CMAKE_INSTALL_PREFIX", dir.c_str());

          cmCPackLogger(
            cmCPackLog::LOG_DEBUG,
            "- Using DESTDIR + CPACK_INSTALL_PREFIX... (mf->AddDefinition)"
              << std::endl);
          cmCPackLogger(cmCPackLog::LOG_DEBUG,
                        "- Setting CMAKE_INSTALL_PREFIX to '" << dir << "'"
                                                              << std::endl);

          // Make sure that DESTDIR + CPACK_INSTALL_PREFIX directory
          // exists:
          //
          if (cmSystemTools::StringStartsWith(dir.c_str(), "/")) {
            dir = tempInstallDirectory + dir;
          } else {
            dir = tempInstallDirectory + "/" + dir;
          }
          /*
           *  We must re-set DESTDIR for each component
           *  We must not add the CPACK_INSTALL_PREFIX part because
           *  it will be added using the override of CMAKE_INSTALL_PREFIX
           *  The main reason for this awkward trick is that
           *  are using DESTDIR for 2 different reasons:
           *     - Because it was asked by the CPack Generator or the user
           *       using CPACK_SET_DESTDIR
           *     - Because it was already used for component install
           *       in order to put things in subdirs...
           */
          cmSystemTools::PutEnv(std::string("DESTDIR=") +
                                tempInstallDirectory);
          cmCPackLogger(cmCPackLog::LOG_DEBUG, "- Creating directory: '"
                          << dir << "'" << std::endl);

          if (!cmsys::SystemTools::MakeDirectory(dir.c_str())) {
            cmCPackLogger(
              cmCPackLog::LOG_ERROR,
              "Problem creating temporary directory: " << dir << std::endl);
            return 0;
          }
        } else {
          mf->AddDefinition("CMAKE_INSTALL_PREFIX",
                            tempInstallDirectory.c_str());

          if (!cmsys::SystemTools::MakeDirectory(
                tempInstallDirectory.c_str())) {
            cmCPackLogger(cmCPackLog::LOG_ERROR,
                          "Problem creating temporary directory: "
                            << tempInstallDirectory << std::endl);
            return 0;
          }

          cmCPackLogger(cmCPackLog::LOG_DEBUG,
                        "- Using non-DESTDIR install... (mf->AddDefinition)"
                          << std::endl);
          cmCPackLogger(cmCPackLog::LOG_DEBUG,
                        "- Setting CMAKE_INSTALL_PREFIX to '"
                          << tempInstallDirectory << "'" << std::endl);
        }

        if (!buildConfig.empty()) {
          mf->AddDefinition("BUILD_TYPE", buildConfig.c_str());
        }
        std::string installComponentLowerCase =
          cmSystemTools::LowerCase(installComponent);
        if (installComponentLowerCase != "all") {
          mf->AddDefinition("CMAKE_INSTALL_COMPONENT",
                            installComponent.c_str());
        }

        // strip on TRUE, ON, 1, one or several file names, but not on
        // FALSE, OFF, 0 and an empty string
        if (!cmSystemTools::IsOff(this->GetOption("CPACK_STRIP_FILES"))) {
          mf->AddDefinition("CMAKE_INSTALL_DO_STRIP", "1");
        }
        // Remember the list of files before installation
        // of the current component (if we are in component install)
        const char* InstallPrefix = tempInstallDirectory.c_str();
        std::vector<std::string> filesBefore;
        std::string findExpr(InstallPrefix);
        if (componentInstall) {
          cmsys::Glob glB;
          findExpr += "/*";
          glB.RecurseOn();
          glB.SetRecurseListDirs(true);
          glB.FindFiles(findExpr);
          filesBefore = glB.GetFiles();
          std::sort(filesBefore.begin(), filesBefore.end());
        }

        // If CPack was asked to warn on ABSOLUTE INSTALL DESTINATION
        // then forward request to cmake_install.cmake script
        if (this->IsOn("CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION")) {
          mf->AddDefinition("CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION", "1");
        }
        // If current CPack generator does support
        // ABSOLUTE INSTALL DESTINATION or CPack has been asked for
        // then ask cmake_install.cmake script to error out
        // as soon as it occurs (before installing file)
        if (!SupportsAbsoluteDestination() ||
            this->IsOn("CPACK_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION")) {
          mf->AddDefinition("CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION",
                            "1");
        }
        // do installation
        int res = mf->ReadListFile(installFile.c_str());
        // forward definition of CMAKE_ABSOLUTE_DESTINATION_FILES
        // to CPack (may be used by generators like CPack RPM or DEB)
        // in order to transparently handle ABSOLUTE PATH
        if (mf->GetDefinition("CMAKE_ABSOLUTE_DESTINATION_FILES")) {
          mf->AddDefinition(
            "CPACK_ABSOLUTE_DESTINATION_FILES",
            mf->GetDefinition("CMAKE_ABSOLUTE_DESTINATION_FILES"));
        }

        // Now rebuild the list of files after installation
        // of the current component (if we are in component install)
        if (componentInstall) {
          cmsys::Glob glA;
          glA.RecurseOn();
          glA.SetRecurseListDirs(true);
          glA.SetRecurseThroughSymlinks(false);
          glA.FindFiles(findExpr);
          std::vector<std::string> filesAfter = glA.GetFiles();
          std::sort(filesAfter.begin(), filesAfter.end());
          std::vector<std::string>::iterator diff;
          std::vector<std::string> result(filesAfter.size());
          diff = std::set_difference(filesAfter.begin(), filesAfter.end(),
                                     filesBefore.begin(), filesBefore.end(),
                                     result.begin());

          std::vector<std::string>::iterator fit;
          std::string localFileName;
          // Populate the File field of each component
          for (fit = result.begin(); fit != diff; ++fit) {
            localFileName =
              cmSystemTools::RelativePath(InstallPrefix, fit->c_str());
            localFileName =
              localFileName.substr(localFileName.find_first_not_of('/'));
            Components[installComponent].Files.push_back(localFileName);
            cmCPackLogger(cmCPackLog::LOG_DEBUG, "Adding file <"
                            << localFileName << "> to component <"
                            << installComponent << ">" << std::endl);
          }
        }

        if (CM_NULLPTR !=
            mf->GetDefinition("CPACK_ABSOLUTE_DESTINATION_FILES")) {
          if (!absoluteDestFiles.empty()) {
            absoluteDestFiles += ";";
          }
          absoluteDestFiles +=
            mf->GetDefinition("CPACK_ABSOLUTE_DESTINATION_FILES");
          cmCPackLogger(cmCPackLog::LOG_DEBUG,
                        "Got some ABSOLUTE DESTINATION FILES: "
                          << absoluteDestFiles << std::endl);
          // define component specific var
          if (componentInstall) {
            std::string absoluteDestFileComponent =
              std::string("CPACK_ABSOLUTE_DESTINATION_FILES") + "_" +
              GetComponentInstallDirNameSuffix(installComponent);
            if (CM_NULLPTR != this->GetOption(absoluteDestFileComponent)) {
              std::string absoluteDestFilesListComponent =
                this->GetOption(absoluteDestFileComponent);
              absoluteDestFilesListComponent += ";";
              absoluteDestFilesListComponent +=
                mf->GetDefinition("CPACK_ABSOLUTE_DESTINATION_FILES");
              this->SetOption(absoluteDestFileComponent,
                              absoluteDestFilesListComponent.c_str());
            } else {
              this->SetOption(
                absoluteDestFileComponent,
                mf->GetDefinition("CPACK_ABSOLUTE_DESTINATION_FILES"));
            }
          }
        }
        if (cmSystemTools::GetErrorOccuredFlag() || !res) {
          return 0;
        }
      }
    }
  }
  this->SetOption("CPACK_ABSOLUTE_DESTINATION_FILES",
                  absoluteDestFiles.c_str());
  return 1;
}

bool cmCPackGenerator::ReadListFile(const char* moduleName)
{
  bool retval;
  std::string fullPath = this->MakefileMap->GetModulesFile(moduleName);
  retval = this->MakefileMap->ReadListFile(fullPath.c_str());
  // include FATAL_ERROR and ERROR in the return status
  retval = retval && (!cmSystemTools::GetErrorOccuredFlag());
  return retval;
}

void cmCPackGenerator::SetOptionIfNotSet(const std::string& op,
                                         const char* value)
{
  const char* def = this->MakefileMap->GetDefinition(op);
  if (def && *def) {
    return;
  }
  this->SetOption(op, value);
}

void cmCPackGenerator::SetOption(const std::string& op, const char* value)
{
  if (!value) {
    this->MakefileMap->RemoveDefinition(op);
    return;
  }
  cmCPackLogger(cmCPackLog::LOG_DEBUG, this->GetNameOfClass()
                  << "::SetOption(" << op << ", " << value << ")"
                  << std::endl);
  this->MakefileMap->AddDefinition(op, value);
}

int cmCPackGenerator::DoPackage()
{
  cmCPackLogger(cmCPackLog::LOG_OUTPUT, "Create package using " << this->Name
                                                                << std::endl);

  // Prepare CPack internal name and check
  // values for many CPACK_xxx vars
  if (!this->PrepareNames()) {
    return 0;
  }

  // Digest Component grouping specification
  if (!this->PrepareGroupingKind()) {
    return 0;
  }

  if (cmSystemTools::IsOn(
        this->GetOption("CPACK_REMOVE_TOPLEVEL_DIRECTORY"))) {
    const char* toplevelDirectory =
      this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
    if (cmSystemTools::FileExists(toplevelDirectory)) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Remove toplevel directory: "
                      << toplevelDirectory << std::endl);
      if (!cmSystemTools::RepeatedRemoveDirectory(toplevelDirectory)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Problem removing toplevel directory: "
                        << toplevelDirectory << std::endl);
        return 0;
      }
    }
  }
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "About to install project "
                  << std::endl);

  if (!this->InstallProject()) {
    return 0;
  }
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Done install project " << std::endl);

  const char* tempPackageFileName =
    this->GetOption("CPACK_TEMPORARY_PACKAGE_FILE_NAME");
  const char* tempDirectory = this->GetOption("CPACK_TEMPORARY_DIRECTORY");

  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Find files" << std::endl);
  cmsys::Glob gl;
  std::string findExpr = tempDirectory;
  findExpr += "/*";
  gl.RecurseOn();
  gl.SetRecurseListDirs(true);
  gl.SetRecurseThroughSymlinks(false);
  if (!gl.FindFiles(findExpr)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot find any files in the packaging tree" << std::endl);
    return 0;
  }

  cmCPackLogger(cmCPackLog::LOG_OUTPUT, "Create package" << std::endl);
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Package files to: "
                  << (tempPackageFileName ? tempPackageFileName : "(NULL)")
                  << std::endl);
  if (cmSystemTools::FileExists(tempPackageFileName)) {
    cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Remove old package file"
                    << std::endl);
    cmSystemTools::RemoveFile(tempPackageFileName);
  }
  if (cmSystemTools::IsOn(
        this->GetOption("CPACK_INCLUDE_TOPLEVEL_DIRECTORY"))) {
    tempDirectory = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  }

  // The files to be installed
  files = gl.GetFiles();

  packageFileNames.clear();
  /* Put at least one file name into the list of
   * wanted packageFileNames. The specific generator
   * may update this during PackageFiles.
   * (either putting several names or updating the provided one)
   */
  packageFileNames.push_back(tempPackageFileName ? tempPackageFileName : "");
  toplevel = tempDirectory;
  if (!this->PackageFiles() || cmSystemTools::GetErrorOccuredFlag()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem compressing the directory"
                    << std::endl);
    return 0;
  }

  /* Prepare checksum algorithm*/
  const char* algo = this->GetOption("CPACK_PACKAGE_CHECKSUM");
  CM_AUTO_PTR<cmCryptoHash> crypto = cmCryptoHash::New(algo ? algo : "");

  /*
   * Copy the generated packages to final destination
   *  - there may be several of them
   *  - the initially provided name may have changed
   *    (because the specific generator did 'normalize' it)
   */
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Copying final package(s) ["
                  << packageFileNames.size() << "]:" << std::endl);
  std::vector<std::string>::iterator it;
  /* now copy package one by one */
  for (it = packageFileNames.begin(); it != packageFileNames.end(); ++it) {
    std::string tmpPF(this->GetOption("CPACK_OUTPUT_FILE_PREFIX"));
    std::string filename(cmSystemTools::GetFilenameName(*it));
    tempPackageFileName = it->c_str();
    tmpPF += "/" + filename;
    const char* packageFileName = tmpPF.c_str();
    cmCPackLogger(cmCPackLog::LOG_DEBUG, "Copy final package(s): "
                    << (tempPackageFileName ? tempPackageFileName : "(NULL)")
                    << " to " << (packageFileName ? packageFileName : "(NULL)")
                    << std::endl);
    if (!cmSystemTools::CopyFileIfDifferent(tempPackageFileName,
                                            packageFileName)) {
      cmCPackLogger(
        cmCPackLog::LOG_ERROR, "Problem copying the package: "
          << (tempPackageFileName ? tempPackageFileName : "(NULL)") << " to "
          << (packageFileName ? packageFileName : "(NULL)") << std::endl);
      return 0;
    }
    cmCPackLogger(cmCPackLog::LOG_OUTPUT, "- package: "
                    << packageFileName << " generated." << std::endl);

    /* Generate checksum file */
    if (crypto.get() != CM_NULLPTR) {
      std::string hashFile(this->GetOption("CPACK_OUTPUT_FILE_PREFIX"));
      hashFile +=
        "/" + filename.substr(0, filename.rfind(this->GetOutputExtension()));
      hashFile += "." + cmSystemTools::LowerCase(algo);
      cmsys::ofstream outF(hashFile.c_str());
      if (!outF) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot create checksum file: "
                        << hashFile << std::endl);
        return 0;
      }
      outF << crypto->HashFile(packageFileName) << "  " << filename << "\n";
      cmCPackLogger(cmCPackLog::LOG_OUTPUT, "- checksum file: "
                      << hashFile << " generated." << std::endl);
    }
  }

  return 1;
}

int cmCPackGenerator::Initialize(const std::string& name, cmMakefile* mf)
{
  this->MakefileMap = mf;
  this->Name = name;
  // set the running generator name
  this->SetOption("CPACK_GENERATOR", this->Name.c_str());
  // Load the project specific config file
  const char* config = this->GetOption("CPACK_PROJECT_CONFIG_FILE");
  if (config) {
    mf->ReadListFile(config);
  }
  int result = this->InitializeInternal();
  if (cmSystemTools::GetErrorOccuredFlag()) {
    return 0;
  }

  // If a generator subclass did not already set this option in its
  // InitializeInternal implementation, and the project did not already set
  // it, the default value should be:
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/");

  return result;
}

int cmCPackGenerator::InitializeInternal()
{
  return 1;
}

bool cmCPackGenerator::IsSet(const std::string& name) const
{
  return this->MakefileMap->IsSet(name);
}

bool cmCPackGenerator::IsOn(const std::string& name) const
{
  return cmSystemTools::IsOn(GetOption(name));
}

bool cmCPackGenerator::IsSetToOff(const std::string& op) const
{
  const char* ret = this->MakefileMap->GetDefinition(op);
  if (ret && *ret) {
    return cmSystemTools::IsOff(ret);
  }
  return false;
}

bool cmCPackGenerator::IsSetToEmpty(const std::string& op) const
{
  const char* ret = this->MakefileMap->GetDefinition(op);
  if (ret) {
    return !*ret;
  }
  return false;
}

const char* cmCPackGenerator::GetOption(const std::string& op) const
{
  const char* ret = this->MakefileMap->GetDefinition(op);
  if (!ret) {
    cmCPackLogger(cmCPackLog::LOG_DEBUG,
                  "Warning, GetOption return NULL for: " << op << std::endl);
  }
  return ret;
}

std::vector<std::string> cmCPackGenerator::GetOptions() const
{
  return this->MakefileMap->GetDefinitions();
}

int cmCPackGenerator::PackageFiles()
{
  return 0;
}

const char* cmCPackGenerator::GetInstallPath()
{
  if (!this->InstallPath.empty()) {
    return this->InstallPath.c_str();
  }
#if defined(_WIN32) && !defined(__CYGWIN__)
  std::string prgfiles;
  std::string sysDrive;
  if (cmsys::SystemTools::GetEnv("ProgramFiles", prgfiles)) {
    this->InstallPath = prgfiles;
  } else if (cmsys::SystemTools::GetEnv("SystemDrive", sysDrive)) {
    this->InstallPath = sysDrive;
    this->InstallPath += "/Program Files";
  } else {
    this->InstallPath = "c:/Program Files";
  }
  this->InstallPath += "/";
  this->InstallPath += this->GetOption("CPACK_PACKAGE_NAME");
  this->InstallPath += "-";
  this->InstallPath += this->GetOption("CPACK_PACKAGE_VERSION");
#elif defined(__HAIKU__)
  char dir[B_PATH_NAME_LENGTH];
  if (find_directory(B_SYSTEM_DIRECTORY, -1, false, dir, sizeof(dir)) ==
      B_OK) {
    this->InstallPath = dir;
  } else {
    this->InstallPath = "/boot/system";
  }
#else
  this->InstallPath = "/usr/local/";
#endif
  return this->InstallPath.c_str();
}

const char* cmCPackGenerator::GetPackagingInstallPrefix()
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "GetPackagingInstallPrefix: '"
                  << this->GetOption("CPACK_PACKAGING_INSTALL_PREFIX") << "'"
                  << std::endl);

  return this->GetOption("CPACK_PACKAGING_INSTALL_PREFIX");
}

std::string cmCPackGenerator::FindTemplate(const char* name)
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Look for template: "
                  << (name ? name : "(NULL)") << std::endl);
  std::string ffile = this->MakefileMap->GetModulesFile(name);
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Found template: " << ffile
                                                          << std::endl);
  return ffile;
}

bool cmCPackGenerator::ConfigureString(const std::string& inString,
                                       std::string& outString)
{
  this->MakefileMap->ConfigureString(inString, outString, true, false);
  return true;
}

bool cmCPackGenerator::ConfigureFile(const char* inName, const char* outName,
                                     bool copyOnly /* = false */)
{
  return this->MakefileMap->ConfigureFile(inName, outName, copyOnly, true,
                                          false) == 1;
}

int cmCPackGenerator::CleanTemporaryDirectory()
{
  std::string tempInstallDirectoryWithPostfix =
    this->GetOption("CPACK_TEMPORARY_INSTALL_DIRECTORY");
  const char* tempInstallDirectory = tempInstallDirectoryWithPostfix.c_str();
  if (cmsys::SystemTools::FileExists(tempInstallDirectory)) {
    cmCPackLogger(cmCPackLog::LOG_OUTPUT,
                  "- Clean temporary : " << tempInstallDirectory << std::endl);
    if (!cmSystemTools::RepeatedRemoveDirectory(tempInstallDirectory)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Problem removing temporary directory: "
                      << tempInstallDirectory << std::endl);
      return 0;
    }
  }
  return 1;
}

cmInstalledFile const* cmCPackGenerator::GetInstalledFile(
  std::string const& name) const
{
  cmake const* cm = this->MakefileMap->GetCMakeInstance();
  return cm->GetInstalledFile(name);
}

int cmCPackGenerator::PrepareGroupingKind()
{
  // find a component package method specified by the user
  ComponentPackageMethod method = UNKNOWN_COMPONENT_PACKAGE_METHOD;

  if (this->GetOption("CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE")) {
    method = ONE_PACKAGE;
  }

  if (this->GetOption("CPACK_COMPONENTS_IGNORE_GROUPS")) {
    method = ONE_PACKAGE_PER_COMPONENT;
  }

  if (this->GetOption("CPACK_COMPONENTS_ONE_PACKAGE_PER_GROUP")) {
    method = ONE_PACKAGE_PER_GROUP;
  }

  std::string groupingType;

  // Second way to specify grouping
  if (CM_NULLPTR != this->GetOption("CPACK_COMPONENTS_GROUPING")) {
    groupingType = this->GetOption("CPACK_COMPONENTS_GROUPING");
  }

  if (!groupingType.empty()) {
    cmCPackLogger(cmCPackLog::LOG_VERBOSE, "["
                    << this->Name << "]"
                    << " requested component grouping = " << groupingType
                    << std::endl);
    if (groupingType == "ALL_COMPONENTS_IN_ONE") {
      method = ONE_PACKAGE;
    } else if (groupingType == "IGNORE") {
      method = ONE_PACKAGE_PER_COMPONENT;
    } else if (groupingType == "ONE_PER_GROUP") {
      method = ONE_PACKAGE_PER_GROUP;
    } else {
      cmCPackLogger(
        cmCPackLog::LOG_WARNING, "["
          << this->Name << "]"
          << " requested component grouping type <" << groupingType
          << "> UNKNOWN not in (ALL_COMPONENTS_IN_ONE,IGNORE,ONE_PER_GROUP)"
          << std::endl);
    }
  }

  // Some components were defined but NO group
  // fallback to default if not group based
  if (method == ONE_PACKAGE_PER_GROUP && this->ComponentGroups.empty() &&
      !this->Components.empty()) {
    if (componentPackageMethod == ONE_PACKAGE) {
      method = ONE_PACKAGE;
    } else {
      method = ONE_PACKAGE_PER_COMPONENT;
    }
    cmCPackLogger(
      cmCPackLog::LOG_WARNING, "["
        << this->Name << "]"
        << " One package per component group requested, "
        << "but NO component groups exist: Ignoring component group."
        << std::endl);
  }

  // if user specified packaging method, override the default packaging method
  if (method != UNKNOWN_COMPONENT_PACKAGE_METHOD) {
    componentPackageMethod = method;
  }

  const char* method_names[] = { "ALL_COMPONENTS_IN_ONE", "IGNORE_GROUPS",
                                 "ONE_PER_GROUP" };

  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "["
                  << this->Name << "]"
                  << " requested component grouping = "
                  << method_names[componentPackageMethod] << std::endl);

  return 1;
}

std::string cmCPackGenerator::GetComponentInstallDirNameSuffix(
  const std::string& componentName)
{
  return componentName;
}
std::string cmCPackGenerator::GetComponentPackageFileName(
  const std::string& initialPackageFileName,
  const std::string& groupOrComponentName, bool isGroupName)
{

  /*
   * the default behavior is to use the
   * component [group] name as a suffix
   */
  std::string suffix = "-" + groupOrComponentName;
  /* check if we should use DISPLAY name */
  std::string dispNameVar = "CPACK_" + Name + "_USE_DISPLAY_NAME_IN_FILENAME";
  if (IsOn(dispNameVar)) {
    /* the component Group case */
    if (isGroupName) {
      std::string groupDispVar = "CPACK_COMPONENT_GROUP_" +
        cmSystemTools::UpperCase(groupOrComponentName) + "_DISPLAY_NAME";
      const char* groupDispName = GetOption(groupDispVar);
      if (groupDispName) {
        suffix = "-" + std::string(groupDispName);
      }
    }
    /* the [single] component case */
    else {
      std::string dispVar = "CPACK_COMPONENT_" +
        cmSystemTools::UpperCase(groupOrComponentName) + "_DISPLAY_NAME";
      const char* dispName = GetOption(dispVar);
      if (dispName) {
        suffix = "-" + std::string(dispName);
      }
    }
  }
  return initialPackageFileName + suffix;
}

enum cmCPackGenerator::CPackSetDestdirSupport
cmCPackGenerator::SupportsSetDestdir() const
{
  return cmCPackGenerator::SETDESTDIR_SUPPORTED;
}

bool cmCPackGenerator::SupportsAbsoluteDestination() const
{
  return true;
}

bool cmCPackGenerator::SupportsComponentInstallation() const
{
  return false;
}

bool cmCPackGenerator::WantsComponentInstallation() const
{
  return (!IsOn("CPACK_MONOLITHIC_INSTALL") && SupportsComponentInstallation()
          // check that we have at least one group or component
          && (!this->ComponentGroups.empty() || !this->Components.empty()));
}

cmCPackInstallationType* cmCPackGenerator::GetInstallationType(
  const std::string& projectName, const std::string& name)
{
  (void)projectName;
  bool hasInstallationType = this->InstallationTypes.count(name) != 0;
  cmCPackInstallationType* installType = &this->InstallationTypes[name];
  if (!hasInstallationType) {
    // Define the installation type
    std::string macroPrefix =
      "CPACK_INSTALL_TYPE_" + cmsys::SystemTools::UpperCase(name);
    installType->Name = name;

    const char* displayName = this->GetOption(macroPrefix + "_DISPLAY_NAME");
    if (displayName && *displayName) {
      installType->DisplayName = displayName;
    } else {
      installType->DisplayName = installType->Name;
    }

    installType->Index = static_cast<unsigned>(this->InstallationTypes.size());
  }
  return installType;
}

cmCPackComponent* cmCPackGenerator::GetComponent(
  const std::string& projectName, const std::string& name)
{
  bool hasComponent = this->Components.count(name) != 0;
  cmCPackComponent* component = &this->Components[name];
  if (!hasComponent) {
    // Define the component
    std::string macroPrefix =
      "CPACK_COMPONENT_" + cmsys::SystemTools::UpperCase(name);
    component->Name = name;
    const char* displayName = this->GetOption(macroPrefix + "_DISPLAY_NAME");
    if (displayName && *displayName) {
      component->DisplayName = displayName;
    } else {
      component->DisplayName = component->Name;
    }
    component->IsHidden = this->IsOn(macroPrefix + "_HIDDEN");
    component->IsRequired = this->IsOn(macroPrefix + "_REQUIRED");
    component->IsDisabledByDefault = this->IsOn(macroPrefix + "_DISABLED");
    component->IsDownloaded = this->IsOn(macroPrefix + "_DOWNLOADED") ||
      cmSystemTools::IsOn(this->GetOption("CPACK_DOWNLOAD_ALL"));

    const char* archiveFile = this->GetOption(macroPrefix + "_ARCHIVE_FILE");
    if (archiveFile && *archiveFile) {
      component->ArchiveFile = archiveFile;
    }

    const char* plist = this->GetOption(macroPrefix + "_PLIST");
    if (plist && *plist) {
      component->Plist = plist;
    }

    const char* groupName = this->GetOption(macroPrefix + "_GROUP");
    if (groupName && *groupName) {
      component->Group = GetComponentGroup(projectName, groupName);
      component->Group->Components.push_back(component);
    } else {
      component->Group = CM_NULLPTR;
    }

    const char* description = this->GetOption(macroPrefix + "_DESCRIPTION");
    if (description && *description) {
      component->Description = description;
    }

    // Determine the installation types.
    const char* installTypes = this->GetOption(macroPrefix + "_INSTALL_TYPES");
    if (installTypes && *installTypes) {
      std::vector<std::string> installTypesVector;
      cmSystemTools::ExpandListArgument(installTypes, installTypesVector);
      std::vector<std::string>::iterator installTypesIt;
      for (installTypesIt = installTypesVector.begin();
           installTypesIt != installTypesVector.end(); ++installTypesIt) {
        component->InstallationTypes.push_back(
          this->GetInstallationType(projectName, *installTypesIt));
      }
    }

    // Determine the component dependencies.
    const char* depends = this->GetOption(macroPrefix + "_DEPENDS");
    if (depends && *depends) {
      std::vector<std::string> dependsVector;
      cmSystemTools::ExpandListArgument(depends, dependsVector);
      std::vector<std::string>::iterator dependIt;
      for (dependIt = dependsVector.begin(); dependIt != dependsVector.end();
           ++dependIt) {
        cmCPackComponent* child = GetComponent(projectName, *dependIt);
        component->Dependencies.push_back(child);
        child->ReverseDependencies.push_back(component);
      }
    }
  }
  return component;
}

cmCPackComponentGroup* cmCPackGenerator::GetComponentGroup(
  const std::string& projectName, const std::string& name)
{
  (void)projectName;
  std::string macroPrefix =
    "CPACK_COMPONENT_GROUP_" + cmsys::SystemTools::UpperCase(name);
  bool hasGroup = this->ComponentGroups.count(name) != 0;
  cmCPackComponentGroup* group = &this->ComponentGroups[name];
  if (!hasGroup) {
    // Define the group
    group->Name = name;
    const char* displayName = this->GetOption(macroPrefix + "_DISPLAY_NAME");
    if (displayName && *displayName) {
      group->DisplayName = displayName;
    } else {
      group->DisplayName = group->Name;
    }

    const char* description = this->GetOption(macroPrefix + "_DESCRIPTION");
    if (description && *description) {
      group->Description = description;
    }
    group->IsBold = this->IsOn(macroPrefix + "_BOLD_TITLE");
    group->IsExpandedByDefault = this->IsOn(macroPrefix + "_EXPANDED");
    const char* parentGroupName =
      this->GetOption(macroPrefix + "_PARENT_GROUP");
    if (parentGroupName && *parentGroupName) {
      group->ParentGroup = GetComponentGroup(projectName, parentGroupName);
      group->ParentGroup->Subgroups.push_back(group);
    } else {
      group->ParentGroup = CM_NULLPTR;
    }
  }
  return group;
}
