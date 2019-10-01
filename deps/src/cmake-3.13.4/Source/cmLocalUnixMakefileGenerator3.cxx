/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalUnixMakefileGenerator3.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Terminal.h"
#include <algorithm>
#include <memory> // IWYU pragma: keep
#include <sstream>
#include <stdio.h>
#include <utility>

#include "cmAlgorithms.h"
#include "cmCustomCommandGenerator.h"
#include "cmFileTimeComparison.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmMakefileTargetGenerator.h"
#include "cmOutputConverter.h"
#include "cmRulePlaceholderExpander.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmake.h"

// Include dependency scanners for supported languages.  Only the
// C/C++ scanner is needed for bootstrapping CMake.
#include "cmDependsC.h"
#ifdef CMAKE_BUILD_WITH_CMAKE
#include "cmDependsFortran.h"
#endif

// Escape special characters in Makefile dependency lines
class cmMakeSafe
{
public:
  cmMakeSafe(const char* s)
    : Data(s)
  {
  }
  cmMakeSafe(std::string const& s)
    : Data(s.c_str())
  {
  }

private:
  const char* Data;
  friend std::ostream& operator<<(std::ostream& os, cmMakeSafe const& self)
  {
    for (const char* c = self.Data; *c; ++c) {
      switch (*c) {
        case '=':
          os << "$(EQUALS)";
          break;
        default:
          os << *c;
          break;
      }
    }
    return os;
  }
};

// Helper function used below.
static std::string cmSplitExtension(std::string const& in, std::string& base)
{
  std::string ext;
  std::string::size_type dot_pos = in.rfind('.');
  if (dot_pos != std::string::npos) {
    // Remove the extension first in case &base == &in.
    ext = in.substr(dot_pos);
    base = in.substr(0, dot_pos);
  } else {
    base = in;
  }
  return ext;
}

cmLocalUnixMakefileGenerator3::cmLocalUnixMakefileGenerator3(
  cmGlobalGenerator* gg, cmMakefile* mf)
  : cmLocalCommonGenerator(gg, mf, mf->GetCurrentBinaryDirectory())
{
  this->MakefileVariableSize = 0;
  this->ColorMakefile = false;
  this->SkipPreprocessedSourceRules = false;
  this->SkipAssemblySourceRules = false;
  this->MakeCommandEscapeTargetTwice = false;
  this->BorlandMakeCurlyHack = false;
}

cmLocalUnixMakefileGenerator3::~cmLocalUnixMakefileGenerator3()
{
}

void cmLocalUnixMakefileGenerator3::Generate()
{
  // Record whether some options are enabled to avoid checking many
  // times later.
  if (!this->GetGlobalGenerator()->GetCMakeInstance()->GetIsInTryCompile()) {
    this->ColorMakefile = this->Makefile->IsOn("CMAKE_COLOR_MAKEFILE");
  }
  this->SkipPreprocessedSourceRules =
    this->Makefile->IsOn("CMAKE_SKIP_PREPROCESSED_SOURCE_RULES");
  this->SkipAssemblySourceRules =
    this->Makefile->IsOn("CMAKE_SKIP_ASSEMBLY_SOURCE_RULES");

  // Generate the rule files for each target.
  const std::vector<cmGeneratorTarget*>& targets = this->GetGeneratorTargets();
  cmGlobalUnixMakefileGenerator3* gg =
    static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
  for (cmGeneratorTarget* target : targets) {
    if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    std::unique_ptr<cmMakefileTargetGenerator> tg(
      cmMakefileTargetGenerator::New(target));
    if (tg) {
      tg->WriteRuleFiles();
      gg->RecordTargetProgress(tg.get());
    }
  }

  // write the local Makefile
  this->WriteLocalMakefile();

  // Write the cmake file with information for this directory.
  this->WriteDirectoryInformationFile();
}

void cmLocalUnixMakefileGenerator3::ComputeHomeRelativeOutputPath()
{
  // Compute the path to use when referencing the current output
  // directory from the top output directory.
  this->HomeRelativeOutputPath = this->MaybeConvertToRelativePath(
    this->GetBinaryDirectory(), this->GetCurrentBinaryDirectory());
  if (this->HomeRelativeOutputPath == ".") {
    this->HomeRelativeOutputPath.clear();
  }
  if (!this->HomeRelativeOutputPath.empty()) {
    this->HomeRelativeOutputPath += "/";
  }
}

void cmLocalUnixMakefileGenerator3::GetLocalObjectFiles(
  std::map<std::string, LocalObjectInfo>& localObjectFiles)
{
  const std::vector<cmGeneratorTarget*>& targets = this->GetGeneratorTargets();
  for (cmGeneratorTarget* gt : targets) {
    if (gt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    std::vector<cmSourceFile const*> objectSources;
    gt->GetObjectSources(
      objectSources, this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
    // Compute full path to object file directory for this target.
    std::string dir;
    dir += gt->LocalGenerator->GetCurrentBinaryDirectory();
    dir += "/";
    dir += this->GetTargetDirectory(gt);
    dir += "/";
    // Compute the name of each object file.
    for (cmSourceFile const* sf : objectSources) {
      bool hasSourceExtension = true;
      std::string objectName =
        this->GetObjectFileNameWithoutTarget(*sf, dir, &hasSourceExtension);
      if (cmSystemTools::FileIsFullPath(objectName)) {
        objectName = cmSystemTools::GetFilenameName(objectName);
      }
      LocalObjectInfo& info = localObjectFiles[objectName];
      info.HasSourceExtension = hasSourceExtension;
      info.emplace_back(gt, sf->GetLanguage());
    }
  }
}

void cmLocalUnixMakefileGenerator3::GetIndividualFileTargets(
  std::vector<std::string>& targets)
{
  std::map<std::string, LocalObjectInfo> localObjectFiles;
  this->GetLocalObjectFiles(localObjectFiles);
  for (auto const& localObjectFile : localObjectFiles) {
    targets.push_back(localObjectFile.first);

    std::string::size_type dot_pos = localObjectFile.first.rfind(".");
    std::string base = localObjectFile.first.substr(0, dot_pos);
    if (localObjectFile.second.HasPreprocessRule) {
      targets.push_back(base + ".i");
    }

    if (localObjectFile.second.HasAssembleRule) {
      targets.push_back(base + ".s");
    }
  }
}

void cmLocalUnixMakefileGenerator3::WriteLocalMakefile()
{
  // generate the includes
  std::string ruleFileName = "Makefile";

  // Open the rule file.  This should be copy-if-different because the
  // rules may depend on this file itself.
  std::string ruleFileNameFull = this->ConvertToFullPath(ruleFileName);
  cmGeneratedFileStream ruleFileStream(
    ruleFileNameFull, false, this->GlobalGenerator->GetMakefileEncoding());
  if (!ruleFileStream) {
    return;
  }
  // always write the top makefile
  if (!this->IsRootMakefile()) {
    ruleFileStream.SetCopyIfDifferent(true);
  }

  // write the all rules
  this->WriteLocalAllRules(ruleFileStream);

  // only write local targets unless at the top Keep track of targets already
  // listed.
  std::set<std::string> emittedTargets;
  if (!this->IsRootMakefile()) {
    // write our targets, and while doing it collect up the object
    // file rules
    this->WriteLocalMakefileTargets(ruleFileStream, emittedTargets);
  } else {
    cmGlobalUnixMakefileGenerator3* gg =
      static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
    gg->WriteConvenienceRules(ruleFileStream, emittedTargets);
  }

  bool do_preprocess_rules = this->GetCreatePreprocessedSourceRules();
  bool do_assembly_rules = this->GetCreateAssemblySourceRules();

  std::map<std::string, LocalObjectInfo> localObjectFiles;
  this->GetLocalObjectFiles(localObjectFiles);

  // now write out the object rules
  // for each object file name
  for (auto& localObjectFile : localObjectFiles) {
    // Add a convenience rule for building the object file.
    this->WriteObjectConvenienceRule(
      ruleFileStream, "target to build an object file",
      localObjectFile.first.c_str(), localObjectFile.second);

    // Check whether preprocessing and assembly rules make sense.
    // They make sense only for C and C++ sources.
    bool lang_has_preprocessor = false;
    bool lang_has_assembly = false;

    for (LocalObjectEntry const& entry : localObjectFile.second) {
      if (entry.Language == "C" || entry.Language == "CXX" ||
          entry.Language == "CUDA" || entry.Language == "Fortran") {
        // Right now, C, C++, Fortran and CUDA have both a preprocessor and the
        // ability to generate assembly code
        lang_has_preprocessor = true;
        lang_has_assembly = true;
        break;
      }
    }

    // Add convenience rules for preprocessed and assembly files.
    if (lang_has_preprocessor && do_preprocess_rules) {
      std::string::size_type dot_pos = localObjectFile.first.rfind(".");
      std::string base = localObjectFile.first.substr(0, dot_pos);
      this->WriteObjectConvenienceRule(
        ruleFileStream, "target to preprocess a source file",
        (base + ".i").c_str(), localObjectFile.second);
      localObjectFile.second.HasPreprocessRule = true;
    }

    if (lang_has_assembly && do_assembly_rules) {
      std::string::size_type dot_pos = localObjectFile.first.rfind(".");
      std::string base = localObjectFile.first.substr(0, dot_pos);
      this->WriteObjectConvenienceRule(
        ruleFileStream, "target to generate assembly for a file",
        (base + ".s").c_str(), localObjectFile.second);
      localObjectFile.second.HasAssembleRule = true;
    }
  }

  // add a help target as long as there isn;t a real target named help
  if (emittedTargets.insert("help").second) {
    cmGlobalUnixMakefileGenerator3* gg =
      static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
    gg->WriteHelpRule(ruleFileStream, this);
  }

  this->WriteSpecialTargetsBottom(ruleFileStream);
}

void cmLocalUnixMakefileGenerator3::WriteObjectConvenienceRule(
  std::ostream& ruleFileStream, const char* comment, const char* output,
  LocalObjectInfo const& info)
{
  // If the rule includes the source file extension then create a
  // version that has the extension removed.  The help should include
  // only the version without source extension.
  bool inHelp = true;
  if (info.HasSourceExtension) {
    // Remove the last extension.  This should be kept.
    std::string outBase1 = output;
    std::string outExt1 = cmSplitExtension(outBase1, outBase1);

    // Now remove the source extension and put back the last
    // extension.
    std::string outNoExt;
    cmSplitExtension(outBase1, outNoExt);
    outNoExt += outExt1;

    // Add a rule to drive the rule below.
    std::vector<std::string> depends;
    depends.push_back(output);
    std::vector<std::string> no_commands;
    this->WriteMakeRule(ruleFileStream, nullptr, outNoExt, depends,
                        no_commands, true, true);
    inHelp = false;
  }

  // Recursively make the rule for each target using the object file.
  std::vector<std::string> commands;
  for (LocalObjectEntry const& t : info) {
    std::string tgtMakefileName = this->GetRelativeTargetDirectory(t.Target);
    std::string targetName = tgtMakefileName;
    tgtMakefileName += "/build.make";
    targetName += "/";
    targetName += output;
    commands.push_back(
      this->GetRecursiveMakeCall(tgtMakefileName.c_str(), targetName));
  }
  this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                        this->GetCurrentBinaryDirectory());

  // Write the rule to the makefile.
  std::vector<std::string> no_depends;
  this->WriteMakeRule(ruleFileStream, comment, output, no_depends, commands,
                      true, inHelp);
}

void cmLocalUnixMakefileGenerator3::WriteLocalMakefileTargets(
  std::ostream& ruleFileStream, std::set<std::string>& emitted)
{
  std::vector<std::string> depends;
  std::vector<std::string> commands;

  // for each target we just provide a rule to cd up to the top and do a make
  // on the target
  const std::vector<cmGeneratorTarget*>& targets = this->GetGeneratorTargets();
  std::string localName;
  for (cmGeneratorTarget* target : targets) {
    if ((target->GetType() == cmStateEnums::EXECUTABLE) ||
        (target->GetType() == cmStateEnums::STATIC_LIBRARY) ||
        (target->GetType() == cmStateEnums::SHARED_LIBRARY) ||
        (target->GetType() == cmStateEnums::MODULE_LIBRARY) ||
        (target->GetType() == cmStateEnums::OBJECT_LIBRARY) ||
        (target->GetType() == cmStateEnums::UTILITY)) {
      emitted.insert(target->GetName());

      // for subdirs add a rule to build this specific target by name.
      localName = this->GetRelativeTargetDirectory(target);
      localName += "/rule";
      commands.clear();
      depends.clear();

      // Build the target for this pass.
      std::string makefile2 = cmake::GetCMakeFilesDirectoryPostSlash();
      makefile2 += "Makefile2";
      commands.push_back(
        this->GetRecursiveMakeCall(makefile2.c_str(), localName));
      this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                            this->GetCurrentBinaryDirectory());
      this->WriteMakeRule(ruleFileStream, "Convenience name for target.",
                          localName, depends, commands, true);

      // Add a target with the canonical name (no prefix, suffix or path).
      if (localName != target->GetName()) {
        commands.clear();
        depends.push_back(localName);
        this->WriteMakeRule(ruleFileStream, "Convenience name for target.",
                            target->GetName(), depends, commands, true);
      }

      // Add a fast rule to build the target
      std::string makefileName = this->GetRelativeTargetDirectory(target);
      makefileName += "/build.make";
      // make sure the makefile name is suitable for a makefile
      std::string makeTargetName = this->GetRelativeTargetDirectory(target);
      makeTargetName += "/build";
      localName = target->GetName();
      localName += "/fast";
      depends.clear();
      commands.clear();
      commands.push_back(
        this->GetRecursiveMakeCall(makefileName.c_str(), makeTargetName));
      this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                            this->GetCurrentBinaryDirectory());
      this->WriteMakeRule(ruleFileStream, "fast build rule for target.",
                          localName, depends, commands, true);

      // Add a local name for the rule to relink the target before
      // installation.
      if (target->NeedRelinkBeforeInstall(this->ConfigName)) {
        makeTargetName = this->GetRelativeTargetDirectory(target);
        makeTargetName += "/preinstall";
        localName = target->GetName();
        localName += "/preinstall";
        depends.clear();
        commands.clear();
        commands.push_back(
          this->GetRecursiveMakeCall(makefile2.c_str(), makeTargetName));
        this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                              this->GetCurrentBinaryDirectory());
        this->WriteMakeRule(ruleFileStream,
                            "Manual pre-install relink rule for target.",
                            localName, depends, commands, true);
      }
    }
  }
}

void cmLocalUnixMakefileGenerator3::WriteDirectoryInformationFile()
{
  std::string infoFileName = this->GetCurrentBinaryDirectory();
  infoFileName += cmake::GetCMakeFilesDirectory();
  infoFileName += "/CMakeDirectoryInformation.cmake";

  // Open the output file.
  cmGeneratedFileStream infoFileStream(infoFileName);
  if (!infoFileStream) {
    return;
  }

  infoFileStream.SetCopyIfDifferent(true);
  // Write the do not edit header.
  this->WriteDisclaimer(infoFileStream);

  // Setup relative path conversion tops.
  /* clang-format off */
  infoFileStream
    << "# Relative path conversion top directories.\n"
    << "set(CMAKE_RELATIVE_PATH_TOP_SOURCE \""
    << this->StateSnapshot.GetDirectory().GetRelativePathTopSource()
    << "\")\n"
    << "set(CMAKE_RELATIVE_PATH_TOP_BINARY \""
    << this->StateSnapshot.GetDirectory().GetRelativePathTopBinary()
    << "\")\n"
    << "\n";
  /* clang-format on */

  // Tell the dependency scanner to use unix paths if necessary.
  if (cmSystemTools::GetForceUnixPaths()) {
    /* clang-format off */
    infoFileStream
      << "# Force unix paths in dependencies.\n"
      << "set(CMAKE_FORCE_UNIX_PATHS 1)\n"
      << "\n";
    /* clang-format on */
  }

  // Store the include regular expressions for this directory.
  infoFileStream << "\n"
                 << "# The C and CXX include file regular expressions for "
                 << "this directory.\n";
  infoFileStream << "set(CMAKE_C_INCLUDE_REGEX_SCAN ";
  this->WriteCMakeArgument(infoFileStream,
                           this->Makefile->GetIncludeRegularExpression());
  infoFileStream << ")\n";
  infoFileStream << "set(CMAKE_C_INCLUDE_REGEX_COMPLAIN ";
  this->WriteCMakeArgument(infoFileStream,
                           this->Makefile->GetComplainRegularExpression());
  infoFileStream << ")\n";
  infoFileStream
    << "set(CMAKE_CXX_INCLUDE_REGEX_SCAN ${CMAKE_C_INCLUDE_REGEX_SCAN})\n";
  infoFileStream << "set(CMAKE_CXX_INCLUDE_REGEX_COMPLAIN "
                    "${CMAKE_C_INCLUDE_REGEX_COMPLAIN})\n";
}

std::string cmLocalUnixMakefileGenerator3::ConvertToFullPath(
  const std::string& localPath)
{
  std::string dir = this->GetCurrentBinaryDirectory();
  dir += "/";
  dir += localPath;
  return dir;
}

const std::string& cmLocalUnixMakefileGenerator3::GetHomeRelativeOutputPath()
{
  return this->HomeRelativeOutputPath;
}

void cmLocalUnixMakefileGenerator3::WriteMakeRule(
  std::ostream& os, const char* comment, const std::string& target,
  const std::vector<std::string>& depends,
  const std::vector<std::string>& commands, bool symbolic, bool in_help)
{
  // Make sure there is a target.
  if (target.empty()) {
    cmSystemTools::Error("No target for WriteMakeRule! called with comment: ",
                         comment);
    return;
  }

  std::string replace;

  // Write the comment describing the rule in the makefile.
  if (comment) {
    replace = comment;
    std::string::size_type lpos = 0;
    std::string::size_type rpos;
    while ((rpos = replace.find('\n', lpos)) != std::string::npos) {
      os << "# " << replace.substr(lpos, rpos - lpos) << "\n";
      lpos = rpos + 1;
    }
    os << "# " << replace.substr(lpos) << "\n";
  }

  // Construct the left hand side of the rule.
  std::string tgt = cmSystemTools::ConvertToOutputPath(
    this->MaybeConvertToRelativePath(this->GetBinaryDirectory(), target));

  const char* space = "";
  if (tgt.size() == 1) {
    // Add a space before the ":" to avoid drive letter confusion on
    // Windows.
    space = " ";
  }

  // Mark the rule as symbolic if requested.
  if (symbolic) {
    if (const char* sym =
          this->Makefile->GetDefinition("CMAKE_MAKE_SYMBOLIC_RULE")) {
      os << cmMakeSafe(tgt) << space << ": " << sym << "\n";
    }
  }

  // Write the rule.
  if (depends.empty()) {
    // No dependencies.  The commands will always run.
    os << cmMakeSafe(tgt) << space << ":\n";
  } else {
    // Split dependencies into multiple rule lines.  This allows for
    // very long dependency lists even on older make implementations.
    std::string binDir = this->GetBinaryDirectory();
    for (std::string const& depend : depends) {
      replace = depend;
      replace = cmSystemTools::ConvertToOutputPath(
        this->MaybeConvertToRelativePath(binDir, replace));
      os << cmMakeSafe(tgt) << space << ": " << cmMakeSafe(replace) << "\n";
    }
  }

  // Write the list of commands.
  os << cmWrap("\t", commands, "", "\n") << "\n";
  if (symbolic && !this->IsWatcomWMake()) {
    os << ".PHONY : " << cmMakeSafe(tgt) << "\n";
  }
  os << "\n";
  // Add the output to the local help if requested.
  if (in_help) {
    this->LocalHelp.push_back(target);
  }
}

std::string cmLocalUnixMakefileGenerator3::MaybeConvertWatcomShellCommand(
  std::string const& cmd)
{
  if (this->IsWatcomWMake() && cmSystemTools::FileIsFullPath(cmd) &&
      cmd.find_first_of("( )") != std::string::npos) {
    // On Watcom WMake use the windows short path for the command
    // name.  This is needed to avoid funny quoting problems on
    // lines with shell redirection operators.
    std::string scmd;
    if (cmSystemTools::GetShortPath(cmd, scmd)) {
      return this->ConvertToOutputFormat(scmd, cmOutputConverter::SHELL);
    }
  }
  return std::string();
}

void cmLocalUnixMakefileGenerator3::WriteMakeVariables(
  std::ostream& makefileStream)
{
  this->WriteDivider(makefileStream);
  makefileStream << "# Set environment variables for the build.\n"
                 << "\n";
  cmGlobalUnixMakefileGenerator3* gg =
    static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
  if (gg->DefineWindowsNULL) {
    makefileStream << "!IF \"$(OS)\" == \"Windows_NT\"\n"
                   << "NULL=\n"
                   << "!ELSE\n"
                   << "NULL=nul\n"
                   << "!ENDIF\n";
  }
  if (this->IsWindowsShell()) {
    makefileStream << "SHELL = cmd.exe\n"
                   << "\n";
  } else {
#if !defined(__VMS)
    /* clang-format off */
      makefileStream
        << "# The shell in which to execute make rules.\n"
        << "SHELL = /bin/sh\n"
        << "\n";
/* clang-format on */
#endif
  }

  std::string cmakeShellCommand =
    this->MaybeConvertWatcomShellCommand(cmSystemTools::GetCMakeCommand());
  if (cmakeShellCommand.empty()) {
    cmakeShellCommand = this->ConvertToOutputFormat(
      cmSystemTools::CollapseFullPath(cmSystemTools::GetCMakeCommand()),
      cmOutputConverter::SHELL);
  }

  /* clang-format off */
  makefileStream
    << "# The CMake executable.\n"
    << "CMAKE_COMMAND = "
    << cmakeShellCommand
    << "\n"
    << "\n";
  makefileStream
    << "# The command to remove a file.\n"
    << "RM = "
    << cmakeShellCommand
    << " -E remove -f\n"
    << "\n";
  makefileStream
    << "# Escaping for special characters.\n"
    << "EQUALS = =\n"
    << "\n";
  makefileStream
    << "# The top-level source directory on which CMake was run.\n"
    << "CMAKE_SOURCE_DIR = "
    << this->ConvertToOutputFormat(
      cmSystemTools::CollapseFullPath(this->GetSourceDirectory()),
                     cmOutputConverter::SHELL)
    << "\n"
    << "\n";
  makefileStream
    << "# The top-level build directory on which CMake was run.\n"
    << "CMAKE_BINARY_DIR = "
    << this->ConvertToOutputFormat(
      cmSystemTools::CollapseFullPath(this->GetBinaryDirectory()),
                     cmOutputConverter::SHELL)
    << "\n"
    << "\n";
  /* clang-format on */
}

void cmLocalUnixMakefileGenerator3::WriteSpecialTargetsTop(
  std::ostream& makefileStream)
{
  this->WriteDivider(makefileStream);
  makefileStream << "# Special targets provided by cmake.\n"
                 << "\n";

  std::vector<std::string> no_commands;
  std::vector<std::string> no_depends;

  // Special target to cleanup operation of make tool.
  // This should be the first target except for the default_target in
  // the interface Makefile.
  this->WriteMakeRule(makefileStream,
                      "Disable implicit rules so canonical targets will work.",
                      ".SUFFIXES", no_depends, no_commands, false);

  if (!this->IsNMake() && !this->IsWatcomWMake() &&
      !this->BorlandMakeCurlyHack) {
    // turn off RCS and SCCS automatic stuff from gmake
    makefileStream
      << "# Remove some rules from gmake that .SUFFIXES does not remove.\n"
      << "SUFFIXES =\n\n";
  }
  // Add a fake suffix to keep HP happy.  Must be max 32 chars for SGI make.
  std::vector<std::string> depends;
  depends.push_back(".hpux_make_needs_suffix_list");
  this->WriteMakeRule(makefileStream, nullptr, ".SUFFIXES", depends,
                      no_commands, false);
  if (this->IsWatcomWMake()) {
    // Switch on WMake feature, if an error or interrupt occurs during
    // makefile processing, the current target being made may be deleted
    // without prompting (the same as command line -e option).
    /* clang-format off */
    makefileStream <<
      "\n"
      ".ERASE\n"
      "\n"
      ;
    /* clang-format on */
  }
  if (this->Makefile->IsOn("CMAKE_VERBOSE_MAKEFILE")) {
    /* clang-format off */
    makefileStream
      << "# Produce verbose output by default.\n"
      << "VERBOSE = 1\n"
      << "\n";
    /* clang-format on */
  }
  if (this->IsWatcomWMake()) {
    /* clang-format off */
    makefileStream <<
      "!ifndef VERBOSE\n"
      ".SILENT\n"
      "!endif\n"
      "\n"
      ;
    /* clang-format on */
  } else {
    // Write special target to silence make output.  This must be after
    // the default target in case VERBOSE is set (which changes the
    // name).  The setting of CMAKE_VERBOSE_MAKEFILE to ON will cause a
    // "VERBOSE=1" to be added as a make variable which will change the
    // name of this special target.  This gives a make-time choice to
    // the user.
    this->WriteMakeRule(makefileStream,
                        "Suppress display of executed commands.",
                        "$(VERBOSE).SILENT", no_depends, no_commands, false);
  }

  // Work-around for makes that drop rules that have no dependencies
  // or commands.
  cmGlobalUnixMakefileGenerator3* gg =
    static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
  std::string hack = gg->GetEmptyRuleHackDepends();
  if (!hack.empty()) {
    no_depends.push_back(std::move(hack));
  }
  std::string hack_cmd = gg->GetEmptyRuleHackCommand();
  if (!hack_cmd.empty()) {
    no_commands.push_back(std::move(hack_cmd));
  }

  // Special symbolic target that never exists to force dependers to
  // run their rules.
  this->WriteMakeRule(makefileStream, "A target that is always out of date.",
                      "cmake_force", no_depends, no_commands, true);

  // Variables for reference by other rules.
  this->WriteMakeVariables(makefileStream);
}

void cmLocalUnixMakefileGenerator3::WriteSpecialTargetsBottom(
  std::ostream& makefileStream)
{
  this->WriteDivider(makefileStream);
  makefileStream << "# Special targets to cleanup operation of make.\n"
                 << "\n";

  // Write special "cmake_check_build_system" target to run cmake with
  // the --check-build-system flag.
  if (!this->GlobalGenerator->GlobalSettingIsOn(
        "CMAKE_SUPPRESS_REGENERATION")) {
    // Build command to run CMake to check if anything needs regenerating.
    std::vector<std::string> commands;
    cmake* cm = this->GlobalGenerator->GetCMakeInstance();
    if (cm->DoWriteGlobVerifyTarget()) {
      std::string rescanRule = "$(CMAKE_COMMAND) -P ";
      rescanRule += this->ConvertToOutputFormat(cm->GetGlobVerifyScript(),
                                                cmOutputConverter::SHELL);
      commands.push_back(rescanRule);
    }
    std::string cmakefileName = cmake::GetCMakeFilesDirectoryPostSlash();
    cmakefileName += "Makefile.cmake";
    std::string runRule =
      "$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)";
    runRule += " --check-build-system ";
    runRule +=
      this->ConvertToOutputFormat(cmakefileName, cmOutputConverter::SHELL);
    runRule += " 0";

    std::vector<std::string> no_depends;
    commands.push_back(std::move(runRule));
    if (!this->IsRootMakefile()) {
      this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                            this->GetCurrentBinaryDirectory());
    }
    this->WriteMakeRule(makefileStream,
                        "Special rule to run CMake to check the build system "
                        "integrity.\n"
                        "No rule that depends on this can have "
                        "commands that come from listfiles\n"
                        "because they might be regenerated.",
                        "cmake_check_build_system", no_depends, commands,
                        true);
  }
}

void cmLocalUnixMakefileGenerator3::WriteConvenienceRule(
  std::ostream& ruleFileStream, const std::string& realTarget,
  const std::string& helpTarget)
{
  // A rule is only needed if the names are different.
  if (realTarget != helpTarget) {
    // The helper target depends on the real target.
    std::vector<std::string> depends;
    depends.push_back(realTarget);

    // There are no commands.
    std::vector<std::string> no_commands;

    // Write the rule.
    this->WriteMakeRule(ruleFileStream, "Convenience name for target.",
                        helpTarget, depends, no_commands, true);
  }
}

std::string cmLocalUnixMakefileGenerator3::GetRelativeTargetDirectory(
  cmGeneratorTarget* target)
{
  std::string dir = this->HomeRelativeOutputPath;
  dir += this->GetTargetDirectory(target);
  return dir;
}

void cmLocalUnixMakefileGenerator3::AppendFlags(
  std::string& flags, const std::string& newFlags) const
{
  if (this->IsWatcomWMake() && !newFlags.empty()) {
    std::string newf = newFlags;
    if (newf.find("\\\"") != std::string::npos) {
      cmSystemTools::ReplaceString(newf, "\\\"", "\"");
      this->cmLocalGenerator::AppendFlags(flags, newf);
      return;
    }
  }
  this->cmLocalGenerator::AppendFlags(flags, newFlags);
}

void cmLocalUnixMakefileGenerator3::AppendFlags(std::string& flags,
                                                const char* newFlags) const
{
  this->cmLocalGenerator::AppendFlags(flags, newFlags);
}

void cmLocalUnixMakefileGenerator3::AppendRuleDepend(
  std::vector<std::string>& depends, const char* ruleFileName)
{
  // Add a dependency on the rule file itself unless an option to skip
  // it is specifically enabled by the user or project.
  const char* nodep =
    this->Makefile->GetDefinition("CMAKE_SKIP_RULE_DEPENDENCY");
  if (!nodep || cmSystemTools::IsOff(nodep)) {
    depends.push_back(ruleFileName);
  }
}

void cmLocalUnixMakefileGenerator3::AppendRuleDepends(
  std::vector<std::string>& depends, std::vector<std::string> const& ruleFiles)
{
  // Add a dependency on the rule file itself unless an option to skip
  // it is specifically enabled by the user or project.
  if (!this->Makefile->IsOn("CMAKE_SKIP_RULE_DEPENDENCY")) {
    depends.insert(depends.end(), ruleFiles.begin(), ruleFiles.end());
  }
}

void cmLocalUnixMakefileGenerator3::AppendCustomDepends(
  std::vector<std::string>& depends, const std::vector<cmCustomCommand>& ccs)
{
  for (cmCustomCommand const& cc : ccs) {
    cmCustomCommandGenerator ccg(cc, this->ConfigName, this);
    this->AppendCustomDepend(depends, ccg);
  }
}

void cmLocalUnixMakefileGenerator3::AppendCustomDepend(
  std::vector<std::string>& depends, cmCustomCommandGenerator const& ccg)
{
  for (std::string const& d : ccg.GetDepends()) {
    // Lookup the real name of the dependency in case it is a CMake target.
    std::string dep;
    if (this->GetRealDependency(d, this->ConfigName, dep)) {
      depends.push_back(std::move(dep));
    }
  }
}

void cmLocalUnixMakefileGenerator3::AppendCustomCommands(
  std::vector<std::string>& commands, const std::vector<cmCustomCommand>& ccs,
  cmGeneratorTarget* target, std::string const& relative)
{
  for (cmCustomCommand const& cc : ccs) {
    cmCustomCommandGenerator ccg(cc, this->ConfigName, this);
    this->AppendCustomCommand(commands, ccg, target, relative, true);
  }
}

void cmLocalUnixMakefileGenerator3::AppendCustomCommand(
  std::vector<std::string>& commands, cmCustomCommandGenerator const& ccg,
  cmGeneratorTarget* target, std::string const& relative, bool echo_comment,
  std::ostream* content)
{
  // Optionally create a command to display the custom command's
  // comment text.  This is used for pre-build, pre-link, and
  // post-build command comments.  Custom build step commands have
  // their comments generated elsewhere.
  if (echo_comment) {
    const char* comment = ccg.GetComment();
    if (comment && *comment) {
      this->AppendEcho(commands, comment,
                       cmLocalUnixMakefileGenerator3::EchoGenerate);
    }
  }

  // if the command specified a working directory use it.
  std::string dir = this->GetCurrentBinaryDirectory();
  std::string workingDir = ccg.GetWorkingDirectory();
  if (!workingDir.empty()) {
    dir = workingDir;
  }
  if (content) {
    *content << dir;
  }

  std::unique_ptr<cmRulePlaceholderExpander> rulePlaceholderExpander(
    this->CreateRulePlaceholderExpander());

  // Add each command line to the set of commands.
  std::vector<std::string> commands1;
  std::string currentBinDir = this->GetCurrentBinaryDirectory();
  for (unsigned int c = 0; c < ccg.GetNumberOfCommands(); ++c) {
    // Build the command line in a single string.
    std::string cmd = ccg.GetCommand(c);
    if (!cmd.empty()) {
      // Use "call " before any invocations of .bat or .cmd files
      // invoked as custom commands in the WindowsShell.
      //
      bool useCall = false;

      if (this->IsWindowsShell()) {
        std::string suffix;
        if (cmd.size() > 4) {
          suffix = cmSystemTools::LowerCase(cmd.substr(cmd.size() - 4));
          if (suffix == ".bat" || suffix == ".cmd") {
            useCall = true;
          }
        }
      }

      cmSystemTools::ReplaceString(cmd, "/./", "/");
      // Convert the command to a relative path only if the current
      // working directory will be the start-output directory.
      bool had_slash = cmd.find('/') != std::string::npos;
      if (workingDir.empty()) {
        cmd = this->MaybeConvertToRelativePath(currentBinDir, cmd);
      }
      bool has_slash = cmd.find('/') != std::string::npos;
      if (had_slash && !has_slash) {
        // This command was specified as a path to a file in the
        // current directory.  Add a leading "./" so it can run
        // without the current directory being in the search path.
        cmd = "./" + cmd;
      }

      std::string launcher;
      // Short-circuit if there is no launcher.
      const char* val = this->GetRuleLauncher(target, "RULE_LAUNCH_CUSTOM");
      if (val && *val) {
        // Expand rule variables referenced in the given launcher command.
        cmRulePlaceholderExpander::RuleVariables vars;
        vars.CMTargetName = target->GetName().c_str();
        vars.CMTargetType = cmState::GetTargetTypeName(target->GetType());
        std::string output;
        const std::vector<std::string>& outputs = ccg.GetOutputs();
        if (!outputs.empty()) {
          output = outputs[0];
          if (workingDir.empty()) {
            output = this->MaybeConvertToRelativePath(
              this->GetCurrentBinaryDirectory(), output);
          }
          output =
            this->ConvertToOutputFormat(output, cmOutputConverter::SHELL);
        }
        vars.Output = output.c_str();

        launcher = val;
        rulePlaceholderExpander->ExpandRuleVariables(this, launcher, vars);
        if (!launcher.empty()) {
          launcher += " ";
        }
      }

      std::string shellCommand = this->MaybeConvertWatcomShellCommand(cmd);
      if (shellCommand.empty()) {
        shellCommand =
          this->ConvertToOutputFormat(cmd, cmOutputConverter::SHELL);
      }
      cmd = launcher + shellCommand;

      ccg.AppendArguments(c, cmd);
      if (content) {
        // Rule content does not include the launcher.
        *content << (cmd.c_str() + launcher.size());
      }
      if (this->BorlandMakeCurlyHack) {
        // Borland Make has a very strange bug.  If the first curly
        // brace anywhere in the command string is a left curly, it
        // must be written {{} instead of just {.  Otherwise some
        // curly braces are removed.  The hack can be skipped if the
        // first curly brace is the last character.
        std::string::size_type lcurly = cmd.find('{');
        if (lcurly != std::string::npos && lcurly < (cmd.size() - 1)) {
          std::string::size_type rcurly = cmd.find('}');
          if (rcurly == std::string::npos || rcurly > lcurly) {
            // The first curly is a left curly.  Use the hack.
            std::string hack_cmd = cmd.substr(0, lcurly);
            hack_cmd += "{{}";
            hack_cmd += cmd.substr(lcurly + 1);
            cmd = hack_cmd;
          }
        }
      }
      if (launcher.empty()) {
        if (useCall) {
          cmd = "call " + cmd;
        } else if (this->IsNMake() && cmd[0] == '"') {
          cmd = "echo >nul && " + cmd;
        }
      }
      commands1.push_back(std::move(cmd));
    }
  }

  // Setup the proper working directory for the commands.
  this->CreateCDCommand(commands1, dir, relative);

  // push back the custom commands
  commands.insert(commands.end(), commands1.begin(), commands1.end());
}

void cmLocalUnixMakefileGenerator3::AppendCleanCommand(
  std::vector<std::string>& commands, const std::vector<std::string>& files,
  cmGeneratorTarget* target, const char* filename)
{
  std::string currentBinDir = this->GetCurrentBinaryDirectory();
  std::string cleanfile = currentBinDir;
  cleanfile += "/";
  cleanfile += this->GetTargetDirectory(target);
  cleanfile += "/cmake_clean";
  if (filename) {
    cleanfile += "_";
    cleanfile += filename;
  }
  cleanfile += ".cmake";
  std::string cleanfilePath = cmSystemTools::CollapseFullPath(cleanfile);
  cmsys::ofstream fout(cleanfilePath.c_str());
  if (!fout) {
    cmSystemTools::Error("Could not create ", cleanfilePath.c_str());
  }
  if (!files.empty()) {
    fout << "file(REMOVE_RECURSE\n";
    for (std::string const& file : files) {
      std::string fc = this->MaybeConvertToRelativePath(currentBinDir, file);
      fout << "  " << cmOutputConverter::EscapeForCMake(fc) << "\n";
    }
    fout << ")\n";
  }
  {
    std::string remove = "$(CMAKE_COMMAND) -P ";
    remove += this->ConvertToOutputFormat(
      this->MaybeConvertToRelativePath(this->GetCurrentBinaryDirectory(),
                                       cleanfile),
      cmOutputConverter::SHELL);
    commands.push_back(std::move(remove));
  }

  // For the main clean rule add per-language cleaning.
  if (!filename) {
    // Get the set of source languages in the target.
    std::set<std::string> languages;
    target->GetLanguages(
      languages, this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
    /* clang-format off */
    fout << "\n"
         << "# Per-language clean rules from dependency scanning.\n"
         << "foreach(lang " << cmJoin(languages, " ") << ")\n"
         << "  include(" << this->GetTargetDirectory(target)
         << "/cmake_clean_${lang}.cmake OPTIONAL)\n"
         << "endforeach()\n";
    /* clang-format on */
  }
}

void cmLocalUnixMakefileGenerator3::AppendEcho(
  std::vector<std::string>& commands, std::string const& text, EchoColor color,
  EchoProgress const* progress)
{
  // Choose the color for the text.
  std::string color_name;
  if (this->GlobalGenerator->GetToolSupportsColor() && this->ColorMakefile) {
    // See cmake::ExecuteEchoColor in cmake.cxx for these options.
    // This color set is readable on both black and white backgrounds.
    switch (color) {
      case EchoNormal:
        break;
      case EchoDepend:
        color_name = "--magenta --bold ";
        break;
      case EchoBuild:
        color_name = "--green ";
        break;
      case EchoLink:
        color_name = "--green --bold ";
        break;
      case EchoGenerate:
        color_name = "--blue --bold ";
        break;
      case EchoGlobal:
        color_name = "--cyan ";
        break;
    }
  }

  // Echo one line at a time.
  std::string line;
  line.reserve(200);
  for (const char* c = text.c_str();; ++c) {
    if (*c == '\n' || *c == '\0') {
      // Avoid writing a blank last line on end-of-string.
      if (*c != '\0' || !line.empty()) {
        // Add a command to echo this line.
        std::string cmd;
        if (color_name.empty() && !progress) {
          // Use the native echo command.
          cmd = "@echo ";
          cmd += this->EscapeForShell(line, false, true);
        } else {
          // Use cmake to echo the text in color.
          cmd = "@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) ";
          cmd += color_name;
          if (progress) {
            cmd += "--progress-dir=";
            cmd += this->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(progress->Dir),
              cmOutputConverter::SHELL);
            cmd += " ";
            cmd += "--progress-num=";
            cmd += progress->Arg;
            cmd += " ";
          }
          cmd += this->EscapeForShell(line);
        }
        commands.push_back(std::move(cmd));
      }

      // Reset the line to empty.
      line.clear();

      // Progress appears only on first line.
      progress = nullptr;

      // Terminate on end-of-string.
      if (*c == '\0') {
        return;
      }
    } else if (*c != '\r') {
      // Append this character to the current line.
      line += *c;
    }
  }
}

std::string cmLocalUnixMakefileGenerator3::CreateMakeVariable(
  std::string const& s, std::string const& s2)
{
  std::string unmodified = s;
  unmodified += s2;
  // if there is no restriction on the length of make variables
  // and there are no "." characters in the string, then return the
  // unmodified combination.
  if ((!this->MakefileVariableSize &&
       unmodified.find('.') == std::string::npos) &&
      (!this->MakefileVariableSize &&
       unmodified.find('+') == std::string::npos) &&
      (!this->MakefileVariableSize &&
       unmodified.find('-') == std::string::npos)) {
    return unmodified;
  }

  // see if the variable has been defined before and return
  // the modified version of the variable
  std::map<std::string, std::string>::iterator i =
    this->MakeVariableMap.find(unmodified);
  if (i != this->MakeVariableMap.end()) {
    return i->second;
  }
  // start with the unmodified variable
  std::string ret = unmodified;
  // if this there is no value for this->MakefileVariableSize then
  // the string must have bad characters in it
  if (!this->MakefileVariableSize) {
    std::replace(ret.begin(), ret.end(), '.', '_');
    cmSystemTools::ReplaceString(ret, "-", "__");
    cmSystemTools::ReplaceString(ret, "+", "___");
    int ni = 0;
    char buffer[5];
    // make sure the _ version is not already used, if
    // it is used then add number to the end of the variable
    while (this->ShortMakeVariableMap.count(ret) && ni < 1000) {
      ++ni;
      sprintf(buffer, "%04d", ni);
      ret = unmodified + buffer;
    }
    this->ShortMakeVariableMap[ret] = "1";
    this->MakeVariableMap[unmodified] = ret;
    return ret;
  }

  // if the string is greater than 32 chars it is an invalid variable name
  // for borland make
  if (static_cast<int>(ret.size()) > this->MakefileVariableSize) {
    int keep = this->MakefileVariableSize - 8;
    int size = keep + 3;
    std::string str1 = s;
    std::string str2 = s2;
    // we must shorten the combined string by 4 characters
    // keep no more than 24 characters from the second string
    if (static_cast<int>(str2.size()) > keep) {
      str2 = str2.substr(0, keep);
    }
    if (static_cast<int>(str1.size()) + static_cast<int>(str2.size()) > size) {
      str1 = str1.substr(0, size - str2.size());
    }
    char buffer[5];
    int ni = 0;
    sprintf(buffer, "%04d", ni);
    ret = str1 + str2 + buffer;
    while (this->ShortMakeVariableMap.count(ret) && ni < 1000) {
      ++ni;
      sprintf(buffer, "%04d", ni);
      ret = str1 + str2 + buffer;
    }
    if (ni == 1000) {
      cmSystemTools::Error("Borland makefile variable length too long");
      return unmodified;
    }
    // once an unused variable is found
    this->ShortMakeVariableMap[ret] = "1";
  }
  // always make an entry into the unmodified to variable map
  this->MakeVariableMap[unmodified] = ret;
  return ret;
}

bool cmLocalUnixMakefileGenerator3::UpdateDependencies(const char* tgtInfo,
                                                       bool verbose,
                                                       bool color)
{
  // read in the target info file
  if (!this->Makefile->ReadListFile(tgtInfo) ||
      cmSystemTools::GetErrorOccuredFlag()) {
    cmSystemTools::Error("Target DependInfo.cmake file not found");
  }

  // Check if any multiple output pairs have a missing file.
  this->CheckMultipleOutputs(verbose);

  std::string dir = cmSystemTools::GetFilenamePath(tgtInfo);
  std::string internalDependFile = dir + "/depend.internal";
  std::string dependFile = dir + "/depend.make";

  // If the target DependInfo.cmake file has changed since the last
  // time dependencies were scanned then force rescanning.  This may
  // happen when a new source file is added and CMake regenerates the
  // project but no other sources were touched.
  bool needRescanDependInfo = false;
  cmFileTimeComparison* ftc =
    this->GlobalGenerator->GetCMakeInstance()->GetFileComparison();
  {
    int result;
    if (!ftc->FileTimeCompare(internalDependFile.c_str(), tgtInfo, &result) ||
        result < 0) {
      if (verbose) {
        std::ostringstream msg;
        msg << "Dependee \"" << tgtInfo << "\" is newer than depender \""
            << internalDependFile << "\"." << std::endl;
        cmSystemTools::Stdout(msg.str().c_str());
      }
      needRescanDependInfo = true;
    }
  }

  // If the directory information is newer than depend.internal, include dirs
  // may have changed. In this case discard all old dependencies.
  bool needRescanDirInfo = false;
  std::string dirInfoFile = this->GetCurrentBinaryDirectory();
  dirInfoFile += cmake::GetCMakeFilesDirectory();
  dirInfoFile += "/CMakeDirectoryInformation.cmake";
  {
    int result;
    if (!ftc->FileTimeCompare(internalDependFile.c_str(), dirInfoFile.c_str(),
                              &result) ||
        result < 0) {
      if (verbose) {
        std::ostringstream msg;
        msg << "Dependee \"" << dirInfoFile << "\" is newer than depender \""
            << internalDependFile << "\"." << std::endl;
        cmSystemTools::Stdout(msg.str().c_str());
      }
      needRescanDirInfo = true;
    }
  }

  // Check the implicit dependencies to see if they are up to date.
  // The build.make file may have explicit dependencies for the object
  // files but these will not affect the scanning process so they need
  // not be considered.
  std::map<std::string, cmDepends::DependencyVector> validDependencies;
  bool needRescanDependencies = false;
  if (!needRescanDirInfo) {
    cmDependsC checker;
    checker.SetVerbose(verbose);
    checker.SetFileComparison(ftc);
    // cmDependsC::Check() fills the vector validDependencies() with the
    // dependencies for those files where they are still valid, i.e. neither
    // the files themselves nor any files they depend on have changed.
    // We don't do that if the CMakeDirectoryInformation.cmake file has
    // changed, because then potentially all dependencies have changed.
    // This information is given later on to cmDependsC, which then only
    // rescans the files where it did not get valid dependencies via this
    // dependency vector. This means that in the normal case, when only
    // few or one file have been edited, then also only this one file is
    // actually scanned again, instead of all files for this target.
    needRescanDependencies = !checker.Check(
      dependFile.c_str(), internalDependFile.c_str(), validDependencies);
  }

  if (needRescanDependInfo || needRescanDirInfo || needRescanDependencies) {
    // The dependencies must be regenerated.
    std::string targetName = cmSystemTools::GetFilenameName(dir);
    targetName = targetName.substr(0, targetName.length() - 4);
    std::string message = "Scanning dependencies of target ";
    message += targetName;
    cmSystemTools::MakefileColorEcho(cmsysTerminal_Color_ForegroundMagenta |
                                       cmsysTerminal_Color_ForegroundBold,
                                     message.c_str(), true, color);

    return this->ScanDependencies(dir.c_str(), validDependencies);
  }

  // The dependencies are already up-to-date.
  return true;
}

bool cmLocalUnixMakefileGenerator3::ScanDependencies(
  const char* targetDir,
  std::map<std::string, cmDepends::DependencyVector>& validDeps)
{
  // Read the directory information file.
  cmMakefile* mf = this->Makefile;
  bool haveDirectoryInfo = false;
  std::string dirInfoFile = this->GetCurrentBinaryDirectory();
  dirInfoFile += cmake::GetCMakeFilesDirectory();
  dirInfoFile += "/CMakeDirectoryInformation.cmake";
  if (mf->ReadListFile(dirInfoFile.c_str()) &&
      !cmSystemTools::GetErrorOccuredFlag()) {
    haveDirectoryInfo = true;
  }

  // Lookup useful directory information.
  if (haveDirectoryInfo) {
    // Test whether we need to force Unix paths.
    if (const char* force = mf->GetDefinition("CMAKE_FORCE_UNIX_PATHS")) {
      if (!cmSystemTools::IsOff(force)) {
        cmSystemTools::SetForceUnixPaths(true);
      }
    }

    // Setup relative path top directories.
    if (const char* relativePathTopSource =
          mf->GetDefinition("CMAKE_RELATIVE_PATH_TOP_SOURCE")) {
      this->StateSnapshot.GetDirectory().SetRelativePathTopSource(
        relativePathTopSource);
    }
    if (const char* relativePathTopBinary =
          mf->GetDefinition("CMAKE_RELATIVE_PATH_TOP_BINARY")) {
      this->StateSnapshot.GetDirectory().SetRelativePathTopBinary(
        relativePathTopBinary);
    }
  } else {
    cmSystemTools::Error("Directory Information file not found");
  }

  // create the file stream for the depends file
  std::string dir = targetDir;

  // Open the make depends file.  This should be copy-if-different
  // because the make tool may try to reload it needlessly otherwise.
  std::string ruleFileNameFull = dir;
  ruleFileNameFull += "/depend.make";
  cmGeneratedFileStream ruleFileStream(
    ruleFileNameFull, false, this->GlobalGenerator->GetMakefileEncoding());
  ruleFileStream.SetCopyIfDifferent(true);
  if (!ruleFileStream) {
    return false;
  }

  // Open the cmake dependency tracking file.  This should not be
  // copy-if-different because dependencies are re-scanned when it is
  // older than the DependInfo.cmake.
  std::string internalRuleFileNameFull = dir;
  internalRuleFileNameFull += "/depend.internal";
  cmGeneratedFileStream internalRuleFileStream(
    internalRuleFileNameFull, false,
    this->GlobalGenerator->GetMakefileEncoding());
  if (!internalRuleFileStream) {
    return false;
  }

  this->WriteDisclaimer(ruleFileStream);
  this->WriteDisclaimer(internalRuleFileStream);

  // for each language we need to scan, scan it
  std::string const& langStr =
    mf->GetSafeDefinition("CMAKE_DEPENDS_LANGUAGES");
  std::vector<std::string> langs;
  cmSystemTools::ExpandListArgument(langStr, langs);
  for (std::string const& lang : langs) {
    // construct the checker
    // Create the scanner for this language
    cmDepends* scanner = nullptr;
    if (lang == "C" || lang == "CXX" || lang == "RC" || lang == "ASM" ||
        lang == "CUDA") {
      // TODO: Handle RC (resource files) dependencies correctly.
      scanner = new cmDependsC(this, targetDir, lang, &validDeps);
    }
#ifdef CMAKE_BUILD_WITH_CMAKE
    else if (lang == "Fortran") {
      ruleFileStream << "# Note that incremental build could trigger "
                     << "a call to cmake_copy_f90_mod on each re-build\n";
      scanner = new cmDependsFortran(this);
    // } else if (lang == "Java") {
    //   scanner = new cmDependsJava();
    }
#endif

    if (scanner) {
      scanner->SetLocalGenerator(this);
      scanner->SetFileComparison(
        this->GlobalGenerator->GetCMakeInstance()->GetFileComparison());
      scanner->SetLanguage(lang);
      scanner->SetTargetDirectory(dir.c_str());
      scanner->Write(ruleFileStream, internalRuleFileStream);

      // free the scanner for this language
      delete scanner;
    }
  }

  return true;
}

void cmLocalUnixMakefileGenerator3::CheckMultipleOutputs(bool verbose)
{
  cmMakefile* mf = this->Makefile;

  // Get the string listing the multiple output pairs.
  const char* pairs_string = mf->GetDefinition("CMAKE_MULTIPLE_OUTPUT_PAIRS");
  if (!pairs_string) {
    return;
  }

  // Convert the string to a list and preserve empty entries.
  std::vector<std::string> pairs;
  cmSystemTools::ExpandListArgument(pairs_string, pairs, true);
  for (std::vector<std::string>::const_iterator i = pairs.begin();
       i != pairs.end() && (i + 1) != pairs.end();) {
    const std::string& depender = *i++;
    const std::string& dependee = *i++;

    // If the depender is missing then delete the dependee to make
    // sure both will be regenerated.
    if (cmSystemTools::FileExists(dependee) &&
        !cmSystemTools::FileExists(depender)) {
      if (verbose) {
        std::ostringstream msg;
        msg << "Deleting primary custom command output \"" << dependee
            << "\" because another output \"" << depender
            << "\" does not exist." << std::endl;
        cmSystemTools::Stdout(msg.str().c_str());
      }
      cmSystemTools::RemoveFile(dependee);
    }
  }
}

void cmLocalUnixMakefileGenerator3::WriteLocalAllRules(
  std::ostream& ruleFileStream)
{
  this->WriteDisclaimer(ruleFileStream);

  // Write the main entry point target.  This must be the VERY first
  // target so that make with no arguments will run it.
  {
    // Just depend on the all target to drive the build.
    std::vector<std::string> depends;
    std::vector<std::string> no_commands;
    depends.push_back("all");

    // Write the rule.
    this->WriteMakeRule(ruleFileStream,
                        "Default target executed when no arguments are "
                        "given to make.",
                        "default_target", depends, no_commands, true);

    // Help out users that try "gmake target1 target2 -j".
    cmGlobalUnixMakefileGenerator3* gg =
      static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
    if (gg->AllowNotParallel()) {
      std::vector<std::string> no_depends;
      this->WriteMakeRule(ruleFileStream,
                          "Allow only one \"make -f "
                          "Makefile2\" at a time, but pass "
                          "parallelism.",
                          ".NOTPARALLEL", no_depends, no_commands, false);
    }
  }

  this->WriteSpecialTargetsTop(ruleFileStream);

  // Include the progress variables for the target.
  // Write all global targets
  this->WriteDivider(ruleFileStream);
  ruleFileStream << "# Targets provided globally by CMake.\n"
                 << "\n";
  const std::vector<cmGeneratorTarget*>& targets = this->GetGeneratorTargets();
  for (cmGeneratorTarget* gt : targets) {
    if (gt->GetType() == cmStateEnums::GLOBAL_TARGET) {
      std::string targetString =
        "Special rule for the target " + gt->GetName();
      std::vector<std::string> commands;
      std::vector<std::string> depends;

      const char* text = gt->GetProperty("EchoString");
      if (!text) {
        text = "Running external command ...";
      }
      depends.insert(depends.end(), gt->GetUtilities().begin(),
                     gt->GetUtilities().end());
      this->AppendEcho(commands, text,
                       cmLocalUnixMakefileGenerator3::EchoGlobal);

      // Global targets store their rules in pre- and post-build commands.
      this->AppendCustomDepends(depends, gt->GetPreBuildCommands());
      this->AppendCustomDepends(depends, gt->GetPostBuildCommands());
      this->AppendCustomCommands(commands, gt->GetPreBuildCommands(), gt,
                                 this->GetCurrentBinaryDirectory());
      this->AppendCustomCommands(commands, gt->GetPostBuildCommands(), gt,
                                 this->GetCurrentBinaryDirectory());
      std::string targetName = gt->GetName();
      this->WriteMakeRule(ruleFileStream, targetString.c_str(), targetName,
                          depends, commands, true);

      // Provide a "/fast" version of the target.
      depends.clear();
      if ((targetName == "install") || (targetName == "install/local") ||
          (targetName == "install/strip")) {
        // Provide a fast install target that does not depend on all
        // but has the same command.
        depends.push_back("preinstall/fast");
      } else {
        // Just forward to the real target so at least it will work.
        depends.push_back(targetName);
        commands.clear();
      }
      targetName += "/fast";
      this->WriteMakeRule(ruleFileStream, targetString.c_str(), targetName,
                          depends, commands, true);
    }
  }

  std::vector<std::string> depends;
  std::vector<std::string> commands;

  // Write the all rule.
  std::string recursiveTarget = this->GetCurrentBinaryDirectory();
  recursiveTarget += "/all";

  bool regenerate =
    !this->GlobalGenerator->GlobalSettingIsOn("CMAKE_SUPPRESS_REGENERATION");
  if (regenerate) {
    depends.push_back("cmake_check_build_system");
  }

  std::string progressDir = this->GetBinaryDirectory();
  progressDir += cmake::GetCMakeFilesDirectory();
  {
    std::ostringstream progCmd;
    progCmd << "$(CMAKE_COMMAND) -E cmake_progress_start ";
    progCmd << this->ConvertToOutputFormat(
      cmSystemTools::CollapseFullPath(progressDir), cmOutputConverter::SHELL);

    std::string progressFile = cmake::GetCMakeFilesDirectory();
    progressFile += "/progress.marks";
    std::string progressFileNameFull = this->ConvertToFullPath(progressFile);
    progCmd << " "
            << this->ConvertToOutputFormat(
                 cmSystemTools::CollapseFullPath(progressFileNameFull),
                 cmOutputConverter::SHELL);
    commands.push_back(progCmd.str());
  }
  std::string mf2Dir = cmake::GetCMakeFilesDirectoryPostSlash();
  mf2Dir += "Makefile2";
  commands.push_back(
    this->GetRecursiveMakeCall(mf2Dir.c_str(), recursiveTarget));
  this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                        this->GetCurrentBinaryDirectory());
  {
    std::ostringstream progCmd;
    progCmd << "$(CMAKE_COMMAND) -E cmake_progress_start "; // # 0
    progCmd << this->ConvertToOutputFormat(
      cmSystemTools::CollapseFullPath(progressDir), cmOutputConverter::SHELL);
    progCmd << " 0";
    commands.push_back(progCmd.str());
  }
  this->WriteMakeRule(ruleFileStream, "The main all target", "all", depends,
                      commands, true);

  // Write the clean rule.
  recursiveTarget = this->GetCurrentBinaryDirectory();
  recursiveTarget += "/clean";
  commands.clear();
  depends.clear();
  commands.push_back(
    this->GetRecursiveMakeCall(mf2Dir.c_str(), recursiveTarget));
  this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                        this->GetCurrentBinaryDirectory());
  this->WriteMakeRule(ruleFileStream, "The main clean target", "clean",
                      depends, commands, true);
  commands.clear();
  depends.clear();
  depends.push_back("clean");
  this->WriteMakeRule(ruleFileStream, "The main clean target", "clean/fast",
                      depends, commands, true);

  // Write the preinstall rule.
  recursiveTarget = this->GetCurrentBinaryDirectory();
  recursiveTarget += "/preinstall";
  commands.clear();
  depends.clear();
  const char* noall =
    this->Makefile->GetDefinition("CMAKE_SKIP_INSTALL_ALL_DEPENDENCY");
  if (!noall || cmSystemTools::IsOff(noall)) {
    // Drive the build before installing.
    depends.push_back("all");
  } else if (regenerate) {
    // At least make sure the build system is up to date.
    depends.push_back("cmake_check_build_system");
  }
  commands.push_back(
    this->GetRecursiveMakeCall(mf2Dir.c_str(), recursiveTarget));
  this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                        this->GetCurrentBinaryDirectory());
  this->WriteMakeRule(ruleFileStream, "Prepare targets for installation.",
                      "preinstall", depends, commands, true);
  depends.clear();
  this->WriteMakeRule(ruleFileStream, "Prepare targets for installation.",
                      "preinstall/fast", depends, commands, true);

  if (regenerate) {
    // write the depend rule, really a recompute depends rule
    depends.clear();
    commands.clear();
    cmake* cm = this->GlobalGenerator->GetCMakeInstance();
    if (cm->DoWriteGlobVerifyTarget()) {
      std::string rescanRule = "$(CMAKE_COMMAND) -P ";
      rescanRule += this->ConvertToOutputFormat(cm->GetGlobVerifyScript(),
                                                cmOutputConverter::SHELL);
      commands.push_back(rescanRule);
    }
    std::string cmakefileName = cmake::GetCMakeFilesDirectoryPostSlash();
    cmakefileName += "Makefile.cmake";
    {
      std::string runRule =
        "$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)";
      runRule += " --check-build-system ";
      runRule +=
        this->ConvertToOutputFormat(cmakefileName, cmOutputConverter::SHELL);
      runRule += " 1";
      commands.push_back(std::move(runRule));
    }
    this->CreateCDCommand(commands, this->GetBinaryDirectory(),
                          this->GetCurrentBinaryDirectory());
    this->WriteMakeRule(ruleFileStream, "clear depends", "depend", depends,
                        commands, true);
  }
}

void cmLocalUnixMakefileGenerator3::ClearDependencies(cmMakefile* mf,
                                                      bool verbose)
{
  // Get the list of target files to check
  const char* infoDef = mf->GetDefinition("CMAKE_DEPEND_INFO_FILES");
  if (!infoDef) {
    return;
  }
  std::vector<std::string> files;
  cmSystemTools::ExpandListArgument(infoDef, files);

  // Each depend information file corresponds to a target.  Clear the
  // dependencies for that target.
  cmDepends clearer;
  clearer.SetVerbose(verbose);
  for (std::string const& file : files) {
    std::string dir = cmSystemTools::GetFilenamePath(file);

    // Clear the implicit dependency makefile.
    std::string dependFile = dir + "/depend.make";
    clearer.Clear(dependFile.c_str());

    // Remove the internal dependency check file to force
    // regeneration.
    std::string internalDependFile = dir + "/depend.internal";
    cmSystemTools::RemoveFile(internalDependFile);
  }
}

namespace {
// Helper predicate for removing absolute paths that don't point to the
// source or binary directory. It is used when CMAKE_DEPENDS_IN_PROJECT_ONLY
// is set ON, to only consider in-project dependencies during the build.
class NotInProjectDir
{
public:
  // Constructor with the source and binary directory's path
  NotInProjectDir(const std::string& sourceDir, const std::string& binaryDir)
    : SourceDir(sourceDir)
    , BinaryDir(binaryDir)
  {
  }

  // Operator evaluating the predicate
  bool operator()(const std::string& path) const
  {
    // Keep all relative paths:
    if (!cmSystemTools::FileIsFullPath(path)) {
      return false;
    }
    // If it's an absolute path, check if it starts with the source
    // direcotory:
    return (
      !(IsInDirectory(SourceDir, path) || IsInDirectory(BinaryDir, path)));
  }

private:
  // Helper function used by the predicate
  static bool IsInDirectory(const std::string& baseDir,
                            const std::string& testDir)
  {
    // First check if the test directory "starts with" the base directory:
    if (testDir.find(baseDir) != 0) {
      return false;
    }
    // If it does, then check that it's either the same string, or that the
    // next character is a slash:
    return ((testDir.size() == baseDir.size()) ||
            (testDir[baseDir.size()] == '/'));
  }

  // The path to the source directory
  std::string SourceDir;
  // The path to the binary directory
  std::string BinaryDir;
};
}

void cmLocalUnixMakefileGenerator3::WriteDependLanguageInfo(
  std::ostream& cmakefileStream, cmGeneratorTarget* target)
{
  ImplicitDependLanguageMap const& implicitLangs =
    this->GetImplicitDepends(target);

  // list the languages
  cmakefileStream
    << "# The set of languages for which implicit dependencies are needed:\n";
  cmakefileStream << "set(CMAKE_DEPENDS_LANGUAGES\n";
  for (auto const& implicitLang : implicitLangs) {
    cmakefileStream << "  \"" << implicitLang.first << "\"\n";
  }
  cmakefileStream << "  )\n";

  // now list the files for each language
  cmakefileStream
    << "# The set of files for implicit dependencies of each language:\n";
  for (auto const& implicitLang : implicitLangs) {
    cmakefileStream << "set(CMAKE_DEPENDS_CHECK_" << implicitLang.first
                    << "\n";
    ImplicitDependFileMap const& implicitPairs = implicitLang.second;

    // for each file pair
    for (auto const& implicitPair : implicitPairs) {
      for (auto const& di : implicitPair.second) {
        cmakefileStream << "  \"" << di << "\" ";
        cmakefileStream << "\"" << implicitPair.first << "\"\n";
      }
    }
    cmakefileStream << "  )\n";

    // Tell the dependency scanner what compiler is used.
    std::string cidVar = "CMAKE_";
    cidVar += implicitLang.first;
    cidVar += "_COMPILER_ID";
    const char* cid = this->Makefile->GetDefinition(cidVar);
    if (cid && *cid) {
      cmakefileStream << "set(CMAKE_" << implicitLang.first
                      << "_COMPILER_ID \"" << cid << "\")\n";
    }

    // Build a list of preprocessor definitions for the target.
    std::set<std::string> defines;
    this->AddCompileDefinitions(defines, target, this->ConfigName,
                                implicitLang.first);
    if (!defines.empty()) {
      /* clang-format off */
      cmakefileStream
        << "\n"
        << "# Preprocessor definitions for this target.\n"
        << "set(CMAKE_TARGET_DEFINITIONS_" << implicitLang.first << "\n";
      /* clang-format on */
      for (std::string const& define : defines) {
        cmakefileStream << "  " << cmOutputConverter::EscapeForCMake(define)
                        << "\n";
      }
      cmakefileStream << "  )\n";
    }

    // Target-specific include directories:
    cmakefileStream << "\n"
                    << "# The include file search paths:\n";
    cmakefileStream << "set(CMAKE_" << implicitLang.first
                    << "_TARGET_INCLUDE_PATH\n";
    std::vector<std::string> includes;

    const std::string& config =
      this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
    this->GetIncludeDirectories(includes, target, implicitLang.first, config);
    std::string binaryDir = this->GetState()->GetBinaryDirectory();
    if (this->Makefile->IsOn("CMAKE_DEPENDS_IN_PROJECT_ONLY")) {
      std::string const& sourceDir = this->GetState()->GetSourceDirectory();
      cmEraseIf(includes, ::NotInProjectDir(sourceDir, binaryDir));
    }
    for (std::string const& include : includes) {
      cmakefileStream << "  \""
                      << this->MaybeConvertToRelativePath(binaryDir, include)
                      << "\"\n";
    }
    cmakefileStream << "  )\n";
  }

  // Store include transform rule properties.  Write the directory
  // rules first because they may be overridden by later target rules.
  std::vector<std::string> transformRules;
  if (const char* xform =
        this->Makefile->GetProperty("IMPLICIT_DEPENDS_INCLUDE_TRANSFORM")) {
    cmSystemTools::ExpandListArgument(xform, transformRules);
  }
  if (const char* xform =
        target->GetProperty("IMPLICIT_DEPENDS_INCLUDE_TRANSFORM")) {
    cmSystemTools::ExpandListArgument(xform, transformRules);
  }
  if (!transformRules.empty()) {
    cmakefileStream << "set(CMAKE_INCLUDE_TRANSFORMS\n";
    for (std::string const& tr : transformRules) {
      cmakefileStream << "  " << cmOutputConverter::EscapeForCMake(tr) << "\n";
    }
    cmakefileStream << "  )\n";
  }
}

void cmLocalUnixMakefileGenerator3::WriteDisclaimer(std::ostream& os)
{
  os << "# CMAKE generated file: DO NOT EDIT!\n"
     << "# Generated by \"" << this->GlobalGenerator->GetName() << "\""
     << " Generator, CMake Version " << cmVersion::GetMajorVersion() << "."
     << cmVersion::GetMinorVersion() << "\n\n";
}

std::string cmLocalUnixMakefileGenerator3::GetRecursiveMakeCall(
  const char* makefile, const std::string& tgt)
{
  // Call make on the given file.
  std::string cmd;
  cmd += "$(MAKE) -f ";
  cmd += this->ConvertToOutputFormat(makefile, cmOutputConverter::SHELL);
  cmd += " ";

  cmGlobalUnixMakefileGenerator3* gg =
    static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
  // Pass down verbosity level.
  if (!gg->MakeSilentFlag.empty()) {
    cmd += gg->MakeSilentFlag;
    cmd += " ";
  }

  // Most unix makes will pass the command line flags to make down to
  // sub-invoked makes via an environment variable.  However, some
  // makes do not support that, so you have to pass the flags
  // explicitly.
  if (gg->PassMakeflags) {
    cmd += "-$(MAKEFLAGS) ";
  }

  // Add the target.
  if (!tgt.empty()) {
    // The make target is always relative to the top of the build tree.
    std::string tgt2 =
      this->MaybeConvertToRelativePath(this->GetBinaryDirectory(), tgt);

    // The target may have been written with windows paths.
    cmSystemTools::ConvertToOutputSlashes(tgt2);

    // Escape one extra time if the make tool requires it.
    if (this->MakeCommandEscapeTargetTwice) {
      tgt2 = this->EscapeForShell(tgt2, true, false);
    }

    // The target name is now a string that should be passed verbatim
    // on the command line.
    cmd += this->EscapeForShell(tgt2, true, false);
  }
  return cmd;
}

void cmLocalUnixMakefileGenerator3::WriteDivider(std::ostream& os)
{
  os << "#======================================"
     << "=======================================\n";
}

void cmLocalUnixMakefileGenerator3::WriteCMakeArgument(std::ostream& os,
                                                       const char* s)
{
  // Write the given string to the stream with escaping to get it back
  // into CMake through the lexical scanner.
  os << "\"";
  for (const char* c = s; *c; ++c) {
    if (*c == '\\') {
      os << "\\\\";
    } else if (*c == '"') {
      os << "\\\"";
    } else {
      os << *c;
    }
  }
  os << "\"";
}

std::string cmLocalUnixMakefileGenerator3::ConvertToQuotedOutputPath(
  const char* p, bool useWatcomQuote)
{
  // Split the path into its components.
  std::vector<std::string> components;
  cmSystemTools::SplitPath(p, components);

  // Open the quoted result.
  std::string result;
  if (useWatcomQuote) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    result = "'";
#else
    result = "\"'";
#endif
  } else {
    result = "\"";
  }

  // Return an empty path if there are no components.
  if (!components.empty()) {
    // Choose a slash direction and fix root component.
    const char* slash = "/";
#if defined(_WIN32) && !defined(__CYGWIN__)
    if (!cmSystemTools::GetForceUnixPaths()) {
      slash = "\\";
      for (char& i : components[0]) {
        if (i == '/') {
          i = '\\';
        }
      }
    }
#endif

    // Begin the quoted result with the root component.
    result += components[0];

    if (components.size() > 1) {
      // Now add the rest of the components separated by the proper slash
      // direction for this platform.
      std::vector<std::string>::const_iterator compEnd = std::remove(
        components.begin() + 1, components.end() - 1, std::string());
      std::vector<std::string>::const_iterator compStart =
        components.begin() + 1;
      result += cmJoin(cmMakeRange(compStart, compEnd), slash);
      // Only the last component can be empty to avoid double slashes.
      result += slash;
      result += components.back();
    }
  }

  // Close the quoted result.
  if (useWatcomQuote) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    result += "'";
#else
    result += "'\"";
#endif
  } else {
    result += "\"";
  }

  return result;
}

std::string cmLocalUnixMakefileGenerator3::GetTargetDirectory(
  cmGeneratorTarget const* target) const
{
  std::string dir = cmake::GetCMakeFilesDirectoryPostSlash();
  dir += target->GetName();
#if defined(__VMS)
  dir += "_dir";
#else
  dir += ".dir";
#endif
  return dir;
}

cmLocalUnixMakefileGenerator3::ImplicitDependLanguageMap const&
cmLocalUnixMakefileGenerator3::GetImplicitDepends(const cmGeneratorTarget* tgt)
{
  return this->ImplicitDepends[tgt->GetName()];
}

void cmLocalUnixMakefileGenerator3::AddImplicitDepends(
  const cmGeneratorTarget* tgt, const std::string& lang, const char* obj,
  const char* src)
{
  this->ImplicitDepends[tgt->GetName()][lang][obj].push_back(src);
}

void cmLocalUnixMakefileGenerator3::CreateCDCommand(
  std::vector<std::string>& commands, std::string const& tgtDir,
  std::string const& relDir)
{
  // do we need to cd?
  if (tgtDir == relDir) {
    return;
  }

  // In a Windows shell we must change drive letter too.  The shell
  // used by NMake and Borland make does not support "cd /d" so this
  // feature simply cannot work with them (Borland make does not even
  // support changing the drive letter with just "d:").
  const char* cd_cmd = this->IsMinGWMake() ? "cd /d " : "cd ";

  cmGlobalUnixMakefileGenerator3* gg =
    static_cast<cmGlobalUnixMakefileGenerator3*>(this->GlobalGenerator);
  if (!gg->UnixCD) {
    // On Windows we must perform each step separately and then change
    // back because the shell keeps the working directory between
    // commands.
    std::string cmd = cd_cmd;
    cmd += this->ConvertToOutputForExisting(tgtDir);
    commands.insert(commands.begin(), cmd);

    // Change back to the starting directory.
    cmd = cd_cmd;
    cmd += this->ConvertToOutputForExisting(relDir);
    commands.push_back(std::move(cmd));
  } else {
    // On UNIX we must construct a single shell command to change
    // directory and build because make resets the directory between
    // each command.
    std::string outputForExisting = this->ConvertToOutputForExisting(tgtDir);
    std::string prefix = cd_cmd + outputForExisting + " && ";
    std::transform(commands.begin(), commands.end(), commands.begin(),
                   [&prefix](std::string const& s) { return prefix + s; });
  }
}

std::string cmLocalUnixMakefileGenerator3::MaybeConvertToRelativePath(
  std::string const& base, std::string const& path)
{
  if (!cmOutputConverter::ContainedInDirectory(
        base, path, this->GetStateSnapshot().GetDirectory())) {
    return path;
  }
  return cmOutputConverter::ForceToRelativePath(base, path);
}
