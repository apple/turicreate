/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGenInitializer.h"
#include "cmQtAutoGen.h"

#include "cmAlgorithms.h"
#include "cmCustomCommand.h"
#include "cmCustomCommandLines.h"
#include "cmDuration.h"
#include "cmFilePathChecksum.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLinkItem.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmPolicies.h"
#include "cmProcessOutput.h"
#include "cmSourceFile.h"
#include "cmSourceGroup.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"
#include "cmsys/FStream.hxx"
#include "cmsys/SystemInformation.hxx"

#include <algorithm>
#include <array>
#include <deque>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static std::size_t GetParallelCPUCount()
{
  static std::size_t count = 0;
  // Detect only on the first call
  if (count == 0) {
    cmsys::SystemInformation info;
    info.RunCPUCheck();
    count = info.GetNumberOfPhysicalCPU();
    count = std::max<std::size_t>(count, 1);
    count = std::min<std::size_t>(count, cmQtAutoGen::ParallelMax);
  }
  return count;
}

static bool AddToSourceGroup(cmMakefile* makefile, std::string const& fileName,
                             cmQtAutoGen::GeneratorT genType)
{
  cmSourceGroup* sourceGroup = nullptr;
  // Acquire source group
  {
    std::string property;
    std::string groupName;
    {
      std::array<std::string, 2> props;
      // Use generator specific group name
      switch (genType) {
        case cmQtAutoGen::GeneratorT::MOC:
          props[0] = "AUTOMOC_SOURCE_GROUP";
          break;
        case cmQtAutoGen::GeneratorT::RCC:
          props[0] = "AUTORCC_SOURCE_GROUP";
          break;
        default:
          props[0] = "AUTOGEN_SOURCE_GROUP";
          break;
      }
      props[1] = "AUTOGEN_SOURCE_GROUP";
      for (std::string& prop : props) {
        const char* propName = makefile->GetState()->GetGlobalProperty(prop);
        if ((propName != nullptr) && (*propName != '\0')) {
          groupName = propName;
          property = std::move(prop);
          break;
        }
      }
    }
    // Generate a source group on demand
    if (!groupName.empty()) {
      sourceGroup = makefile->GetOrCreateSourceGroup(groupName);
      if (sourceGroup == nullptr) {
        std::ostringstream ost;
        ost << cmQtAutoGen::GeneratorNameUpper(genType);
        ost << ": " << property;
        ost << ": Could not find or create the source group ";
        ost << cmQtAutoGen::Quoted(groupName);
        cmSystemTools::Error(ost.str().c_str());
        return false;
      }
    }
  }
  if (sourceGroup != nullptr) {
    sourceGroup->AddGroupFile(fileName);
  }
  return true;
}

static void AddCleanFile(cmMakefile* makefile, std::string const& fileName)
{
  makefile->AppendProperty("ADDITIONAL_MAKE_CLEAN_FILES", fileName.c_str(),
                           false);
}

static std::string FileProjectRelativePath(cmMakefile* makefile,
                                           std::string const& fileName)
{
  std::string res;
  {
    std::string pSource = cmSystemTools::RelativePath(
      makefile->GetCurrentSourceDirectory(), fileName);
    std::string pBinary = cmSystemTools::RelativePath(
      makefile->GetCurrentBinaryDirectory(), fileName);
    if (pSource.size() < pBinary.size()) {
      res = std::move(pSource);
    } else if (pBinary.size() < fileName.size()) {
      res = std::move(pBinary);
    } else {
      res = fileName;
    }
  }
  return res;
}

/* @brief Tests if targetDepend is a STATIC_LIBRARY and if any of its
 * recursive STATIC_LIBRARY dependencies depends on targetOrigin
 * (STATIC_LIBRARY cycle).
 */
static bool StaticLibraryCycle(cmGeneratorTarget const* targetOrigin,
                               cmGeneratorTarget const* targetDepend,
                               std::string const& config)
{
  bool cycle = false;
  if ((targetOrigin->GetType() == cmStateEnums::STATIC_LIBRARY) &&
      (targetDepend->GetType() == cmStateEnums::STATIC_LIBRARY)) {
    std::set<cmGeneratorTarget const*> knownLibs;
    std::deque<cmGeneratorTarget const*> testLibs;

    // Insert initial static_library dependency
    knownLibs.insert(targetDepend);
    testLibs.push_back(targetDepend);

    while (!testLibs.empty()) {
      cmGeneratorTarget const* testTarget = testLibs.front();
      testLibs.pop_front();
      // Check if the test target is the origin target (cycle)
      if (testTarget == targetOrigin) {
        cycle = true;
        break;
      }
      // Collect all static_library dependencies from the test target
      cmLinkImplementationLibraries const* libs =
        testTarget->GetLinkImplementationLibraries(config);
      if (libs != nullptr) {
        for (cmLinkItem const& item : libs->Libraries) {
          cmGeneratorTarget const* depTarget = item.Target;
          if ((depTarget != nullptr) &&
              (depTarget->GetType() == cmStateEnums::STATIC_LIBRARY) &&
              knownLibs.insert(depTarget).second) {
            testLibs.push_back(depTarget);
          }
        }
      }
    }
  }
  return cycle;
}

cmQtAutoGenInitializer::cmQtAutoGenInitializer(cmGeneratorTarget* target,
                                               bool mocEnabled,
                                               bool uicEnabled,
                                               bool rccEnabled,
                                               IntegerVersion const& qtVersion)
  : Target(target)
  , QtVersion(qtVersion)
{
  Moc.Enabled = mocEnabled;
  Uic.Enabled = uicEnabled;
  Rcc.Enabled = rccEnabled;
}

bool cmQtAutoGenInitializer::InitCustomTargets()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();
  cmLocalGenerator* localGen = this->Target->GetLocalGenerator();
  cmGlobalGenerator* globalGen = localGen->GetGlobalGenerator();

  // Configurations
  this->MultiConfig = globalGen->IsMultiConfig();
  this->ConfigDefault = makefile->GetConfigurations(this->ConfigsList);
  if (this->ConfigsList.empty()) {
    this->ConfigsList.push_back(this->ConfigDefault);
  }

  // Verbosity
  this->Verbosity = makefile->GetSafeDefinition("CMAKE_AUTOGEN_VERBOSE");
  if (!this->Verbosity.empty()) {
    unsigned long iVerb = 0;
    if (!cmSystemTools::StringToULong(this->Verbosity.c_str(), &iVerb)) {
      // Non numeric verbosity
      this->Verbosity = cmSystemTools::IsOn(this->Verbosity) ? "1" : "0";
    }
  }

  // Targets FOLDER
  {
    const char* folder =
      makefile->GetState()->GetGlobalProperty("AUTOMOC_TARGETS_FOLDER");
    if (folder == nullptr) {
      folder =
        makefile->GetState()->GetGlobalProperty("AUTOGEN_TARGETS_FOLDER");
    }
    // Inherit FOLDER property from target (#13688)
    if (folder == nullptr) {
      folder = this->Target->GetProperty("FOLDER");
    }
    if (folder != nullptr) {
      this->TargetsFolder = folder;
    }
  }

  // Common directories
  {
    // Collapsed current binary directory
    std::string const cbd = cmSystemTools::CollapseFullPath(
      std::string(), makefile->GetCurrentBinaryDirectory());

    // Info directory
    this->Dir.Info = cbd;
    this->Dir.Info += makefile->GetCMakeInstance()->GetCMakeFilesDirectory();
    this->Dir.Info += '/';
    this->Dir.Info += this->Target->GetName();
    this->Dir.Info += "_autogen";
    this->Dir.Info += ".dir";
    cmSystemTools::ConvertToUnixSlashes(this->Dir.Info);

    // Build directory
    this->Dir.Build = this->Target->GetSafeProperty("AUTOGEN_BUILD_DIR");
    if (this->Dir.Build.empty()) {
      this->Dir.Build = cbd;
      this->Dir.Build += '/';
      this->Dir.Build += this->Target->GetName();
      this->Dir.Build += "_autogen";
    }
    cmSystemTools::ConvertToUnixSlashes(this->Dir.Build);
    // Cleanup build directory
    AddCleanFile(makefile, this->Dir.Build);

    // Working directory
    this->Dir.Work = cbd;
    cmSystemTools::ConvertToUnixSlashes(this->Dir.Work);

    // Include directory
    this->Dir.Include = this->Dir.Build;
    this->Dir.Include += "/include";
    if (this->MultiConfig) {
      this->Dir.Include += "_$<CONFIG>";
    }
    // Per config include directories
    if (this->MultiConfig) {
      for (std::string const& cfg : this->ConfigsList) {
        std::string& dir = this->Dir.ConfigInclude[cfg];
        dir = this->Dir.Build;
        dir += "/include_";
        dir += cfg;
      }
    }
  }

  // Moc, Uic and _autogen target settings
  if (this->Moc.Enabled || this->Uic.Enabled) {
    // Init moc specific settings
    if (this->Moc.Enabled && !InitMoc()) {
      return false;
    }

    // Init uic specific settings
    if (this->Uic.Enabled && !InitUic()) {
      return false;
    }

    // Autogen target name
    this->AutogenTarget.Name = this->Target->GetName();
    this->AutogenTarget.Name += "_autogen";

    // Autogen target parallel processing
    this->AutogenTarget.Parallel =
      this->Target->GetSafeProperty("AUTOGEN_PARALLEL");
    if (this->AutogenTarget.Parallel.empty() ||
        (this->AutogenTarget.Parallel == "AUTO")) {
      // Autodetect number of CPUs
      this->AutogenTarget.Parallel = std::to_string(GetParallelCPUCount());
    }

    // Autogen target info and settings files
    {
      this->AutogenTarget.InfoFile = this->Dir.Info;
      this->AutogenTarget.InfoFile += "/AutogenInfo.cmake";

      this->AutogenTarget.SettingsFile = this->Dir.Info;
      this->AutogenTarget.SettingsFile += "/AutogenOldSettings.txt";

      if (this->MultiConfig) {
        for (std::string const& cfg : this->ConfigsList) {
          std::string& filename = this->AutogenTarget.ConfigSettingsFile[cfg];
          filename =
            AppendFilenameSuffix(this->AutogenTarget.SettingsFile, "_" + cfg);
          AddCleanFile(makefile, filename);
        }
      } else {
        AddCleanFile(makefile, this->AutogenTarget.SettingsFile);
      }
    }

    // Autogen target: Compute user defined dependencies
    {
      std::string const deps =
        this->Target->GetSafeProperty("AUTOGEN_TARGET_DEPENDS");
      if (!deps.empty()) {
        std::vector<std::string> extraDeps;
        cmSystemTools::ExpandListArgument(deps, extraDeps);
        for (std::string const& depName : extraDeps) {
          // Allow target and file dependencies
          auto* depTarget = makefile->FindTargetToUse(depName);
          if (depTarget != nullptr) {
            this->AutogenTarget.DependTargets.insert(depTarget);
          } else {
            this->AutogenTarget.DependFiles.insert(depName);
          }
        }
      }
    }
  }

  // Init rcc specific settings
  if (this->Rcc.Enabled && !InitRcc()) {
    return false;
  }

  // Add autogen include directory to the origin target INCLUDE_DIRECTORIES
  if (this->Moc.Enabled || this->Uic.Enabled ||
      (this->Rcc.Enabled && this->MultiConfig)) {
    this->Target->AddIncludeDirectory(this->Dir.Include, true);
  }

  // Scan files
  if (!this->InitScanFiles()) {
    return false;
  }

  // Create autogen target
  if ((this->Moc.Enabled || this->Uic.Enabled) && !this->InitAutogenTarget()) {
    return false;
  }

  // Create rcc targets
  if (this->Rcc.Enabled && !this->InitRccTargets()) {
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::InitMoc()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();
  cmLocalGenerator* localGen = this->Target->GetLocalGenerator();

  // Mocs compilation file
  this->Moc.MocsCompilation = this->Dir.Build;
  this->Moc.MocsCompilation += "/mocs_compilation.cpp";

  // Moc predefs command
  if (this->Target->GetPropertyAsBool("AUTOMOC_COMPILER_PREDEFINES") &&
      (this->QtVersion >= IntegerVersion(5, 8))) {
    this->Moc.PredefsCmd =
      makefile->GetSafeDefinition("CMAKE_CXX_COMPILER_PREDEFINES_COMMAND");
  }

  // Moc includes
  {
    // We need to disable this until we have all implicit includes available.
    // See issue #18669.
    // bool const appendImplicit = (this->QtVersion.Major == 5);

    auto GetIncludeDirs = [this,
                           localGen](std::string const& cfg) -> std::string {
      bool const appendImplicit = false;
      // Get the include dirs for this target, without stripping the implicit
      // include dirs off, see
      // https://gitlab.kitware.com/cmake/cmake/issues/13667
      std::vector<std::string> dirs;
      localGen->GetIncludeDirectories(dirs, this->Target, "CXX", cfg, false,
                                      appendImplicit);
      return cmJoin(dirs, ";");
    };

    // Default configuration include directories
    this->Moc.Includes = GetIncludeDirs(this->ConfigDefault);
    // Other configuration settings
    if (this->MultiConfig) {
      for (std::string const& cfg : this->ConfigsList) {
        std::string dirs = GetIncludeDirs(cfg);
        if (dirs != this->Moc.Includes) {
          this->Moc.ConfigIncludes[cfg] = std::move(dirs);
        }
      }
    }
  }

  // Moc compile definitions
  {
    auto GetCompileDefinitions =
      [this, localGen](std::string const& cfg) -> std::string {
      std::set<std::string> defines;
      localGen->AddCompileDefinitions(defines, this->Target, cfg, "CXX");
      return cmJoin(defines, ";");
    };

    // Default configuration defines
    this->Moc.Defines = GetCompileDefinitions(this->ConfigDefault);
    // Other configuration defines
    if (this->MultiConfig) {
      for (std::string const& cfg : this->ConfigsList) {
        std::string defines = GetCompileDefinitions(cfg);
        if (defines != this->Moc.Defines) {
          this->Moc.ConfigDefines[cfg] = std::move(defines);
        }
      }
    }
  }

  // Moc executable
  if (!GetMocExecutable()) {
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::InitUic()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();

  // Uic search paths
  {
    std::string const usp =
      this->Target->GetSafeProperty("AUTOUIC_SEARCH_PATHS");
    if (!usp.empty()) {
      cmSystemTools::ExpandListArgument(usp, this->Uic.SearchPaths);
      std::string const& srcDir = makefile->GetCurrentSourceDirectory();
      for (std::string& path : this->Uic.SearchPaths) {
        path = cmSystemTools::CollapseFullPath(path, srcDir);
      }
    }
  }
  // Uic target options
  {
    auto UicGetOpts = [this](std::string const& cfg) -> std::string {
      std::vector<std::string> opts;
      this->Target->GetAutoUicOptions(opts, cfg);
      return cmJoin(opts, ";");
    };

    // Default settings
    this->Uic.Options = UicGetOpts(this->ConfigDefault);

    // Configuration specific settings
    if (this->MultiConfig) {
      for (std::string const& cfg : this->ConfigsList) {
        std::string options = UicGetOpts(cfg);
        if (options != this->Uic.Options) {
          this->Uic.ConfigOptions[cfg] = std::move(options);
        }
      }
    }
  }
  // .ui files skip and options
  {
    std::string const uiExt = "ui";
    std::string pathError;
    for (cmSourceFile* sf : makefile->GetSourceFiles()) {
      // sf->GetExtension() is only valid after sf->GetFullPath() ...
      // Since we're iterating over source files that might be not in the
      // target we need to check for path errors (not existing files).
      std::string const& fPath = sf->GetFullPath(&pathError);
      if (!pathError.empty()) {
        pathError.clear();
        continue;
      }
      if (sf->GetExtension() == uiExt) {
        std::string const absFile = cmSystemTools::GetRealPath(fPath);
        // Check if the .ui file should be skipped
        if (sf->GetPropertyAsBool("SKIP_AUTOUIC") ||
            sf->GetPropertyAsBool("SKIP_AUTOGEN")) {
          this->Uic.Skip.insert(absFile);
        }
        // Check if the .ui file has uic options
        std::string const uicOpts = sf->GetSafeProperty("AUTOUIC_OPTIONS");
        if (!uicOpts.empty()) {
          // Check if file isn't skipped
          if (this->Uic.Skip.count(absFile) == 0) {
            this->Uic.FileFiles.push_back(absFile);
            std::vector<std::string> optsVec;
            cmSystemTools::ExpandListArgument(uicOpts, optsVec);
            this->Uic.FileOptions.push_back(std::move(optsVec));
          }
        }
      }
    }
  }

  // Uic executable
  if (!GetUicExecutable()) {
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::InitRcc()
{
  if (!GetRccExecutable()) {
    return false;
  }
  return true;
}

bool cmQtAutoGenInitializer::InitScanFiles()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();

  // Scan through target files
  {
    std::string const qrcExt = "qrc";
    std::vector<cmSourceFile*> srcFiles;
    this->Target->GetConfigCommonSourceFiles(srcFiles);
    for (cmSourceFile* sf : srcFiles) {
      if (sf->GetPropertyAsBool("SKIP_AUTOGEN")) {
        continue;
      }
      // sf->GetExtension() is only valid after sf->GetFullPath() ...
      std::string const& fPath = sf->GetFullPath();
      std::string const& ext = sf->GetExtension();
      // Register generated files that will be scanned by moc or uic
      if (this->Moc.Enabled || this->Uic.Enabled) {
        cmSystemTools::FileFormat const fileType =
          cmSystemTools::GetFileFormat(ext.c_str());
        if ((fileType == cmSystemTools::CXX_FILE_FORMAT) ||
            (fileType == cmSystemTools::HEADER_FILE_FORMAT)) {
          std::string const absPath = cmSystemTools::GetRealPath(fPath);
          if ((this->Moc.Enabled && !sf->GetPropertyAsBool("SKIP_AUTOMOC")) ||
              (this->Uic.Enabled && !sf->GetPropertyAsBool("SKIP_AUTOUIC"))) {
            // Register source
            const bool generated = sf->GetPropertyAsBool("GENERATED");
            if (fileType == cmSystemTools::HEADER_FILE_FORMAT) {
              if (generated) {
                this->AutogenTarget.HeadersGenerated.push_back(absPath);
              } else {
                this->AutogenTarget.Headers.push_back(absPath);
              }
            } else {
              if (generated) {
                this->AutogenTarget.SourcesGenerated.push_back(absPath);
              } else {
                this->AutogenTarget.Sources.push_back(absPath);
              }
            }
          }
        }
      }
      // Register rcc enabled files
      if (this->Rcc.Enabled && (ext == qrcExt) &&
          !sf->GetPropertyAsBool("SKIP_AUTORCC")) {
        // Register qrc file
        {
          Qrc qrc;
          qrc.QrcFile = cmSystemTools::GetRealPath(fPath);
          qrc.QrcName =
            cmSystemTools::GetFilenameWithoutLastExtension(qrc.QrcFile);
          qrc.Generated = sf->GetPropertyAsBool("GENERATED");
          // RCC options
          {
            std::string const opts = sf->GetSafeProperty("AUTORCC_OPTIONS");
            if (!opts.empty()) {
              cmSystemTools::ExpandListArgument(opts, qrc.Options);
            }
          }
          this->Rcc.Qrcs.push_back(std::move(qrc));
        }
      }
    }
  }
  // cmGeneratorTarget::GetConfigCommonSourceFiles computes the target's
  // sources meta data cache. Clear it so that OBJECT library targets that
  // are AUTOGEN initialized after this target get their added
  // mocs_compilation.cpp source acknowledged by this target.
  this->Target->ClearSourcesCache();

  if (this->Moc.Enabled || this->Uic.Enabled) {
    // Read skip files from makefile sources
    {
      std::string pathError;
      for (cmSourceFile* sf : makefile->GetSourceFiles()) {
        // sf->GetExtension() is only valid after sf->GetFullPath() ...
        // Since we're iterating over source files that might be not in the
        // target we need to check for path errors (not existing files).
        std::string const& fPath = sf->GetFullPath(&pathError);
        if (!pathError.empty()) {
          pathError.clear();
          continue;
        }
        cmSystemTools::FileFormat const fileType =
          cmSystemTools::GetFileFormat(sf->GetExtension().c_str());
        if (!(fileType == cmSystemTools::CXX_FILE_FORMAT) &&
            !(fileType == cmSystemTools::HEADER_FILE_FORMAT)) {
          continue;
        }
        const bool skipAll = sf->GetPropertyAsBool("SKIP_AUTOGEN");
        const bool mocSkip = this->Moc.Enabled &&
          (skipAll || sf->GetPropertyAsBool("SKIP_AUTOMOC"));
        const bool uicSkip = this->Uic.Enabled &&
          (skipAll || sf->GetPropertyAsBool("SKIP_AUTOUIC"));
        if (mocSkip || uicSkip) {
          std::string const absFile = cmSystemTools::GetRealPath(fPath);
          if (mocSkip) {
            this->Moc.Skip.insert(absFile);
          }
          if (uicSkip) {
            this->Uic.Skip.insert(absFile);
          }
        }
      }
    }

    // Process GENERATED sources and headers
    if (!this->AutogenTarget.SourcesGenerated.empty() ||
        !this->AutogenTarget.HeadersGenerated.empty()) {
      // Check status of policy CMP0071
      bool policyAccept = false;
      bool policyWarn = false;
      cmPolicies::PolicyStatus const CMP0071_status =
        makefile->GetPolicyStatus(cmPolicies::CMP0071);
      switch (CMP0071_status) {
        case cmPolicies::WARN:
          policyWarn = true;
          CM_FALLTHROUGH;
        case cmPolicies::OLD:
          // Ignore GENERATED file
          break;
        case cmPolicies::REQUIRED_IF_USED:
        case cmPolicies::REQUIRED_ALWAYS:
        case cmPolicies::NEW:
          // Process GENERATED file
          policyAccept = true;
          break;
      }

      if (policyAccept) {
        // Accept GENERATED sources
        for (std::string const& absFile :
             this->AutogenTarget.HeadersGenerated) {
          this->AutogenTarget.Headers.push_back(absFile);
          this->AutogenTarget.DependFiles.insert(absFile);
        }
        for (std::string const& absFile :
             this->AutogenTarget.SourcesGenerated) {
          this->AutogenTarget.Sources.push_back(absFile);
          this->AutogenTarget.DependFiles.insert(absFile);
        }
      } else {
        if (policyWarn) {
          std::string msg;
          msg += cmPolicies::GetPolicyWarning(cmPolicies::CMP0071);
          msg += "\n";
          std::string tools;
          std::string property;
          if (this->Moc.Enabled && this->Uic.Enabled) {
            tools = "AUTOMOC and AUTOUIC";
            property = "SKIP_AUTOGEN";
          } else if (this->Moc.Enabled) {
            tools = "AUTOMOC";
            property = "SKIP_AUTOMOC";
          } else if (this->Uic.Enabled) {
            tools = "AUTOUIC";
            property = "SKIP_AUTOUIC";
          }
          msg += "For compatibility, CMake is excluding the GENERATED source "
                 "file(s):\n";
          for (const std::string& absFile :
               this->AutogenTarget.HeadersGenerated) {
            msg.append("  ").append(Quoted(absFile)).append("\n");
          }
          for (const std::string& absFile :
               this->AutogenTarget.SourcesGenerated) {
            msg.append("  ").append(Quoted(absFile)).append("\n");
          }
          msg += "from processing by ";
          msg += tools;
          msg +=
            ". If any of the files should be processed, set CMP0071 to NEW. "
            "If any of the files should not be processed, "
            "explicitly exclude them by setting the source file property ";
          msg += property;
          msg += ":\n  set_property(SOURCE file.h PROPERTY ";
          msg += property;
          msg += " ON)\n";
          makefile->IssueMessage(cmake::AUTHOR_WARNING, msg);
        }
      }
    }
    // Sort headers and sources
    if (this->Moc.Enabled || this->Uic.Enabled) {
      std::sort(this->AutogenTarget.Headers.begin(),
                this->AutogenTarget.Headers.end());
      std::sort(this->AutogenTarget.Sources.begin(),
                this->AutogenTarget.Sources.end());
    }
  }

  // Process qrc files
  if (!this->Rcc.Qrcs.empty()) {
    const bool QtV5 = (this->QtVersion.Major == 5);
    // Target rcc options
    std::vector<std::string> optionsTarget;
    cmSystemTools::ExpandListArgument(
      this->Target->GetSafeProperty("AUTORCC_OPTIONS"), optionsTarget);

    // Check if file name is unique
    for (Qrc& qrc : this->Rcc.Qrcs) {
      qrc.Unique = true;
      for (Qrc const& qrc2 : this->Rcc.Qrcs) {
        if ((&qrc != &qrc2) && (qrc.QrcName == qrc2.QrcName)) {
          qrc.Unique = false;
          break;
        }
      }
    }
    // Path checksum and file names
    {
      cmFilePathChecksum const fpathCheckSum(makefile);
      for (Qrc& qrc : this->Rcc.Qrcs) {
        qrc.PathChecksum = fpathCheckSum.getPart(qrc.QrcFile);
        // RCC output file name
        {
          std::string rccFile = this->Dir.Build + "/";
          rccFile += qrc.PathChecksum;
          rccFile += "/qrc_";
          rccFile += qrc.QrcName;
          rccFile += ".cpp";
          qrc.RccFile = std::move(rccFile);
        }
        {
          std::string base = this->Dir.Info;
          base += "/RCC";
          base += qrc.QrcName;
          if (!qrc.Unique) {
            base += qrc.PathChecksum;
          }

          qrc.LockFile = base;
          qrc.LockFile += ".lock";

          qrc.InfoFile = base;
          qrc.InfoFile += "Info.cmake";

          qrc.SettingsFile = base;
          qrc.SettingsFile += "Settings.txt";

          if (this->MultiConfig) {
            for (std::string const& cfg : this->ConfigsList) {
              qrc.ConfigSettingsFile[cfg] =
                AppendFilenameSuffix(qrc.SettingsFile, "_" + cfg);
            }
          }
        }
      }
    }
    // RCC options
    for (Qrc& qrc : this->Rcc.Qrcs) {
      // Target options
      std::vector<std::string> opts = optionsTarget;
      // Merge computed "-name XYZ" option
      {
        std::string name = qrc.QrcName;
        // Replace '-' with '_'. The former is not valid for symbol names.
        std::replace(name.begin(), name.end(), '-', '_');
        if (!qrc.Unique) {
          name += "_";
          name += qrc.PathChecksum;
        }
        std::vector<std::string> nameOpts;
        nameOpts.emplace_back("-name");
        nameOpts.emplace_back(std::move(name));
        RccMergeOptions(opts, nameOpts, QtV5);
      }
      // Merge file option
      RccMergeOptions(opts, qrc.Options, QtV5);
      qrc.Options = std::move(opts);
    }
    // RCC resources
    for (Qrc& qrc : this->Rcc.Qrcs) {
      if (!qrc.Generated) {
        std::string error;
        if (!RccListInputs(qrc.QrcFile, qrc.Resources, error)) {
          cmSystemTools::Error(error.c_str());
          return false;
        }
      }
    }
  }

  return true;
}

bool cmQtAutoGenInitializer::InitAutogenTarget()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();
  cmLocalGenerator* localGen = this->Target->GetLocalGenerator();
  cmGlobalGenerator* globalGen = localGen->GetGlobalGenerator();

  // Register info file as generated by CMake
  makefile->AddCMakeOutputFile(this->AutogenTarget.InfoFile);

  // Files provided by the autogen target
  std::vector<std::string> autogenProvides;
  if (this->Moc.Enabled) {
    this->AddGeneratedSource(this->Moc.MocsCompilation, GeneratorT::MOC);
    autogenProvides.push_back(this->Moc.MocsCompilation);
  }

  // Compose target comment
  std::string autogenComment;
  {
    std::string tools;
    if (this->Moc.Enabled) {
      tools += "MOC";
    }
    if (this->Uic.Enabled) {
      if (!tools.empty()) {
        tools += " and ";
      }
      tools += "UIC";
    }
    autogenComment = "Automatic ";
    autogenComment += tools;
    autogenComment += " for target ";
    autogenComment += this->Target->GetName();
  }

  // Compose command lines
  cmCustomCommandLines commandLines;
  {
    cmCustomCommandLine currentLine;
    currentLine.push_back(cmSystemTools::GetCMakeCommand());
    currentLine.push_back("-E");
    currentLine.push_back("cmake_autogen");
    currentLine.push_back(this->AutogenTarget.InfoFile);
    currentLine.push_back("$<CONFIGURATION>");
    commandLines.push_back(std::move(currentLine));
  }

  // Use PRE_BUILD on demand
  bool usePRE_BUILD = false;
  if (globalGen->GetName().find("Visual Studio") != std::string::npos) {
    // Under VS use a PRE_BUILD event instead of a separate target to
    // reduce the number of targets loaded into the IDE.
    // This also works around a VS 11 bug that may skip updating the target:
    //  https://connect.microsoft.com/VisualStudio/feedback/details/769495
    usePRE_BUILD = true;
  }
  // Disable PRE_BUILD in some cases
  if (usePRE_BUILD) {
    // Cannot use PRE_BUILD with file depends
    if (!this->AutogenTarget.DependFiles.empty()) {
      usePRE_BUILD = false;
    }
  }
  // Create the autogen target/command
  if (usePRE_BUILD) {
    // Add additional autogen target dependencies to origin target
    for (cmTarget* depTarget : this->AutogenTarget.DependTargets) {
      this->Target->Target->AddUtility(depTarget->GetName(), makefile);
    }

    // Add the pre-build command directly to bypass the OBJECT_LIBRARY
    // rejection in cmMakefile::AddCustomCommandToTarget because we know
    // PRE_BUILD will work for an OBJECT_LIBRARY in this specific case.
    //
    // PRE_BUILD does not support file dependencies!
    const std::vector<std::string> no_output;
    const std::vector<std::string> no_deps;
    cmCustomCommand cc(makefile, no_output, autogenProvides, no_deps,
                       commandLines, autogenComment.c_str(),
                       this->Dir.Work.c_str());
    cc.SetEscapeOldStyle(false);
    cc.SetEscapeAllowMakeVars(true);
    this->Target->Target->AddPreBuildCommand(cc);
  } else {

    // Add link library target dependencies to the autogen target
    // dependencies
    {
      // add_dependencies/addUtility do not support generator expressions.
      // We depend only on the libraries found in all configs therefore.
      std::map<cmGeneratorTarget const*, std::size_t> commonTargets;
      for (std::string const& config : this->ConfigsList) {
        cmLinkImplementationLibraries const* libs =
          this->Target->GetLinkImplementationLibraries(config);
        if (libs != nullptr) {
          for (cmLinkItem const& item : libs->Libraries) {
            cmGeneratorTarget const* libTarget = item.Target;
            if ((libTarget != nullptr) &&
                !StaticLibraryCycle(this->Target, libTarget, config)) {
              // Increment target config count
              commonTargets[libTarget]++;
            }
          }
        }
      }
      for (auto const& item : commonTargets) {
        if (item.second == this->ConfigsList.size()) {
          this->AutogenTarget.DependTargets.insert(item.first->Target);
        }
      }
    }

    // Create autogen target
    cmTarget* autogenTarget = makefile->AddUtilityCommand(
      this->AutogenTarget.Name, cmMakefile::TargetOrigin::Generator, true,
      this->Dir.Work.c_str(), /*byproducts=*/autogenProvides,
      std::vector<std::string>(this->AutogenTarget.DependFiles.begin(),
                               this->AutogenTarget.DependFiles.end()),
      commandLines, false, autogenComment.c_str());
    // Create autogen generator target
    localGen->AddGeneratorTarget(
      new cmGeneratorTarget(autogenTarget, localGen));

    // Forward origin utilities to autogen target
    for (std::string const& depName : this->Target->Target->GetUtilities()) {
      autogenTarget->AddUtility(depName, makefile);
    }
    // Add additional autogen target dependencies to autogen target
    for (cmTarget* depTarget : this->AutogenTarget.DependTargets) {
      autogenTarget->AddUtility(depTarget->GetName(), makefile);
    }

    // Set FOLDER property in autogen target
    if (!this->TargetsFolder.empty()) {
      autogenTarget->SetProperty("FOLDER", this->TargetsFolder.c_str());
    }

    // Add autogen target to the origin target dependencies
    this->Target->Target->AddUtility(this->AutogenTarget.Name, makefile);
  }

  return true;
}

bool cmQtAutoGenInitializer::InitRccTargets()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();
  cmLocalGenerator* localGen = this->Target->GetLocalGenerator();

  for (Qrc const& qrc : this->Rcc.Qrcs) {
    // Register info file as generated by CMake
    makefile->AddCMakeOutputFile(qrc.InfoFile);
    // Register file at target
    this->AddGeneratedSource(qrc.RccFile, GeneratorT::RCC);

    std::vector<std::string> ccOutput;
    ccOutput.push_back(qrc.RccFile);

    cmCustomCommandLines commandLines;
    if (this->MultiConfig) {
      // Build for all configurations
      for (std::string const& config : this->ConfigsList) {
        cmCustomCommandLine currentLine;
        currentLine.push_back(cmSystemTools::GetCMakeCommand());
        currentLine.push_back("-E");
        currentLine.push_back("cmake_autorcc");
        currentLine.push_back(qrc.InfoFile);
        currentLine.push_back(config);
        commandLines.push_back(std::move(currentLine));
      }
    } else {
      cmCustomCommandLine currentLine;
      currentLine.push_back(cmSystemTools::GetCMakeCommand());
      currentLine.push_back("-E");
      currentLine.push_back("cmake_autorcc");
      currentLine.push_back(qrc.InfoFile);
      currentLine.push_back("$<CONFIG>");
      commandLines.push_back(std::move(currentLine));
    }
    std::string ccComment = "Automatic RCC for ";
    ccComment += FileProjectRelativePath(makefile, qrc.QrcFile);

    if (qrc.Generated) {
      // Create custom rcc target
      std::string ccName;
      {
        ccName = this->Target->GetName();
        ccName += "_arcc_";
        ccName += qrc.QrcName;
        if (!qrc.Unique) {
          ccName += "_";
          ccName += qrc.PathChecksum;
        }
        std::vector<std::string> ccDepends;
        // Add the .qrc and info file to the custom target dependencies
        ccDepends.push_back(qrc.QrcFile);
        ccDepends.push_back(qrc.InfoFile);

        cmTarget* autoRccTarget = makefile->AddUtilityCommand(
          ccName, cmMakefile::TargetOrigin::Generator, true,
          this->Dir.Work.c_str(), ccOutput, ccDepends, commandLines, false,
          ccComment.c_str());
        // Create autogen generator target
        localGen->AddGeneratorTarget(
          new cmGeneratorTarget(autoRccTarget, localGen));

        // Set FOLDER property in autogen target
        if (!this->TargetsFolder.empty()) {
          autoRccTarget->SetProperty("FOLDER", this->TargetsFolder.c_str());
        }
      }
      // Add autogen target to the origin target dependencies
      this->Target->Target->AddUtility(ccName, makefile);
    } else {
      // Create custom rcc command
      {
        std::vector<std::string> ccByproducts;
        std::vector<std::string> ccDepends;
        // Add the .qrc and info file to the custom command dependencies
        ccDepends.push_back(qrc.QrcFile);
        ccDepends.push_back(qrc.InfoFile);

        // Add the resource files to the dependencies
        for (std::string const& fileName : qrc.Resources) {
          // Add resource file to the custom command dependencies
          ccDepends.push_back(fileName);
        }
        makefile->AddCustomCommandToOutput(ccOutput, ccByproducts, ccDepends,
                                           /*main_dependency*/ std::string(),
                                           commandLines, ccComment.c_str(),
                                           this->Dir.Work.c_str());
      }
      // Reconfigure when .qrc file changes
      makefile->AddCMakeDependFile(qrc.QrcFile);
    }
  }

  return true;
}

bool cmQtAutoGenInitializer::SetupCustomTargets()
{
  // Create info directory on demand
  if (!cmSystemTools::MakeDirectory(this->Dir.Info)) {
    std::string emsg = ("AutoGen: Could not create directory: ");
    emsg += Quoted(this->Dir.Info);
    cmSystemTools::Error(emsg.c_str());
    return false;
  }

  // Generate autogen target info file
  if (this->Moc.Enabled || this->Uic.Enabled) {
    // Write autogen target info files
    if (!this->SetupWriteAutogenInfo()) {
      return false;
    }
  }

  // Write AUTORCC info files
  if (this->Rcc.Enabled && !this->SetupWriteRccInfo()) {
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::SetupWriteAutogenInfo()
{
  cmMakefile* makefile = this->Target->Target->GetMakefile();

  cmGeneratedFileStream ofs;
  ofs.SetCopyIfDifferent(true);
  ofs.Open(this->AutogenTarget.InfoFile, false, true);
  if (ofs) {
    // Utility lambdas
    auto CWrite = [&ofs](const char* key, std::string const& value) {
      ofs << "set(" << key << " " << cmOutputConverter::EscapeForCMake(value)
          << ")\n";
    };
    auto CWriteUInt = [&ofs](const char* key, unsigned int value) {
      ofs << "set(" << key << " " << value << ")\n";
    };
    auto CWriteList = [&CWrite](const char* key,
                                std::vector<std::string> const& list) {
      CWrite(key, cmJoin(list, ";"));
    };
    auto CWriteNestedLists =
      [&CWrite](const char* key,
                std::vector<std::vector<std::string>> const& lists) {
        std::vector<std::string> seplist;
        for (const std::vector<std::string>& list : lists) {
          std::string blist = "{";
          blist += cmJoin(list, ";");
          blist += "}";
          seplist.push_back(std::move(blist));
        }
        CWrite(key, cmJoin(seplist, cmQtAutoGen::ListSep));
      };
    auto CWriteSet = [&CWrite](const char* key,
                               std::set<std::string> const& list) {
      CWrite(key, cmJoin(list, ";"));
    };
    auto CWriteMap = [&ofs](const char* key,
                            std::map<std::string, std::string> const& map) {
      for (auto const& item : map) {
        ofs << "set(" << key << "_" << item.first << " "
            << cmOutputConverter::EscapeForCMake(item.second) << ")\n";
      }
    };
    auto MfDef = [makefile](const char* key) {
      return makefile->GetSafeDefinition(key);
    };

    // Write
    ofs << "# Meta\n";
    CWrite("AM_MULTI_CONFIG", this->MultiConfig ? "TRUE" : "FALSE");
    CWrite("AM_PARALLEL", this->AutogenTarget.Parallel);
    CWrite("AM_VERBOSITY", this->Verbosity);

    ofs << "# Directories\n";
    CWrite("AM_CMAKE_SOURCE_DIR", MfDef("CMAKE_SOURCE_DIR"));
    CWrite("AM_CMAKE_BINARY_DIR", MfDef("CMAKE_BINARY_DIR"));
    CWrite("AM_CMAKE_CURRENT_SOURCE_DIR", MfDef("CMAKE_CURRENT_SOURCE_DIR"));
    CWrite("AM_CMAKE_CURRENT_BINARY_DIR", MfDef("CMAKE_CURRENT_BINARY_DIR"));
    CWrite("AM_CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE",
           MfDef("CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE"));
    CWrite("AM_BUILD_DIR", this->Dir.Build);
    CWrite("AM_INCLUDE_DIR", this->Dir.Include);
    CWriteMap("AM_INCLUDE_DIR", this->Dir.ConfigInclude);

    ofs << "# Files\n";
    CWriteList("AM_SOURCES", this->AutogenTarget.Sources);
    CWriteList("AM_HEADERS", this->AutogenTarget.Headers);
    CWrite("AM_SETTINGS_FILE", this->AutogenTarget.SettingsFile);
    CWriteMap("AM_SETTINGS_FILE", this->AutogenTarget.ConfigSettingsFile);

    ofs << "# Qt\n";
    CWriteUInt("AM_QT_VERSION_MAJOR", this->QtVersion.Major);
    CWrite("AM_QT_MOC_EXECUTABLE", this->Moc.Executable);
    CWrite("AM_QT_UIC_EXECUTABLE", this->Uic.Executable);

    if (this->Moc.Enabled) {
      ofs << "# MOC settings\n";
      CWriteSet("AM_MOC_SKIP", this->Moc.Skip);
      CWrite("AM_MOC_DEFINITIONS", this->Moc.Defines);
      CWriteMap("AM_MOC_DEFINITIONS", this->Moc.ConfigDefines);
      CWrite("AM_MOC_INCLUDES", this->Moc.Includes);
      CWriteMap("AM_MOC_INCLUDES", this->Moc.ConfigIncludes);
      CWrite("AM_MOC_OPTIONS",
             this->Target->GetSafeProperty("AUTOMOC_MOC_OPTIONS"));
      CWrite("AM_MOC_RELAXED_MODE", MfDef("CMAKE_AUTOMOC_RELAXED_MODE"));
      CWrite("AM_MOC_MACRO_NAMES",
             this->Target->GetSafeProperty("AUTOMOC_MACRO_NAMES"));
      CWrite("AM_MOC_DEPEND_FILTERS",
             this->Target->GetSafeProperty("AUTOMOC_DEPEND_FILTERS"));
      CWrite("AM_MOC_PREDEFS_CMD", this->Moc.PredefsCmd);
    }

    if (this->Uic.Enabled) {
      ofs << "# UIC settings\n";
      CWriteSet("AM_UIC_SKIP", this->Uic.Skip);
      CWrite("AM_UIC_TARGET_OPTIONS", this->Uic.Options);
      CWriteMap("AM_UIC_TARGET_OPTIONS", this->Uic.ConfigOptions);
      CWriteList("AM_UIC_OPTIONS_FILES", this->Uic.FileFiles);
      CWriteNestedLists("AM_UIC_OPTIONS_OPTIONS", this->Uic.FileOptions);
      CWriteList("AM_UIC_SEARCH_PATHS", this->Uic.SearchPaths);
    }
  } else {
    std::string err = "AutoGen: Could not write file ";
    err += this->AutogenTarget.InfoFile;
    cmSystemTools::Error(err.c_str());
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::SetupWriteRccInfo()
{
  for (Qrc const& qrc : this->Rcc.Qrcs) {
    cmGeneratedFileStream ofs;
    ofs.SetCopyIfDifferent(true);
    ofs.Open(qrc.InfoFile, false, true);
    if (ofs) {
      // Utility lambdas
      auto CWrite = [&ofs](const char* key, std::string const& value) {
        ofs << "set(" << key << " " << cmOutputConverter::EscapeForCMake(value)
            << ")\n";
      };
      auto CWriteMap = [&ofs](const char* key,
                              std::map<std::string, std::string> const& map) {
        for (auto const& item : map) {
          ofs << "set(" << key << "_" << item.first << " "
              << cmOutputConverter::EscapeForCMake(item.second) << ")\n";
        }
      };

      // Write
      ofs << "# Configurations\n";
      CWrite("ARCC_MULTI_CONFIG", this->MultiConfig ? "TRUE" : "FALSE");
      CWrite("ARCC_VERBOSITY", this->Verbosity);
      ofs << "# Settings file\n";
      CWrite("ARCC_SETTINGS_FILE", qrc.SettingsFile);
      CWriteMap("ARCC_SETTINGS_FILE", qrc.ConfigSettingsFile);

      ofs << "# Directories\n";
      CWrite("ARCC_BUILD_DIR", this->Dir.Build);
      CWrite("ARCC_INCLUDE_DIR", this->Dir.Include);
      CWriteMap("ARCC_INCLUDE_DIR", this->Dir.ConfigInclude);

      ofs << "# Rcc executable\n";
      CWrite("ARCC_RCC_EXECUTABLE", this->Rcc.Executable);
      CWrite("ARCC_RCC_LIST_OPTIONS", cmJoin(this->Rcc.ListOptions, ";"));

      ofs << "# Rcc job\n";
      CWrite("ARCC_LOCK_FILE", qrc.LockFile);
      CWrite("ARCC_SOURCE", qrc.QrcFile);
      CWrite("ARCC_OUTPUT_CHECKSUM", qrc.PathChecksum);
      CWrite("ARCC_OUTPUT_NAME", cmSystemTools::GetFilenameName(qrc.RccFile));
      CWrite("ARCC_OPTIONS", cmJoin(qrc.Options, ";"));
      CWrite("ARCC_INPUTS", cmJoin(qrc.Resources, ";"));
    } else {
      std::string err = "AutoRcc: Could not write file ";
      err += qrc.InfoFile;
      cmSystemTools::Error(err.c_str());
      return false;
    }
  }

  return true;
}

void cmQtAutoGenInitializer::AddGeneratedSource(std::string const& filename,
                                                GeneratorT genType)
{
  // Register source file in makefile
  cmMakefile* makefile = this->Target->Target->GetMakefile();
  {
    cmSourceFile* gFile = makefile->GetOrCreateSource(filename, true);
    gFile->SetProperty("GENERATED", "1");
    gFile->SetProperty("SKIP_AUTOGEN", "On");
  }

  // Add source file to source group
  AddToSourceGroup(makefile, filename, genType);

  // Add source file to target
  this->Target->AddSource(filename);
}

cmQtAutoGenInitializer::IntegerVersion cmQtAutoGenInitializer::GetQtVersion(
  cmGeneratorTarget const* target)
{
  cmQtAutoGenInitializer::IntegerVersion res;
  cmMakefile* makefile = target->Target->GetMakefile();

  // -- Major version
  std::string qtMajor = makefile->GetSafeDefinition("QT_VERSION_MAJOR");
  if (qtMajor.empty()) {
    qtMajor = makefile->GetSafeDefinition("Qt5Core_VERSION_MAJOR");
  }
  if (qtMajor.empty()) {
    const char* dirprop = makefile->GetProperty("Qt5Core_VERSION_MAJOR");
    if (dirprop) {
      qtMajor = dirprop;
    }
  }
  {
    const char* targetQtVersion =
      target->GetLinkInterfaceDependentStringProperty("QT_MAJOR_VERSION", "");
    if (targetQtVersion != nullptr) {
      qtMajor = targetQtVersion;
    }
  }

  // -- Minor version
  std::string qtMinor;
  if (!qtMajor.empty()) {
    if (qtMajor == "5") {
      qtMinor = makefile->GetSafeDefinition("Qt5Core_VERSION_MINOR");
      if (qtMinor.empty()) {
        const char* dirprop = makefile->GetProperty("Qt5Core_VERSION_MINOR");
        if (dirprop) {
          qtMinor = dirprop;
        }
      }
    }
    if (qtMinor.empty()) {
      qtMinor = makefile->GetSafeDefinition("QT_VERSION_MINOR");
    }
    {
      const char* targetQtVersion =
        target->GetLinkInterfaceDependentStringProperty("QT_MINOR_VERSION",
                                                        "");
      if (targetQtVersion != nullptr) {
        qtMinor = targetQtVersion;
      }
    }
  }

  // -- Convert to integer
  if (!qtMajor.empty() && !qtMinor.empty()) {
    unsigned long majorUL(0);
    unsigned long minorUL(0);
    if (cmSystemTools::StringToULong(qtMajor.c_str(), &majorUL) &&
        cmSystemTools::StringToULong(qtMinor.c_str(), &minorUL)) {
      res.Major = static_cast<unsigned int>(majorUL);
      res.Minor = static_cast<unsigned int>(minorUL);
    }
  }

  return res;
}

bool cmQtAutoGenInitializer::GetMocExecutable()
{
  std::string err;

  // Find moc executable
  {
    std::string targetName;
    if (this->QtVersion.Major == 5) {
      targetName = "Qt5::moc";
    } else if (this->QtVersion.Major == 4) {
      targetName = "Qt4::moc";
    } else {
      err = "The AUTOMOC feature supports only Qt 4 and Qt 5";
    }
    if (!targetName.empty()) {
      cmLocalGenerator* localGen = this->Target->GetLocalGenerator();
      cmGeneratorTarget* tgt = localGen->FindGeneratorTargetToUse(targetName);
      if (tgt != nullptr) {
        this->Moc.Executable = tgt->ImportedGetLocation("");
      } else {
        err = "Could not find target " + targetName;
      }
    }
  }

  // Test moc command
  if (err.empty()) {
    if (cmSystemTools::FileExists(this->Moc.Executable, true)) {
      std::vector<std::string> command;
      command.push_back(this->Moc.Executable);
      command.push_back("-h");
      std::string stdOut;
      std::string stdErr;
      int retVal = 0;
      bool result = cmSystemTools::RunSingleCommand(
        command, &stdOut, &stdErr, &retVal, nullptr,
        cmSystemTools::OUTPUT_NONE, cmDuration::zero(), cmProcessOutput::Auto);
      if (!result) {
        err = "The moc test command failed: ";
        err += QuotedCommand(command);
      }
    } else {
      err = "The moc executable ";
      err += Quoted(this->Moc.Executable);
      err += " does not exist";
    }
  }

  // Print error
  if (!err.empty()) {
    std::string msg = "AutoMoc (";
    msg += this->Target->GetName();
    msg += "): ";
    msg += err;
    cmSystemTools::Error(msg.c_str());
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::GetUicExecutable()
{
  std::string err;

  // Find uic executable
  {
    std::string targetName;
    if (this->QtVersion.Major == 5) {
      targetName = "Qt5::uic";
    } else if (this->QtVersion.Major == 4) {
      targetName = "Qt4::uic";
    } else {
      err = "The AUTOUIC feature supports only Qt 4 and Qt 5";
    }
    if (!targetName.empty()) {
      cmLocalGenerator* localGen = this->Target->GetLocalGenerator();
      cmGeneratorTarget* tgt = localGen->FindGeneratorTargetToUse(targetName);
      if (tgt != nullptr) {
        this->Uic.Executable = tgt->ImportedGetLocation("");
      } else {
        if (this->QtVersion.Major == 5) {
          // Project does not use Qt5Widgets, but has AUTOUIC ON anyway
        } else {
          err = "Could not find target " + targetName;
        }
      }
    }
  }

  // Test uic command
  if (err.empty() && !this->Uic.Executable.empty()) {
    if (cmSystemTools::FileExists(this->Uic.Executable, true)) {
      std::vector<std::string> command;
      command.push_back(this->Uic.Executable);
      command.push_back("-h");
      std::string stdOut;
      std::string stdErr;
      int retVal = 0;
      bool result = cmSystemTools::RunSingleCommand(
        command, &stdOut, &stdErr, &retVal, nullptr,
        cmSystemTools::OUTPUT_NONE, cmDuration::zero(), cmProcessOutput::Auto);
      if (!result) {
        err = "The uic test command failed: ";
        err += QuotedCommand(command);
      }
    } else {
      err = "The uic executable ";
      err += Quoted(this->Uic.Executable);
      err += " does not exist";
    }
  }

  // Print error
  if (!err.empty()) {
    std::string msg = "AutoUic (";
    msg += this->Target->GetName();
    msg += "): ";
    msg += err;
    cmSystemTools::Error(msg.c_str());
    return false;
  }

  return true;
}

bool cmQtAutoGenInitializer::GetRccExecutable()
{
  std::string err;

  // Find rcc executable
  {
    std::string targetName;
    if (this->QtVersion.Major == 5) {
      targetName = "Qt5::rcc";
    } else if (this->QtVersion.Major == 4) {
      targetName = "Qt4::rcc";
    } else {
      err = "The AUTORCC feature supports only Qt 4 and Qt 5";
    }
    if (!targetName.empty()) {
      cmLocalGenerator* localGen = this->Target->GetLocalGenerator();
      cmGeneratorTarget* tgt = localGen->FindGeneratorTargetToUse(targetName);
      if (tgt != nullptr) {
        this->Rcc.Executable = tgt->ImportedGetLocation("");
      } else {
        err = "Could not find target " + targetName;
      }
    }
  }

  // Test rcc command
  if (err.empty()) {
    if (cmSystemTools::FileExists(this->Rcc.Executable, true)) {
      std::vector<std::string> command;
      command.push_back(this->Rcc.Executable);
      command.push_back("-h");
      std::string stdOut;
      std::string stdErr;
      int retVal = 0;
      bool result = cmSystemTools::RunSingleCommand(
        command, &stdOut, &stdErr, &retVal, nullptr,
        cmSystemTools::OUTPUT_NONE, cmDuration::zero(), cmProcessOutput::Auto);
      if (result) {
        // Detect if rcc supports (-)-list
        if (this->QtVersion.Major == 5) {
          if (stdOut.find("--list") != std::string::npos) {
            this->Rcc.ListOptions.push_back("--list");
          } else {
            this->Rcc.ListOptions.push_back("-list");
          }
        }
      } else {
        err = "The rcc test command failed: ";
        err += QuotedCommand(command);
      }
    } else {
      err = "The rcc executable ";
      err += Quoted(this->Rcc.Executable);
      err += " does not exist";
    }
  }

  // Print error
  if (!err.empty()) {
    std::string msg = "AutoRcc (";
    msg += this->Target->GetName();
    msg += "): ";
    msg += err;
    cmSystemTools::Error(msg.c_str());
    return false;
  }

  return true;
}

/// @brief Reads the resource files list from from a .qrc file
/// @arg fileName Must be the absolute path of the .qrc file
/// @return True if the rcc file was successfully read
bool cmQtAutoGenInitializer::RccListInputs(std::string const& fileName,
                                           std::vector<std::string>& files,
                                           std::string& error)
{
  if (!cmSystemTools::FileExists(fileName)) {
    error = "rcc resource file does not exist:\n  ";
    error += Quoted(fileName);
    error += "\n";
    return false;
  }
  if (!this->Rcc.ListOptions.empty()) {
    // Use rcc for file listing
    if (this->Rcc.Executable.empty()) {
      error = "rcc executable not available";
      return false;
    }

    // Run rcc list command in the directory of the qrc file with the
    // pathless
    // qrc file name argument. This way rcc prints relative paths.
    // This avoids issues on Windows when the qrc file is in a path that
    // contains non-ASCII characters.
    std::string const fileDir = cmSystemTools::GetFilenamePath(fileName);
    std::string const fileNameName = cmSystemTools::GetFilenameName(fileName);

    bool result = false;
    int retVal = 0;
    std::string rccStdOut;
    std::string rccStdErr;
    {
      std::vector<std::string> cmd;
      cmd.push_back(this->Rcc.Executable);
      cmd.insert(cmd.end(), this->Rcc.ListOptions.begin(),
                 this->Rcc.ListOptions.end());
      cmd.push_back(fileNameName);
      result = cmSystemTools::RunSingleCommand(
        cmd, &rccStdOut, &rccStdErr, &retVal, fileDir.c_str(),
        cmSystemTools::OUTPUT_NONE, cmDuration::zero(), cmProcessOutput::Auto);
    }
    if (!result || retVal) {
      error = "rcc list process failed for:\n  ";
      error += Quoted(fileName);
      error += "\n";
      error += rccStdOut;
      error += "\n";
      error += rccStdErr;
      error += "\n";
      return false;
    }
    if (!RccListParseOutput(rccStdOut, rccStdErr, files, error)) {
      return false;
    }
  } else {
    // We can't use rcc for the file listing.
    // Read the qrc file content into string and parse it.
    {
      std::string qrcContents;
      {
        cmsys::ifstream ifs(fileName.c_str());
        if (ifs) {
          std::ostringstream osst;
          osst << ifs.rdbuf();
          qrcContents = osst.str();
        } else {
          error = "rcc file not readable:\n  ";
          error += Quoted(fileName);
          error += "\n";
          return false;
        }
      }
      // Parse string content
      RccListParseContent(qrcContents, files);
    }
  }

  // Convert relative paths to absolute paths
  RccListConvertFullPath(cmSystemTools::GetFilenamePath(fileName), files);
  return true;
}
