/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTarget.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <assert.h>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <unordered_set>

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmMessenger.h"
#include "cmOutputConverter.h"
#include "cmProperty.h"
#include "cmSourceFile.h"
#include "cmSourceFileLocation.h"
#include "cmSourceFileLocationKind.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmTargetPropertyComputer.h"
#include "cmake.h"

template <>
const char* cmTargetPropertyComputer::ComputeLocationForBuild<cmTarget>(
  cmTarget const* tgt)
{
  static std::string loc;
  if (tgt->IsImported()) {
    loc = tgt->ImportedGetFullPath("", cmStateEnums::RuntimeBinaryArtifact);
    return loc.c_str();
  }

  cmGlobalGenerator* gg = tgt->GetGlobalGenerator();
  if (!gg->GetConfigureDoneCMP0026()) {
    gg->CreateGenerationObjects();
  }
  cmGeneratorTarget* gt = gg->FindGeneratorTarget(tgt->GetName());
  loc = gt->GetLocationForBuild();
  return loc.c_str();
}

template <>
const char* cmTargetPropertyComputer::ComputeLocation<cmTarget>(
  cmTarget const* tgt, const std::string& config)
{
  static std::string loc;
  if (tgt->IsImported()) {
    loc =
      tgt->ImportedGetFullPath(config, cmStateEnums::RuntimeBinaryArtifact);
    return loc.c_str();
  }

  cmGlobalGenerator* gg = tgt->GetGlobalGenerator();
  if (!gg->GetConfigureDoneCMP0026()) {
    gg->CreateGenerationObjects();
  }
  cmGeneratorTarget* gt = gg->FindGeneratorTarget(tgt->GetName());
  loc = gt->GetFullPath(config, cmStateEnums::RuntimeBinaryArtifact);
  return loc.c_str();
}

template <>
const char* cmTargetPropertyComputer::GetSources<cmTarget>(
  cmTarget const* tgt, cmMessenger* messenger,
  cmListFileBacktrace const& context)
{
  cmStringRange entries = tgt->GetSourceEntries();
  if (entries.empty()) {
    return nullptr;
  }

  std::ostringstream ss;
  const char* sep = "";
  for (std::string const& entry : entries) {
    std::vector<std::string> files;
    cmSystemTools::ExpandListArgument(entry, files);
    for (std::string const& file : files) {
      if (cmHasLiteralPrefix(file, "$<TARGET_OBJECTS:") &&
          file[file.size() - 1] == '>') {
        std::string objLibName = file.substr(17, file.size() - 18);

        if (cmGeneratorExpression::Find(objLibName) != std::string::npos) {
          ss << sep;
          sep = ";";
          ss << file;
          continue;
        }

        bool addContent = false;
        bool noMessage = true;
        std::ostringstream e;
        cmake::MessageType messageType = cmake::AUTHOR_WARNING;
        switch (context.GetBottom().GetPolicy(cmPolicies::CMP0051)) {
          case cmPolicies::WARN:
            e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0051) << "\n";
            noMessage = false;
          case cmPolicies::OLD:
            break;
          case cmPolicies::REQUIRED_ALWAYS:
          case cmPolicies::REQUIRED_IF_USED:
          case cmPolicies::NEW:
            addContent = true;
        }
        if (!noMessage) {
          e << "Target \"" << tgt->GetName()
            << "\" contains $<TARGET_OBJECTS> generator expression in its "
               "sources list.  This content was not previously part of the "
               "SOURCES property when that property was read at configure "
               "time.  Code reading that property needs to be adapted to "
               "ignore the generator expression using the string(GENEX_STRIP) "
               "command.";
          messenger->IssueMessage(messageType, e.str(), context);
        }
        if (addContent) {
          ss << sep;
          sep = ";";
          ss << file;
        }
      } else if (cmGeneratorExpression::Find(file) == std::string::npos) {
        ss << sep;
        sep = ";";
        ss << file;
      } else {
        cmSourceFile* sf = tgt->GetMakefile()->GetOrCreateSource(file);
        // Construct what is known about this source file location.
        cmSourceFileLocation const& location = sf->GetLocation();
        std::string sname = location.GetDirectory();
        if (!sname.empty()) {
          sname += "/";
        }
        sname += location.GetName();

        ss << sep;
        sep = ";";
        // Append this list entry.
        ss << sname;
      }
    }
  }
  static std::string srcs;
  srcs = ss.str();
  return srcs.c_str();
}

class cmTargetInternals
{
public:
  std::vector<std::string> IncludeDirectoriesEntries;
  std::vector<cmListFileBacktrace> IncludeDirectoriesBacktraces;
  std::vector<std::string> CompileOptionsEntries;
  std::vector<cmListFileBacktrace> CompileOptionsBacktraces;
  std::vector<std::string> CompileFeaturesEntries;
  std::vector<cmListFileBacktrace> CompileFeaturesBacktraces;
  std::vector<std::string> CompileDefinitionsEntries;
  std::vector<cmListFileBacktrace> CompileDefinitionsBacktraces;
  std::vector<std::string> SourceEntries;
  std::vector<cmListFileBacktrace> SourceBacktraces;
  std::vector<std::string> LinkOptionsEntries;
  std::vector<cmListFileBacktrace> LinkOptionsBacktraces;
  std::vector<std::string> LinkDirectoriesEntries;
  std::vector<cmListFileBacktrace> LinkDirectoriesBacktraces;
  std::vector<std::string> LinkImplementationPropertyEntries;
  std::vector<cmListFileBacktrace> LinkImplementationPropertyBacktraces;
};

cmTarget::cmTarget(std::string const& name, cmStateEnums::TargetType type,
                   Visibility vis, cmMakefile* mf)
{
  assert(mf);
  this->IsGeneratorProvided = false;
  this->Name = name;
  this->TargetTypeValue = type;
  this->Makefile = mf;
  this->HaveInstallRule = false;
  this->DLLPlatform = false;
  this->IsAndroid = false;
  this->IsImportedTarget =
    (vis == VisibilityImported || vis == VisibilityImportedGlobally);
  this->ImportedGloballyVisible = vis == VisibilityImportedGlobally;
  this->BuildInterfaceIncludesAppended = false;

  // Check whether this is a DLL platform.
  this->DLLPlatform =
    !this->Makefile->GetSafeDefinition("CMAKE_IMPORT_LIBRARY_SUFFIX").empty();

  // Check whether we are targeting an Android platform.
  this->IsAndroid =
    (this->Makefile->GetSafeDefinition("CMAKE_SYSTEM_NAME") == "Android");

  // Setup default property values.
  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetType() != cmStateEnums::UTILITY) {
    this->SetPropertyDefault("ANDROID_API", nullptr);
    this->SetPropertyDefault("ANDROID_API_MIN", nullptr);
    this->SetPropertyDefault("ANDROID_ARCH", nullptr);
    this->SetPropertyDefault("ANDROID_STL_TYPE", nullptr);
    this->SetPropertyDefault("ANDROID_SKIP_ANT_STEP", nullptr);
    this->SetPropertyDefault("ANDROID_PROCESS_MAX", nullptr);
    this->SetPropertyDefault("ANDROID_PROGUARD", nullptr);
    this->SetPropertyDefault("ANDROID_PROGUARD_CONFIG_PATH", nullptr);
    this->SetPropertyDefault("ANDROID_SECURE_PROPS_PATH", nullptr);
    this->SetPropertyDefault("ANDROID_NATIVE_LIB_DIRECTORIES", nullptr);
    this->SetPropertyDefault("ANDROID_NATIVE_LIB_DEPENDENCIES", nullptr);
    this->SetPropertyDefault("ANDROID_JAVA_SOURCE_DIR", nullptr);
    this->SetPropertyDefault("ANDROID_JAR_DIRECTORIES", nullptr);
    this->SetPropertyDefault("ANDROID_JAR_DEPENDENCIES", nullptr);
    this->SetPropertyDefault("ANDROID_ASSETS_DIRECTORIES", nullptr);
    this->SetPropertyDefault("ANDROID_ANT_ADDITIONAL_OPTIONS", nullptr);
    this->SetPropertyDefault("BUILD_RPATH", nullptr);
    this->SetPropertyDefault("INSTALL_NAME_DIR", nullptr);
    this->SetPropertyDefault("INSTALL_RPATH", "");
    this->SetPropertyDefault("INSTALL_RPATH_USE_LINK_PATH", "OFF");
    this->SetPropertyDefault("INTERPROCEDURAL_OPTIMIZATION", nullptr);
    this->SetPropertyDefault("SKIP_BUILD_RPATH", "OFF");
    this->SetPropertyDefault("BUILD_WITH_INSTALL_RPATH", "OFF");
    this->SetPropertyDefault("ARCHIVE_OUTPUT_DIRECTORY", nullptr);
    this->SetPropertyDefault("LIBRARY_OUTPUT_DIRECTORY", nullptr);
    this->SetPropertyDefault("RUNTIME_OUTPUT_DIRECTORY", nullptr);
    this->SetPropertyDefault("PDB_OUTPUT_DIRECTORY", nullptr);
    this->SetPropertyDefault("COMPILE_PDB_OUTPUT_DIRECTORY", nullptr);
    this->SetPropertyDefault("Fortran_FORMAT", nullptr);
    this->SetPropertyDefault("Fortran_MODULE_DIRECTORY", nullptr);
    this->SetPropertyDefault("Fortran_COMPILER_LAUNCHER", nullptr);
    this->SetPropertyDefault("GNUtoMS", nullptr);
    this->SetPropertyDefault("OSX_ARCHITECTURES", nullptr);
    this->SetPropertyDefault("IOS_INSTALL_COMBINED", nullptr);
    this->SetPropertyDefault("AUTOMOC", nullptr);
    this->SetPropertyDefault("AUTOUIC", nullptr);
    this->SetPropertyDefault("AUTORCC", nullptr);
    this->SetPropertyDefault("AUTOGEN_PARALLEL", nullptr);
    this->SetPropertyDefault("AUTOMOC_COMPILER_PREDEFINES", nullptr);
    this->SetPropertyDefault("AUTOMOC_DEPEND_FILTERS", nullptr);
    this->SetPropertyDefault("AUTOMOC_MACRO_NAMES", nullptr);
    this->SetPropertyDefault("AUTOMOC_MOC_OPTIONS", nullptr);
    this->SetPropertyDefault("AUTOUIC_OPTIONS", nullptr);
    this->SetPropertyDefault("AUTOUIC_SEARCH_PATHS", nullptr);
    this->SetPropertyDefault("AUTORCC_OPTIONS", nullptr);
    this->SetPropertyDefault("LINK_DEPENDS_NO_SHARED", nullptr);
    this->SetPropertyDefault("LINK_INTERFACE_LIBRARIES", nullptr);
    this->SetPropertyDefault("WIN32_EXECUTABLE", nullptr);
    this->SetPropertyDefault("MACOSX_BUNDLE", nullptr);
    this->SetPropertyDefault("MACOSX_RPATH", nullptr);
    this->SetPropertyDefault("NO_SYSTEM_FROM_IMPORTED", nullptr);
    this->SetPropertyDefault("BUILD_WITH_INSTALL_NAME_DIR", nullptr);
    this->SetPropertyDefault("C_CLANG_TIDY", nullptr);
    this->SetPropertyDefault("C_COMPILER_LAUNCHER", nullptr);
    this->SetPropertyDefault("C_CPPLINT", nullptr);
    this->SetPropertyDefault("C_CPPCHECK", nullptr);
    this->SetPropertyDefault("C_INCLUDE_WHAT_YOU_USE", nullptr);
    this->SetPropertyDefault("LINK_WHAT_YOU_USE", nullptr);
    this->SetPropertyDefault("C_STANDARD", nullptr);
    this->SetPropertyDefault("C_STANDARD_REQUIRED", nullptr);
    this->SetPropertyDefault("C_EXTENSIONS", nullptr);
    this->SetPropertyDefault("CXX_CLANG_TIDY", nullptr);
    this->SetPropertyDefault("CXX_COMPILER_LAUNCHER", nullptr);
    this->SetPropertyDefault("CXX_CPPLINT", nullptr);
    this->SetPropertyDefault("CXX_CPPCHECK", nullptr);
    this->SetPropertyDefault("CXX_INCLUDE_WHAT_YOU_USE", nullptr);
    this->SetPropertyDefault("CXX_STANDARD", nullptr);
    this->SetPropertyDefault("CXX_STANDARD_REQUIRED", nullptr);
    this->SetPropertyDefault("CXX_EXTENSIONS", nullptr);
    this->SetPropertyDefault("CUDA_STANDARD", nullptr);
    this->SetPropertyDefault("CUDA_STANDARD_REQUIRED", nullptr);
    this->SetPropertyDefault("CUDA_EXTENSIONS", nullptr);
    this->SetPropertyDefault("CUDA_COMPILER_LAUNCHER", nullptr);
    this->SetPropertyDefault("CUDA_SEPARABLE_COMPILATION", nullptr);
    this->SetPropertyDefault("LINK_SEARCH_START_STATIC", nullptr);
    this->SetPropertyDefault("LINK_SEARCH_END_STATIC", nullptr);
    this->SetPropertyDefault("FOLDER", nullptr);
#ifdef __APPLE__
    if (this->GetGlobalGenerator()->IsXcode()) {
      this->SetPropertyDefault("XCODE_SCHEME_ADDRESS_SANITIZER", nullptr);
      this->SetPropertyDefault(
        "XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_THREAD_SANITIZER", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_THREAD_SANITIZER_STOP", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER",
                               nullptr);
      this->SetPropertyDefault(
        "XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER_STOP", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_DISABLE_MAIN_THREAD_CHECKER",
                               nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_MAIN_THREAD_CHECKER_STOP",
                               nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_MALLOC_SCRIBBLE", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_MALLOC_GUARD_EDGES", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_GUARD_MALLOC", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_ZOMBIE_OBJECTS", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_MALLOC_STACK", nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_DYNAMIC_LINKER_API_USAGE",
                               nullptr);
      this->SetPropertyDefault("XCODE_SCHEME_DYNAMIC_LIBRARY_LOADS", nullptr);
    }
#endif
  }

  // Collect the set of configuration types.
  std::vector<std::string> configNames;
  mf->GetConfigurations(configNames);

  // Setup per-configuration property default values.
  if (this->GetType() != cmStateEnums::UTILITY) {
    const char* configProps[] = {
      /* clang-format needs this comment to break after the opening brace */
      "ARCHIVE_OUTPUT_DIRECTORY_",     "LIBRARY_OUTPUT_DIRECTORY_",
      "RUNTIME_OUTPUT_DIRECTORY_",     "PDB_OUTPUT_DIRECTORY_",
      "COMPILE_PDB_OUTPUT_DIRECTORY_", "MAP_IMPORTED_CONFIG_",
      "INTERPROCEDURAL_OPTIMIZATION_", nullptr
    };
    for (std::string const& configName : configNames) {
      std::string configUpper = cmSystemTools::UpperCase(configName);
      for (const char** p = configProps; *p; ++p) {
        // Interface libraries have no output locations, so honor only
        // the configuration map.
        if (this->TargetTypeValue == cmStateEnums::INTERFACE_LIBRARY &&
            strcmp(*p, "MAP_IMPORTED_CONFIG_") != 0) {
          continue;
        }
        std::string property = *p;
        property += configUpper;
        this->SetPropertyDefault(property, nullptr);
      }

      // Initialize per-configuration name postfix property from the
      // variable only for non-executable targets.  This preserves
      // compatibility with previous CMake versions in which executables
      // did not support this variable.  Projects may still specify the
      // property directly.
      if (this->TargetTypeValue != cmStateEnums::EXECUTABLE &&
          this->TargetTypeValue != cmStateEnums::INTERFACE_LIBRARY) {
        std::string property = cmSystemTools::UpperCase(configName);
        property += "_POSTFIX";
        this->SetPropertyDefault(property, nullptr);
      }
    }
  }

  // Save the backtrace of target construction.
  this->Backtrace = this->Makefile->GetBacktrace();

  if (!this->IsImported()) {
    // Initialize the INCLUDE_DIRECTORIES property based on the current value
    // of the same directory property:
    const cmStringRange parentIncludes =
      this->Makefile->GetIncludeDirectoriesEntries();
    const cmBacktraceRange parentIncludesBts =
      this->Makefile->GetIncludeDirectoriesBacktraces();

    this->Internal->IncludeDirectoriesEntries.insert(
      this->Internal->IncludeDirectoriesEntries.end(), parentIncludes.begin(),
      parentIncludes.end());
    this->Internal->IncludeDirectoriesBacktraces.insert(
      this->Internal->IncludeDirectoriesBacktraces.end(),
      parentIncludesBts.begin(), parentIncludesBts.end());

    const std::set<std::string> parentSystemIncludes =
      this->Makefile->GetSystemIncludeDirectories();

    this->SystemIncludeDirectories.insert(parentSystemIncludes.begin(),
                                          parentSystemIncludes.end());

    const cmStringRange parentCompileOptions =
      this->Makefile->GetCompileOptionsEntries();
    const cmBacktraceRange parentCompileOptionsBts =
      this->Makefile->GetCompileOptionsBacktraces();

    this->Internal->CompileOptionsEntries.insert(
      this->Internal->CompileOptionsEntries.end(),
      parentCompileOptions.begin(), parentCompileOptions.end());
    this->Internal->CompileOptionsBacktraces.insert(
      this->Internal->CompileOptionsBacktraces.end(),
      parentCompileOptionsBts.begin(), parentCompileOptionsBts.end());

    const cmStringRange parentLinkOptions =
      this->Makefile->GetLinkOptionsEntries();
    const cmBacktraceRange parentLinkOptionsBts =
      this->Makefile->GetLinkOptionsBacktraces();

    this->Internal->LinkOptionsEntries.insert(
      this->Internal->LinkOptionsEntries.end(), parentLinkOptions.begin(),
      parentLinkOptions.end());
    this->Internal->LinkOptionsBacktraces.insert(
      this->Internal->LinkOptionsBacktraces.end(),
      parentLinkOptionsBts.begin(), parentLinkOptionsBts.end());

    const cmStringRange parentLinkDirectories =
      this->Makefile->GetLinkDirectoriesEntries();
    const cmBacktraceRange parentLinkDirectoriesBts =
      this->Makefile->GetLinkDirectoriesBacktraces();

    this->Internal->LinkDirectoriesEntries.insert(
      this->Internal->LinkDirectoriesEntries.end(),
      parentLinkDirectories.begin(), parentLinkDirectories.end());
    this->Internal->LinkDirectoriesBacktraces.insert(
      this->Internal->LinkDirectoriesBacktraces.end(),
      parentLinkDirectoriesBts.begin(), parentLinkDirectoriesBts.end());
  }

  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetType() != cmStateEnums::UTILITY) {
    this->SetPropertyDefault("C_VISIBILITY_PRESET", nullptr);
    this->SetPropertyDefault("CXX_VISIBILITY_PRESET", nullptr);
    this->SetPropertyDefault("CUDA_VISIBILITY_PRESET", nullptr);
    this->SetPropertyDefault("VISIBILITY_INLINES_HIDDEN", nullptr);
  }

  if (this->TargetTypeValue == cmStateEnums::EXECUTABLE) {
    this->SetPropertyDefault("ANDROID_GUI", nullptr);
    this->SetPropertyDefault("CROSSCOMPILING_EMULATOR", nullptr);
    this->SetPropertyDefault("ENABLE_EXPORTS", nullptr);
  }
  if (this->TargetTypeValue == cmStateEnums::SHARED_LIBRARY ||
      this->TargetTypeValue == cmStateEnums::MODULE_LIBRARY) {
    this->SetProperty("POSITION_INDEPENDENT_CODE", "True");
  }
  if (this->TargetTypeValue == cmStateEnums::SHARED_LIBRARY ||
      this->TargetTypeValue == cmStateEnums::EXECUTABLE) {
    this->SetPropertyDefault("WINDOWS_EXPORT_ALL_SYMBOLS", nullptr);
  }

  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetType() != cmStateEnums::UTILITY) {
    this->SetPropertyDefault("POSITION_INDEPENDENT_CODE", nullptr);
  }

  // Record current policies for later use.
  this->Makefile->RecordPolicies(this->PolicyMap);

  if (this->TargetTypeValue == cmStateEnums::INTERFACE_LIBRARY) {
    // This policy is checked in a few conditions. The properties relevant
    // to the policy are always ignored for cmStateEnums::INTERFACE_LIBRARY
    // targets,
    // so ensure that the conditions don't lead to nonsense.
    this->PolicyMap.Set(cmPolicies::CMP0022, cmPolicies::NEW);
  }

  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetType() != cmStateEnums::UTILITY) {
    this->SetPropertyDefault("JOB_POOL_COMPILE", nullptr);
    this->SetPropertyDefault("JOB_POOL_LINK", nullptr);
  }

  if (this->TargetTypeValue <= cmStateEnums::UTILITY) {
    this->SetPropertyDefault("DOTNET_TARGET_FRAMEWORK_VERSION", nullptr);
  }

  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetType() != cmStateEnums::UTILITY) {

    // check for "CMAKE_VS_GLOBALS" variable and set up target properties
    // if any
    const char* globals = mf->GetDefinition("CMAKE_VS_GLOBALS");
    if (globals) {
      const std::string genName = mf->GetGlobalGenerator()->GetName();
      if (cmHasLiteralPrefix(genName, "Visual Studio")) {
        std::vector<std::string> props;
        cmSystemTools::ExpandListArgument(globals, props);
        const std::string vsGlobal = "VS_GLOBAL_";
        for (const std::string& i : props) {
          // split NAME=VALUE
          const std::string::size_type assignment = i.find('=');
          if (assignment != std::string::npos) {
            const std::string propName = vsGlobal + i.substr(0, assignment);
            const std::string propValue = i.substr(assignment + 1);
            this->SetPropertyDefault(propName, propValue.c_str());
          }
        }
      }
    }
  }
}

cmGlobalGenerator* cmTarget::GetGlobalGenerator() const
{
  return this->GetMakefile()->GetGlobalGenerator();
}

void cmTarget::AddUtility(const std::string& u, cmMakefile* makefile)
{
  if (this->Utilities.insert(u).second && makefile) {
    this->UtilityBacktraces.insert(
      std::make_pair(u, makefile->GetBacktrace()));
  }
}

cmListFileBacktrace const* cmTarget::GetUtilityBacktrace(
  const std::string& u) const
{
  std::map<std::string, cmListFileBacktrace>::const_iterator i =
    this->UtilityBacktraces.find(u);
  if (i == this->UtilityBacktraces.end()) {
    return nullptr;
  }

  return &i->second;
}

cmListFileBacktrace const& cmTarget::GetBacktrace() const
{
  return this->Backtrace;
}

bool cmTarget::IsExecutableWithExports() const
{
  return (this->GetType() == cmStateEnums::EXECUTABLE &&
          this->GetPropertyAsBool("ENABLE_EXPORTS"));
}

bool cmTarget::HasImportLibrary() const
{
  return (this->DLLPlatform &&
          (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
           this->IsExecutableWithExports()));
}

bool cmTarget::IsFrameworkOnApple() const
{
  return ((this->GetType() == cmStateEnums::SHARED_LIBRARY ||
           this->GetType() == cmStateEnums::STATIC_LIBRARY) &&
          this->Makefile->IsOn("APPLE") &&
          this->GetPropertyAsBool("FRAMEWORK"));
}

bool cmTarget::IsAppBundleOnApple() const
{
  return (this->GetType() == cmStateEnums::EXECUTABLE &&
          this->Makefile->IsOn("APPLE") &&
          this->GetPropertyAsBool("MACOSX_BUNDLE"));
}

void cmTarget::AddTracedSources(std::vector<std::string> const& srcs)
{
  if (!srcs.empty()) {
    cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
    this->Internal->SourceEntries.push_back(cmJoin(srcs, ";"));
    this->Internal->SourceBacktraces.push_back(lfbt);
  }
}

void cmTarget::AddSources(std::vector<std::string> const& srcs)
{
  std::string srcFiles;
  const char* sep = "";
  for (auto filename : srcs) {
    if (!cmGeneratorExpression::StartsWithGeneratorExpression(filename)) {
      if (!filename.empty()) {
        filename = this->ProcessSourceItemCMP0049(filename);
        if (filename.empty()) {
          return;
        }
      }
      this->Makefile->GetOrCreateSource(filename);
    }
    srcFiles += sep;
    srcFiles += filename;
    sep = ";";
  }
  if (!srcFiles.empty()) {
    cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
    this->Internal->SourceEntries.push_back(std::move(srcFiles));
    this->Internal->SourceBacktraces.push_back(lfbt);
  }
}

std::string cmTarget::ProcessSourceItemCMP0049(const std::string& s)
{
  std::string src = s;

  // For backwards compatibility replace variables in source names.
  // This should eventually be removed.
  this->Makefile->ExpandVariablesInString(src);
  if (src != s) {
    std::ostringstream e;
    bool noMessage = false;
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0049)) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0049) << "\n";
        break;
      case cmPolicies::OLD:
        noMessage = true;
        break;
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::NEW:
        messageType = cmake::FATAL_ERROR;
    }
    if (!noMessage) {
      e << "Legacy variable expansion in source file \"" << s
        << "\" expanded to \"" << src << "\" in target \"" << this->GetName()
        << "\".  This behavior will be removed in a "
           "future version of CMake.";
      this->Makefile->IssueMessage(messageType, e.str());
      if (messageType == cmake::FATAL_ERROR) {
        return "";
      }
    }
  }
  return src;
}

cmSourceFile* cmTarget::AddSourceCMP0049(const std::string& s)
{
  std::string src = this->ProcessSourceItemCMP0049(s);
  if (!s.empty() && src.empty()) {
    return nullptr;
  }
  return this->AddSource(src);
}

struct CreateLocation
{
  cmMakefile const* Makefile;

  CreateLocation(cmMakefile const* mf)
    : Makefile(mf)
  {
  }

  cmSourceFileLocation operator()(const std::string& filename)
  {
    return cmSourceFileLocation(this->Makefile, filename);
  }
};

struct LocationMatcher
{
  const cmSourceFileLocation& Needle;

  LocationMatcher(const cmSourceFileLocation& needle)
    : Needle(needle)
  {
  }

  bool operator()(cmSourceFileLocation& loc)
  {
    return loc.Matches(this->Needle);
  }
};

struct TargetPropertyEntryFinder
{
private:
  const cmSourceFileLocation& Needle;

public:
  TargetPropertyEntryFinder(const cmSourceFileLocation& needle)
    : Needle(needle)
  {
  }

  bool operator()(std::string const& entry)
  {
    std::vector<std::string> files;
    cmSystemTools::ExpandListArgument(entry, files);
    std::vector<cmSourceFileLocation> locations;
    locations.reserve(files.size());
    std::transform(files.begin(), files.end(), std::back_inserter(locations),
                   CreateLocation(this->Needle.GetMakefile()));

    return std::find_if(locations.begin(), locations.end(),
                        LocationMatcher(this->Needle)) != locations.end();
  }
};

cmSourceFile* cmTarget::AddSource(const std::string& src)
{
  cmSourceFileLocation sfl(this->Makefile, src,
                           cmSourceFileLocationKind::Known);
  if (std::find_if(this->Internal->SourceEntries.begin(),
                   this->Internal->SourceEntries.end(),
                   TargetPropertyEntryFinder(sfl)) ==
      this->Internal->SourceEntries.end()) {
    cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
    this->Internal->SourceEntries.push_back(src);
    this->Internal->SourceBacktraces.push_back(lfbt);
  }
  if (cmGeneratorExpression::Find(src) != std::string::npos) {
    return nullptr;
  }
  return this->Makefile->GetOrCreateSource(src, false,
                                           cmSourceFileLocationKind::Known);
}

void cmTarget::ClearDependencyInformation(cmMakefile& mf)
{
  std::string depname = this->GetName();
  depname += "_LIB_DEPENDS";
  mf.RemoveCacheDefinition(depname);
}

std::string cmTarget::GetDebugGeneratorExpressions(
  const std::string& value, cmTargetLinkLibraryType llt) const
{
  if (llt == GENERAL_LibraryType) {
    return value;
  }

  // Get the list of configurations considered to be DEBUG.
  std::vector<std::string> debugConfigs =
    this->Makefile->GetCMakeInstance()->GetDebugConfigs();

  std::string configString = "$<CONFIG:" + debugConfigs[0] + ">";

  if (debugConfigs.size() > 1) {
    for (std::vector<std::string>::const_iterator li =
           debugConfigs.begin() + 1;
         li != debugConfigs.end(); ++li) {
      configString += ",$<CONFIG:" + *li + ">";
    }
    configString = "$<OR:" + configString + ">";
  }

  if (llt == OPTIMIZED_LibraryType) {
    configString = "$<NOT:" + configString + ">";
  }
  return "$<" + configString + ":" + value + ">";
}

static std::string targetNameGenex(const std::string& lib)
{
  return "$<TARGET_NAME:" + lib + ">";
}

bool cmTarget::PushTLLCommandTrace(TLLSignature signature,
                                   cmListFileContext const& lfc)
{
  bool ret = true;
  if (!this->TLLCommands.empty()) {
    if (this->TLLCommands.back().first != signature) {
      ret = false;
    }
  }
  if (this->TLLCommands.empty() || this->TLLCommands.back().second != lfc) {
    this->TLLCommands.push_back(std::make_pair(signature, lfc));
  }
  return ret;
}

void cmTarget::GetTllSignatureTraces(std::ostream& s, TLLSignature sig) const
{
  const char* sigString =
    (sig == cmTarget::KeywordTLLSignature ? "keyword" : "plain");
  s << "The uses of the " << sigString << " signature are here:\n";
  cmOutputConverter converter(this->GetMakefile()->GetStateSnapshot());
  for (auto const& cmd : this->TLLCommands) {
    if (cmd.first == sig) {
      cmListFileContext lfc = cmd.second;
      lfc.FilePath = converter.ConvertToRelativePath(
        this->Makefile->GetState()->GetSourceDirectory(), lfc.FilePath);
      s << " * " << lfc << std::endl;
    }
  }
}

void cmTarget::AddLinkLibrary(cmMakefile& mf, const std::string& lib,
                              cmTargetLinkLibraryType llt)
{
  this->AddLinkLibrary(mf, lib, lib, llt);
}

void cmTarget::AddLinkLibrary(cmMakefile& mf, std::string const& lib,
                              std::string const& libRef,
                              cmTargetLinkLibraryType llt)
{
  cmTarget* tgt = mf.FindTargetToUse(lib);
  {
    const bool isNonImportedTarget = tgt && !tgt->IsImported();

    const std::string libName =
      (isNonImportedTarget && llt != GENERAL_LibraryType)
      ? targetNameGenex(libRef)
      : libRef;
    this->AppendProperty(
      "LINK_LIBRARIES",
      this->GetDebugGeneratorExpressions(libName, llt).c_str());
  }

  if (cmGeneratorExpression::Find(lib) != std::string::npos || lib != libRef ||
      (tgt &&
       (tgt->GetType() == cmStateEnums::INTERFACE_LIBRARY ||
        tgt->GetType() == cmStateEnums::OBJECT_LIBRARY)) ||
      (this->Name == lib)) {
    return;
  }

  this->OriginalLinkLibraries.emplace_back(lib, llt);

  // Add the explicit dependency information for libraries. This is
  // simply a set of libraries separated by ";". There should always
  // be a trailing ";". These library names are not canonical, in that
  // they may be "-framework x", "-ly", "/path/libz.a", etc.
  // We shouldn't remove duplicates here because external libraries
  // may be purposefully duplicated to handle recursive dependencies,
  // and we removing one instance will break the link line. Duplicates
  // will be appropriately eliminated at emit time.
  if (this->TargetTypeValue >= cmStateEnums::STATIC_LIBRARY &&
      this->TargetTypeValue <= cmStateEnums::MODULE_LIBRARY &&
      (this->GetPolicyStatusCMP0073() == cmPolicies::OLD ||
       this->GetPolicyStatusCMP0073() == cmPolicies::WARN)) {
    std::string targetEntry = this->Name;
    targetEntry += "_LIB_DEPENDS";
    std::string dependencies;
    const char* old_val = mf.GetDefinition(targetEntry);
    if (old_val) {
      dependencies += old_val;
    }
    switch (llt) {
      case GENERAL_LibraryType:
        dependencies += "general";
        break;
      case DEBUG_LibraryType:
        dependencies += "debug";
        break;
      case OPTIMIZED_LibraryType:
        dependencies += "optimized";
        break;
    }
    dependencies += ";";
    dependencies += lib;
    dependencies += ";";
    mf.AddCacheDefinition(targetEntry, dependencies.c_str(),
                          "Dependencies for the target", cmStateEnums::STATIC);
  }
}

void cmTarget::AddSystemIncludeDirectories(const std::set<std::string>& incs)
{
  this->SystemIncludeDirectories.insert(incs.begin(), incs.end());
}

cmStringRange cmTarget::GetIncludeDirectoriesEntries() const
{
  return cmMakeRange(this->Internal->IncludeDirectoriesEntries);
}

cmBacktraceRange cmTarget::GetIncludeDirectoriesBacktraces() const
{
  return cmMakeRange(this->Internal->IncludeDirectoriesBacktraces);
}

cmStringRange cmTarget::GetCompileOptionsEntries() const
{
  return cmMakeRange(this->Internal->CompileOptionsEntries);
}

cmBacktraceRange cmTarget::GetCompileOptionsBacktraces() const
{
  return cmMakeRange(this->Internal->CompileOptionsBacktraces);
}

cmStringRange cmTarget::GetCompileFeaturesEntries() const
{
  return cmMakeRange(this->Internal->CompileFeaturesEntries);
}

cmBacktraceRange cmTarget::GetCompileFeaturesBacktraces() const
{
  return cmMakeRange(this->Internal->CompileFeaturesBacktraces);
}

cmStringRange cmTarget::GetCompileDefinitionsEntries() const
{
  return cmMakeRange(this->Internal->CompileDefinitionsEntries);
}

cmBacktraceRange cmTarget::GetCompileDefinitionsBacktraces() const
{
  return cmMakeRange(this->Internal->CompileDefinitionsBacktraces);
}

cmStringRange cmTarget::GetSourceEntries() const
{
  return cmMakeRange(this->Internal->SourceEntries);
}

cmBacktraceRange cmTarget::GetSourceBacktraces() const
{
  return cmMakeRange(this->Internal->SourceBacktraces);
}

cmStringRange cmTarget::GetLinkOptionsEntries() const
{
  return cmMakeRange(this->Internal->LinkOptionsEntries);
}

cmBacktraceRange cmTarget::GetLinkOptionsBacktraces() const
{
  return cmMakeRange(this->Internal->LinkOptionsBacktraces);
}

cmStringRange cmTarget::GetLinkDirectoriesEntries() const
{
  return cmMakeRange(this->Internal->LinkDirectoriesEntries);
}

cmBacktraceRange cmTarget::GetLinkDirectoriesBacktraces() const
{
  return cmMakeRange(this->Internal->LinkDirectoriesBacktraces);
}

cmStringRange cmTarget::GetLinkImplementationEntries() const
{
  return cmMakeRange(this->Internal->LinkImplementationPropertyEntries);
}

cmBacktraceRange cmTarget::GetLinkImplementationBacktraces() const
{
  return cmMakeRange(this->Internal->LinkImplementationPropertyBacktraces);
}

void cmTarget::SetProperty(const std::string& prop, const char* value)
{
  if (!cmTargetPropertyComputer::PassesWhitelist(
        this->GetType(), prop, this->Makefile->GetMessenger(),
        this->Makefile->GetBacktrace())) {
    return;
  }
#define MAKE_STATIC_PROP(PROP) static const std::string prop##PROP = #PROP
  MAKE_STATIC_PROP(COMPILE_DEFINITIONS);
  MAKE_STATIC_PROP(COMPILE_FEATURES);
  MAKE_STATIC_PROP(COMPILE_OPTIONS);
  MAKE_STATIC_PROP(CUDA_PTX_COMPILATION);
  MAKE_STATIC_PROP(EXPORT_NAME);
  MAKE_STATIC_PROP(IMPORTED_GLOBAL);
  MAKE_STATIC_PROP(INCLUDE_DIRECTORIES);
  MAKE_STATIC_PROP(LINK_OPTIONS);
  MAKE_STATIC_PROP(LINK_DIRECTORIES);
  MAKE_STATIC_PROP(LINK_LIBRARIES);
  MAKE_STATIC_PROP(MANUALLY_ADDED_DEPENDENCIES);
  MAKE_STATIC_PROP(NAME);
  MAKE_STATIC_PROP(SOURCES);
  MAKE_STATIC_PROP(TYPE);
#undef MAKE_STATIC_PROP
  if (prop == propMANUALLY_ADDED_DEPENDENCIES) {
    std::ostringstream e;
    e << "MANUALLY_ADDED_DEPENDENCIES property is read-only\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == propNAME) {
    std::ostringstream e;
    e << "NAME property is read-only\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == propTYPE) {
    std::ostringstream e;
    e << "TYPE property is read-only\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == propEXPORT_NAME && this->IsImported()) {
    std::ostringstream e;
    e << "EXPORT_NAME property can't be set on imported targets (\""
      << this->Name << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == propSOURCES && this->IsImported()) {
    std::ostringstream e;
    e << "SOURCES property can't be set on imported targets (\"" << this->Name
      << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == propIMPORTED_GLOBAL && !this->IsImported()) {
    std::ostringstream e;
    e << "IMPORTED_GLOBAL property can't be set on non-imported targets (\""
      << this->Name << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }

  if (prop == propINCLUDE_DIRECTORIES) {
    this->Internal->IncludeDirectoriesEntries.clear();
    this->Internal->IncludeDirectoriesBacktraces.clear();
    if (value) {
      this->Internal->IncludeDirectoriesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->IncludeDirectoriesBacktraces.push_back(lfbt);
    }
  } else if (prop == propCOMPILE_OPTIONS) {
    this->Internal->CompileOptionsEntries.clear();
    this->Internal->CompileOptionsBacktraces.clear();
    if (value) {
      this->Internal->CompileOptionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileOptionsBacktraces.push_back(lfbt);
    }
  } else if (prop == propCOMPILE_FEATURES) {
    this->Internal->CompileFeaturesEntries.clear();
    this->Internal->CompileFeaturesBacktraces.clear();
    if (value) {
      this->Internal->CompileFeaturesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileFeaturesBacktraces.push_back(lfbt);
    }
  } else if (prop == propCOMPILE_DEFINITIONS) {
    this->Internal->CompileDefinitionsEntries.clear();
    this->Internal->CompileDefinitionsBacktraces.clear();
    if (value) {
      this->Internal->CompileDefinitionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileDefinitionsBacktraces.push_back(lfbt);
    }
  } else if (prop == propLINK_OPTIONS) {
    this->Internal->LinkOptionsEntries.clear();
    this->Internal->LinkOptionsBacktraces.clear();
    if (value) {
      this->Internal->LinkOptionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkOptionsBacktraces.push_back(lfbt);
    }
  } else if (prop == propLINK_DIRECTORIES) {
    this->Internal->LinkDirectoriesEntries.clear();
    this->Internal->LinkDirectoriesBacktraces.clear();
    if (value) {
      this->Internal->LinkDirectoriesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkDirectoriesBacktraces.push_back(lfbt);
    }
  } else if (prop == propLINK_LIBRARIES) {
    this->Internal->LinkImplementationPropertyEntries.clear();
    this->Internal->LinkImplementationPropertyBacktraces.clear();
    if (value) {
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkImplementationPropertyEntries.push_back(value);
      this->Internal->LinkImplementationPropertyBacktraces.push_back(lfbt);
    }
  } else if (prop == propSOURCES) {
    this->Internal->SourceEntries.clear();
    this->Internal->SourceBacktraces.clear();
    if (value) {
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->SourceEntries.push_back(value);
      this->Internal->SourceBacktraces.push_back(lfbt);
    }
  } else if (prop == propIMPORTED_GLOBAL) {
    if (!cmSystemTools::IsOn(value)) {
      std::ostringstream e;
      e << "IMPORTED_GLOBAL property can't be set to FALSE on targets (\""
        << this->Name << "\")\n";
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
      return;
    }
    /* no need to change anything if value does not change */
    if (!this->ImportedGloballyVisible) {
      this->ImportedGloballyVisible = true;
      this->GetGlobalGenerator()->IndexTarget(this);
    }
  } else if (cmHasLiteralPrefix(prop, "IMPORTED_LIBNAME") &&
             !this->CheckImportedLibName(prop, value ? value : "")) {
    /* error was reported by check method */
  } else if (prop == propCUDA_PTX_COMPILATION &&
             this->GetType() != cmStateEnums::OBJECT_LIBRARY) {
    std::ostringstream e;
    e << "CUDA_PTX_COMPILATION property can only be applied to OBJECT "
         "targets (\""
      << this->Name << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  } else {
    this->Properties.SetProperty(prop, value);
  }
}

void cmTarget::AppendProperty(const std::string& prop, const char* value,
                              bool asString)
{
  if (!cmTargetPropertyComputer::PassesWhitelist(
        this->GetType(), prop, this->Makefile->GetMessenger(),
        this->Makefile->GetBacktrace())) {
    return;
  }
  if (prop == "NAME") {
    std::ostringstream e;
    e << "NAME property is read-only\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == "EXPORT_NAME" && this->IsImported()) {
    std::ostringstream e;
    e << "EXPORT_NAME property can't be set on imported targets (\""
      << this->Name << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == "SOURCES" && this->IsImported()) {
    std::ostringstream e;
    e << "SOURCES property can't be set on imported targets (\"" << this->Name
      << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == "IMPORTED_GLOBAL") {
    std::ostringstream e;
    e << "IMPORTED_GLOBAL property can't be appended, only set on imported "
         "targets (\""
      << this->Name << "\")\n";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }
  if (prop == "INCLUDE_DIRECTORIES") {
    if (value && *value) {
      this->Internal->IncludeDirectoriesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->IncludeDirectoriesBacktraces.push_back(lfbt);
    }
  } else if (prop == "COMPILE_OPTIONS") {
    if (value && *value) {
      this->Internal->CompileOptionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileOptionsBacktraces.push_back(lfbt);
    }
  } else if (prop == "COMPILE_FEATURES") {
    if (value && *value) {
      this->Internal->CompileFeaturesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileFeaturesBacktraces.push_back(lfbt);
    }
  } else if (prop == "COMPILE_DEFINITIONS") {
    if (value && *value) {
      this->Internal->CompileDefinitionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->CompileDefinitionsBacktraces.push_back(lfbt);
    }
  } else if (prop == "LINK_OPTIONS") {
    if (value && *value) {
      this->Internal->LinkOptionsEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkOptionsBacktraces.push_back(lfbt);
    }
  } else if (prop == "LINK_DIRECTORIES") {
    if (value && *value) {
      this->Internal->LinkDirectoriesEntries.push_back(value);
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkDirectoriesBacktraces.push_back(lfbt);
    }
  } else if (prop == "LINK_LIBRARIES") {
    if (value && *value) {
      cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
      this->Internal->LinkImplementationPropertyEntries.push_back(value);
      this->Internal->LinkImplementationPropertyBacktraces.push_back(lfbt);
    }
  } else if (prop == "SOURCES") {
    cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
    this->Internal->SourceEntries.push_back(value);
    this->Internal->SourceBacktraces.push_back(lfbt);
  } else if (cmHasLiteralPrefix(prop, "IMPORTED_LIBNAME")) {
    this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                 prop + " property may not be APPENDed.");
  } else {
    this->Properties.AppendProperty(prop, value, asString);
  }
}

void cmTarget::AppendBuildInterfaceIncludes()
{
  if (this->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GetType() != cmStateEnums::STATIC_LIBRARY &&
      this->GetType() != cmStateEnums::MODULE_LIBRARY &&
      this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      !this->IsExecutableWithExports()) {
    return;
  }
  if (this->BuildInterfaceIncludesAppended) {
    return;
  }
  this->BuildInterfaceIncludesAppended = true;

  if (this->Makefile->IsOn("CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE")) {
    std::string dirs = this->Makefile->GetCurrentBinaryDirectory();
    if (!dirs.empty()) {
      dirs += ';';
    }
    dirs += this->Makefile->GetCurrentSourceDirectory();
    if (!dirs.empty()) {
      this->AppendProperty("INTERFACE_INCLUDE_DIRECTORIES",
                           ("$<BUILD_INTERFACE:" + dirs + ">").c_str());
    }
  }
}

void cmTarget::InsertInclude(std::string const& entry,
                             cmListFileBacktrace const& bt, bool before)
{
  std::vector<std::string>::iterator position = before
    ? this->Internal->IncludeDirectoriesEntries.begin()
    : this->Internal->IncludeDirectoriesEntries.end();

  std::vector<cmListFileBacktrace>::iterator btPosition = before
    ? this->Internal->IncludeDirectoriesBacktraces.begin()
    : this->Internal->IncludeDirectoriesBacktraces.end();

  this->Internal->IncludeDirectoriesEntries.insert(position, entry);
  this->Internal->IncludeDirectoriesBacktraces.insert(btPosition, bt);
}

void cmTarget::InsertCompileOption(std::string const& entry,
                                   cmListFileBacktrace const& bt, bool before)
{
  std::vector<std::string>::iterator position = before
    ? this->Internal->CompileOptionsEntries.begin()
    : this->Internal->CompileOptionsEntries.end();

  std::vector<cmListFileBacktrace>::iterator btPosition = before
    ? this->Internal->CompileOptionsBacktraces.begin()
    : this->Internal->CompileOptionsBacktraces.end();

  this->Internal->CompileOptionsEntries.insert(position, entry);
  this->Internal->CompileOptionsBacktraces.insert(btPosition, bt);
}

void cmTarget::InsertCompileDefinition(std::string const& entry,
                                       cmListFileBacktrace const& bt)
{
  this->Internal->CompileDefinitionsEntries.push_back(entry);
  this->Internal->CompileDefinitionsBacktraces.push_back(bt);
}

void cmTarget::InsertLinkOption(std::string const& entry,
                                cmListFileBacktrace const& bt, bool before)
{
  std::vector<std::string>::iterator position = before
    ? this->Internal->LinkOptionsEntries.begin()
    : this->Internal->LinkOptionsEntries.end();

  std::vector<cmListFileBacktrace>::iterator btPosition = before
    ? this->Internal->LinkOptionsBacktraces.begin()
    : this->Internal->LinkOptionsBacktraces.end();

  this->Internal->LinkOptionsEntries.insert(position, entry);
  this->Internal->LinkOptionsBacktraces.insert(btPosition, bt);
}

void cmTarget::InsertLinkDirectory(std::string const& entry,
                                   cmListFileBacktrace const& bt, bool before)
{
  std::vector<std::string>::iterator position = before
    ? this->Internal->LinkDirectoriesEntries.begin()
    : this->Internal->LinkDirectoriesEntries.end();

  std::vector<cmListFileBacktrace>::iterator btPosition = before
    ? this->Internal->LinkDirectoriesBacktraces.begin()
    : this->Internal->LinkDirectoriesBacktraces.end();

  this->Internal->LinkDirectoriesEntries.insert(position, entry);
  this->Internal->LinkDirectoriesBacktraces.insert(btPosition, bt);
}

static void cmTargetCheckLINK_INTERFACE_LIBRARIES(const std::string& prop,
                                                  const char* value,
                                                  cmMakefile* context,
                                                  bool imported)
{
  // Look for link-type keywords in the value.
  static cmsys::RegularExpression keys("(^|;)(debug|optimized|general)(;|$)");
  if (!keys.find(value)) {
    return;
  }

  // Support imported and non-imported versions of the property.
  const char* base = (imported ? "IMPORTED_LINK_INTERFACE_LIBRARIES"
                               : "LINK_INTERFACE_LIBRARIES");

  // Report an error.
  std::ostringstream e;
  e << "Property " << prop << " may not contain link-type keyword \""
    << keys.match(2) << "\".  "
    << "The " << base << " property has a per-configuration "
    << "version called " << base << "_<CONFIG> which may be "
    << "used to specify per-configuration rules.";
  if (!imported) {
    e << "  "
      << "Alternatively, an IMPORTED library may be created, configured "
      << "with a per-configuration location, and then named in the "
      << "property value.  "
      << "See the add_library command's IMPORTED mode for details."
      << "\n"
      << "If you have a list of libraries that already contains the "
      << "keyword, use the target_link_libraries command with its "
      << "LINK_INTERFACE_LIBRARIES mode to set the property.  "
      << "The command automatically recognizes link-type keywords and sets "
      << "the LINK_INTERFACE_LIBRARIES and LINK_INTERFACE_LIBRARIES_DEBUG "
      << "properties accordingly.";
  }
  context->IssueMessage(cmake::FATAL_ERROR, e.str());
}

static void cmTargetCheckINTERFACE_LINK_LIBRARIES(const char* value,
                                                  cmMakefile* context)
{
  // Look for link-type keywords in the value.
  static cmsys::RegularExpression keys("(^|;)(debug|optimized|general)(;|$)");
  if (!keys.find(value)) {
    return;
  }

  // Report an error.
  std::ostringstream e;

  e << "Property INTERFACE_LINK_LIBRARIES may not contain link-type "
       "keyword \""
    << keys.match(2)
    << "\".  The INTERFACE_LINK_LIBRARIES "
       "property may contain configuration-sensitive generator-expressions "
       "which may be used to specify per-configuration rules.";

  context->IssueMessage(cmake::FATAL_ERROR, e.str());
}

static void cmTargetCheckIMPORTED_GLOBAL(const cmTarget* target,
                                         cmMakefile* context)
{
  std::vector<cmTarget*> targets = context->GetOwnedImportedTargets();
  std::vector<cmTarget*>::const_iterator it =
    std::find(targets.begin(), targets.end(), target);
  if (it == targets.end()) {
    std::ostringstream e;
    e << "Attempt to promote imported target \"" << target->GetName()
      << "\" to global scope (by setting IMPORTED_GLOBAL) "
         "which is not built in this directory.";
    context->IssueMessage(cmake::FATAL_ERROR, e.str());
  }
}

void cmTarget::CheckProperty(const std::string& prop,
                             cmMakefile* context) const
{
  // Certain properties need checking.
  if (cmHasLiteralPrefix(prop, "LINK_INTERFACE_LIBRARIES")) {
    if (const char* value = this->GetProperty(prop)) {
      cmTargetCheckLINK_INTERFACE_LIBRARIES(prop, value, context, false);
    }
  }
  if (cmHasLiteralPrefix(prop, "IMPORTED_LINK_INTERFACE_LIBRARIES")) {
    if (const char* value = this->GetProperty(prop)) {
      cmTargetCheckLINK_INTERFACE_LIBRARIES(prop, value, context, true);
    }
  }
  if (prop == "INTERFACE_LINK_LIBRARIES") {
    if (const char* value = this->GetProperty(prop)) {
      cmTargetCheckINTERFACE_LINK_LIBRARIES(value, context);
    }
  }
  if (prop == "IMPORTED_GLOBAL") {
    if (this->IsImported()) {
      cmTargetCheckIMPORTED_GLOBAL(this, context);
    }
  }
}

const char* cmTarget::GetComputedProperty(
  const std::string& prop, cmMessenger* messenger,
  cmListFileBacktrace const& context) const
{
  return cmTargetPropertyComputer::GetProperty(this, prop, messenger, context);
}

const char* cmTarget::GetProperty(const std::string& prop) const
{
  static std::unordered_set<std::string> specialProps;
#define MAKE_STATIC_PROP(PROP) static const std::string prop##PROP = #PROP
  MAKE_STATIC_PROP(LINK_LIBRARIES);
  MAKE_STATIC_PROP(TYPE);
  MAKE_STATIC_PROP(INCLUDE_DIRECTORIES);
  MAKE_STATIC_PROP(COMPILE_FEATURES);
  MAKE_STATIC_PROP(COMPILE_OPTIONS);
  MAKE_STATIC_PROP(COMPILE_DEFINITIONS);
  MAKE_STATIC_PROP(LINK_OPTIONS);
  MAKE_STATIC_PROP(LINK_DIRECTORIES);
  MAKE_STATIC_PROP(IMPORTED);
  MAKE_STATIC_PROP(IMPORTED_GLOBAL);
  MAKE_STATIC_PROP(MANUALLY_ADDED_DEPENDENCIES);
  MAKE_STATIC_PROP(NAME);
  MAKE_STATIC_PROP(BINARY_DIR);
  MAKE_STATIC_PROP(SOURCE_DIR);
  MAKE_STATIC_PROP(SOURCES);
#undef MAKE_STATIC_PROP
  if (specialProps.empty()) {
    specialProps.insert(propLINK_LIBRARIES);
    specialProps.insert(propTYPE);
    specialProps.insert(propINCLUDE_DIRECTORIES);
    specialProps.insert(propCOMPILE_FEATURES);
    specialProps.insert(propCOMPILE_OPTIONS);
    specialProps.insert(propCOMPILE_DEFINITIONS);
    specialProps.insert(propLINK_OPTIONS);
    specialProps.insert(propLINK_DIRECTORIES);
    specialProps.insert(propIMPORTED);
    specialProps.insert(propIMPORTED_GLOBAL);
    specialProps.insert(propMANUALLY_ADDED_DEPENDENCIES);
    specialProps.insert(propNAME);
    specialProps.insert(propBINARY_DIR);
    specialProps.insert(propSOURCE_DIR);
    specialProps.insert(propSOURCES);
  }
  if (specialProps.count(prop)) {
    if (prop == propLINK_LIBRARIES) {
      if (this->Internal->LinkImplementationPropertyEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->LinkImplementationPropertyEntries, ";");
      return output.c_str();
    }
    // the type property returns what type the target is
    if (prop == propTYPE) {
      return cmState::GetTargetTypeName(this->GetType());
    }
    if (prop == propINCLUDE_DIRECTORIES) {
      if (this->Internal->IncludeDirectoriesEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->IncludeDirectoriesEntries, ";");
      return output.c_str();
    }
    if (prop == propCOMPILE_FEATURES) {
      if (this->Internal->CompileFeaturesEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->CompileFeaturesEntries, ";");
      return output.c_str();
    }
    if (prop == propCOMPILE_OPTIONS) {
      if (this->Internal->CompileOptionsEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->CompileOptionsEntries, ";");
      return output.c_str();
    }
    if (prop == propCOMPILE_DEFINITIONS) {
      if (this->Internal->CompileDefinitionsEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->CompileDefinitionsEntries, ";");
      return output.c_str();
    }
    if (prop == propLINK_OPTIONS) {
      if (this->Internal->LinkOptionsEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->LinkOptionsEntries, ";");
      return output.c_str();
    }
    if (prop == propLINK_DIRECTORIES) {
      if (this->Internal->LinkDirectoriesEntries.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Internal->LinkDirectoriesEntries, ";");

      return output.c_str();
    }
    if (prop == propMANUALLY_ADDED_DEPENDENCIES) {
      if (this->Utilities.empty()) {
        return nullptr;
      }

      static std::string output;
      output = cmJoin(this->Utilities, ";");
      return output.c_str();
    }
    if (prop == propIMPORTED) {
      return this->IsImported() ? "TRUE" : "FALSE";
    }
    if (prop == propIMPORTED_GLOBAL) {
      return this->IsImportedGloballyVisible() ? "TRUE" : "FALSE";
    }
    if (prop == propNAME) {
      return this->GetName().c_str();
    }
    if (prop == propBINARY_DIR) {
      return this->GetMakefile()
        ->GetStateSnapshot()
        .GetDirectory()
        .GetCurrentBinary()
        .c_str();
    }
    if (prop == propSOURCE_DIR) {
      return this->GetMakefile()
        ->GetStateSnapshot()
        .GetDirectory()
        .GetCurrentSource()
        .c_str();
    }
  }

  const char* retVal = this->Properties.GetPropertyValue(prop);
  if (!retVal) {
    const bool chain = this->GetMakefile()->GetState()->IsPropertyChained(
      prop, cmProperty::TARGET);
    if (chain) {
      return this->Makefile->GetStateSnapshot().GetDirectory().GetProperty(
        prop, chain);
    }
  }
  return retVal;
}

const char* cmTarget::GetSafeProperty(const std::string& prop) const
{
  const char* ret = this->GetProperty(prop);
  if (!ret) {
    return "";
  }
  return ret;
}

bool cmTarget::GetPropertyAsBool(const std::string& prop) const
{
  return cmSystemTools::IsOn(this->GetProperty(prop));
}

const char* cmTarget::GetSuffixVariableInternal(
  cmStateEnums::ArtifactType artifact) const
{
  switch (this->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      return "CMAKE_STATIC_LIBRARY_SUFFIX";
    case cmStateEnums::SHARED_LIBRARY:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          return "CMAKE_SHARED_LIBRARY_SUFFIX";
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_SUFFIX";
      }
      break;
    case cmStateEnums::MODULE_LIBRARY:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          return "CMAKE_SHARED_MODULE_SUFFIX";
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_SUFFIX";
      }
      break;
    case cmStateEnums::EXECUTABLE:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          // Android GUI application packages store the native
          // binary as a shared library.
          return (this->IsAndroid && this->GetPropertyAsBool("ANDROID_GUI")
                    ? "CMAKE_SHARED_LIBRARY_SUFFIX"
                    : "CMAKE_EXECUTABLE_SUFFIX");
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_SUFFIX";
      }
      break;
    default:
      break;
  }
  return "";
}

const char* cmTarget::GetPrefixVariableInternal(
  cmStateEnums::ArtifactType artifact) const
{
  switch (this->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      return "CMAKE_STATIC_LIBRARY_PREFIX";
    case cmStateEnums::SHARED_LIBRARY:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          return "CMAKE_SHARED_LIBRARY_PREFIX";
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_PREFIX";
      }
      break;
    case cmStateEnums::MODULE_LIBRARY:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          return "CMAKE_SHARED_MODULE_PREFIX";
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_PREFIX";
      }
      break;
    case cmStateEnums::EXECUTABLE:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          // Android GUI application packages store the native
          // binary as a shared library.
          return (this->IsAndroid && this->GetPropertyAsBool("ANDROID_GUI")
                    ? "CMAKE_SHARED_LIBRARY_PREFIX"
                    : "");
        case cmStateEnums::ImportLibraryArtifact:
          return "CMAKE_IMPORT_LIBRARY_PREFIX";
      }
      break;
    default:
      break;
  }
  return "";
}

std::string cmTarget::ImportedGetFullPath(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  assert(this->IsImported());

  // Lookup/compute/cache the import information for this
  // configuration.
  std::string desired_config = config;
  if (config.empty()) {
    desired_config = "NOCONFIG";
  }

  std::string result;

  const char* loc = nullptr;
  const char* imp = nullptr;
  std::string suffix;

  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
      this->GetMappedConfig(desired_config, &loc, &imp, suffix)) {
    switch (artifact) {
      case cmStateEnums::RuntimeBinaryArtifact:
        if (loc) {
          result = loc;
        } else {
          std::string impProp = "IMPORTED_LOCATION";
          impProp += suffix;
          if (const char* config_location = this->GetProperty(impProp)) {
            result = config_location;
          } else if (const char* location =
                       this->GetProperty("IMPORTED_LOCATION")) {
            result = location;
          }
        }
        break;

      case cmStateEnums::ImportLibraryArtifact:
        if (imp) {
          result = imp;
        } else if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
                   this->IsExecutableWithExports()) {
          std::string impProp = "IMPORTED_IMPLIB";
          impProp += suffix;
          if (const char* config_implib = this->GetProperty(impProp)) {
            result = config_implib;
          } else if (const char* implib =
                       this->GetProperty("IMPORTED_IMPLIB")) {
            result = implib;
          }
        }
        break;
    }
  }

  if (result.empty()) {
    result = this->GetName();
    result += "-NOTFOUND";
  }
  return result;
}

void cmTarget::SetPropertyDefault(const std::string& property,
                                  const char* default_value)
{
  // Compute the name of the variable holding the default value.
  std::string var = "CMAKE_";
  var += property;

  if (const char* value = this->Makefile->GetDefinition(var)) {
    this->SetProperty(property, value);
  } else if (default_value) {
    this->SetProperty(property, default_value);
  }
}

bool cmTarget::CheckImportedLibName(std::string const& prop,
                                    std::string const& value) const
{
  if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY ||
      !this->IsImported()) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      prop +
        " property may be set only on imported INTERFACE library targets.");
    return false;
  }
  if (!value.empty()) {
    if (value[0] == '-') {
      this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                   prop + " property value\n  " + value +
                                     "\nmay not start with '-'.");
      return false;
    }
    std::string::size_type bad = value.find_first_of(":/\\;");
    if (bad != std::string::npos) {
      this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                   prop + " property value\n  " + value +
                                     "\nmay not contain '" +
                                     value.substr(bad, 1) + "'.");
      return false;
    }
  }
  return true;
}

bool cmTarget::GetMappedConfig(std::string const& desired_config,
                               const char** loc, const char** imp,
                               std::string& suffix) const
{
  std::string config_upper;
  if (!desired_config.empty()) {
    config_upper = cmSystemTools::UpperCase(desired_config);
  }

  std::string locPropBase;
  if (this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    locPropBase = "IMPORTED_LIBNAME";
  } else if (this->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    locPropBase = "IMPORTED_OBJECTS";
  } else {
    locPropBase = "IMPORTED_LOCATION";
  }

  // Track the configuration-specific property suffix.
  suffix = "_";
  suffix += config_upper;

  std::vector<std::string> mappedConfigs;
  {
    std::string mapProp = "MAP_IMPORTED_CONFIG_";
    mapProp += config_upper;
    if (const char* mapValue = this->GetProperty(mapProp)) {
      cmSystemTools::ExpandListArgument(mapValue, mappedConfigs, true);
    }
  }

  // If we needed to find one of the mapped configurations but did not
  // On a DLL platform there may be only IMPORTED_IMPLIB for a shared
  // library or an executable with exports.
  bool allowImp = this->HasImportLibrary();

  // If a mapping was found, check its configurations.
  for (std::vector<std::string>::const_iterator mci = mappedConfigs.begin();
       !*loc && !*imp && mci != mappedConfigs.end(); ++mci) {
    // Look for this configuration.
    if (mci->empty()) {
      // An empty string in the mapping has a special meaning:
      // look up the config-less properties.
      *loc = this->GetProperty(locPropBase);
      if (allowImp) {
        *imp = this->GetProperty("IMPORTED_IMPLIB");
      }
      // If it was found, set the suffix.
      if (*loc || *imp) {
        suffix.clear();
      }
    } else {
      std::string mcUpper = cmSystemTools::UpperCase(*mci);
      std::string locProp = locPropBase + "_";
      locProp += mcUpper;
      *loc = this->GetProperty(locProp);
      if (allowImp) {
        std::string impProp = "IMPORTED_IMPLIB_";
        impProp += mcUpper;
        *imp = this->GetProperty(impProp);
      }

      // If it was found, use it for all properties below.
      if (*loc || *imp) {
        suffix = "_";
        suffix += mcUpper;
      }
    }
  }

  // If we needed to find one of the mapped configurations but did not
  // then the target location is not found.  The project does not want
  // any other configuration.
  if (!mappedConfigs.empty() && !*loc && !*imp) {
    // Interface libraries are always available because their
    // library name is optional so it is okay to leave *loc empty.
    return this->GetType() == cmStateEnums::INTERFACE_LIBRARY;
  }

  // If we have not yet found it then there are no mapped
  // configurations.  Look for an exact-match.
  if (!*loc && !*imp) {
    std::string locProp = locPropBase;
    locProp += suffix;
    *loc = this->GetProperty(locProp);
    if (allowImp) {
      std::string impProp = "IMPORTED_IMPLIB";
      impProp += suffix;
      *imp = this->GetProperty(impProp);
    }
  }

  // If we have not yet found it then there are no mapped
  // configurations and no exact match.
  if (!*loc && !*imp) {
    // The suffix computed above is not useful.
    suffix.clear();

    // Look for a configuration-less location.  This may be set by
    // manually-written code.
    *loc = this->GetProperty(locPropBase);
    if (allowImp) {
      *imp = this->GetProperty("IMPORTED_IMPLIB");
    }
  }

  // If we have not yet found it then the project is willing to try
  // any available configuration.
  if (!*loc && !*imp) {
    std::vector<std::string> availableConfigs;
    if (const char* iconfigs = this->GetProperty("IMPORTED_CONFIGURATIONS")) {
      cmSystemTools::ExpandListArgument(iconfigs, availableConfigs);
    }
    for (std::vector<std::string>::const_iterator aci =
           availableConfigs.begin();
         !*loc && !*imp && aci != availableConfigs.end(); ++aci) {
      suffix = "_";
      suffix += cmSystemTools::UpperCase(*aci);
      std::string locProp = locPropBase;
      locProp += suffix;
      *loc = this->GetProperty(locProp);
      if (allowImp) {
        std::string impProp = "IMPORTED_IMPLIB";
        impProp += suffix;
        *imp = this->GetProperty(impProp);
      }
    }
  }
  // If we have not yet found it then the target location is not available.
  if (!*loc && !*imp) {
    // Interface libraries are always available because their
    // library name is optional so it is okay to leave *loc empty.
    return this->GetType() == cmStateEnums::INTERFACE_LIBRARY;
  }

  return true;
}

cmTargetInternalPointer::cmTargetInternalPointer()
{
  this->Pointer = new cmTargetInternals;
}

cmTargetInternalPointer::cmTargetInternalPointer(
  cmTargetInternalPointer const& r)
{
  // Ideally cmTarget instances should never be copied.  However until
  // we can make a sweep to remove that, this copy constructor avoids
  // allowing the resources (Internals) to be copied.
  this->Pointer = new cmTargetInternals(*r.Pointer);
}

cmTargetInternalPointer::~cmTargetInternalPointer()
{
  delete this->Pointer;
}

cmTargetInternalPointer& cmTargetInternalPointer::operator=(
  cmTargetInternalPointer const& r)
{
  if (this == &r) {
    return *this;
  } // avoid warning on HP about self check
  // Ideally cmTarget instances should never be copied.  However until
  // we can make a sweep to remove that, this copy constructor avoids
  // allowing the resources (Internals) to be copied.
  cmTargetInternals* oldPointer = this->Pointer;
  this->Pointer = new cmTargetInternals(*r.Pointer);
  delete oldPointer;
  return *this;
}
