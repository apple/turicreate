/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExtraCodeLiteGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"
#include "cmake.h"

#include "cmsys/SystemInformation.hxx"
#include <map>
#include <set>
#include <sstream>
#include <string.h>
#include <utility>

cmExtraCodeLiteGenerator::cmExtraCodeLiteGenerator()
  : cmExternalMakefileProjectGenerator()
  , ConfigName("NoConfig")
  , CpuCount(2)
{
}

cmExternalMakefileProjectGeneratorFactory*
cmExtraCodeLiteGenerator::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmExtraCodeLiteGenerator>
    factory("CodeLite", "Generates CodeLite project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
#if defined(_WIN32)
    factory.AddSupportedGlobalGenerator("MinGW Makefiles");
    factory.AddSupportedGlobalGenerator("NMake Makefiles");
#endif
    factory.AddSupportedGlobalGenerator("Ninja");
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
  }

  return &factory;
}

void cmExtraCodeLiteGenerator::Generate()
{
  // Hold root tree information for creating the workspace
  std::string workspaceProjectName;
  std::string workspaceOutputDir;
  std::string workspaceFileName;
  std::string workspaceSourcePath;

  const std::map<std::string, std::vector<cmLocalGenerator*> >& projectMap =
    this->GlobalGenerator->GetProjectMap();

  // loop projects and locate the root project.
  // and extract the information for creating the worspace
  // root makefile
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = projectMap.begin();
       it != projectMap.end(); ++it) {
    const cmMakefile* mf = it->second[0]->GetMakefile();
    this->ConfigName = GetConfigurationName(mf);

    if (strcmp(it->second[0]->GetCurrentBinaryDirectory(),
               it->second[0]->GetBinaryDirectory()) == 0) {
      workspaceOutputDir = it->second[0]->GetCurrentBinaryDirectory();
      workspaceProjectName = it->second[0]->GetProjectName();
      workspaceSourcePath = it->second[0]->GetSourceDirectory();
      workspaceFileName = workspaceOutputDir + "/";
      workspaceFileName += workspaceProjectName + ".workspace";
      this->WorkspacePath = it->second[0]->GetCurrentBinaryDirectory();
      ;
      break;
    }
  }

  cmGeneratedFileStream fout(workspaceFileName.c_str());
  cmXMLWriter xml(fout);

  xml.StartDocument("utf-8");
  xml.StartElement("CodeLite_Workspace");
  xml.Attribute("Name", workspaceProjectName);

  bool const targetsAreProjects =
    this->GlobalGenerator->GlobalSettingIsOn("CMAKE_CODELITE_USE_TARGETS");

  std::vector<std::string> ProjectNames;
  if (targetsAreProjects) {
    ProjectNames = CreateProjectsByTarget(&xml);
  } else {
    ProjectNames = CreateProjectsByProjectMaps(&xml);
  }

  xml.StartElement("BuildMatrix");
  xml.StartElement("WorkspaceConfiguration");
  xml.Attribute("Name", this->ConfigName);
  xml.Attribute("Selected", "yes");

  for (std::vector<std::string>::iterator it(ProjectNames.begin());
       it != ProjectNames.end(); it++) {
    xml.StartElement("Project");
    xml.Attribute("Name", *it);
    xml.Attribute("ConfigName", this->ConfigName);
    xml.EndElement();
  }

  xml.EndElement(); // WorkspaceConfiguration
  xml.EndElement(); // BuildMatrix
  xml.EndElement(); // CodeLite_Workspace
}

// Create projects where targets are the projects
std::vector<std::string> cmExtraCodeLiteGenerator::CreateProjectsByTarget(
  cmXMLWriter* xml)
{
  std::vector<std::string> retval;
  // for each target in the workspace create a codelite project
  const std::vector<cmLocalGenerator*>& lgs =
    this->GlobalGenerator->GetLocalGenerators();
  for (std::vector<cmLocalGenerator*>::const_iterator lg(lgs.begin());
       lg != lgs.end(); lg++) {
    for (std::vector<cmGeneratorTarget*>::const_iterator lt =
           (*lg)->GetGeneratorTargets().begin();
         lt != (*lg)->GetGeneratorTargets().end(); lt++) {
      cmStateEnums::TargetType type = (*lt)->GetType();
      std::string outputDir = (*lg)->GetCurrentBinaryDirectory();
      std::string targetName = (*lt)->GetName();
      std::string filename = outputDir + "/" + targetName + ".project";
      retval.push_back(targetName);
      // Make the project file relative to the workspace
      std::string relafilename = cmSystemTools::RelativePath(
        this->WorkspacePath.c_str(), filename.c_str());
      std::string visualname = targetName;
      switch (type) {
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
          visualname = "lib" + visualname;
          CM_FALLTHROUGH;
        case cmStateEnums::EXECUTABLE:
          xml->StartElement("Project");
          xml->Attribute("Name", visualname);
          xml->Attribute("Path", relafilename);
          xml->Attribute("Active", "No");
          xml->EndElement();

          CreateNewProjectFile(*lt, filename);
          break;
        default:
          break;
      }
    }
  }
  return retval;
}

// The "older way of doing it.
std::vector<std::string> cmExtraCodeLiteGenerator::CreateProjectsByProjectMaps(
  cmXMLWriter* xml)
{
  std::vector<std::string> retval;
  // for each sub project in the workspace create a codelite project
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); it++) {

    std::string outputDir = it->second[0]->GetCurrentBinaryDirectory();
    std::string projectName = it->second[0]->GetProjectName();
    retval.push_back(projectName);
    std::string filename = outputDir + "/" + projectName + ".project";

    // Make the project file relative to the workspace
    filename = cmSystemTools::RelativePath(this->WorkspacePath.c_str(),
                                           filename.c_str());

    // create a project file
    this->CreateProjectFile(it->second);
    xml->StartElement("Project");
    xml->Attribute("Name", projectName);
    xml->Attribute("Path", filename);
    xml->Attribute("Active", "No");
    xml->EndElement();
  }
  return retval;
}

/* create the project file */
void cmExtraCodeLiteGenerator::CreateProjectFile(
  const std::vector<cmLocalGenerator*>& lgs)
{
  std::string outputDir = lgs[0]->GetCurrentBinaryDirectory();
  std::string projectName = lgs[0]->GetProjectName();
  std::string filename = outputDir + "/";

  filename += projectName + ".project";
  this->CreateNewProjectFile(lgs, filename);
}

std::string cmExtraCodeLiteGenerator::CollectSourceFiles(
  const cmMakefile* makefile, const cmGeneratorTarget* gt,
  std::map<std::string, cmSourceFile*>& cFiles,
  std::set<std::string>& otherFiles)
{
  const std::vector<std::string>& srcExts =
    this->GlobalGenerator->GetCMakeInstance()->GetSourceExtensions();

  std::string projectType;
  switch (gt->GetType()) {
    case cmStateEnums::EXECUTABLE: {
      projectType = "Executable";
    } break;
    case cmStateEnums::STATIC_LIBRARY: {
      projectType = "Static Library";
    } break;
    case cmStateEnums::SHARED_LIBRARY: {
      projectType = "Dynamic Library";
    } break;
    case cmStateEnums::MODULE_LIBRARY: {
      projectType = "Dynamic Library";
    } break;
    default: // intended fallthrough
      break;
  }

  switch (gt->GetType()) {
    case cmStateEnums::EXECUTABLE:
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY: {
      std::vector<cmSourceFile*> sources;
      gt->GetSourceFiles(sources,
                         makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
      for (std::vector<cmSourceFile*>::const_iterator si = sources.begin();
           si != sources.end(); si++) {
        // check whether it is a C/C++ implementation file
        bool isCFile = false;
        std::string lang = (*si)->GetLanguage();
        if (lang == "C" || lang == "CXX") {
          std::string const& srcext = (*si)->GetExtension();
          for (std::vector<std::string>::const_iterator ext = srcExts.begin();
               ext != srcExts.end(); ++ext) {
            if (srcext == *ext) {
              isCFile = true;
              break;
            }
          }
        }

        // then put it accordingly into one of the two containers
        if (isCFile) {
          cFiles[(*si)->GetFullPath()] = *si;
        } else {
          otherFiles.insert((*si)->GetFullPath());
        }
      }
    }
    default: // intended fallthrough
      break;
  }
  return projectType;
}

void cmExtraCodeLiteGenerator::CreateNewProjectFile(
  const std::vector<cmLocalGenerator*>& lgs, const std::string& filename)
{
  const cmMakefile* mf = lgs[0]->GetMakefile();
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }
  cmXMLWriter xml(fout);

  ////////////////////////////////////
  xml.StartDocument("utf-8");
  xml.StartElement("CodeLite_Project");
  xml.Attribute("Name", lgs[0]->GetProjectName());
  xml.Attribute("InternalType", "");

  std::string projectType;

  // Collect all used source files in the project
  // Sort them into two containers, one for C/C++ implementation files
  // which may have an acompanying header, one for all other files
  std::map<std::string, cmSourceFile*> cFiles;
  std::set<std::string> otherFiles;

  for (std::vector<cmLocalGenerator*>::const_iterator lg = lgs.begin();
       lg != lgs.end(); lg++) {
    cmMakefile* makefile = (*lg)->GetMakefile();
    std::vector<cmGeneratorTarget*> targets = (*lg)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin();
         ti != targets.end(); ti++) {
      projectType = CollectSourceFiles(makefile, *ti, cFiles, otherFiles);
    }
  }

  // Get the project path ( we need it later to convert files to
  // their relative path)
  std::string projectPath = cmSystemTools::GetFilenamePath(filename);

  CreateProjectSourceEntries(cFiles, otherFiles, &xml, projectPath, mf,
                             projectType, "");

  xml.EndElement(); // CodeLite_Project
}

void cmExtraCodeLiteGenerator::FindMatchingHeaderfiles(
  std::map<std::string, cmSourceFile*>& cFiles,
  std::set<std::string>& otherFiles)
{

  const std::vector<std::string>& headerExts =
    this->GlobalGenerator->GetCMakeInstance()->GetHeaderExtensions();

  // The following loop tries to add header files matching to implementation
  // files to the project. It does that by iterating over all source files,
  // replacing the file name extension with ".h" and checks whether such a
  // file exists. If it does, it is inserted into the map of files.
  // A very similar version of that code exists also in the kdevelop
  // project generator.
  for (std::map<std::string, cmSourceFile*>::const_iterator sit =
         cFiles.begin();
       sit != cFiles.end(); ++sit) {
    std::string headerBasename = cmSystemTools::GetFilenamePath(sit->first);
    headerBasename += "/";
    headerBasename += cmSystemTools::GetFilenameWithoutExtension(sit->first);

    // check if there's a matching header around
    for (std::vector<std::string>::const_iterator ext = headerExts.begin();
         ext != headerExts.end(); ++ext) {
      std::string hname = headerBasename;
      hname += ".";
      hname += *ext;
      // if it's already in the set, don't check if it exists on disk
      std::set<std::string>::const_iterator headerIt = otherFiles.find(hname);
      if (headerIt != otherFiles.end()) {
        break;
      }

      if (cmSystemTools::FileExists(hname.c_str())) {
        otherFiles.insert(hname);
        break;
      }
    }
  }
}

void cmExtraCodeLiteGenerator::CreateFoldersAndFiles(
  std::set<std::string>& cFiles, cmXMLWriter& xml,
  const std::string& projectPath)
{
  std::vector<std::string> tmp_path;
  std::vector<std::string> components;
  size_t numOfEndEl = 0;

  for (std::set<std::string>::const_iterator it = cFiles.begin();
       it != cFiles.end(); ++it) {
    std::string frelapath =
      cmSystemTools::RelativePath(projectPath.c_str(), it->c_str());
    cmsys::SystemTools::SplitPath(frelapath, components, false);
    components.pop_back(); // erase last member -> it is file, not folder
    components.erase(components.begin()); // erase "root"

    size_t sizeOfSkip = 0;

    for (size_t i = 0; i < components.size(); ++i) {
      // skip relative path
      if (components[i] == ".." || components[i] == ".") {
        sizeOfSkip++;
        continue;
      }

      // same folder
      if (tmp_path.size() > i - sizeOfSkip &&
          tmp_path[i - sizeOfSkip] == components[i]) {
        continue;
      }

      // delete "old" subfolders
      if (tmp_path.size() > i - sizeOfSkip) {
        numOfEndEl = tmp_path.size() - i + sizeOfSkip;
        tmp_path.erase(tmp_path.end() - numOfEndEl, tmp_path.end());
        for (; numOfEndEl--;) {
          xml.EndElement();
        }
      }

      // add folder
      xml.StartElement("VirtualDirectory");
      xml.Attribute("Name", components[i]);
      tmp_path.push_back(components[i]);
    }

    // delete "old" subfolders
    numOfEndEl = tmp_path.size() - components.size() + sizeOfSkip;
    if (numOfEndEl) {
      tmp_path.erase(tmp_path.end() - numOfEndEl, tmp_path.end());
      for (; numOfEndEl--;) {
        xml.EndElement();
      }
    }

    // add file
    xml.StartElement("File");
    xml.Attribute("Name", frelapath);
    xml.EndElement();
  }

  // end of folders
  numOfEndEl = tmp_path.size();
  for (; numOfEndEl--;) {
    xml.EndElement();
  }
}

void cmExtraCodeLiteGenerator::CreateFoldersAndFiles(
  std::map<std::string, cmSourceFile*>& cFiles, cmXMLWriter& xml,
  const std::string& projectPath)
{
  std::set<std::string> s;
  for (std::map<std::string, cmSourceFile*>::const_iterator it =
         cFiles.begin();
       it != cFiles.end(); ++it) {
    s.insert(it->first);
  }
  this->CreateFoldersAndFiles(s, xml, projectPath);
}

void cmExtraCodeLiteGenerator::CreateProjectSourceEntries(
  std::map<std::string, cmSourceFile*>& cFiles,
  std::set<std::string>& otherFiles, cmXMLWriter* _xml,
  const std::string& projectPath, const cmMakefile* mf,
  const std::string& projectType, const std::string& targetName)
{

  cmXMLWriter& xml(*_xml);
  FindMatchingHeaderfiles(cFiles, otherFiles);
  // Create 2 virtual folders: src and include
  // and place all the implementation files into the src
  // folder, the rest goes to the include folder
  xml.StartElement("VirtualDirectory");
  xml.Attribute("Name", "src");

  // insert all source files in the codelite project
  // first the C/C++ implementation files, then all others
  this->CreateFoldersAndFiles(cFiles, xml, projectPath);
  xml.EndElement(); // VirtualDirectory

  xml.StartElement("VirtualDirectory");
  xml.Attribute("Name", "include");
  this->CreateFoldersAndFiles(otherFiles, xml, projectPath);
  xml.EndElement(); // VirtualDirectory

  // Get the number of CPUs. We use this information for the make -jN
  // command
  cmsys::SystemInformation info;
  info.RunCPUCheck();

  this->CpuCount =
    info.GetNumberOfLogicalCPU() * info.GetNumberOfPhysicalCPU();

  std::string codeliteCompilerName = this->GetCodeLiteCompilerName(mf);

  xml.StartElement("Settings");
  xml.Attribute("Type", projectType);

  xml.StartElement("Configuration");
  xml.Attribute("Name", this->ConfigName);
  xml.Attribute("CompilerType", this->GetCodeLiteCompilerName(mf));
  xml.Attribute("DebuggerType", "GNU gdb debugger");
  xml.Attribute("Type", projectType);
  xml.Attribute("BuildCmpWithGlobalSettings", "append");
  xml.Attribute("BuildLnkWithGlobalSettings", "append");
  xml.Attribute("BuildResWithGlobalSettings", "append");

  xml.StartElement("Compiler");
  xml.Attribute("Options", "-g");
  xml.Attribute("Required", "yes");
  xml.Attribute("PreCompiledHeader", "");
  xml.StartElement("IncludePath");
  xml.Attribute("Value", ".");
  xml.EndElement(); // IncludePath
  xml.EndElement(); // Compiler

  xml.StartElement("Linker");
  xml.Attribute("Options", "");
  xml.Attribute("Required", "yes");
  xml.EndElement(); // Linker

  xml.StartElement("ResourceCompiler");
  xml.Attribute("Options", "");
  xml.Attribute("Required", "no");
  xml.EndElement(); // ResourceCompiler

  xml.StartElement("General");
  std::string outputPath = mf->GetSafeDefinition("EXECUTABLE_OUTPUT_PATH");
  std::string relapath;
  if (!outputPath.empty()) {
    relapath = cmSystemTools::RelativePath(this->WorkspacePath.c_str(),
                                           outputPath.c_str());
    xml.Attribute("OutputFile", relapath + "/$(ProjectName)");
  } else {
    xml.Attribute("OutputFile", "$(IntermediateDirectory)/$(ProjectName)");
  }
  xml.Attribute("IntermediateDirectory", "./");
  xml.Attribute("Command", "./$(ProjectName)");
  xml.Attribute("CommandArguments", "");
  if (!outputPath.empty()) {
    xml.Attribute("WorkingDirectory", relapath);
  } else {
    xml.Attribute("WorkingDirectory", "$(IntermediateDirectory)");
  }
  xml.Attribute("PauseExecWhenProcTerminates", "yes");
  xml.EndElement(); // General

  xml.StartElement("Debugger");
  xml.Attribute("IsRemote", "no");
  xml.Attribute("RemoteHostName", "");
  xml.Attribute("RemoteHostPort", "");
  xml.Attribute("DebuggerPath", "");
  xml.Element("PostConnectCommands");
  xml.Element("StartupCommands");
  xml.EndElement(); // Debugger

  xml.Element("PreBuild");
  xml.Element("PostBuild");

  xml.StartElement("CustomBuild");
  xml.Attribute("Enabled", "yes");
  xml.Element("RebuildCommand", GetRebuildCommand(mf, targetName));
  xml.Element("CleanCommand", GetCleanCommand(mf, targetName));
  xml.Element("BuildCommand", GetBuildCommand(mf, targetName));
  xml.Element("SingleFileCommand", GetSingleFileBuildCommand(mf));
  xml.Element("PreprocessFileCommand");
  xml.Element("WorkingDirectory", "$(WorkspacePath)");
  xml.EndElement(); // CustomBuild

  xml.StartElement("AdditionalRules");
  xml.Element("CustomPostBuild");
  xml.Element("CustomPreBuild");
  xml.EndElement(); // AdditionalRules

  xml.EndElement(); // Configuration
  xml.StartElement("GlobalSettings");

  xml.StartElement("Compiler");
  xml.Attribute("Options", "");
  xml.StartElement("IncludePath");
  xml.Attribute("Value", ".");
  xml.EndElement(); // IncludePath
  xml.EndElement(); // Compiler

  xml.StartElement("Linker");
  xml.Attribute("Options", "");
  xml.StartElement("LibraryPath");
  xml.Attribute("Value", ".");
  xml.EndElement(); // LibraryPath
  xml.EndElement(); // Linker

  xml.StartElement("ResourceCompiler");
  xml.Attribute("Options", "");
  xml.EndElement(); // ResourceCompiler

  xml.EndElement(); // GlobalSettings
  xml.EndElement(); // Settings
}

void cmExtraCodeLiteGenerator::CreateNewProjectFile(
  const cmGeneratorTarget* gt, const std::string& filename)
{
  const cmMakefile* mf = gt->Makefile;
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }
  cmXMLWriter xml(fout);

  ////////////////////////////////////
  xml.StartDocument("utf-8");
  xml.StartElement("CodeLite_Project");
  std::string targetName = gt->GetName();
  std::string visualname = targetName;
  switch (gt->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
      visualname = "lib" + targetName;
    default: // intended fallthrough
      break;
  }
  xml.Attribute("Name", visualname);
  xml.Attribute("InternalType", "");

  // Collect all used source files in the project
  // Sort them into two containers, one for C/C++ implementation files
  // which may have an acompanying header, one for all other files
  std::string projectType;

  std::map<std::string, cmSourceFile*> cFiles;
  std::set<std::string> otherFiles;

  projectType = CollectSourceFiles(mf, gt, cFiles, otherFiles);

  // Get the project path ( we need it later to convert files to
  // their relative path)
  std::string projectPath = cmSystemTools::GetFilenamePath(filename);

  CreateProjectSourceEntries(cFiles, otherFiles, &xml, projectPath, mf,
                             projectType, targetName);

  xml.EndElement(); // CodeLite_Project
}

std::string cmExtraCodeLiteGenerator::GetCodeLiteCompilerName(
  const cmMakefile* mf) const
{
  // figure out which language to use
  // for now care only for C and C++
  std::string compilerIdVar = "CMAKE_CXX_COMPILER_ID";
  if (!this->GlobalGenerator->GetLanguageEnabled("CXX")) {
    compilerIdVar = "CMAKE_C_COMPILER_ID";
  }

  std::string compilerId = mf->GetSafeDefinition(compilerIdVar);
  std::string compiler = "gnu g++"; // default to g++

  // Since we need the compiler for parsing purposes only
  // it does not matter if we use clang or clang++, same as
  // "gnu gcc" vs "gnu g++"
  if (compilerId == "MSVC") {
    compiler = "VC++";
  } else if (compilerId == "Clang") {
    compiler = "clang++";
  } else if (compilerId == "GNU") {
    compiler = "gnu g++";
  }
  return compiler;
}

std::string cmExtraCodeLiteGenerator::GetConfigurationName(
  const cmMakefile* mf) const
{
  std::string confName = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");
  // Trim the configuration name from whitespaces (left and right)
  confName.erase(0, confName.find_first_not_of(" \t\r\v\n"));
  confName.erase(confName.find_last_not_of(" \t\r\v\n") + 1);
  if (confName.empty()) {
    confName = "NoConfig";
  }
  return confName;
}

std::string cmExtraCodeLiteGenerator::GetBuildCommand(
  const cmMakefile* mf, const std::string& targetName) const
{
  std::string generator = mf->GetSafeDefinition("CMAKE_GENERATOR");
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string buildCommand = make; // Default
  std::ostringstream ss;
  if (generator == "NMake Makefiles" || generator == "Ninja") {
    ss << make;
  } else if (generator == "MinGW Makefiles" || generator == "Unix Makefiles") {
    ss << make << " -j " << this->CpuCount;
  }
  if (!targetName.empty()) {
    ss << " " << targetName;
  }
  buildCommand = ss.str();
  return buildCommand;
}

std::string cmExtraCodeLiteGenerator::GetCleanCommand(
  const cmMakefile* mf, const std::string& targetName) const
{
  std::string generator = mf->GetSafeDefinition("CMAKE_GENERATOR");
  std::ostringstream ss;
  std::string buildcommand = GetBuildCommand(mf, "");
  if (!targetName.empty() && generator == "Ninja") {
    ss << buildcommand << " -t clean " << targetName;
  } else {
    ss << buildcommand << " clean";
  }
  return ss.str();
}

std::string cmExtraCodeLiteGenerator::GetRebuildCommand(
  const cmMakefile* mf, const std::string& targetName) const
{
  return GetCleanCommand(mf, targetName) + " && " +
    GetBuildCommand(mf, targetName);
}

std::string cmExtraCodeLiteGenerator::GetSingleFileBuildCommand(
  const cmMakefile* mf) const
{
  std::string buildCommand;
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string generator = mf->GetSafeDefinition("CMAKE_GENERATOR");
  if (generator == "Unix Makefiles" || generator == "MinGW Makefiles") {
    std::ostringstream ss;
    ss << make << " -f$(ProjectPath)/Makefile $(CurrentFileName).cpp.o";
    buildCommand = ss.str();
  }
  return buildCommand;
}
