/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmNinjaTargetGenerator.h"

#include "cm_jsoncpp_value.h"
#include "cm_jsoncpp_writer.h"
#include <algorithm>
#include <assert.h>
#include <iterator>
#include <map>
#include <sstream>
#include <string.h>

#include "cmAlgorithms.h"
#include "cmComputeLinkInformation.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalNinjaGenerator.h"
#include "cmLocalGenerator.h"
#include "cmLocalNinjaGenerator.h"
#include "cmMakefile.h"
#include "cmNinjaNormalTargetGenerator.h"
#include "cmNinjaUtilityTargetGenerator.h"
#include "cmOutputConverter.h"
#include "cmRulePlaceholderExpander.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmNinjaTargetGenerator* cmNinjaTargetGenerator::New(cmGeneratorTarget* target)
{
  switch (target->GetType()) {
    case cmStateEnums::EXECUTABLE:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
    case cmStateEnums::OBJECT_LIBRARY:
      return new cmNinjaNormalTargetGenerator(target);

    case cmStateEnums::UTILITY:
    case cmStateEnums::GLOBAL_TARGET:
      return new cmNinjaUtilityTargetGenerator(target);

    default:
      return CM_NULLPTR;
  }
}

cmNinjaTargetGenerator::cmNinjaTargetGenerator(cmGeneratorTarget* target)
  : cmCommonTargetGenerator(target)
  , MacOSXContentGenerator(CM_NULLPTR)
  , OSXBundleGenerator(CM_NULLPTR)
  , MacContentFolders()
  , LocalGenerator(
      static_cast<cmLocalNinjaGenerator*>(target->GetLocalGenerator()))
  , Objects()
{
  MacOSXContentGenerator = new MacOSXContentGeneratorType(this);
}

cmNinjaTargetGenerator::~cmNinjaTargetGenerator()
{
  delete this->MacOSXContentGenerator;
}

cmGeneratedFileStream& cmNinjaTargetGenerator::GetBuildFileStream() const
{
  return *this->GetGlobalGenerator()->GetBuildFileStream();
}

cmGeneratedFileStream& cmNinjaTargetGenerator::GetRulesFileStream() const
{
  return *this->GetGlobalGenerator()->GetRulesFileStream();
}

cmGlobalNinjaGenerator* cmNinjaTargetGenerator::GetGlobalGenerator() const
{
  return this->LocalGenerator->GetGlobalNinjaGenerator();
}

std::string cmNinjaTargetGenerator::LanguageCompilerRule(
  const std::string& lang) const
{
  return lang + "_COMPILER__" +
    cmGlobalNinjaGenerator::EncodeRuleName(this->GeneratorTarget->GetName());
}

std::string cmNinjaTargetGenerator::LanguagePreprocessRule(
  std::string const& lang) const
{
  return lang + "_PREPROCESS__" +
    cmGlobalNinjaGenerator::EncodeRuleName(this->GeneratorTarget->GetName());
}

bool cmNinjaTargetGenerator::NeedExplicitPreprocessing(
  std::string const& lang) const
{
  return lang == "Fortran";
}

std::string cmNinjaTargetGenerator::LanguageDyndepRule(
  const std::string& lang) const
{
  return lang + "_DYNDEP__" +
    cmGlobalNinjaGenerator::EncodeRuleName(this->GeneratorTarget->GetName());
}

bool cmNinjaTargetGenerator::NeedDyndep(std::string const& lang) const
{
  return lang == "Fortran";
}

std::string cmNinjaTargetGenerator::OrderDependsTargetForTarget()
{
  return "cmake_object_order_depends_target_" + this->GetTargetName();
}

// TODO: Most of the code is picked up from
// void cmMakefileExecutableTargetGenerator::WriteExecutableRule(bool relink),
// void cmMakefileTargetGenerator::WriteTargetLanguageFlags()
// Refactor it.
std::string cmNinjaTargetGenerator::ComputeFlagsForObject(
  cmSourceFile const* source, const std::string& language)
{
  std::string flags = this->GetFlags(language);

  // Add Fortran format flags.
  if (language == "Fortran") {
    this->AppendFortranFormatFlags(flags, *source);
  }

  // Add source file specific flags.
  if (const char* cflags = source->GetProperty("COMPILE_FLAGS")) {
    std::string config = this->LocalGenerator->GetConfigName();
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(cflags);
    const char* evaluatedFlags = cge->Evaluate(this->LocalGenerator, config,
                                               false, this->GeneratorTarget);
    this->LocalGenerator->AppendFlags(flags, evaluatedFlags);
  }

  return flags;
}

void cmNinjaTargetGenerator::AddIncludeFlags(std::string& languageFlags,
                                             std::string const& language)
{
  std::vector<std::string> includes;
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              language, this->GetConfigName());
  // Add include directory flags.
  std::string includeFlags = this->LocalGenerator->GetIncludeFlags(
    includes, this->GeneratorTarget, language,
    language == "RC", // full include paths for RC needed by cmcldeps
    false, this->GetConfigName());
  if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
    std::replace(includeFlags.begin(), includeFlags.end(), '\\', '/');
  }

  this->LocalGenerator->AppendFlags(languageFlags, includeFlags);
}

bool cmNinjaTargetGenerator::NeedDepTypeMSVC(const std::string& lang) const
{
  return strcmp(this->GetMakefile()->GetSafeDefinition("CMAKE_NINJA_DEPTYPE_" +
                                                       lang),
                "msvc") == 0;
}

// TODO: Refactor with
// void cmMakefileTargetGenerator::WriteTargetLanguageFlags().
std::string cmNinjaTargetGenerator::ComputeDefines(cmSourceFile const* source,
                                                   const std::string& language)
{
  std::set<std::string> defines;
  this->LocalGenerator->AppendDefines(
    defines, source->GetProperty("COMPILE_DEFINITIONS"));
  {
    std::string defPropName = "COMPILE_DEFINITIONS_";
    defPropName += cmSystemTools::UpperCase(this->GetConfigName());
    this->LocalGenerator->AppendDefines(defines,
                                        source->GetProperty(defPropName));
  }

  std::string definesString = this->GetDefines(language);
  this->LocalGenerator->JoinDefines(defines, definesString, language);

  return definesString;
}

cmNinjaDeps cmNinjaTargetGenerator::ComputeLinkDeps() const
{
  // Static libraries never depend on other targets for linking.
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY ||
      this->GeneratorTarget->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    return cmNinjaDeps();
  }

  cmComputeLinkInformation* cli =
    this->GeneratorTarget->GetLinkInformation(this->GetConfigName());
  if (!cli) {
    return cmNinjaDeps();
  }

  const std::vector<std::string>& deps = cli->GetDepends();
  cmNinjaDeps result(deps.size());
  std::transform(deps.begin(), deps.end(), result.begin(), MapToNinjaPath());

  // Add a dependency on the link definitions file, if any.
  if (cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
        this->GeneratorTarget->GetModuleDefinitionInfo(
          this->GetConfigName())) {
    for (std::vector<cmSourceFile const*>::const_iterator i =
           mdi->Sources.begin();
         i != mdi->Sources.end(); ++i) {
      result.push_back(this->ConvertToNinjaPath((*i)->GetFullPath()));
    }
  }

  // Add a dependency on user-specified manifest files, if any.
  std::vector<cmSourceFile const*> manifest_srcs;
  this->GeneratorTarget->GetManifests(manifest_srcs, this->ConfigName);
  for (std::vector<cmSourceFile const*>::iterator mi = manifest_srcs.begin();
       mi != manifest_srcs.end(); ++mi) {
    result.push_back(this->ConvertToNinjaPath((*mi)->GetFullPath()));
  }

  // Add user-specified dependencies.
  if (const char* linkDepends =
        this->GeneratorTarget->GetProperty("LINK_DEPENDS")) {
    std::vector<std::string> linkDeps;
    cmSystemTools::ExpandListArgument(linkDepends, linkDeps);
    std::transform(linkDeps.begin(), linkDeps.end(),
                   std::back_inserter(result), MapToNinjaPath());
  }

  return result;
}

std::string cmNinjaTargetGenerator::GetSourceFilePath(
  cmSourceFile const* source) const
{
  return ConvertToNinjaPath(source->GetFullPath());
}

std::string cmNinjaTargetGenerator::GetObjectFilePath(
  cmSourceFile const* source) const
{
  std::string path = this->LocalGenerator->GetHomeRelativeOutputPath();
  if (!path.empty()) {
    path += "/";
  }
  std::string const& objectName = this->GeneratorTarget->GetObjectName(source);
  path += this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  path += "/";
  path += objectName;
  return path;
}

std::string cmNinjaTargetGenerator::GetPreprocessedFilePath(
  cmSourceFile const* source) const
{
  // Choose an extension to compile already-preprocessed source.
  std::string ppExt = source->GetExtension();
  if (cmHasLiteralPrefix(ppExt, "F")) {
    // Some Fortran compilers automatically enable preprocessing for
    // upper-case extensions.  Since the source is already preprocessed,
    // use a lower-case extension.
    ppExt = cmSystemTools::LowerCase(ppExt);
  }
  if (ppExt == "fpp") {
    // Some Fortran compilers automatically enable preprocessing for
    // the ".fpp" extension.  Since the source is already preprocessed,
    // use the ".f" extension.
    ppExt = "f";
  }

  // Take the object file name and replace the extension.
  std::string const& objName = this->GeneratorTarget->GetObjectName(source);
  std::string const& objExt =
    this->GetGlobalGenerator()->GetLanguageOutputExtension(*source);
  assert(objName.size() >= objExt.size());
  std::string const ppName =
    objName.substr(0, objName.size() - objExt.size()) + "-pp." + ppExt;

  std::string path = this->LocalGenerator->GetHomeRelativeOutputPath();
  if (!path.empty()) {
    path += "/";
  }
  path += this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  path += "/";
  path += ppName;
  return path;
}

std::string cmNinjaTargetGenerator::GetDyndepFilePath(
  std::string const& lang) const
{
  std::string path = this->LocalGenerator->GetHomeRelativeOutputPath();
  if (!path.empty()) {
    path += "/";
  }
  path += this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  path += "/";
  path += lang;
  path += ".dd";
  return path;
}

std::string cmNinjaTargetGenerator::GetTargetDependInfoPath(
  std::string const& lang) const
{
  std::string path = this->Makefile->GetCurrentBinaryDirectory();
  path += "/";
  path += this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  path += "/" + lang + "DependInfo.json";
  return path;
}

std::string cmNinjaTargetGenerator::GetTargetOutputDir() const
{
  std::string dir = this->GeneratorTarget->GetDirectory(this->GetConfigName());
  return ConvertToNinjaPath(dir);
}

std::string cmNinjaTargetGenerator::GetTargetFilePath(
  const std::string& name) const
{
  std::string path = this->GetTargetOutputDir();
  if (path.empty() || path == ".") {
    return name;
  }
  path += "/";
  path += name;
  return path;
}

std::string cmNinjaTargetGenerator::GetTargetName() const
{
  return this->GeneratorTarget->GetName();
}

bool cmNinjaTargetGenerator::SetMsvcTargetPdbVariable(cmNinjaVars& vars) const
{
  cmMakefile* mf = this->GetMakefile();
  if (mf->GetDefinition("MSVC_C_ARCHITECTURE_ID") ||
      mf->GetDefinition("MSVC_CXX_ARCHITECTURE_ID") ||
      mf->GetDefinition("MSVC_CUDA_ARCHITECTURE_ID")) {
    std::string pdbPath;
    std::string compilePdbPath = this->ComputeTargetCompilePDB();
    if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE ||
        this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY ||
        this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
        this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
      pdbPath = this->GeneratorTarget->GetPDBDirectory(this->GetConfigName());
      pdbPath += "/";
      pdbPath += this->GeneratorTarget->GetPDBName(this->GetConfigName());
    }

    vars["TARGET_PDB"] = this->GetLocalGenerator()->ConvertToOutputFormat(
      ConvertToNinjaPath(pdbPath), cmOutputConverter::SHELL);
    vars["TARGET_COMPILE_PDB"] =
      this->GetLocalGenerator()->ConvertToOutputFormat(
        ConvertToNinjaPath(compilePdbPath), cmOutputConverter::SHELL);

    EnsureParentDirectoryExists(pdbPath);
    EnsureParentDirectoryExists(compilePdbPath);
    return true;
  }
  return false;
}

void cmNinjaTargetGenerator::WriteLanguageRules(const std::string& language)
{
#ifdef NINJA_GEN_VERBOSE_FILES
  this->GetRulesFileStream() << "# Rules for language " << language << "\n\n";
#endif
  this->WriteCompileRule(language);
}

void cmNinjaTargetGenerator::WriteCompileRule(const std::string& lang)
{
  cmRulePlaceholderExpander::RuleVariables vars;
  vars.CMTargetName = this->GetGeneratorTarget()->GetName().c_str();
  vars.CMTargetType =
    cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType());
  vars.Language = lang.c_str();
  vars.Source = "$in";
  vars.Object = "$out";
  vars.Defines = "$DEFINES";
  vars.Includes = "$INCLUDES";
  vars.TargetPDB = "$TARGET_PDB";
  vars.TargetCompilePDB = "$TARGET_COMPILE_PDB";
  vars.ObjectDir = "$OBJECT_DIR";
  vars.ObjectFileDir = "$OBJECT_FILE_DIR";

  // For some cases we do an explicit preprocessor invocation.
  bool const explicitPP = this->NeedExplicitPreprocessing(lang);
  bool const needDyndep = this->NeedDyndep(lang);

  cmMakefile* mf = this->GetMakefile();

  std::string flags = "$FLAGS";
  std::string rspfile;
  std::string rspcontent;
  std::string responseFlag;

  bool const lang_supports_response = !(lang == "RC" || lang == "CUDA");
  if (lang_supports_response && this->ForceResponseFile()) {
    rspfile = "$RSP_FILE";
    responseFlag = "@" + rspfile;
    rspcontent = " $DEFINES $INCLUDES $FLAGS";
    flags = responseFlag;
    vars.Defines = "";
    vars.Includes = "";
  }

  // Tell ninja dependency format so all deps can be loaded into a database
  std::string deptype;
  std::string depfile;
  std::string cldeps;
  if (explicitPP) {
    // The explicit preprocessing step will handle dependency scanning.
  } else if (this->NeedDepTypeMSVC(lang)) {
    deptype = "msvc";
    depfile = "";
    flags += " /showIncludes";
  } else if (mf->IsOn("CMAKE_NINJA_CMCLDEPS_" + lang)) {
    // For the MS resource compiler we need cmcldeps, but skip dependencies
    // for source-file try_compile cases because they are always fresh.
    if (!mf->GetIsSourceFileTryCompile()) {
      deptype = "gcc";
      depfile = "$DEP_FILE";
      const std::string cl = mf->GetDefinition("CMAKE_C_COMPILER")
        ? mf->GetSafeDefinition("CMAKE_C_COMPILER")
        : mf->GetSafeDefinition("CMAKE_CXX_COMPILER");
      cldeps = "\"";
      cldeps += cmSystemTools::GetCMClDepsCommand();
      cldeps += "\" " + lang + " " + vars.Source + " \"$DEP_FILE\" $out \"";
      cldeps += mf->GetSafeDefinition("CMAKE_CL_SHOWINCLUDES_PREFIX");
      cldeps += "\" \"" + cl + "\" ";
    }
  } else {
    deptype = "gcc";
    const char* langdeptype = mf->GetDefinition("CMAKE_NINJA_DEPTYPE_" + lang);
    if (langdeptype) {
      deptype = langdeptype;
    }
    depfile = "$DEP_FILE";
    const std::string flagsName = "CMAKE_DEPFILE_FLAGS_" + lang;
    std::string depfileFlags = mf->GetSafeDefinition(flagsName);
    if (!depfileFlags.empty()) {
      cmSystemTools::ReplaceString(depfileFlags, "<DEPFILE>", "$DEP_FILE");
      cmSystemTools::ReplaceString(depfileFlags, "<OBJECT>", "$out");
      cmSystemTools::ReplaceString(depfileFlags, "<CMAKE_C_COMPILER>",
                                   mf->GetDefinition("CMAKE_C_COMPILER"));
      flags += " " + depfileFlags;
    }
  }

  vars.Flags = flags.c_str();
  vars.DependencyFile = depfile.c_str();

  CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
    this->GetLocalGenerator()->CreateRulePlaceholderExpander());

  std::string const tdi = this->GetLocalGenerator()->ConvertToOutputFormat(
    ConvertToNinjaPath(this->GetTargetDependInfoPath(lang)),
    cmLocalGenerator::SHELL);

  std::string launcher;
  const char* val = this->GetLocalGenerator()->GetRuleLauncher(
    this->GetGeneratorTarget(), "RULE_LAUNCH_COMPILE");
  if (val && *val) {
    launcher = val;
    launcher += " ";
  }

  if (explicitPP) {
    // Lookup the explicit preprocessing rule.
    std::string const ppVar = "CMAKE_" + lang + "_PREPROCESS_SOURCE";
    std::string const ppCmd =
      this->GetMakefile()->GetRequiredDefinition(ppVar);

    // Explicit preprocessing always uses a depfile.
    std::string const ppDeptype; // no deps= for multiple outputs
    std::string const ppDepfile = "$DEP_FILE";

    cmRulePlaceholderExpander::RuleVariables ppVars;
    ppVars.CMTargetName = vars.CMTargetName;
    ppVars.CMTargetType = vars.CMTargetType;
    ppVars.Language = vars.Language;
    ppVars.Object = "$out"; // for RULE_LAUNCH_COMPILE
    ppVars.PreprocessedSource = "$out";
    ppVars.DependencyFile = ppDepfile.c_str();

    // Preprocessing uses the original source,
    // compilation uses preprocessed output.
    ppVars.Source = vars.Source;
    vars.Source = "$in";

    // Preprocessing and compilation use the same flags.
    ppVars.Flags = vars.Flags;

    // Move preprocessor definitions to the preprocessor rule.
    ppVars.Defines = vars.Defines;
    vars.Defines = "";

    // Copy include directories to the preprocessor rule.  The Fortran
    // compilation rule still needs them for the INCLUDE directive.
    ppVars.Includes = vars.Includes;

    // Rule for preprocessing source file.
    std::vector<std::string> ppCmds;
    cmSystemTools::ExpandListArgument(ppCmd, ppCmds);

    for (std::vector<std::string>::iterator i = ppCmds.begin();
         i != ppCmds.end(); ++i) {
      *i = launcher + *i;
      rulePlaceholderExpander->ExpandRuleVariables(this->GetLocalGenerator(),
                                                   *i, ppVars);
    }

    // Run CMake dependency scanner on preprocessed output.
    std::string const cmake = this->GetLocalGenerator()->ConvertToOutputFormat(
      cmSystemTools::GetCMakeCommand(), cmLocalGenerator::SHELL);
    ppCmds.push_back(
      cmake + " -E cmake_ninja_depends"
              " --tdi=" +
      tdi + " --pp=$out"
            " --dep=$DEP_FILE" +
      (needDyndep ? " --obj=$OBJ_FILE --ddi=$DYNDEP_INTERMEDIATE_FILE" : ""));

    std::string const ppCmdLine =
      this->GetLocalGenerator()->BuildCommandLine(ppCmds);

    // Write the rule for preprocessing file of the given language.
    std::ostringstream ppComment;
    ppComment << "Rule for preprocessing " << lang << " files.";
    std::ostringstream ppDesc;
    ppDesc << "Building " << lang << " preprocessed $out";
    this->GetGlobalGenerator()->AddRule(this->LanguagePreprocessRule(lang),
                                        ppCmdLine, ppDesc.str(),
                                        ppComment.str(), ppDepfile, ppDeptype,
                                        /*rspfile*/ "",
                                        /*rspcontent*/ "",
                                        /*restat*/ "",
                                        /*generator*/ false);
  }

  if (needDyndep) {
    // Write the rule for ninja dyndep file generation.
    std::vector<std::string> ddCmds;

#ifdef _WIN32
    // Windows command line length is limited -> use response file for dyndep
    // rules
    std::string ddRspFile = "$out.rsp";
    std::string ddRspContent = "$in";
    std::string ddInput = "@" + ddRspFile;
#else
    std::string ddRspFile;
    std::string ddRspContent;
    std::string ddInput = "$in";
#endif

    // Run CMake dependency scanner on preprocessed output.
    std::string const cmake = this->GetLocalGenerator()->ConvertToOutputFormat(
      cmSystemTools::GetCMakeCommand(), cmLocalGenerator::SHELL);
    ddCmds.push_back(cmake + " -E cmake_ninja_dyndep"
                             " --tdi=" +
                     tdi + " --dd=$out"
                           " " +
                     ddInput);

    std::string const ddCmdLine =
      this->GetLocalGenerator()->BuildCommandLine(ddCmds);

    std::ostringstream ddComment;
    ddComment << "Rule to generate ninja dyndep files for " << lang << ".";
    std::ostringstream ddDesc;
    ddDesc << "Generating " << lang << " dyndep file $out";
    this->GetGlobalGenerator()->AddRule(
      this->LanguageDyndepRule(lang), ddCmdLine, ddDesc.str(), ddComment.str(),
      /*depfile*/ "",
      /*deps*/ "", ddRspFile, ddRspContent,
      /*restat*/ "",
      /*generator*/ false);
  }

  // Rule for compiling object file.
  std::vector<std::string> compileCmds;
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
    std::string compileCmd = mf->GetRequiredDefinition(cmdVar);
    cmSystemTools::ExpandListArgument(compileCmd, compileCmds);
  } else {
    const std::string cmdVar =
      std::string("CMAKE_") + lang + "_COMPILE_OBJECT";
    std::string compileCmd = mf->GetRequiredDefinition(cmdVar);
    cmSystemTools::ExpandListArgument(compileCmd, compileCmds);
  }

  // Maybe insert an include-what-you-use runner.
  if (!compileCmds.empty() && (lang == "C" || lang == "CXX")) {
    std::string const iwyu_prop = lang + "_INCLUDE_WHAT_YOU_USE";
    const char* iwyu = this->GeneratorTarget->GetProperty(iwyu_prop);
    std::string const tidy_prop = lang + "_CLANG_TIDY";
    const char* tidy = this->GeneratorTarget->GetProperty(tidy_prop);
    std::string const cpplint_prop = lang + "_CPPLINT";
    const char* cpplint = this->GeneratorTarget->GetProperty(cpplint_prop);
    if ((iwyu && *iwyu) || (tidy && *tidy) || (cpplint && *cpplint)) {
      std::string run_iwyu = this->GetLocalGenerator()->ConvertToOutputFormat(
        cmSystemTools::GetCMakeCommand(), cmOutputConverter::SHELL);
      run_iwyu += " -E __run_iwyu";
      if (iwyu && *iwyu) {
        run_iwyu += " --iwyu=";
        run_iwyu += this->GetLocalGenerator()->EscapeForShell(iwyu);
      }
      if (tidy && *tidy) {
        run_iwyu += " --tidy=";
        run_iwyu += this->GetLocalGenerator()->EscapeForShell(tidy);
      }
      if (cpplint && *cpplint) {
        run_iwyu += " --cpplint=";
        run_iwyu += this->GetLocalGenerator()->EscapeForShell(cpplint);
      }
      if ((tidy && *tidy) || (cpplint && *cpplint)) {
        run_iwyu += " --source=$in";
      }
      run_iwyu += " -- ";
      compileCmds.front().insert(0, run_iwyu);
    }
  }

  // Maybe insert a compiler launcher like ccache or distcc
  if (!compileCmds.empty() && (lang == "C" || lang == "CXX")) {
    std::string const clauncher_prop = lang + "_COMPILER_LAUNCHER";
    const char* clauncher = this->GeneratorTarget->GetProperty(clauncher_prop);
    if (clauncher && *clauncher) {
      std::vector<std::string> launcher_cmd;
      cmSystemTools::ExpandListArgument(clauncher, launcher_cmd, true);
      for (std::vector<std::string>::iterator i = launcher_cmd.begin(),
                                              e = launcher_cmd.end();
           i != e; ++i) {
        *i = this->LocalGenerator->EscapeForShell(*i);
      }
      std::string const& run_launcher = cmJoin(launcher_cmd, " ") + " ";
      compileCmds.front().insert(0, run_launcher);
    }
  }

  if (!compileCmds.empty()) {
    compileCmds.front().insert(0, cldeps);
  }

  for (std::vector<std::string>::iterator i = compileCmds.begin();
       i != compileCmds.end(); ++i) {
    *i = launcher + *i;
    rulePlaceholderExpander->ExpandRuleVariables(this->GetLocalGenerator(), *i,
                                                 vars);
  }

  std::string cmdLine =
    this->GetLocalGenerator()->BuildCommandLine(compileCmds);

  // Write the rule for compiling file of the given language.
  std::ostringstream comment;
  comment << "Rule for compiling " << lang << " files.";
  std::ostringstream description;
  description << "Building " << lang << " object $out";
  this->GetGlobalGenerator()->AddRule(
    this->LanguageCompilerRule(lang), cmdLine, description.str(),
    comment.str(), depfile, deptype, rspfile, rspcontent,
    /*restat*/ "",
    /*generator*/ false);
}

void cmNinjaTargetGenerator::WriteObjectBuildStatements()
{
  // Write comments.
  cmGlobalNinjaGenerator::WriteDivider(this->GetBuildFileStream());
  this->GetBuildFileStream()
    << "# Object build statements for "
    << cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType())
    << " target " << this->GetTargetName() << "\n\n";

  std::string config = this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
  std::vector<cmSourceFile const*> customCommands;
  this->GeneratorTarget->GetCustomCommands(customCommands, config);
  for (std::vector<cmSourceFile const*>::const_iterator si =
         customCommands.begin();
       si != customCommands.end(); ++si) {
    cmCustomCommand const* cc = (*si)->GetCustomCommand();
    this->GetLocalGenerator()->AddCustomCommandTarget(
      cc, this->GetGeneratorTarget());
    // Record the custom commands for this target. The container is used
    // in WriteObjectBuildStatement when called in a loop below.
    this->CustomCommands.push_back(cc);
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
    this->Objects.push_back(this->GetSourceFilePath(*si));
  }

  cmNinjaDeps orderOnlyDeps;
  this->GetLocalGenerator()->AppendTargetDepends(
    this->GeneratorTarget, orderOnlyDeps, DependOnTargetOrdering);

  // Add order-only dependencies on other files associated with the target.
  orderOnlyDeps.insert(orderOnlyDeps.end(), this->ExtraFiles.begin(),
                       this->ExtraFiles.end());

  // Add order-only dependencies on custom command outputs.
  for (std::vector<cmCustomCommand const*>::const_iterator cci =
         this->CustomCommands.begin();
       cci != this->CustomCommands.end(); ++cci) {
    cmCustomCommand const* cc = *cci;
    cmCustomCommandGenerator ccg(*cc, this->GetConfigName(),
                                 this->GetLocalGenerator());
    const std::vector<std::string>& ccoutputs = ccg.GetOutputs();
    const std::vector<std::string>& ccbyproducts = ccg.GetByproducts();
    std::transform(ccoutputs.begin(), ccoutputs.end(),
                   std::back_inserter(orderOnlyDeps), MapToNinjaPath());
    std::transform(ccbyproducts.begin(), ccbyproducts.end(),
                   std::back_inserter(orderOnlyDeps), MapToNinjaPath());
  }

  std::sort(orderOnlyDeps.begin(), orderOnlyDeps.end());
  orderOnlyDeps.erase(std::unique(orderOnlyDeps.begin(), orderOnlyDeps.end()),
                      orderOnlyDeps.end());

  {
    cmNinjaDeps orderOnlyTarget;
    orderOnlyTarget.push_back(this->OrderDependsTargetForTarget());
    this->GetGlobalGenerator()->WritePhonyBuild(
      this->GetBuildFileStream(),
      "Order-only phony target for " + this->GetTargetName(), orderOnlyTarget,
      cmNinjaDeps(), cmNinjaDeps(), orderOnlyDeps);
  }
  std::vector<cmSourceFile const*> objectSources;
  this->GeneratorTarget->GetObjectSources(objectSources, config);
  for (std::vector<cmSourceFile const*>::const_iterator si =
         objectSources.begin();
       si != objectSources.end(); ++si) {
    this->WriteObjectBuildStatement(*si);
  }

  if (!this->DDIFiles.empty()) {
    std::string const ddComment;
    std::string const ddRule = this->LanguageDyndepRule("Fortran");
    cmNinjaDeps ddOutputs;
    cmNinjaDeps ddImplicitOuts;
    cmNinjaDeps const& ddExplicitDeps = this->DDIFiles;
    cmNinjaDeps ddImplicitDeps;
    cmNinjaDeps ddOrderOnlyDeps;
    cmNinjaVars ddVars;

    this->WriteTargetDependInfo("Fortran");

    ddOutputs.push_back(this->GetDyndepFilePath("Fortran"));

    // Make sure dyndep files for all our dependencies have already
    // been generated so that the 'FortranModules.json' files they
    // produced as side-effects are available for us to read.
    // Ideally we should depend on the 'FortranModules.json' files
    // from our dependencies directly, but we don't know which of
    // our dependencies produces them.  Fixing this will require
    // refactoring the Ninja generator to generate targets in
    // dependency order so that we can collect the needed information.
    this->GetLocalGenerator()->AppendTargetDepends(
      this->GeneratorTarget, ddOrderOnlyDeps, DependOnTargetArtifact);

    this->GetGlobalGenerator()->WriteBuild(
      this->GetBuildFileStream(), ddComment, ddRule, ddOutputs, ddImplicitOuts,
      ddExplicitDeps, ddImplicitDeps, ddOrderOnlyDeps, ddVars);
  }

  this->GetBuildFileStream() << "\n";
}

void cmNinjaTargetGenerator::WriteObjectBuildStatement(
  cmSourceFile const* source)
{
  std::string const language = source->GetLanguage();
  std::string const sourceFileName =
    language == "RC" ? source->GetFullPath() : this->GetSourceFilePath(source);
  std::string const objectDir =
    this->ConvertToNinjaPath(this->GeneratorTarget->GetSupportDirectory());
  std::string const objectFileName =
    this->ConvertToNinjaPath(this->GetObjectFilePath(source));
  std::string const objectFileDir =
    cmSystemTools::GetFilenamePath(objectFileName);

  cmNinjaVars vars;
  vars["FLAGS"] = this->ComputeFlagsForObject(source, language);
  vars["DEFINES"] = this->ComputeDefines(source, language);
  vars["INCLUDES"] = this->GetIncludes(language);
  if (!this->NeedDepTypeMSVC(language)) {
    vars["DEP_FILE"] = this->GetLocalGenerator()->ConvertToOutputFormat(
      objectFileName + ".d", cmOutputConverter::SHELL);
  }

  this->ExportObjectCompileCommand(
    language, sourceFileName, objectDir, objectFileName, objectFileDir,
    vars["FLAGS"], vars["DEFINES"], vars["INCLUDES"]);

  std::string comment;
  std::string rule = this->LanguageCompilerRule(language);

  cmNinjaDeps outputs;
  outputs.push_back(objectFileName);
  // Add this object to the list of object files.
  this->Objects.push_back(objectFileName);

  cmNinjaDeps explicitDeps;
  explicitDeps.push_back(sourceFileName);

  cmNinjaDeps implicitDeps;
  if (const char* objectDeps = source->GetProperty("OBJECT_DEPENDS")) {
    std::vector<std::string> depList;
    cmSystemTools::ExpandListArgument(objectDeps, depList);
    for (std::vector<std::string>::iterator odi = depList.begin();
         odi != depList.end(); ++odi) {
      if (cmSystemTools::FileIsFullPath(*odi)) {
        *odi = cmSystemTools::CollapseFullPath(*odi);
      }
    }
    std::transform(depList.begin(), depList.end(),
                   std::back_inserter(implicitDeps), MapToNinjaPath());
  }

  cmNinjaDeps orderOnlyDeps;
  orderOnlyDeps.push_back(this->OrderDependsTargetForTarget());

  // If the source file is GENERATED and does not have a custom command
  // (either attached to this source file or another one), assume that one of
  // the target dependencies, OBJECT_DEPENDS or header file custom commands
  // will rebuild the file.
  if (source->GetPropertyAsBool("GENERATED") && !source->GetCustomCommand() &&
      !this->GetGlobalGenerator()->HasCustomCommandOutput(sourceFileName)) {
    this->GetGlobalGenerator()->AddAssumedSourceDependencies(sourceFileName,
                                                             orderOnlyDeps);
  }

  // For some cases we need to generate a ninja dyndep file.
  bool const needDyndep = this->NeedDyndep(language);

  // For some cases we do an explicit preprocessor invocation.
  bool const explicitPP = this->NeedExplicitPreprocessing(language);
  if (explicitPP) {
    std::string const ppComment;
    std::string const ppRule = this->LanguagePreprocessRule(language);
    cmNinjaDeps ppOutputs;
    cmNinjaDeps ppImplicitOuts;
    cmNinjaDeps ppExplicitDeps;
    cmNinjaDeps ppImplicitDeps;
    cmNinjaDeps ppOrderOnlyDeps;
    cmNinjaVars ppVars;

    std::string const ppFileName =
      this->ConvertToNinjaPath(this->GetPreprocessedFilePath(source));
    ppOutputs.push_back(ppFileName);

    // Move compilation dependencies to the preprocessing build statement.
    std::swap(ppExplicitDeps, explicitDeps);
    std::swap(ppImplicitDeps, implicitDeps);
    std::swap(ppOrderOnlyDeps, orderOnlyDeps);
    std::swap(ppVars["IN_ABS"], vars["IN_ABS"]);

    // The actual compilation will now use the preprocessed source.
    explicitDeps.push_back(ppFileName);

    // Preprocessing and compilation use the same flags.
    ppVars["FLAGS"] = vars["FLAGS"];

    // Move preprocessor definitions to the preprocessor build statement.
    std::swap(ppVars["DEFINES"], vars["DEFINES"]);

    // Copy include directories to the preprocessor build statement.  The
    // Fortran compilation build statement still needs them for the INCLUDE
    // directive.
    ppVars["INCLUDES"] = vars["INCLUDES"];

    // Prepend source file's original directory as an include directory
    // so e.g. Fortran INCLUDE statements can look for files in it.
    std::vector<std::string> sourceDirectory;
    sourceDirectory.push_back(
      cmSystemTools::GetParentDirectory(source->GetFullPath()));

    std::string sourceDirectoryFlag = this->LocalGenerator->GetIncludeFlags(
      sourceDirectory, this->GeneratorTarget, language, false, false,
      this->GetConfigName());

    vars["INCLUDES"] = sourceDirectoryFlag + " " + vars["INCLUDES"];

    // Explicit preprocessing always uses a depfile.
    ppVars["DEP_FILE"] = this->GetLocalGenerator()->ConvertToOutputFormat(
      ppFileName + ".d", cmOutputConverter::SHELL);
    // The actual compilation does not need a depfile because it
    // depends on the already-preprocessed source.
    vars.erase("DEP_FILE");

    if (needDyndep) {
      // Tell dependency scanner the object file that will result from
      // compiling the preprocessed source.
      ppVars["OBJ_FILE"] = objectFileName;

      // Tell dependency scanner where to store dyndep intermediate results.
      std::string const ddiFile = ppFileName + ".ddi";
      ppVars["DYNDEP_INTERMEDIATE_FILE"] = ddiFile;
      ppImplicitOuts.push_back(ddiFile);
      this->DDIFiles.push_back(ddiFile);
    }

    this->addPoolNinjaVariable("JOB_POOL_COMPILE", this->GetGeneratorTarget(),
                               ppVars);

    this->GetGlobalGenerator()->WriteBuild(
      this->GetBuildFileStream(), ppComment, ppRule, ppOutputs, ppImplicitOuts,
      ppExplicitDeps, ppImplicitDeps, ppOrderOnlyDeps, ppVars);
  }
  if (needDyndep) {
    std::string const dyndep = this->GetDyndepFilePath(language);
    orderOnlyDeps.push_back(dyndep);
    vars["dyndep"] = dyndep;
  }

  EnsureParentDirectoryExists(objectFileName);

  vars["OBJECT_DIR"] = this->GetLocalGenerator()->ConvertToOutputFormat(
    objectDir, cmOutputConverter::SHELL);
  vars["OBJECT_FILE_DIR"] = this->GetLocalGenerator()->ConvertToOutputFormat(
    objectFileDir, cmOutputConverter::SHELL);

  this->addPoolNinjaVariable("JOB_POOL_COMPILE", this->GetGeneratorTarget(),
                             vars);

  this->SetMsvcTargetPdbVariable(vars);

  bool const lang_supports_response =
    !(language == "RC" || language == "CUDA");
  int const commandLineLengthLimit =
    ((lang_supports_response && this->ForceResponseFile())) ? -1 : 0;
  std::string const rspfile = objectFileName + ".rsp";

  this->GetGlobalGenerator()->WriteBuild(
    this->GetBuildFileStream(), comment, rule, outputs,
    /*implicitOuts=*/cmNinjaDeps(), explicitDeps, implicitDeps, orderOnlyDeps,
    vars, rspfile, commandLineLengthLimit);

  if (const char* objectOutputs = source->GetProperty("OBJECT_OUTPUTS")) {
    std::vector<std::string> outputList;
    cmSystemTools::ExpandListArgument(objectOutputs, outputList);
    std::transform(outputList.begin(), outputList.end(), outputList.begin(),
                   MapToNinjaPath());
    this->GetGlobalGenerator()->WritePhonyBuild(this->GetBuildFileStream(),
                                                "Additional output files.",
                                                outputList, outputs);
  }
}

void cmNinjaTargetGenerator::WriteTargetDependInfo(std::string const& lang)
{
  Json::Value tdi(Json::objectValue);
  tdi["language"] = lang;
  tdi["compiler-id"] =
    this->Makefile->GetSafeDefinition("CMAKE_" + lang + "_COMPILER_ID");

  if (lang == "Fortran") {
    std::string mod_dir = this->GeneratorTarget->GetFortranModuleDirectory(
      this->Makefile->GetHomeOutputDirectory());
    if (mod_dir.empty()) {
      mod_dir = this->Makefile->GetCurrentBinaryDirectory();
    }
    tdi["module-dir"] = mod_dir;
  }

  tdi["dir-cur-bld"] = this->Makefile->GetCurrentBinaryDirectory();
  tdi["dir-cur-src"] = this->Makefile->GetCurrentSourceDirectory();
  tdi["dir-top-bld"] = this->Makefile->GetHomeOutputDirectory();
  tdi["dir-top-src"] = this->Makefile->GetHomeDirectory();

  Json::Value& tdi_include_dirs = tdi["include-dirs"] = Json::arrayValue;
  std::vector<std::string> includes;
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              lang, this->GetConfigName());
  for (std::vector<std::string>::iterator i = includes.begin();
       i != includes.end(); ++i) {
    // Convert the include directories the same way we do for -I flags.
    // See upstream ninja issue 1251.
    tdi_include_dirs.append(this->ConvertToNinjaPath(*i));
  }

  Json::Value& tdi_linked_target_dirs = tdi["linked-target-dirs"] =
    Json::arrayValue;
  std::vector<std::string> linked = this->GetLinkedTargetDirectories();
  for (std::vector<std::string>::iterator i = linked.begin();
       i != linked.end(); ++i) {
    tdi_linked_target_dirs.append(*i);
  }

  std::string const tdin = this->GetTargetDependInfoPath(lang);
  cmGeneratedFileStream tdif(tdin.c_str());
  tdif << tdi;
}

void cmNinjaTargetGenerator::ExportObjectCompileCommand(
  std::string const& language, std::string const& sourceFileName,
  std::string const& objectDir, std::string const& objectFileName,
  std::string const& objectFileDir, std::string const& flags,
  std::string const& defines, std::string const& includes)
{
  if (!this->Makefile->IsOn("CMAKE_EXPORT_COMPILE_COMMANDS")) {
    return;
  }

  cmRulePlaceholderExpander::RuleVariables compileObjectVars;
  compileObjectVars.Language = language.c_str();

  std::string escapedSourceFileName = sourceFileName;

  if (!cmSystemTools::FileIsFullPath(sourceFileName.c_str())) {
    escapedSourceFileName = cmSystemTools::CollapseFullPath(
      escapedSourceFileName, this->GetGlobalGenerator()
                               ->GetCMakeInstance()
                               ->GetHomeOutputDirectory());
  }

  escapedSourceFileName = this->LocalGenerator->ConvertToOutputFormat(
    escapedSourceFileName, cmOutputConverter::SHELL);

  compileObjectVars.Source = escapedSourceFileName.c_str();
  compileObjectVars.Object = objectFileName.c_str();
  compileObjectVars.ObjectDir = objectDir.c_str();
  compileObjectVars.ObjectFileDir = objectFileDir.c_str();
  compileObjectVars.Flags = flags.c_str();
  compileObjectVars.Defines = defines.c_str();
  compileObjectVars.Includes = includes.c_str();

  // Rule for compiling object file.
  std::vector<std::string> compileCmds;
  if (language == "CUDA") {
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
    std::string compileCmd =
      this->GetMakefile()->GetRequiredDefinition(cmdVar);
    cmSystemTools::ExpandListArgument(compileCmd, compileCmds);
  } else {
    const std::string cmdVar =
      std::string("CMAKE_") + language + "_COMPILE_OBJECT";
    std::string compileCmd =
      this->GetMakefile()->GetRequiredDefinition(cmdVar);
    cmSystemTools::ExpandListArgument(compileCmd, compileCmds);
  }

  CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
    this->GetLocalGenerator()->CreateRulePlaceholderExpander());

  for (std::vector<std::string>::iterator i = compileCmds.begin();
       i != compileCmds.end(); ++i) {
    // no launcher for CMAKE_EXPORT_COMPILE_COMMANDS
    rulePlaceholderExpander->ExpandRuleVariables(this->GetLocalGenerator(), *i,
                                                 compileObjectVars);
  }

  std::string cmdLine =
    this->GetLocalGenerator()->BuildCommandLine(compileCmds);

  this->GetGlobalGenerator()->AddCXXCompileCommand(cmdLine, sourceFileName);
}

void cmNinjaTargetGenerator::EnsureDirectoryExists(
  const std::string& path) const
{
  if (cmSystemTools::FileIsFullPath(path.c_str())) {
    cmSystemTools::MakeDirectory(path.c_str());
  } else {
    cmGlobalNinjaGenerator* gg = this->GetGlobalGenerator();
    std::string fullPath =
      std::string(gg->GetCMakeInstance()->GetHomeOutputDirectory());
    // Also ensures their is a trailing slash.
    gg->StripNinjaOutputPathPrefixAsSuffix(fullPath);
    fullPath += path;
    cmSystemTools::MakeDirectory(fullPath.c_str());
  }
}

void cmNinjaTargetGenerator::EnsureParentDirectoryExists(
  const std::string& path) const
{
  EnsureDirectoryExists(cmSystemTools::GetParentDirectory(path));
}

void cmNinjaTargetGenerator::MacOSXContentGeneratorType::operator()(
  cmSourceFile const& source, const char* pkgloc)
{
  // Skip OS X content when not building a Framework or Bundle.
  if (!this->Generator->GetGeneratorTarget()->IsBundleOnApple()) {
    return;
  }

  std::string macdir =
    this->Generator->OSXBundleGenerator->InitMacOSXContentDirectory(pkgloc);

  // Get the input file location.
  std::string input = source.GetFullPath();
  input = this->Generator->GetGlobalGenerator()->ConvertToNinjaPath(input);

  // Get the output file location.
  std::string output = macdir;
  output += "/";
  output += cmSystemTools::GetFilenameName(input);
  output = this->Generator->GetGlobalGenerator()->ConvertToNinjaPath(output);

  // Write a build statement to copy the content into the bundle.
  this->Generator->GetGlobalGenerator()->WriteMacOSXContentBuild(input,
                                                                 output);

  // Add as a dependency to the target so that it gets called.
  this->Generator->ExtraFiles.push_back(output);
}

void cmNinjaTargetGenerator::addPoolNinjaVariable(
  const std::string& pool_property, cmGeneratorTarget* target,
  cmNinjaVars& vars)
{
  const char* pool = target->GetProperty(pool_property);
  if (pool) {
    vars["pool"] = pool;
  }
}

bool cmNinjaTargetGenerator::ForceResponseFile()
{
  static std::string const forceRspFile = "CMAKE_NINJA_FORCE_RESPONSE_FILE";
  return (this->GetMakefile()->IsDefinitionSet(forceRspFile) ||
          cmSystemTools::HasEnv(forceRspFile));
}
