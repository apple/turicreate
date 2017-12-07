/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalNinjaGenerator.h"

#include "cm_jsoncpp_reader.h"
#include "cm_jsoncpp_value.h"
#include "cm_jsoncpp_writer.h"
#include "cmsys/FStream.hxx"
#include <algorithm>
#include <ctype.h>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdio.h>

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmFortranParser.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpressionEvaluationFile.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmLocalNinjaGenerator.h"
#include "cmMakefile.h"
#include "cmNinjaLinkLineComputer.h"
#include "cmOutputConverter.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetDepend.h"
#include "cmVersion.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

class cmLinkLineComputer;

const char* cmGlobalNinjaGenerator::NINJA_BUILD_FILE = "build.ninja";
const char* cmGlobalNinjaGenerator::NINJA_RULES_FILE = "rules.ninja";
const char* cmGlobalNinjaGenerator::INDENT = "  ";
#ifdef _WIN32
std::string const cmGlobalNinjaGenerator::SHELL_NOOP = "cd .";
#else
std::string const cmGlobalNinjaGenerator::SHELL_NOOP = ":";
#endif

void cmGlobalNinjaGenerator::Indent(std::ostream& os, int count)
{
  for (int i = 0; i < count; ++i) {
    os << cmGlobalNinjaGenerator::INDENT;
  }
}

void cmGlobalNinjaGenerator::WriteDivider(std::ostream& os)
{
  os << "# ======================================"
        "=======================================\n";
}

void cmGlobalNinjaGenerator::WriteComment(std::ostream& os,
                                          const std::string& comment)
{
  if (comment.empty()) {
    return;
  }

  std::string::size_type lpos = 0;
  std::string::size_type rpos;
  os << "\n#############################################\n";
  while ((rpos = comment.find('\n', lpos)) != std::string::npos) {
    os << "# " << comment.substr(lpos, rpos - lpos) << "\n";
    lpos = rpos + 1;
  }
  os << "# " << comment.substr(lpos) << "\n\n";
}

cmLinkLineComputer* cmGlobalNinjaGenerator::CreateLinkLineComputer(
  cmOutputConverter* outputConverter,
  cmStateDirectory const& /* stateDir */) const
{
  return new cmNinjaLinkLineComputer(
    outputConverter,
    this->LocalGenerators[0]->GetStateSnapshot().GetDirectory(), this);
}

std::string cmGlobalNinjaGenerator::EncodeRuleName(std::string const& name)
{
  // Ninja rule names must match "[a-zA-Z0-9_.-]+".  Use ".xx" to encode
  // "." and all invalid characters as hexadecimal.
  std::string encoded;
  for (std::string::const_iterator i = name.begin(); i != name.end(); ++i) {
    if (isalnum(*i) || *i == '_' || *i == '-') {
      encoded += *i;
    } else {
      char buf[16];
      sprintf(buf, ".%02x", static_cast<unsigned int>(*i));
      encoded += buf;
    }
  }
  return encoded;
}

static bool IsIdentChar(char c)
{
  return ('a' <= c && c <= 'z') ||
    ('+' <= c && c <= '9') || // +,-./ and numbers
    ('A' <= c && c <= 'Z') || (c == '_') || (c == '$') || (c == '\\') ||
    (c == ' ') || (c == ':');
}

std::string cmGlobalNinjaGenerator::EncodeIdent(const std::string& ident,
                                                std::ostream& vars)
{
  if (std::find_if(ident.begin(), ident.end(),
                   std::not1(std::ptr_fun(IsIdentChar))) != ident.end()) {
    static unsigned VarNum = 0;
    std::ostringstream names;
    names << "ident" << VarNum++;
    vars << names.str() << " = " << ident << "\n";
    return "$" + names.str();
  }
  std::string result = ident;
  cmSystemTools::ReplaceString(result, " ", "$ ");
  cmSystemTools::ReplaceString(result, ":", "$:");
  return result;
}

std::string cmGlobalNinjaGenerator::EncodeLiteral(const std::string& lit)
{
  std::string result = lit;
  cmSystemTools::ReplaceString(result, "$", "$$");
  cmSystemTools::ReplaceString(result, "\n", "$\n");
  return result;
}

std::string cmGlobalNinjaGenerator::EncodePath(const std::string& path)
{
  std::string result = path; // NOLINT(clang-tidy)
#ifdef _WIN32
  if (this->IsGCCOnWindows())
    std::replace(result.begin(), result.end(), '\\', '/');
  else
    std::replace(result.begin(), result.end(), '/', '\\');
#endif
  return EncodeLiteral(result);
}

void cmGlobalNinjaGenerator::WriteBuild(
  std::ostream& os, const std::string& comment, const std::string& rule,
  const cmNinjaDeps& outputs, const cmNinjaDeps& implicitOuts,
  const cmNinjaDeps& explicitDeps, const cmNinjaDeps& implicitDeps,
  const cmNinjaDeps& orderOnlyDeps, const cmNinjaVars& variables,
  const std::string& rspfile, int cmdLineLimit, bool* usedResponseFile)
{
  // Make sure there is a rule.
  if (rule.empty()) {
    cmSystemTools::Error("No rule for WriteBuildStatement! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Make sure there is at least one output file.
  if (outputs.empty()) {
    cmSystemTools::Error("No output files for WriteBuildStatement! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  cmGlobalNinjaGenerator::WriteComment(os, comment);

  std::string arguments;

  // TODO: Better formatting for when there are multiple input/output files.

  // Write explicit dependencies.
  for (cmNinjaDeps::const_iterator i = explicitDeps.begin();
       i != explicitDeps.end(); ++i) {
    arguments += " " + EncodeIdent(EncodePath(*i), os);
  }

  // Write implicit dependencies.
  if (!implicitDeps.empty()) {
    arguments += " |";
    for (cmNinjaDeps::const_iterator i = implicitDeps.begin();
         i != implicitDeps.end(); ++i) {
      arguments += " " + EncodeIdent(EncodePath(*i), os);
    }
  }

  // Write order-only dependencies.
  if (!orderOnlyDeps.empty()) {
    arguments += " ||";
    for (cmNinjaDeps::const_iterator i = orderOnlyDeps.begin();
         i != orderOnlyDeps.end(); ++i) {
      arguments += " " + EncodeIdent(EncodePath(*i), os);
    }
  }

  arguments += "\n";

  std::string build;

  // Write outputs files.
  build += "build";
  for (cmNinjaDeps::const_iterator i = outputs.begin(); i != outputs.end();
       ++i) {
    build += " " + EncodeIdent(EncodePath(*i), os);
    if (this->ComputingUnknownDependencies) {
      this->CombinedBuildOutputs.insert(*i);
    }
  }
  if (!implicitOuts.empty()) {
    build += " |";
    for (cmNinjaDeps::const_iterator i = implicitOuts.begin();
         i != implicitOuts.end(); ++i) {
      build += " " + EncodeIdent(EncodePath(*i), os);
    }
  }
  build += ":";

  // Write the rule.
  build += " " + rule;

  // Write the variables bound to this build statement.
  std::ostringstream variable_assignments;
  for (cmNinjaVars::const_iterator i = variables.begin(); i != variables.end();
       ++i) {
    cmGlobalNinjaGenerator::WriteVariable(variable_assignments, i->first,
                                          i->second, "", 1);
  }

  // check if a response file rule should be used
  std::string buildstr = build;
  std::string assignments = variable_assignments.str();
  const std::string& args = arguments;
  bool useResponseFile = false;
  if (cmdLineLimit < 0 ||
      (cmdLineLimit > 0 &&
       (args.size() + buildstr.size() + assignments.size() + 1000) >
         static_cast<size_t>(cmdLineLimit))) {
    variable_assignments.str(std::string());
    cmGlobalNinjaGenerator::WriteVariable(variable_assignments, "RSP_FILE",
                                          rspfile, "", 1);
    assignments += variable_assignments.str();
    useResponseFile = true;
  }
  if (usedResponseFile) {
    *usedResponseFile = useResponseFile;
  }

  os << buildstr << args << assignments;
}

void cmGlobalNinjaGenerator::WritePhonyBuild(
  std::ostream& os, const std::string& comment, const cmNinjaDeps& outputs,
  const cmNinjaDeps& explicitDeps, const cmNinjaDeps& implicitDeps,
  const cmNinjaDeps& orderOnlyDeps, const cmNinjaVars& variables)
{
  this->WriteBuild(os, comment, "phony", outputs,
                   /*implicitOuts=*/cmNinjaDeps(), explicitDeps, implicitDeps,
                   orderOnlyDeps, variables);
}

void cmGlobalNinjaGenerator::AddCustomCommandRule()
{
  this->AddRule("CUSTOM_COMMAND", "$COMMAND", "$DESC",
                "Rule for running custom commands.",
                /*depfile*/ "",
                /*deptype*/ "",
                /*rspfile*/ "",
                /*rspcontent*/ "",
                /*restat*/ "", // bound on each build statement as needed
                /*generator*/ false);
}

void cmGlobalNinjaGenerator::WriteCustomCommandBuild(
  const std::string& command, const std::string& description,
  const std::string& comment, const std::string& depfile, bool uses_terminal,
  bool restat, const cmNinjaDeps& outputs, const cmNinjaDeps& deps,
  const cmNinjaDeps& orderOnly)
{
  std::string cmd = command; // NOLINT(clang-tidy)
#ifdef _WIN32
  if (cmd.empty())
    // TODO Shouldn't an empty command be handled by ninja?
    cmd = "cmd.exe /c";
#endif

  this->AddCustomCommandRule();

  cmNinjaVars vars;
  vars["COMMAND"] = cmd;
  vars["DESC"] = EncodeLiteral(description);
  if (restat) {
    vars["restat"] = "1";
  }
  if (uses_terminal && SupportsConsolePool()) {
    vars["pool"] = "console";
  }
  if (!depfile.empty()) {
    vars["depfile"] = depfile;
  }
  this->WriteBuild(*this->BuildFileStream, comment, "CUSTOM_COMMAND", outputs,
                   /*implicitOuts=*/cmNinjaDeps(), deps, cmNinjaDeps(),
                   orderOnly, vars);

  if (this->ComputingUnknownDependencies) {
    // we need to track every dependency that comes in, since we are trying
    // to find dependencies that are side effects of build commands
    for (cmNinjaDeps::const_iterator i = deps.begin(); i != deps.end(); ++i) {
      this->CombinedCustomCommandExplicitDependencies.insert(*i);
    }
  }
}

void cmGlobalNinjaGenerator::AddMacOSXContentRule()
{
  cmLocalGenerator* lg = this->LocalGenerators[0];

  std::ostringstream cmd;
  cmd << lg->ConvertToOutputFormat(cmSystemTools::GetCMakeCommand(),
                                   cmOutputConverter::SHELL)
      << " -E copy $in $out";

  this->AddRule("COPY_OSX_CONTENT", cmd.str(), "Copying OS X Content $out",
                "Rule for copying OS X bundle content file.",
                /*depfile*/ "",
                /*deptype*/ "",
                /*rspfile*/ "",
                /*rspcontent*/ "",
                /*restat*/ "",
                /*generator*/ false);
}

void cmGlobalNinjaGenerator::WriteMacOSXContentBuild(const std::string& input,
                                                     const std::string& output)
{
  this->AddMacOSXContentRule();

  cmNinjaDeps outputs;
  outputs.push_back(output);
  cmNinjaDeps deps;
  deps.push_back(input);
  cmNinjaVars vars;

  this->WriteBuild(*this->BuildFileStream, "", "COPY_OSX_CONTENT", outputs,
                   /*implicitOuts=*/cmNinjaDeps(), deps, cmNinjaDeps(),
                   cmNinjaDeps(), cmNinjaVars());
}

void cmGlobalNinjaGenerator::WriteRule(
  std::ostream& os, const std::string& name, const std::string& command,
  const std::string& description, const std::string& comment,
  const std::string& depfile, const std::string& deptype,
  const std::string& rspfile, const std::string& rspcontent,
  const std::string& restat, bool generator)
{
  // Make sure the rule has a name.
  if (name.empty()) {
    cmSystemTools::Error("No name given for WriteRuleStatement! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Make sure a command is given.
  if (command.empty()) {
    cmSystemTools::Error("No command given for WriteRuleStatement! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  cmGlobalNinjaGenerator::WriteComment(os, comment);

  // Write the rule.
  os << "rule " << name << "\n";

  // Write the depfile if any.
  if (!depfile.empty()) {
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "depfile = " << depfile << "\n";
  }

  // Write the deptype if any.
  if (!deptype.empty()) {
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "deps = " << deptype << "\n";
  }

  // Write the command.
  cmGlobalNinjaGenerator::Indent(os, 1);
  os << "command = " << command << "\n";

  // Write the description if any.
  if (!description.empty()) {
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "description = " << description << "\n";
  }

  if (!rspfile.empty()) {
    if (rspcontent.empty()) {
      cmSystemTools::Error("No rspfile_content given!", comment.c_str());
      return;
    }
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "rspfile = " << rspfile << "\n";
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "rspfile_content = " << rspcontent << "\n";
  }

  if (!restat.empty()) {
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "restat = " << restat << "\n";
  }

  if (generator) {
    cmGlobalNinjaGenerator::Indent(os, 1);
    os << "generator = 1\n";
  }

  os << "\n";
}

void cmGlobalNinjaGenerator::WriteVariable(std::ostream& os,
                                           const std::string& name,
                                           const std::string& value,
                                           const std::string& comment,
                                           int indent)
{
  // Make sure we have a name.
  if (name.empty()) {
    cmSystemTools::Error("No name given for WriteVariable! called "
                         "with comment: ",
                         comment.c_str());
    return;
  }

  // Do not add a variable if the value is empty.
  std::string val = cmSystemTools::TrimWhitespace(value);
  if (val.empty()) {
    return;
  }

  cmGlobalNinjaGenerator::WriteComment(os, comment);
  cmGlobalNinjaGenerator::Indent(os, indent);
  os << name << " = " << val << "\n";
}

void cmGlobalNinjaGenerator::WriteInclude(std::ostream& os,
                                          const std::string& filename,
                                          const std::string& comment)
{
  cmGlobalNinjaGenerator::WriteComment(os, comment);
  os << "include " << filename << "\n";
}

void cmGlobalNinjaGenerator::WriteDefault(std::ostream& os,
                                          const cmNinjaDeps& targets,
                                          const std::string& comment)
{
  cmGlobalNinjaGenerator::WriteComment(os, comment);
  os << "default";
  for (cmNinjaDeps::const_iterator i = targets.begin(); i != targets.end();
       ++i) {
    os << " " << *i;
  }
  os << "\n";
}

cmGlobalNinjaGenerator::cmGlobalNinjaGenerator(cmake* cm)
  : cmGlobalCommonGenerator(cm)
  , BuildFileStream(CM_NULLPTR)
  , RulesFileStream(CM_NULLPTR)
  , CompileCommandsStream(CM_NULLPTR)
  , Rules()
  , AllDependencies()
  , UsingGCCOnWindows(false)
  , ComputingUnknownDependencies(false)
  , PolicyCMP0058(cmPolicies::WARN)
  , NinjaSupportsConsolePool(false)
  , NinjaSupportsImplicitOuts(false)
  , NinjaSupportsDyndeps(0)
{
#ifdef _WIN32
  cm->GetState()->SetWindowsShell(true);
#endif
  // // Ninja is not ported to non-Unix OS yet.
  // this->ForceUnixPaths = true;
  this->FindMakeProgramFile = "CMakeNinjaFindMake.cmake";
}

// Virtual public methods.

cmLocalGenerator* cmGlobalNinjaGenerator::CreateLocalGenerator(cmMakefile* mf)
{
  return new cmLocalNinjaGenerator(this, mf);
}

codecvt::Encoding cmGlobalNinjaGenerator::GetMakefileEncoding() const
{
#ifdef _WIN32
  // Ninja on Windows does not support non-ANSI characters.
  // https://github.com/ninja-build/ninja/issues/1195
  return codecvt::ANSI;
#else
  // No encoding conversion needed on other platforms.
  return codecvt::None;
#endif
}

void cmGlobalNinjaGenerator::GetDocumentation(cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalNinjaGenerator::GetActualName();
  entry.Brief = "Generates build.ninja files.";
}

// Implemented in all cmGlobaleGenerator sub-classes.
// Used in:
//   Source/cmLocalGenerator.cxx
//   Source/cmake.cxx
void cmGlobalNinjaGenerator::Generate()
{
  // Check minimum Ninja version.
  if (cmSystemTools::VersionCompare(cmSystemTools::OP_LESS,
                                    this->NinjaVersion.c_str(),
                                    RequiredNinjaVersion().c_str())) {
    std::ostringstream msg;
    msg << "The detected version of Ninja (" << this->NinjaVersion;
    msg << ") is less than the version of Ninja required by CMake (";
    msg << this->RequiredNinjaVersion() << ").";
    this->GetCMakeInstance()->IssueMessage(cmake::FATAL_ERROR, msg.str());
    return;
  }
  this->OpenBuildFileStream();
  this->OpenRulesFileStream();

  this->TargetDependsClosures.clear();

  this->InitOutputPathPrefix();
  this->TargetAll = this->NinjaOutputPath("all");
  this->CMakeCacheFile = this->NinjaOutputPath("CMakeCache.txt");

  this->PolicyCMP0058 =
    this->LocalGenerators[0]->GetMakefile()->GetPolicyStatus(
      cmPolicies::CMP0058);
  this->ComputingUnknownDependencies =
    (this->PolicyCMP0058 == cmPolicies::OLD ||
     this->PolicyCMP0058 == cmPolicies::WARN);

  this->cmGlobalGenerator::Generate();

  this->WriteAssumedSourceDependencies();
  this->WriteTargetAliases(*this->BuildFileStream);
  this->WriteFolderTargets(*this->BuildFileStream);
  this->WriteUnknownExplicitDependencies(*this->BuildFileStream);
  this->WriteBuiltinTargets(*this->BuildFileStream);

  if (cmSystemTools::GetErrorOccuredFlag()) {
    this->RulesFileStream->setstate(std::ios::failbit);
    this->BuildFileStream->setstate(std::ios::failbit);
  }

  this->CloseCompileCommandsStream();
  this->CloseRulesFileStream();
  this->CloseBuildFileStream();
}

bool cmGlobalNinjaGenerator::FindMakeProgram(cmMakefile* mf)
{
  if (!this->cmGlobalGenerator::FindMakeProgram(mf)) {
    return false;
  }
  if (const char* ninjaCommand = mf->GetDefinition("CMAKE_MAKE_PROGRAM")) {
    this->NinjaCommand = ninjaCommand;
    std::vector<std::string> command;
    command.push_back(this->NinjaCommand);
    command.push_back("--version");
    std::string version;
    std::string error;
    if (!cmSystemTools::RunSingleCommand(command, &version, &error, CM_NULLPTR,
                                         CM_NULLPTR,
                                         cmSystemTools::OUTPUT_NONE)) {
      mf->IssueMessage(cmake::FATAL_ERROR, "Running\n '" +
                         cmJoin(command, "' '") + "'\n"
                                                  "failed with:\n " +
                         error);
      cmSystemTools::SetFatalErrorOccured();
      return false;
    }
    this->NinjaVersion = cmSystemTools::TrimWhitespace(version);
    this->CheckNinjaFeatures();
  }
  return true;
}

void cmGlobalNinjaGenerator::CheckNinjaFeatures()
{
  this->NinjaSupportsConsolePool = !cmSystemTools::VersionCompare(
    cmSystemTools::OP_LESS, this->NinjaVersion.c_str(),
    RequiredNinjaVersionForConsolePool().c_str());
  this->NinjaSupportsImplicitOuts = !cmSystemTools::VersionCompare(
    cmSystemTools::OP_LESS, this->NinjaVersion.c_str(),
    this->RequiredNinjaVersionForImplicitOuts().c_str());
  {
    // Our ninja branch adds ".dyndep-#" to its version number,
    // where '#' is a feature-specific version number.  Extract it.
    static std::string const k_DYNDEP_ = ".dyndep-";
    std::string::size_type pos = this->NinjaVersion.find(k_DYNDEP_);
    if (pos != std::string::npos) {
      const char* fv = this->NinjaVersion.c_str() + pos + k_DYNDEP_.size();
      cmSystemTools::StringToULong(fv, &this->NinjaSupportsDyndeps);
    }
  }
}

bool cmGlobalNinjaGenerator::CheckLanguages(
  std::vector<std::string> const& languages, cmMakefile* mf) const
{
  if (std::find(languages.begin(), languages.end(), "Fortran") !=
      languages.end()) {
    return this->CheckFortran(mf);
  }
  return true;
}

bool cmGlobalNinjaGenerator::CheckFortran(cmMakefile* mf) const
{
  if (this->NinjaSupportsDyndeps == 1) {
    return true;
  }

  std::ostringstream e;
  if (this->NinjaSupportsDyndeps == 0) {
    /* clang-format off */
    e <<
      "The Ninja generator does not support Fortran using Ninja version\n"
      "  " + this->NinjaVersion + "\n"
      "due to lack of required features.  "
      "Kitware has implemented the required features but as of this version "
      "of CMake they have not been integrated to upstream ninja.  "
      "Pending integration, Kitware maintains a branch at:\n"
      "  https://github.com/Kitware/ninja/tree/features-for-fortran#readme\n"
      "with the required features.  "
      "One may build ninja from that branch to get support for Fortran."
      ;
    /* clang-format on */
  } else {
    /* clang-format off */
    e <<
      "The Ninja generator in this version of CMake does not support Fortran "
      "using Ninja version\n"
      "  " + this->NinjaVersion + "\n"
      "because its 'dyndep' feature version is " <<
      this->NinjaSupportsDyndeps << ".  "
      "This version of CMake is aware only of 'dyndep' feature version 1."
      ;
    /* clang-format on */
  }
  mf->IssueMessage(cmake::FATAL_ERROR, e.str());
  cmSystemTools::SetFatalErrorOccured();
  return false;
}

void cmGlobalNinjaGenerator::EnableLanguage(
  std::vector<std::string> const& langs, cmMakefile* mf, bool optional)
{
  this->cmGlobalGenerator::EnableLanguage(langs, mf, optional);
  for (std::vector<std::string>::const_iterator l = langs.begin();
       l != langs.end(); ++l) {
    if (*l == "NONE") {
      continue;
    }
    this->ResolveLanguageCompiler(*l, mf, optional);
  }
#ifdef _WIN32
  if (strcmp(mf->GetSafeDefinition("CMAKE_C_SIMULATE_ID"), "MSVC") != 0 &&
      strcmp(mf->GetSafeDefinition("CMAKE_CXX_SIMULATE_ID"), "MSVC") != 0 &&
      (mf->IsOn("CMAKE_COMPILER_IS_MINGW") ||
       strcmp(mf->GetSafeDefinition("CMAKE_C_COMPILER_ID"), "GNU") == 0 ||
       strcmp(mf->GetSafeDefinition("CMAKE_CXX_COMPILER_ID"), "GNU") == 0 ||
       strcmp(mf->GetSafeDefinition("CMAKE_C_COMPILER_ID"), "Clang") == 0 ||
       strcmp(mf->GetSafeDefinition("CMAKE_CXX_COMPILER_ID"), "Clang") == 0)) {
    this->UsingGCCOnWindows = true;
  }
#endif
}

// Implemented by:
//   cmGlobalUnixMakefileGenerator3
//   cmGlobalGhsMultiGenerator
//   cmGlobalVisualStudio10Generator
//   cmGlobalVisualStudio7Generator
//   cmGlobalXCodeGenerator
// Called by:
//   cmGlobalGenerator::Build()
void cmGlobalNinjaGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& /*projectName*/, const std::string& /*projectDir*/,
  const std::string& targetName, const std::string& /*config*/, bool /*fast*/,
  bool verbose, std::vector<std::string> const& makeOptions)
{
  makeCommand.push_back(this->SelectMakeProgram(makeProgram));

  if (verbose) {
    makeCommand.push_back("-v");
  }

  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
  if (!targetName.empty()) {
    if (targetName == "clean") {
      makeCommand.push_back("-t");
      makeCommand.push_back("clean");
    } else {
      makeCommand.push_back(targetName);
    }
  }
}

// Non-virtual public methods.

void cmGlobalNinjaGenerator::AddRule(
  const std::string& name, const std::string& command,
  const std::string& description, const std::string& comment,
  const std::string& depfile, const std::string& deptype,
  const std::string& rspfile, const std::string& rspcontent,
  const std::string& restat, bool generator)
{
  // Do not add the same rule twice.
  if (this->HasRule(name)) {
    return;
  }

  this->Rules.insert(name);
  cmGlobalNinjaGenerator::WriteRule(*this->RulesFileStream, name, command,
                                    description, comment, depfile, deptype,
                                    rspfile, rspcontent, restat, generator);

  this->RuleCmdLength[name] = (int)command.size();
}

bool cmGlobalNinjaGenerator::HasRule(const std::string& name)
{
  RulesSetType::const_iterator rule = this->Rules.find(name);
  return (rule != this->Rules.end());
}

// Private virtual overrides

std::string cmGlobalNinjaGenerator::GetEditCacheCommand() const
{
  // Ninja by design does not run interactive tools in the terminal,
  // so our only choice is cmake-gui.
  return cmSystemTools::GetCMakeGUICommand();
}

void cmGlobalNinjaGenerator::ComputeTargetObjectDirectory(
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

// Private methods

void cmGlobalNinjaGenerator::OpenBuildFileStream()
{
  // Compute Ninja's build file path.
  std::string buildFilePath =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  buildFilePath += "/";
  buildFilePath += cmGlobalNinjaGenerator::NINJA_BUILD_FILE;

  // Get a stream where to generate things.
  if (!this->BuildFileStream) {
    this->BuildFileStream = new cmGeneratedFileStream(
      buildFilePath.c_str(), false, this->GetMakefileEncoding());
    if (!this->BuildFileStream) {
      // An error message is generated by the constructor if it cannot
      // open the file.
      return;
    }
  }

  // Write the do not edit header.
  this->WriteDisclaimer(*this->BuildFileStream);

  // Write a comment about this file.
  *this->BuildFileStream
    << "# This file contains all the build statements describing the\n"
    << "# compilation DAG.\n\n";
}

void cmGlobalNinjaGenerator::CloseBuildFileStream()
{
  if (this->BuildFileStream) {
    delete this->BuildFileStream;
    this->BuildFileStream = CM_NULLPTR;
  } else {
    cmSystemTools::Error("Build file stream was not open.");
  }
}

void cmGlobalNinjaGenerator::OpenRulesFileStream()
{
  // Compute Ninja's build file path.
  std::string rulesFilePath =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  rulesFilePath += "/";
  rulesFilePath += cmGlobalNinjaGenerator::NINJA_RULES_FILE;

  // Get a stream where to generate things.
  if (!this->RulesFileStream) {
    this->RulesFileStream = new cmGeneratedFileStream(
      rulesFilePath.c_str(), false, this->GetMakefileEncoding());
    if (!this->RulesFileStream) {
      // An error message is generated by the constructor if it cannot
      // open the file.
      return;
    }
  }

  // Write the do not edit header.
  this->WriteDisclaimer(*this->RulesFileStream);

  // Write comment about this file.
  /* clang-format off */
  *this->RulesFileStream
    << "# This file contains all the rules used to get the outputs files\n"
    << "# built from the input files.\n"
    << "# It is included in the main '" << NINJA_BUILD_FILE << "'.\n\n"
    ;
  /* clang-format on */
}

void cmGlobalNinjaGenerator::CloseRulesFileStream()
{
  if (this->RulesFileStream) {
    delete this->RulesFileStream;
    this->RulesFileStream = CM_NULLPTR;
  } else {
    cmSystemTools::Error("Rules file stream was not open.");
  }
}

static void EnsureTrailingSlash(std::string& path)
{
  if (path.empty()) {
    return;
  }
  std::string::value_type last = path[path.size() - 1];
#ifdef _WIN32
  if (last != '\\') {
    path += '\\';
  }
#else
  if (last != '/') {
    path += '/';
  }
#endif
}

std::string cmGlobalNinjaGenerator::ConvertToNinjaPath(
  const std::string& path) const
{
  cmLocalNinjaGenerator* ng =
    static_cast<cmLocalNinjaGenerator*>(this->LocalGenerators[0]);
  std::string convPath = ng->ConvertToRelativePath(
    this->LocalGenerators[0]->GetState()->GetBinaryDirectory(), path);
  convPath = this->NinjaOutputPath(convPath);
#ifdef _WIN32
  std::replace(convPath.begin(), convPath.end(), '/', '\\');
#endif
  return convPath;
}

void cmGlobalNinjaGenerator::AddCXXCompileCommand(
  const std::string& commandLine, const std::string& sourceFile)
{
  // Compute Ninja's build file path.
  std::string buildFileDir =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  if (!this->CompileCommandsStream) {
    std::string buildFilePath = buildFileDir + "/compile_commands.json";
    if (this->ComputingUnknownDependencies) {
      this->CombinedBuildOutputs.insert(
        this->NinjaOutputPath("compile_commands.json"));
    }

    // Get a stream where to generate things.
    this->CompileCommandsStream =
      new cmGeneratedFileStream(buildFilePath.c_str());
    *this->CompileCommandsStream << "[";
  } else {
    *this->CompileCommandsStream << "," << std::endl;
  }

  std::string sourceFileName = sourceFile;
  if (!cmSystemTools::FileIsFullPath(sourceFileName.c_str())) {
    sourceFileName = cmSystemTools::CollapseFullPath(
      sourceFileName, this->GetCMakeInstance()->GetHomeOutputDirectory());
  }

  /* clang-format off */
  *this->CompileCommandsStream << "\n{\n"
     << "  \"directory\": \""
     << cmGlobalGenerator::EscapeJSON(buildFileDir) << "\",\n"
     << "  \"command\": \""
     << cmGlobalGenerator::EscapeJSON(commandLine) << "\",\n"
     << "  \"file\": \""
     << cmGlobalGenerator::EscapeJSON(sourceFileName) << "\"\n"
     << "}";
  /* clang-format on */
}

void cmGlobalNinjaGenerator::CloseCompileCommandsStream()
{
  if (this->CompileCommandsStream) {
    *this->CompileCommandsStream << "\n]";
    delete this->CompileCommandsStream;
    this->CompileCommandsStream = CM_NULLPTR;
  }
}

void cmGlobalNinjaGenerator::WriteDisclaimer(std::ostream& os)
{
  os << "# CMAKE generated file: DO NOT EDIT!\n"
     << "# Generated by \"" << this->GetName() << "\""
     << " Generator, CMake Version " << cmVersion::GetMajorVersion() << "."
     << cmVersion::GetMinorVersion() << "\n\n";
}

void cmGlobalNinjaGenerator::AddDependencyToAll(cmGeneratorTarget* target)
{
  this->AppendTargetOutputs(target, this->AllDependencies);
}

void cmGlobalNinjaGenerator::AddDependencyToAll(const std::string& input)
{
  this->AllDependencies.push_back(input);
}

void cmGlobalNinjaGenerator::WriteAssumedSourceDependencies()
{
  for (std::map<std::string, std::set<std::string> >::iterator i =
         this->AssumedSourceDependencies.begin();
       i != this->AssumedSourceDependencies.end(); ++i) {
    cmNinjaDeps deps;
    std::copy(i->second.begin(), i->second.end(), std::back_inserter(deps));
    WriteCustomCommandBuild(/*command=*/"", /*description=*/"",
                            "Assume dependencies for generated source file.",
                            /*depfile*/ "", /*uses_terminal*/ false,
                            /*restat*/ true, cmNinjaDeps(1, i->first), deps);
  }
}

std::string OrderDependsTargetForTarget(cmGeneratorTarget const* target)
{
  return "cmake_object_order_depends_target_" + target->GetName();
}

void cmGlobalNinjaGenerator::AppendTargetOutputs(
  cmGeneratorTarget const* target, cmNinjaDeps& outputs,
  cmNinjaTargetDepends depends)
{
  std::string configName =
    target->Target->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");

  // for frameworks, we want the real name, not smple name
  // frameworks always appear versioned, and the build.ninja
  // will always attempt to manage symbolic links instead
  // of letting cmOSXBundleGenerator do it.
  bool realname = target->IsFrameworkOnApple();

  switch (target->GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY: {
      if (depends == DependOnTargetOrdering) {
        outputs.push_back(OrderDependsTargetForTarget(target));
        break;
      }
    }
    // FALLTHROUGH
    case cmStateEnums::EXECUTABLE: {
      outputs.push_back(this->ConvertToNinjaPath(target->GetFullPath(
        configName, cmStateEnums::RuntimeBinaryArtifact, realname)));
      break;
    }
    case cmStateEnums::OBJECT_LIBRARY: {
      if (depends == DependOnTargetOrdering) {
        outputs.push_back(OrderDependsTargetForTarget(target));
        break;
      }
    }
    // FALLTHROUGH
    case cmStateEnums::GLOBAL_TARGET:
    case cmStateEnums::UTILITY: {
      std::string path =
        target->GetLocalGenerator()->GetCurrentBinaryDirectory() +
        std::string("/") + target->GetName();
      outputs.push_back(this->ConvertToNinjaPath(path));
      break;
    }

    default:
      return;
  }
}

void cmGlobalNinjaGenerator::AppendTargetDepends(
  cmGeneratorTarget const* target, cmNinjaDeps& outputs,
  cmNinjaTargetDepends depends)
{
  if (target->GetType() == cmStateEnums::GLOBAL_TARGET) {
    // These depend only on other CMake-provided targets, e.g. "all".
    std::set<std::string> const& utils = target->GetUtilities();
    for (std::set<std::string>::const_iterator i = utils.begin();
         i != utils.end(); ++i) {
      std::string d =
        target->GetLocalGenerator()->GetCurrentBinaryDirectory() +
        std::string("/") + *i;
      outputs.push_back(this->ConvertToNinjaPath(d));
    }
  } else {
    cmNinjaDeps outs;
    cmTargetDependSet const& targetDeps = this->GetTargetDirectDepends(target);
    for (cmTargetDependSet::const_iterator i = targetDeps.begin();
         i != targetDeps.end(); ++i) {
      if ((*i)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        continue;
      }
      this->AppendTargetOutputs(*i, outs, depends);
    }
    std::sort(outs.begin(), outs.end());
    outputs.insert(outputs.end(), outs.begin(), outs.end());
  }
}

void cmGlobalNinjaGenerator::AppendTargetDependsClosure(
  cmGeneratorTarget const* target, cmNinjaDeps& outputs)
{
  TargetDependsClosureMap::iterator i =
    this->TargetDependsClosures.find(target);
  if (i == this->TargetDependsClosures.end()) {
    TargetDependsClosureMap::value_type e(
      target, std::set<cmGeneratorTarget const*>());
    i = this->TargetDependsClosures.insert(e).first;
    this->ComputeTargetDependsClosure(target, i->second);
  }
  std::set<cmGeneratorTarget const*> const& targets = i->second;
  cmNinjaDeps outs;
  for (std::set<cmGeneratorTarget const*>::const_iterator ti = targets.begin();
       ti != targets.end(); ++ti) {
    this->AppendTargetOutputs(*ti, outs);
  }
  std::sort(outs.begin(), outs.end());
  outputs.insert(outputs.end(), outs.begin(), outs.end());
}

void cmGlobalNinjaGenerator::ComputeTargetDependsClosure(
  cmGeneratorTarget const* target, std::set<cmGeneratorTarget const*>& depends)
{
  cmTargetDependSet const& targetDeps = this->GetTargetDirectDepends(target);
  for (cmTargetDependSet::const_iterator i = targetDeps.begin();
       i != targetDeps.end(); ++i) {
    if ((*i)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    if (depends.insert(*i).second) {
      this->ComputeTargetDependsClosure(*i, depends);
    }
  }
}

void cmGlobalNinjaGenerator::AddTargetAlias(const std::string& alias,
                                            cmGeneratorTarget* target)
{
  std::string buildAlias = this->NinjaOutputPath(alias);
  cmNinjaDeps outputs;
  this->AppendTargetOutputs(target, outputs);
  // Mark the target's outputs as ambiguous to ensure that no other target uses
  // the output as an alias.
  for (cmNinjaDeps::iterator i = outputs.begin(); i != outputs.end(); ++i) {
    TargetAliases[*i] = CM_NULLPTR;
  }

  // Insert the alias into the map.  If the alias was already present in the
  // map and referred to another target, mark it as ambiguous.
  std::pair<TargetAliasMap::iterator, bool> newAlias =
    TargetAliases.insert(std::make_pair(buildAlias, target));
  if (newAlias.second && newAlias.first->second != target) {
    newAlias.first->second = CM_NULLPTR;
  }
}

void cmGlobalNinjaGenerator::WriteTargetAliases(std::ostream& os)
{
  cmGlobalNinjaGenerator::WriteDivider(os);
  os << "# Target aliases.\n\n";

  for (TargetAliasMap::const_iterator i = TargetAliases.begin();
       i != TargetAliases.end(); ++i) {
    // Don't write ambiguous aliases.
    if (!i->second) {
      continue;
    }

    cmNinjaDeps deps;
    this->AppendTargetOutputs(i->second, deps);

    this->WritePhonyBuild(os, "", cmNinjaDeps(1, i->first), deps);
  }
}

void cmGlobalNinjaGenerator::WriteFolderTargets(std::ostream& os)
{
  cmGlobalNinjaGenerator::WriteDivider(os);
  os << "# Folder targets.\n\n";

  std::map<std::string, cmNinjaDeps> targetsPerFolder;
  for (std::vector<cmLocalGenerator*>::const_iterator lgi =
         this->LocalGenerators.begin();
       lgi != this->LocalGenerators.end(); ++lgi) {
    cmLocalGenerator const* lg = *lgi;
    const std::string currentBinaryFolder(
      lg->GetStateSnapshot().GetDirectory().GetCurrentBinary());
    // The directory-level rule should depend on the target-level rules
    // for all targets in the directory.
    targetsPerFolder[currentBinaryFolder] = cmNinjaDeps();
    for (std::vector<cmGeneratorTarget*>::const_iterator ti =
           lg->GetGeneratorTargets().begin();
         ti != lg->GetGeneratorTargets().end(); ++ti) {
      cmGeneratorTarget const* gt = *ti;
      cmStateEnums::TargetType const type = gt->GetType();
      if ((type == cmStateEnums::EXECUTABLE ||
           type == cmStateEnums::STATIC_LIBRARY ||
           type == cmStateEnums::SHARED_LIBRARY ||
           type == cmStateEnums::MODULE_LIBRARY ||
           type == cmStateEnums::OBJECT_LIBRARY ||
           type == cmStateEnums::UTILITY) &&
          !gt->GetPropertyAsBool("EXCLUDE_FROM_ALL")) {
        targetsPerFolder[currentBinaryFolder].push_back(gt->GetName());
      }
    }

    // The directory-level rule should depend on the directory-level
    // rules of the subdirectories.
    std::vector<cmStateSnapshot> const& children =
      lg->GetStateSnapshot().GetChildren();
    for (std::vector<cmStateSnapshot>::const_iterator stateIt =
           children.begin();
         stateIt != children.end(); ++stateIt) {
      std::string const currentBinaryDir =
        stateIt->GetDirectory().GetCurrentBinary();

      targetsPerFolder[currentBinaryFolder].push_back(
        this->ConvertToNinjaPath(currentBinaryDir + "/all"));
    }
  }

  std::string const rootBinaryDir =
    this->LocalGenerators[0]->GetBinaryDirectory();
  for (std::map<std::string, cmNinjaDeps>::const_iterator it =
         targetsPerFolder.begin();
       it != targetsPerFolder.end(); ++it) {
    cmGlobalNinjaGenerator::WriteDivider(os);
    std::string const& currentBinaryDir = it->first;

    // Do not generate a rule for the root binary dir.
    if (rootBinaryDir.length() >= currentBinaryDir.length()) {
      continue;
    }

    std::string const comment = "Folder: " + currentBinaryDir;
    cmNinjaDeps output(1);
    output.push_back(this->ConvertToNinjaPath(currentBinaryDir + "/all"));

    this->WritePhonyBuild(os, comment, output, it->second);
  }
}

void cmGlobalNinjaGenerator::WriteUnknownExplicitDependencies(std::ostream& os)
{
  if (!this->ComputingUnknownDependencies) {
    return;
  }

  // We need to collect the set of known build outputs.
  // Start with those generated by WriteBuild calls.
  // No other method needs this so we can take ownership
  // of the set locally and throw it out when we are done.
  std::set<std::string> knownDependencies;
  knownDependencies.swap(this->CombinedBuildOutputs);

  // now write out the unknown explicit dependencies.

  // union the configured files, evaluations files and the
  // CombinedBuildOutputs,
  // and then difference with CombinedExplicitDependencies to find the explicit
  // dependencies that we have no rule for

  cmGlobalNinjaGenerator::WriteDivider(os);
  /* clang-format off */
  os << "# Unknown Build Time Dependencies.\n"
     << "# Tell Ninja that they may appear as side effects of build rules\n"
     << "# otherwise ordered by order-only dependencies.\n\n";
  /* clang-format on */

  // get the list of files that cmake itself has generated as a
  // product of configuration.

  for (std::vector<cmLocalGenerator*>::const_iterator i =
         this->LocalGenerators.begin();
       i != this->LocalGenerators.end(); ++i) {
    // get the vector of files created by this makefile and convert them
    // to ninja paths, which are all relative in respect to the build directory
    const std::vector<std::string>& files =
      (*i)->GetMakefile()->GetOutputFiles();
    typedef std::vector<std::string>::const_iterator vect_it;
    for (vect_it j = files.begin(); j != files.end(); ++j) {
      knownDependencies.insert(this->ConvertToNinjaPath(*j));
    }
    // get list files which are implicit dependencies as well and will be phony
    // for rebuild manifest
    std::vector<std::string> const& lf = (*i)->GetMakefile()->GetListFiles();
    typedef std::vector<std::string>::const_iterator vect_it;
    for (vect_it j = lf.begin(); j != lf.end(); ++j) {
      knownDependencies.insert(this->ConvertToNinjaPath(*j));
    }
    std::vector<cmGeneratorExpressionEvaluationFile*> const& ef =
      (*i)->GetMakefile()->GetEvaluationFiles();
    for (std::vector<cmGeneratorExpressionEvaluationFile*>::const_iterator li =
           ef.begin();
         li != ef.end(); ++li) {
      // get all the files created by generator expressions and convert them
      // to ninja paths
      std::vector<std::string> evaluationFiles = (*li)->GetFiles();
      for (vect_it j = evaluationFiles.begin(); j != evaluationFiles.end();
           ++j) {
        knownDependencies.insert(this->ConvertToNinjaPath(*j));
      }
    }
  }
  knownDependencies.insert(this->CMakeCacheFile);

  for (TargetAliasMap::const_iterator i = this->TargetAliases.begin();
       i != this->TargetAliases.end(); ++i) {
    knownDependencies.insert(this->ConvertToNinjaPath(i->first));
  }

  // remove all source files we know will exist.
  typedef std::map<std::string, std::set<std::string> >::const_iterator map_it;
  for (map_it i = this->AssumedSourceDependencies.begin();
       i != this->AssumedSourceDependencies.end(); ++i) {
    knownDependencies.insert(this->ConvertToNinjaPath(i->first));
  }

  // now we difference with CombinedCustomCommandExplicitDependencies to find
  // the list of items we know nothing about.
  // We have encoded all the paths in CombinedCustomCommandExplicitDependencies
  // and knownDependencies so no matter if unix or windows paths they
  // should all match now.

  std::vector<std::string> unknownExplicitDepends;
  this->CombinedCustomCommandExplicitDependencies.erase(this->TargetAll);

  std::set_difference(this->CombinedCustomCommandExplicitDependencies.begin(),
                      this->CombinedCustomCommandExplicitDependencies.end(),
                      knownDependencies.begin(), knownDependencies.end(),
                      std::back_inserter(unknownExplicitDepends));

  std::string const rootBuildDirectory =
    this->GetCMakeInstance()->GetHomeOutputDirectory();
  bool const inSourceBuild =
    (rootBuildDirectory == this->GetCMakeInstance()->GetHomeDirectory());
  std::vector<std::string> warnExplicitDepends;
  for (std::vector<std::string>::const_iterator i =
         unknownExplicitDepends.begin();
       i != unknownExplicitDepends.end(); ++i) {
    // verify the file is in the build directory
    std::string const absDepPath =
      cmSystemTools::CollapseFullPath(*i, rootBuildDirectory.c_str());
    bool const inBuildDir =
      cmSystemTools::IsSubDirectory(absDepPath, rootBuildDirectory);
    if (inBuildDir) {
      cmNinjaDeps deps(1, *i);
      this->WritePhonyBuild(os, "", deps, cmNinjaDeps());
      if (this->PolicyCMP0058 == cmPolicies::WARN && !inSourceBuild &&
          warnExplicitDepends.size() < 10) {
        warnExplicitDepends.push_back(*i);
      }
    }
  }

  if (!warnExplicitDepends.empty()) {
    std::ostringstream w;
    /* clang-format off */
    w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0058) << "\n"
      "This project specifies custom command DEPENDS on files "
      "in the build tree that are not specified as the OUTPUT or "
      "BYPRODUCTS of any add_custom_command or add_custom_target:\n"
      " " << cmJoin(warnExplicitDepends, "\n ") <<
      "\n"
      "For compatibility with versions of CMake that did not have "
      "the BYPRODUCTS option, CMake is generating phony rules for "
      "such files to convince 'ninja' to build."
      "\n"
      "Project authors should add the missing BYPRODUCTS or OUTPUT "
      "options to the custom commands that produce these files."
      ;
    /* clang-format on */
    this->GetCMakeInstance()->IssueMessage(cmake::AUTHOR_WARNING, w.str());
  }
}

void cmGlobalNinjaGenerator::WriteBuiltinTargets(std::ostream& os)
{
  // Write headers.
  cmGlobalNinjaGenerator::WriteDivider(os);
  os << "# Built-in targets\n\n";

  this->WriteTargetAll(os);
  this->WriteTargetRebuildManifest(os);
  this->WriteTargetClean(os);
  this->WriteTargetHelp(os);
}

void cmGlobalNinjaGenerator::WriteTargetAll(std::ostream& os)
{
  cmNinjaDeps outputs;
  outputs.push_back(this->TargetAll);

  this->WritePhonyBuild(os, "The main all target.", outputs,
                        this->AllDependencies);

  if (!this->HasOutputPathPrefix()) {
    cmGlobalNinjaGenerator::WriteDefault(os, outputs,
                                         "Make the all target the default.");
  }
}

void cmGlobalNinjaGenerator::WriteTargetRebuildManifest(std::ostream& os)
{
  cmLocalGenerator* lg = this->LocalGenerators[0];

  std::ostringstream cmd;
  cmd << lg->ConvertToOutputFormat(cmSystemTools::GetCMakeCommand(),
                                   cmOutputConverter::SHELL)
      << " -H"
      << lg->ConvertToOutputFormat(lg->GetSourceDirectory(),
                                   cmOutputConverter::SHELL)
      << " -B"
      << lg->ConvertToOutputFormat(lg->GetBinaryDirectory(),
                                   cmOutputConverter::SHELL);
  WriteRule(*this->RulesFileStream, "RERUN_CMAKE", cmd.str(),
            "Re-running CMake...", "Rule for re-running cmake.",
            /*depfile=*/"",
            /*deptype=*/"",
            /*rspfile=*/"",
            /*rspcontent*/ "",
            /*restat=*/"",
            /*generator=*/true);

  cmNinjaDeps implicitDeps;
  for (std::vector<cmLocalGenerator*>::const_iterator i =
         this->LocalGenerators.begin();
       i != this->LocalGenerators.end(); ++i) {
    std::vector<std::string> const& lf = (*i)->GetMakefile()->GetListFiles();
    for (std::vector<std::string>::const_iterator fi = lf.begin();
         fi != lf.end(); ++fi) {
      implicitDeps.push_back(this->ConvertToNinjaPath(*fi));
    }
  }
  implicitDeps.push_back(this->CMakeCacheFile);

  std::sort(implicitDeps.begin(), implicitDeps.end());
  implicitDeps.erase(std::unique(implicitDeps.begin(), implicitDeps.end()),
                     implicitDeps.end());

  cmNinjaVars variables;
  // Use 'console' pool to get non buffered output of the CMake re-run call
  // Available since Ninja 1.5
  if (SupportsConsolePool()) {
    variables["pool"] = "console";
  }

  std::string const ninjaBuildFile = this->NinjaOutputPath(NINJA_BUILD_FILE);
  this->WriteBuild(os, "Re-run CMake if any of its inputs changed.",
                   "RERUN_CMAKE",
                   /*outputs=*/cmNinjaDeps(1, ninjaBuildFile),
                   /*implicitOuts=*/cmNinjaDeps(),
                   /*explicitDeps=*/cmNinjaDeps(), implicitDeps,
                   /*orderOnlyDeps=*/cmNinjaDeps(), variables);

  this->WritePhonyBuild(os, "A missing CMake input file is not an error.",
                        implicitDeps, cmNinjaDeps());
}

std::string cmGlobalNinjaGenerator::ninjaCmd() const
{
  cmLocalGenerator* lgen = this->LocalGenerators[0];
  if (lgen) {
    return lgen->ConvertToOutputFormat(this->NinjaCommand,
                                       cmOutputConverter::SHELL);
  }
  return "ninja";
}

bool cmGlobalNinjaGenerator::SupportsConsolePool() const
{
  return this->NinjaSupportsConsolePool;
}

bool cmGlobalNinjaGenerator::SupportsImplicitOuts() const
{
  return this->NinjaSupportsImplicitOuts;
}

void cmGlobalNinjaGenerator::WriteTargetClean(std::ostream& os)
{
  WriteRule(*this->RulesFileStream, "CLEAN", ninjaCmd() + " -t clean",
            "Cleaning all built files...",
            "Rule for cleaning all built files.",
            /*depfile=*/"",
            /*deptype=*/"",
            /*rspfile=*/"",
            /*rspcontent*/ "",
            /*restat=*/"",
            /*generator=*/false);
  WriteBuild(os, "Clean all the built files.", "CLEAN",
             /*outputs=*/cmNinjaDeps(1, this->NinjaOutputPath("clean")),
             /*implicitOuts=*/cmNinjaDeps(),
             /*explicitDeps=*/cmNinjaDeps(),
             /*implicitDeps=*/cmNinjaDeps(),
             /*orderOnlyDeps=*/cmNinjaDeps(),
             /*variables=*/cmNinjaVars());
}

void cmGlobalNinjaGenerator::WriteTargetHelp(std::ostream& os)
{
  WriteRule(*this->RulesFileStream, "HELP", ninjaCmd() + " -t targets",
            "All primary targets available:",
            "Rule for printing all primary targets available.",
            /*depfile=*/"",
            /*deptype=*/"",
            /*rspfile=*/"",
            /*rspcontent*/ "",
            /*restat=*/"",
            /*generator=*/false);
  WriteBuild(os, "Print all primary targets available.", "HELP",
             /*outputs=*/cmNinjaDeps(1, this->NinjaOutputPath("help")),
             /*implicitOuts=*/cmNinjaDeps(),
             /*explicitDeps=*/cmNinjaDeps(),
             /*implicitDeps=*/cmNinjaDeps(),
             /*orderOnlyDeps=*/cmNinjaDeps(),
             /*variables=*/cmNinjaVars());
}

void cmGlobalNinjaGenerator::InitOutputPathPrefix()
{
  this->OutputPathPrefix =
    this->LocalGenerators[0]->GetMakefile()->GetSafeDefinition(
      "CMAKE_NINJA_OUTPUT_PATH_PREFIX");
  EnsureTrailingSlash(this->OutputPathPrefix);
}

std::string cmGlobalNinjaGenerator::NinjaOutputPath(
  std::string const& path) const
{
  if (!this->HasOutputPathPrefix() || cmSystemTools::FileIsFullPath(path)) {
    return path;
  }
  return this->OutputPathPrefix + path;
}

void cmGlobalNinjaGenerator::StripNinjaOutputPathPrefixAsSuffix(
  std::string& path)
{
  if (path.empty()) {
    return;
  }
  EnsureTrailingSlash(path);
  cmStripSuffixIfExists(path, this->OutputPathPrefix);
}

/*

We use the following approach to support Fortran.  Each target already
has a <target>.dir/ directory used to hold intermediate files for CMake.
For each target, a FortranDependInfo.json file is generated by CMake with
information about include directories, module directories, and the locations
the per-target directories for target dependencies.

Compilation of source files within a target is split into the following steps:

1. Preprocess all sources, scan preprocessed output for module dependencies.
   This step is done with independent build statements for each source,
   and can therefore be done in parallel.

    rule Fortran_PREPROCESS
      depfile = $DEP_FILE
      command = gfortran -cpp $DEFINES $INCLUDES $FLAGS -E $in -o $out &&
                cmake -E cmake_ninja_depends \
                  --tdi=FortranDependInfo.json --pp=$out --dep=$DEP_FILE \
                  --obj=$OBJ_FILE --ddi=$DYNDEP_INTERMEDIATE_FILE

    build src.f90-pp.f90 | src.f90-pp.f90.ddi: Fortran_PREPROCESS src.f90
      OBJ_FILE = src.f90.o
      DEP_FILE = src.f90-pp.f90.d
      DYNDEP_INTERMEDIATE_FILE = src.f90-pp.f90.ddi

   The ``cmake -E cmake_ninja_depends`` tool reads the preprocessed output
   and generates the ninja depfile for preprocessor dependencies.  It also
   generates a "ddi" file (in a format private to CMake) that lists the
   object file that compilation will produce along with the module names
   it provides and/or requires.  The "ddi" file is an implicit output
   because it should not appear in "$out" but is generated by the rule.

2. Consolidate the per-source module dependencies saved in the "ddi"
   files from all sources to produce a ninja "dyndep" file, ``Fortran.dd``.

    rule Fortran_DYNDEP
      command = cmake -E cmake_ninja_dyndep \
                  --tdi=FortranDependInfo.json --dd=$out $in

    build Fortran.dd: Fortran_DYNDEP src1.f90-pp.f90.ddi src2.f90-pp.f90.ddi

   The ``cmake -E cmake_ninja_dyndep`` tool reads the "ddi" files from all
   sources in the target and the ``FortranModules.json`` files from targets
   on which the target depends.  It computes dependency edges on compilations
   that require modules to those that provide the modules.  This information
   is placed in the ``Fortran.dd`` file for ninja to load later.  It also
   writes the expected location of modules provided by this target into
   ``FortranModules.json`` for use by dependent targets.

3. Compile all sources after loading dynamically discovered dependencies
   of the compilation build statements from their ``dyndep`` bindings.

    rule Fortran_COMPILE
      command = gfortran $INCLUDES $FLAGS -c $in -o $out

    build src1.f90.o: Fortran_COMPILE src1.f90-pp.f90 || Fortran.dd
      dyndep = Fortran.dd

   The "dyndep" binding tells ninja to load dynamically discovered
   dependency information from ``Fortran.dd``.  This adds information
   such as:

    build src1.f90.o | mod1.mod: dyndep
      restat = 1

   This tells ninja that ``mod1.mod`` is an implicit output of compiling
   the object file ``src1.f90.o``.  The ``restat`` binding tells it that
   the timestamp of the output may not always change.  Additionally:

    build src2.f90.o: dyndep | mod1.mod

   This tells ninja that ``mod1.mod`` is a dependency of compiling the
   object file ``src2.f90.o``.  This ensures that ``src1.f90.o`` and
   ``mod1.mod`` will always be up to date before ``src2.f90.o`` is built
   (because the latter consumes the module).
*/

int cmcmd_cmake_ninja_depends(std::vector<std::string>::const_iterator argBeg,
                              std::vector<std::string>::const_iterator argEnd)
{
  std::string arg_tdi;
  std::string arg_pp;
  std::string arg_dep;
  std::string arg_obj;
  std::string arg_ddi;
  for (std::vector<std::string>::const_iterator a = argBeg; a != argEnd; ++a) {
    std::string const& arg = *a;
    if (cmHasLiteralPrefix(arg, "--tdi=")) {
      arg_tdi = arg.substr(6);
    } else if (cmHasLiteralPrefix(arg, "--pp=")) {
      arg_pp = arg.substr(5);
    } else if (cmHasLiteralPrefix(arg, "--dep=")) {
      arg_dep = arg.substr(6);
    } else if (cmHasLiteralPrefix(arg, "--obj=")) {
      arg_obj = arg.substr(6);
    } else if (cmHasLiteralPrefix(arg, "--ddi=")) {
      arg_ddi = arg.substr(6);
    } else {
      cmSystemTools::Error("-E cmake_ninja_depends unknown argument: ",
                           arg.c_str());
      return 1;
    }
  }
  if (arg_tdi.empty()) {
    cmSystemTools::Error("-E cmake_ninja_depends requires value for --tdi=");
    return 1;
  }
  if (arg_pp.empty()) {
    cmSystemTools::Error("-E cmake_ninja_depends requires value for --pp=");
    return 1;
  }
  if (arg_dep.empty()) {
    cmSystemTools::Error("-E cmake_ninja_depends requires value for --dep=");
    return 1;
  }
  if (arg_obj.empty()) {
    cmSystemTools::Error("-E cmake_ninja_depends requires value for --obj=");
    return 1;
  }
  if (arg_ddi.empty()) {
    cmSystemTools::Error("-E cmake_ninja_depends requires value for --ddi=");
    return 1;
  }

  std::vector<std::string> includes;
  {
    Json::Value tdio;
    Json::Value const& tdi = tdio;
    {
      cmsys::ifstream tdif(arg_tdi.c_str(), std::ios::in | std::ios::binary);
      Json::Reader reader;
      if (!reader.parse(tdif, tdio, false)) {
        cmSystemTools::Error("-E cmake_ninja_depends failed to parse ",
                             arg_tdi.c_str(),
                             reader.getFormattedErrorMessages().c_str());
        return 1;
      }
    }

    Json::Value const& tdi_include_dirs = tdi["include-dirs"];
    if (tdi_include_dirs.isArray()) {
      for (Json::Value::const_iterator i = tdi_include_dirs.begin();
           i != tdi_include_dirs.end(); ++i) {
        includes.push_back(i->asString());
      }
    }
  }

  cmFortranSourceInfo info;
  std::set<std::string> defines;
  cmFortranParser parser(includes, defines, info);
  if (!cmFortranParser_FilePush(&parser, arg_pp.c_str())) {
    cmSystemTools::Error("-E cmake_ninja_depends failed to open ",
                         arg_pp.c_str());
    return 1;
  }
  if (cmFortran_yyparse(parser.Scanner) != 0) {
    // Failed to parse the file.
    return 1;
  }

  {
    cmGeneratedFileStream depfile(arg_dep.c_str());
    depfile << cmSystemTools::ConvertToUnixOutputPath(arg_pp) << ":";
    for (std::set<std::string>::iterator i = info.Includes.begin();
         i != info.Includes.end(); ++i) {
      depfile << " \\\n " << cmSystemTools::ConvertToUnixOutputPath(*i);
    }
    depfile << "\n";
  }

  Json::Value ddi(Json::objectValue);
  ddi["object"] = arg_obj;

  Json::Value& ddi_provides = ddi["provides"] = Json::arrayValue;
  for (std::set<std::string>::iterator i = info.Provides.begin();
       i != info.Provides.end(); ++i) {
    ddi_provides.append(*i);
  }
  Json::Value& ddi_requires = ddi["requires"] = Json::arrayValue;
  for (std::set<std::string>::iterator i = info.Requires.begin();
       i != info.Requires.end(); ++i) {
    // Require modules not provided in the same source.
    if (!info.Provides.count(*i)) {
      ddi_requires.append(*i);
    }
  }

  cmGeneratedFileStream ddif(arg_ddi.c_str());
  ddif << ddi;
  if (!ddif) {
    cmSystemTools::Error("-E cmake_ninja_depends failed to write ",
                         arg_ddi.c_str());
    return 1;
  }
  return 0;
}

struct cmFortranObjectInfo
{
  std::string Object;
  std::vector<std::string> Provides;
  std::vector<std::string> Requires;
};

bool cmGlobalNinjaGenerator::WriteDyndepFile(
  std::string const& dir_top_src, std::string const& dir_top_bld,
  std::string const& dir_cur_src, std::string const& dir_cur_bld,
  std::string const& arg_dd, std::vector<std::string> const& arg_ddis,
  std::string const& module_dir,
  std::vector<std::string> const& linked_target_dirs)
{
  // Setup path conversions.
  {
    cmStateSnapshot snapshot = this->GetCMakeInstance()->GetCurrentSnapshot();
    snapshot.GetDirectory().SetCurrentSource(dir_cur_src);
    snapshot.GetDirectory().SetCurrentBinary(dir_cur_bld);
    snapshot.GetDirectory().SetRelativePathTopSource(dir_top_src.c_str());
    snapshot.GetDirectory().SetRelativePathTopBinary(dir_top_bld.c_str());
    CM_AUTO_PTR<cmMakefile> mfd(new cmMakefile(this, snapshot));
    CM_AUTO_PTR<cmLocalNinjaGenerator> lgd(static_cast<cmLocalNinjaGenerator*>(
      this->CreateLocalGenerator(mfd.get())));
    this->Makefiles.push_back(mfd.release());
    this->LocalGenerators.push_back(lgd.release());
  }

  std::vector<cmFortranObjectInfo> objects;
  for (std::vector<std::string>::const_iterator ddii = arg_ddis.begin();
       ddii != arg_ddis.end(); ++ddii) {
    // Load the ddi file and compute the module file paths it provides.
    Json::Value ddio;
    Json::Value const& ddi = ddio;
    cmsys::ifstream ddif(ddii->c_str(), std::ios::in | std::ios::binary);
    Json::Reader reader;
    if (!reader.parse(ddif, ddio, false)) {
      cmSystemTools::Error("-E cmake_ninja_dyndep failed to parse ",
                           ddii->c_str(),
                           reader.getFormattedErrorMessages().c_str());
      return false;
    }

    cmFortranObjectInfo info;
    info.Object = ddi["object"].asString();
    Json::Value const& ddi_provides = ddi["provides"];
    if (ddi_provides.isArray()) {
      for (Json::Value::const_iterator i = ddi_provides.begin();
           i != ddi_provides.end(); ++i) {
        info.Provides.push_back(i->asString());
      }
    }
    Json::Value const& ddi_requires = ddi["requires"];
    if (ddi_requires.isArray()) {
      for (Json::Value::const_iterator i = ddi_requires.begin();
           i != ddi_requires.end(); ++i) {
        info.Requires.push_back(i->asString());
      }
    }
    objects.push_back(info);
  }

  // Map from module name to module file path, if known.
  std::map<std::string, std::string> mod_files;

  // Populate the module map with those provided by linked targets first.
  for (std::vector<std::string>::const_iterator di =
         linked_target_dirs.begin();
       di != linked_target_dirs.end(); ++di) {
    std::string const ltmn = *di + "/FortranModules.json";
    Json::Value ltm;
    cmsys::ifstream ltmf(ltmn.c_str(), std::ios::in | std::ios::binary);
    Json::Reader reader;
    if (ltmf && !reader.parse(ltmf, ltm, false)) {
      cmSystemTools::Error("-E cmake_ninja_dyndep failed to parse ",
                           di->c_str(),
                           reader.getFormattedErrorMessages().c_str());
      return false;
    }
    if (ltm.isObject()) {
      for (Json::Value::iterator i = ltm.begin(); i != ltm.end(); ++i) {
        mod_files[i.key().asString()] = i->asString();
      }
    }
  }

  // Extend the module map with those provided by this target.
  // We do this after loading the modules provided by linked targets
  // in case we have one of the same name that must be preferred.
  Json::Value tm = Json::objectValue;
  for (std::vector<cmFortranObjectInfo>::iterator oi = objects.begin();
       oi != objects.end(); ++oi) {
    for (std::vector<std::string>::iterator i = oi->Provides.begin();
         i != oi->Provides.end(); ++i) {
      std::string const mod = module_dir + *i + ".mod";
      mod_files[*i] = mod;
      tm[*i] = mod;
    }
  }

  cmGeneratedFileStream ddf(arg_dd.c_str());
  ddf << "ninja_dyndep_version = 1.0\n";

  for (std::vector<cmFortranObjectInfo>::iterator oi = objects.begin();
       oi != objects.end(); ++oi) {
    std::string const ddComment;
    std::string const ddRule = "dyndep";
    cmNinjaDeps ddOutputs;
    cmNinjaDeps ddImplicitOuts;
    cmNinjaDeps ddExplicitDeps;
    cmNinjaDeps ddImplicitDeps;
    cmNinjaDeps ddOrderOnlyDeps;
    cmNinjaVars ddVars;

    ddOutputs.push_back(oi->Object);
    for (std::vector<std::string>::iterator i = oi->Provides.begin();
         i != oi->Provides.end(); ++i) {
      ddImplicitOuts.push_back(this->ConvertToNinjaPath(mod_files[*i]));
    }
    for (std::vector<std::string>::iterator i = oi->Requires.begin();
         i != oi->Requires.end(); ++i) {
      std::map<std::string, std::string>::iterator m = mod_files.find(*i);
      if (m != mod_files.end()) {
        ddImplicitDeps.push_back(this->ConvertToNinjaPath(m->second));
      }
    }
    if (!oi->Provides.empty()) {
      ddVars["restat"] = "1";
    }

    this->WriteBuild(ddf, ddComment, ddRule, ddOutputs, ddImplicitOuts,
                     ddExplicitDeps, ddImplicitDeps, ddOrderOnlyDeps, ddVars);
  }

  // Store the map of modules provided by this target in a file for
  // use by dependents that reference this target in linked-target-dirs.
  std::string const target_mods_file =
    cmSystemTools::GetFilenamePath(arg_dd) + "/FortranModules.json";
  cmGeneratedFileStream tmf(target_mods_file.c_str());
  tmf << tm;

  return true;
}

int cmcmd_cmake_ninja_dyndep(std::vector<std::string>::const_iterator argBeg,
                             std::vector<std::string>::const_iterator argEnd)
{
  std::vector<std::string> arg_full =
    cmSystemTools::HandleResponseFile(argBeg, argEnd);

  std::string arg_dd;
  std::string arg_tdi;
  std::vector<std::string> arg_ddis;
  for (std::vector<std::string>::const_iterator a = arg_full.begin();
       a != arg_full.end(); ++a) {
    std::string const& arg = *a;
    if (cmHasLiteralPrefix(arg, "--tdi=")) {
      arg_tdi = arg.substr(6);
    } else if (cmHasLiteralPrefix(arg, "--dd=")) {
      arg_dd = arg.substr(5);
    } else if (!cmHasLiteralPrefix(arg, "--") &&
               cmHasLiteralSuffix(arg, ".ddi")) {
      arg_ddis.push_back(arg);
    } else {
      cmSystemTools::Error("-E cmake_ninja_dyndep unknown argument: ",
                           arg.c_str());
      return 1;
    }
  }
  if (arg_tdi.empty()) {
    cmSystemTools::Error("-E cmake_ninja_dyndep requires value for --tdi=");
    return 1;
  }
  if (arg_dd.empty()) {
    cmSystemTools::Error("-E cmake_ninja_dyndep requires value for --dd=");
    return 1;
  }

  Json::Value tdio;
  Json::Value const& tdi = tdio;
  {
    cmsys::ifstream tdif(arg_tdi.c_str(), std::ios::in | std::ios::binary);
    Json::Reader reader;
    if (!reader.parse(tdif, tdio, false)) {
      cmSystemTools::Error("-E cmake_ninja_dyndep failed to parse ",
                           arg_tdi.c_str(),
                           reader.getFormattedErrorMessages().c_str());
      return 1;
    }
  }

  std::string const dir_cur_bld = tdi["dir-cur-bld"].asString();
  std::string const dir_cur_src = tdi["dir-cur-src"].asString();
  std::string const dir_top_bld = tdi["dir-top-bld"].asString();
  std::string const dir_top_src = tdi["dir-top-src"].asString();
  std::string module_dir = tdi["module-dir"].asString();
  if (!module_dir.empty()) {
    module_dir += "/";
  }
  std::vector<std::string> linked_target_dirs;
  Json::Value const& tdi_linked_target_dirs = tdi["linked-target-dirs"];
  if (tdi_linked_target_dirs.isArray()) {
    for (Json::Value::const_iterator i = tdi_linked_target_dirs.begin();
         i != tdi_linked_target_dirs.end(); ++i) {
      linked_target_dirs.push_back(i->asString());
    }
  }

  cmake cm(cmake::RoleInternal);
  cm.SetHomeDirectory(dir_top_src);
  cm.SetHomeOutputDirectory(dir_top_bld);
  CM_AUTO_PTR<cmGlobalNinjaGenerator> ggd(
    static_cast<cmGlobalNinjaGenerator*>(cm.CreateGlobalGenerator("Ninja")));
  if (!ggd.get() ||
      !ggd->WriteDyndepFile(dir_top_src, dir_top_bld, dir_cur_src, dir_cur_bld,
                            arg_dd, arg_ddis, module_dir,
                            linked_target_dirs)) {
    return 1;
  }
  return 0;
}
