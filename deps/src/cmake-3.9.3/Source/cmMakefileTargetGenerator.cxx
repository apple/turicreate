/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMakefileTargetGenerator.h"

#include <sstream>
#include <stdio.h>
#include <utility>

#include "cmAlgorithms.h"
#include "cmComputeLinkInformation.h"
#include "cmCustomCommand.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmMakefileExecutableTargetGenerator.h"
#include "cmMakefileLibraryTargetGenerator.h"
#include "cmMakefileUtilityTargetGenerator.h"
#include "cmOutputConverter.h"
#include "cmRulePlaceholderExpander.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmMakefileTargetGenerator::cmMakefileTargetGenerator(cmGeneratorTarget* target)
  : cmCommonTargetGenerator(target)
  , OSXBundleGenerator(CM_NULLPTR)
  , MacOSXContentGenerator(CM_NULLPTR)
{
  this->BuildFileStream = CM_NULLPTR;
  this->InfoFileStream = CM_NULLPTR;
  this->FlagFileStream = CM_NULLPTR;
  this->CustomCommandDriver = OnBuild;
  this->LocalGenerator =
    static_cast<cmLocalUnixMakefileGenerator3*>(target->GetLocalGenerator());
  this->GlobalGenerator = static_cast<cmGlobalUnixMakefileGenerator3*>(
    this->LocalGenerator->GetGlobalGenerator());
  cmake* cm = this->GlobalGenerator->GetCMakeInstance();
  this->NoRuleMessages = false;
  if (const char* ruleStatus =
        cm->GetState()->GetGlobalProperty("RULE_MESSAGES")) {
    this->NoRuleMessages = cmSystemTools::IsOff(ruleStatus);
  }
  MacOSXContentGenerator = new MacOSXContentGeneratorType(this);
}

cmMakefileTargetGenerator::~cmMakefileTargetGenerator()
{
  delete MacOSXContentGenerator;
}

cmMakefileTargetGenerator* cmMakefileTargetGenerator::New(
  cmGeneratorTarget* tgt)
{
  cmMakefileTargetGenerator* result = CM_NULLPTR;

  switch (tgt->GetType()) {
    case cmStateEnums::EXECUTABLE:
      result = new cmMakefileExecutableTargetGenerator(tgt);
      break;
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
    case cmStateEnums::OBJECT_LIBRARY:
      result = new cmMakefileLibraryTargetGenerator(tgt);
      break;
    case cmStateEnums::UTILITY:
      result = new cmMakefileUtilityTargetGenerator(tgt);
      break;
    default:
      return result;
      // break; /* unreachable */
  }
  return result;
}

void cmMakefileTargetGenerator::CreateRuleFile()
{
  // Create a directory for this target.
  this->TargetBuildDirectory =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  this->TargetBuildDirectoryFull =
    this->LocalGenerator->ConvertToFullPath(this->TargetBuildDirectory);
  cmSystemTools::MakeDirectory(this->TargetBuildDirectoryFull.c_str());

  // Construct the rule file name.
  this->BuildFileName = this->TargetBuildDirectory;
  this->BuildFileName += "/build.make";
  this->BuildFileNameFull = this->TargetBuildDirectoryFull;
  this->BuildFileNameFull += "/build.make";

  // Construct the rule file name.
  this->ProgressFileNameFull = this->TargetBuildDirectoryFull;
  this->ProgressFileNameFull += "/progress.make";

  // reset the progress count
  this->NumberOfProgressActions = 0;

  // Open the rule file.  This should be copy-if-different because the
  // rules may depend on this file itself.
  this->BuildFileStream =
    new cmGeneratedFileStream(this->BuildFileNameFull.c_str(), false,
                              this->GlobalGenerator->GetMakefileEncoding());
  this->BuildFileStream->SetCopyIfDifferent(true);
  if (!this->BuildFileStream) {
    return;
  }
  this->LocalGenerator->WriteDisclaimer(*this->BuildFileStream);
  if (this->GlobalGenerator->AllowDeleteOnError()) {
    std::vector<std::string> no_depends;
    std::vector<std::string> no_commands;
    this->LocalGenerator->WriteMakeRule(
      *this->BuildFileStream, "Delete rule output on recipe failure.",
      ".DELETE_ON_ERROR", no_depends, no_commands, false);
  }
  this->LocalGenerator->WriteSpecialTargetsTop(*this->BuildFileStream);
}

void cmMakefileTargetGenerator::WriteTargetBuildRules()
{
  const std::string& config =
    this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");

  // write the custom commands for this target
  // Look for files registered for cleaning in this directory.
  if (const char* additional_clean_files =
        this->Makefile->GetProperty("ADDITIONAL_MAKE_CLEAN_FILES")) {
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge =
      ge.Parse(additional_clean_files);

    cmSystemTools::ExpandListArgument(
      cge->Evaluate(this->LocalGenerator, config, false, this->GeneratorTarget,
                    CM_NULLPTR, CM_NULLPTR),
      this->CleanFiles);
  }

  // add custom commands to the clean rules?
  const char* clean_no_custom = this->Makefile->GetProperty("CLEAN_NO_CUSTOM");
  bool clean = cmSystemTools::IsOff(clean_no_custom);

  // First generate the object rule files.  Save a list of all object
  // files for this target.
  std::vector<cmSourceFile const*> customCommands;
  this->GeneratorTarget->GetCustomCommands(customCommands, config);
  std::string currentBinDir =
    this->LocalGenerator->GetCurrentBinaryDirectory();
  for (std::vector<cmSourceFile const*>::const_iterator si =
         customCommands.begin();
       si != customCommands.end(); ++si) {
    cmCustomCommandGenerator ccg(*(*si)->GetCustomCommand(), this->ConfigName,
                                 this->LocalGenerator);
    this->GenerateCustomRuleFile(ccg);
    if (clean) {
      const std::vector<std::string>& outputs = ccg.GetOutputs();
      for (std::vector<std::string>::const_iterator o = outputs.begin();
           o != outputs.end(); ++o) {
        this->CleanFiles.push_back(
          this->LocalGenerator->MaybeConvertToRelativePath(currentBinDir, *o));
      }
    }
  }
  std::vector<cmSourceFile const*> headerSources;
  this->GeneratorTarget->GetHeaderSources(headerSources, config);
  this->OSXBundleGenerator->GenerateMacOSXContentStatements(
    headerSources, this->MacOSXContentGenerator);
  std::vector<cmSourceFile const*> extraSources;
  this->GeneratorTarget->GetExtraSources(extraSources, config);
  this->OSXBundleGenerator->GenerateMacOSXContentStatements(
    extraSources, this->MacOSXContentGenerator);
  std::vector<cmSourceFile const*> externalObjects;
  this->GeneratorTarget->GetExternalObjects(externalObjects, config);
  for (std::vector<cmSourceFile const*>::const_iterator si =
         externalObjects.begin();
       si != externalObjects.end(); ++si) {
    this->ExternalObjects.push_back((*si)->GetFullPath());
  }
  std::vector<cmSourceFile const*> objectSources;
  this->GeneratorTarget->GetObjectSources(objectSources, config);
  for (std::vector<cmSourceFile const*>::const_iterator si =
         objectSources.begin();
       si != objectSources.end(); ++si) {
    // Generate this object file's rule file.
    this->WriteObjectRuleFiles(**si);
  }
}

void cmMakefileTargetGenerator::WriteCommonCodeRules()
{
  const char* root = (this->Makefile->IsOn("CMAKE_MAKE_INCLUDE_FROM_ROOT")
                        ? "$(CMAKE_BINARY_DIR)/"
                        : "");

  // Include the dependencies for the target.
  std::string dependFileNameFull = this->TargetBuildDirectoryFull;
  dependFileNameFull += "/depend.make";
  *this->BuildFileStream
    << "# Include any dependencies generated for this target.\n"
    << this->GlobalGenerator->IncludeDirective << " " << root
    << cmSystemTools::ConvertToOutputPath(
         this->LocalGenerator
           ->MaybeConvertToRelativePath(
             this->LocalGenerator->GetBinaryDirectory(), dependFileNameFull)
           .c_str())
    << "\n\n";

  if (!this->NoRuleMessages) {
    // Include the progress variables for the target.
    *this->BuildFileStream
      << "# Include the progress variables for this target.\n"
      << this->GlobalGenerator->IncludeDirective << " " << root
      << cmSystemTools::ConvertToOutputPath(
           this->LocalGenerator
             ->MaybeConvertToRelativePath(
               this->LocalGenerator->GetBinaryDirectory(),
               this->ProgressFileNameFull)
             .c_str())
      << "\n\n";
  }

  // make sure the depend file exists
  if (!cmSystemTools::FileExists(dependFileNameFull.c_str())) {
    // Write an empty dependency file.
    cmGeneratedFileStream depFileStream(
      dependFileNameFull.c_str(), false,
      this->GlobalGenerator->GetMakefileEncoding());
    depFileStream << "# Empty dependencies file for "
                  << this->GeneratorTarget->GetName() << ".\n"
                  << "# This may be replaced when dependencies are built."
                  << std::endl;
  }

  // Open the flags file.  This should be copy-if-different because the
  // rules may depend on this file itself.
  this->FlagFileNameFull = this->TargetBuildDirectoryFull;
  this->FlagFileNameFull += "/flags.make";
  this->FlagFileStream =
    new cmGeneratedFileStream(this->FlagFileNameFull.c_str(), false,
                              this->GlobalGenerator->GetMakefileEncoding());
  this->FlagFileStream->SetCopyIfDifferent(true);
  if (!this->FlagFileStream) {
    return;
  }
  this->LocalGenerator->WriteDisclaimer(*this->FlagFileStream);

  // Include the flags for the target.
  *this->BuildFileStream
    << "# Include the compile flags for this target's objects.\n"
    << this->GlobalGenerator->IncludeDirective << " " << root
    << cmSystemTools::ConvertToOutputPath(
         this->LocalGenerator
           ->MaybeConvertToRelativePath(
             this->LocalGenerator->GetBinaryDirectory(),
             this->FlagFileNameFull)
           .c_str())
    << "\n\n";
}

void cmMakefileTargetGenerator::WriteTargetLanguageFlags()
{
  // write language flags for target
  std::set<std::string> languages;
  this->GeneratorTarget->GetLanguages(
    languages, this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
  // put the compiler in the rules.make file so that if it changes
  // things rebuild
  for (std::set<std::string>::const_iterator l = languages.begin();
       l != languages.end(); ++l) {
    std::string compiler = "CMAKE_";
    compiler += *l;
    compiler += "_COMPILER";
    *this->FlagFileStream << "# compile " << *l << " with "
                          << this->Makefile->GetSafeDefinition(compiler)
                          << "\n";
  }

  for (std::set<std::string>::const_iterator l = languages.begin();
       l != languages.end(); ++l) {
    std::string flags = this->GetFlags(*l);
    std::string defines = this->GetDefines(*l);
    std::string includes = this->GetIncludes(*l);
    // Escape comment characters so they do not terminate assignment.
    cmSystemTools::ReplaceString(flags, "#", "\\#");
    cmSystemTools::ReplaceString(defines, "#", "\\#");
    cmSystemTools::ReplaceString(includes, "#", "\\#");
    *this->FlagFileStream << *l << "_FLAGS = " << flags << "\n\n";
    *this->FlagFileStream << *l << "_DEFINES = " << defines << "\n\n";
    *this->FlagFileStream << *l << "_INCLUDES = " << includes << "\n\n";
  }
}

void cmMakefileTargetGenerator::MacOSXContentGeneratorType::operator()(
  cmSourceFile const& source, const char* pkgloc)
{
  // Skip OS X content when not building a Framework or Bundle.
  if (!this->Generator->GetGeneratorTarget()->IsBundleOnApple()) {
    return;
  }

  std::string macdir =
    this->Generator->OSXBundleGenerator->InitMacOSXContentDirectory(pkgloc);

  // Get the input file location.
  std::string const& input = source.GetFullPath();

  // Get the output file location.
  std::string output = macdir;
  output += "/";
  output += cmSystemTools::GetFilenameName(input);
  this->Generator->CleanFiles.push_back(
    this->Generator->LocalGenerator->MaybeConvertToRelativePath(
      this->Generator->LocalGenerator->GetCurrentBinaryDirectory(), output));
  output = this->Generator->LocalGenerator->MaybeConvertToRelativePath(
    this->Generator->LocalGenerator->GetBinaryDirectory(), output);

  // Create a rule to copy the content into the bundle.
  std::vector<std::string> depends;
  std::vector<std::string> commands;
  depends.push_back(input);
  std::string copyEcho = "Copying OS X content ";
  copyEcho += output;
  this->Generator->LocalGenerator->AppendEcho(
    commands, copyEcho, cmLocalUnixMakefileGenerator3::EchoBuild);
  std::string copyCommand = "$(CMAKE_COMMAND) -E copy ";
  copyCommand += this->Generator->LocalGenerator->ConvertToOutputFormat(
    input, cmOutputConverter::SHELL);
  copyCommand += " ";
  copyCommand += this->Generator->LocalGenerator->ConvertToOutputFormat(
    output, cmOutputConverter::SHELL);
  commands.push_back(copyCommand);
  this->Generator->LocalGenerator->WriteMakeRule(
    *this->Generator->BuildFileStream, CM_NULLPTR, output, depends, commands,
    false);
  this->Generator->ExtraFiles.insert(output);
}

void cmMakefileTargetGenerator::WriteObjectRuleFiles(
  cmSourceFile const& source)
{
  // Identify the language of the source file.
  const std::string& lang =
    this->LocalGenerator->GetSourceFileLanguage(source);
  if (lang.empty()) {
    // don't know anything about this file so skip it
    return;
  }

  // Get the full path name of the object file.
  std::string const& objectName =
    this->GeneratorTarget->GetObjectName(&source);
  std::string obj =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  obj += "/";
  obj += objectName;

  // Avoid generating duplicate rules.
  if (this->ObjectFiles.find(obj) == this->ObjectFiles.end()) {
    this->ObjectFiles.insert(obj);
  } else {
    std::ostringstream err;
    err << "Warning: Source file \"" << source.GetFullPath()
        << "\" is listed multiple times for target \""
        << this->GeneratorTarget->GetName() << "\".";
    cmSystemTools::Message(err.str().c_str(), "Warning");
    return;
  }

  // Create the directory containing the object file.  This may be a
  // subdirectory under the target's directory.
  std::string dir = cmSystemTools::GetFilenamePath(obj);
  cmSystemTools::MakeDirectory(
    this->LocalGenerator->ConvertToFullPath(dir).c_str());

  // Save this in the target's list of object files.
  this->Objects.push_back(obj);
  this->CleanFiles.push_back(obj);

  // TODO: Remove
  // std::string relativeObj
  //= this->LocalGenerator->GetHomeRelativeOutputPath();
  // relativeObj += obj;

  // we compute some depends when writing the depend.make that we will also
  // use in the build.make, same with depMakeFile
  std::vector<std::string> depends;

  // generate the build rule file
  this->WriteObjectBuildFile(obj, lang, source, depends);

  // The object file should be checked for dependency integrity.
  std::string objFullPath = this->LocalGenerator->GetCurrentBinaryDirectory();
  objFullPath += "/";
  objFullPath += obj;
  objFullPath = cmSystemTools::CollapseFullPath(objFullPath);
  std::string srcFullPath =
    cmSystemTools::CollapseFullPath(source.GetFullPath());
  this->LocalGenerator->AddImplicitDepends(
    this->GeneratorTarget, lang, objFullPath.c_str(), srcFullPath.c_str());
}

void cmMakefileTargetGenerator::WriteObjectBuildFile(
  std::string& obj, const std::string& lang, cmSourceFile const& source,
  std::vector<std::string>& depends)
{
  this->LocalGenerator->AppendRuleDepend(depends,
                                         this->FlagFileNameFull.c_str());
  this->LocalGenerator->AppendRuleDepends(depends,
                                          this->FlagFileDepends[lang]);

  // generate the depend scanning rule
  this->WriteObjectDependRules(source, depends);

  std::string relativeObj = this->LocalGenerator->GetHomeRelativeOutputPath();
  relativeObj += obj;
  // Write the build rule.

  // Build the set of compiler flags.
  std::string flags;

  // Add language-specific flags.
  std::string langFlags = "$(";
  langFlags += lang;
  langFlags += "_FLAGS)";
  this->LocalGenerator->AppendFlags(flags, langFlags);

  std::string config = this->LocalGenerator->GetConfigName();
  std::string configUpper = cmSystemTools::UpperCase(config);

  // Add Fortran format flags.
  if (lang == "Fortran") {
    this->AppendFortranFormatFlags(flags, source);
  }

  // Add flags from source file properties.
  if (const char* cflags = source.GetProperty("COMPILE_FLAGS")) {
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(cflags);
    const char* evaluatedFlags = cge->Evaluate(this->LocalGenerator, config,
                                               false, this->GeneratorTarget);
    this->LocalGenerator->AppendFlags(flags, evaluatedFlags);
    *this->FlagFileStream << "# Custom flags: " << relativeObj
                          << "_FLAGS = " << evaluatedFlags << "\n"
                          << "\n";
  }

  // Add language-specific defines.
  std::set<std::string> defines;

  // Add source-sepcific preprocessor definitions.
  if (const char* compile_defs = source.GetProperty("COMPILE_DEFINITIONS")) {
    this->LocalGenerator->AppendDefines(defines, compile_defs);
    *this->FlagFileStream << "# Custom defines: " << relativeObj
                          << "_DEFINES = " << compile_defs << "\n"
                          << "\n";
  }
  std::string defPropName = "COMPILE_DEFINITIONS_";
  defPropName += configUpper;
  if (const char* config_compile_defs = source.GetProperty(defPropName)) {
    this->LocalGenerator->AppendDefines(defines, config_compile_defs);
    *this->FlagFileStream << "# Custom defines: " << relativeObj << "_DEFINES_"
                          << configUpper << " = " << config_compile_defs
                          << "\n"
                          << "\n";
  }

  // Get the output paths for source and object files.
  std::string sourceFile = this->LocalGenerator->ConvertToOutputFormat(
    source.GetFullPath(), cmOutputConverter::SHELL);

  // Construct the build message.
  std::vector<std::string> no_commands;
  std::vector<std::string> commands;

  // add in a progress call if needed
  this->NumberOfProgressActions++;

  if (!this->NoRuleMessages) {
    cmLocalUnixMakefileGenerator3::EchoProgress progress;
    this->MakeEchoProgress(progress);
    std::string buildEcho = "Building ";
    buildEcho += lang;
    buildEcho += " object ";
    buildEcho += relativeObj;
    this->LocalGenerator->AppendEcho(commands, buildEcho,
                                     cmLocalUnixMakefileGenerator3::EchoBuild,
                                     &progress);
  }

  std::string targetOutPathReal;
  std::string targetOutPathPDB;
  std::string targetOutPathCompilePDB;
  {
    std::string targetFullPathReal;
    std::string targetFullPathPDB;
    std::string targetFullPathCompilePDB = this->ComputeTargetCompilePDB();
    if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE ||
        this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY ||
        this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
        this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
      targetFullPathReal = this->GeneratorTarget->GetFullPath(
        this->ConfigName, cmStateEnums::RuntimeBinaryArtifact, true);
      targetFullPathPDB =
        this->GeneratorTarget->GetPDBDirectory(this->ConfigName);
      targetFullPathPDB += "/";
      targetFullPathPDB += this->GeneratorTarget->GetPDBName(this->ConfigName);
    }

    targetOutPathReal = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), targetFullPathReal),
      cmOutputConverter::SHELL);
    targetOutPathPDB = this->LocalGenerator->ConvertToOutputFormat(
      targetFullPathPDB, cmOutputConverter::SHELL);
    targetOutPathCompilePDB = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(),
        targetFullPathCompilePDB),
      cmOutputConverter::SHELL);

    if (this->LocalGenerator->IsMinGWMake() &&
        cmHasLiteralSuffix(targetOutPathCompilePDB, "\\")) {
      // mingw32-make incorrectly interprets 'a\ b c' as 'a b' and 'c'
      // (but 'a\ b "c"' as 'a\', 'b', and 'c'!).  Workaround this by
      // avoiding a trailing backslash in the argument.
      targetOutPathCompilePDB[targetOutPathCompilePDB.size() - 1] = '/';
    }
  }
  cmRulePlaceholderExpander::RuleVariables vars;
  vars.CMTargetName = this->GeneratorTarget->GetName().c_str();
  vars.CMTargetType =
    cmState::GetTargetTypeName(this->GeneratorTarget->GetType());
  vars.Language = lang.c_str();
  vars.Target = targetOutPathReal.c_str();
  vars.TargetPDB = targetOutPathPDB.c_str();
  vars.TargetCompilePDB = targetOutPathCompilePDB.c_str();
  vars.Source = sourceFile.c_str();
  std::string shellObj =
    this->LocalGenerator->ConvertToOutputFormat(obj, cmOutputConverter::SHELL);
  vars.Object = shellObj.c_str();
  std::string objectDir = this->GeneratorTarget->GetSupportDirectory();
  objectDir = this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), objectDir),
    cmOutputConverter::SHELL);
  vars.ObjectDir = objectDir.c_str();
  std::string objectFileDir = cmSystemTools::GetFilenamePath(obj);
  objectFileDir = this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), objectFileDir),
    cmOutputConverter::SHELL);
  vars.ObjectFileDir = objectFileDir.c_str();
  vars.Flags = flags.c_str();

  std::string definesString = "$(";
  definesString += lang;
  definesString += "_DEFINES)";

  this->LocalGenerator->JoinDefines(defines, definesString, lang);

  vars.Defines = definesString.c_str();

  std::string const includesString = "$(" + lang + "_INCLUDES)";
  vars.Includes = includesString.c_str();

  // At the moment, it is assumed that C, C++, Fortran, and CUDA have both
  // assembly and preprocessor capabilities. The same is true for the
  // ability to export compile commands
  bool lang_has_preprocessor = ((lang == "C") || (lang == "CXX") ||
                                (lang == "Fortran") || (lang == "CUDA"));
  bool const lang_has_assembly = lang_has_preprocessor;
  bool const lang_can_export_cmds = lang_has_preprocessor;

  CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
    this->LocalGenerator->CreateRulePlaceholderExpander());

  // Construct the compile rules.
  {
    std::vector<std::string> compileCommands;
    if (lang == "CUDA") {
      std::string cmdVar;
      if (this->GeneratorTarget->GetPropertyAsBool(
            "CUDA_SEPARABLE_COMPILATION")) {
        cmdVar = std::string("CMAKE_CUDA_COMPILE_SEPARABLE_COMPILATION");
      } else if (this->GeneratorTarget->GetPropertyAsBool(
                   "CUDA_PTX_COMPILATION")) {
        cmdVar = std::string("CMAKE_CUDA_COMPILE_PTX_COMPILATION");
      } else {
        cmdVar = std::string("CMAKE_CUDA_COMPILE_WHOLE_COMPILATION");
      }
      std::string compileRule = this->Makefile->GetRequiredDefinition(cmdVar);
      cmSystemTools::ExpandListArgument(compileRule, compileCommands);
    } else {
      const std::string cmdVar =
        std::string("CMAKE_") + lang + "_COMPILE_OBJECT";
      std::string compileRule = this->Makefile->GetRequiredDefinition(cmdVar);
      cmSystemTools::ExpandListArgument(compileRule, compileCommands);
    }

    if (this->Makefile->IsOn("CMAKE_EXPORT_COMPILE_COMMANDS") &&
        lang_can_export_cmds && compileCommands.size() == 1) {
      std::string compileCommand = compileCommands[0];

      // no launcher for CMAKE_EXPORT_COMPILE_COMMANDS
      rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator,
                                                   compileCommand, vars);
      std::string workingDirectory = cmSystemTools::CollapseFullPath(
        this->LocalGenerator->GetCurrentBinaryDirectory());
      compileCommand.replace(compileCommand.find(langFlags), langFlags.size(),
                             this->GetFlags(lang));
      std::string langDefines = std::string("$(") + lang + "_DEFINES)";
      compileCommand.replace(compileCommand.find(langDefines),
                             langDefines.size(), this->GetDefines(lang));
      std::string langIncludes = std::string("$(") + lang + "_INCLUDES)";
      compileCommand.replace(compileCommand.find(langIncludes),
                             langIncludes.size(), this->GetIncludes(lang));
      this->GlobalGenerator->AddCXXCompileCommand(
        source.GetFullPath(), workingDirectory, compileCommand);
    }

    // Maybe insert an include-what-you-use runner.
    if (!compileCommands.empty() && (lang == "C" || lang == "CXX")) {
      std::string const iwyu_prop = lang + "_INCLUDE_WHAT_YOU_USE";
      const char* iwyu = this->GeneratorTarget->GetProperty(iwyu_prop);
      std::string const tidy_prop = lang + "_CLANG_TIDY";
      const char* tidy = this->GeneratorTarget->GetProperty(tidy_prop);
      std::string const cpplint_prop = lang + "_CPPLINT";
      const char* cpplint = this->GeneratorTarget->GetProperty(cpplint_prop);
      if ((iwyu && *iwyu) || (tidy && *tidy) || (cpplint && *cpplint)) {
        std::string run_iwyu = "$(CMAKE_COMMAND) -E __run_iwyu";
        if (iwyu && *iwyu) {
          run_iwyu += " --iwyu=";
          run_iwyu += this->LocalGenerator->EscapeForShell(iwyu);
        }
        if (tidy && *tidy) {
          run_iwyu += " --tidy=";
          run_iwyu += this->LocalGenerator->EscapeForShell(tidy);
        }
        if (cpplint && *cpplint) {
          run_iwyu += " --cpplint=";
          run_iwyu += this->LocalGenerator->EscapeForShell(cpplint);
        }
        if ((tidy && *tidy) || (cpplint && *cpplint)) {
          run_iwyu += " --source=";
          run_iwyu += sourceFile;
        }
        run_iwyu += " -- ";
        compileCommands.front().insert(0, run_iwyu);
      }
    }

    // Maybe insert a compiler launcher like ccache or distcc
    if (!compileCommands.empty() && (lang == "C" || lang == "CXX")) {
      std::string const clauncher_prop = lang + "_COMPILER_LAUNCHER";
      const char* clauncher =
        this->GeneratorTarget->GetProperty(clauncher_prop);
      if (clauncher && *clauncher) {
        std::vector<std::string> launcher_cmd;
        cmSystemTools::ExpandListArgument(clauncher, launcher_cmd, true);
        for (std::vector<std::string>::iterator i = launcher_cmd.begin(),
                                                e = launcher_cmd.end();
             i != e; ++i) {
          *i = this->LocalGenerator->EscapeForShell(*i);
        }
        std::string const& run_launcher = cmJoin(launcher_cmd, " ") + " ";
        compileCommands.front().insert(0, run_launcher);
      }
    }

    std::string launcher;
    {
      const char* val = this->LocalGenerator->GetRuleLauncher(
        this->GeneratorTarget, "RULE_LAUNCH_COMPILE");
      if (val && *val) {
        launcher = val;
        launcher += " ";
      }
    }

    // Expand placeholders in the commands.
    for (std::vector<std::string>::iterator i = compileCommands.begin();
         i != compileCommands.end(); ++i) {
      *i = launcher + *i;
      rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator, *i,
                                                   vars);
    }

    // Change the command working directory to the local build tree.
    this->LocalGenerator->CreateCDCommand(
      compileCommands, this->LocalGenerator->GetCurrentBinaryDirectory(),
      this->LocalGenerator->GetBinaryDirectory());
    commands.insert(commands.end(), compileCommands.begin(),
                    compileCommands.end());
  }

  // Check for extra outputs created by the compilation.
  std::vector<std::string> outputs(1, relativeObj);
  if (const char* extra_outputs_str = source.GetProperty("OBJECT_OUTPUTS")) {
    // Register these as extra files to clean.
    cmSystemTools::ExpandListArgument(extra_outputs_str, outputs);
    this->CleanFiles.insert(this->CleanFiles.end(), outputs.begin() + 1,
                            outputs.end());
  }

  // Write the rule.
  this->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR, outputs, depends,
                      commands);

  bool do_preprocess_rules = lang_has_preprocessor &&
    this->LocalGenerator->GetCreatePreprocessedSourceRules();
  bool do_assembly_rules =
    lang_has_assembly && this->LocalGenerator->GetCreateAssemblySourceRules();
  if (do_preprocess_rules || do_assembly_rules) {
    std::vector<std::string> force_depends;
    force_depends.push_back("cmake_force");
    std::string::size_type dot_pos = relativeObj.rfind('.');
    std::string relativeObjBase = relativeObj.substr(0, dot_pos);
    dot_pos = obj.rfind('.');
    std::string objBase = obj.substr(0, dot_pos);

    if (do_preprocess_rules) {
      commands.clear();
      std::string relativeObjI = relativeObjBase + ".i";
      std::string objI = objBase + ".i";

      std::string preprocessEcho = "Preprocessing ";
      preprocessEcho += lang;
      preprocessEcho += " source to ";
      preprocessEcho += objI;
      this->LocalGenerator->AppendEcho(
        commands, preprocessEcho, cmLocalUnixMakefileGenerator3::EchoBuild);

      std::string preprocessRuleVar = "CMAKE_";
      preprocessRuleVar += lang;
      preprocessRuleVar += "_CREATE_PREPROCESSED_SOURCE";
      if (const char* preprocessRule =
            this->Makefile->GetDefinition(preprocessRuleVar)) {
        std::vector<std::string> preprocessCommands;
        cmSystemTools::ExpandListArgument(preprocessRule, preprocessCommands);

        std::string shellObjI = this->LocalGenerator->ConvertToOutputFormat(
          objI, cmOutputConverter::SHELL);
        vars.PreprocessedSource = shellObjI.c_str();

        // Expand placeholders in the commands.
        for (std::vector<std::string>::iterator i = preprocessCommands.begin();
             i != preprocessCommands.end(); ++i) {
          // no launcher for preprocessor commands
          rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator,
                                                       *i, vars);
        }

        this->LocalGenerator->CreateCDCommand(
          preprocessCommands,
          this->LocalGenerator->GetCurrentBinaryDirectory(),
          this->LocalGenerator->GetBinaryDirectory());
        commands.insert(commands.end(), preprocessCommands.begin(),
                        preprocessCommands.end());
      } else {
        std::string cmd = "$(CMAKE_COMMAND) -E cmake_unimplemented_variable ";
        cmd += preprocessRuleVar;
        commands.push_back(cmd);
      }

      this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                          relativeObjI, force_depends,
                                          commands, false);
    }

    if (do_assembly_rules) {
      commands.clear();
      std::string relativeObjS = relativeObjBase + ".s";
      std::string objS = objBase + ".s";

      std::string assemblyEcho = "Compiling ";
      assemblyEcho += lang;
      assemblyEcho += " source to assembly ";
      assemblyEcho += objS;
      this->LocalGenerator->AppendEcho(
        commands, assemblyEcho, cmLocalUnixMakefileGenerator3::EchoBuild);

      std::string assemblyRuleVar = "CMAKE_";
      assemblyRuleVar += lang;
      assemblyRuleVar += "_CREATE_ASSEMBLY_SOURCE";
      if (const char* assemblyRule =
            this->Makefile->GetDefinition(assemblyRuleVar)) {
        std::vector<std::string> assemblyCommands;
        cmSystemTools::ExpandListArgument(assemblyRule, assemblyCommands);

        std::string shellObjS = this->LocalGenerator->ConvertToOutputFormat(
          objS, cmOutputConverter::SHELL);
        vars.AssemblySource = shellObjS.c_str();

        // Expand placeholders in the commands.
        for (std::vector<std::string>::iterator i = assemblyCommands.begin();
             i != assemblyCommands.end(); ++i) {
          // no launcher for assembly commands
          rulePlaceholderExpander->ExpandRuleVariables(this->LocalGenerator,
                                                       *i, vars);
        }

        this->LocalGenerator->CreateCDCommand(
          assemblyCommands, this->LocalGenerator->GetCurrentBinaryDirectory(),
          this->LocalGenerator->GetBinaryDirectory());
        commands.insert(commands.end(), assemblyCommands.begin(),
                        assemblyCommands.end());
      } else {
        std::string cmd = "$(CMAKE_COMMAND) -E cmake_unimplemented_variable ";
        cmd += assemblyRuleVar;
        commands.push_back(cmd);
      }

      this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                          relativeObjS, force_depends,
                                          commands, false);
    }
  }

  // If the language needs provides-requires mode, create the
  // corresponding targets.
  std::string objectRequires = relativeObj;
  objectRequires += ".requires";
  std::vector<std::string> p_depends;
  // always provide an empty requires target
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      objectRequires, p_depends, no_commands,
                                      true);

  // write a build rule to recursively build what this obj provides
  std::string objectProvides = relativeObj;
  objectProvides += ".provides";
  std::string temp = relativeObj;
  temp += ".provides.build";
  std::vector<std::string> r_commands;
  std::string tgtMakefileName =
    this->LocalGenerator->GetRelativeTargetDirectory(this->GeneratorTarget);
  tgtMakefileName += "/build.make";
  r_commands.push_back(
    this->LocalGenerator->GetRecursiveMakeCall(tgtMakefileName.c_str(), temp));

  p_depends.clear();
  p_depends.push_back(objectRequires);
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      objectProvides, p_depends, r_commands,
                                      true);

  // write the provides.build rule dependency on the obj file
  p_depends.clear();
  p_depends.push_back(relativeObj);
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR, temp,
                                      p_depends, no_commands, false);
}

void cmMakefileTargetGenerator::WriteTargetRequiresRules()
{
  std::vector<std::string> depends;
  std::vector<std::string> no_commands;

  // Construct the name of the dependency generation target.
  std::string depTarget =
    this->LocalGenerator->GetRelativeTargetDirectory(this->GeneratorTarget);
  depTarget += "/requires";

  // This target drives dependency generation for all object files.
  std::string relPath = this->LocalGenerator->GetHomeRelativeOutputPath();
  std::string objTarget;
  for (std::vector<std::string>::const_iterator obj = this->Objects.begin();
       obj != this->Objects.end(); ++obj) {
    objTarget = relPath;
    objTarget += *obj;
    objTarget += ".requires";
    depends.push_back(objTarget);
  }

  // Write the rule.
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      depTarget, depends, no_commands, true);
}

void cmMakefileTargetGenerator::WriteTargetCleanRules()
{
  std::vector<std::string> depends;
  std::vector<std::string> commands;

  // Construct the clean target name.
  std::string cleanTarget =
    this->LocalGenerator->GetRelativeTargetDirectory(this->GeneratorTarget);
  cleanTarget += "/clean";

  // Construct the clean command.
  this->LocalGenerator->AppendCleanCommand(commands, this->CleanFiles,
                                           this->GeneratorTarget);
  this->LocalGenerator->CreateCDCommand(
    commands, this->LocalGenerator->GetCurrentBinaryDirectory(),
    this->LocalGenerator->GetBinaryDirectory());

  // Write the rule.
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      cleanTarget, depends, commands, true);
}

bool cmMakefileTargetGenerator::WriteMakeRule(
  std::ostream& os, const char* comment,
  const std::vector<std::string>& outputs,
  const std::vector<std::string>& depends,
  const std::vector<std::string>& commands, bool in_help)
{
  bool symbolic = false;
  if (outputs.empty()) {
    return symbolic;
  }

  // Check whether we need to bother checking for a symbolic output.
  bool need_symbolic = this->GlobalGenerator->GetNeedSymbolicMark();

  // Check whether the first output is marked as symbolic.
  if (need_symbolic) {
    if (cmSourceFile* sf = this->Makefile->GetSource(outputs[0])) {
      symbolic = sf->GetPropertyAsBool("SYMBOLIC");
    }
  }

  // We always attach the actual commands to the first output.
  this->LocalGenerator->WriteMakeRule(os, comment, outputs[0], depends,
                                      commands, symbolic, in_help);

  // For single outputs, we are done.
  if (outputs.size() == 1) {
    return symbolic;
  }

  // For multiple outputs, make the extra ones depend on the first one.
  std::vector<std::string> const output_depends(1, outputs[0]);
  std::string binDir = this->LocalGenerator->GetBinaryDirectory();
  for (std::vector<std::string>::const_iterator o = outputs.begin() + 1;
       o != outputs.end(); ++o) {
    // Touch the extra output so "make" knows that it was updated,
    // but only if the output was acually created.
    std::string const out = this->LocalGenerator->ConvertToOutputFormat(
      this->LocalGenerator->MaybeConvertToRelativePath(binDir, *o),
      cmOutputConverter::SHELL);
    std::vector<std::string> output_commands;

    bool o_symbolic = false;
    if (need_symbolic) {
      if (cmSourceFile* sf = this->Makefile->GetSource(*o)) {
        o_symbolic = sf->GetPropertyAsBool("SYMBOLIC");
      }
    }
    symbolic = symbolic && o_symbolic;

    if (!o_symbolic) {
      output_commands.push_back("@$(CMAKE_COMMAND) -E touch_nocreate " + out);
    }
    this->LocalGenerator->WriteMakeRule(os, CM_NULLPTR, *o, output_depends,
                                        output_commands, o_symbolic, in_help);

    if (!o_symbolic) {
      // At build time, remove the first output if this one does not exist
      // so that "make" will rerun the real commands that create this one.
      MultipleOutputPairsType::value_type p(*o, outputs[0]);
      this->MultipleOutputPairs.insert(p);
    }
  }
  return symbolic;
}

void cmMakefileTargetGenerator::WriteTargetDependRules()
{
  // must write the targets depend info file
  std::string dir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  this->InfoFileNameFull = dir;
  this->InfoFileNameFull += "/DependInfo.cmake";
  this->InfoFileNameFull =
    this->LocalGenerator->ConvertToFullPath(this->InfoFileNameFull);
  this->InfoFileStream =
    new cmGeneratedFileStream(this->InfoFileNameFull.c_str());
  this->InfoFileStream->SetCopyIfDifferent(true);
  if (!*this->InfoFileStream) {
    return;
  }
  this->LocalGenerator->WriteDependLanguageInfo(*this->InfoFileStream,
                                                this->GeneratorTarget);

  // Store multiple output pairs in the depend info file.
  if (!this->MultipleOutputPairs.empty()) {
    /* clang-format off */
    *this->InfoFileStream
      << "\n"
      << "# Pairs of files generated by the same build rule.\n"
      << "set(CMAKE_MULTIPLE_OUTPUT_PAIRS\n";
    /* clang-format on */
    for (MultipleOutputPairsType::const_iterator pi =
           this->MultipleOutputPairs.begin();
         pi != this->MultipleOutputPairs.end(); ++pi) {
      *this->InfoFileStream
        << "  " << cmOutputConverter::EscapeForCMake(pi->first) << " "
        << cmOutputConverter::EscapeForCMake(pi->second) << "\n";
    }
    *this->InfoFileStream << "  )\n\n";
  }

  // Store list of targets linked directly or transitively.
  {
    /* clang-format off */
  *this->InfoFileStream
    << "\n"
    << "# Targets to which this target links.\n"
    << "set(CMAKE_TARGET_LINKED_INFO_FILES\n";
    /* clang-format on */
    std::vector<std::string> dirs = this->GetLinkedTargetDirectories();
    for (std::vector<std::string>::iterator i = dirs.begin(); i != dirs.end();
         ++i) {
      *this->InfoFileStream << "  \"" << *i << "/DependInfo.cmake\"\n";
    }
    *this->InfoFileStream << "  )\n";
  }

  std::string const& working_dir =
    this->LocalGenerator->GetCurrentBinaryDirectory();

  /* clang-format off */
  *this->InfoFileStream
    << "\n"
    << "# Fortran module output directory.\n"
    << "set(CMAKE_Fortran_TARGET_MODULE_DIR \""
    << this->GeneratorTarget->GetFortranModuleDirectory(working_dir)
    << "\")\n";
  /* clang-format on */

  // and now write the rule to use it
  std::vector<std::string> depends;
  std::vector<std::string> commands;

  // Construct the name of the dependency generation target.
  std::string depTarget =
    this->LocalGenerator->GetRelativeTargetDirectory(this->GeneratorTarget);
  depTarget += "/depend";

  // Add a command to call CMake to scan dependencies.  CMake will
  // touch the corresponding depends file after scanning dependencies.
  std::ostringstream depCmd;
// TODO: Account for source file properties and directory-level
// definitions when scanning for dependencies.
#if !defined(_WIN32) || defined(__CYGWIN__)
  // This platform supports symlinks, so cmSystemTools will translate
  // paths.  Make sure PWD is set to the original name of the home
  // output directory to help cmSystemTools to create the same
  // translation table for the dependency scanning process.
  depCmd << "cd " << (this->LocalGenerator->ConvertToOutputFormat(
                       cmSystemTools::CollapseFullPath(
                         this->LocalGenerator->GetBinaryDirectory()),
                       cmOutputConverter::SHELL))
         << " && ";
#endif
  // Generate a call this signature:
  //
  //   cmake -E cmake_depends <generator>
  //                          <home-src-dir> <start-src-dir>
  //                          <home-out-dir> <start-out-dir>
  //                          <dep-info> --color=$(COLOR)
  //
  // This gives the dependency scanner enough information to recreate
  // the state of our local generator sufficiently for its needs.
  depCmd << "$(CMAKE_COMMAND) -E cmake_depends \""
         << this->GlobalGenerator->GetName() << "\" "
         << this->LocalGenerator->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(
                this->LocalGenerator->GetSourceDirectory()),
              cmOutputConverter::SHELL)
         << " "
         << this->LocalGenerator->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(
                this->LocalGenerator->GetCurrentSourceDirectory()),
              cmOutputConverter::SHELL)
         << " "
         << this->LocalGenerator->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(
                this->LocalGenerator->GetBinaryDirectory()),
              cmOutputConverter::SHELL)
         << " "
         << this->LocalGenerator->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(
                this->LocalGenerator->GetCurrentBinaryDirectory()),
              cmOutputConverter::SHELL)
         << " "
         << this->LocalGenerator->ConvertToOutputFormat(
              cmSystemTools::CollapseFullPath(this->InfoFileNameFull),
              cmOutputConverter::SHELL);
  if (this->LocalGenerator->GetColorMakefile()) {
    depCmd << " --color=$(COLOR)";
  }
  commands.push_back(depCmd.str());

  // Make sure all custom command outputs in this target are built.
  if (this->CustomCommandDriver == OnDepends) {
    this->DriveCustomCommands(depends);
  }

  // Write the rule.
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      depTarget, depends, commands, true);
}

void cmMakefileTargetGenerator::DriveCustomCommands(
  std::vector<std::string>& depends)
{
  // Depend on all custom command outputs.
  std::vector<cmSourceFile*> sources;
  this->GeneratorTarget->GetSourceFiles(
    sources, this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
  for (std::vector<cmSourceFile*>::const_iterator source = sources.begin();
       source != sources.end(); ++source) {
    if (cmCustomCommand* cc = (*source)->GetCustomCommand()) {
      cmCustomCommandGenerator ccg(*cc, this->ConfigName,
                                   this->LocalGenerator);
      const std::vector<std::string>& outputs = ccg.GetOutputs();
      depends.insert(depends.end(), outputs.begin(), outputs.end());
    }
  }
}

void cmMakefileTargetGenerator::WriteObjectDependRules(
  cmSourceFile const& source, std::vector<std::string>& depends)
{
  // Create the list of dependencies known at cmake time.  These are
  // shared between the object file and dependency scanning rule.
  depends.push_back(source.GetFullPath());
  if (const char* objectDeps = source.GetProperty("OBJECT_DEPENDS")) {
    cmSystemTools::ExpandListArgument(objectDeps, depends);
  }
}

void cmMakefileTargetGenerator::GenerateCustomRuleFile(
  cmCustomCommandGenerator const& ccg)
{
  // Collect the commands.
  std::vector<std::string> commands;
  std::string comment = this->LocalGenerator->ConstructComment(ccg);
  if (!comment.empty()) {
    // add in a progress call if needed
    this->NumberOfProgressActions++;
    if (!this->NoRuleMessages) {
      cmLocalUnixMakefileGenerator3::EchoProgress progress;
      this->MakeEchoProgress(progress);
      this->LocalGenerator->AppendEcho(
        commands, comment, cmLocalUnixMakefileGenerator3::EchoGenerate,
        &progress);
    }
  }

  // Now append the actual user-specified commands.
  std::ostringstream content;
  this->LocalGenerator->AppendCustomCommand(
    commands, ccg, this->GeneratorTarget,
    this->LocalGenerator->GetBinaryDirectory(), false, &content);

  // Collect the dependencies.
  std::vector<std::string> depends;
  this->LocalGenerator->AppendCustomDepend(depends, ccg);

  // Write the rule.
  const std::vector<std::string>& outputs = ccg.GetOutputs();
  bool symbolic = this->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      outputs, depends, commands);

  // If the rule has changed make sure the output is rebuilt.
  if (!symbolic) {
    this->GlobalGenerator->AddRuleHash(ccg.GetOutputs(), content.str());
  }

  // Setup implicit dependency scanning.
  for (cmCustomCommand::ImplicitDependsList::const_iterator idi =
         ccg.GetCC().GetImplicitDepends().begin();
       idi != ccg.GetCC().GetImplicitDepends().end(); ++idi) {
    std::string objFullPath = cmSystemTools::CollapseFullPath(outputs[0]);
    std::string srcFullPath = cmSystemTools::CollapseFullPath(idi->second);
    this->LocalGenerator->AddImplicitDepends(this->GeneratorTarget, idi->first,
                                             objFullPath.c_str(),
                                             srcFullPath.c_str());
  }
}

void cmMakefileTargetGenerator::MakeEchoProgress(
  cmLocalUnixMakefileGenerator3::EchoProgress& progress) const
{
  progress.Dir = this->LocalGenerator->GetBinaryDirectory();
  progress.Dir += cmake::GetCMakeFilesDirectory();
  std::ostringstream progressArg;
  progressArg << "$(CMAKE_PROGRESS_" << this->NumberOfProgressActions << ")";
  progress.Arg = progressArg.str();
}

void cmMakefileTargetGenerator::WriteObjectsVariable(
  std::string& variableName, std::string& variableNameExternal,
  bool useWatcomQuote)
{
  // Write a make variable assignment that lists all objects for the
  // target.
  variableName = this->LocalGenerator->CreateMakeVariable(
    this->GeneratorTarget->GetName(), "_OBJECTS");
  *this->BuildFileStream << "# Object files for target "
                         << this->GeneratorTarget->GetName() << "\n"
                         << variableName << " =";
  std::string object;
  const char* lineContinue =
    this->Makefile->GetDefinition("CMAKE_MAKE_LINE_CONTINUE");
  if (!lineContinue) {
    lineContinue = "\\";
  }
  for (std::vector<std::string>::const_iterator i = this->Objects.begin();
       i != this->Objects.end(); ++i) {
    *this->BuildFileStream << " " << lineContinue << "\n";
    *this->BuildFileStream << this->LocalGenerator->ConvertToQuotedOutputPath(
      i->c_str(), useWatcomQuote);
  }
  *this->BuildFileStream << "\n";

  // Write a make variable assignment that lists all external objects
  // for the target.
  variableNameExternal = this->LocalGenerator->CreateMakeVariable(
    this->GeneratorTarget->GetName(), "_EXTERNAL_OBJECTS");
  /* clang-format off */
  *this->BuildFileStream
    << "\n"
    << "# External object files for target "
    << this->GeneratorTarget->GetName() << "\n"
    << variableNameExternal << " =";
  /* clang-format on */
  std::string currentBinDir =
    this->LocalGenerator->GetCurrentBinaryDirectory();
  for (std::vector<std::string>::const_iterator i =
         this->ExternalObjects.begin();
       i != this->ExternalObjects.end(); ++i) {
    object =
      this->LocalGenerator->MaybeConvertToRelativePath(currentBinDir, *i);
    *this->BuildFileStream << " " << lineContinue << "\n";
    *this->BuildFileStream << this->LocalGenerator->ConvertToQuotedOutputPath(
      i->c_str(), useWatcomQuote);
  }
  *this->BuildFileStream << "\n"
                         << "\n";
}

class cmMakefileTargetGeneratorObjectStrings
{
public:
  cmMakefileTargetGeneratorObjectStrings(std::vector<std::string>& strings,
                                         cmOutputConverter* outputConverter,
                                         cmStateDirectory const& stateDir,
                                         std::string::size_type limit)
    : Strings(strings)
    , OutputConverter(outputConverter)
    , StateDir(stateDir)
    , LengthLimit(limit)
  {
    this->Space = "";
  }
  void Feed(std::string const& obj)
  {
    // Construct the name of the next object.
    this->NextObject = this->OutputConverter->ConvertToOutputFormat(
      this->MaybeConvertToRelativePath(obj), cmOutputConverter::RESPONSE);

    // Roll over to next string if the limit will be exceeded.
    if (this->LengthLimit != std::string::npos &&
        (this->CurrentString.length() + 1 + this->NextObject.length() >
         this->LengthLimit)) {
      this->Strings.push_back(this->CurrentString);
      this->CurrentString = "";
      this->Space = "";
    }

    // Separate from previous object.
    this->CurrentString += this->Space;
    this->Space = " ";

    // Append this object.
    this->CurrentString += this->NextObject;
  }
  void Done() { this->Strings.push_back(this->CurrentString); }

private:
  std::string MaybeConvertToRelativePath(std::string const& obj)
  {
    if (!cmOutputConverter::ContainedInDirectory(
          this->StateDir.GetCurrentBinary(), obj, this->StateDir)) {
      return obj;
    }
    return cmOutputConverter::ForceToRelativePath(
      this->StateDir.GetCurrentBinary(), obj);
  }

  std::vector<std::string>& Strings;
  cmOutputConverter* OutputConverter;
  cmStateDirectory StateDir;
  std::string::size_type LengthLimit;
  std::string CurrentString;
  std::string NextObject;
  const char* Space;
};

void cmMakefileTargetGenerator::WriteObjectsStrings(
  std::vector<std::string>& objStrings, std::string::size_type limit)
{
  cmMakefileTargetGeneratorObjectStrings helper(
    objStrings, this->LocalGenerator,
    this->LocalGenerator->GetStateSnapshot().GetDirectory(), limit);
  for (std::vector<std::string>::const_iterator i = this->Objects.begin();
       i != this->Objects.end(); ++i) {
    helper.Feed(*i);
  }
  for (std::vector<std::string>::const_iterator i =
         this->ExternalObjects.begin();
       i != this->ExternalObjects.end(); ++i) {
    helper.Feed(*i);
  }
  helper.Done();
}

void cmMakefileTargetGenerator::WriteTargetDriverRule(
  const std::string& main_output, bool relink)
{
  // Compute the name of the driver target.
  std::string dir =
    this->LocalGenerator->GetRelativeTargetDirectory(this->GeneratorTarget);
  std::string buildTargetRuleName = dir;
  buildTargetRuleName += relink ? "/preinstall" : "/build";
  buildTargetRuleName = this->LocalGenerator->MaybeConvertToRelativePath(
    this->LocalGenerator->GetBinaryDirectory(), buildTargetRuleName);

  // Build the list of target outputs to drive.
  std::vector<std::string> depends;
  depends.push_back(main_output);

  const char* comment = CM_NULLPTR;
  if (relink) {
    // Setup the comment for the preinstall driver.
    comment = "Rule to relink during preinstall.";
  } else {
    // Setup the comment for the main build driver.
    comment = "Rule to build all files generated by this target.";

    // Make sure all custom command outputs in this target are built.
    if (this->CustomCommandDriver == OnBuild) {
      this->DriveCustomCommands(depends);
    }

    // Make sure the extra files are built.
    depends.insert(depends.end(), this->ExtraFiles.begin(),
                   this->ExtraFiles.end());
  }

  // Write the driver rule.
  std::vector<std::string> no_commands;
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, comment,
                                      buildTargetRuleName, depends,
                                      no_commands, true);
}

void cmMakefileTargetGenerator::AppendTargetDepends(
  std::vector<std::string>& depends)
{
  // Static libraries never depend on anything for linking.
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
    return;
  }

  // Loop over all library dependencies.
  const char* cfg = this->LocalGenerator->GetConfigName().c_str();
  if (cmComputeLinkInformation* cli =
        this->GeneratorTarget->GetLinkInformation(cfg)) {
    std::vector<std::string> const& libDeps = cli->GetDepends();
    depends.insert(depends.end(), libDeps.begin(), libDeps.end());
  }
}

void cmMakefileTargetGenerator::AppendObjectDepends(
  std::vector<std::string>& depends)
{
  // Add dependencies on the compiled object files.
  std::string relPath = this->LocalGenerator->GetHomeRelativeOutputPath();
  std::string objTarget;
  for (std::vector<std::string>::const_iterator obj = this->Objects.begin();
       obj != this->Objects.end(); ++obj) {
    objTarget = relPath;
    objTarget += *obj;
    depends.push_back(objTarget);
  }

  // Add dependencies on the external object files.
  depends.insert(depends.end(), this->ExternalObjects.begin(),
                 this->ExternalObjects.end());

  // Add a dependency on the rule file itself.
  this->LocalGenerator->AppendRuleDepend(depends,
                                         this->BuildFileNameFull.c_str());
}

void cmMakefileTargetGenerator::AppendLinkDepends(
  std::vector<std::string>& depends)
{
  this->AppendObjectDepends(depends);

  // Add dependencies on targets that must be built first.
  this->AppendTargetDepends(depends);

  // Add a dependency on the link definitions file, if any.
  if (cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
        this->GeneratorTarget->GetModuleDefinitionInfo(
          this->GetConfigName())) {
    for (std::vector<cmSourceFile const*>::const_iterator i =
           mdi->Sources.begin();
         i != mdi->Sources.end(); ++i) {
      depends.push_back((*i)->GetFullPath());
    }
  }

  // Add a dependency on user-specified manifest files, if any.
  std::vector<cmSourceFile const*> manifest_srcs;
  this->GeneratorTarget->GetManifests(manifest_srcs, this->ConfigName);
  for (std::vector<cmSourceFile const*>::iterator mi = manifest_srcs.begin();
       mi != manifest_srcs.end(); ++mi) {
    depends.push_back((*mi)->GetFullPath());
  }

  // Add user-specified dependencies.
  if (const char* linkDepends =
        this->GeneratorTarget->GetProperty("LINK_DEPENDS")) {
    cmSystemTools::ExpandListArgument(linkDepends, depends);
  }
}

std::string cmMakefileTargetGenerator::GetLinkRule(
  const std::string& linkRuleVar)
{
  std::string linkRule = this->Makefile->GetRequiredDefinition(linkRuleVar);
  if (this->GeneratorTarget->HasImplibGNUtoMS()) {
    std::string ruleVar = "CMAKE_";
    ruleVar += this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
    ruleVar += "_GNUtoMS_RULE";
    if (const char* rule = this->Makefile->GetDefinition(ruleVar)) {
      linkRule += rule;
    }
  }
  return linkRule;
}

void cmMakefileTargetGenerator::CloseFileStreams()
{
  delete this->BuildFileStream;
  delete this->InfoFileStream;
  delete this->FlagFileStream;
}

void cmMakefileTargetGenerator::CreateLinkScript(
  const char* name, std::vector<std::string> const& link_commands,
  std::vector<std::string>& makefile_commands,
  std::vector<std::string>& makefile_depends)
{
  // Create the link script file.
  std::string linkScriptName = this->TargetBuildDirectoryFull;
  linkScriptName += "/";
  linkScriptName += name;
  cmGeneratedFileStream linkScriptStream(linkScriptName.c_str());
  linkScriptStream.SetCopyIfDifferent(true);
  for (std::vector<std::string>::const_iterator cmd = link_commands.begin();
       cmd != link_commands.end(); ++cmd) {
    // Do not write out empty commands or commands beginning in the
    // shell no-op ":".
    if (!cmd->empty() && (*cmd)[0] != ':') {
      linkScriptStream << *cmd << "\n";
    }
  }

  // Create the makefile command to invoke the link script.
  std::string link_command = "$(CMAKE_COMMAND) -E cmake_link_script ";
  link_command += this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), linkScriptName),
    cmOutputConverter::SHELL);
  link_command += " --verbose=$(VERBOSE)";
  makefile_commands.push_back(link_command);
  makefile_depends.push_back(linkScriptName);
}

bool cmMakefileTargetGenerator::CheckUseResponseFileForObjects(
  std::string const& l) const
{
  // Check for an explicit setting one way or the other.
  std::string const responseVar =
    "CMAKE_" + l + "_USE_RESPONSE_FILE_FOR_OBJECTS";
  if (const char* val = this->Makefile->GetDefinition(responseVar)) {
    if (*val) {
      return cmSystemTools::IsOn(val);
    }
  }

  // Check for a system limit.
  if (size_t const limit = cmSystemTools::CalculateCommandLineLengthLimit()) {
    // Compute the total length of our list of object files with room
    // for argument separation and quoting.  This does not convert paths
    // relative to CMAKE_CURRENT_BINARY_DIR like the final list will be, so the
    // actual list will likely be much shorter than this.  However, in the
    // worst case all objects will remain as absolute paths.
    size_t length = 0;
    for (std::vector<std::string>::const_iterator i = this->Objects.begin();
         i != this->Objects.end(); ++i) {
      length += i->size() + 3;
    }
    for (std::vector<std::string>::const_iterator i =
           this->ExternalObjects.begin();
         i != this->ExternalObjects.end(); ++i) {
      length += i->size() + 3;
    }

    // We need to guarantee room for both objects and libraries, so
    // if the objects take up more than half then use a response file
    // for them.
    if (length > (limit / 2)) {
      return true;
    }
  }

  // We do not need a response file for objects.
  return false;
}

bool cmMakefileTargetGenerator::CheckUseResponseFileForLibraries(
  std::string const& l) const
{
  // Check for an explicit setting one way or the other.
  std::string const responseVar =
    "CMAKE_" + l + "_USE_RESPONSE_FILE_FOR_LIBRARIES";
  if (const char* val = this->Makefile->GetDefinition(responseVar)) {
    if (*val) {
      return cmSystemTools::IsOn(val);
    }
  }

  // We do not need a response file for libraries.
  return false;
}

std::string cmMakefileTargetGenerator::CreateResponseFile(
  const char* name, std::string const& options,
  std::vector<std::string>& makefile_depends)
{
  // Create the response file.
  std::string responseFileNameFull = this->TargetBuildDirectoryFull;
  responseFileNameFull += "/";
  responseFileNameFull += name;
  cmGeneratedFileStream responseStream(responseFileNameFull.c_str());
  responseStream.SetCopyIfDifferent(true);
  responseStream << options << "\n";

  // Add a dependency so the target will rebuild when the set of
  // objects changes.
  makefile_depends.push_back(responseFileNameFull);

  // Construct the name to be used on the command line.
  std::string responseFileName = this->TargetBuildDirectory;
  responseFileName += "/";
  responseFileName += name;
  return responseFileName;
}

cmLinkLineComputer* cmMakefileTargetGenerator::CreateLinkLineComputer(
  cmOutputConverter* outputConverter, cmStateDirectory const& stateDir)
{
  if (this->Makefile->IsOn("MSVC60")) {
    return this->GlobalGenerator->CreateMSVC60LinkLineComputer(outputConverter,
                                                               stateDir);
  }
  return this->GlobalGenerator->CreateLinkLineComputer(outputConverter,
                                                       stateDir);
}

void cmMakefileTargetGenerator::CreateLinkLibs(
  cmLinkLineComputer* linkLineComputer, std::string& linkLibs,
  bool useResponseFile, std::vector<std::string>& makefile_depends)
{
  std::string frameworkPath;
  std::string linkPath;
  std::string config = this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
  cmComputeLinkInformation* pcli =
    this->GeneratorTarget->GetLinkInformation(config);
  this->LocalGenerator->OutputLinkLibraries(pcli, linkLineComputer, linkLibs,
                                            frameworkPath, linkPath);
  linkLibs = frameworkPath + linkPath + linkLibs;

  if (useResponseFile &&
      linkLibs.find_first_not_of(' ') != std::string::npos) {
    // Lookup the response file reference flag.
    std::string responseFlagVar = "CMAKE_";
    responseFlagVar +=
      this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
    responseFlagVar += "_RESPONSE_FILE_LINK_FLAG";
    const char* responseFlag = this->Makefile->GetDefinition(responseFlagVar);
    if (!responseFlag) {
      responseFlag = "@";
    }

    // Create this response file.
    std::string link_rsp =
      this->CreateResponseFile("linklibs.rsp", linkLibs, makefile_depends);

    // Reference the response file.
    linkLibs = responseFlag;
    linkLibs += this->LocalGenerator->ConvertToOutputFormat(
      link_rsp, cmOutputConverter::SHELL);
  }
}

void cmMakefileTargetGenerator::CreateObjectLists(
  bool useLinkScript, bool useArchiveRules, bool useResponseFile,
  std::string& buildObjs, std::vector<std::string>& makefile_depends,
  bool useWatcomQuote)
{
  std::string variableName;
  std::string variableNameExternal;
  this->WriteObjectsVariable(variableName, variableNameExternal,
                             useWatcomQuote);
  if (useResponseFile) {
    // MSVC response files cannot exceed 128K.
    std::string::size_type const responseFileLimit = 131000;

    // Construct the individual object list strings.
    std::vector<std::string> object_strings;
    this->WriteObjectsStrings(object_strings, responseFileLimit);

    // Lookup the response file reference flag.
    std::string responseFlagVar = "CMAKE_";
    responseFlagVar +=
      this->GeneratorTarget->GetLinkerLanguage(this->ConfigName);
    responseFlagVar += "_RESPONSE_FILE_LINK_FLAG";
    const char* responseFlag = this->Makefile->GetDefinition(responseFlagVar);
    if (!responseFlag) {
      responseFlag = "@";
    }

    // Write a response file for each string.
    const char* sep = "";
    for (unsigned int i = 0; i < object_strings.size(); ++i) {
      // Number the response files.
      char rsp[32];
      sprintf(rsp, "objects%u.rsp", i + 1);

      // Create this response file.
      std::string objects_rsp =
        this->CreateResponseFile(rsp, object_strings[i], makefile_depends);

      // Separate from previous response file references.
      buildObjs += sep;
      sep = " ";

      // Reference the response file.
      buildObjs += responseFlag;
      buildObjs += this->LocalGenerator->ConvertToOutputFormat(
        objects_rsp, cmOutputConverter::SHELL);
    }
  } else if (useLinkScript) {
    if (!useArchiveRules) {
      std::vector<std::string> objStrings;
      this->WriteObjectsStrings(objStrings);
      buildObjs = objStrings[0];
    }
  } else {
    buildObjs = "$(";
    buildObjs += variableName;
    buildObjs += ") $(";
    buildObjs += variableNameExternal;
    buildObjs += ")";
  }
}

void cmMakefileTargetGenerator::AddIncludeFlags(std::string& flags,
                                                const std::string& lang)
{
  std::string responseVar = "CMAKE_";
  responseVar += lang;
  responseVar += "_USE_RESPONSE_FILE_FOR_INCLUDES";
  bool useResponseFile = this->Makefile->IsOn(responseVar);

  std::vector<std::string> includes;
  const std::string& config =
    this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              lang, config);

  std::string includeFlags = this->LocalGenerator->GetIncludeFlags(
    includes, this->GeneratorTarget, lang, false, useResponseFile, config);
  if (includeFlags.empty()) {
    return;
  }

  if (useResponseFile) {
    std::string name = "includes_";
    name += lang;
    name += ".rsp";
    std::string arg = "@" +
      this->CreateResponseFile(name.c_str(), includeFlags,
                               this->FlagFileDepends[lang]);
    this->LocalGenerator->AppendFlags(flags, arg);
  } else {
    this->LocalGenerator->AppendFlags(flags, includeFlags);
  }
}

void cmMakefileTargetGenerator::GenDefFile(
  std::vector<std::string>& real_link_commands)
{
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    this->GeneratorTarget->GetModuleDefinitionInfo(this->GetConfigName());
  if (!mdi || !mdi->DefFileGenerated) {
    return;
  }
  std::string cmd = cmSystemTools::GetCMakeCommand();
  cmd =
    this->LocalGenerator->ConvertToOutputFormat(cmd, cmOutputConverter::SHELL);
  cmd += " -E __create_def ";
  cmd += this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), mdi->DefFile),
    cmOutputConverter::SHELL);
  cmd += " ";
  std::string objlist_file = mdi->DefFile + ".objs";
  cmd += this->LocalGenerator->ConvertToOutputFormat(
    this->LocalGenerator->MaybeConvertToRelativePath(
      this->LocalGenerator->GetCurrentBinaryDirectory(), objlist_file),
    cmOutputConverter::SHELL);
  real_link_commands.insert(real_link_commands.begin(), cmd);
  // create a list of obj files for the -E __create_def to read
  cmGeneratedFileStream fout(objlist_file.c_str());

  if (mdi->WindowsExportAllSymbols) {
    for (std::vector<std::string>::const_iterator i = this->Objects.begin();
         i != this->Objects.end(); ++i) {
      if (cmHasLiteralSuffix(*i, ".obj")) {
        fout << *i << "\n";
      }
    }
    for (std::vector<std::string>::const_iterator i =
           this->ExternalObjects.begin();
         i != this->ExternalObjects.end(); ++i) {
      fout << *i << "\n";
    }
  }

  for (std::vector<cmSourceFile const*>::const_iterator i =
         mdi->Sources.begin();
       i != mdi->Sources.end(); ++i) {
    fout << (*i)->GetFullPath() << "\n";
  }
}
