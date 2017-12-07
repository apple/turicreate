
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudioGenerator.h"

#include "cmsys/Encoding.hxx"
#include <iostream>
#include <windows.h>

#include "cmAlgorithms.h"
#include "cmCallVisualStudioMacro.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmLocalVisualStudioGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmTarget.h"

cmGlobalVisualStudioGenerator::cmGlobalVisualStudioGenerator(cmake* cm)
  : cmGlobalGenerator(cm)
{
  cm->GetState()->SetIsGeneratorMultiConfig(true);
  cm->GetState()->SetWindowsShell(true);
  cm->GetState()->SetWindowsVSIDE(true);
}

cmGlobalVisualStudioGenerator::~cmGlobalVisualStudioGenerator()
{
}

cmGlobalVisualStudioGenerator::VSVersion
cmGlobalVisualStudioGenerator::GetVersion() const
{
  return this->Version;
}

void cmGlobalVisualStudioGenerator::SetVersion(VSVersion v)
{
  this->Version = v;
}

std::string cmGlobalVisualStudioGenerator::GetRegistryBase()
{
  return cmGlobalVisualStudioGenerator::GetRegistryBase(this->GetIDEVersion());
}

std::string cmGlobalVisualStudioGenerator::GetRegistryBase(const char* version)
{
  std::string key = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\";
  return key + version;
}

void cmGlobalVisualStudioGenerator::AddExtraIDETargets()
{
  // Add a special target that depends on ALL projects for easy build
  // of one configuration only.
  const char* no_working_dir = 0;
  std::vector<std::string> no_depends;
  cmCustomCommandLines no_commands;
  std::map<std::string, std::vector<cmLocalGenerator*> >::iterator it;
  for (it = this->ProjectMap.begin(); it != this->ProjectMap.end(); ++it) {
    std::vector<cmLocalGenerator*>& gen = it->second;
    // add the ALL_BUILD to the first local generator of each project
    if (!gen.empty()) {
      // Use no actual command lines so that the target itself is not
      // considered always out of date.
      cmTarget* allBuild = gen[0]->GetMakefile()->AddUtilityCommand(
        "ALL_BUILD", true, no_working_dir, no_depends, no_commands, false,
        "Build all projects");

      cmGeneratorTarget* gt = new cmGeneratorTarget(allBuild, gen[0]);
      gen[0]->AddGeneratorTarget(gt);

      //
      // Organize in the "predefined targets" folder:
      //
      if (this->UseFolderProperty()) {
        allBuild->SetProperty("FOLDER", this->GetPredefinedTargetsFolder());
      }

      // Now make all targets depend on the ALL_BUILD target
      for (std::vector<cmLocalGenerator*>::iterator i = gen.begin();
           i != gen.end(); ++i) {
        std::vector<cmGeneratorTarget*> targets = (*i)->GetGeneratorTargets();
        for (std::vector<cmGeneratorTarget*>::iterator t = targets.begin();
             t != targets.end(); ++t) {
          cmGeneratorTarget* tgt = *t;
          if (tgt->GetType() == cmStateEnums::GLOBAL_TARGET ||
              tgt->IsImported()) {
            continue;
          }
          if (!this->IsExcluded(gen[0], tgt)) {
            allBuild->AddUtility(tgt->GetName());
          }
        }
      }
    }
  }

  // Configure CMake Visual Studio macros, for this user on this version
  // of Visual Studio.
  this->ConfigureCMakeVisualStudioMacros();

  // Add CMakeLists.txt with custom command to rerun CMake.
  for (std::vector<cmLocalGenerator*>::const_iterator lgi =
         this->LocalGenerators.begin();
       lgi != this->LocalGenerators.end(); ++lgi) {
    cmLocalVisualStudioGenerator* lg =
      static_cast<cmLocalVisualStudioGenerator*>(*lgi);
    lg->AddCMakeListsRules();
  }
}

void cmGlobalVisualStudioGenerator::ComputeTargetObjectDirectory(
  cmGeneratorTarget* gt) const
{
  std::string dir = gt->LocalGenerator->GetCurrentBinaryDirectory();
  dir += "/";
  std::string tgtDir = gt->LocalGenerator->GetTargetDirectory(gt);
  if (!tgtDir.empty()) {
    dir += tgtDir;
    dir += "/";
  }
  const char* cd = this->GetCMakeCFGIntDir();
  if (cd && *cd) {
    dir += cd;
    dir += "/";
  }
  gt->ObjectDirectory = dir;
}

bool IsVisualStudioMacrosFileRegistered(const std::string& macrosFile,
                                        const std::string& regKeyBase,
                                        std::string& nextAvailableSubKeyName);

void RegisterVisualStudioMacros(const std::string& macrosFile,
                                const std::string& regKeyBase);

#define CMAKE_VSMACROS_FILENAME "CMakeVSMacros2.vsmacros"

#define CMAKE_VSMACROS_RELOAD_MACRONAME                                       \
  "Macros.CMakeVSMacros2.Macros.ReloadProjects"

#define CMAKE_VSMACROS_STOP_MACRONAME "Macros.CMakeVSMacros2.Macros.StopBuild"

void cmGlobalVisualStudioGenerator::ConfigureCMakeVisualStudioMacros()
{
  std::string dir = this->GetUserMacrosDirectory();

  if (dir != "") {
    std::string src = cmSystemTools::GetCMakeRoot();
    src += "/Templates/" CMAKE_VSMACROS_FILENAME;

    std::string dst = dir + "/CMakeMacros/" CMAKE_VSMACROS_FILENAME;

    // Copy the macros file to the user directory only if the
    // destination does not exist or the source location is newer.
    // This will allow the user to edit the macros for development
    // purposes but newer versions distributed with CMake will replace
    // older versions in user directories.
    int res;
    if (!cmSystemTools::FileTimeCompare(src.c_str(), dst.c_str(), &res) ||
        res > 0) {
      if (!cmSystemTools::CopyFileAlways(src.c_str(), dst.c_str())) {
        std::ostringstream oss;
        oss << "Could not copy from: " << src << std::endl;
        oss << "                 to: " << dst << std::endl;
        cmSystemTools::Message(oss.str().c_str(), "Warning");
      }
    }

    RegisterVisualStudioMacros(dst, this->GetUserMacrosRegKeyBase());
  }
}

void cmGlobalVisualStudioGenerator::CallVisualStudioMacro(
  MacroName m, const char* vsSolutionFile)
{
  // If any solution or project files changed during the generation,
  // tell Visual Studio to reload them...
  cmMakefile* mf = this->LocalGenerators[0]->GetMakefile();
  std::string dir = this->GetUserMacrosDirectory();

  // Only really try to call the macro if:
  //  - there is a UserMacrosDirectory
  //  - the CMake vsmacros file exists
  //  - the CMake vsmacros file is registered
  //  - there were .sln/.vcproj files changed during generation
  //
  if (dir != "") {
    std::string macrosFile = dir + "/CMakeMacros/" CMAKE_VSMACROS_FILENAME;
    std::string nextSubkeyName;
    if (cmSystemTools::FileExists(macrosFile.c_str()) &&
        IsVisualStudioMacrosFileRegistered(
          macrosFile, this->GetUserMacrosRegKeyBase(), nextSubkeyName)) {
      std::string topLevelSlnName;
      if (vsSolutionFile) {
        topLevelSlnName = vsSolutionFile;
      } else {
        topLevelSlnName = mf->GetCurrentBinaryDirectory();
        topLevelSlnName += "/";
        topLevelSlnName += this->LocalGenerators[0]->GetProjectName();
        topLevelSlnName += ".sln";
      }

      if (m == MacroReload) {
        std::vector<std::string> filenames;
        this->GetFilesReplacedDuringGenerate(filenames);
        if (!filenames.empty()) {
          // Convert vector to semi-colon delimited string of filenames:
          std::string projects;
          std::vector<std::string>::iterator it = filenames.begin();
          if (it != filenames.end()) {
            projects = *it;
            ++it;
          }
          for (; it != filenames.end(); ++it) {
            projects += ";";
            projects += *it;
          }
          cmCallVisualStudioMacro::CallMacro(
            topLevelSlnName, CMAKE_VSMACROS_RELOAD_MACRONAME, projects,
            this->GetCMakeInstance()->GetDebugOutput());
        }
      } else if (m == MacroStop) {
        cmCallVisualStudioMacro::CallMacro(
          topLevelSlnName, CMAKE_VSMACROS_STOP_MACRONAME, "",
          this->GetCMakeInstance()->GetDebugOutput());
      }
    }
  }
}

std::string cmGlobalVisualStudioGenerator::GetUserMacrosDirectory()
{
  return "";
}

std::string cmGlobalVisualStudioGenerator::GetUserMacrosRegKeyBase()
{
  return "";
}

void cmGlobalVisualStudioGenerator::FillLinkClosure(
  const cmGeneratorTarget* target, TargetSet& linked)
{
  if (linked.insert(target).second) {
    TargetDependSet const& depends = this->GetTargetDirectDepends(target);
    for (TargetDependSet::const_iterator di = depends.begin();
         di != depends.end(); ++di) {
      if (di->IsLink()) {
        this->FillLinkClosure(*di, linked);
      }
    }
  }
}

cmGlobalVisualStudioGenerator::TargetSet const&
cmGlobalVisualStudioGenerator::GetTargetLinkClosure(cmGeneratorTarget* target)
{
  TargetSetMap::iterator i = this->TargetLinkClosure.find(target);
  if (i == this->TargetLinkClosure.end()) {
    TargetSetMap::value_type entry(target, TargetSet());
    i = this->TargetLinkClosure.insert(entry).first;
    this->FillLinkClosure(target, i->second);
  }
  return i->second;
}

void cmGlobalVisualStudioGenerator::FollowLinkDepends(
  const cmGeneratorTarget* target, std::set<const cmGeneratorTarget*>& linked)
{
  if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return;
  }
  if (linked.insert(target).second &&
      target->GetType() == cmStateEnums::STATIC_LIBRARY) {
    // Static library targets do not list their link dependencies so
    // we must follow them transitively now.
    TargetDependSet const& depends = this->GetTargetDirectDepends(target);
    for (TargetDependSet::const_iterator di = depends.begin();
         di != depends.end(); ++di) {
      if (di->IsLink()) {
        this->FollowLinkDepends(*di, linked);
      }
    }
  }
}

bool cmGlobalVisualStudioGenerator::ComputeTargetDepends()
{
  if (!this->cmGlobalGenerator::ComputeTargetDepends()) {
    return false;
  }
  std::map<std::string, std::vector<cmLocalGenerator*> >::iterator it;
  for (it = this->ProjectMap.begin(); it != this->ProjectMap.end(); ++it) {
    std::vector<cmLocalGenerator*>& gen = it->second;
    for (std::vector<cmLocalGenerator*>::iterator i = gen.begin();
         i != gen.end(); ++i) {
      std::vector<cmGeneratorTarget*> targets = (*i)->GetGeneratorTargets();
      for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin();
           ti != targets.end(); ++ti) {
        this->ComputeVSTargetDepends(*ti);
      }
    }
  }
  return true;
}

static bool VSLinkable(cmGeneratorTarget const* t)
{
  return t->IsLinkable() || t->GetType() == cmStateEnums::OBJECT_LIBRARY;
}

void cmGlobalVisualStudioGenerator::ComputeVSTargetDepends(
  cmGeneratorTarget* target)
{
  if (this->VSTargetDepends.find(target) != this->VSTargetDepends.end()) {
    return;
  }
  VSDependSet& vsTargetDepend = this->VSTargetDepends[target];
  // VS <= 7.1 has two behaviors that affect solution dependencies.
  //
  // (1) Solution-level dependencies between a linkable target and a
  // library cause that library to be linked.  We use an intermedite
  // empty utility target to express the dependency.  (VS 8 and above
  // provide a project file "LinkLibraryDependencies" setting to
  // choose whether to activate this behavior.  We disable it except
  // when linking external project files.)
  //
  // (2) We cannot let static libraries depend directly on targets to
  // which they "link" because the librarian tool will copy the
  // targets into the static library.  While the work-around for
  // behavior (1) would also avoid this, it would create a large
  // number of extra utility targets for little gain.  Instead, use
  // the above work-around only for dependencies explicitly added by
  // the add_dependencies() command.  Approximate link dependencies by
  // leaving them out for the static library itself but following them
  // transitively for other targets.

  bool allowLinkable = (target->GetType() != cmStateEnums::STATIC_LIBRARY &&
                        target->GetType() != cmStateEnums::SHARED_LIBRARY &&
                        target->GetType() != cmStateEnums::MODULE_LIBRARY &&
                        target->GetType() != cmStateEnums::EXECUTABLE);

  TargetDependSet const& depends = this->GetTargetDirectDepends(target);

  // Collect implicit link dependencies (target_link_libraries).
  // Static libraries cannot depend on their link implementation
  // due to behavior (2), but they do not really need to.
  std::set<cmGeneratorTarget const*> linkDepends;
  if (target->GetType() != cmStateEnums::STATIC_LIBRARY) {
    for (TargetDependSet::const_iterator di = depends.begin();
         di != depends.end(); ++di) {
      cmTargetDepend dep = *di;
      if (dep.IsLink()) {
        this->FollowLinkDepends(*di, linkDepends);
      }
    }
  }

  // Collect explicit util dependencies (add_dependencies).
  std::set<cmGeneratorTarget const*> utilDepends;
  for (TargetDependSet::const_iterator di = depends.begin();
       di != depends.end(); ++di) {
    cmTargetDepend dep = *di;
    if (dep.IsUtil()) {
      this->FollowLinkDepends(*di, utilDepends);
    }
  }

  // Collect all targets linked by this target so we can avoid
  // intermediate targets below.
  TargetSet linked;
  if (target->GetType() != cmStateEnums::STATIC_LIBRARY) {
    linked = this->GetTargetLinkClosure(target);
  }

  // Emit link dependencies.
  for (std::set<cmGeneratorTarget const*>::iterator di = linkDepends.begin();
       di != linkDepends.end(); ++di) {
    cmGeneratorTarget const* dep = *di;
    vsTargetDepend.insert(dep->GetName());
  }

  // Emit util dependencies.  Possibly use intermediate targets.
  for (std::set<cmGeneratorTarget const*>::iterator di = utilDepends.begin();
       di != utilDepends.end(); ++di) {
    cmGeneratorTarget const* dgt = *di;
    if (allowLinkable || !VSLinkable(dgt) || linked.count(dgt)) {
      // Direct dependency allowed.
      vsTargetDepend.insert(dgt->GetName());
    } else {
      // Direct dependency on linkable target not allowed.
      // Use an intermediate utility target.
      vsTargetDepend.insert(this->GetUtilityDepend(dgt));
    }
  }
}

bool cmGlobalVisualStudioGenerator::FindMakeProgram(cmMakefile* mf)
{
  // Visual Studio generators know how to lookup their build tool
  // directly instead of needing a helper module to do it, so we
  // do not actually need to put CMAKE_MAKE_PROGRAM into the cache.
  if (cmSystemTools::IsOff(mf->GetDefinition("CMAKE_MAKE_PROGRAM"))) {
    mf->AddDefinition("CMAKE_MAKE_PROGRAM", this->GetVSMakeProgram().c_str());
  }
  return true;
}

std::string cmGlobalVisualStudioGenerator::GetUtilityDepend(
  cmGeneratorTarget const* target)
{
  UtilityDependsMap::iterator i = this->UtilityDepends.find(target);
  if (i == this->UtilityDepends.end()) {
    std::string name = this->WriteUtilityDepend(target);
    UtilityDependsMap::value_type entry(target, name);
    i = this->UtilityDepends.insert(entry).first;
  }
  return i->second;
}

std::string cmGlobalVisualStudioGenerator::GetStartupProjectName(
  cmLocalGenerator const* root) const
{
  const char* n = root->GetMakefile()->GetProperty("VS_STARTUP_PROJECT");
  if (n && *n) {
    std::string startup = n;
    if (this->FindTarget(startup)) {
      return startup;
    } else {
      root->GetMakefile()->IssueMessage(
        cmake::AUTHOR_WARNING,
        "Directory property VS_STARTUP_PROJECT specifies target "
        "'" +
          startup + "' that does not exist.  Ignoring.");
    }
  }

  // default, if not specified
  return this->GetAllTargetName();
}

bool IsVisualStudioMacrosFileRegistered(const std::string& macrosFile,
                                        const std::string& regKeyBase,
                                        std::string& nextAvailableSubKeyName)
{
  bool macrosRegistered = false;

  std::string s1;
  std::string s2;

  // Make lowercase local copies, convert to Unix slashes, and
  // see if the resulting strings are the same:
  s1 = cmSystemTools::LowerCase(macrosFile);
  cmSystemTools::ConvertToUnixSlashes(s1);

  std::string keyname;
  HKEY hkey = NULL;
  LONG result = ERROR_SUCCESS;
  DWORD index = 0;

  keyname = regKeyBase + "\\OtherProjects7";
  hkey = NULL;
  result =
    RegOpenKeyExW(HKEY_CURRENT_USER, cmsys::Encoding::ToWide(keyname).c_str(),
                  0, KEY_READ, &hkey);
  if (ERROR_SUCCESS == result) {
    // Iterate the subkeys and look for the values of interest in each subkey:
    wchar_t subkeyname[256];
    DWORD cch_subkeyname = sizeof(subkeyname) * sizeof(subkeyname[0]);
    wchar_t keyclass[256];
    DWORD cch_keyclass = sizeof(keyclass) * sizeof(keyclass[0]);
    FILETIME lastWriteTime;
    lastWriteTime.dwHighDateTime = 0;
    lastWriteTime.dwLowDateTime = 0;

    while (ERROR_SUCCESS == RegEnumKeyExW(hkey, index, subkeyname,
                                          &cch_subkeyname, 0, keyclass,
                                          &cch_keyclass, &lastWriteTime)) {
      // Open the subkey and query the values of interest:
      HKEY hsubkey = NULL;
      result = RegOpenKeyExW(hkey, subkeyname, 0, KEY_READ, &hsubkey);
      if (ERROR_SUCCESS == result) {
        DWORD valueType = REG_SZ;
        wchar_t data1[256];
        DWORD cch_data1 = sizeof(data1) * sizeof(data1[0]);
        RegQueryValueExW(hsubkey, L"Path", 0, &valueType, (LPBYTE)&data1[0],
                         &cch_data1);

        DWORD data2 = 0;
        DWORD cch_data2 = sizeof(data2);
        RegQueryValueExW(hsubkey, L"Security", 0, &valueType, (LPBYTE)&data2,
                         &cch_data2);

        DWORD data3 = 0;
        DWORD cch_data3 = sizeof(data3);
        RegQueryValueExW(hsubkey, L"StorageFormat", 0, &valueType,
                         (LPBYTE)&data3, &cch_data3);

        s2 = cmSystemTools::LowerCase(cmsys::Encoding::ToNarrow(data1));
        cmSystemTools::ConvertToUnixSlashes(s2);
        if (s2 == s1) {
          macrosRegistered = true;
        }

        std::string fullname = cmsys::Encoding::ToNarrow(data1);
        std::string filename;
        std::string filepath;
        std::string filepathname;
        std::string filepathpath;
        if (cmSystemTools::FileExists(fullname.c_str())) {
          filename = cmSystemTools::GetFilenameName(fullname);
          filepath = cmSystemTools::GetFilenamePath(fullname);
          filepathname = cmSystemTools::GetFilenameName(filepath);
          filepathpath = cmSystemTools::GetFilenamePath(filepath);
        }

        // std::cout << keyname << "\\" << subkeyname << ":" << std::endl;
        // std::cout << "  Path: " << data1 << std::endl;
        // std::cout << "  Security: " << data2 << std::endl;
        // std::cout << "  StorageFormat: " << data3 << std::endl;
        // std::cout << "  filename: " << filename << std::endl;
        // std::cout << "  filepath: " << filepath << std::endl;
        // std::cout << "  filepathname: " << filepathname << std::endl;
        // std::cout << "  filepathpath: " << filepathpath << std::endl;
        // std::cout << std::endl;

        RegCloseKey(hsubkey);
      } else {
        std::cout << "error opening subkey: " << subkeyname << std::endl;
        std::cout << std::endl;
      }

      ++index;
      cch_subkeyname = sizeof(subkeyname) * sizeof(subkeyname[0]);
      cch_keyclass = sizeof(keyclass) * sizeof(keyclass[0]);
      lastWriteTime.dwHighDateTime = 0;
      lastWriteTime.dwLowDateTime = 0;
    }

    RegCloseKey(hkey);
  } else {
    std::cout << "error opening key: " << keyname << std::endl;
    std::cout << std::endl;
  }

  // Pass back next available sub key name, assuming sub keys always
  // follow the expected naming scheme. Expected naming scheme is that
  // the subkeys of OtherProjects7 is 0 to n-1, so it's ok to use "n"
  // as the name of the next subkey.
  std::ostringstream ossNext;
  ossNext << index;
  nextAvailableSubKeyName = ossNext.str();

  keyname = regKeyBase + "\\RecordingProject7";
  hkey = NULL;
  result =
    RegOpenKeyExW(HKEY_CURRENT_USER, cmsys::Encoding::ToWide(keyname).c_str(),
                  0, KEY_READ, &hkey);
  if (ERROR_SUCCESS == result) {
    DWORD valueType = REG_SZ;
    wchar_t data1[256];
    DWORD cch_data1 = sizeof(data1) * sizeof(data1[0]);
    RegQueryValueExW(hkey, L"Path", 0, &valueType, (LPBYTE)&data1[0],
                     &cch_data1);

    DWORD data2 = 0;
    DWORD cch_data2 = sizeof(data2);
    RegQueryValueExW(hkey, L"Security", 0, &valueType, (LPBYTE)&data2,
                     &cch_data2);

    DWORD data3 = 0;
    DWORD cch_data3 = sizeof(data3);
    RegQueryValueExW(hkey, L"StorageFormat", 0, &valueType, (LPBYTE)&data3,
                     &cch_data3);

    s2 = cmSystemTools::LowerCase(cmsys::Encoding::ToNarrow(data1));
    cmSystemTools::ConvertToUnixSlashes(s2);
    if (s2 == s1) {
      macrosRegistered = true;
    }

    // std::cout << keyname << ":" << std::endl;
    // std::cout << "  Path: " << data1 << std::endl;
    // std::cout << "  Security: " << data2 << std::endl;
    // std::cout << "  StorageFormat: " << data3 << std::endl;
    // std::cout << std::endl;

    RegCloseKey(hkey);
  } else {
    std::cout << "error opening key: " << keyname << std::endl;
    std::cout << std::endl;
  }

  return macrosRegistered;
}

void WriteVSMacrosFileRegistryEntry(const std::string& nextAvailableSubKeyName,
                                    const std::string& macrosFile,
                                    const std::string& regKeyBase)
{
  std::string keyname = regKeyBase + "\\OtherProjects7";
  HKEY hkey = NULL;
  LONG result =
    RegOpenKeyExW(HKEY_CURRENT_USER, cmsys::Encoding::ToWide(keyname).c_str(),
                  0, KEY_READ | KEY_WRITE, &hkey);
  if (ERROR_SUCCESS == result) {
    // Create the subkey and set the values of interest:
    HKEY hsubkey = NULL;
    wchar_t lpClass[] = L"";
    result = RegCreateKeyExW(
      hkey, cmsys::Encoding::ToWide(nextAvailableSubKeyName).c_str(), 0,
      lpClass, 0, KEY_READ | KEY_WRITE, 0, &hsubkey, 0);
    if (ERROR_SUCCESS == result) {
      DWORD dw = 0;

      std::string s(macrosFile);
      std::replace(s.begin(), s.end(), '/', '\\');
      std::wstring ws = cmsys::Encoding::ToWide(s);

      result =
        RegSetValueExW(hsubkey, L"Path", 0, REG_SZ, (LPBYTE)ws.c_str(),
                       static_cast<DWORD>(ws.size() + 1) * sizeof(wchar_t));
      if (ERROR_SUCCESS != result) {
        std::cout << "error result 1: " << result << std::endl;
        std::cout << std::endl;
      }

      // Security value is always "1" for sample macros files (seems to be "2"
      // if you put the file somewhere outside the standard VSMacros folder)
      dw = 1;
      result = RegSetValueExW(hsubkey, L"Security", 0, REG_DWORD, (LPBYTE)&dw,
                              sizeof(DWORD));
      if (ERROR_SUCCESS != result) {
        std::cout << "error result 2: " << result << std::endl;
        std::cout << std::endl;
      }

      // StorageFormat value is always "0" for sample macros files
      dw = 0;
      result = RegSetValueExW(hsubkey, L"StorageFormat", 0, REG_DWORD,
                              (LPBYTE)&dw, sizeof(DWORD));
      if (ERROR_SUCCESS != result) {
        std::cout << "error result 3: " << result << std::endl;
        std::cout << std::endl;
      }

      RegCloseKey(hsubkey);
    } else {
      std::cout << "error creating subkey: " << nextAvailableSubKeyName
                << std::endl;
      std::cout << std::endl;
    }
    RegCloseKey(hkey);
  } else {
    std::cout << "error opening key: " << keyname << std::endl;
    std::cout << std::endl;
  }
}

void RegisterVisualStudioMacros(const std::string& macrosFile,
                                const std::string& regKeyBase)
{
  bool macrosRegistered;
  std::string nextAvailableSubKeyName;

  macrosRegistered = IsVisualStudioMacrosFileRegistered(
    macrosFile, regKeyBase, nextAvailableSubKeyName);

  if (!macrosRegistered) {
    int count =
      cmCallVisualStudioMacro::GetNumberOfRunningVisualStudioInstances("ALL");

    // Only register the macros file if there are *no* instances of Visual
    // Studio running. If we register it while one is running, first, it has
    // no effect on the running instance; second, and worse, Visual Studio
    // removes our newly added registration entry when it quits. Instead,
    // emit a warning asking the user to exit all running Visual Studio
    // instances...
    //
    if (0 != count) {
      std::ostringstream oss;
      oss << "Could not register CMake's Visual Studio macros file '"
          << CMAKE_VSMACROS_FILENAME "' while Visual Studio is running."
          << " Please exit all running instances of Visual Studio before"
          << " continuing." << std::endl
          << std::endl
          << "CMake needs to register Visual Studio macros when its macros"
          << " file is updated or when it detects that its current macros file"
          << " is no longer registered with Visual Studio." << std::endl;
      cmSystemTools::Message(oss.str().c_str(), "Warning");

      // Count them again now that the warning is over. In the case of a GUI
      // warning, the user may have gone to close Visual Studio and then come
      // back to the CMake GUI and clicked ok on the above warning. If so,
      // then register the macros *now* if the count is *now* 0...
      //
      count = cmCallVisualStudioMacro::GetNumberOfRunningVisualStudioInstances(
        "ALL");

      // Also re-get the nextAvailableSubKeyName in case Visual Studio
      // wrote out new registered macros information as it was exiting:
      //
      if (0 == count) {
        IsVisualStudioMacrosFileRegistered(macrosFile, regKeyBase,
                                           nextAvailableSubKeyName);
      }
    }

    // Do another if check - 'count' may have changed inside the above if:
    //
    if (0 == count) {
      WriteVSMacrosFileRegistryEntry(nextAvailableSubKeyName, macrosFile,
                                     regKeyBase);
    }
  }
}
bool cmGlobalVisualStudioGenerator::TargetIsFortranOnly(
  cmGeneratorTarget const* gt)
{
  // check to see if this is a fortran build
  std::set<std::string> languages;
  {
    // Issue diagnostic if the source files depend on the config.
    std::vector<cmSourceFile*> sources;
    if (!gt->GetConfigCommonSourceFiles(sources)) {
      return false;
    }
  }
  // If there's only one source language, Fortran has to be used
  // in order for the sources to compile.
  // Note: Via linker propagation, LINKER_LANGUAGE could become CXX in
  // this situation and mismatch from the actual language of the linker.
  gt->GetLanguages(languages, "");
  if (languages.size() == 1) {
    if (*languages.begin() == "Fortran") {
      return true;
    }
  }

  // In the case of mixed object files or sources mixed with objects,
  // decide the language based on the value of LINKER_LANGUAGE.
  // This will not make it possible to mix source files of different
  // languages, but object libraries will be linked together in the
  // same fashion as other generators do.
  if (gt->GetLinkerLanguage("") == "Fortran") {
    return true;
  }

  return false;
}

bool cmGlobalVisualStudioGenerator::TargetIsCSharpOnly(
  cmGeneratorTarget const* gt)
{
  // check to see if this is a C# build
  std::set<std::string> languages;
  {
    // Issue diagnostic if the source files depend on the config.
    std::vector<cmSourceFile*> sources;
    if (!gt->GetConfigCommonSourceFiles(sources)) {
      return false;
    }
    // Only "real" targets are allowed to be C# targets.
    if (gt->Target->GetType() > cmStateEnums::OBJECT_LIBRARY) {
      return false;
    }
  }
  gt->GetLanguages(languages, "");
  if (languages.size() == 1) {
    if (*languages.begin() == "CSharp") {
      return true;
    }
  }
  return false;
}

bool cmGlobalVisualStudioGenerator::TargetCanBeReferenced(
  cmGeneratorTarget const* gt)
{
  if (this->TargetIsCSharpOnly(gt)) {
    return true;
  }
  if (gt->GetType() != cmStateEnums::SHARED_LIBRARY &&
      gt->GetType() != cmStateEnums::EXECUTABLE) {
    return false;
  }
  return true;
}

bool cmGlobalVisualStudioGenerator::TargetCompare::operator()(
  cmGeneratorTarget const* l, cmGeneratorTarget const* r) const
{
  // Make sure a given named target is ordered first,
  // e.g. to set ALL_BUILD as the default active project.
  // When the empty string is named this is a no-op.
  if (r->GetName() == this->First) {
    return false;
  }
  if (l->GetName() == this->First) {
    return true;
  }
  return l->GetName() < r->GetName();
}

cmGlobalVisualStudioGenerator::OrderedTargetDependSet::OrderedTargetDependSet(
  TargetDependSet const& targets, std::string const& first)
  : derived(TargetCompare(first))
{
  this->insert(targets.begin(), targets.end());
}

cmGlobalVisualStudioGenerator::OrderedTargetDependSet::OrderedTargetDependSet(
  TargetSet const& targets, std::string const& first)
  : derived(TargetCompare(first))
{
  for (TargetSet::const_iterator it = targets.begin(); it != targets.end();
       ++it) {
    this->insert(*it);
  }
}

std::string cmGlobalVisualStudioGenerator::ExpandCFGIntDir(
  const std::string& str, const std::string& config) const
{
  std::string replace = GetCMakeCFGIntDir();

  std::string tmp = str;
  for (std::string::size_type i = tmp.find(replace); i != std::string::npos;
       i = tmp.find(replace, i)) {
    tmp.replace(i, replace.size(), config);
    i += config.size();
  }
  return tmp;
}

void cmGlobalVisualStudioGenerator::AddSymbolExportCommand(
  cmGeneratorTarget* gt, std::vector<cmCustomCommand>& commands,
  std::string const& configName)
{
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    gt->GetModuleDefinitionInfo(configName);
  if (!mdi || !mdi->DefFileGenerated) {
    return;
  }

  std::vector<std::string> outputs;
  outputs.push_back(mdi->DefFile);
  std::vector<std::string> empty;
  std::vector<cmSourceFile const*> objectSources;
  gt->GetObjectSources(objectSources, configName);
  std::map<cmSourceFile const*, std::string> mapping;
  for (std::vector<cmSourceFile const*>::const_iterator it =
         objectSources.begin();
       it != objectSources.end(); ++it) {
    mapping[*it];
  }
  gt->LocalGenerator->ComputeObjectFilenames(mapping, gt);
  std::string obj_dir = gt->ObjectDirectory;
  std::string cmakeCommand = cmSystemTools::GetCMakeCommand();
  cmSystemTools::ConvertToWindowsExtendedPath(cmakeCommand);
  cmCustomCommandLine cmdl;
  cmdl.push_back(cmakeCommand);
  cmdl.push_back("-E");
  cmdl.push_back("__create_def");
  cmdl.push_back(mdi->DefFile);
  std::string obj_dir_expanded = obj_dir;
  cmSystemTools::ReplaceString(obj_dir_expanded, this->GetCMakeCFGIntDir(),
                               configName.c_str());
  cmSystemTools::MakeDirectory(obj_dir_expanded);
  std::string const objs_file = obj_dir_expanded + "/objects.txt";
  cmdl.push_back(objs_file);
  cmGeneratedFileStream fout(objs_file.c_str());
  if (!fout) {
    cmSystemTools::Error("could not open ", objs_file.c_str());
    return;
  }

  if (mdi->WindowsExportAllSymbols) {
    std::vector<std::string> objs;
    for (std::vector<cmSourceFile const*>::const_iterator it =
           objectSources.begin();
         it != objectSources.end(); ++it) {
      // Find the object file name corresponding to this source file.
      std::map<cmSourceFile const*, std::string>::const_iterator map_it =
        mapping.find(*it);
      // It must exist because we populated the mapping just above.
      assert(!map_it->second.empty());
      std::string objFile = obj_dir + map_it->second;
      objs.push_back(objFile);
    }
    std::vector<cmSourceFile const*> externalObjectSources;
    gt->GetExternalObjects(externalObjectSources, configName);
    for (std::vector<cmSourceFile const*>::const_iterator it =
           externalObjectSources.begin();
         it != externalObjectSources.end(); ++it) {
      objs.push_back((*it)->GetFullPath());
    }

    for (std::vector<std::string>::iterator it = objs.begin();
         it != objs.end(); ++it) {
      std::string objFile = *it;
      // replace $(ConfigurationName) in the object names
      cmSystemTools::ReplaceString(objFile, this->GetCMakeCFGIntDir(),
                                   configName.c_str());
      if (cmHasLiteralSuffix(objFile, ".obj")) {
        fout << objFile << "\n";
      }
    }
  }

  for (std::vector<cmSourceFile const*>::const_iterator i =
         mdi->Sources.begin();
       i != mdi->Sources.end(); ++i) {
    fout << (*i)->GetFullPath() << "\n";
  }

  cmCustomCommandLines commandLines;
  commandLines.push_back(cmdl);
  cmCustomCommand command(gt->Target->GetMakefile(), outputs, empty, empty,
                          commandLines, "Auto build dll exports", ".");
  commands.push_back(command);
}
