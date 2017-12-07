/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExtraSublimeTextGenerator.h"

#include "cmsys/RegularExpression.hxx"
#include <set>
#include <sstream>
#include <string.h>
#include <utility>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

/*
Sublime Text 2 Generator
Author: Morné Chamberlain
This generator was initially based off of the CodeBlocks generator.

Some useful URLs:
Homepage:
http://www.sublimetext.com/

File format docs:
http://www.sublimetext.com/docs/2/projects.html
http://sublimetext.info/docs/en/reference/build_systems.html
*/

cmExternalMakefileProjectGeneratorFactory*
cmExtraSublimeTextGenerator::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmExtraSublimeTextGenerator>
    factory("Sublime Text 2", "Generates Sublime Text 2 project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
#if defined(_WIN32)
    factory.AddSupportedGlobalGenerator("MinGW Makefiles");
    factory.AddSupportedGlobalGenerator("NMake Makefiles");
// disable until somebody actually tests it:
// factory.AddSupportedGlobalGenerator("MSYS Makefiles");
#endif
    factory.AddSupportedGlobalGenerator("Ninja");
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
  }

  return &factory;
}

cmExtraSublimeTextGenerator::cmExtraSublimeTextGenerator()
  : cmExternalMakefileProjectGenerator()
{
  this->ExcludeBuildFolder = false;
}

void cmExtraSublimeTextGenerator::Generate()
{
  this->ExcludeBuildFolder = this->GlobalGenerator->GlobalSettingIsOn(
    "CMAKE_SUBLIME_TEXT_2_EXCLUDE_BUILD_TREE");
  this->EnvSettings = this->GlobalGenerator->GetSafeGlobalSetting(
    "CMAKE_SUBLIME_TEXT_2_ENV_SETTINGS");

  // for each sub project in the project create a sublime text 2 project
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); ++it) {
    // create a project file
    this->CreateProjectFile(it->second);
  }
}

void cmExtraSublimeTextGenerator::CreateProjectFile(
  const std::vector<cmLocalGenerator*>& lgs)
{
  std::string outputDir = lgs[0]->GetCurrentBinaryDirectory();
  std::string projectName = lgs[0]->GetProjectName();

  const std::string filename =
    outputDir + "/" + projectName + ".sublime-project";

  this->CreateNewProjectFile(lgs, filename);
}

void cmExtraSublimeTextGenerator::CreateNewProjectFile(
  const std::vector<cmLocalGenerator*>& lgs, const std::string& filename)
{
  const cmMakefile* mf = lgs[0]->GetMakefile();

  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  const std::string& sourceRootRelativeToOutput = cmSystemTools::RelativePath(
    lgs[0]->GetBinaryDirectory(), lgs[0]->GetSourceDirectory());
  // Write the folder entries to the project file
  fout << "{\n";
  fout << "\t\"folders\":\n\t[\n\t";
  if (!sourceRootRelativeToOutput.empty()) {
    fout << "\t{\n\t\t\t\"path\": \"" << sourceRootRelativeToOutput << "\"";
    const std::string& outputRelativeToSourceRoot =
      cmSystemTools::RelativePath(lgs[0]->GetSourceDirectory(),
                                  lgs[0]->GetBinaryDirectory());
    if ((!outputRelativeToSourceRoot.empty()) &&
        ((outputRelativeToSourceRoot.length() < 3) ||
         (outputRelativeToSourceRoot.substr(0, 3) != "../"))) {
      if (this->ExcludeBuildFolder) {
        fout << ",\n\t\t\t\"folder_exclude_patterns\": [\""
             << outputRelativeToSourceRoot << "\"]";
      }
    }
  } else {
    fout << "\t{\n\t\t\t\"path\": \"./\"";
  }
  fout << "\n\t\t}";
  // End of the folders section
  fout << "\n\t]";

  // Write the beginning of the build systems section to the project file
  fout << ",\n\t\"build_systems\":\n\t[\n\t";

  // Set of include directories over all targets (sublime text/sublimeclang
  // doesn't currently support these settings per build system, only project
  // wide
  MapSourceFileFlags sourceFileFlags;
  AppendAllTargets(lgs, mf, fout, sourceFileFlags);

  // End of build_systems
  fout << "\n\t]";
  std::string systemName = mf->GetSafeDefinition("CMAKE_SYSTEM_NAME");
  std::vector<std::string> tokens;
  cmSystemTools::ExpandListArgument(this->EnvSettings, tokens);

  if (!this->EnvSettings.empty()) {
    fout << ",";
    fout << "\n\t\"env\":";
    fout << "\n\t{";
    fout << "\n\t\t" << systemName << ":";
    fout << "\n\t\t{";
    for (std::vector<std::string>::iterator i = tokens.begin();
         i != tokens.end(); ++i) {
      size_t const pos = i->find_first_of('=');

      if (pos != std::string::npos) {
        std::string varName = i->substr(0, pos);
        std::string varValue = i->substr(pos + 1);

        fout << "\n\t\t\t\"" << varName << "\":\"" << varValue << "\"";
      } else {
        std::ostringstream e;
        e << "Could not parse Env Vars specified in "
             "\"CMAKE_SUBLIME_TEXT_2_ENV_SETTINGS\""
          << ", corrupted string " << *i;
        mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      }
    }
    fout << "\n\t\t}";
    fout << "\n\t}";
  }
  fout << "\n}";
}

void cmExtraSublimeTextGenerator::AppendAllTargets(
  const std::vector<cmLocalGenerator*>& lgs, const cmMakefile* mf,
  cmGeneratedFileStream& fout, MapSourceFileFlags& sourceFileFlags)
{
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string compiler;
  if (!lgs.empty()) {
    this->AppendTarget(fout, "all", lgs[0], CM_NULLPTR, make.c_str(), mf,
                       compiler.c_str(), sourceFileFlags, true);
    this->AppendTarget(fout, "clean", lgs[0], CM_NULLPTR, make.c_str(), mf,
                       compiler.c_str(), sourceFileFlags, false);
  }

  // add all executable and library targets and some of the GLOBAL
  // and UTILITY targets
  for (std::vector<cmLocalGenerator*>::const_iterator lg = lgs.begin();
       lg != lgs.end(); lg++) {
    cmMakefile* makefile = (*lg)->GetMakefile();
    std::vector<cmGeneratorTarget*> targets = (*lg)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin();
         ti != targets.end(); ti++) {
      std::string targetName = (*ti)->GetName();
      switch ((*ti)->GetType()) {
        case cmStateEnums::GLOBAL_TARGET: {
          // Only add the global targets from CMAKE_BINARY_DIR,
          // not from the subdirs
          if (strcmp((*lg)->GetCurrentBinaryDirectory(),
                     (*lg)->GetBinaryDirectory()) == 0) {
            this->AppendTarget(fout, targetName, *lg, CM_NULLPTR, make.c_str(),
                               makefile, compiler.c_str(), sourceFileFlags,
                               false);
          }
        } break;
        case cmStateEnums::UTILITY:
          // Add all utility targets, except the Nightly/Continuous/
          // Experimental-"sub"targets as e.g. NightlyStart
          if (((targetName.find("Nightly") == 0) &&
               (targetName != "Nightly")) ||
              ((targetName.find("Continuous") == 0) &&
               (targetName != "Continuous")) ||
              ((targetName.find("Experimental") == 0) &&
               (targetName != "Experimental"))) {
            break;
          }

          this->AppendTarget(fout, targetName, *lg, CM_NULLPTR, make.c_str(),
                             makefile, compiler.c_str(), sourceFileFlags,
                             false);
          break;
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY: {
          this->AppendTarget(fout, targetName, *lg, *ti, make.c_str(),
                             makefile, compiler.c_str(), sourceFileFlags,
                             false);
          std::string fastTarget = targetName;
          fastTarget += "/fast";
          this->AppendTarget(fout, fastTarget, *lg, *ti, make.c_str(),
                             makefile, compiler.c_str(), sourceFileFlags,
                             false);
        } break;
        default:
          break;
      }
    }
  }
}

void cmExtraSublimeTextGenerator::AppendTarget(
  cmGeneratedFileStream& fout, const std::string& targetName,
  cmLocalGenerator* lg, cmGeneratorTarget* target, const char* make,
  const cmMakefile* makefile, const char* /*compiler*/,
  MapSourceFileFlags& sourceFileFlags, bool firstTarget)
{

  if (target != CM_NULLPTR) {
    std::vector<cmSourceFile*> sourceFiles;
    target->GetSourceFiles(sourceFiles,
                           makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
    std::vector<cmSourceFile*>::const_iterator sourceFilesEnd =
      sourceFiles.end();
    for (std::vector<cmSourceFile*>::const_iterator iter = sourceFiles.begin();
         iter != sourceFilesEnd; ++iter) {
      cmSourceFile* sourceFile = *iter;
      MapSourceFileFlags::iterator sourceFileFlagsIter =
        sourceFileFlags.find(sourceFile->GetFullPath());
      if (sourceFileFlagsIter == sourceFileFlags.end()) {
        sourceFileFlagsIter =
          sourceFileFlags
            .insert(MapSourceFileFlags::value_type(sourceFile->GetFullPath(),
                                                   std::vector<std::string>()))
            .first;
      }
      std::vector<std::string>& flags = sourceFileFlagsIter->second;
      std::string flagsString = this->ComputeFlagsForObject(*iter, lg, target);
      std::string definesString = this->ComputeDefines(*iter, lg, target);
      flags.clear();
      cmsys::RegularExpression flagRegex;
      // Regular expression to extract compiler flags from a string
      // https://gist.github.com/3944250
      const char* regexString =
        "(^|[ ])-[DIOUWfgs][^= ]+(=\\\"[^\"]+\\\"|=[^\"][^ ]+)?";
      flagRegex.compile(regexString);
      std::string workString = flagsString + " " + definesString;
      while (flagRegex.find(workString)) {
        std::string::size_type start = flagRegex.start();
        if (workString[start] == ' ') {
          start++;
        }
        flags.push_back(workString.substr(start, flagRegex.end() - start));
        if (flagRegex.end() < workString.size()) {
          workString = workString.substr(flagRegex.end());
        } else {
          workString = "";
        }
      }
    }
  }

  // Ninja uses ninja.build files (look for a way to get the output file name
  // from cmMakefile or something)
  std::string makefileName;
  if (this->GlobalGenerator->GetName() == "Ninja") {
    makefileName = "build.ninja";
  } else {
    makefileName = "Makefile";
  }
  if (!firstTarget) {
    fout << ",\n\t";
  }
  fout << "\t{\n\t\t\t\"name\": \"" << lg->GetProjectName() << " - "
       << targetName << "\",\n";
  fout << "\t\t\t\"cmd\": ["
       << this->BuildMakeCommand(make, makefileName.c_str(), targetName)
       << "],\n";
  fout << "\t\t\t\"working_dir\": \"${project_path}\",\n";
  fout << "\t\t\t\"file_regex\": \""
          "^(..[^:]*)(?::|\\\\()([0-9]+)(?::|\\\\))(?:([0-9]+):)?\\\\s*(.*)"
          "\"\n";
  fout << "\t\t}";
}

// Create the command line for building the given target using the selected
// make
std::string cmExtraSublimeTextGenerator::BuildMakeCommand(
  const std::string& make, const char* makefile, const std::string& target)
{
  std::string command = "\"";
  command += make + "\"";
  std::string generator = this->GlobalGenerator->GetName();
  if (generator == "NMake Makefiles") {
    std::string makefileName = cmSystemTools::ConvertToOutputPath(makefile);
    command += ", \"/NOLOGO\", \"/f\", \"";
    command += makefileName + "\"";
    command += ", \"" + target + "\"";
  } else if (generator == "Ninja") {
    std::string makefileName = cmSystemTools::ConvertToOutputPath(makefile);
    command += ", \"-f\", \"";
    command += makefileName + "\"";
    command += ", \"" + target + "\"";
  } else {
    std::string makefileName;
    if (generator == "MinGW Makefiles") {
      // no escaping of spaces in this case, see
      // https://gitlab.kitware.com/cmake/cmake/issues/10014
      makefileName = makefile;
    } else {
      makefileName = cmSystemTools::ConvertToOutputPath(makefile);
    }
    command += ", \"-f\", \"";
    command += makefileName + "\"";
    command += ", \"" + target + "\"";
  }
  return command;
}

// TODO: Most of the code is picked up from the Ninja generator, refactor it.
std::string cmExtraSublimeTextGenerator::ComputeFlagsForObject(
  cmSourceFile* source, cmLocalGenerator* lg, cmGeneratorTarget* gtgt)
{
  std::string flags;
  std::string language = source->GetLanguage();
  if (language.empty()) {
    language = "C";
  }
  std::string const& config =
    lg->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");

  lg->GetTargetCompileFlags(gtgt, config, language, flags);

  // Add include directory flags.
  {
    std::vector<std::string> includes;
    lg->GetIncludeDirectories(includes, gtgt, language, config);
    std::string includeFlags = lg->GetIncludeFlags(includes, gtgt, language,
                                                   true); // full include paths
    lg->AppendFlags(flags, includeFlags);
  }

  // Add source file specific flags.
  if (const char* cflags = source->GetProperty("COMPILE_FLAGS")) {
    cmGeneratorExpression ge;
    const char* processed = ge.Parse(cflags)->Evaluate(lg, config);
    lg->AppendFlags(flags, processed);
  }

  return flags;
}

// TODO: Refactor with
// void cmMakefileTargetGenerator::WriteTargetLanguageFlags().
std::string cmExtraSublimeTextGenerator::ComputeDefines(
  cmSourceFile* source, cmLocalGenerator* lg, cmGeneratorTarget* target)

{
  std::set<std::string> defines;
  cmMakefile* makefile = lg->GetMakefile();
  const std::string& language = source->GetLanguage();
  const std::string& config = makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");

  // Add the export symbol definition for shared library objects.
  if (const char* exportMacro = target->GetExportMacro()) {
    lg->AppendDefines(defines, exportMacro);
  }

  // Add preprocessor definitions for this target and configuration.
  lg->AddCompileDefinitions(defines, target, config, language);
  lg->AppendDefines(defines, source->GetProperty("COMPILE_DEFINITIONS"));
  {
    std::string defPropName = "COMPILE_DEFINITIONS_";
    defPropName += cmSystemTools::UpperCase(config);
    lg->AppendDefines(defines, source->GetProperty(defPropName));
  }

  std::string definesString;
  lg->JoinDefines(defines, definesString, language);

  return definesString;
}
