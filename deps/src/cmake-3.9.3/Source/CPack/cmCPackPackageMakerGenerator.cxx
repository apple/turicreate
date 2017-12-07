/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackPackageMakerGenerator.h"

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"
#include <assert.h>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "cmCPackComponentGroup.h"
#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"

static inline unsigned int getVersion(unsigned int major, unsigned int minor)
{
  assert(major < 256 && minor < 256);
  return ((major & 0xFF) << 16 | minor);
}

cmCPackPackageMakerGenerator::cmCPackPackageMakerGenerator()
{
  this->PackageMakerVersion = 0.0;
  this->PackageCompatibilityVersion = getVersion(10, 4);
}

cmCPackPackageMakerGenerator::~cmCPackPackageMakerGenerator()
{
}

bool cmCPackPackageMakerGenerator::SupportsComponentInstallation() const
{
  return this->PackageCompatibilityVersion >= getVersion(10, 4);
}

int cmCPackPackageMakerGenerator::PackageFiles()
{
  // TODO: Use toplevel
  //       It is used! Is this an obsolete comment?

  std::string resDir; // Where this package's resources will go.
  std::string packageDirFileName =
    this->GetOption("CPACK_TEMPORARY_DIRECTORY");
  if (this->Components.empty()) {
    packageDirFileName += ".pkg";
    resDir = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
    resDir += "/Resources";
  } else {
    packageDirFileName += ".mpkg";
    if (!cmsys::SystemTools::MakeDirectory(packageDirFileName.c_str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "unable to create package directory " << packageDirFileName
                                                          << std::endl);
      return 0;
    }

    resDir = packageDirFileName;
    resDir += "/Contents";
    if (!cmsys::SystemTools::MakeDirectory(resDir.c_str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "unable to create package subdirectory " << resDir
                                                             << std::endl);
      return 0;
    }

    resDir += "/Resources";
    if (!cmsys::SystemTools::MakeDirectory(resDir.c_str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "unable to create package subdirectory " << resDir
                                                             << std::endl);
      return 0;
    }

    resDir += "/en.lproj";
  }

  const char* preflight = this->GetOption("CPACK_PREFLIGHT_SCRIPT");
  const char* postflight = this->GetOption("CPACK_POSTFLIGHT_SCRIPT");
  const char* postupgrade = this->GetOption("CPACK_POSTUPGRADE_SCRIPT");

  if (this->Components.empty()) {
    // Create directory structure
    std::string preflightDirName = resDir + "/PreFlight";
    std::string postflightDirName = resDir + "/PostFlight";
    // if preflight or postflight scripts not there create directories
    // of the same name, I think this makes it work
    if (!preflight) {
      if (!cmsys::SystemTools::MakeDirectory(preflightDirName.c_str())) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Problem creating installer directory: "
                        << preflightDirName << std::endl);
        return 0;
      }
    }
    if (!postflight) {
      if (!cmsys::SystemTools::MakeDirectory(postflightDirName.c_str())) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Problem creating installer directory: "
                        << postflightDirName << std::endl);
        return 0;
      }
    }
    // if preflight, postflight, or postupgrade are set
    // then copy them into the resource directory and make
    // them executable
    if (preflight) {
      this->CopyInstallScript(resDir, preflight, "preflight");
    }
    if (postflight) {
      this->CopyInstallScript(resDir, postflight, "postflight");
    }
    if (postupgrade) {
      this->CopyInstallScript(resDir, postupgrade, "postupgrade");
    }
  } else if (postflight) {
    // create a postflight component to house the script
    this->PostFlightComponent.Name = "PostFlight";
    this->PostFlightComponent.DisplayName = "PostFlight";
    this->PostFlightComponent.Description = "PostFlight";
    this->PostFlightComponent.IsHidden = true;

    // empty directory for pkg contents
    std::string packageDir = toplevel + "/" + PostFlightComponent.Name;
    if (!cmsys::SystemTools::MakeDirectory(packageDir.c_str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Problem creating component packages directory: "
                      << packageDir << std::endl);
      return 0;
    }

    // create package
    std::string packageFileDir = packageDirFileName + "/Contents/Packages/";
    if (!cmsys::SystemTools::MakeDirectory(packageFileDir.c_str())) {
      cmCPackLogger(
        cmCPackLog::LOG_ERROR,
        "Problem creating component PostFlight Packages directory: "
          << packageFileDir << std::endl);
      return 0;
    }
    std::string packageFile =
      packageFileDir + this->GetPackageName(PostFlightComponent);
    if (!this->GenerateComponentPackage(
          packageFile.c_str(), packageDir.c_str(), PostFlightComponent)) {
      return 0;
    }

    // copy postflight script into resource directory of .pkg
    std::string resourceDir = packageFile + "/Contents/Resources";
    this->CopyInstallScript(resourceDir, postflight, "postflight");
  }

  if (!this->Components.empty()) {
    // Create the directory where component packages will be built.
    std::string basePackageDir = packageDirFileName;
    basePackageDir += "/Contents/Packages";
    if (!cmsys::SystemTools::MakeDirectory(basePackageDir.c_str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Problem creating component packages directory: "
                      << basePackageDir << std::endl);
      return 0;
    }

    // Create the directory where downloaded component packages will
    // be placed.
    const char* userUploadDirectory =
      this->GetOption("CPACK_UPLOAD_DIRECTORY");
    std::string uploadDirectory;
    if (userUploadDirectory && *userUploadDirectory) {
      uploadDirectory = userUploadDirectory;
    } else {
      uploadDirectory = this->GetOption("CPACK_PACKAGE_DIRECTORY");
      uploadDirectory += "/CPackUploads";
    }

    // Create packages for each component
    bool warnedAboutDownloadCompatibility = false;

    std::map<std::string, cmCPackComponent>::iterator compIt;
    for (compIt = this->Components.begin(); compIt != this->Components.end();
         ++compIt) {
      std::string packageFile;
      if (compIt->second.IsDownloaded) {
        if (this->PackageCompatibilityVersion >= getVersion(10, 5) &&
            this->PackageMakerVersion >= 3.0) {
          // Build this package within the upload directory.
          packageFile = uploadDirectory;

          if (!cmSystemTools::FileExists(uploadDirectory.c_str())) {
            if (!cmSystemTools::MakeDirectory(uploadDirectory.c_str())) {
              cmCPackLogger(cmCPackLog::LOG_ERROR,
                            "Unable to create package upload directory "
                              << uploadDirectory << std::endl);
              return 0;
            }
          }
        } else if (!warnedAboutDownloadCompatibility) {
          if (this->PackageCompatibilityVersion < getVersion(10, 5)) {
            cmCPackLogger(
              cmCPackLog::LOG_WARNING,
              "CPack warning: please set CPACK_OSX_PACKAGE_VERSION to 10.5 "
              "or greater enable downloaded packages. CPack will build a "
              "non-downloaded package."
                << std::endl);
          }

          if (this->PackageMakerVersion < 3) {
            cmCPackLogger(cmCPackLog::LOG_WARNING,
                          "CPack warning: unable to build downloaded "
                          "packages with PackageMaker versions prior "
                          "to 3.0. CPack will build a non-downloaded package."
                            << std::endl);
          }

          warnedAboutDownloadCompatibility = true;
        }
      }

      if (packageFile.empty()) {
        // Build this package within the overall distribution
        // metapackage.
        packageFile = basePackageDir;

        // We're not downloading this component, even if the user
        // requested it.
        compIt->second.IsDownloaded = false;
      }

      packageFile += '/';
      packageFile += GetPackageName(compIt->second);

      std::string packageDir = toplevel;
      packageDir += '/';
      packageDir += compIt->first;
      if (!this->GenerateComponentPackage(
            packageFile.c_str(), packageDir.c_str(), compIt->second)) {
        return 0;
      }
    }
  }
  this->SetOption("CPACK_MODULE_VERSION_SUFFIX", "");

  // Copy or create all of the resource files we need.
  if (!this->CopyCreateResourceFile("License", resDir) ||
      !this->CopyCreateResourceFile("ReadMe", resDir) ||
      !this->CopyCreateResourceFile("Welcome", resDir) ||
      !this->CopyResourcePlistFile("Info.plist") ||
      !this->CopyResourcePlistFile("Description.plist")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem copying the resource files"
                    << std::endl);
    return 0;
  }

  if (this->Components.empty()) {
    // Use PackageMaker to build the package.
    std::ostringstream pkgCmd;
    pkgCmd << "\"" << this->GetOption("CPACK_INSTALLER_PROGRAM")
           << "\" -build -p \"" << packageDirFileName << "\"";
    if (this->Components.empty()) {
      pkgCmd << " -f \"" << this->GetOption("CPACK_TEMPORARY_DIRECTORY");
    } else {
      pkgCmd << " -mi \"" << this->GetOption("CPACK_TEMPORARY_DIRECTORY")
             << "/packages/";
    }
    pkgCmd << "\" -r \"" << this->GetOption("CPACK_TOPLEVEL_DIRECTORY")
           << "/Resources\" -i \""
           << this->GetOption("CPACK_TOPLEVEL_DIRECTORY")
           << "/Info.plist\" -d \""
           << this->GetOption("CPACK_TOPLEVEL_DIRECTORY")
           << "/Description.plist\"";
    if (this->PackageMakerVersion > 2.0) {
      pkgCmd << " -v";
    }
    if (!RunPackageMaker(pkgCmd.str().c_str(), packageDirFileName.c_str()))
      return 0;
  } else {
    // We have built the package in place. Generate the
    // distribution.dist file to describe it for the installer.
    WriteDistributionFile(packageDirFileName.c_str());
  }

  std::string tmpFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  tmpFile += "/hdiutilOutput.log";
  std::ostringstream dmgCmd;
  dmgCmd << "\"" << this->GetOption("CPACK_INSTALLER_PROGRAM_DISK_IMAGE")
         << "\" create -ov -format UDZO -srcfolder \"" << packageDirFileName
         << "\" \"" << packageFileNames[0] << "\"";
  std::string output;
  int retVal = 1;
  int numTries = 10;
  bool res = false;
  while (numTries > 0) {
    res =
      cmSystemTools::RunSingleCommand(dmgCmd.str().c_str(), &output, &output,
                                      &retVal, 0, this->GeneratorVerbose, 0);
    if (res && !retVal) {
      numTries = -1;
      break;
    }
    cmSystemTools::Delay(500);
    numTries--;
  }
  if (!res || retVal) {
    cmGeneratedFileStream ofs(tmpFile.c_str());
    ofs << "# Run command: " << dmgCmd.str() << std::endl
        << "# Output:" << std::endl
        << output << std::endl;
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem running hdiutil command: "
                    << dmgCmd.str() << std::endl
                    << "Please check " << tmpFile << " for errors"
                    << std::endl);
    return 0;
  }

  return 1;
}

int cmCPackPackageMakerGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/usr");

  // Starting with Xcode 4.3, PackageMaker is a separate app, and you
  // can put it anywhere you want. So... use a variable for its location.
  // People who put it in unexpected places can use the variable to tell
  // us where it is.
  //
  // Use the following locations, in "most recent installation" order,
  // to search for the PackageMaker app. Assume people who copy it into
  // the new Xcode 4.3 app in "/Applications" will copy it into the nested
  // Applications folder inside the Xcode bundle itself. Or directly in
  // the "/Applications" directory.
  //
  // If found, save result in the CPACK_INSTALLER_PROGRAM variable.

  std::vector<std::string> paths;
  paths.push_back("/Applications/Xcode.app/Contents/Applications"
                  "/PackageMaker.app/Contents/MacOS");
  paths.push_back("/Applications/Utilities"
                  "/PackageMaker.app/Contents/MacOS");
  paths.push_back("/Applications"
                  "/PackageMaker.app/Contents/MacOS");
  paths.push_back("/Developer/Applications/Utilities"
                  "/PackageMaker.app/Contents/MacOS");
  paths.push_back("/Developer/Applications"
                  "/PackageMaker.app/Contents/MacOS");

  std::string pkgPath;
  const char* inst_program = this->GetOption("CPACK_INSTALLER_PROGRAM");
  if (inst_program && *inst_program) {
    pkgPath = inst_program;
  } else {
    pkgPath = cmSystemTools::FindProgram("PackageMaker", paths, false);
    if (pkgPath.empty()) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find PackageMaker compiler"
                      << std::endl);
      return 0;
    }
    this->SetOptionIfNotSet("CPACK_INSTALLER_PROGRAM", pkgPath.c_str());
  }

  // Get path to the real PackageMaker, not a symlink:
  pkgPath = cmSystemTools::GetRealPath(pkgPath);
  // Up from there to find the version.plist file in the "Contents" dir:
  std::string contents_dir;
  contents_dir = cmSystemTools::GetFilenamePath(pkgPath);
  contents_dir = cmSystemTools::GetFilenamePath(contents_dir);

  std::string versionFile = contents_dir + "/version.plist";

  if (!cmSystemTools::FileExists(versionFile.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot find PackageMaker compiler version file: "
                    << versionFile << std::endl);
    return 0;
  }

  cmsys::ifstream ifs(versionFile.c_str());
  if (!ifs) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot open PackageMaker compiler version file"
                    << std::endl);
    return 0;
  }

  // Check the PackageMaker version
  cmsys::RegularExpression rexKey("<key>CFBundleShortVersionString</key>");
  cmsys::RegularExpression rexVersion("<string>([0-9]+.[0-9.]+)</string>");
  std::string line;
  bool foundKey = false;
  while (cmSystemTools::GetLineFromStream(ifs, line)) {
    if (rexKey.find(line)) {
      foundKey = true;
      break;
    }
  }
  if (!foundKey) {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR,
      "Cannot find CFBundleShortVersionString in the PackageMaker compiler "
      "version file"
        << std::endl);
    return 0;
  }
  if (!cmSystemTools::GetLineFromStream(ifs, line) || !rexVersion.find(line)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem reading the PackageMaker compiler version file: "
                    << versionFile << std::endl);
    return 0;
  }
  this->PackageMakerVersion = atof(rexVersion.match(1).c_str());
  if (this->PackageMakerVersion < 1.0) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Require PackageMaker 1.0 or higher"
                    << std::endl);
    return 0;
  }
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "PackageMaker version is: "
                  << this->PackageMakerVersion << std::endl);

  // Determine the package compatibility version. If it wasn't
  // specified by the user, we define it based on which features the
  // user requested.
  const char* packageCompat = this->GetOption("CPACK_OSX_PACKAGE_VERSION");
  if (packageCompat && *packageCompat) {
    unsigned int majorVersion = 10;
    unsigned int minorVersion = 5;
    int res = sscanf(packageCompat, "%u.%u", &majorVersion, &minorVersion);
    if (res == 2) {
      this->PackageCompatibilityVersion =
        getVersion(majorVersion, minorVersion);
    }
  } else if (this->GetOption("CPACK_DOWNLOAD_SITE")) {
    this->SetOption("CPACK_OSX_PACKAGE_VERSION", "10.5");
    this->PackageCompatibilityVersion = getVersion(10, 5);
  } else if (this->GetOption("CPACK_COMPONENTS_ALL")) {
    this->SetOption("CPACK_OSX_PACKAGE_VERSION", "10.4");
    this->PackageCompatibilityVersion = getVersion(10, 4);
  } else {
    this->SetOption("CPACK_OSX_PACKAGE_VERSION", "10.3");
    this->PackageCompatibilityVersion = getVersion(10, 3);
  }

  std::vector<std::string> no_paths;
  pkgPath = cmSystemTools::FindProgram("hdiutil", no_paths, false);
  if (pkgPath.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find hdiutil compiler"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_INSTALLER_PROGRAM_DISK_IMAGE",
                          pkgPath.c_str());

  return this->Superclass::InitializeInternal();
}

bool cmCPackPackageMakerGenerator::RunPackageMaker(const char* command,
                                                   const char* packageFile)
{
  std::string tmpFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  tmpFile += "/PackageMakerOutput.log";

  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Execute: " << command << std::endl);
  std::string output;
  int retVal = 1;
  bool res = cmSystemTools::RunSingleCommand(
    command, &output, &output, &retVal, 0, this->GeneratorVerbose, 0);
  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Done running package maker"
                  << std::endl);
  if (!res || retVal) {
    cmGeneratedFileStream ofs(tmpFile.c_str());
    ofs << "# Run command: " << command << std::endl
        << "# Output:" << std::endl
        << output << std::endl;
    cmCPackLogger(
      cmCPackLog::LOG_ERROR, "Problem running PackageMaker command: "
        << command << std::endl
        << "Please check " << tmpFile << " for errors" << std::endl);
    return false;
  }
  // sometimes the command finishes but the directory is not yet
  // created, so try 10 times to see if it shows up
  int tries = 10;
  while (tries > 0 && !cmSystemTools::FileExists(packageFile)) {
    cmSystemTools::Delay(500);
    tries--;
  }
  if (!cmSystemTools::FileExists(packageFile)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Problem running PackageMaker command: "
                    << command << std::endl
                    << "Package not created: " << packageFile << std::endl);
    return false;
  }

  return true;
}

bool cmCPackPackageMakerGenerator::GenerateComponentPackage(
  const char* packageFile, const char* packageDir,
  const cmCPackComponent& component)
{
  cmCPackLogger(cmCPackLog::LOG_OUTPUT, "-   Building component package: "
                  << packageFile << std::endl);

  // The command that will be used to run PackageMaker
  std::ostringstream pkgCmd;

  if (this->PackageCompatibilityVersion < getVersion(10, 5) ||
      this->PackageMakerVersion < 3.0) {
    // Create Description.plist and Info.plist files for normal Mac OS
    // X packages, which work on Mac OS X 10.3 and newer.
    std::string descriptionFile = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
    descriptionFile += '/' + component.Name + "-Description.plist";
    cmsys::ofstream out(descriptionFile.c_str());
    cmXMLWriter xout(out);
    xout.StartDocument();
    xout.Doctype("plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\""
                 "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
    xout.StartElement("plist");
    xout.Attribute("version", "1.4");
    xout.StartElement("dict");
    xout.Element("key", "IFPkgDescriptionTitle");
    xout.Element("string", component.DisplayName);
    xout.Element("key", "IFPkgDescriptionVersion");
    xout.Element("string", this->GetOption("CPACK_PACKAGE_VERSION"));
    xout.Element("key", "IFPkgDescriptionDescription");
    xout.Element("string", component.Description);
    xout.EndElement(); // dict
    xout.EndElement(); // plist
    xout.EndDocument();
    out.close();

    // Create the Info.plist file for this component
    std::string moduleVersionSuffix = ".";
    moduleVersionSuffix += component.Name;
    this->SetOption("CPACK_MODULE_VERSION_SUFFIX",
                    moduleVersionSuffix.c_str());
    std::string infoFileName = component.Name;
    infoFileName += "-Info.plist";
    if (!this->CopyResourcePlistFile("Info.plist", infoFileName.c_str())) {
      return false;
    }

    pkgCmd << "\"" << this->GetOption("CPACK_INSTALLER_PROGRAM")
           << "\" -build -p \"" << packageFile << "\""
           << " -f \"" << packageDir << "\""
           << " -i \"" << this->GetOption("CPACK_TOPLEVEL_DIRECTORY") << "/"
           << infoFileName << "\""
           << " -d \"" << descriptionFile << "\"";
  } else {
    // Create a "flat" package on Mac OS X 10.5 and newer. Flat
    // packages are stored in a single file, rather than a directory
    // like normal packages, and can be downloaded by the installer
    // on-the-fly in Mac OS X 10.5 or newer. Thus, we need to create
    // flat packages when the packages will be downloaded on the fly.
    std::string pkgId = "com.";
    pkgId += this->GetOption("CPACK_PACKAGE_VENDOR");
    pkgId += '.';
    pkgId += this->GetOption("CPACK_PACKAGE_NAME");
    pkgId += '.';
    pkgId += component.Name;

    pkgCmd << "\"" << this->GetOption("CPACK_INSTALLER_PROGRAM")
           << "\" --root \"" << packageDir << "\""
           << " --id " << pkgId << " --target "
           << this->GetOption("CPACK_OSX_PACKAGE_VERSION") << " --out \""
           << packageFile << "\"";
  }

  // Run PackageMaker
  return RunPackageMaker(pkgCmd.str().c_str(), packageFile);
}
