/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalUnixMakefileGenerator3.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <utility>

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmMakefileTargetGenerator.h"
#include "cmOutputConverter.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetDepend.h"
#include "cmake.h"

cmGlobalUnixMakefileGenerator3::cmGlobalUnixMakefileGenerator3(cmake* cm)
  : cmGlobalCommonGenerator(cm)
{
  // This type of makefile always requires unix style paths
  this->ForceUnixPaths = true;
  this->FindMakeProgramFile = "CMakeUnixFindMake.cmake";
  this->ToolSupportsColor = true;

#if defined(_WIN32) || defined(__VMS)
  this->UseLinkScript = false;
#else
  this->UseLinkScript = true;
#endif
  this->CommandDatabase = CM_NULLPTR;

  this->IncludeDirective = "include";
  this->DefineWindowsNULL = false;
  this->PassMakeflags = false;
  this->UnixCD = true;
}

void cmGlobalUnixMakefileGenerator3::EnableLanguage(
  std::vector<std::string> const& languages, cmMakefile* mf, bool optional)
{
  this->cmGlobalGenerator::EnableLanguage(languages, mf, optional);
  for (std::vector<std::string>::const_iterator l = languages.begin();
       l != languages.end(); ++l) {
    if (*l == "NONE") {
      continue;
    }
    this->ResolveLanguageCompiler(*l, mf, optional);
  }
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator* cmGlobalUnixMakefileGenerator3::CreateLocalGenerator(
  cmMakefile* mf)
{
  return new cmLocalUnixMakefileGenerator3(this, mf);
}

void cmGlobalUnixMakefileGenerator3::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalUnixMakefileGenerator3::GetActualName();
  entry.Brief = "Generates standard UNIX makefiles.";
}

std::string cmGlobalUnixMakefileGenerator3::GetEditCacheCommand() const
{
  // If generating for an extra IDE, the edit_cache target cannot
  // launch a terminal-interactive tool, so always use cmake-gui.
  if (!this->GetExtraGeneratorName().empty()) {
    return cmSystemTools::GetCMakeGUICommand();
  }

  // Use an internal cache entry to track the latest dialog used
  // to edit the cache, and use that for the edit_cache target.
  cmake* cm = this->GetCMakeInstance();
  std::string editCacheCommand = cm->GetCMakeEditCommand();
  if (!cm->GetCacheDefinition("CMAKE_EDIT_COMMAND") ||
      !editCacheCommand.empty()) {
    if (editCacheCommand.empty()) {
      editCacheCommand = cmSystemTools::GetCMakeCursesCommand();
    }
    if (editCacheCommand.empty()) {
      editCacheCommand = cmSystemTools::GetCMakeGUICommand();
    }
    if (!editCacheCommand.empty()) {
      cm->AddCacheEntry("CMAKE_EDIT_COMMAND", editCacheCommand.c_str(),
                        "Path to cache edit program executable.",
                        cmStateEnums::INTERNAL);
    }
  }
  const char* edit_cmd = cm->GetCacheDefinition("CMAKE_EDIT_COMMAND");
  return edit_cmd ? edit_cmd : "";
}

void cmGlobalUnixMakefileGenerator3::ComputeTargetObjectDirectory(
  cmGeneratorTarget* gt) const
{
  // Compute full path to object file directory for this target.
  std::string dir;
  dir += gt->LocalGenerator->GetCurrentBinaryDirectory();
  dir += "/";
  dir += gt->LocalGenerator->GetTargetDirectory(gt);
  dir += "/";
  gt->ObjectDirectory = dir;
}

void cmGlobalUnixMakefileGenerator3::Configure()
{
  // Initialize CMAKE_EDIT_COMMAND cache entry.
  this->GetEditCacheCommand();

  this->cmGlobalGenerator::Configure();
}

void cmGlobalUnixMakefileGenerator3::Generate()
{
  // first do superclass method
  this->cmGlobalGenerator::Generate();

  // initialize progress
  unsigned long total = 0;
  for (ProgressMapType::const_iterator pmi = this->ProgressMap.begin();
       pmi != this->ProgressMap.end(); ++pmi) {
    total += pmi->second.NumberOfActions;
  }

  // write each target's progress.make this loop is done twice. Bascially the
  // Generate pass counts all the actions, the first loop below determines
  // how many actions have progress updates for each target and writes to
  // corrrect variable values for everything except the all targets. The
  // second loop actually writes out correct values for the all targets as
  // well. This is because the all targets require more information that is
  // computed in the first loop.
  unsigned long current = 0;
  for (ProgressMapType::iterator pmi = this->ProgressMap.begin();
       pmi != this->ProgressMap.end(); ++pmi) {
    pmi->second.WriteProgressVariables(total, current);
  }
  for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
    cmLocalGenerator* lg = this->LocalGenerators[i];
    std::string markFileName = lg->GetCurrentBinaryDirectory();
    markFileName += "/";
    markFileName += cmake::GetCMakeFilesDirectory();
    markFileName += "/progress.marks";
    cmGeneratedFileStream markFile(markFileName.c_str());
    markFile << this->CountProgressMarksInAll(lg) << "\n";
  }

  // write the main makefile
  this->WriteMainMakefile2();
  this->WriteMainCMakefile();

  if (this->CommandDatabase != CM_NULLPTR) {
    *this->CommandDatabase << std::endl << "]";
    delete this->CommandDatabase;
    this->CommandDatabase = CM_NULLPTR;
  }
}

void cmGlobalUnixMakefileGenerator3::AddCXXCompileCommand(
  const std::string& sourceFile, const std::string& workingDirectory,
  const std::string& compileCommand)
{
  if (this->CommandDatabase == CM_NULLPTR) {
    std::string commandDatabaseName =
      std::string(this->GetCMakeInstance()->GetHomeOutputDirectory()) +
      "/compile_commands.json";
    this->CommandDatabase =
      new cmGeneratedFileStream(commandDatabaseName.c_str());
    *this->CommandDatabase << "[" << std::endl;
  } else {
    *this->CommandDatabase << "," << std::endl;
  }
  *this->CommandDatabase << "{" << std::endl
                         << "  \"directory\": \""
                         << cmGlobalGenerator::EscapeJSON(workingDirectory)
                         << "\"," << std::endl
                         << "  \"command\": \""
                         << cmGlobalGenerator::EscapeJSON(compileCommand)
                         << "\"," << std::endl
                         << "  \"file\": \""
                         << cmGlobalGenerator::EscapeJSON(sourceFile) << "\""
                         << std::endl
                         << "}";
}

void cmGlobalUnixMakefileGenerator3::WriteMainMakefile2()
{
  // Open the output file.  This should not be copy-if-different
  // because the check-build-system step compares the makefile time to
  // see if the build system must be regenerated.
  std::string makefileName =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  makefileName += cmake::GetCMakeFilesDirectory();
  makefileName += "/Makefile2";
  cmGeneratedFileStream makefileStream(makefileName.c_str(), false,
                                       this->GetMakefileEncoding());
  if (!makefileStream) {
    return;
  }

  // get a local generator for some useful methods
  cmLocalUnixMakefileGenerator3* lg =
    static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[0]);

  // Write the do not edit header.
  lg->WriteDisclaimer(makefileStream);

  // Write the main entry point target.  This must be the VERY first
  // target so that make with no arguments will run it.
  // Just depend on the all target to drive the build.
  std::vector<std::string> depends;
  std::vector<std::string> no_commands;
  depends.push_back("all");

  // Write the rule.
  lg->WriteMakeRule(makefileStream,
                    "Default target executed when no arguments are "
                    "given to make.",
                    "default_target", depends, no_commands, true);

  depends.clear();

  // The all and preinstall rules might never have any dependencies
  // added to them.
  if (this->EmptyRuleHackDepends != "") {
    depends.push_back(this->EmptyRuleHackDepends);
  }

  // Write and empty all:
  lg->WriteMakeRule(makefileStream, "The main recursive all target", "all",
                    depends, no_commands, true);

  // Write an empty preinstall:
  lg->WriteMakeRule(makefileStream, "The main recursive preinstall target",
                    "preinstall", depends, no_commands, true);

  // Write out the "special" stuff
  lg->WriteSpecialTargetsTop(makefileStream);

  // write the target convenience rules
  unsigned int i;
  for (i = 0; i < this->LocalGenerators.size(); ++i) {
    lg = static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[i]);
    this->WriteConvenienceRules2(makefileStream, lg);
  }

  lg = static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[0]);
  lg->WriteSpecialTargetsBottom(makefileStream);
}

void cmGlobalUnixMakefileGenerator3::WriteMainCMakefile()
{
  // Open the output file.  This should not be copy-if-different
  // because the check-build-system step compares the makefile time to
  // see if the build system must be regenerated.
  std::string cmakefileName =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  cmakefileName += cmake::GetCMakeFilesDirectory();
  cmakefileName += "/Makefile.cmake";
  cmGeneratedFileStream cmakefileStream(cmakefileName.c_str());
  if (!cmakefileStream) {
    return;
  }

  std::string makefileName =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  makefileName += "/Makefile";

  // get a local generator for some useful methods
  cmLocalUnixMakefileGenerator3* lg =
    static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[0]);

  // Write the do not edit header.
  lg->WriteDisclaimer(cmakefileStream);

  // Save the generator name
  cmakefileStream << "# The generator used is:\n"
                  << "set(CMAKE_DEPENDS_GENERATOR \"" << this->GetName()
                  << "\")\n\n";

  // for each cmMakefile get its list of dependencies
  std::vector<std::string> lfiles;
  for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
    lg = static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[i]);

    // Get the list of files contributing to this generation step.
    lfiles.insert(lfiles.end(), lg->GetMakefile()->GetListFiles().begin(),
                  lg->GetMakefile()->GetListFiles().end());
  }
  // Sort the list and remove duplicates.
  std::sort(lfiles.begin(), lfiles.end(), std::less<std::string>());
#if !defined(__VMS) // The Compaq STL on VMS crashes, so accept duplicates.
  std::vector<std::string>::iterator new_end =
    std::unique(lfiles.begin(), lfiles.end());
  lfiles.erase(new_end, lfiles.end());
#endif

  // reset lg to the first makefile
  lg = static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[0]);

  std::string currentBinDir = lg->GetCurrentBinaryDirectory();
  // Save the list to the cmake file.
  cmakefileStream
    << "# The top level Makefile was generated from the following files:\n"
    << "set(CMAKE_MAKEFILE_DEPENDS\n"
    << "  \"CMakeCache.txt\"\n";
  for (std::vector<std::string>::const_iterator i = lfiles.begin();
       i != lfiles.end(); ++i) {
    cmakefileStream << "  \"" << lg->ConvertToRelativePath(currentBinDir, *i)
                    << "\"\n";
  }
  cmakefileStream << "  )\n\n";

  // Build the path to the cache check file.
  std::string check = this->GetCMakeInstance()->GetHomeOutputDirectory();
  check += cmake::GetCMakeFilesDirectory();
  check += "/cmake.check_cache";

  // Set the corresponding makefile in the cmake file.
  cmakefileStream << "# The corresponding makefile is:\n"
                  << "set(CMAKE_MAKEFILE_OUTPUTS\n"
                  << "  \""
                  << lg->ConvertToRelativePath(currentBinDir, makefileName)
                  << "\"\n"
                  << "  \"" << lg->ConvertToRelativePath(currentBinDir, check)
                  << "\"\n";
  cmakefileStream << "  )\n\n";

  const std::string binDir = lg->GetBinaryDirectory();

  // CMake must rerun if a byproduct is missing.
  {
    cmakefileStream << "# Byproducts of CMake generate step:\n"
                    << "set(CMAKE_MAKEFILE_PRODUCTS\n";
    const std::vector<std::string>& outfiles =
      lg->GetMakefile()->GetOutputFiles();
    for (std::vector<std::string>::const_iterator k = outfiles.begin();
         k != outfiles.end(); ++k) {
      cmakefileStream << "  \"" << lg->ConvertToRelativePath(binDir, *k)
                      << "\"\n";
    }

    // add in all the directory information files
    std::string tmpStr;
    for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
      lg =
        static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[i]);
      tmpStr = lg->GetCurrentBinaryDirectory();
      tmpStr += cmake::GetCMakeFilesDirectory();
      tmpStr += "/CMakeDirectoryInformation.cmake";
      cmakefileStream << "  \"" << lg->ConvertToRelativePath(binDir, tmpStr)
                      << "\"\n";
    }
    cmakefileStream << "  )\n\n";
  }

  this->WriteMainCMakefileLanguageRules(cmakefileStream,
                                        this->LocalGenerators);
}

void cmGlobalUnixMakefileGenerator3::WriteMainCMakefileLanguageRules(
  cmGeneratedFileStream& cmakefileStream,
  std::vector<cmLocalGenerator*>& lGenerators)
{
  cmLocalUnixMakefileGenerator3* lg;

  // now list all the target info files
  cmakefileStream << "# Dependency information for all targets:\n";
  cmakefileStream << "set(CMAKE_DEPEND_INFO_FILES\n";
  for (unsigned int i = 0; i < lGenerators.size(); ++i) {
    lg = static_cast<cmLocalUnixMakefileGenerator3*>(lGenerators[i]);
    // for all of out targets
    std::vector<cmGeneratorTarget*> tgts = lg->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
         l != tgts.end(); l++) {
      if (((*l)->GetType() == cmStateEnums::EXECUTABLE) ||
          ((*l)->GetType() == cmStateEnums::STATIC_LIBRARY) ||
          ((*l)->GetType() == cmStateEnums::SHARED_LIBRARY) ||
          ((*l)->GetType() == cmStateEnums::MODULE_LIBRARY) ||
          ((*l)->GetType() == cmStateEnums::OBJECT_LIBRARY) ||
          ((*l)->GetType() == cmStateEnums::UTILITY)) {
        cmGeneratorTarget* gt = *l;
        std::string tname = lg->GetRelativeTargetDirectory(gt);
        tname += "/DependInfo.cmake";
        cmSystemTools::ConvertToUnixSlashes(tname);
        cmakefileStream << "  \"" << tname << "\"\n";
      }
    }
  }
  cmakefileStream << "  )\n";
}

void cmGlobalUnixMakefileGenerator3::WriteDirectoryRule2(
  std::ostream& ruleFileStream, cmLocalUnixMakefileGenerator3* lg,
  const char* pass, bool check_all, bool check_relink)
{
  // Get the relative path to the subdirectory from the top.
  std::string makeTarget = lg->GetCurrentBinaryDirectory();
  makeTarget += "/";
  makeTarget += pass;

  // The directory-level rule should depend on the target-level rules
  // for all targets in the directory.
  std::vector<std::string> depends;
  std::vector<cmGeneratorTarget*> targets = lg->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator l = targets.begin();
       l != targets.end(); ++l) {
    cmGeneratorTarget* gtarget = *l;
    int type = gtarget->GetType();
    if ((type == cmStateEnums::EXECUTABLE) ||
        (type == cmStateEnums::STATIC_LIBRARY) ||
        (type == cmStateEnums::SHARED_LIBRARY) ||
        (type == cmStateEnums::MODULE_LIBRARY) ||
        (type == cmStateEnums::OBJECT_LIBRARY) ||
        (type == cmStateEnums::UTILITY)) {
      // Add this to the list of depends rules in this directory.
      if ((!check_all || !gtarget->GetPropertyAsBool("EXCLUDE_FROM_ALL")) &&
          (!check_relink ||
           gtarget->NeedRelinkBeforeInstall(lg->GetConfigName()))) {
        std::string tname = lg->GetRelativeTargetDirectory(gtarget);
        tname += "/";
        tname += pass;
        depends.push_back(tname);
      }
    }
  }

  // The directory-level rule should depend on the directory-level
  // rules of the subdirectories.
  std::vector<cmStateSnapshot> children = lg->GetStateSnapshot().GetChildren();
  for (std::vector<cmStateSnapshot>::const_iterator ci = children.begin();
       ci != children.end(); ++ci) {
    std::string subdir = ci->GetDirectory().GetCurrentBinary();
    subdir += "/";
    subdir += pass;
    depends.push_back(subdir);
  }

  // Work-around for makes that drop rules that have no dependencies
  // or commands.
  if (depends.empty() && this->EmptyRuleHackDepends != "") {
    depends.push_back(this->EmptyRuleHackDepends);
  }

  // Write the rule.
  std::string doc = "Convenience name for \"";
  doc += pass;
  doc += "\" pass in the directory.";
  std::vector<std::string> no_commands;
  lg->WriteMakeRule(ruleFileStream, doc.c_str(), makeTarget, depends,
                    no_commands, true);
}

void cmGlobalUnixMakefileGenerator3::WriteDirectoryRules2(
  std::ostream& ruleFileStream, cmLocalUnixMakefileGenerator3* lg)
{
  // Only subdirectories need these rules.
  if (lg->IsRootMakefile()) {
    return;
  }

  // Begin the directory-level rules section.
  std::string dir = cmSystemTools::ConvertToOutputPath(
    lg->ConvertToRelativePath(lg->GetBinaryDirectory(),
                              lg->GetCurrentBinaryDirectory())
      .c_str());
  lg->WriteDivider(ruleFileStream);
  ruleFileStream << "# Directory level rules for directory " << dir << "\n\n";

  // Write directory-level rules for "all".
  this->WriteDirectoryRule2(ruleFileStream, lg, "all", true, false);

  // Write directory-level rules for "clean".
  this->WriteDirectoryRule2(ruleFileStream, lg, "clean", false, false);

  // Write directory-level rules for "preinstall".
  this->WriteDirectoryRule2(ruleFileStream, lg, "preinstall", true, true);
}

void cmGlobalUnixMakefileGenerator3::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& /*projectName*/, const std::string& /*projectDir*/,
  const std::string& targetName, const std::string& /*config*/, bool fast,
  bool /*verbose*/, std::vector<std::string> const& makeOptions)
{
  makeCommand.push_back(this->SelectMakeProgram(makeProgram));

  // Since we have full control over the invocation of nmake, let us
  // make it quiet.
  if (cmHasLiteralPrefix(this->GetName(), "NMake Makefiles")) {
    makeCommand.push_back("/NOLOGO");
  }
  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
  if (!targetName.empty()) {
    cmMakefile* mf;
    if (!this->Makefiles.empty()) {
      mf = this->Makefiles[0];
    } else {
      cmStateSnapshot snapshot = this->CMakeInstance->GetCurrentSnapshot();
      snapshot.GetDirectory().SetCurrentSource(
        this->CMakeInstance->GetHomeDirectory());
      snapshot.GetDirectory().SetCurrentBinary(
        this->CMakeInstance->GetHomeOutputDirectory());
      snapshot.SetDefaultDefinitions();
      mf = new cmMakefile(this, snapshot);
    }

    std::string tname = targetName;
    if (fast) {
      tname += "/fast";
    }
    cmOutputConverter conv(mf->GetStateSnapshot());
    tname =
      conv.ConvertToRelativePath(mf->GetState()->GetBinaryDirectory(), tname);
    cmSystemTools::ConvertToOutputSlashes(tname);
    makeCommand.push_back(tname);
    if (this->Makefiles.empty()) {
      delete mf;
    }
  }
}

void cmGlobalUnixMakefileGenerator3::WriteConvenienceRules(
  std::ostream& ruleFileStream, std::set<std::string>& emitted)
{
  std::vector<std::string> depends;
  std::vector<std::string> commands;

  depends.push_back("cmake_check_build_system");

  // write the target convenience rules
  unsigned int i;
  cmLocalUnixMakefileGenerator3* lg;
  for (i = 0; i < this->LocalGenerators.size(); ++i) {
    lg = static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[i]);
    // for each target Generate the rule files for each target.
    std::vector<cmGeneratorTarget*> targets = lg->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator t = targets.begin();
         t != targets.end(); ++t) {
      cmGeneratorTarget* gtarget = *t;
      // Don't emit the same rule twice (e.g. two targets with the same
      // simple name)
      int type = gtarget->GetType();
      std::string name = gtarget->GetName();
      if (!name.empty() && emitted.insert(name).second &&
          // Handle user targets here.  Global targets are handled in
          // the local generator on a per-directory basis.
          ((type == cmStateEnums::EXECUTABLE) ||
           (type == cmStateEnums::STATIC_LIBRARY) ||
           (type == cmStateEnums::SHARED_LIBRARY) ||
           (type == cmStateEnums::MODULE_LIBRARY) ||
           (type == cmStateEnums::OBJECT_LIBRARY) ||
           (type == cmStateEnums::UTILITY))) {
        // Add a rule to build the target by name.
        lg->WriteDivider(ruleFileStream);
        ruleFileStream << "# Target rules for targets named " << name
                       << "\n\n";

        // Write the rule.
        commands.clear();
        std::string tmp = cmake::GetCMakeFilesDirectoryPostSlash();
        tmp += "Makefile2";
        commands.push_back(lg->GetRecursiveMakeCall(tmp.c_str(), name));
        depends.clear();
        depends.push_back("cmake_check_build_system");
        lg->WriteMakeRule(ruleFileStream, "Build rule for target.", name,
                          depends, commands, true);

        // Add a fast rule to build the target
        std::string localName = lg->GetRelativeTargetDirectory(gtarget);
        std::string makefileName;
        makefileName = localName;
        makefileName += "/build.make";
        depends.clear();
        commands.clear();
        std::string makeTargetName = localName;
        makeTargetName += "/build";
        localName = name;
        localName += "/fast";
        commands.push_back(
          lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));
        lg->WriteMakeRule(ruleFileStream, "fast build rule for target.",
                          localName, depends, commands, true);

        // Add a local name for the rule to relink the target before
        // installation.
        if (gtarget->NeedRelinkBeforeInstall(lg->GetConfigName())) {
          makeTargetName = lg->GetRelativeTargetDirectory(gtarget);
          makeTargetName += "/preinstall";
          localName = name;
          localName += "/preinstall";
          depends.clear();
          commands.clear();
          commands.push_back(
            lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));
          lg->WriteMakeRule(ruleFileStream,
                            "Manual pre-install relink rule for target.",
                            localName, depends, commands, true);
        }
      }
    }
  }
}

void cmGlobalUnixMakefileGenerator3::WriteConvenienceRules2(
  std::ostream& ruleFileStream, cmLocalUnixMakefileGenerator3* lg)
{
  std::vector<std::string> depends;
  std::vector<std::string> commands;
  std::string localName;
  std::string makeTargetName;

  // write the directory level rules for this local gen
  this->WriteDirectoryRules2(ruleFileStream, lg);

  depends.push_back("cmake_check_build_system");

  // for each target Generate the rule files for each target.
  std::vector<cmGeneratorTarget*> targets = lg->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator t = targets.begin();
       t != targets.end(); ++t) {
    cmGeneratorTarget* gtarget = *t;
    int type = gtarget->GetType();
    std::string name = gtarget->GetName();
    if (!name.empty() && ((type == cmStateEnums::EXECUTABLE) ||
                          (type == cmStateEnums::STATIC_LIBRARY) ||
                          (type == cmStateEnums::SHARED_LIBRARY) ||
                          (type == cmStateEnums::MODULE_LIBRARY) ||
                          (type == cmStateEnums::OBJECT_LIBRARY) ||
                          (type == cmStateEnums::UTILITY))) {
      std::string makefileName;
      // Add a rule to build the target by name.
      localName = lg->GetRelativeTargetDirectory(gtarget);
      makefileName = localName;
      makefileName += "/build.make";

      bool needRequiresStep = this->NeedRequiresStep(gtarget);

      lg->WriteDivider(ruleFileStream);
      ruleFileStream << "# Target rules for target " << localName << "\n\n";

      commands.clear();
      makeTargetName = localName;
      makeTargetName += "/depend";
      commands.push_back(
        lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));

      // add requires if we need it for this generator
      if (needRequiresStep) {
        makeTargetName = localName;
        makeTargetName += "/requires";
        commands.push_back(
          lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));
      }
      makeTargetName = localName;
      makeTargetName += "/build";
      commands.push_back(
        lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));

      // Write the rule.
      localName += "/all";
      depends.clear();

      cmLocalUnixMakefileGenerator3::EchoProgress progress;
      progress.Dir = lg->GetBinaryDirectory();
      progress.Dir += cmake::GetCMakeFilesDirectory();
      {
        std::ostringstream progressArg;
        const char* sep = "";
        std::vector<unsigned long>& progFiles =
          this->ProgressMap[gtarget].Marks;
        for (std::vector<unsigned long>::iterator i = progFiles.begin();
             i != progFiles.end(); ++i) {
          progressArg << sep << *i;
          sep = ",";
        }
        progress.Arg = progressArg.str();
      }

      bool targetMessages = true;
      if (const char* tgtMsg =
            this->GetCMakeInstance()->GetState()->GetGlobalProperty(
              "TARGET_MESSAGES")) {
        targetMessages = cmSystemTools::IsOn(tgtMsg);
      }

      if (targetMessages) {
        lg->AppendEcho(commands, "Built target " + name,
                       cmLocalUnixMakefileGenerator3::EchoNormal, &progress);
      }

      this->AppendGlobalTargetDepends(depends, gtarget);
      lg->WriteMakeRule(ruleFileStream, "All Build rule for target.",
                        localName, depends, commands, true);

      // add the all/all dependency
      if (!this->IsExcluded(this->LocalGenerators[0], gtarget)) {
        depends.clear();
        depends.push_back(localName);
        commands.clear();
        lg->WriteMakeRule(ruleFileStream, "Include target in all.", "all",
                          depends, commands, true);
      }

      // Write the rule.
      commands.clear();

      {
        // TODO: Convert the total progress count to a make variable.
        std::ostringstream progCmd;
        progCmd << "$(CMAKE_COMMAND) -E cmake_progress_start ";
        // # in target
        progCmd << lg->ConvertToOutputFormat(
          cmSystemTools::CollapseFullPath(progress.Dir),
          cmOutputConverter::SHELL);
        //
        std::set<cmGeneratorTarget const*> emitted;
        progCmd << " " << this->CountProgressMarksInTarget(gtarget, emitted);
        commands.push_back(progCmd.str());
      }
      std::string tmp = cmake::GetCMakeFilesDirectoryPostSlash();
      tmp += "Makefile2";
      commands.push_back(lg->GetRecursiveMakeCall(tmp.c_str(), localName));
      {
        std::ostringstream progCmd;
        progCmd << "$(CMAKE_COMMAND) -E cmake_progress_start "; // # 0
        progCmd << lg->ConvertToOutputFormat(
          cmSystemTools::CollapseFullPath(progress.Dir),
          cmOutputConverter::SHELL);
        progCmd << " 0";
        commands.push_back(progCmd.str());
      }
      depends.clear();
      depends.push_back("cmake_check_build_system");
      localName = lg->GetRelativeTargetDirectory(gtarget);
      localName += "/rule";
      lg->WriteMakeRule(ruleFileStream,
                        "Build rule for subdir invocation for target.",
                        localName, depends, commands, true);

      // Add a target with the canonical name (no prefix, suffix or path).
      commands.clear();
      depends.clear();
      depends.push_back(localName);
      lg->WriteMakeRule(ruleFileStream, "Convenience name for target.", name,
                        depends, commands, true);

      // Add rules to prepare the target for installation.
      if (gtarget->NeedRelinkBeforeInstall(lg->GetConfigName())) {
        localName = lg->GetRelativeTargetDirectory(gtarget);
        localName += "/preinstall";
        depends.clear();
        commands.clear();
        commands.push_back(
          lg->GetRecursiveMakeCall(makefileName.c_str(), localName));
        lg->WriteMakeRule(ruleFileStream,
                          "Pre-install relink rule for target.", localName,
                          depends, commands, true);

        if (!this->IsExcluded(this->LocalGenerators[0], gtarget)) {
          depends.clear();
          depends.push_back(localName);
          commands.clear();
          lg->WriteMakeRule(ruleFileStream, "Prepare target for install.",
                            "preinstall", depends, commands, true);
        }
      }

      // add the clean rule
      localName = lg->GetRelativeTargetDirectory(gtarget);
      makeTargetName = localName;
      makeTargetName += "/clean";
      depends.clear();
      commands.clear();
      commands.push_back(
        lg->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));
      lg->WriteMakeRule(ruleFileStream, "clean rule for target.",
                        makeTargetName, depends, commands, true);
      commands.clear();
      depends.push_back(makeTargetName);
      lg->WriteMakeRule(ruleFileStream, "clean rule for target.", "clean",
                        depends, commands, true);
    }
  }
}

// Build a map that contains a the set of targets used by each local
// generator directory level.
void cmGlobalUnixMakefileGenerator3::InitializeProgressMarks()
{
  this->DirectoryTargetsMap.clear();
  // Loop over all targets in all local generators.
  for (std::vector<cmLocalGenerator*>::const_iterator lgi =
         this->LocalGenerators.begin();
       lgi != this->LocalGenerators.end(); ++lgi) {
    cmLocalGenerator* lg = *lgi;
    std::vector<cmGeneratorTarget*> targets = lg->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::const_iterator t = targets.begin();
         t != targets.end(); ++t) {
      cmGeneratorTarget* gt = *t;

      cmLocalGenerator* tlg = gt->GetLocalGenerator();

      if (gt->GetType() == cmStateEnums::INTERFACE_LIBRARY ||
          gt->GetPropertyAsBool("EXCLUDE_FROM_ALL")) {
        continue;
      }

      cmStateSnapshot csnp = lg->GetStateSnapshot();
      cmStateSnapshot tsnp = tlg->GetStateSnapshot();

      // Consider the directory containing the target and all its
      // parents until something excludes the target.
      for (; csnp.IsValid() && !this->IsExcluded(csnp, tsnp);
           csnp = csnp.GetBuildsystemDirectoryParent()) {
        // This local generator includes the target.
        std::set<cmGeneratorTarget const*>& targetSet =
          this->DirectoryTargetsMap[csnp];
        targetSet.insert(gt);

        // Add dependencies of the included target.  An excluded
        // target may still be included if it is a dependency of a
        // non-excluded target.
        TargetDependSet const& tgtdeps = this->GetTargetDirectDepends(gt);
        for (TargetDependSet::const_iterator ti = tgtdeps.begin();
             ti != tgtdeps.end(); ++ti) {
          targetSet.insert(*ti);
        }
      }
    }
  }
}

size_t cmGlobalUnixMakefileGenerator3::CountProgressMarksInTarget(
  cmGeneratorTarget const* target, std::set<cmGeneratorTarget const*>& emitted)
{
  size_t count = 0;
  if (emitted.insert(target).second) {
    count = this->ProgressMap[target].Marks.size();
    TargetDependSet const& depends = this->GetTargetDirectDepends(target);
    for (TargetDependSet::const_iterator di = depends.begin();
         di != depends.end(); ++di) {
      if ((*di)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        continue;
      }
      count += this->CountProgressMarksInTarget(*di, emitted);
    }
  }
  return count;
}

size_t cmGlobalUnixMakefileGenerator3::CountProgressMarksInAll(
  cmLocalGenerator* lg)
{
  size_t count = 0;
  std::set<cmGeneratorTarget const*> emitted;
  std::set<cmGeneratorTarget const*> const& targets =
    this->DirectoryTargetsMap[lg->GetStateSnapshot()];
  for (std::set<cmGeneratorTarget const*>::const_iterator t = targets.begin();
       t != targets.end(); ++t) {
    count += this->CountProgressMarksInTarget(*t, emitted);
  }
  return count;
}

void cmGlobalUnixMakefileGenerator3::RecordTargetProgress(
  cmMakefileTargetGenerator* tg)
{
  TargetProgress& tp = this->ProgressMap[tg->GetGeneratorTarget()];
  tp.NumberOfActions = tg->GetNumberOfProgressActions();
  tp.VariableFile = tg->GetProgressFileNameFull();
}

void cmGlobalUnixMakefileGenerator3::TargetProgress::WriteProgressVariables(
  unsigned long total, unsigned long& current)
{
  cmGeneratedFileStream fout(this->VariableFile.c_str());
  for (unsigned long i = 1; i <= this->NumberOfActions; ++i) {
    fout << "CMAKE_PROGRESS_" << i << " = ";
    if (total <= 100) {
      unsigned long num = i + current;
      fout << num;
      this->Marks.push_back(num);
    } else if (((i + current) * 100) / total >
               ((i - 1 + current) * 100) / total) {
      unsigned long num = ((i + current) * 100) / total;
      fout << num;
      this->Marks.push_back(num);
    }
    fout << "\n";
  }
  fout << "\n";
  current += this->NumberOfActions;
}

void cmGlobalUnixMakefileGenerator3::AppendGlobalTargetDepends(
  std::vector<std::string>& depends, cmGeneratorTarget* target)
{
  TargetDependSet const& depends_set = this->GetTargetDirectDepends(target);
  for (TargetDependSet::const_iterator i = depends_set.begin();
       i != depends_set.end(); ++i) {
    // Create the target-level dependency.
    cmGeneratorTarget const* dep = *i;
    if (dep->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    cmLocalUnixMakefileGenerator3* lg3 =
      static_cast<cmLocalUnixMakefileGenerator3*>(dep->GetLocalGenerator());
    std::string tgtName =
      lg3->GetRelativeTargetDirectory(const_cast<cmGeneratorTarget*>(dep));
    tgtName += "/all";
    depends.push_back(tgtName);
  }
}

void cmGlobalUnixMakefileGenerator3::WriteHelpRule(
  std::ostream& ruleFileStream, cmLocalUnixMakefileGenerator3* lg)
{
  // add the help target
  std::string path;
  std::vector<std::string> no_depends;
  std::vector<std::string> commands;
  lg->AppendEcho(commands, "The following are some of the valid targets "
                           "for this Makefile:");
  lg->AppendEcho(commands, "... all (the default if no target is provided)");
  lg->AppendEcho(commands, "... clean");
  lg->AppendEcho(commands, "... depend");

  // Keep track of targets already listed.
  std::set<std::string> emittedTargets;

  // for each local generator
  unsigned int i;
  cmLocalUnixMakefileGenerator3* lg2;
  for (i = 0; i < this->LocalGenerators.size(); ++i) {
    lg2 =
      static_cast<cmLocalUnixMakefileGenerator3*>(this->LocalGenerators[i]);
    // for the passed in makefile or if this is the top Makefile wripte out
    // the targets
    if (lg2 == lg || lg->IsRootMakefile()) {
      // for each target Generate the rule files for each target.
      std::vector<cmGeneratorTarget*> targets = lg2->GetGeneratorTargets();
      for (std::vector<cmGeneratorTarget*>::iterator t = targets.begin();
           t != targets.end(); ++t) {
        cmGeneratorTarget* target = *t;
        cmStateEnums::TargetType type = target->GetType();
        if ((type == cmStateEnums::EXECUTABLE) ||
            (type == cmStateEnums::STATIC_LIBRARY) ||
            (type == cmStateEnums::SHARED_LIBRARY) ||
            (type == cmStateEnums::MODULE_LIBRARY) ||
            (type == cmStateEnums::OBJECT_LIBRARY) ||
            (type == cmStateEnums::GLOBAL_TARGET) ||
            (type == cmStateEnums::UTILITY)) {
          std::string const& name = target->GetName();
          if (emittedTargets.insert(name).second) {
            path = "... ";
            path += name;
            lg->AppendEcho(commands, path);
          }
        }
      }
    }
  }
  std::vector<std::string> const& localHelp = lg->GetLocalHelp();
  for (std::vector<std::string>::const_iterator o = localHelp.begin();
       o != localHelp.end(); ++o) {
    path = "... ";
    path += *o;
    lg->AppendEcho(commands, path);
  }
  lg->WriteMakeRule(ruleFileStream, "Help Target", "help", no_depends,
                    commands, true);
  ruleFileStream << "\n\n";
}

bool cmGlobalUnixMakefileGenerator3::NeedRequiresStep(
  const cmGeneratorTarget* target)
{
  std::set<std::string> languages;
  target->GetLanguages(
    languages,
    target->Target->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE"));
  for (std::set<std::string>::const_iterator l = languages.begin();
       l != languages.end(); ++l) {
    std::string var = "CMAKE_NEEDS_REQUIRES_STEP_";
    var += *l;
    var += "_FLAG";
    if (target->Target->GetMakefile()->GetDefinition(var)) {
      return true;
    }
  }
  return false;
}
