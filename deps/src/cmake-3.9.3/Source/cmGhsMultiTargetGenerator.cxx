/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGhsMultiTargetGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGhsMultiGenerator.h"
#include "cmLinkLineComputer.h"
#include "cmLocalGhsMultiGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmTarget.h"
#include <assert.h>

std::string const cmGhsMultiTargetGenerator::DDOption("-dynamic");

cmGhsMultiTargetGenerator::cmGhsMultiTargetGenerator(cmGeneratorTarget* target)
  : GeneratorTarget(target)
  , LocalGenerator(
      static_cast<cmLocalGhsMultiGenerator*>(target->GetLocalGenerator()))
  , Makefile(target->Target->GetMakefile())
  , TargetGroup(DetermineIfTargetGroup(target))
  , DynamicDownload(false)
{
  this->RelBuildFilePath = this->GetRelBuildFilePath(target);

  this->RelOutputFileName = this->RelBuildFilePath + target->GetName() + ".a";

  this->RelBuildFileName = this->RelBuildFilePath;
  this->RelBuildFileName += this->GetBuildFileName(target);

  std::string absPathToRoot = this->GetAbsPathToRoot(target);
  absPathToRoot = this->AddSlashIfNeededToPath(absPathToRoot);
  this->AbsBuildFilePath = absPathToRoot + this->RelBuildFilePath;
  this->AbsBuildFileName = absPathToRoot + this->RelBuildFileName;
  this->AbsOutputFileName = absPathToRoot + this->RelOutputFileName;
}

cmGhsMultiTargetGenerator::~cmGhsMultiTargetGenerator()
{
  cmDeleteAll(this->FolderBuildStreams);
}

std::string cmGhsMultiTargetGenerator::GetRelBuildFilePath(
  const cmGeneratorTarget* target)
{
  std::string output = target->GetEffectiveFolderName();
  cmSystemTools::ConvertToUnixSlashes(output);
  if (!output.empty()) {
    output += "/";
  }
  output += target->GetName() + "/";
  return output;
}

std::string cmGhsMultiTargetGenerator::GetAbsPathToRoot(
  const cmGeneratorTarget* target)
{
  return target->GetLocalGenerator()->GetBinaryDirectory();
}

std::string cmGhsMultiTargetGenerator::GetAbsBuildFilePath(
  const cmGeneratorTarget* target)
{
  std::string output;
  output = cmGhsMultiTargetGenerator::GetAbsPathToRoot(target);
  output = cmGhsMultiTargetGenerator::AddSlashIfNeededToPath(output);
  output += cmGhsMultiTargetGenerator::GetRelBuildFilePath(target);
  return output;
}

std::string cmGhsMultiTargetGenerator::GetRelBuildFileName(
  const cmGeneratorTarget* target)
{
  std::string output;
  output = cmGhsMultiTargetGenerator::GetRelBuildFilePath(target);
  output = cmGhsMultiTargetGenerator::AddSlashIfNeededToPath(output);
  output += cmGhsMultiTargetGenerator::GetBuildFileName(target);
  return output;
}

std::string cmGhsMultiTargetGenerator::GetBuildFileName(
  const cmGeneratorTarget* target)
{
  std::string output;
  output = target->GetName();
  output += cmGlobalGhsMultiGenerator::FILE_EXTENSION;
  return output;
}

std::string cmGhsMultiTargetGenerator::AddSlashIfNeededToPath(
  std::string const& input)
{
  std::string output(input);
  if (!cmHasLiteralSuffix(output, "/")) {
    output += "/";
  }
  return output;
}

void cmGhsMultiTargetGenerator::Generate()
{
  std::vector<cmSourceFile*> objectSources = this->GetSources();
  if (!objectSources.empty() && this->IncludeThisTarget()) {
    if (!cmSystemTools::FileExists(this->AbsBuildFilePath.c_str())) {
      cmSystemTools::MakeDirectory(this->AbsBuildFilePath.c_str());
    }
    cmGlobalGhsMultiGenerator::Open(std::string(""), this->AbsBuildFileName,
                                    &this->FolderBuildStreams);
    cmGlobalGhsMultiGenerator::OpenBuildFileStream(
      this->GetFolderBuildStreams());
    std::string config = this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
    if (0 == config.length()) {
      config = "RELEASE";
    }
    const std::string language(
      this->GeneratorTarget->GetLinkerLanguage(config));
    config = cmSystemTools::UpperCase(config);
    this->DynamicDownload = this->DetermineIfDynamicDownload(config, language);
    if (this->DynamicDownload) {
      *this->GetFolderBuildStreams() << "#component integrity_dynamic_download"
                                     << std::endl;
    }
    GhsMultiGpj::WriteGpjTag(this->GetGpjTag(), this->GetFolderBuildStreams());
    cmGlobalGhsMultiGenerator::WriteDisclaimer(this->GetFolderBuildStreams());

    bool const notKernel = this->IsNotKernel(config, language);
    this->WriteTypeSpecifics(config, notKernel);
    this->SetCompilerFlags(config, language, notKernel);
    this->WriteCompilerFlags(config, language);
    this->WriteCompilerDefinitions(config, language);
    this->WriteIncludes(config, language);
    if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
      this->WriteTargetLinkLibraries(config, language);
    }
    this->WriteCustomCommands();

    std::map<const cmSourceFile*, std::string> objectNames =
      cmGhsMultiTargetGenerator::GetObjectNames(
        &objectSources, this->LocalGenerator, this->GeneratorTarget);

    this->WriteSources(objectSources, objectNames);
  }
}

bool cmGhsMultiTargetGenerator::IncludeThisTarget()
{
  bool output = true;
  char const* excludeFromAll =
    this->GeneratorTarget->GetProperty("EXCLUDE_FROM_ALL");
  if (NULL != excludeFromAll && '1' == excludeFromAll[0] &&
      '\0' == excludeFromAll[1]) {
    output = false;
  }
  return output;
}

std::vector<cmSourceFile*> cmGhsMultiTargetGenerator::GetSources() const
{
  std::vector<cmSourceFile*> output;
  std::string config = this->Makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
  this->GeneratorTarget->GetSourceFiles(output, config);
  return output;
}

GhsMultiGpj::Types cmGhsMultiTargetGenerator::GetGpjTag() const
{
  return cmGhsMultiTargetGenerator::GetGpjTag(this->GeneratorTarget);
}

GhsMultiGpj::Types cmGhsMultiTargetGenerator::GetGpjTag(
  const cmGeneratorTarget* target)
{
  GhsMultiGpj::Types output;
  if (cmGhsMultiTargetGenerator::DetermineIfTargetGroup(target)) {
    output = GhsMultiGpj::INTERGRITY_APPLICATION;
  } else if (target->GetType() == cmStateEnums::STATIC_LIBRARY) {
    output = GhsMultiGpj::LIBRARY;
  } else {
    output = GhsMultiGpj::PROGRAM;
  }
  return output;
}

cmGlobalGhsMultiGenerator* cmGhsMultiTargetGenerator::GetGlobalGenerator()
  const
{
  return static_cast<cmGlobalGhsMultiGenerator*>(
    this->LocalGenerator->GetGlobalGenerator());
}

void cmGhsMultiTargetGenerator::WriteTypeSpecifics(const std::string& config,
                                                   bool const notKernel)
{
  std::string outputDir(this->GetOutputDirectory(config));
  std::string outputFilename(this->GetOutputFilename(config));

  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
    std::string const static_library_suffix =
      this->Makefile->GetSafeDefinition("CMAKE_STATIC_LIBRARY_SUFFIX");
    *this->GetFolderBuildStreams() << "    -o \"" << outputDir
                                   << outputFilename << static_library_suffix
                                   << "\"" << std::endl;
  } else if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
    if (notKernel && !this->IsTargetGroup()) {
      *this->GetFolderBuildStreams() << "    -relprog" << std::endl;
    }
    if (this->IsTargetGroup()) {
      *this->GetFolderBuildStreams()
        << "    -o \"" << outputDir << outputFilename << ".elf\"" << std::endl;
      *this->GetFolderBuildStreams() << "    :extraOutputFile=\"" << outputDir
                                     << outputFilename << ".elf.ael\""
                                     << std::endl;
    } else {
      std::string const executable_suffix =
        this->Makefile->GetSafeDefinition("CMAKE_EXECUTABLE_SUFFIX");
      *this->GetFolderBuildStreams() << "    -o \"" << outputDir
                                     << outputFilename << executable_suffix
                                     << "\"" << std::endl;
    }
  }
}

void cmGhsMultiTargetGenerator::SetCompilerFlags(std::string const& config,
                                                 const std::string& language,
                                                 bool const notKernel)
{
  std::map<std::string, std::string>::iterator i =
    this->FlagsByLanguage.find(language);
  if (i == this->FlagsByLanguage.end()) {
    std::string flags;
    const char* lang = language.c_str();

    if (notKernel) {
      this->LocalGenerator->AddLanguageFlags(flags, this->GeneratorTarget,
                                             lang, config);
    } else {
      this->LocalGenerator->AddLanguageFlags(flags, this->GeneratorTarget,
                                             lang + std::string("_GHS_KERNEL"),
                                             config);
    }
    this->LocalGenerator->AddCMP0018Flags(flags, this->GeneratorTarget, lang,
                                          config);
    this->LocalGenerator->AddVisibilityPresetFlags(
      flags, this->GeneratorTarget, lang);

    // Append old-style preprocessor definition flags.
    if (this->Makefile->GetDefineFlags() != " ") {
      this->LocalGenerator->AppendFlags(flags,
                                        this->Makefile->GetDefineFlags());
    }

    // Add target-specific flags.
    this->LocalGenerator->AddCompileOptions(flags, this->GeneratorTarget, lang,
                                            config);

    std::map<std::string, std::string>::value_type entry(language, flags);
    i = this->FlagsByLanguage.insert(entry).first;
  }
}

std::string cmGhsMultiTargetGenerator::GetDefines(const std::string& language,
                                                  std::string const& config)
{
  std::map<std::string, std::string>::iterator i =
    this->DefinesByLanguage.find(language);
  if (i == this->DefinesByLanguage.end()) {
    std::set<std::string> defines;
    const char* lang = language.c_str();
    // Add the export symbol definition for shared library objects.
    if (const char* exportMacro = this->GeneratorTarget->GetExportMacro()) {
      this->LocalGenerator->AppendDefines(defines, exportMacro);
    }

    // Add preprocessor definitions for this target and configuration.
    this->LocalGenerator->AddCompileDefinitions(defines, this->GeneratorTarget,
                                                config, language);

    std::string definesString;
    this->LocalGenerator->JoinDefines(defines, definesString, lang);

    std::map<std::string, std::string>::value_type entry(language,
                                                         definesString);
    i = this->DefinesByLanguage.insert(entry).first;
  }
  return i->second;
}

void cmGhsMultiTargetGenerator::WriteCompilerFlags(std::string const&,
                                                   const std::string& language)
{
  std::map<std::string, std::string>::iterator flagsByLangI =
    this->FlagsByLanguage.find(language);
  if (flagsByLangI != this->FlagsByLanguage.end()) {
    if (!flagsByLangI->second.empty()) {
      *this->GetFolderBuildStreams() << "    " << flagsByLangI->second
                                     << std::endl;
    }
  }
}

void cmGhsMultiTargetGenerator::WriteCompilerDefinitions(
  const std::string& config, const std::string& language)
{
  std::vector<std::string> compileDefinitions;
  this->GeneratorTarget->GetCompileDefinitions(compileDefinitions, config,
                                               language);
  for (std::vector<std::string>::const_iterator cdI =
         compileDefinitions.begin();
       cdI != compileDefinitions.end(); ++cdI) {
    *this->GetFolderBuildStreams() << "    -D" << (*cdI) << std::endl;
  }
}

void cmGhsMultiTargetGenerator::WriteIncludes(const std::string& config,
                                              const std::string& language)
{
  std::vector<std::string> includes;
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              language, config);

  for (std::vector<std::string>::const_iterator includes_i = includes.begin();
       includes_i != includes.end(); ++includes_i) {
    *this->GetFolderBuildStreams() << "    -I\"" << *includes_i << "\""
                                   << std::endl;
  }
}

void cmGhsMultiTargetGenerator::WriteTargetLinkLibraries(
  std::string const& config, std::string const& language)
{
  // library directories
  cmTargetDependSet tds =
    this->GetGlobalGenerator()->GetTargetDirectDepends(this->GeneratorTarget);
  for (cmTargetDependSet::iterator tdsI = tds.begin(); tdsI != tds.end();
       ++tdsI) {
    const cmGeneratorTarget* tg = *tdsI;
    *this->GetFolderBuildStreams() << "    -L\"" << GetAbsBuildFilePath(tg)
                                   << "\"" << std::endl;
  }
  // library targets
  cmTarget::LinkLibraryVectorType llv =
    this->GeneratorTarget->Target->GetOriginalLinkLibraries();
  for (cmTarget::LinkLibraryVectorType::const_iterator llvI = llv.begin();
       llvI != llv.end(); ++llvI) {
    std::string libName = llvI->first;
    // if it is a user defined target get the full path to the lib
    cmTarget* tg(GetGlobalGenerator()->FindTarget(libName));
    if (NULL != tg) {
      libName = tg->GetName() + ".a";
    }
    *this->GetFolderBuildStreams() << "    -l\"" << libName << "\""
                                   << std::endl;
  }

  if (!this->TargetGroup) {
    std::string linkLibraries;
    std::string flags;
    std::string linkFlags;
    std::string frameworkPath;
    std::string linkPath;
    std::string createRule =
      this->GeneratorTarget->GetCreateRuleVariable(language, config);
    bool useWatcomQuote =
      this->Makefile->IsOn(createRule + "_USE_WATCOM_QUOTE");
    CM_AUTO_PTR<cmLinkLineComputer> linkLineComputer(
      this->GetGlobalGenerator()->CreateLinkLineComputer(
        this->LocalGenerator,
        this->LocalGenerator->GetStateSnapshot().GetDirectory()));
    linkLineComputer->SetUseWatcomQuote(useWatcomQuote);

    this->LocalGenerator->GetTargetFlags(
      linkLineComputer.get(), config, linkLibraries, flags, linkFlags,
      frameworkPath, linkPath, this->GeneratorTarget);
    linkFlags = cmSystemTools::TrimWhitespace(linkFlags);

    if (!linkPath.empty()) {
      linkPath = " " + linkPath.substr(0U, linkPath.size() - 1U);
      *this->GetFolderBuildStreams() << linkPath;
    }

    if (!linkFlags.empty()) {
      *this->GetFolderBuildStreams() << "    " << linkFlags << std::endl;
    }
  }
}

void cmGhsMultiTargetGenerator::WriteCustomCommands()
{
  WriteCustomCommandsHelper(this->GeneratorTarget->GetPreBuildCommands(),
                            cmTarget::PRE_BUILD);
  WriteCustomCommandsHelper(this->GeneratorTarget->GetPostBuildCommands(),
                            cmTarget::POST_BUILD);
}

void cmGhsMultiTargetGenerator::WriteCustomCommandsHelper(
  std::vector<cmCustomCommand> const& commandsSet,
  cmTarget::CustomCommandType const commandType)
{
  for (std::vector<cmCustomCommand>::const_iterator commandsSetI =
         commandsSet.begin();
       commandsSetI != commandsSet.end(); ++commandsSetI) {
    cmCustomCommandLines const& commands = commandsSetI->GetCommandLines();
    for (cmCustomCommandLines::const_iterator commandI = commands.begin();
         commandI != commands.end(); ++commandI) {
      switch (commandType) {
        case cmTarget::PRE_BUILD:
          *this->GetFolderBuildStreams() << "    :preexecShellSafe=";
          break;
        case cmTarget::POST_BUILD:
          *this->GetFolderBuildStreams() << "    :postexecShellSafe=";
          break;
        default:
          assert("Only pre and post are supported");
      }
      cmCustomCommandLine const& command = *commandI;
      for (cmCustomCommandLine::const_iterator commandLineI = command.begin();
           commandLineI != command.end(); ++commandLineI) {
        std::string subCommandE =
          this->LocalGenerator->EscapeForShell(*commandLineI, true);
        if (!command.empty()) {
          *this->GetFolderBuildStreams()
            << (command.begin() == commandLineI ? "'" : " ");
          // Need to double escape backslashes
          cmSystemTools::ReplaceString(subCommandE, "\\", "\\\\");
        }
        *this->GetFolderBuildStreams() << subCommandE;
      }
      if (!command.empty()) {
        *this->GetFolderBuildStreams() << "'" << std::endl;
      }
    }
  }
}

std::map<const cmSourceFile*, std::string>
cmGhsMultiTargetGenerator::GetObjectNames(
  std::vector<cmSourceFile*>* const objectSources,
  cmLocalGhsMultiGenerator* const localGhsMultiGenerator,
  cmGeneratorTarget* const generatorTarget)
{
  std::map<std::string, std::vector<cmSourceFile*> > filenameToSource;
  std::map<cmSourceFile*, std::string> sourceToFilename;
  for (std::vector<cmSourceFile*>::const_iterator sf = objectSources->begin();
       sf != objectSources->end(); ++sf) {
    const std::string filename =
      cmSystemTools::GetFilenameName((*sf)->GetFullPath());
    const std::string lower_filename = cmSystemTools::LowerCase(filename);
    filenameToSource[lower_filename].push_back(*sf);
    sourceToFilename[*sf] = lower_filename;
  }

  std::vector<cmSourceFile*> duplicateSources;
  for (std::map<std::string, std::vector<cmSourceFile*> >::const_iterator
         msvSourceI = filenameToSource.begin();
       msvSourceI != filenameToSource.end(); ++msvSourceI) {
    if (msvSourceI->second.size() > 1) {
      duplicateSources.insert(duplicateSources.end(),
                              msvSourceI->second.begin(),
                              msvSourceI->second.end());
    }
  }

  std::map<const cmSourceFile*, std::string> objectNamesCorrected;

  for (std::vector<cmSourceFile*>::const_iterator sf =
         duplicateSources.begin();
       sf != duplicateSources.end(); ++sf) {
    std::string const longestObjectDirectory(
      cmGhsMultiTargetGenerator::ComputeLongestObjectDirectory(
        localGhsMultiGenerator, generatorTarget, *sf));
    std::string objFilenameName =
      localGhsMultiGenerator->GetObjectFileNameWithoutTarget(
        **sf, longestObjectDirectory);
    cmsys::SystemTools::ReplaceString(objFilenameName, "/", "_");
    objectNamesCorrected[*sf] = objFilenameName;
  }

  return objectNamesCorrected;
}

void cmGhsMultiTargetGenerator::WriteSources(
  std::vector<cmSourceFile*> const& objectSources,
  std::map<const cmSourceFile*, std::string> const& objectNames)
{
  for (std::vector<cmSourceFile*>::const_iterator si = objectSources.begin();
       si != objectSources.end(); ++si) {
    std::vector<cmSourceGroup> sourceGroups(this->Makefile->GetSourceGroups());
    char const* sourceFullPath = (*si)->GetFullPath().c_str();
    cmSourceGroup* sourceGroup =
      this->Makefile->FindSourceGroup(sourceFullPath, sourceGroups);
    std::string sgPath(sourceGroup->GetFullName());
    cmSystemTools::ConvertToUnixSlashes(sgPath);
    cmGlobalGhsMultiGenerator::AddFilesUpToPath(
      this->GetFolderBuildStreams(), &this->FolderBuildStreams,
      this->LocalGenerator->GetBinaryDirectory(), sgPath,
      GhsMultiGpj::SUBPROJECT, this->RelBuildFilePath);

    std::string fullSourcePath((*si)->GetFullPath());
    if ((*si)->GetExtension() == "int" || (*si)->GetExtension() == "bsp") {
      *this->FolderBuildStreams[sgPath] << fullSourcePath << std::endl;
    } else {
      // WORKAROUND: GHS MULTI needs the path to use backslashes without quotes
      //  to open files in search as of version 6.1.6
      cmsys::SystemTools::ReplaceString(fullSourcePath, "/", "\\");
      *this->FolderBuildStreams[sgPath] << fullSourcePath << std::endl;
    }

    if ("ld" != (*si)->GetExtension() && "int" != (*si)->GetExtension() &&
        "bsp" != (*si)->GetExtension()) {
      this->WriteObjectLangOverride(this->FolderBuildStreams[sgPath], (*si));
      if (objectNames.end() != objectNames.find(*si)) {
        *this->FolderBuildStreams[sgPath]
          << "    -o \"" << objectNames.find(*si)->second << "\"" << std::endl;
      }

      this->WriteObjectDir(this->FolderBuildStreams[sgPath],
                           this->AbsBuildFilePath + sgPath);
    }
  }
}

void cmGhsMultiTargetGenerator::WriteObjectLangOverride(
  cmGeneratedFileStream* fileStream, cmSourceFile* sourceFile)
{
  const char* rawLangProp = sourceFile->GetProperty("LANGUAGE");
  if (NULL != rawLangProp) {
    std::string sourceLangProp(rawLangProp);
    std::string extension(sourceFile->GetExtension());
    if ("CXX" == sourceLangProp && ("c" == extension || "C" == extension)) {
      *fileStream << "    -dotciscxx" << std::endl;
    }
  }
}

void cmGhsMultiTargetGenerator::WriteObjectDir(
  cmGeneratedFileStream* fileStream, std::string const& dir)
{
  std::string workingDir(dir);
  cmSystemTools::ConvertToUnixSlashes(workingDir);
  if (!workingDir.empty()) {
    workingDir += "/";
  }
  workingDir += "Objs";
  *fileStream << "    -object_dir=\"" << workingDir << "\"" << std::endl;
}

std::string cmGhsMultiTargetGenerator::GetOutputDirectory(
  const std::string& config) const
{
  std::string outputDir(AbsBuildFilePath);

  const char* runtimeOutputProp =
    this->GeneratorTarget->GetProperty("RUNTIME_OUTPUT_DIRECTORY");
  if (NULL != runtimeOutputProp) {
    outputDir = runtimeOutputProp;
  }

  std::string configCapped(cmSystemTools::UpperCase(config));
  const char* runtimeOutputSProp = this->GeneratorTarget->GetProperty(
    "RUNTIME_OUTPUT_DIRECTORY_" + configCapped);
  if (NULL != runtimeOutputSProp) {
    outputDir = runtimeOutputSProp;
  }
  cmSystemTools::ConvertToUnixSlashes(outputDir);

  if (!outputDir.empty()) {
    outputDir += "/";
  }

  return outputDir;
}

std::string cmGhsMultiTargetGenerator::GetOutputFilename(
  const std::string& config) const
{
  std::string outputFilename(this->GeneratorTarget->GetName());

  const char* outputNameProp =
    this->GeneratorTarget->GetProperty("OUTPUT_NAME");
  if (NULL != outputNameProp) {
    outputFilename = outputNameProp;
  }

  std::string configCapped(cmSystemTools::UpperCase(config));
  const char* outputNameSProp =
    this->GeneratorTarget->GetProperty(configCapped + "_OUTPUT_NAME");
  if (NULL != outputNameSProp) {
    outputFilename = outputNameSProp;
  }

  return outputFilename;
}

std::string cmGhsMultiTargetGenerator::ComputeLongestObjectDirectory(
  cmLocalGhsMultiGenerator const* localGhsMultiGenerator,
  cmGeneratorTarget* const generatorTarget, cmSourceFile* const sourceFile)
{
  std::string dir_max;
  dir_max +=
    localGhsMultiGenerator->GetMakefile()->GetCurrentBinaryDirectory();
  dir_max += "/";
  dir_max += generatorTarget->Target->GetName();
  dir_max += "/";
  std::vector<cmSourceGroup> sourceGroups(
    localGhsMultiGenerator->GetMakefile()->GetSourceGroups());
  char const* const sourceFullPath = sourceFile->GetFullPath().c_str();
  cmSourceGroup* sourceGroup =
    localGhsMultiGenerator->GetMakefile()->FindSourceGroup(sourceFullPath,
                                                           sourceGroups);
  std::string const sgPath(sourceGroup->GetFullName());
  dir_max += sgPath;
  dir_max += "/Objs/libs/";
  dir_max += generatorTarget->Target->GetName();
  dir_max += "/";
  return dir_max;
}

bool cmGhsMultiTargetGenerator::IsNotKernel(std::string const& config,
                                            const std::string& language)
{
  bool output;
  std::vector<std::string> options;
  this->GeneratorTarget->GetCompileOptions(options, config, language);
  output =
    options.end() == std::find(options.begin(), options.end(), "-kernel");
  return output;
}

bool cmGhsMultiTargetGenerator::DetermineIfTargetGroup(
  const cmGeneratorTarget* target)
{
  bool output = false;
  std::vector<cmSourceFile*> sources;
  std::string config =
    target->Target->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");
  target->GetSourceFiles(sources, config);
  for (std::vector<cmSourceFile*>::const_iterator sources_i = sources.begin();
       sources.end() != sources_i; ++sources_i) {
    if ("int" == (*sources_i)->GetExtension()) {
      output = true;
    }
  }
  return output;
}

bool cmGhsMultiTargetGenerator::DetermineIfDynamicDownload(
  std::string const& config, const std::string& language)
{
  std::vector<std::string> options;
  bool output = false;
  this->GeneratorTarget->GetCompileOptions(options, config, language);
  for (std::vector<std::string>::const_iterator options_i = options.begin();
       options_i != options.end(); ++options_i) {
    std::string option = *options_i;
    if (this->DDOption == option) {
      output = true;
    }
  }
  return output;
}
