/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmNinjaNormalTargetGenerator.h"

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <map>
#include <set>
#include <sstream>

#include "cmAlgorithms.h"
#include "cmCustomCommand.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalNinjaGenerator.h"
#include "cmLinkLineComputer.h"
#include "cmLinkLineDeviceComputer.h"
#include "cmLocalGenerator.h"
#include "cmLocalNinjaGenerator.h"
#include "cmMakefile.h"
#include "cmNinjaTypes.h"
#include "cmOSXBundleGenerator.h"
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

cmNinjaNormalTargetGenerator::cmNinjaNormalTargetGenerator(
  cmGeneratorTarget* target)
  : cmNinjaTargetGenerator(target)
  , TargetNameOut()
  , TargetNameSO()
  , TargetNameReal()
  , TargetNameImport()
  , TargetNamePDB()
  , TargetLinkLanguage("")
  , DeviceLinkObject()
{
  this->TargetLinkLanguage = target->GetLinkerLanguage(this->GetConfigName());
  if (target->GetType() == cmStateEnums::EXECUTABLE) {
    this->GetGeneratorTarget()->GetExecutableNames(
      this->TargetNameOut, this->TargetNameReal, this->TargetNameImport,
      this->TargetNamePDB, GetLocalGenerator()->GetConfigName());
  } else {
    this->GetGeneratorTarget()->GetLibraryNames(
      this->TargetNameOut, this->TargetNameSO, this->TargetNameReal,
      this->TargetNameImport, this->TargetNamePDB,
      GetLocalGenerator()->GetConfigName());
  }

  if (target->GetType() != cmStateEnums::OBJECT_LIBRARY) {
    // on Windows the output dir is already needed at compile time
    // ensure the directory exists (OutDir test)
    EnsureDirectoryExists(target->GetDirectory(this->GetConfigName()));
  }

  this->OSXBundleGenerator =
    new cmOSXBundleGenerator(target, this->GetConfigName());
  this->OSXBundleGenerator->SetMacContentFolders(&this->MacContentFolders);
}

cmNinjaNormalTargetGenerator::~cmNinjaNormalTargetGenerator()
{
  delete this->OSXBundleGenerator;
}

void cmNinjaNormalTargetGenerator::Generate()
{
  if (this->TargetLinkLanguage.empty()) {
    cmSystemTools::Error("CMake can not determine linker language for "
                         "target: ",
                         this->GetGeneratorTarget()->GetName().c_str());
    return;
  }

  // Write the rules for each language.
  this->WriteLanguagesRules();

  // Write the build statements
  this->WriteObjectBuildStatements();

  if (this->GetGeneratorTarget()->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    this->WriteObjectLibStatement();
  } else {
    // If this target has cuda language link inputs, and we need to do
    // device linking
    this->WriteDeviceLinkStatement();
    this->WriteLinkStatement();
  }
}

void cmNinjaNormalTargetGenerator::WriteLanguagesRules()
{
#ifdef NINJA_GEN_VERBOSE_FILES
  cmGlobalNinjaGenerator::WriteDivider(this->GetRulesFileStream());
  this->GetRulesFileStream()
    << "# Rules for each languages for "
    << cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType())
    << " target " << this->GetTargetName() << "\n\n";
#endif

  // Write rules for languages compiled in this target.
  std::set<std::string> languages;
  std::vector<cmSourceFile const*> sourceFiles;
  this->GetGeneratorTarget()->GetObjectSources(
    sourceFiles, this->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE"));
  for (std::vector<cmSourceFile const*>::const_iterator i =
         sourceFiles.begin();
       i != sourceFiles.end(); ++i) {
    const std::string& lang = (*i)->GetLanguage();
    if (!lang.empty()) {
      languages.insert(lang);
    }
  }
  for (std::set<std::string>::const_iterator l = languages.begin();
       l != languages.end(); ++l) {
    this->WriteLanguageRules(*l);
  }
}

const char* cmNinjaNormalTargetGenerator::GetVisibleTypeName() const
{
  switch (this->GetGeneratorTarget()->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      return "static library";
    case cmStateEnums::SHARED_LIBRARY:
      return "shared library";
    case cmStateEnums::MODULE_LIBRARY:
      if (this->GetGeneratorTarget()->IsCFBundleOnApple()) {
        return "CFBundle shared module";
      } else {
        return "shared module";
      }
    case cmStateEnums::EXECUTABLE:
      return "executable";
    default:
      return CM_NULLPTR;
  }
}

std::string cmNinjaNormalTargetGenerator::LanguageLinkerRule() const
{
  return this->TargetLinkLanguage + "_" +
    cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType()) +
    "_LINKER__" + cmGlobalNinjaGenerator::EncodeRuleName(
                    this->GetGeneratorTarget()->GetName());
}

std::string cmNinjaNormalTargetGenerator::LanguageLinkerDeviceRule() const
{
  return this->TargetLinkLanguage + "_" +
    cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType()) +
    "_DEVICE_LINKER__" + cmGlobalNinjaGenerator::EncodeRuleName(
                           this->GetGeneratorTarget()->GetName());
}

struct cmNinjaRemoveNoOpCommands
{
  bool operator()(std::string const& cmd)
  {
    return cmd.empty() || cmd[0] == ':';
  }
};

void cmNinjaNormalTargetGenerator::WriteDeviceLinkRule(bool useResponseFile)
{
  cmStateEnums::TargetType targetType = this->GetGeneratorTarget()->GetType();
  std::string ruleName = this->LanguageLinkerDeviceRule();
  // Select whether to use a response file for objects.
  std::string rspfile;
  std::string rspcontent;

  if (!this->GetGlobalGenerator()->HasRule(ruleName)) {
    cmRulePlaceholderExpander::RuleVariables vars;
    vars.CMTargetName = this->GetGeneratorTarget()->GetName().c_str();
    vars.CMTargetType =
      cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType());

    vars.Language = "CUDA";

    std::string responseFlag;
    if (!useResponseFile) {
      vars.Objects = "$in";
      vars.LinkLibraries = "$LINK_LIBRARIES";
    } else {
      std::string cmakeVarLang = "CMAKE_";
      cmakeVarLang += this->TargetLinkLanguage;

      // build response file name
      std::string cmakeLinkVar = cmakeVarLang + "_RESPONSE_FILE_LINK_FLAG";
      const char* flag = GetMakefile()->GetDefinition(cmakeLinkVar);
      if (flag) {
        responseFlag = flag;
      } else {
        responseFlag = "@";
      }
      rspfile = "$RSP_FILE";
      responseFlag += rspfile;

      // build response file content
      if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
        rspcontent = "$in";
      } else {
        rspcontent = "$in_newline";
      }
      rspcontent += " $LINK_LIBRARIES";
      vars.Objects = responseFlag.c_str();
      vars.LinkLibraries = "";
    }

    vars.ObjectDir = "$OBJECT_DIR";

    vars.Target = "$TARGET_FILE";

    vars.SONameFlag = "$SONAME_FLAG";
    vars.TargetSOName = "$SONAME";
    vars.TargetPDB = "$TARGET_PDB";
    vars.TargetCompilePDB = "$TARGET_COMPILE_PDB";

    vars.Flags = "$FLAGS";
    vars.LinkFlags = "$LINK_FLAGS";
    vars.Manifests = "$MANIFESTS";

    std::string langFlags;
    if (targetType != cmStateEnums::EXECUTABLE) {
      langFlags += "$LANGUAGE_COMPILE_FLAGS $ARCH_FLAGS";
      vars.LanguageCompileFlags = langFlags.c_str();
    }

    std::string launcher;
    const char* val = this->GetLocalGenerator()->GetRuleLauncher(
      this->GetGeneratorTarget(), "RULE_LAUNCH_LINK");
    if (val && *val) {
      launcher = val;
      launcher += " ";
    }

    CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
      this->GetLocalGenerator()->CreateRulePlaceholderExpander());

    // Rule for linking library/executable.
    std::vector<std::string> linkCmds = this->ComputeDeviceLinkCmd();
    for (std::vector<std::string>::iterator i = linkCmds.begin();
         i != linkCmds.end(); ++i) {
      *i = launcher + *i;
      rulePlaceholderExpander->ExpandRuleVariables(this->GetLocalGenerator(),
                                                   *i, vars);
    }

    // If there is no ranlib the command will be ":".  Skip it.
    cmEraseIf(linkCmds, cmNinjaRemoveNoOpCommands());

    std::string linkCmd =
      this->GetLocalGenerator()->BuildCommandLine(linkCmds);

    // Write the linker rule with response file if needed.
    std::ostringstream comment;
    comment << "Rule for linking " << this->TargetLinkLanguage << " "
            << this->GetVisibleTypeName() << ".";
    std::ostringstream description;
    description << "Linking " << this->TargetLinkLanguage << " "
                << this->GetVisibleTypeName() << " $TARGET_FILE";
    this->GetGlobalGenerator()->AddRule(ruleName, linkCmd, description.str(),
                                        comment.str(),
                                        /*depfile*/ "",
                                        /*deptype*/ "", rspfile, rspcontent,
                                        /*restat*/ "$RESTAT",
                                        /*generator*/ false);
  }
}

void cmNinjaNormalTargetGenerator::WriteLinkRule(bool useResponseFile)
{
  cmStateEnums::TargetType targetType = this->GetGeneratorTarget()->GetType();
  std::string ruleName = this->LanguageLinkerRule();

  // Select whether to use a response file for objects.
  std::string rspfile;
  std::string rspcontent;

  if (!this->GetGlobalGenerator()->HasRule(ruleName)) {
    cmRulePlaceholderExpander::RuleVariables vars;
    vars.CMTargetName = this->GetGeneratorTarget()->GetName().c_str();
    vars.CMTargetType =
      cmState::GetTargetTypeName(this->GetGeneratorTarget()->GetType());

    vars.Language = this->TargetLinkLanguage.c_str();

    std::string responseFlag;
    if (!useResponseFile) {
      vars.Objects = "$in";
      vars.LinkLibraries = "$LINK_PATH $LINK_LIBRARIES";
    } else {
      std::string cmakeVarLang = "CMAKE_";
      cmakeVarLang += this->TargetLinkLanguage;

      // build response file name
      std::string cmakeLinkVar = cmakeVarLang + "_RESPONSE_FILE_LINK_FLAG";
      const char* flag = GetMakefile()->GetDefinition(cmakeLinkVar);
      if (flag) {
        responseFlag = flag;
      } else {
        responseFlag = "@";
      }
      rspfile = "$RSP_FILE";
      responseFlag += rspfile;

      // build response file content
      if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
        rspcontent = "$in";
      } else {
        rspcontent = "$in_newline";
      }
      rspcontent += " $LINK_PATH $LINK_LIBRARIES";
      vars.Objects = responseFlag.c_str();
      vars.LinkLibraries = "";
    }

    vars.ObjectDir = "$OBJECT_DIR";

    vars.Target = "$TARGET_FILE";

    vars.SONameFlag = "$SONAME_FLAG";
    vars.TargetSOName = "$SONAME";
    vars.TargetInstallNameDir = "$INSTALLNAME_DIR";
    vars.TargetPDB = "$TARGET_PDB";

    // Setup the target version.
    std::string targetVersionMajor;
    std::string targetVersionMinor;
    {
      std::ostringstream majorStream;
      std::ostringstream minorStream;
      int major;
      int minor;
      this->GetGeneratorTarget()->GetTargetVersion(major, minor);
      majorStream << major;
      minorStream << minor;
      targetVersionMajor = majorStream.str();
      targetVersionMinor = minorStream.str();
    }
    vars.TargetVersionMajor = targetVersionMajor.c_str();
    vars.TargetVersionMinor = targetVersionMinor.c_str();

    vars.Flags = "$FLAGS";
    vars.LinkFlags = "$LINK_FLAGS";
    vars.Manifests = "$MANIFESTS";

    std::string langFlags;
    if (targetType != cmStateEnums::EXECUTABLE) {
      langFlags += "$LANGUAGE_COMPILE_FLAGS $ARCH_FLAGS";
      vars.LanguageCompileFlags = langFlags.c_str();
    }

    std::string launcher;
    const char* val = this->GetLocalGenerator()->GetRuleLauncher(
      this->GetGeneratorTarget(), "RULE_LAUNCH_LINK");
    if (val && *val) {
      launcher = val;
      launcher += " ";
    }

    CM_AUTO_PTR<cmRulePlaceholderExpander> rulePlaceholderExpander(
      this->GetLocalGenerator()->CreateRulePlaceholderExpander());

    // Rule for linking library/executable.
    std::vector<std::string> linkCmds = this->ComputeLinkCmd();
    for (std::vector<std::string>::iterator i = linkCmds.begin();
         i != linkCmds.end(); ++i) {
      *i = launcher + *i;
      rulePlaceholderExpander->ExpandRuleVariables(this->GetLocalGenerator(),
                                                   *i, vars);
    }

    // If there is no ranlib the command will be ":".  Skip it.
    cmEraseIf(linkCmds, cmNinjaRemoveNoOpCommands());

    linkCmds.insert(linkCmds.begin(), "$PRE_LINK");
    linkCmds.push_back("$POST_BUILD");
    std::string linkCmd =
      this->GetLocalGenerator()->BuildCommandLine(linkCmds);

    // Write the linker rule with response file if needed.
    std::ostringstream comment;
    comment << "Rule for linking " << this->TargetLinkLanguage << " "
            << this->GetVisibleTypeName() << ".";
    std::ostringstream description;
    description << "Linking " << this->TargetLinkLanguage << " "
                << this->GetVisibleTypeName() << " $TARGET_FILE";
    this->GetGlobalGenerator()->AddRule(ruleName, linkCmd, description.str(),
                                        comment.str(),
                                        /*depfile*/ "",
                                        /*deptype*/ "", rspfile, rspcontent,
                                        /*restat*/ "$RESTAT",
                                        /*generator*/ false);
  }

  if (this->TargetNameOut != this->TargetNameReal &&
      !this->GetGeneratorTarget()->IsFrameworkOnApple()) {
    std::string cmakeCommand =
      this->GetLocalGenerator()->ConvertToOutputFormat(
        cmSystemTools::GetCMakeCommand(), cmOutputConverter::SHELL);
    if (targetType == cmStateEnums::EXECUTABLE) {
      this->GetGlobalGenerator()->AddRule(
        "CMAKE_SYMLINK_EXECUTABLE",
        cmakeCommand + " -E cmake_symlink_executable"
                       " $in $out && $POST_BUILD",
        "Creating executable symlink $out", "Rule for creating "
                                            "executable symlink.",
        /*depfile*/ "",
        /*deptype*/ "",
        /*rspfile*/ "",
        /*rspcontent*/ "",
        /*restat*/ "",
        /*generator*/ false);
    } else {
      this->GetGlobalGenerator()->AddRule(
        "CMAKE_SYMLINK_LIBRARY",
        cmakeCommand + " -E cmake_symlink_library"
                       " $in $SONAME $out && $POST_BUILD",
        "Creating library symlink $out", "Rule for creating "
                                         "library symlink.",
        /*depfile*/ "",
        /*deptype*/ "",
        /*rspfile*/ "",
        /*rspcontent*/ "",
        /*restat*/ "",
        /*generator*/ false);
    }
  }
}

std::vector<std::string> cmNinjaNormalTargetGenerator::ComputeDeviceLinkCmd()
{
  std::vector<std::string> linkCmds;

  // this target requires separable cuda compilation
  // now build the correct command depending on if the target is
  // an executable or a dynamic library.
  std::string linkCmd;
  switch (this->GetGeneratorTarget()->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY: {
      const std::string cudaLinkCmd(
        this->GetMakefile()->GetDefinition("CMAKE_CUDA_DEVICE_LINK_LIBRARY"));
      cmSystemTools::ExpandListArgument(cudaLinkCmd, linkCmds);
    } break;
    case cmStateEnums::EXECUTABLE: {
      const std::string cudaLinkCmd(this->GetMakefile()->GetDefinition(
        "CMAKE_CUDA_DEVICE_LINK_EXECUTABLE"));
      cmSystemTools::ExpandListArgument(cudaLinkCmd, linkCmds);
    } break;
    default:
      break;
  }
  return linkCmds;
}

std::vector<std::string> cmNinjaNormalTargetGenerator::ComputeLinkCmd()
{
  std::vector<std::string> linkCmds;
  cmMakefile* mf = this->GetMakefile();
  {
    // If we have a rule variable prefer it. In the case of static libraries
    // this occurs when things like IPO is enabled, and we need to use the
    // CMAKE_<lang>_CREATE_STATIC_LIBRARY_IPO define instead.
    std::string linkCmdVar = this->GetGeneratorTarget()->GetCreateRuleVariable(
      this->TargetLinkLanguage, this->GetConfigName());
    const char* linkCmd = mf->GetDefinition(linkCmdVar);
    if (linkCmd) {
      cmSystemTools::ExpandListArgument(linkCmd, linkCmds);
      if (this->GetGeneratorTarget()->GetPropertyAsBool("LINK_WHAT_YOU_USE")) {
        std::string cmakeCommand =
          this->GetLocalGenerator()->ConvertToOutputFormat(
            cmSystemTools::GetCMakeCommand(), cmLocalGenerator::SHELL);
        cmakeCommand += " -E __run_iwyu --lwyu=";
        cmGeneratorTarget& gt = *this->GetGeneratorTarget();
        const std::string cfgName = this->GetConfigName();
        std::string targetOutput = ConvertToNinjaPath(gt.GetFullPath(cfgName));
        std::string targetOutputReal = this->ConvertToNinjaPath(
          gt.GetFullPath(cfgName, cmStateEnums::RuntimeBinaryArtifact,
                         /*realname=*/true));
        cmakeCommand += targetOutputReal;
        cmakeCommand += " || true";
        linkCmds.push_back(cmakeCommand);
      }
      return linkCmds;
    }
  }
  switch (this->GetGeneratorTarget()->GetType()) {
    case cmStateEnums::STATIC_LIBRARY: {
      // We have archive link commands set. First, delete the existing archive.
      {
        std::string cmakeCommand =
          this->GetLocalGenerator()->ConvertToOutputFormat(
            cmSystemTools::GetCMakeCommand(), cmOutputConverter::SHELL);
        linkCmds.push_back(cmakeCommand + " -E remove $TARGET_FILE");
      }
      // TODO: Use ARCHIVE_APPEND for archives over a certain size.
      {
        std::string linkCmdVar = "CMAKE_";
        linkCmdVar += this->TargetLinkLanguage;
        linkCmdVar += "_ARCHIVE_CREATE";

        linkCmdVar = this->GeneratorTarget->GetFeatureSpecificLinkRuleVariable(
          linkCmdVar, this->TargetLinkLanguage, this->GetConfigName());

        const char* linkCmd = mf->GetRequiredDefinition(linkCmdVar);
        cmSystemTools::ExpandListArgument(linkCmd, linkCmds);
      }
      {
        std::string linkCmdVar = "CMAKE_";
        linkCmdVar += this->TargetLinkLanguage;
        linkCmdVar += "_ARCHIVE_FINISH";

        linkCmdVar = this->GeneratorTarget->GetFeatureSpecificLinkRuleVariable(
          linkCmdVar, this->TargetLinkLanguage, this->GetConfigName());

        const char* linkCmd = mf->GetRequiredDefinition(linkCmdVar);
        cmSystemTools::ExpandListArgument(linkCmd, linkCmds);
      }
      return linkCmds;
    }
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
    case cmStateEnums::EXECUTABLE:
      break;
    default:
      assert(false && "Unexpected target type");
  }
  return std::vector<std::string>();
}

void cmNinjaNormalTargetGenerator::WriteDeviceLinkStatement()
{
  cmGeneratorTarget& genTarget = *this->GetGeneratorTarget();

  // determine if we need to do any device linking for this target
  const std::string cuda_lang("CUDA");
  cmGeneratorTarget::LinkClosure const* closure =
    genTarget.GetLinkClosure(this->GetConfigName());

  const bool hasCUDA =
    (std::find(closure->Languages.begin(), closure->Languages.end(),
               cuda_lang) != closure->Languages.end());

  bool shouldHaveDeviceLinking = false;
  switch (genTarget.GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
    case cmStateEnums::EXECUTABLE:
      shouldHaveDeviceLinking = true;
      break;
    case cmStateEnums::STATIC_LIBRARY:
      shouldHaveDeviceLinking =
        genTarget.GetPropertyAsBool("CUDA_RESOLVE_DEVICE_SYMBOLS");
      break;
    default:
      break;
  }

  if (!(shouldHaveDeviceLinking && hasCUDA)) {
    return;
  }

  // Now we can do device linking

  // First and very important step is to make sure while inside this
  // step our link language is set to CUDA
  std::string cudaLinkLanguage = "CUDA";
  std::string const objExt =
    this->Makefile->GetSafeDefinition("CMAKE_CUDA_OUTPUT_EXTENSION");

  std::string const cfgName = this->GetConfigName();
  std::string const targetOutputReal = ConvertToNinjaPath(
    genTarget.ObjectDirectory + "cmake_device_link" + objExt);

  std::string const targetOutputImplib = ConvertToNinjaPath(
    genTarget.GetFullPath(cfgName, cmStateEnums::ImportLibraryArtifact));

  this->DeviceLinkObject = targetOutputReal;

  // Write comments.
  cmGlobalNinjaGenerator::WriteDivider(this->GetBuildFileStream());
  const cmStateEnums::TargetType targetType = genTarget.GetType();
  this->GetBuildFileStream() << "# Device Link build statements for "
                             << cmState::GetTargetTypeName(targetType)
                             << " target " << this->GetTargetName() << "\n\n";

  // Compute the comment.
  std::ostringstream comment;
  comment << "Link the " << this->GetVisibleTypeName() << " "
          << targetOutputReal;

  cmNinjaDeps emptyDeps;
  cmNinjaVars vars;

  // Compute outputs.
  cmNinjaDeps outputs;
  outputs.push_back(targetOutputReal);
  // Compute specific libraries to link with.
  cmNinjaDeps explicitDeps = this->GetObjects();
  cmNinjaDeps implicitDeps = this->ComputeLinkDeps();

  std::string frameworkPath;
  std::string linkPath;

  std::string createRule = genTarget.GetCreateRuleVariable(
    this->TargetLinkLanguage, this->GetConfigName());
  const bool useWatcomQuote =
    this->GetMakefile()->IsOn(createRule + "_USE_WATCOM_QUOTE");
  cmLocalNinjaGenerator& localGen = *this->GetLocalGenerator();

  vars["TARGET_FILE"] =
    localGen.ConvertToOutputFormat(targetOutputReal, cmOutputConverter::SHELL);

  CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
    new cmNinjaLinkLineDeviceComputer(
      this->GetLocalGenerator(),
      this->GetLocalGenerator()->GetStateSnapshot().GetDirectory(),
      this->GetGlobalGenerator()));
  linkLineComputer->SetUseWatcomQuote(useWatcomQuote);

  localGen.GetTargetFlags(
    linkLineComputer.get(), this->GetConfigName(), vars["LINK_LIBRARIES"],
    vars["FLAGS"], vars["LINK_FLAGS"], frameworkPath, linkPath, &genTarget);

  this->addPoolNinjaVariable("JOB_POOL_LINK", &genTarget, vars);

  vars["LINK_FLAGS"] =
    cmGlobalNinjaGenerator::EncodeLiteral(vars["LINK_FLAGS"]);

  vars["MANIFESTS"] = this->GetManifests();

  vars["LINK_PATH"] = frameworkPath + linkPath;

  // Compute architecture specific link flags.  Yes, these go into a different
  // variable for executables, probably due to a mistake made when duplicating
  // code between the Makefile executable and library generators.
  if (targetType == cmStateEnums::EXECUTABLE) {
    std::string t = vars["FLAGS"];
    localGen.AddArchitectureFlags(t, &genTarget, cudaLinkLanguage, cfgName);
    vars["FLAGS"] = t;
  } else {
    std::string t = vars["ARCH_FLAGS"];
    localGen.AddArchitectureFlags(t, &genTarget, cudaLinkLanguage, cfgName);
    vars["ARCH_FLAGS"] = t;
    t = "";
    localGen.AddLanguageFlagsForLinking(t, &genTarget, cudaLinkLanguage,
                                        cfgName);
    vars["LANGUAGE_COMPILE_FLAGS"] = t;
  }
  if (this->GetGeneratorTarget()->HasSOName(cfgName)) {
    vars["SONAME_FLAG"] =
      this->GetMakefile()->GetSONameFlag(this->TargetLinkLanguage);
    vars["SONAME"] = this->TargetNameSO;
    if (targetType == cmStateEnums::SHARED_LIBRARY) {
      std::string install_dir =
        this->GetGeneratorTarget()->GetInstallNameDirForBuildTree(cfgName);
      if (!install_dir.empty()) {
        vars["INSTALLNAME_DIR"] = localGen.ConvertToOutputFormat(
          install_dir, cmOutputConverter::SHELL);
      }
    }
  }

  cmNinjaDeps byproducts;

  if (!this->TargetNameImport.empty()) {
    const std::string impLibPath = localGen.ConvertToOutputFormat(
      targetOutputImplib, cmOutputConverter::SHELL);
    vars["TARGET_IMPLIB"] = impLibPath;
    EnsureParentDirectoryExists(impLibPath);
    if (genTarget.HasImportLibrary()) {
      byproducts.push_back(targetOutputImplib);
    }
  }

  const std::string objPath = GetGeneratorTarget()->GetSupportDirectory();
  vars["OBJECT_DIR"] = this->GetLocalGenerator()->ConvertToOutputFormat(
    this->ConvertToNinjaPath(objPath), cmOutputConverter::SHELL);
  EnsureDirectoryExists(objPath);

  this->SetMsvcTargetPdbVariable(vars);

  if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
    // ar.exe can't handle backslashes in rsp files (implicitly used by gcc)
    std::string& linkLibraries = vars["LINK_LIBRARIES"];
    std::replace(linkLibraries.begin(), linkLibraries.end(), '\\', '/');
    std::string& link_path = vars["LINK_PATH"];
    std::replace(link_path.begin(), link_path.end(), '\\', '/');
  }

  const std::vector<cmCustomCommand>* cmdLists[3] = {
    &genTarget.GetPreBuildCommands(), &genTarget.GetPreLinkCommands(),
    &genTarget.GetPostBuildCommands()
  };

  std::vector<std::string> preLinkCmdLines, postBuildCmdLines;
  vars["PRE_LINK"] = localGen.BuildCommandLine(preLinkCmdLines);
  vars["POST_BUILD"] = localGen.BuildCommandLine(postBuildCmdLines);

  std::vector<std::string>* cmdLineLists[3] = { &preLinkCmdLines,
                                                &preLinkCmdLines,
                                                &postBuildCmdLines };

  for (unsigned i = 0; i != 3; ++i) {
    for (std::vector<cmCustomCommand>::const_iterator ci =
           cmdLists[i]->begin();
         ci != cmdLists[i]->end(); ++ci) {
      cmCustomCommandGenerator ccg(*ci, cfgName, this->GetLocalGenerator());
      localGen.AppendCustomCommandLines(ccg, *cmdLineLists[i]);
      std::vector<std::string> const& ccByproducts = ccg.GetByproducts();
      std::transform(ccByproducts.begin(), ccByproducts.end(),
                     std::back_inserter(byproducts), MapToNinjaPath());
    }
  }

  cmGlobalNinjaGenerator& globalGen = *this->GetGlobalGenerator();

  // Device linking currently doesn't support response files so
  // do not check if the user has explicitly forced a response file.
  int const commandLineLengthLimit =
    static_cast<int>(cmSystemTools::CalculateCommandLineLengthLimit()) -
    globalGen.GetRuleCmdLength(this->LanguageLinkerDeviceRule());

  const std::string rspfile =
    std::string(cmake::GetCMakeFilesDirectoryPostSlash()) +
    genTarget.GetName() + ".rsp";

  // Gather order-only dependencies.
  cmNinjaDeps orderOnlyDeps;
  this->GetLocalGenerator()->AppendTargetDepends(this->GetGeneratorTarget(),
                                                 orderOnlyDeps);

  // Write the build statement for this target.
  bool usedResponseFile = false;
  globalGen.WriteBuild(this->GetBuildFileStream(), comment.str(),
                       this->LanguageLinkerDeviceRule(), outputs,
                       /*implicitOuts=*/cmNinjaDeps(), explicitDeps,
                       implicitDeps, orderOnlyDeps, vars, rspfile,
                       commandLineLengthLimit, &usedResponseFile);
  this->WriteDeviceLinkRule(usedResponseFile);
}

void cmNinjaNormalTargetGenerator::WriteLinkStatement()
{
  cmGeneratorTarget& gt = *this->GetGeneratorTarget();
  const std::string cfgName = this->GetConfigName();
  std::string targetOutput = ConvertToNinjaPath(gt.GetFullPath(cfgName));
  std::string targetOutputReal = ConvertToNinjaPath(
    gt.GetFullPath(cfgName, cmStateEnums::RuntimeBinaryArtifact,
                   /*realname=*/true));
  std::string targetOutputImplib = ConvertToNinjaPath(
    gt.GetFullPath(cfgName, cmStateEnums::ImportLibraryArtifact));

  if (gt.IsAppBundleOnApple()) {
    // Create the app bundle
    std::string outpath = gt.GetDirectory(cfgName);
    this->OSXBundleGenerator->CreateAppBundle(this->TargetNameOut, outpath);

    // Calculate the output path
    targetOutput = outpath;
    targetOutput += "/";
    targetOutput += this->TargetNameOut;
    targetOutput = this->ConvertToNinjaPath(targetOutput);
    targetOutputReal = outpath;
    targetOutputReal += "/";
    targetOutputReal += this->TargetNameReal;
    targetOutputReal = this->ConvertToNinjaPath(targetOutputReal);
  } else if (gt.IsFrameworkOnApple()) {
    // Create the library framework.
    this->OSXBundleGenerator->CreateFramework(this->TargetNameOut,
                                              gt.GetDirectory(cfgName));
  } else if (gt.IsCFBundleOnApple()) {
    // Create the core foundation bundle.
    this->OSXBundleGenerator->CreateCFBundle(this->TargetNameOut,
                                             gt.GetDirectory(cfgName));
  }

  // Write comments.
  cmGlobalNinjaGenerator::WriteDivider(this->GetBuildFileStream());
  const cmStateEnums::TargetType targetType = gt.GetType();
  this->GetBuildFileStream() << "# Link build statements for "
                             << cmState::GetTargetTypeName(targetType)
                             << " target " << this->GetTargetName() << "\n\n";

  cmNinjaDeps emptyDeps;
  cmNinjaVars vars;

  // Compute the comment.
  std::ostringstream comment;
  comment << "Link the " << this->GetVisibleTypeName() << " "
          << targetOutputReal;

  // Compute outputs.
  cmNinjaDeps outputs;
  outputs.push_back(targetOutputReal);

  // Compute specific libraries to link with.
  cmNinjaDeps explicitDeps = this->GetObjects();
  cmNinjaDeps implicitDeps = this->ComputeLinkDeps();

  if (!this->DeviceLinkObject.empty()) {
    explicitDeps.push_back(this->DeviceLinkObject);
  }

  cmMakefile* mf = this->GetMakefile();

  std::string frameworkPath;
  std::string linkPath;
  cmGeneratorTarget& genTarget = *this->GetGeneratorTarget();

  std::string createRule = genTarget.GetCreateRuleVariable(
    this->TargetLinkLanguage, this->GetConfigName());
  bool useWatcomQuote = mf->IsOn(createRule + "_USE_WATCOM_QUOTE");
  cmLocalNinjaGenerator& localGen = *this->GetLocalGenerator();

  vars["TARGET_FILE"] =
    localGen.ConvertToOutputFormat(targetOutputReal, cmOutputConverter::SHELL);

  CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
    this->GetGlobalGenerator()->CreateLinkLineComputer(
      this->GetLocalGenerator(),
      this->GetLocalGenerator()->GetStateSnapshot().GetDirectory()));
  linkLineComputer->SetUseWatcomQuote(useWatcomQuote);

  localGen.GetTargetFlags(
    linkLineComputer.get(), this->GetConfigName(), vars["LINK_LIBRARIES"],
    vars["FLAGS"], vars["LINK_FLAGS"], frameworkPath, linkPath, &genTarget);

  // Add OS X version flags, if any.
  if (this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
      this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
    this->AppendOSXVerFlag(vars["LINK_FLAGS"], this->TargetLinkLanguage,
                           "COMPATIBILITY", true);
    this->AppendOSXVerFlag(vars["LINK_FLAGS"], this->TargetLinkLanguage,
                           "CURRENT", false);
  }

  this->addPoolNinjaVariable("JOB_POOL_LINK", &gt, vars);

  this->AddModuleDefinitionFlag(linkLineComputer.get(), vars["LINK_FLAGS"]);
  vars["LINK_FLAGS"] =
    cmGlobalNinjaGenerator::EncodeLiteral(vars["LINK_FLAGS"]);

  vars["MANIFESTS"] = this->GetManifests();

  vars["LINK_PATH"] = frameworkPath + linkPath;
  std::string lwyuFlags;
  if (genTarget.GetPropertyAsBool("LINK_WHAT_YOU_USE")) {
    lwyuFlags = " -Wl,--no-as-needed";
  }

  // Compute architecture specific link flags.  Yes, these go into a different
  // variable for executables, probably due to a mistake made when duplicating
  // code between the Makefile executable and library generators.
  if (targetType == cmStateEnums::EXECUTABLE) {
    std::string t = vars["FLAGS"];
    localGen.AddArchitectureFlags(t, &genTarget, TargetLinkLanguage, cfgName);
    t += lwyuFlags;
    vars["FLAGS"] = t;
  } else {
    std::string t = vars["ARCH_FLAGS"];
    localGen.AddArchitectureFlags(t, &genTarget, TargetLinkLanguage, cfgName);
    vars["ARCH_FLAGS"] = t;
    t = "";
    t += lwyuFlags;
    localGen.AddLanguageFlagsForLinking(t, &genTarget, TargetLinkLanguage,
                                        cfgName);
    vars["LANGUAGE_COMPILE_FLAGS"] = t;
  }
  if (this->GetGeneratorTarget()->HasSOName(cfgName)) {
    vars["SONAME_FLAG"] = mf->GetSONameFlag(this->TargetLinkLanguage);
    vars["SONAME"] = this->TargetNameSO;
    if (targetType == cmStateEnums::SHARED_LIBRARY) {
      std::string install_dir =
        this->GetGeneratorTarget()->GetInstallNameDirForBuildTree(cfgName);
      if (!install_dir.empty()) {
        vars["INSTALLNAME_DIR"] = localGen.ConvertToOutputFormat(
          install_dir, cmOutputConverter::SHELL);
      }
    }
  }

  cmNinjaDeps byproducts;

  if (!this->TargetNameImport.empty()) {
    const std::string impLibPath = localGen.ConvertToOutputFormat(
      targetOutputImplib, cmOutputConverter::SHELL);
    vars["TARGET_IMPLIB"] = impLibPath;
    EnsureParentDirectoryExists(impLibPath);
    if (genTarget.HasImportLibrary()) {
      byproducts.push_back(targetOutputImplib);
    }
  }

  if (!this->SetMsvcTargetPdbVariable(vars)) {
    // It is common to place debug symbols at a specific place,
    // so we need a plain target name in the rule available.
    std::string prefix;
    std::string base;
    std::string suffix;
    this->GetGeneratorTarget()->GetFullNameComponents(prefix, base, suffix);
    std::string dbg_suffix = ".dbg";
    // TODO: Where to document?
    if (mf->GetDefinition("CMAKE_DEBUG_SYMBOL_SUFFIX")) {
      dbg_suffix = mf->GetDefinition("CMAKE_DEBUG_SYMBOL_SUFFIX");
    }
    vars["TARGET_PDB"] = base + suffix + dbg_suffix;
  }

  const std::string objPath = GetGeneratorTarget()->GetSupportDirectory();
  vars["OBJECT_DIR"] = this->GetLocalGenerator()->ConvertToOutputFormat(
    this->ConvertToNinjaPath(objPath), cmOutputConverter::SHELL);
  EnsureDirectoryExists(objPath);

  if (this->GetGlobalGenerator()->IsGCCOnWindows()) {
    // ar.exe can't handle backslashes in rsp files (implicitly used by gcc)
    std::string& linkLibraries = vars["LINK_LIBRARIES"];
    std::replace(linkLibraries.begin(), linkLibraries.end(), '\\', '/');
    std::string& link_path = vars["LINK_PATH"];
    std::replace(link_path.begin(), link_path.end(), '\\', '/');
  }

  const std::vector<cmCustomCommand>* cmdLists[3] = {
    &gt.GetPreBuildCommands(), &gt.GetPreLinkCommands(),
    &gt.GetPostBuildCommands()
  };

  std::vector<std::string> preLinkCmdLines, postBuildCmdLines;
  std::vector<std::string>* cmdLineLists[3] = { &preLinkCmdLines,
                                                &preLinkCmdLines,
                                                &postBuildCmdLines };

  for (unsigned i = 0; i != 3; ++i) {
    for (std::vector<cmCustomCommand>::const_iterator ci =
           cmdLists[i]->begin();
         ci != cmdLists[i]->end(); ++ci) {
      cmCustomCommandGenerator ccg(*ci, cfgName, this->GetLocalGenerator());
      localGen.AppendCustomCommandLines(ccg, *cmdLineLists[i]);
      std::vector<std::string> const& ccByproducts = ccg.GetByproducts();
      std::transform(ccByproducts.begin(), ccByproducts.end(),
                     std::back_inserter(byproducts), MapToNinjaPath());
    }
  }

  // maybe create .def file from list of objects
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    gt.GetModuleDefinitionInfo(this->GetConfigName());
  if (mdi && mdi->DefFileGenerated) {
    std::string cmakeCommand =
      this->GetLocalGenerator()->ConvertToOutputFormat(
        cmSystemTools::GetCMakeCommand(), cmOutputConverter::SHELL);
    std::string cmd = cmakeCommand;
    cmd += " -E __create_def ";
    cmd += this->GetLocalGenerator()->ConvertToOutputFormat(
      mdi->DefFile, cmOutputConverter::SHELL);
    cmd += " ";
    std::string obj_list_file = mdi->DefFile + ".objs";
    cmd += this->GetLocalGenerator()->ConvertToOutputFormat(
      obj_list_file, cmOutputConverter::SHELL);
    preLinkCmdLines.push_back(cmd);

    // create a list of obj files for the -E __create_def to read
    cmGeneratedFileStream fout(obj_list_file.c_str());

    if (mdi->WindowsExportAllSymbols) {
      cmNinjaDeps objs = this->GetObjects();
      for (cmNinjaDeps::iterator i = objs.begin(); i != objs.end(); ++i) {
        if (cmHasLiteralSuffix(*i, ".obj")) {
          fout << *i << "\n";
        }
      }
    }

    for (std::vector<cmSourceFile const*>::const_iterator i =
           mdi->Sources.begin();
         i != mdi->Sources.end(); ++i) {
      fout << (*i)->GetFullPath() << "\n";
    }
  }
  // If we have any PRE_LINK commands, we need to go back to CMAKE_BINARY_DIR
  // for
  // the link commands.
  if (!preLinkCmdLines.empty()) {
    const std::string homeOutDir = localGen.ConvertToOutputFormat(
      localGen.GetBinaryDirectory(), cmOutputConverter::SHELL);
    preLinkCmdLines.push_back("cd " + homeOutDir);
  }

  vars["PRE_LINK"] = localGen.BuildCommandLine(preLinkCmdLines);
  std::string postBuildCmdLine = localGen.BuildCommandLine(postBuildCmdLines);

  cmNinjaVars symlinkVars;
  bool const symlinkNeeded =
    (targetOutput != targetOutputReal && !gt.IsFrameworkOnApple());
  if (!symlinkNeeded) {
    vars["POST_BUILD"] = postBuildCmdLine;
  } else {
    vars["POST_BUILD"] = cmGlobalNinjaGenerator::SHELL_NOOP;
    symlinkVars["POST_BUILD"] = postBuildCmdLine;
  }
  cmGlobalNinjaGenerator& globalGen = *this->GetGlobalGenerator();

  bool const lang_supports_response =
    !(this->TargetLinkLanguage == "RC" || this->TargetLinkLanguage == "CUDA");
  int commandLineLengthLimit = -1;
  if (!lang_supports_response || !this->ForceResponseFile()) {
    commandLineLengthLimit =
      static_cast<int>(cmSystemTools::CalculateCommandLineLengthLimit()) -
      globalGen.GetRuleCmdLength(this->LanguageLinkerRule());
  }

  const std::string rspfile =
    std::string(cmake::GetCMakeFilesDirectoryPostSlash()) + gt.GetName() +
    ".rsp";

  // Gather order-only dependencies.
  cmNinjaDeps orderOnlyDeps;
  this->GetLocalGenerator()->AppendTargetDepends(this->GetGeneratorTarget(),
                                                 orderOnlyDeps);

  // Ninja should restat after linking if and only if there are byproducts.
  vars["RESTAT"] = byproducts.empty() ? "" : "1";

  for (cmNinjaDeps::const_iterator oi = byproducts.begin(),
                                   oe = byproducts.end();
       oi != oe; ++oi) {
    this->GetGlobalGenerator()->SeenCustomCommandOutput(*oi);
    outputs.push_back(*oi);
  }

  // Write the build statement for this target.
  bool usedResponseFile = false;
  globalGen.WriteBuild(this->GetBuildFileStream(), comment.str(),
                       this->LanguageLinkerRule(), outputs,
                       /*implicitOuts=*/cmNinjaDeps(), explicitDeps,
                       implicitDeps, orderOnlyDeps, vars, rspfile,
                       commandLineLengthLimit, &usedResponseFile);
  this->WriteLinkRule(usedResponseFile);

  if (symlinkNeeded) {
    if (targetType == cmStateEnums::EXECUTABLE) {
      globalGen.WriteBuild(
        this->GetBuildFileStream(),
        "Create executable symlink " + targetOutput,
        "CMAKE_SYMLINK_EXECUTABLE", cmNinjaDeps(1, targetOutput),
        /*implicitOuts=*/cmNinjaDeps(), cmNinjaDeps(1, targetOutputReal),
        emptyDeps, emptyDeps, symlinkVars);
    } else {
      cmNinjaDeps symlinks;
      std::string const soName =
        this->ConvertToNinjaPath(this->GetTargetFilePath(this->TargetNameSO));
      // If one link has to be created.
      if (targetOutputReal == soName || targetOutput == soName) {
        symlinkVars["SONAME"] = soName;
      } else {
        symlinkVars["SONAME"] = "";
        symlinks.push_back(soName);
      }
      symlinks.push_back(targetOutput);
      globalGen.WriteBuild(
        this->GetBuildFileStream(), "Create library symlink " + targetOutput,
        "CMAKE_SYMLINK_LIBRARY", symlinks,
        /*implicitOuts=*/cmNinjaDeps(), cmNinjaDeps(1, targetOutputReal),
        emptyDeps, emptyDeps, symlinkVars);
    }
  }

  // Add aliases for the file name and the target name.
  globalGen.AddTargetAlias(this->TargetNameOut, &gt);
  globalGen.AddTargetAlias(this->GetTargetName(), &gt);
}

void cmNinjaNormalTargetGenerator::WriteObjectLibStatement()
{
  // Write a phony output that depends on all object files.
  cmNinjaDeps outputs;
  this->GetLocalGenerator()->AppendTargetOutputs(this->GetGeneratorTarget(),
                                                 outputs);
  cmNinjaDeps depends = this->GetObjects();
  this->GetGlobalGenerator()->WritePhonyBuild(
    this->GetBuildFileStream(), "Object library " + this->GetTargetName(),
    outputs, depends);

  // Add aliases for the target name.
  this->GetGlobalGenerator()->AddTargetAlias(this->GetTargetName(),
                                             this->GetGeneratorTarget());
}
