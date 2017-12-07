/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMakefileLibraryTargetGenerator.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLinkLineComputer.h"
#include "cmLinkLineDeviceComputer.h"
#include "cmLocalGenerator.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmOSXBundleGenerator.h"
#include "cmOutputConverter.h"
#include "cmRulePlaceholderExpander.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmMakefileLibraryTargetGenerator::cmMakefileLibraryTargetGenerator(
  cmGeneratorTarget* target)
  : cmMakefileTargetGenerator(target)
{
  this->CustomCommandDriver = OnDepends;
  if (this->GeneratorTarget->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
    this->GeneratorTarget->GetLibraryNames(
      this->TargetNameOut, this->TargetNameSO, this->TargetNameReal,
      this->TargetNameImport, this->TargetNamePDB, this->ConfigName);
  }

  this->OSXBundleGenerator =
    new cmOSXBundleGenerator(target, this->ConfigName);
  this->OSXBundleGenerator->SetMacContentFolders(&this->MacContentFolders);
}

cmMakefileLibraryTargetGenerator::~cmMakefileLibraryTargetGenerator()
{
  delete this->OSXBundleGenerator;
}

void cmMakefileLibraryTargetGenerator::WriteRuleFiles()
{
  // create the build.make file and directory, put in the common blocks
  this->CreateRuleFile();

  // write rules used to help build object files
  this->WriteCommonCodeRules();

  // write the per-target per-language flags
  this->WriteTargetLanguageFlags();

  // write in rules for object files and custom commands
  this->WriteTargetBuildRules();

  // write the link rules
  // Write the rule for this target type.
  switch (this->GeneratorTarget->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      this->WriteStaticLibraryRules();
      break;
    case cmStateEnums::SHARED_LIBRARY:
      this->WriteSharedLibraryRules(false);
      if (this->GeneratorTarget->NeedRelinkBeforeInstall(this->ConfigName)) {
        // Write rules to link an installable version of the target.
        this->WriteSharedLibraryRules(true);
      }
      break;
    case cmStateEnums::MODULE_LIBRARY:
      this->WriteModuleLibraryRules(false);
      if (this->GeneratorTarget->NeedRelinkBeforeInstall(this->ConfigName)) {
        // Write rules to link an installable version of the target.
        this->WriteModuleLibraryRules(true);
      }
      break;
    case cmStateEnums::OBJECT_LIBRARY:
      this->WriteObjectLibraryRules();
      break;
    default:
      // If language is not known, this is an error.
      cmSystemTools::Error("Unknown Library Type");
      break;
  }

  // Write the requires target.
  this->WriteTargetRequiresRules();

  // Write clean target
  this->WriteTargetCleanRules();

  // Write the dependency generation rule.  This must be done last so
  // that multiple output pair information is available.
  this->WriteTargetDependRules();

  // close the streams
  this->CloseFileStreams();
}

void cmMakefileLibraryTargetGenerator::WriteObjectLibraryRules()
{
  std::vector<std::string> commands;
  std::vector<std::string> depends;

  // Add post-build rules.
  this->LocalGenerator->AppendCustomCommands(
    commands, this->GeneratorTarget->GetPostBuildCommands(),
    this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());

  // Depend on the object files.
  this->AppendObjectDepends(depends);

  // Write the rule.
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      this->GeneratorTarget->GetName(),
                                      depends, commands, true);

  // Write the main driver rule to build everything in this target.
  this->WriteTargetDriverRule(this->GeneratorTarget->GetName(), false);
}

void cmMakefileLibraryTargetGenerator::WriteStaticLibraryRules()
{
  const std::string cuda_lang("CUDA");
  cmGeneratorTarget::LinkClosure const* closure =
    this->GeneratorTarget->GetLinkClosure(this->ConfigName);

  const bool hasCUDA =
    (std::find(closure->Languages.begin(), closure->Languages.end(),
               cuda_lang) != closure->Languages.end());

  const bool resolveDeviceSymbols =
    this->GeneratorTarget->GetPropertyAsBool("CUDA_RESOLVE_DEVICE_SYMBOLS");
  if (hasCUDA && resolveDeviceSymbols) {
    std::string linkRuleVar = "CMAKE_CUDA_DEVICE_LINK_LIBRARY";
    std::string extraFlags;
    this->LocalGenerator->AppendFlags(
      extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
    this->WriteDeviceLibraryRules(linkRuleVar, extraFlags, false);
  }

  std::string linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);

  std::string linkRuleVar = this->GeneratorTarget->GetCreateRuleVariable(
    linkLanguage, this->ConfigName);

  std::string extraFlags;
  this->LocalGenerator->GetStaticLibraryFlags(
    extraFlags, cmSystemTools::UpperCase(this->ConfigName),
    this->GeneratorTarget);
  this->WriteLibraryRules(linkRuleVar, extraFlags, false);
}

void cmMakefileLibraryTargetGenerator::WriteSharedLibraryRules(bool relink)
{
  if (this->GeneratorTarget->IsFrameworkOnApple()) {
    this->WriteFrameworkRules(relink);
    return;
  }

  if (!relink) {
    const std::string cuda_lang("CUDA");
    cmGeneratorTarget::LinkClosure const* closure =
      this->GeneratorTarget->GetLinkClosure(this->ConfigName);

    const bool hasCUDA =
      (std::find(closure->Languages.begin(), closure->Languages.end(),
                 cuda_lang) != closure->Languages.end());
    if (hasCUDA) {
      std::string linkRuleVar = "CMAKE_CUDA_DEVICE_LINK_LIBRARY";
      std::string extraFlags;
      this->LocalGenerator->AppendFlags(
        extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
      this->WriteDeviceLibraryRules(linkRuleVar, extraFlags, relink);
    }
  }

  std::string linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
  std::string linkRuleVar = "CMAKE_";
  linkRuleVar += linkLanguage;
  linkRuleVar += "_CREATE_SHARED_LIBRARY";

  std::string extraFlags;
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
  std::string linkFlagsConfig = "LINK_FLAGS_";
  linkFlagsConfig += cmSystemTools::UpperCase(this->ConfigName);
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty(linkFlagsConfig));

  this->LocalGenerator->AddConfigVariableFlags(
    extraFlags, "CMAKE_SHARED_LINKER_FLAGS", this->ConfigName);

  CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
    this->CreateLinkLineComputer(
      this->LocalGenerator,
      this->LocalGenerator->GetStateSnapshot().GetDirectory()));

  this->AddModuleDefinitionFlag(linkLineComputer.get(), extraFlags);

  if (this->GeneratorTarget->GetPropertyAsBool("LINK_WHAT_YOU_USE")) {
    this->LocalGenerator->AppendFlags(extraFlags, " -Wl,--no-as-needed");
  }
  this->WriteLibraryRules(linkRuleVar, extraFlags, relink);
}

void cmMakefileLibraryTargetGenerator::WriteModuleLibraryRules(bool relink)
{

  if (!relink) {
    const std::string cuda_lang("CUDA");
    cmGeneratorTarget::LinkClosure const* closure =
      this->GeneratorTarget->GetLinkClosure(this->ConfigName);

    const bool hasCUDA =
      (std::find(closure->Languages.begin(), closure->Languages.end(),
                 cuda_lang) != closure->Languages.end());
    if (hasCUDA) {
      std::string linkRuleVar = "CMAKE_CUDA_DEVICE_LINK_LIBRARY";
      std::string extraFlags;
      this->LocalGenerator->AppendFlags(
        extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
      this->WriteDeviceLibraryRules(linkRuleVar, extraFlags, relink);
    }
  }

  std::string linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
  std::string linkRuleVar = "CMAKE_";
  linkRuleVar += linkLanguage;
  linkRuleVar += "_CREATE_SHARED_MODULE";

  std::string extraFlags;
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
  std::string linkFlagsConfig = "LINK_FLAGS_";
  linkFlagsConfig += cmSystemTools::UpperCase(this->ConfigName);
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty(linkFlagsConfig));
  this->LocalGenerator->AddConfigVariableFlags(
    extraFlags, "CMAKE_MODULE_LINKER_FLAGS", this->ConfigName);

  CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
    this->CreateLinkLineComputer(
      this->LocalGenerator,
      this->LocalGenerator->GetStateSnapshot().GetDirectory()));

  this->AddModuleDefinitionFlag(linkLineComputer.get(), extraFlags);

  this->WriteLibraryRules(linkRuleVar, extraFlags, relink);
}

void cmMakefileLibraryTargetGenerator::WriteFrameworkRules(bool relink)
{
  std::string linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
  std::string linkRuleVar = "CMAKE_";
  linkRuleVar += linkLanguage;
  linkRuleVar += "_CREATE_MACOSX_FRAMEWORK";

  std::string extraFlags;
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty("LINK_FLAGS"));
  std::string linkFlagsConfig = "LINK_FLAGS_";
  linkFlagsConfig += cmSystemTools::UpperCase(this->ConfigName);
  this->LocalGenerator->AppendFlags(
    extraFlags, this->GeneratorTarget->GetProperty(linkFlagsConfig));
  this->LocalGenerator->AddConfigVariableFlags(
    extraFlags, "CMAKE_MACOSX_FRAMEWORK_LINKER_FLAGS", this->ConfigName);

  this->WriteLibraryRules(linkRuleVar, extraFlags, relink);
}

void cmMakefileLibraryTargetGenerator::WriteDeviceLibraryRules(
  const std::string& linkRuleVar, const std::string& extraFlags, bool relink)
{
#ifdef CMAKE_BUILD_WITH_CMAKE
  // TODO: Merge the methods that call this method to avoid
  // code duplication.
  std::vector<std::string> commands;

  // Build list of dependencies.
  std::vector<std::string> depends;
  this->AppendLinkDepends(depends);

  // Get the language to use for linking this library.
  std::string linkLanguage = "CUDA";
  std::string const objExt =
    this->Makefile->GetSafeDefinition("CMAKE_CUDA_OUTPUT_EXTENSION");

  // Create set of linking flags.
  std::string linkFlags;
  this->LocalGenerator->AppendFlags(linkFlags, extraFlags);

  // Get the name of the device object to generate.
  std::string const targetOutputReal =
    this->GeneratorTarget->ObjectDirectory + "cmake_device_link" + objExt;
  this->DeviceLinkObject = targetOutputReal;

  this->NumberOfProgressActions++;
  if (!this->NoRuleMessages) {
    cmLocalUnixMakefileGenerator3::EchoProgress progress;
    this->MakeEchoProgress(progress);
    // Add the link message.
    std::string buildEcho = "Linking " + linkLanguage + " device code ";
    buildEcho += this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(),
        this->DeviceLinkObject),
      cmOutputConverter::SHELL);
    this->LocalGenerator->AppendEcho(
      commands, buildEcho, cmLocalUnixMakefileGenerator3::EchoLink, &progress);
  }
  // Clean files associated with this library.
  std::vector<std::string> libCleanFiles;
  libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
    this->LocalGenerator->GetCurrentBinaryDirectory(), targetOutputReal));

  // Determine whether a link script will be used.
  bool useLinkScript = this->GlobalGenerator->GetUseLinkScript();

  bool useResponseFileForObjects =
    this->CheckUseResponseFileForObjects(linkLanguage);
  bool const useResponseFileForLibs =
    this->CheckUseResponseFileForLibraries(linkLanguage);

  cmRulePlaceholderExpander::RuleVariables vars;
  vars.Language = linkLanguage.c_str();

  // Expand the rule variables.
  std::vector<std::string> real_link_commands;
  {
    bool useWatcomQuote =
      this->Makefile->IsOn(linkRuleVar + "_USE_WATCOM_QUOTE");

    // Set path conversion for link script shells.
    this->LocalGenerator->SetLinkScriptShell(useLinkScript);

    // Collect up flags to link in needed libraries.
    std::string linkLibs;
    if (this->GeneratorTarget->GetType() != cmStateEnums::STATIC_LIBRARY) {

      CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
        new cmLinkLineDeviceComputer(
          this->LocalGenerator,
          this->LocalGenerator->GetStateSnapshot().GetDirectory()));
      linkLineComputer->SetForResponse(useResponseFileForLibs);
      linkLineComputer->SetUseWatcomQuote(useWatcomQuote);
      linkLineComputer->SetRelink(relink);

      this->CreateLinkLibs(linkLineComputer.get(), linkLibs,
                           useResponseFileForLibs, depends);
    }

    // Construct object file lists that may be needed to expand the
    // rule.
    std::string buildObjs;
    this->CreateObjectLists(useLinkScript, false, // useArchiveRules
                            useResponseFileForObjects, buildObjs, depends,
                            useWatcomQuote);

    cmOutputConverter::OutputFormat output = (useWatcomQuote)
      ? cmOutputConverter::WATCOMQUOTE
      : cmOutputConverter::SHELL;

    std::string objectDir = this->GeneratorTarget->GetSupportDirectory();
    objectDir = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), objectDir),
      cmOutputConverter::SHELL);

    std::string target = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), targetOutputReal),
      output);

    std::string targetFullPathCompilePDB = this->ComputeTargetCompilePDB();
    std::string targetOutPathCompilePDB =
      this->LocalGenerator->ConvertToOutputFormat(targetFullPathCompilePDB,
                                                  cmOutputConverter::SHELL);

    vars.Objects = buildObjs.c_str();
    vars.ObjectDir = objectDir.c_str();
    vars.Target = target.c_str();
    vars.LinkLibraries = linkLibs.c_str();
    vars.ObjectsQuoted = buildObjs.c_str();
    vars.LinkFlags = linkFlags.c_str();
    vars.TargetCompilePDB = targetOutPathCompilePDB.c_str();

    // Add language-specific flags.
    std::string langFlags;
    this->LocalGenerator->AddLanguageFlagsForLinking(
      langFlags, this->GeneratorTarget, linkLanguage, this->ConfigName);

    vars.LanguageCompileFlags = langFlags.c_str();

    std::string launcher;
    const char* val = this->LocalGenerator->GetRuleLauncher(
      this->GeneratorTarget, "RULE_LAUNCH_LINK");
    if (val && *val) {
      launcher = val;
      launcher += " ";
    }

    CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
      this->LocalGenerator->CreateRulePlaceholderExpander());

    // Construct the main link rule and expand placeholders.
    rulePlaceholderExpander->SetTargetImpLib(targetOutputReal);
    std::string linkRule = this->GetLinkRule(linkRuleVar);
    cmSystemTools::ExpandListArgument(linkRule, real_link_commands);

    // Expand placeholders.
    for (std::vector<std::string>::iterator i = real_link_commands.begin();
         i != real_link_commands.end(); ++i) {
      *i = launcher + *i;
      rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator, *i,
                                                   vars);
    }
    // Restore path conversion to normal shells.
    this->LocalGenerator->SetLinkScriptShell(false);

    // Clean all the possible library names and symlinks.
    this->CleanFiles.insert(this->CleanFiles.end(), libCleanFiles.begin(),
                            libCleanFiles.end());
  }

  std::vector<std::string> commands1;
  // Optionally convert the build rule to use a script to avoid long
  // command lines in the make shell.
  if (useLinkScript) {
    // Use a link script.
    const char* name = (relink ? "drelink.txt" : "dlink.txt");
    this->CreateLinkScript(name, real_link_commands, commands1, depends);
  } else {
    // No link script.  Just use the link rule directly.
    commands1 = real_link_commands;
  }
  this->LocalGenerator->CreateCDCommand(
    commands1, this->Makefile->GetCurrentBinaryDirectory(),
    this->LocalGenerator->GetBinaryDirectory());
  commands.insert(commands.end(), commands1.begin(), commands1.end());
  commands1.clear();

  // Compute the list of outputs.
  std::vector<std::string> outputs(1, targetOutputReal);

  // Write the build rule.
  this->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR, outputs, depends,
                      commands, false);

  // Write the main driver rule to build everything in this target.
  this->WriteTargetDriverRule(targetOutputReal, relink);
#else
  static_cast<void>(linkRuleVar);
  static_cast<void>(extraFlags);
  static_cast<void>(relink);
#endif
}

void cmMakefileLibraryTargetGenerator::WriteLibraryRules(
  const std::string& linkRuleVar, const std::string& extraFlags, bool relink)
{
  // TODO: Merge the methods that call this method to avoid
  // code duplication.
  std::vector<std::string> commands;

  // Build list of dependencies.
  std::vector<std::string> depends;
  this->AppendLinkDepends(depends);
  if (!this->DeviceLinkObject.empty()) {
    depends.push_back(this->DeviceLinkObject);
  }

  // Get the language to use for linking this library.
  std::string linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);

  // Make sure we have a link language.
  if (linkLanguage.empty()) {
    cmSystemTools::Error("Cannot determine link language for target \"",
                         this->GeneratorTarget->GetName().c_str(), "\".");
    return;
  }

  // Create set of linking flags.
  std::string linkFlags;
  this->LocalGenerator->AppendFlags(linkFlags, extraFlags);
  this->LocalGenerator->AppendIPOLinkerFlags(linkFlags, this->GeneratorTarget,
                                             this->ConfigName, linkLanguage);

  // Add OSX version flags, if any.
  if (this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
      this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
    this->AppendOSXVerFlag(linkFlags, linkLanguage, "COMPATIBILITY", true);
    this->AppendOSXVerFlag(linkFlags, linkLanguage, "CURRENT", false);
  }

  // Construct the name of the library.
  std::string targetName;
  std::string targetNameSO;
  std::string targetNameReal;
  std::string targetNameImport;
  std::string targetNamePDB;
  this->GeneratorTarget->GetLibraryNames(targetName, targetNameSO,
                                         targetNameReal, targetNameImport,
                                         targetNamePDB, this->ConfigName);

  // Construct the full path version of the names.
  std::string outpath;
  std::string outpathImp;
  if (this->GeneratorTarget->IsFrameworkOnApple()) {
    outpath = this->GeneratorTarget->GetDirectory(this->ConfigName);
    this->OSXBundleGenerator->CreateFramework(targetName, outpath);
    outpath += "/";
  } else if (this->GeneratorTarget->IsCFBundleOnApple()) {
    outpath = this->GeneratorTarget->GetDirectory(this->ConfigName);
    this->OSXBundleGenerator->CreateCFBundle(targetName, outpath);
    outpath += "/";
  } else if (relink) {
    outpath = this->Makefile->GetCurrentBinaryDirectory();
    outpath += cmake::GetCMakeFilesDirectory();
    outpath += "/CMakeRelink.dir";
    cmSystemTools::MakeDirectory(outpath.c_str());
    outpath += "/";
    if (!targetNameImport.empty()) {
      outpathImp = outpath;
    }
  } else {
    outpath = this->GeneratorTarget->GetDirectory(this->ConfigName);
    cmSystemTools::MakeDirectory(outpath.c_str());
    outpath += "/";
    if (!targetNameImport.empty()) {
      outpathImp = this->GeneratorTarget->GetDirectory(
        this->ConfigName, cmStateEnums::ImportLibraryArtifact);
      cmSystemTools::MakeDirectory(outpathImp.c_str());
      outpathImp += "/";
    }
  }

  std::string compilePdbOutputPath =
    this->GeneratorTarget->GetCompilePDBDirectory(this->ConfigName);
  cmSystemTools::MakeDirectory(compilePdbOutputPath.c_str());

  std::string pdbOutputPath =
    this->GeneratorTarget->GetPDBDirectory(this->ConfigName);
  cmSystemTools::MakeDirectory(pdbOutputPath.c_str());
  pdbOutputPath += "/";

  std::string targetFullPath = outpath + targetName;
  std::string targetFullPathPDB = pdbOutputPath + targetNamePDB;
  std::string targetFullPathSO = outpath + targetNameSO;
  std::string targetFullPathReal = outpath + targetNameReal;
  std::string targetFullPathImport = outpathImp + targetNameImport;

  // Construct the output path version of the names for use in command
  // arguments.
  std::string targetOutPathPDB = this->LocalGenerator->ConvertToOutputFormat(
    targetFullPathPDB, cmOutputConverter::SHELL);

  std::string targetOutPath = this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPath),
    cmOutputConverter::SHELL);
  std::string targetOutPathSO = this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathSO),
    cmOutputConverter::SHELL);
  std::string targetOutPathReal = this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathReal),
    cmOutputConverter::SHELL);
  std::string targetOutPathImport =
    this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(),
        targetFullPathImport),
      cmOutputConverter::SHELL);

  this->NumberOfProgressActions++;
  if (!this->NoRuleMessages) {
    cmLocalUnixMakefileGenerator3::EchoProgress progress;
    this->MakeEchoProgress(progress);
    // Add the link message.
    std::string buildEcho = "Linking ";
    buildEcho += linkLanguage;
    switch (this->GeneratorTarget->GetType()) {
      case cmStateEnums::STATIC_LIBRARY:
        buildEcho += " static library ";
        break;
      case cmStateEnums::SHARED_LIBRARY:
        buildEcho += " shared library ";
        break;
      case cmStateEnums::MODULE_LIBRARY:
        if (this->GeneratorTarget->IsCFBundleOnApple()) {
          buildEcho += " CFBundle";
        }
        buildEcho += " shared module ";
        break;
      default:
        buildEcho += " library ";
        break;
    }
    buildEcho += targetOutPath;
    this->LocalGenerator->AppendEcho(
      commands, buildEcho, cmLocalUnixMakefileGenerator3::EchoLink, &progress);
  }

  // Clean files associated with this library.
  std::vector<std::string> libCleanFiles;
  libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
    this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathReal));

  std::vector<std::string> commands1;
  // Add a command to remove any existing files for this library.
  // for static libs only
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
    this->LocalGenerator->AppendCleanCommand(commands1, libCleanFiles,
                                             this->GeneratorTarget, "target");
    this->LocalGenerator->CreateCDCommand(
      commands1, this->Makefile->GetCurrentBinaryDirectory(),
      this->LocalGenerator->GetBinaryDirectory());
    commands.insert(commands.end(), commands1.begin(), commands1.end());
    commands1.clear();
  }

  if (targetName != targetNameReal) {
    libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPath));
  }
  if (targetNameSO != targetNameReal && targetNameSO != targetName) {
    libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathSO));
  }
  if (!targetNameImport.empty()) {
    libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(),
      targetFullPathImport));
    std::string implib;
    if (this->GeneratorTarget->GetImplibGNUtoMS(targetFullPathImport,
                                                implib)) {
      libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), implib));
    }
  }

  // List the PDB for cleaning only when the whole target is
  // cleaned.  We do not want to delete the .pdb file just before
  // linking the target.
  this->CleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
    this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathPDB));

#ifdef _WIN32
  // There may be a manifest file for this target.  Add it to the
  // clean set just in case.
  if (this->GeneratorTarget->GetType() != cmStateEnums::STATIC_LIBRARY) {
    libCleanFiles.push_back(this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(),
      (targetFullPath + ".manifest").c_str()));
  }
#endif

  // Add the pre-build and pre-link rules building but not when relinking.
  if (!relink) {
    this->LocalGenerator->AppendCustomCommands(
      commands, this->GeneratorTarget->GetPreBuildCommands(),
      this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());
    this->LocalGenerator->AppendCustomCommands(
      commands, this->GeneratorTarget->GetPreLinkCommands(),
      this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());
  }

  // Determine whether a link script will be used.
  bool useLinkScript = this->GlobalGenerator->GetUseLinkScript();

  bool useResponseFileForObjects =
    this->CheckUseResponseFileForObjects(linkLanguage);
  bool const useResponseFileForLibs =
    this->CheckUseResponseFileForLibraries(linkLanguage);

  // For static libraries there might be archiving rules.
  bool haveStaticLibraryRule = false;
  std::vector<std::string> archiveCreateCommands;
  std::vector<std::string> archiveAppendCommands;
  std::vector<std::string> archiveFinishCommands;
  std::string::size_type archiveCommandLimit = std::string::npos;
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
    haveStaticLibraryRule = this->Makefile->IsDefinitionSet(linkRuleVar);
    std::string arCreateVar = "CMAKE_";
    arCreateVar += linkLanguage;
    arCreateVar += "_ARCHIVE_CREATE";

    arCreateVar = this->GeneratorTarget->GetFeatureSpecificLinkRuleVariable(
      arCreateVar, linkLanguage, this->ConfigName);

    if (const char* rule = this->Makefile->GetDefinition(arCreateVar)) {
      cmSystemTools::ExpandListArgument(rule, archiveCreateCommands);
    }
    std::string arAppendVar = "CMAKE_";
    arAppendVar += linkLanguage;
    arAppendVar += "_ARCHIVE_APPEND";

    arAppendVar = this->GeneratorTarget->GetFeatureSpecificLinkRuleVariable(
      arAppendVar, linkLanguage, this->ConfigName);

    if (const char* rule = this->Makefile->GetDefinition(arAppendVar)) {
      cmSystemTools::ExpandListArgument(rule, archiveAppendCommands);
    }
    std::string arFinishVar = "CMAKE_";
    arFinishVar += linkLanguage;
    arFinishVar += "_ARCHIVE_FINISH";

    arFinishVar = this->GeneratorTarget->GetFeatureSpecificLinkRuleVariable(
      arFinishVar, linkLanguage, this->ConfigName);

    if (const char* rule = this->Makefile->GetDefinition(arFinishVar)) {
      cmSystemTools::ExpandListArgument(rule, archiveFinishCommands);
    }
  }

  // Decide whether to use archiving rules.
  bool useArchiveRules = !haveStaticLibraryRule &&
    !archiveCreateCommands.empty() && !archiveAppendCommands.empty();
  if (useArchiveRules) {
    // Archiving rules are always run with a link script.
    useLinkScript = true;

    // Archiving rules never use a response file.
    useResponseFileForObjects = false;

    // Limit the length of individual object lists to less than the
    // 32K command line length limit on Windows.  We could make this a
    // platform file variable but this should work everywhere.
    archiveCommandLimit = 30000;
  }

  // Expand the rule variables.
  std::vector<std::string> real_link_commands;
  {
    bool useWatcomQuote =
      this->Makefile->IsOn(linkRuleVar + "_USE_WATCOM_QUOTE");

    // Set path conversion for link script shells.
    this->LocalGenerator->SetLinkScriptShell(useLinkScript);

    // Collect up flags to link in needed libraries.
    std::string linkLibs;
    if (this->GeneratorTarget->GetType() != cmStateEnums::STATIC_LIBRARY) {

      CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
        this->CreateLinkLineComputer(
          this->LocalGenerator,
          this->LocalGenerator->GetStateSnapshot().GetDirectory()));
      linkLineComputer->SetForResponse(useResponseFileForLibs);
      linkLineComputer->SetUseWatcomQuote(useWatcomQuote);
      linkLineComputer->SetRelink(relink);

      this->CreateLinkLibs(linkLineComputer.get(), linkLibs,
                           useResponseFileForLibs, depends);
    }

    // Construct object file lists that may be needed to expand the
    // rule.
    std::string buildObjs;
    this->CreateObjectLists(useLinkScript, useArchiveRules,
                            useResponseFileForObjects, buildObjs, depends,
                            useWatcomQuote);
    if (!this->DeviceLinkObject.empty()) {
      buildObjs += " " +
        this->LocalGenerator->ConvertToOutputFormat(
          this->LocalGenerator->MaybeConvertToRelativePath(
            this->LocalGenerator->GetCurrentBinaryDirectory(),
            this->DeviceLinkObject),
          cmOutputConverter::SHELL);
    }

    // maybe create .def file from list of objects
    this->GenDefFile(real_link_commands);

    std::string manifests = this->GetManifests();

    cmRulePlaceholderExpander::RuleVariables vars;
    vars.TargetPDB = targetOutPathPDB.c_str();

    // Setup the target version.
    std::string targetVersionMajor;
    std::string targetVersionMinor;
    {
      std::ostringstream majorStream;
      std::ostringstream minorStream;
      int major;
      int minor;
      this->GeneratorTarget->GetTargetVersion(major, minor);
      majorStream << major;
      minorStream << minor;
      targetVersionMajor = majorStream.str();
      targetVersionMinor = minorStream.str();
    }
    vars.TargetVersionMajor = targetVersionMajor.c_str();
    vars.TargetVersionMinor = targetVersionMinor.c_str();

    vars.CMTargetName = this->GeneratorTarget->GetName().c_str();
    vars.CMTargetType =
      cmState::GetTargetTypeName(this->GeneratorTarget->GetType());
    vars.Language = linkLanguage.c_str();
    vars.Objects = buildObjs.c_str();
    std::string objectDir = this->GeneratorTarget->GetSupportDirectory();

    objectDir = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), objectDir),
      cmOutputConverter::SHELL);

    vars.ObjectDir = objectDir.c_str();
    cmOutputConverter::OutputFormat output = (useWatcomQuote)
      ? cmOutputConverter::WATCOMQUOTE
      : cmOutputConverter::SHELL;
    std::string target = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathReal),
      output);
    vars.Target = target.c_str();
    vars.LinkLibraries = linkLibs.c_str();
    vars.ObjectsQuoted = buildObjs.c_str();
    if (this->GeneratorTarget->HasSOName(this->ConfigName)) {
      vars.SONameFlag = this->Makefile->GetSONameFlag(linkLanguage);
      vars.TargetSOName = targetNameSO.c_str();
    }
    vars.LinkFlags = linkFlags.c_str();

    vars.Manifests = manifests.c_str();

    // Compute the directory portion of the install_name setting.
    std::string install_name_dir;
    if (this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY) {
      // Get the install_name directory for the build tree.
      install_name_dir =
        this->GeneratorTarget->GetInstallNameDirForBuildTree(this->ConfigName);

      // Set the rule variable replacement value.
      if (install_name_dir.empty()) {
        vars.TargetInstallNameDir = "";
      } else {
        // Convert to a path for the native build tool.
        install_name_dir = this->LocalGenerator->ConvertToOutputFormat(
          install_name_dir, cmOutputConverter::SHELL);
        vars.TargetInstallNameDir = install_name_dir.c_str();
      }
    }

    // Add language-specific flags.
    std::string langFlags;
    this->LocalGenerator->AddLanguageFlagsForLinking(
      langFlags, this->GeneratorTarget, linkLanguage, this->ConfigName);

    this->LocalGenerator->AddArchitectureFlags(
      langFlags, this->GeneratorTarget, linkLanguage, this->ConfigName);

    vars.LanguageCompileFlags = langFlags.c_str();

    std::string launcher;
    const char* val = this->LocalGenerator->GetRuleLauncher(
      this->GeneratorTarget, "RULE_LAUNCH_LINK");
    if (val && *val) {
      launcher = val;
      launcher += " ";
    }

    CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
      this->LocalGenerator->CreateRulePlaceholderExpander());
    // Construct the main link rule and expand placeholders.
    rulePlaceholderExpander->SetTargetImpLib(targetOutPathImport);
    if (useArchiveRules) {
      // Construct the individual object list strings.
      std::vector<std::string> object_strings;
      this->WriteObjectsStrings(object_strings, archiveCommandLimit);

      // Add the cuda device object to the list of archive files. This will
      // only occur on archives which have CUDA_RESOLVE_DEVICE_SYMBOLS enabled
      if (!this->DeviceLinkObject.empty()) {
        object_strings.push_back(this->LocalGenerator->ConvertToOutputFormat(
          this->LocalGenerator->MaybeConvertToRelativePath(
            this->LocalGenerator->GetCurrentBinaryDirectory(),
            this->DeviceLinkObject),
          cmOutputConverter::SHELL));
      }

      // Create the archive with the first set of objects.
      std::vector<std::string>::iterator osi = object_strings.begin();
      {
        vars.Objects = osi->c_str();
        for (std::vector<std::string>::const_iterator i =
               archiveCreateCommands.begin();
             i != archiveCreateCommands.end(); ++i) {
          std::string cmd = launcher + *i;
          rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator,
                                                       cmd, vars);
          real_link_commands.push_back(cmd);
        }
      }
      // Append to the archive with the other object sets.
      for (++osi; osi != object_strings.end(); ++osi) {
        vars.Objects = osi->c_str();
        for (std::vector<std::string>::const_iterator i =
               archiveAppendCommands.begin();
             i != archiveAppendCommands.end(); ++i) {
          std::string cmd = launcher + *i;
          rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator,
                                                       cmd, vars);
          real_link_commands.push_back(cmd);
        }
      }
      // Finish the archive.
      vars.Objects = "";
      for (std::vector<std::string>::const_iterator i =
             archiveFinishCommands.begin();
           i != archiveFinishCommands.end(); ++i) {
        std::string cmd = launcher + *i;
        rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator, cmd,
                                                     vars);
        // If there is no ranlib the command will be ":".  Skip it.
        if (!cmd.empty() && cmd[0] != ':') {
          real_link_commands.push_back(cmd);
        }
      }
    } else {
      // Get the set of commands.
      std::string linkRule = this->GetLinkRule(linkRuleVar);
      cmSystemTools::ExpandListArgument(linkRule, real_link_commands);
      if (this->GeneratorTarget->GetPropertyAsBool("LINK_WHAT_YOU_USE") &&
          (this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY)) {
        std::string cmakeCommand = this->LocalGenerator->ConvertToOutputFormat(
          cmSystemTools::GetCMakeCommand(), cmLocalGenerator::SHELL);
        cmakeCommand += " -E __run_iwyu --lwyu=";
        cmakeCommand += targetOutPathReal;
        real_link_commands.push_back(cmakeCommand);
      }

      // Expand placeholders.
      for (std::vector<std::string>::iterator i = real_link_commands.begin();
           i != real_link_commands.end(); ++i) {
        *i = launcher + *i;
        rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator, *i,
                                                     vars);
      }
    }

    // Restore path conversion to normal shells.
    this->LocalGenerator->SetLinkScriptShell(false);
  }

  // Optionally convert the build rule to use a script to avoid long
  // command lines in the make shell.
  if (useLinkScript) {
    // Use a link script.
    const char* name = (relink ? "relink.txt" : "link.txt");
    this->CreateLinkScript(name, real_link_commands, commands1, depends);
  } else {
    // No link script.  Just use the link rule directly.
    commands1 = real_link_commands;
  }
  this->LocalGenerator->CreateCDCommand(
    commands1, this->Makefile->GetCurrentBinaryDirectory(),
    this->LocalGenerator->GetBinaryDirectory());
  commands.insert(commands.end(), commands1.begin(), commands1.end());
  commands1.clear();

  // Add a rule to create necessary symlinks for the library.
  // Frameworks are handled by cmOSXBundleGenerator.
  if (targetOutPath != targetOutPathReal &&
      !this->GeneratorTarget->IsFrameworkOnApple()) {
    std::string symlink = "$(CMAKE_COMMAND) -E cmake_symlink_library ";
    symlink += targetOutPathReal;
    symlink += " ";
    symlink += targetOutPathSO;
    symlink += " ";
    symlink += targetOutPath;
    commands1.push_back(symlink);
    this->LocalGenerator->CreateCDCommand(
      commands1, this->Makefile->GetCurrentBinaryDirectory(),
      this->LocalGenerator->GetBinaryDirectory());
    commands.insert(commands.end(), commands1.begin(), commands1.end());
    commands1.clear();
  }

  // Add the post-build rules when building but not when relinking.
  if (!relink) {
    this->LocalGenerator->AppendCustomCommands(
      commands, this->GeneratorTarget->GetPostBuildCommands(),
      this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());
  }

  // Compute the list of outputs.
  std::vector<std::string> outputs(1, targetFullPathReal);
  if (targetNameSO != targetNameReal) {
    outputs.push_back(targetFullPathSO);
  }
  if (targetName != targetNameSO && targetName != targetNameReal) {
    outputs.push_back(targetFullPath);
  }

  // Write the build rule.
  this->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR, outputs, depends,
                      commands, false);

  // Write the main driver rule to build everything in this target.
  this->WriteTargetDriverRule(targetFullPath, relink);

  // Clean all the possible library names and symlinks.
  this->CleanFiles.insert(this->CleanFiles.end(), libCleanFiles.begin(),
                          libCleanFiles.end());
}
