/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallCommand.h"

#include "cmsys/Glob.hxx"
#include <sstream>
#include <stddef.h>
#include <utility>

#include "cmAlgorithms.h"
#include "cmCommandArgumentsHelper.h"
#include "cmExportSet.h"
#include "cmExportSetMap.h"
#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmInstallCommandArguments.h"
#include "cmInstallDirectoryGenerator.h"
#include "cmInstallExportGenerator.h"
#include "cmInstallFilesGenerator.h"
#include "cmInstallGenerator.h"
#include "cmInstallScriptGenerator.h"
#include "cmInstallTargetGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetExport.h"
#include "cmake.h"

class cmExecutionStatus;

static cmInstallTargetGenerator* CreateInstallTargetGenerator(
  cmTarget& target, const cmInstallCommandArguments& args, bool impLib,
  bool forceOpt = false, bool namelink = false)
{
  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(target.GetMakefile());
  target.SetHaveInstallRule(true);
  const char* component = namelink ? args.GetNamelinkComponent().c_str()
                                   : args.GetComponent().c_str();
  return new cmInstallTargetGenerator(
    target.GetName(), args.GetDestination().c_str(), impLib,
    args.GetPermissions().c_str(), args.GetConfigurations(), component,
    message, args.GetExcludeFromAll(), args.GetOptional() || forceOpt);
}

static cmInstallFilesGenerator* CreateInstallFilesGenerator(
  cmMakefile* mf, const std::vector<std::string>& absFiles,
  const cmInstallCommandArguments& args, bool programs)
{
  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(mf);
  return new cmInstallFilesGenerator(
    absFiles, args.GetDestination().c_str(), programs,
    args.GetPermissions().c_str(), args.GetConfigurations(),
    args.GetComponent().c_str(), message, args.GetExcludeFromAll(),
    args.GetRename().c_str(), args.GetOptional());
}

// cmInstallCommand
bool cmInstallCommand::InitialPass(std::vector<std::string> const& args,
                                   cmExecutionStatus&)
{
  // Allow calling with no arguments so that arguments may be built up
  // using a variable that may be left empty.
  if (args.empty()) {
    return true;
  }

  // Enable the install target.
  this->Makefile->GetGlobalGenerator()->EnableInstallTarget();

  this->DefaultComponentName =
    this->Makefile->GetSafeDefinition("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME");
  if (this->DefaultComponentName.empty()) {
    this->DefaultComponentName = "Unspecified";
  }

  std::string const& mode = args[0];

  // Switch among the command modes.
  if (mode == "SCRIPT") {
    return this->HandleScriptMode(args);
  }
  if (mode == "CODE") {
    return this->HandleScriptMode(args);
  }
  if (mode == "TARGETS") {
    return this->HandleTargetsMode(args);
  }
  if (mode == "FILES") {
    return this->HandleFilesMode(args);
  }
  if (mode == "PROGRAMS") {
    return this->HandleFilesMode(args);
  }
  if (mode == "DIRECTORY") {
    return this->HandleDirectoryMode(args);
  }
  if (mode == "EXPORT") {
    return this->HandleExportMode(args);
  }
  if (mode == "EXPORT_ANDROID_MK") {
    return this->HandleExportAndroidMKMode(args);
  }

  // Unknown mode.
  std::string e = "called with unknown mode ";
  e += args[0];
  this->SetError(e);
  return false;
}

bool cmInstallCommand::HandleScriptMode(std::vector<std::string> const& args)
{
  std::string component = this->DefaultComponentName;
  int componentCount = 0;
  bool doing_script = false;
  bool doing_code = false;
  bool exclude_from_all = false;

  // Scan the args once for COMPONENT. Only allow one.
  //
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "COMPONENT" && i + 1 < args.size()) {
      ++componentCount;
      ++i;
      component = args[i];
    }
    if (args[i] == "EXCLUDE_FROM_ALL") {
      exclude_from_all = true;
    }
  }

  if (componentCount > 1) {
    this->SetError("given more than one COMPONENT for the SCRIPT or CODE "
                   "signature of the INSTALL command. "
                   "Use multiple INSTALL commands with one COMPONENT each.");
    return false;
  }

  // Scan the args again, this time adding install generators each time we
  // encounter a SCRIPT or CODE arg:
  //
  for (std::string const& arg : args) {
    if (arg == "SCRIPT") {
      doing_script = true;
      doing_code = false;
    } else if (arg == "CODE") {
      doing_script = false;
      doing_code = true;
    } else if (arg == "COMPONENT") {
      doing_script = false;
      doing_code = false;
    } else if (doing_script) {
      doing_script = false;
      std::string script = arg;
      if (!cmSystemTools::FileIsFullPath(script)) {
        script = this->Makefile->GetCurrentSourceDirectory();
        script += "/";
        script += arg;
      }
      if (cmSystemTools::FileIsDirectory(script)) {
        this->SetError("given a directory as value of SCRIPT argument.");
        return false;
      }
      this->Makefile->AddInstallGenerator(new cmInstallScriptGenerator(
        script.c_str(), false, component.c_str(), exclude_from_all));
    } else if (doing_code) {
      doing_code = false;
      std::string const& code = arg;
      this->Makefile->AddInstallGenerator(new cmInstallScriptGenerator(
        code.c_str(), true, component.c_str(), exclude_from_all));
    }
  }

  if (doing_script) {
    this->SetError("given no value for SCRIPT argument.");
    return false;
  }
  if (doing_code) {
    this->SetError("given no value for CODE argument.");
    return false;
  }

  // Tell the global generator about any installation component names
  // specified.
  this->Makefile->GetGlobalGenerator()->AddInstallComponent(component);

  return true;
}

/*struct InstallPart
{
  InstallPart(cmCommandArgumentsHelper* helper, const char* key,
         cmCommandArgumentGroup* group);
  cmCAStringVector argVector;
  cmInstallCommandArguments args;
};*/

bool cmInstallCommand::HandleTargetsMode(std::vector<std::string> const& args)
{
  // This is the TARGETS mode.
  std::vector<cmTarget*> targets;

  cmCommandArgumentsHelper argHelper;
  cmCommandArgumentGroup group;
  cmCAStringVector genericArgVector(&argHelper, nullptr);
  cmCAStringVector archiveArgVector(&argHelper, "ARCHIVE", &group);
  cmCAStringVector libraryArgVector(&argHelper, "LIBRARY", &group);
  cmCAStringVector runtimeArgVector(&argHelper, "RUNTIME", &group);
  cmCAStringVector objectArgVector(&argHelper, "OBJECTS", &group);
  cmCAStringVector frameworkArgVector(&argHelper, "FRAMEWORK", &group);
  cmCAStringVector bundleArgVector(&argHelper, "BUNDLE", &group);
  cmCAStringVector includesArgVector(&argHelper, "INCLUDES", &group);
  cmCAStringVector privateHeaderArgVector(&argHelper, "PRIVATE_HEADER",
                                          &group);
  cmCAStringVector publicHeaderArgVector(&argHelper, "PUBLIC_HEADER", &group);
  cmCAStringVector resourceArgVector(&argHelper, "RESOURCE", &group);
  genericArgVector.Follows(nullptr);
  group.Follows(&genericArgVector);

  argHelper.Parse(&args, nullptr);

  // now parse the generic args (i.e. the ones not specialized on LIBRARY/
  // ARCHIVE, RUNTIME etc. (see above)
  // These generic args also contain the targets and the export stuff
  std::vector<std::string> unknownArgs;
  cmInstallCommandArguments genericArgs(this->DefaultComponentName);
  cmCAStringVector targetList(&genericArgs.Parser, "TARGETS");
  cmCAString exports(&genericArgs.Parser, "EXPORT",
                     &genericArgs.ArgumentGroup);
  targetList.Follows(nullptr);
  genericArgs.ArgumentGroup.Follows(&targetList);
  genericArgs.Parse(&genericArgVector.GetVector(), &unknownArgs);
  bool success = genericArgs.Finalize();

  cmInstallCommandArguments archiveArgs(this->DefaultComponentName);
  cmInstallCommandArguments libraryArgs(this->DefaultComponentName);
  cmInstallCommandArguments runtimeArgs(this->DefaultComponentName);
  cmInstallCommandArguments objectArgs(this->DefaultComponentName);
  cmInstallCommandArguments frameworkArgs(this->DefaultComponentName);
  cmInstallCommandArguments bundleArgs(this->DefaultComponentName);
  cmInstallCommandArguments privateHeaderArgs(this->DefaultComponentName);
  cmInstallCommandArguments publicHeaderArgs(this->DefaultComponentName);
  cmInstallCommandArguments resourceArgs(this->DefaultComponentName);
  cmInstallCommandIncludesArgument includesArgs;

  // now parse the args for specific parts of the target (e.g. LIBRARY,
  // RUNTIME, ARCHIVE etc.
  archiveArgs.Parse(&archiveArgVector.GetVector(), &unknownArgs);
  libraryArgs.Parse(&libraryArgVector.GetVector(), &unknownArgs);
  runtimeArgs.Parse(&runtimeArgVector.GetVector(), &unknownArgs);
  objectArgs.Parse(&objectArgVector.GetVector(), &unknownArgs);
  frameworkArgs.Parse(&frameworkArgVector.GetVector(), &unknownArgs);
  bundleArgs.Parse(&bundleArgVector.GetVector(), &unknownArgs);
  privateHeaderArgs.Parse(&privateHeaderArgVector.GetVector(), &unknownArgs);
  publicHeaderArgs.Parse(&publicHeaderArgVector.GetVector(), &unknownArgs);
  resourceArgs.Parse(&resourceArgVector.GetVector(), &unknownArgs);
  includesArgs.Parse(&includesArgVector.GetVector(), &unknownArgs);

  if (!unknownArgs.empty()) {
    // Unknown argument.
    std::ostringstream e;
    e << "TARGETS given unknown argument \"" << unknownArgs[0] << "\".";
    this->SetError(e.str());
    return false;
  }

  // apply generic args
  archiveArgs.SetGenericArguments(&genericArgs);
  libraryArgs.SetGenericArguments(&genericArgs);
  runtimeArgs.SetGenericArguments(&genericArgs);
  objectArgs.SetGenericArguments(&genericArgs);
  frameworkArgs.SetGenericArguments(&genericArgs);
  bundleArgs.SetGenericArguments(&genericArgs);
  privateHeaderArgs.SetGenericArguments(&genericArgs);
  publicHeaderArgs.SetGenericArguments(&genericArgs);
  resourceArgs.SetGenericArguments(&genericArgs);

  success = success && archiveArgs.Finalize();
  success = success && libraryArgs.Finalize();
  success = success && runtimeArgs.Finalize();
  success = success && objectArgs.Finalize();
  success = success && frameworkArgs.Finalize();
  success = success && bundleArgs.Finalize();
  success = success && privateHeaderArgs.Finalize();
  success = success && publicHeaderArgs.Finalize();
  success = success && resourceArgs.Finalize();

  if (!success) {
    return false;
  }

  // Enforce argument rules too complex to specify for the
  // general-purpose parser.
  if (archiveArgs.GetNamelinkOnly() || runtimeArgs.GetNamelinkOnly() ||
      objectArgs.GetNamelinkOnly() || frameworkArgs.GetNamelinkOnly() ||
      bundleArgs.GetNamelinkOnly() || privateHeaderArgs.GetNamelinkOnly() ||
      publicHeaderArgs.GetNamelinkOnly() || resourceArgs.GetNamelinkOnly()) {
    this->SetError(
      "TARGETS given NAMELINK_ONLY option not in LIBRARY group.  "
      "The NAMELINK_ONLY option may be specified only following LIBRARY.");
    return false;
  }
  if (archiveArgs.GetNamelinkSkip() || runtimeArgs.GetNamelinkSkip() ||
      objectArgs.GetNamelinkSkip() || frameworkArgs.GetNamelinkSkip() ||
      bundleArgs.GetNamelinkSkip() || privateHeaderArgs.GetNamelinkSkip() ||
      publicHeaderArgs.GetNamelinkSkip() || resourceArgs.GetNamelinkSkip()) {
    this->SetError(
      "TARGETS given NAMELINK_SKIP option not in LIBRARY group.  "
      "The NAMELINK_SKIP option may be specified only following LIBRARY.");
    return false;
  }
  if (archiveArgs.HasNamelinkComponent() ||
      runtimeArgs.HasNamelinkComponent() ||
      objectArgs.HasNamelinkComponent() ||
      frameworkArgs.HasNamelinkComponent() ||
      bundleArgs.HasNamelinkComponent() ||
      privateHeaderArgs.HasNamelinkComponent() ||
      publicHeaderArgs.HasNamelinkComponent() ||
      resourceArgs.HasNamelinkComponent()) {
    this->SetError(
      "TARGETS given NAMELINK_COMPONENT option not in LIBRARY group.  "
      "The NAMELINK_COMPONENT option may be specified only following "
      "LIBRARY.");
    return false;
  }
  if (libraryArgs.GetNamelinkOnly() && libraryArgs.GetNamelinkSkip()) {
    this->SetError("TARGETS given NAMELINK_ONLY and NAMELINK_SKIP.  "
                   "At most one of these two options may be specified.");
    return false;
  }

  // Select the mode for installing symlinks to versioned shared libraries.
  cmInstallTargetGenerator::NamelinkModeType namelinkMode =
    cmInstallTargetGenerator::NamelinkModeNone;
  if (libraryArgs.GetNamelinkOnly()) {
    namelinkMode = cmInstallTargetGenerator::NamelinkModeOnly;
  } else if (libraryArgs.GetNamelinkSkip()) {
    namelinkMode = cmInstallTargetGenerator::NamelinkModeSkip;
  }

  // Check if there is something to do.
  if (targetList.GetVector().empty()) {
    return true;
  }

  // Check whether this is a DLL platform.
  bool dll_platform =
    !this->Makefile->GetSafeDefinition("CMAKE_IMPORT_LIBRARY_SUFFIX").empty();

  for (std::string const& tgt : targetList.GetVector()) {

    if (this->Makefile->IsAlias(tgt)) {
      std::ostringstream e;
      e << "TARGETS given target \"" << tgt << "\" which is an alias.";
      this->SetError(e.str());
      return false;
    }
    // Lookup this target in the current directory.
    cmTarget* target = this->Makefile->FindLocalNonAliasTarget(tgt);
    if (!target) {
      // If no local target has been found, find it in the global scope.
      target = this->Makefile->GetGlobalGenerator()->FindTarget(tgt, true);
    }
    if (target) {
      // Found the target.  Check its type.
      if (target->GetType() != cmStateEnums::EXECUTABLE &&
          target->GetType() != cmStateEnums::STATIC_LIBRARY &&
          target->GetType() != cmStateEnums::SHARED_LIBRARY &&
          target->GetType() != cmStateEnums::MODULE_LIBRARY &&
          target->GetType() != cmStateEnums::OBJECT_LIBRARY &&
          target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
        std::ostringstream e;
        e << "TARGETS given target \"" << tgt
          << "\" which is not an executable, library, or module.";
        this->SetError(e.str());
        return false;
      }
      // Store the target in the list to be installed.
      targets.push_back(target);
    } else {
      // Did not find the target.
      std::ostringstream e;
      e << "TARGETS given target \"" << tgt << "\" which does not exist.";
      this->SetError(e.str());
      return false;
    }
  }

  // Keep track of whether we will be performing an installation of
  // any files of the given type.
  bool installsArchive = false;
  bool installsLibrary = false;
  bool installsNamelink = false;
  bool installsRuntime = false;
  bool installsObject = false;
  bool installsFramework = false;
  bool installsBundle = false;
  bool installsPrivateHeader = false;
  bool installsPublicHeader = false;
  bool installsResource = false;

  // Generate install script code to install the given targets.
  for (cmTarget* ti : targets) {
    // Handle each target type.
    cmTarget& target = *ti;
    cmInstallTargetGenerator* archiveGenerator = nullptr;
    cmInstallTargetGenerator* libraryGenerator = nullptr;
    cmInstallTargetGenerator* namelinkGenerator = nullptr;
    cmInstallTargetGenerator* runtimeGenerator = nullptr;
    cmInstallTargetGenerator* objectGenerator = nullptr;
    cmInstallTargetGenerator* frameworkGenerator = nullptr;
    cmInstallTargetGenerator* bundleGenerator = nullptr;
    cmInstallFilesGenerator* privateHeaderGenerator = nullptr;
    cmInstallFilesGenerator* publicHeaderGenerator = nullptr;
    cmInstallFilesGenerator* resourceGenerator = nullptr;

    // Track whether this is a namelink-only rule.
    bool namelinkOnly = false;

    switch (target.GetType()) {
      case cmStateEnums::SHARED_LIBRARY: {
        // Shared libraries are handled differently on DLL and non-DLL
        // platforms.  All windows platforms are DLL platforms including
        // cygwin.  Currently no other platform is a DLL platform.
        if (dll_platform) {
          // When in namelink only mode skip all libraries on Windows.
          if (namelinkMode == cmInstallTargetGenerator::NamelinkModeOnly) {
            continue;
          }

          // This is a DLL platform.
          if (!archiveArgs.GetDestination().empty()) {
            // The import library uses the ARCHIVE properties.
            archiveGenerator =
              CreateInstallTargetGenerator(target, archiveArgs, true);
          }
          if (!runtimeArgs.GetDestination().empty()) {
            // The DLL uses the RUNTIME properties.
            runtimeGenerator =
              CreateInstallTargetGenerator(target, runtimeArgs, false);
          }
          if ((archiveGenerator == nullptr) && (runtimeGenerator == nullptr)) {
            this->SetError("Library TARGETS given no DESTINATION!");
            return false;
          }
        } else {
          // This is a non-DLL platform.
          // If it is marked with FRAMEWORK property use the FRAMEWORK set of
          // INSTALL properties. Otherwise, use the LIBRARY properties.
          if (target.IsFrameworkOnApple()) {
            // When in namelink only mode skip frameworks.
            if (namelinkMode == cmInstallTargetGenerator::NamelinkModeOnly) {
              continue;
            }

            // Use the FRAMEWORK properties.
            if (!frameworkArgs.GetDestination().empty()) {
              frameworkGenerator =
                CreateInstallTargetGenerator(target, frameworkArgs, false);
            } else {
              std::ostringstream e;
              e << "TARGETS given no FRAMEWORK DESTINATION for shared library "
                   "FRAMEWORK target \""
                << target.GetName() << "\".";
              this->SetError(e.str());
              return false;
            }
          } else {
            // The shared library uses the LIBRARY properties.
            if (!libraryArgs.GetDestination().empty()) {
              if (namelinkMode != cmInstallTargetGenerator::NamelinkModeOnly) {
                libraryGenerator =
                  CreateInstallTargetGenerator(target, libraryArgs, false);
                libraryGenerator->SetNamelinkMode(
                  cmInstallTargetGenerator::NamelinkModeSkip);
              }
              if (namelinkMode != cmInstallTargetGenerator::NamelinkModeSkip) {
                namelinkGenerator = CreateInstallTargetGenerator(
                  target, libraryArgs, false, false, true);
                namelinkGenerator->SetNamelinkMode(
                  cmInstallTargetGenerator::NamelinkModeOnly);
              }
              namelinkOnly =
                (namelinkMode == cmInstallTargetGenerator::NamelinkModeOnly);
            } else {
              std::ostringstream e;
              e << "TARGETS given no LIBRARY DESTINATION for shared library "
                   "target \""
                << target.GetName() << "\".";
              this->SetError(e.str());
              return false;
            }
          }
        }
      } break;
      case cmStateEnums::STATIC_LIBRARY: {
        // If it is marked with FRAMEWORK property use the FRAMEWORK set of
        // INSTALL properties. Otherwise, use the LIBRARY properties.
        if (target.IsFrameworkOnApple()) {
          // When in namelink only mode skip frameworks.
          if (namelinkMode == cmInstallTargetGenerator::NamelinkModeOnly) {
            continue;
          }

          // Use the FRAMEWORK properties.
          if (!frameworkArgs.GetDestination().empty()) {
            frameworkGenerator =
              CreateInstallTargetGenerator(target, frameworkArgs, false);
          } else {
            std::ostringstream e;
            e << "TARGETS given no FRAMEWORK DESTINATION for static library "
                 "FRAMEWORK target \""
              << target.GetName() << "\".";
            this->SetError(e.str());
            return false;
          }
        } else {
          // Static libraries use ARCHIVE properties.
          if (!archiveArgs.GetDestination().empty()) {
            archiveGenerator =
              CreateInstallTargetGenerator(target, archiveArgs, false);
          } else {
            std::ostringstream e;
            e << "TARGETS given no ARCHIVE DESTINATION for static library "
                 "target \""
              << target.GetName() << "\".";
            this->SetError(e.str());
            return false;
          }
        }
      } break;
      case cmStateEnums::MODULE_LIBRARY: {
        // Modules use LIBRARY properties.
        if (!libraryArgs.GetDestination().empty()) {
          libraryGenerator =
            CreateInstallTargetGenerator(target, libraryArgs, false);
          libraryGenerator->SetNamelinkMode(namelinkMode);
          namelinkOnly =
            (namelinkMode == cmInstallTargetGenerator::NamelinkModeOnly);
        } else {
          std::ostringstream e;
          e << "TARGETS given no LIBRARY DESTINATION for module target \""
            << target.GetName() << "\".";
          this->SetError(e.str());
          return false;
        }
      } break;
      case cmStateEnums::OBJECT_LIBRARY: {
        // Objects use OBJECT properties.
        if (!objectArgs.GetDestination().empty()) {
          // Verify that we know where the objects are to install them.
          std::string reason;
          if (!this->Makefile->GetGlobalGenerator()
                 ->HasKnownObjectFileLocation(&reason)) {
            std::ostringstream e;
            e << "TARGETS given OBJECT library \"" << target.GetName()
              << "\" whose objects may not be installed" << reason << ".";
            this->SetError(e.str());
            return false;
          }

          objectGenerator =
            CreateInstallTargetGenerator(target, objectArgs, false);
        } else {
          // Installing an OBJECT library without a destination transforms
          // it to an INTERFACE library.  It installs no files but can be
          // exported.
        }
      } break;
      case cmStateEnums::EXECUTABLE: {
        if (target.IsAppBundleOnApple()) {
          // Application bundles use the BUNDLE properties.
          if (!bundleArgs.GetDestination().empty()) {
            bundleGenerator =
              CreateInstallTargetGenerator(target, bundleArgs, false);
          } else if (!runtimeArgs.GetDestination().empty()) {
            bool failure = false;
            if (this->CheckCMP0006(failure)) {
              // For CMake 2.4 compatibility fallback to the RUNTIME
              // properties.
              bundleGenerator =
                CreateInstallTargetGenerator(target, runtimeArgs, false);
            } else if (failure) {
              return false;
            }
          }
          if (!bundleGenerator) {
            std::ostringstream e;
            e << "TARGETS given no BUNDLE DESTINATION for MACOSX_BUNDLE "
                 "executable target \""
              << target.GetName() << "\".";
            this->SetError(e.str());
            return false;
          }
        } else {
          // Executables use the RUNTIME properties.
          if (!runtimeArgs.GetDestination().empty()) {
            runtimeGenerator =
              CreateInstallTargetGenerator(target, runtimeArgs, false);
          } else {
            std::ostringstream e;
            e << "TARGETS given no RUNTIME DESTINATION for executable "
                 "target \""
              << target.GetName() << "\".";
            this->SetError(e.str());
            return false;
          }
        }

        // On DLL platforms an executable may also have an import
        // library.  Install it to the archive destination if it
        // exists.
        if (dll_platform && !archiveArgs.GetDestination().empty() &&
            target.IsExecutableWithExports()) {
          // The import library uses the ARCHIVE properties.
          archiveGenerator =
            CreateInstallTargetGenerator(target, archiveArgs, true, true);
        }
      } break;
      case cmStateEnums::INTERFACE_LIBRARY:
        // Nothing to do. An INTERFACE_LIBRARY can be installed, but the
        // only effect of that is to make it exportable. It installs no
        // other files itself.
        break;
      default:
        // This should never happen due to the above type check.
        // Ignore the case.
        break;
    }

    // These well-known sets of files are installed *automatically* for
    // FRAMEWORK SHARED library targets on the Mac as part of installing the
    // FRAMEWORK.  For other target types or on other platforms, they are not
    // installed automatically and so we need to create install files
    // generators for them.
    bool createInstallGeneratorsForTargetFileSets = true;

    if (target.IsFrameworkOnApple() ||
        target.GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      createInstallGeneratorsForTargetFileSets = false;
    }

    if (createInstallGeneratorsForTargetFileSets && !namelinkOnly) {
      const char* files = target.GetProperty("PRIVATE_HEADER");
      if ((files) && (*files)) {
        std::vector<std::string> relFiles;
        cmSystemTools::ExpandListArgument(files, relFiles);
        std::vector<std::string> absFiles;
        if (!this->MakeFilesFullPath("PRIVATE_HEADER", relFiles, absFiles)) {
          return false;
        }

        // Create the files install generator.
        if (!privateHeaderArgs.GetDestination().empty()) {
          privateHeaderGenerator = CreateInstallFilesGenerator(
            this->Makefile, absFiles, privateHeaderArgs, false);
        } else {
          std::ostringstream e;
          e << "INSTALL TARGETS - target " << target.GetName() << " has "
            << "PRIVATE_HEADER files but no PRIVATE_HEADER DESTINATION.";
          cmSystemTools::Message(e.str().c_str(), "Warning");
        }
      }

      files = target.GetProperty("PUBLIC_HEADER");
      if ((files) && (*files)) {
        std::vector<std::string> relFiles;
        cmSystemTools::ExpandListArgument(files, relFiles);
        std::vector<std::string> absFiles;
        if (!this->MakeFilesFullPath("PUBLIC_HEADER", relFiles, absFiles)) {
          return false;
        }

        // Create the files install generator.
        if (!publicHeaderArgs.GetDestination().empty()) {
          publicHeaderGenerator = CreateInstallFilesGenerator(
            this->Makefile, absFiles, publicHeaderArgs, false);
        } else {
          std::ostringstream e;
          e << "INSTALL TARGETS - target " << target.GetName() << " has "
            << "PUBLIC_HEADER files but no PUBLIC_HEADER DESTINATION.";
          cmSystemTools::Message(e.str().c_str(), "Warning");
        }
      }

      files = target.GetProperty("RESOURCE");
      if ((files) && (*files)) {
        std::vector<std::string> relFiles;
        cmSystemTools::ExpandListArgument(files, relFiles);
        std::vector<std::string> absFiles;
        if (!this->MakeFilesFullPath("RESOURCE", relFiles, absFiles)) {
          return false;
        }

        // Create the files install generator.
        if (!resourceArgs.GetDestination().empty()) {
          resourceGenerator = CreateInstallFilesGenerator(
            this->Makefile, absFiles, resourceArgs, false);
        } else {
          std::ostringstream e;
          e << "INSTALL TARGETS - target " << target.GetName() << " has "
            << "RESOURCE files but no RESOURCE DESTINATION.";
          cmSystemTools::Message(e.str().c_str(), "Warning");
        }
      }
    }

    // Keep track of whether we're installing anything in each category
    installsArchive = installsArchive || archiveGenerator != nullptr;
    installsLibrary = installsLibrary || libraryGenerator != nullptr;
    installsNamelink = installsNamelink || namelinkGenerator != nullptr;
    installsRuntime = installsRuntime || runtimeGenerator != nullptr;
    installsObject = installsObject || objectGenerator != nullptr;
    installsFramework = installsFramework || frameworkGenerator != nullptr;
    installsBundle = installsBundle || bundleGenerator != nullptr;
    installsPrivateHeader =
      installsPrivateHeader || privateHeaderGenerator != nullptr;
    installsPublicHeader =
      installsPublicHeader || publicHeaderGenerator != nullptr;
    installsResource = installsResource || resourceGenerator;

    this->Makefile->AddInstallGenerator(archiveGenerator);
    this->Makefile->AddInstallGenerator(libraryGenerator);
    this->Makefile->AddInstallGenerator(namelinkGenerator);
    this->Makefile->AddInstallGenerator(runtimeGenerator);
    this->Makefile->AddInstallGenerator(objectGenerator);
    this->Makefile->AddInstallGenerator(frameworkGenerator);
    this->Makefile->AddInstallGenerator(bundleGenerator);
    this->Makefile->AddInstallGenerator(privateHeaderGenerator);
    this->Makefile->AddInstallGenerator(publicHeaderGenerator);
    this->Makefile->AddInstallGenerator(resourceGenerator);

    // Add this install rule to an export if one was specified and
    // this is not a namelink-only rule.
    if (!exports.GetString().empty() && !namelinkOnly) {
      cmTargetExport* te = new cmTargetExport;
      te->TargetName = target.GetName();
      te->ArchiveGenerator = archiveGenerator;
      te->BundleGenerator = bundleGenerator;
      te->FrameworkGenerator = frameworkGenerator;
      te->HeaderGenerator = publicHeaderGenerator;
      te->LibraryGenerator = libraryGenerator;
      te->RuntimeGenerator = runtimeGenerator;
      te->ObjectsGenerator = objectGenerator;
      this->Makefile->GetGlobalGenerator()
        ->GetExportSets()[exports.GetString()]
        ->AddTargetExport(te);

      te->InterfaceIncludeDirectories =
        cmJoin(includesArgs.GetIncludeDirs(), ";");
    }
  }

  // Tell the global generator about any installation component names
  // specified
  if (installsArchive) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      archiveArgs.GetComponent());
  }
  if (installsLibrary) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      libraryArgs.GetComponent());
  }
  if (installsNamelink) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      libraryArgs.GetNamelinkComponent());
  }
  if (installsRuntime) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      runtimeArgs.GetComponent());
  }
  if (installsObject) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      objectArgs.GetComponent());
  }
  if (installsFramework) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      frameworkArgs.GetComponent());
  }
  if (installsBundle) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      bundleArgs.GetComponent());
  }
  if (installsPrivateHeader) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      privateHeaderArgs.GetComponent());
  }
  if (installsPublicHeader) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      publicHeaderArgs.GetComponent());
  }
  if (installsResource) {
    this->Makefile->GetGlobalGenerator()->AddInstallComponent(
      resourceArgs.GetComponent());
  }

  return true;
}

bool cmInstallCommand::HandleFilesMode(std::vector<std::string> const& args)
{
  // This is the FILES mode.
  bool programs = (args[0] == "PROGRAMS");
  cmInstallCommandArguments ica(this->DefaultComponentName);
  cmCAStringVector files(&ica.Parser, programs ? "PROGRAMS" : "FILES");
  files.Follows(nullptr);
  ica.ArgumentGroup.Follows(&files);
  std::vector<std::string> unknownArgs;
  ica.Parse(&args, &unknownArgs);

  if (!unknownArgs.empty()) {
    // Unknown argument.
    std::ostringstream e;
    e << args[0] << " given unknown argument \"" << unknownArgs[0] << "\".";
    this->SetError(e.str());
    return false;
  }

  const std::vector<std::string>& filesVector = files.GetVector();

  // Check if there is something to do.
  if (filesVector.empty()) {
    return true;
  }

  if (!ica.GetRename().empty() && filesVector.size() > 1) {
    // The rename option works only with one file.
    std::ostringstream e;
    e << args[0] << " given RENAME option with more than one file.";
    this->SetError(e.str());
    return false;
  }

  std::vector<std::string> absFiles;
  if (!this->MakeFilesFullPath(args[0].c_str(), filesVector, absFiles)) {
    return false;
  }

  cmPolicies::PolicyStatus status =
    this->Makefile->GetPolicyStatus(cmPolicies::CMP0062);

  cmGlobalGenerator* gg = this->Makefile->GetGlobalGenerator();
  for (std::string const& file : filesVector) {
    if (gg->IsExportedTargetsFile(file)) {
      const char* modal = nullptr;
      std::ostringstream e;
      cmake::MessageType messageType = cmake::AUTHOR_WARNING;

      switch (status) {
        case cmPolicies::WARN:
          e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0062) << "\n";
          modal = "should";
        case cmPolicies::OLD:
          break;
        case cmPolicies::REQUIRED_IF_USED:
        case cmPolicies::REQUIRED_ALWAYS:
        case cmPolicies::NEW:
          modal = "may";
          messageType = cmake::FATAL_ERROR;
      }
      if (modal) {
        e << "The file\n  " << file
          << "\nwas generated by the export() "
             "command.  It "
          << modal
          << " not be installed with the "
             "install() command.  Use the install(EXPORT) mechanism "
             "instead.  See the cmake-packages(7) manual for more.\n";
        this->Makefile->IssueMessage(messageType, e.str());
        if (messageType == cmake::FATAL_ERROR) {
          return false;
        }
      }
    }
  }

  if (!ica.Finalize()) {
    return false;
  }

  if (ica.GetDestination().empty()) {
    // A destination is required.
    std::ostringstream e;
    e << args[0] << " given no DESTINATION!";
    this->SetError(e.str());
    return false;
  }

  // Create the files install generator.
  this->Makefile->AddInstallGenerator(
    CreateInstallFilesGenerator(this->Makefile, absFiles, ica, programs));

  // Tell the global generator about any installation component names
  // specified.
  this->Makefile->GetGlobalGenerator()->AddInstallComponent(
    ica.GetComponent());

  return true;
}

bool cmInstallCommand::HandleDirectoryMode(
  std::vector<std::string> const& args)
{
  enum Doing
  {
    DoingNone,
    DoingDirs,
    DoingDestination,
    DoingPattern,
    DoingRegex,
    DoingPermsFile,
    DoingPermsDir,
    DoingPermsMatch,
    DoingConfigurations,
    DoingComponent
  };
  Doing doing = DoingDirs;
  bool in_match_mode = false;
  bool optional = false;
  bool exclude_from_all = false;
  bool message_never = false;
  std::vector<std::string> dirs;
  const char* destination = nullptr;
  std::string permissions_file;
  std::string permissions_dir;
  std::vector<std::string> configurations;
  std::string component = this->DefaultComponentName;
  std::string literal_args;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "DESTINATION") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the destination property.
      doing = DoingDestination;
    } else if (args[i] == "OPTIONAL") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Mark the rule as optional.
      optional = true;
      doing = DoingNone;
    } else if (args[i] == "MESSAGE_NEVER") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Mark the rule as quiet.
      message_never = true;
      doing = DoingNone;
    } else if (args[i] == "PATTERN") {
      // Switch to a new pattern match rule.
      doing = DoingPattern;
      in_match_mode = true;
    } else if (args[i] == "REGEX") {
      // Switch to a new regex match rule.
      doing = DoingRegex;
      in_match_mode = true;
    } else if (args[i] == "EXCLUDE") {
      // Add this property to the current match rule.
      if (!in_match_mode || doing == DoingPattern || doing == DoingRegex) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" before a PATTERN or REGEX is given.";
        this->SetError(e.str());
        return false;
      }
      literal_args += " EXCLUDE";
      doing = DoingNone;
    } else if (args[i] == "PERMISSIONS") {
      if (!in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" before a PATTERN or REGEX is given.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the current match permissions property.
      literal_args += " PERMISSIONS";
      doing = DoingPermsMatch;
    } else if (args[i] == "FILE_PERMISSIONS") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the file permissions property.
      doing = DoingPermsFile;
    } else if (args[i] == "DIRECTORY_PERMISSIONS") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the directory permissions property.
      doing = DoingPermsDir;
    } else if (args[i] == "USE_SOURCE_PERMISSIONS") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Add this option literally.
      literal_args += " USE_SOURCE_PERMISSIONS";
      doing = DoingNone;
    } else if (args[i] == "FILES_MATCHING") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Add this option literally.
      literal_args += " FILES_MATCHING";
      doing = DoingNone;
    } else if (args[i] == "CONFIGURATIONS") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the configurations property.
      doing = DoingConfigurations;
    } else if (args[i] == "COMPONENT") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }

      // Switch to setting the component property.
      doing = DoingComponent;
    } else if (args[i] == "EXCLUDE_FROM_ALL") {
      if (in_match_mode) {
        std::ostringstream e;
        e << args[0] << " does not allow \"" << args[i]
          << "\" after PATTERN or REGEX.";
        this->SetError(e.str());
        return false;
      }
      exclude_from_all = true;
      doing = DoingNone;
    } else if (doing == DoingDirs) {
      // Convert this directory to a full path.
      std::string dir = args[i];
      std::string::size_type gpos = cmGeneratorExpression::Find(dir);
      if (gpos != 0 && !cmSystemTools::FileIsFullPath(dir)) {
        dir = this->Makefile->GetCurrentSourceDirectory();
        dir += "/";
        dir += args[i];
      }

      // Make sure the name is a directory.
      if (cmSystemTools::FileExists(dir) &&
          !cmSystemTools::FileIsDirectory(dir)) {
        std::ostringstream e;
        e << args[0] << " given non-directory \"" << args[i]
          << "\" to install.";
        this->SetError(e.str());
        return false;
      }

      // Store the directory for installation.
      dirs.push_back(std::move(dir));
    } else if (doing == DoingConfigurations) {
      configurations.push_back(args[i]);
    } else if (doing == DoingDestination) {
      destination = args[i].c_str();
      doing = DoingNone;
    } else if (doing == DoingPattern) {
      // Convert the pattern to a regular expression.  Require a
      // leading slash and trailing end-of-string in the matched
      // string to make sure the pattern matches only whole file
      // names.
      literal_args += " REGEX \"/";
      std::string regex = cmsys::Glob::PatternToRegex(args[i], false);
      cmSystemTools::ReplaceString(regex, "\\", "\\\\");
      literal_args += regex;
      literal_args += "$\"";
      doing = DoingNone;
    } else if (doing == DoingRegex) {
      literal_args += " REGEX \"";
// Match rules are case-insensitive on some platforms.
#if defined(_WIN32) || defined(__APPLE__) || defined(__CYGWIN__)
      std::string regex = cmSystemTools::LowerCase(args[i]);
#else
      std::string regex = args[i];
#endif
      cmSystemTools::ReplaceString(regex, "\\", "\\\\");
      literal_args += regex;
      literal_args += "\"";
      doing = DoingNone;
    } else if (doing == DoingComponent) {
      component = args[i];
      doing = DoingNone;
    } else if (doing == DoingPermsFile) {
      // Check the requested permission.
      if (!cmInstallCommandArguments::CheckPermissions(args[i],
                                                       permissions_file)) {
        std::ostringstream e;
        e << args[0] << " given invalid file permission \"" << args[i]
          << "\".";
        this->SetError(e.str());
        return false;
      }
    } else if (doing == DoingPermsDir) {
      // Check the requested permission.
      if (!cmInstallCommandArguments::CheckPermissions(args[i],
                                                       permissions_dir)) {
        std::ostringstream e;
        e << args[0] << " given invalid directory permission \"" << args[i]
          << "\".";
        this->SetError(e.str());
        return false;
      }
    } else if (doing == DoingPermsMatch) {
      // Check the requested permission.
      if (!cmInstallCommandArguments::CheckPermissions(args[i],
                                                       literal_args)) {
        std::ostringstream e;
        e << args[0] << " given invalid permission \"" << args[i] << "\".";
        this->SetError(e.str());
        return false;
      }
    } else {
      // Unknown argument.
      std::ostringstream e;
      e << args[0] << " given unknown argument \"" << args[i] << "\".";
      this->SetError(e.str());
      return false;
    }
  }

  // Support installing an empty directory.
  if (dirs.empty() && destination) {
    dirs.emplace_back();
  }

  // Check if there is something to do.
  if (dirs.empty()) {
    return true;
  }
  if (!destination) {
    // A destination is required.
    std::ostringstream e;
    e << args[0] << " given no DESTINATION!";
    this->SetError(e.str());
    return false;
  }

  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(this->Makefile, message_never);

  // Create the directory install generator.
  this->Makefile->AddInstallGenerator(new cmInstallDirectoryGenerator(
    dirs, destination, permissions_file.c_str(), permissions_dir.c_str(),
    configurations, component.c_str(), message, exclude_from_all,
    literal_args.c_str(), optional));

  // Tell the global generator about any installation component names
  // specified.
  this->Makefile->GetGlobalGenerator()->AddInstallComponent(component);

  return true;
}

bool cmInstallCommand::HandleExportAndroidMKMode(
  std::vector<std::string> const& args)
{
#ifdef CMAKE_BUILD_WITH_CMAKE
  // This is the EXPORT mode.
  cmInstallCommandArguments ica(this->DefaultComponentName);
  cmCAString exp(&ica.Parser, "EXPORT_ANDROID_MK");
  cmCAString name_space(&ica.Parser, "NAMESPACE", &ica.ArgumentGroup);
  cmCAEnabler exportOld(&ica.Parser, "EXPORT_LINK_INTERFACE_LIBRARIES",
                        &ica.ArgumentGroup);
  cmCAString filename(&ica.Parser, "FILE", &ica.ArgumentGroup);
  exp.Follows(nullptr);

  ica.ArgumentGroup.Follows(&exp);
  std::vector<std::string> unknownArgs;
  ica.Parse(&args, &unknownArgs);

  if (!unknownArgs.empty()) {
    // Unknown argument.
    std::ostringstream e;
    e << args[0] << " given unknown argument \"" << unknownArgs[0] << "\".";
    this->SetError(e.str());
    return false;
  }

  if (!ica.Finalize()) {
    return false;
  }

  // Make sure there is a destination.
  if (ica.GetDestination().empty()) {
    // A destination is required.
    std::ostringstream e;
    e << args[0] << " given no DESTINATION!";
    this->SetError(e.str());
    return false;
  }

  // Check the file name.
  std::string fname = filename.GetString();
  if (fname.find_first_of(":/\\") != std::string::npos) {
    std::ostringstream e;
    e << args[0] << " given invalid export file name \"" << fname << "\".  "
      << "The FILE argument may not contain a path.  "
      << "Specify the path in the DESTINATION argument.";
    this->SetError(e.str());
    return false;
  }

  // Check the file extension.
  if (!fname.empty() &&
      cmSystemTools::GetFilenameLastExtension(fname) != ".mk") {
    std::ostringstream e;
    e << args[0] << " given invalid export file name \"" << fname << "\".  "
      << "The FILE argument must specify a name ending in \".mk\".";
    this->SetError(e.str());
    return false;
  }
  if (fname.find_first_of(":/\\") != std::string::npos) {
    std::ostringstream e;
    e << args[0] << " given export name \"" << exp.GetString() << "\".  "
      << "This name cannot be safely converted to a file name.  "
      << "Specify a different export name or use the FILE option to set "
      << "a file name explicitly.";
    this->SetError(e.str());
    return false;
  }
  // Use the default name
  if (fname.empty()) {
    fname = "Android.mk";
  }

  cmExportSet* exportSet =
    this->Makefile->GetGlobalGenerator()->GetExportSets()[exp.GetString()];

  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(this->Makefile);

  // Create the export install generator.
  cmInstallExportGenerator* exportGenerator = new cmInstallExportGenerator(
    exportSet, ica.GetDestination().c_str(), ica.GetPermissions().c_str(),
    ica.GetConfigurations(), ica.GetComponent().c_str(), message,
    ica.GetExcludeFromAll(), fname.c_str(), name_space.GetCString(),
    exportOld.IsEnabled(), true);
  this->Makefile->AddInstallGenerator(exportGenerator);

  return true;
#else
  static_cast<void>(args);
  this->SetError("EXPORT_ANDROID_MK not supported in bootstrap cmake");
  return false;
#endif
}

bool cmInstallCommand::HandleExportMode(std::vector<std::string> const& args)
{
  // This is the EXPORT mode.
  cmInstallCommandArguments ica(this->DefaultComponentName);
  cmCAString exp(&ica.Parser, "EXPORT");
  cmCAString name_space(&ica.Parser, "NAMESPACE", &ica.ArgumentGroup);
  cmCAEnabler exportOld(&ica.Parser, "EXPORT_LINK_INTERFACE_LIBRARIES",
                        &ica.ArgumentGroup);
  cmCAString filename(&ica.Parser, "FILE", &ica.ArgumentGroup);
  exp.Follows(nullptr);

  ica.ArgumentGroup.Follows(&exp);
  std::vector<std::string> unknownArgs;
  ica.Parse(&args, &unknownArgs);

  if (!unknownArgs.empty()) {
    // Unknown argument.
    std::ostringstream e;
    e << args[0] << " given unknown argument \"" << unknownArgs[0] << "\".";
    this->SetError(e.str());
    return false;
  }

  if (!ica.Finalize()) {
    return false;
  }

  // Make sure there is a destination.
  if (ica.GetDestination().empty()) {
    // A destination is required.
    std::ostringstream e;
    e << args[0] << " given no DESTINATION!";
    this->SetError(e.str());
    return false;
  }

  // Check the file name.
  std::string fname = filename.GetString();
  if (fname.find_first_of(":/\\") != std::string::npos) {
    std::ostringstream e;
    e << args[0] << " given invalid export file name \"" << fname << "\".  "
      << "The FILE argument may not contain a path.  "
      << "Specify the path in the DESTINATION argument.";
    this->SetError(e.str());
    return false;
  }

  // Check the file extension.
  if (!fname.empty() &&
      cmSystemTools::GetFilenameLastExtension(fname) != ".cmake") {
    std::ostringstream e;
    e << args[0] << " given invalid export file name \"" << fname << "\".  "
      << "The FILE argument must specify a name ending in \".cmake\".";
    this->SetError(e.str());
    return false;
  }

  // Construct the file name.
  if (fname.empty()) {
    fname = exp.GetString();
    fname += ".cmake";

    if (fname.find_first_of(":/\\") != std::string::npos) {
      std::ostringstream e;
      e << args[0] << " given export name \"" << exp.GetString() << "\".  "
        << "This name cannot be safely converted to a file name.  "
        << "Specify a different export name or use the FILE option to set "
        << "a file name explicitly.";
      this->SetError(e.str());
      return false;
    }
  }

  cmExportSet* exportSet =
    this->Makefile->GetGlobalGenerator()->GetExportSets()[exp.GetString()];
  if (exportOld.IsEnabled()) {
    for (cmTargetExport* te : *exportSet->GetTargetExports()) {
      cmTarget* tgt =
        this->Makefile->GetGlobalGenerator()->FindTarget(te->TargetName);
      const bool newCMP0022Behavior =
        (tgt && tgt->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
         tgt->GetPolicyStatusCMP0022() != cmPolicies::OLD);

      if (!newCMP0022Behavior) {
        std::ostringstream e;
        e << "INSTALL(EXPORT) given keyword \""
          << "EXPORT_LINK_INTERFACE_LIBRARIES"
          << "\", but target \"" << te->TargetName
          << "\" does not have policy CMP0022 set to NEW.";
        this->SetError(e.str());
        return false;
      }
    }
  }

  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(this->Makefile);

  // Create the export install generator.
  cmInstallExportGenerator* exportGenerator = new cmInstallExportGenerator(
    exportSet, ica.GetDestination().c_str(), ica.GetPermissions().c_str(),
    ica.GetConfigurations(), ica.GetComponent().c_str(), message,
    ica.GetExcludeFromAll(), fname.c_str(), name_space.GetCString(),
    exportOld.IsEnabled(), false);
  this->Makefile->AddInstallGenerator(exportGenerator);

  return true;
}

bool cmInstallCommand::MakeFilesFullPath(
  const char* modeName, const std::vector<std::string>& relFiles,
  std::vector<std::string>& absFiles)
{
  for (std::string const& relFile : relFiles) {
    std::string file = relFile;
    std::string::size_type gpos = cmGeneratorExpression::Find(file);
    if (gpos != 0 && !cmSystemTools::FileIsFullPath(file)) {
      file = this->Makefile->GetCurrentSourceDirectory();
      file += "/";
      file += relFile;
    }

    // Make sure the file is not a directory.
    if (gpos == std::string::npos && cmSystemTools::FileIsDirectory(file)) {
      std::ostringstream e;
      e << modeName << " given directory \"" << relFile << "\" to install.";
      this->SetError(e.str());
      return false;
    }
    // Store the file for installation.
    absFiles.push_back(std::move(file));
  }
  return true;
}

bool cmInstallCommand::CheckCMP0006(bool& failure)
{
  switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0006)) {
    case cmPolicies::WARN:
      this->Makefile->IssueMessage(
        cmake::AUTHOR_WARNING,
        cmPolicies::GetPolicyWarning(cmPolicies::CMP0006));
      CM_FALLTHROUGH;
    case cmPolicies::OLD:
      // OLD behavior is to allow compatibility
      return true;
    case cmPolicies::NEW:
      // NEW behavior is to disallow compatibility
      break;
    case cmPolicies::REQUIRED_IF_USED:
    case cmPolicies::REQUIRED_ALWAYS:
      failure = true;
      this->Makefile->IssueMessage(
        cmake::FATAL_ERROR,
        cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0006));
      break;
  }
  return false;
}
