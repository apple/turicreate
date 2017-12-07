/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExtraCodeBlocksGenerator.h"

#include <map>
#include <ostream>
#include <string.h>
#include <utility>

#include "cmAlgorithms.h"
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

/* Some useful URLs:
Homepage:
http://www.codeblocks.org

File format docs:
http://wiki.codeblocks.org/index.php?title=File_formats_description
http://wiki.codeblocks.org/index.php?title=Workspace_file
http://wiki.codeblocks.org/index.php?title=Project_file

Discussion:
http://forums.codeblocks.org/index.php/topic,6789.0.html
*/

cmExtraCodeBlocksGenerator::cmExtraCodeBlocksGenerator()
  : cmExternalMakefileProjectGenerator()
{
}

cmExternalMakefileProjectGeneratorFactory*
cmExtraCodeBlocksGenerator::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmExtraCodeBlocksGenerator>
    factory("CodeBlocks", "Generates CodeBlocks project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
#if defined(_WIN32)
    factory.AddSupportedGlobalGenerator("MinGW Makefiles");
    factory.AddSupportedGlobalGenerator("NMake Makefiles");
    factory.AddSupportedGlobalGenerator("NMake Makefiles JOM");
// disable until somebody actually tests it:
// this->AddSupportedGlobalGenerator("MSYS Makefiles");
#endif
    factory.AddSupportedGlobalGenerator("Ninja");
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
  }

  return &factory;
}

void cmExtraCodeBlocksGenerator::Generate()
{
  // for each sub project in the project create a codeblocks project
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); ++it) {
    // create a project file
    this->CreateProjectFile(it->second);
  }
}

/* create the project file */
void cmExtraCodeBlocksGenerator::CreateProjectFile(
  const std::vector<cmLocalGenerator*>& lgs)
{
  std::string outputDir = lgs[0]->GetCurrentBinaryDirectory();
  std::string projectName = lgs[0]->GetProjectName();

  std::string filename = outputDir + "/";
  filename += projectName + ".cbp";
  std::string sessionFilename = outputDir + "/";
  sessionFilename += projectName + ".layout";

  this->CreateNewProjectFile(lgs, filename);
}

/* Tree is used to create a "Virtual Folder" in CodeBlocks, in which all
 CMake files this project depends on will be put. This means additionally
 to the "Sources" and "Headers" virtual folders of CodeBlocks, there will
 now also be a "CMake Files" virtual folder.
 Patch by Daniel Teske <daniel.teske AT nokia.com> (which use C::B project
 files in QtCreator).*/
struct Tree
{
  std::string path; // only one component of the path
  std::vector<Tree> folders;
  std::vector<std::string> files;
  void InsertPath(const std::vector<std::string>& splitted,
                  std::vector<std::string>::size_type start,
                  const std::string& fileName);
  void BuildVirtualFolder(cmXMLWriter& xml) const;
  void BuildVirtualFolderImpl(std::string& virtualFolders,
                              const std::string& prefix) const;
  void BuildUnit(cmXMLWriter& xml, const std::string& fsPath) const;
  void BuildUnitImpl(cmXMLWriter& xml, const std::string& virtualFolderPath,
                     const std::string& fsPath) const;
};

void Tree::InsertPath(const std::vector<std::string>& splitted,
                      std::vector<std::string>::size_type start,
                      const std::string& fileName)
{
  if (start == splitted.size()) {
    files.push_back(fileName);
    return;
  }
  for (std::vector<Tree>::iterator it = folders.begin(); it != folders.end();
       ++it) {
    if ((*it).path == splitted[start]) {
      if (start + 1 < splitted.size()) {
        it->InsertPath(splitted, start + 1, fileName);
        return;
      }
      // last part of splitted
      it->files.push_back(fileName);
      return;
    }
  }
  // Not found in folders, thus insert
  Tree newFolder;
  newFolder.path = splitted[start];
  if (start + 1 < splitted.size()) {
    newFolder.InsertPath(splitted, start + 1, fileName);
    folders.push_back(newFolder);
    return;
  }
  // last part of splitted
  newFolder.files.push_back(fileName);
  folders.push_back(newFolder);
}

void Tree::BuildVirtualFolder(cmXMLWriter& xml) const
{
  xml.StartElement("Option");
  std::string virtualFolders = "CMake Files\\;";
  for (std::vector<Tree>::const_iterator it = folders.begin();
       it != folders.end(); ++it) {
    it->BuildVirtualFolderImpl(virtualFolders, "");
  }
  xml.Attribute("virtualFolders", virtualFolders);
  xml.EndElement();
}

void Tree::BuildVirtualFolderImpl(std::string& virtualFolders,
                                  const std::string& prefix) const
{
  virtualFolders += "CMake Files\\" + prefix + path + "\\;";
  for (std::vector<Tree>::const_iterator it = folders.begin();
       it != folders.end(); ++it) {
    it->BuildVirtualFolderImpl(virtualFolders, prefix + path + "\\");
  }
}

void Tree::BuildUnit(cmXMLWriter& xml, const std::string& fsPath) const
{
  for (std::vector<std::string>::const_iterator it = files.begin();
       it != files.end(); ++it) {
    xml.StartElement("Unit");
    xml.Attribute("filename", fsPath + *it);

    xml.StartElement("Option");
    xml.Attribute("virtualFolder", "CMake Files\\");
    xml.EndElement();

    xml.EndElement();
  }
  for (std::vector<Tree>::const_iterator it = folders.begin();
       it != folders.end(); ++it) {
    it->BuildUnitImpl(xml, "", fsPath);
  }
}

void Tree::BuildUnitImpl(cmXMLWriter& xml,
                         const std::string& virtualFolderPath,
                         const std::string& fsPath) const
{
  for (std::vector<std::string>::const_iterator it = files.begin();
       it != files.end(); ++it) {
    xml.StartElement("Unit");
    xml.Attribute("filename", fsPath + path + "/" + *it);

    xml.StartElement("Option");
    xml.Attribute("virtualFolder",
                  "CMake Files\\" + virtualFolderPath + path + "\\");
    xml.EndElement();

    xml.EndElement();
  }
  for (std::vector<Tree>::const_iterator it = folders.begin();
       it != folders.end(); ++it) {
    it->BuildUnitImpl(xml, virtualFolderPath + path + "\\",
                      fsPath + path + "/");
  }
}

void cmExtraCodeBlocksGenerator::CreateNewProjectFile(
  const std::vector<cmLocalGenerator*>& lgs, const std::string& filename)
{
  const cmMakefile* mf = lgs[0]->GetMakefile();
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  Tree tree;

  // build tree of virtual folders
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); ++it) {
    // Collect all files
    std::vector<std::string> listFiles;
    for (std::vector<cmLocalGenerator*>::const_iterator jt =
           it->second.begin();
         jt != it->second.end(); ++jt) {
      const std::vector<std::string>& files =
        (*jt)->GetMakefile()->GetListFiles();
      listFiles.insert(listFiles.end(), files.begin(), files.end());
    }

    // Convert
    for (std::vector<std::string>::const_iterator jt = listFiles.begin();
         jt != listFiles.end(); ++jt) {
      // don't put cmake's own files into the project (#12110):
      if (jt->find(cmSystemTools::GetCMakeRoot()) == 0) {
        continue;
      }

      const std::string& relative = cmSystemTools::RelativePath(
        it->second[0]->GetSourceDirectory(), jt->c_str());
      std::vector<std::string> splitted;
      cmSystemTools::SplitPath(relative, splitted, false);
      // Split filename from path
      std::string fileName = *(splitted.end() - 1);
      splitted.erase(splitted.end() - 1, splitted.end());

      // We don't want paths with CMakeFiles in them
      // or do we?
      // In speedcrunch those where purely internal
      if (!splitted.empty() &&
          relative.find("CMakeFiles") == std::string::npos) {
        tree.InsertPath(splitted, 1, fileName);
      }
    }
  }

  // figure out the compiler
  std::string compiler = this->GetCBCompilerId(mf);
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  const std::string makeArgs =
    mf->GetSafeDefinition("CMAKE_CODEBLOCKS_MAKE_ARGUMENTS");

  cmXMLWriter xml(fout);
  xml.StartDocument();
  xml.StartElement("CodeBlocks_project_file");

  xml.StartElement("FileVersion");
  xml.Attribute("major", 1);
  xml.Attribute("minor", 6);
  xml.EndElement();

  xml.StartElement("Project");

  xml.StartElement("Option");
  xml.Attribute("title", lgs[0]->GetProjectName());
  xml.EndElement();

  xml.StartElement("Option");
  xml.Attribute("makefile_is_custom", 1);
  xml.EndElement();

  xml.StartElement("Option");
  xml.Attribute("compiler", compiler);
  xml.EndElement();

  // Now build a virtual tree
  tree.BuildVirtualFolder(xml);

  xml.StartElement("Build");

  this->AppendTarget(xml, "all", CM_NULLPTR, make.c_str(), lgs[0],
                     compiler.c_str(), makeArgs);

  // add all executable and library targets and some of the GLOBAL
  // and UTILITY targets
  for (std::vector<cmLocalGenerator*>::const_iterator lg = lgs.begin();
       lg != lgs.end(); lg++) {
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
            this->AppendTarget(xml, targetName, CM_NULLPTR, make.c_str(), *lg,
                               compiler.c_str(), makeArgs);
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

          this->AppendTarget(xml, targetName, CM_NULLPTR, make.c_str(), *lg,
                             compiler.c_str(), makeArgs);
          break;
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY: {
          cmGeneratorTarget* gt = *ti;
          this->AppendTarget(xml, targetName, gt, make.c_str(), *lg,
                             compiler.c_str(), makeArgs);
          std::string fastTarget = targetName;
          fastTarget += "/fast";
          this->AppendTarget(xml, fastTarget, gt, make.c_str(), *lg,
                             compiler.c_str(), makeArgs);
        } break;
        default:
          break;
      }
    }
  }

  xml.EndElement(); // Build

  // Collect all used source files in the project.
  // Keep a list of C/C++ source files which might have an acompanying header
  // that should be looked for.
  typedef std::map<std::string, CbpUnit> all_files_map_t;
  all_files_map_t allFiles;
  std::vector<std::string> cFiles;

  std::vector<std::string> const& srcExts =
    this->GlobalGenerator->GetCMakeInstance()->GetSourceExtensions();

  for (std::vector<cmLocalGenerator*>::const_iterator lg = lgs.begin();
       lg != lgs.end(); lg++) {
    cmMakefile* makefile = (*lg)->GetMakefile();
    std::vector<cmGeneratorTarget*> targets = (*lg)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin();
         ti != targets.end(); ti++) {
      switch ((*ti)->GetType()) {
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY:
        case cmStateEnums::UTILITY: // can have sources since 2.6.3
        {
          std::vector<cmSourceFile*> sources;
          cmGeneratorTarget* gt = *ti;
          gt->GetSourceFiles(sources,
                             makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
          for (std::vector<cmSourceFile*>::const_iterator si = sources.begin();
               si != sources.end(); si++) {
            // don't add source files from UTILITY target which have the
            // GENERATED property set:
            if (gt->GetType() == cmStateEnums::UTILITY &&
                (*si)->GetPropertyAsBool("GENERATED")) {
              continue;
            }

            // check whether it is a C/C++ implementation file
            bool isCFile = false;
            std::string lang = (*si)->GetLanguage();
            if (lang == "C" || lang == "CXX") {
              std::string const& srcext = (*si)->GetExtension();
              for (std::vector<std::string>::const_iterator ext =
                     srcExts.begin();
                   ext != srcExts.end(); ++ext) {
                if (srcext == *ext) {
                  isCFile = true;
                  break;
                }
              }
            }

            std::string const& fullPath = (*si)->GetFullPath();

            if (isCFile) {
              cFiles.push_back(fullPath);
            }

            CbpUnit& cbpUnit = allFiles[fullPath];
            cbpUnit.Targets.push_back(*ti);
          }
        }
        default: // intended fallthrough
          break;
      }
    }
  }

  std::vector<std::string> const& headerExts =
    this->GlobalGenerator->GetCMakeInstance()->GetHeaderExtensions();

  // The following loop tries to add header files matching to implementation
  // files to the project. It does that by iterating over all
  // C/C++ source files,
  // replacing the file name extension with ".h" and checks whether such a
  // file exists. If it does, it is inserted into the map of files.
  // A very similar version of that code exists also in the kdevelop
  // project generator.
  for (std::vector<std::string>::const_iterator sit = cFiles.begin();
       sit != cFiles.end(); ++sit) {
    std::string const& fileName = *sit;
    std::string headerBasename = cmSystemTools::GetFilenamePath(fileName);
    headerBasename += "/";
    headerBasename += cmSystemTools::GetFilenameWithoutExtension(fileName);

    // check if there's a matching header around
    for (std::vector<std::string>::const_iterator ext = headerExts.begin();
         ext != headerExts.end(); ++ext) {
      std::string hname = headerBasename;
      hname += ".";
      hname += *ext;
      // if it's already in the set, don't check if it exists on disk
      if (allFiles.find(hname) != allFiles.end()) {
        break;
      }

      if (cmSystemTools::FileExists(hname.c_str())) {
        allFiles[hname].Targets = allFiles[fileName].Targets;
        break;
      }
    }
  }

  // insert all source files in the CodeBlocks project
  for (all_files_map_t::const_iterator sit = allFiles.begin();
       sit != allFiles.end(); ++sit) {
    std::string const& unitFilename = sit->first;
    CbpUnit const& unit = sit->second;

    xml.StartElement("Unit");
    xml.Attribute("filename", unitFilename);

    for (std::vector<const cmGeneratorTarget*>::const_iterator ti =
           unit.Targets.begin();
         ti != unit.Targets.end(); ++ti) {
      xml.StartElement("Option");
      xml.Attribute("target", (*ti)->GetName());
      xml.EndElement();
    }

    xml.EndElement();
  }

  // Add CMakeLists.txt
  tree.BuildUnit(xml, std::string(mf->GetHomeDirectory()) + "/");

  xml.EndElement(); // Project
  xml.EndElement(); // CodeBlocks_project_file
  xml.EndDocument();
}

// Write a dummy file for OBJECT libraries, so C::B can reference some file
std::string cmExtraCodeBlocksGenerator::CreateDummyTargetFile(
  cmLocalGenerator* lg, cmGeneratorTarget* target) const
{
  // this file doesn't seem to be used by C::B in custom makefile mode,
  // but we generate a unique file for each OBJECT library so in case
  // C::B uses it in some way, the targets don't interfere with each other.
  std::string filename = lg->GetCurrentBinaryDirectory();
  filename += "/";
  filename += lg->GetTargetDirectory(target);
  filename += "/";
  filename += target->GetName();
  filename += ".objlib";
  cmGeneratedFileStream fout(filename.c_str());
  if (fout) {
    /* clang-format off */
    fout << "# This is a dummy file for the OBJECT library "
         << target->GetName()
         << " for the CMake CodeBlocks project generator.\n"
         << "# Don't edit, this file will be overwritten.\n";
    /* clang-format on */
  }
  return filename;
}

// Generate the xml code for one target.
void cmExtraCodeBlocksGenerator::AppendTarget(
  cmXMLWriter& xml, const std::string& targetName, cmGeneratorTarget* target,
  const char* make, const cmLocalGenerator* lg, const char* compiler,
  const std::string& makeFlags)
{
  cmMakefile const* makefile = lg->GetMakefile();
  std::string makefileName = lg->GetCurrentBinaryDirectory();
  makefileName += "/Makefile";

  xml.StartElement("Target");
  xml.Attribute("title", targetName);

  if (target != CM_NULLPTR) {
    int cbTargetType = this->GetCBTargetType(target);
    std::string workingDir = lg->GetCurrentBinaryDirectory();
    if (target->GetType() == cmStateEnums::EXECUTABLE) {
      // Determine the directory where the executable target is created, and
      // set the working directory to this dir.
      const char* runtimeOutputDir =
        makefile->GetDefinition("CMAKE_RUNTIME_OUTPUT_DIRECTORY");
      if (runtimeOutputDir != CM_NULLPTR) {
        workingDir = runtimeOutputDir;
      } else {
        const char* executableOutputDir =
          makefile->GetDefinition("EXECUTABLE_OUTPUT_PATH");
        if (executableOutputDir != CM_NULLPTR) {
          workingDir = executableOutputDir;
        }
      }
    }

    std::string buildType = makefile->GetSafeDefinition("CMAKE_BUILD_TYPE");
    std::string location;
    if (target->GetType() == cmStateEnums::OBJECT_LIBRARY) {
      location =
        this->CreateDummyTargetFile(const_cast<cmLocalGenerator*>(lg), target);
    } else {
      location = target->GetLocation(buildType);
    }

    xml.StartElement("Option");
    xml.Attribute("output", location);
    xml.Attribute("prefix_auto", 0);
    xml.Attribute("extension_auto", 0);
    xml.EndElement();

    xml.StartElement("Option");
    xml.Attribute("working_dir", workingDir);
    xml.EndElement();

    xml.StartElement("Option");
    xml.Attribute("object_output", "./");
    xml.EndElement();

    xml.StartElement("Option");
    xml.Attribute("type", cbTargetType);
    xml.EndElement();

    xml.StartElement("Option");
    xml.Attribute("compiler", compiler);
    xml.EndElement();

    xml.StartElement("Compiler");

    // the compilerdefines for this target
    std::vector<std::string> cdefs;
    target->GetCompileDefinitions(cdefs, buildType, "C");

    // Expand the list.
    for (std::vector<std::string>::const_iterator di = cdefs.begin();
         di != cdefs.end(); ++di) {
      xml.StartElement("Add");
      xml.Attribute("option", "-D" + *di);
      xml.EndElement();
    }

    // the include directories for this target
    std::vector<std::string> allIncludeDirs;

    std::vector<std::string> includes;
    lg->GetIncludeDirectories(includes, target, "C", buildType);

    allIncludeDirs.insert(allIncludeDirs.end(), includes.begin(),
                          includes.end());

    std::string systemIncludeDirs = makefile->GetSafeDefinition(
      "CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS");
    if (!systemIncludeDirs.empty()) {
      std::vector<std::string> dirs;
      cmSystemTools::ExpandListArgument(systemIncludeDirs, dirs);
      allIncludeDirs.insert(allIncludeDirs.end(), dirs.begin(), dirs.end());
    }

    systemIncludeDirs = makefile->GetSafeDefinition(
      "CMAKE_EXTRA_GENERATOR_C_SYSTEM_INCLUDE_DIRS");
    if (!systemIncludeDirs.empty()) {
      std::vector<std::string> dirs;
      cmSystemTools::ExpandListArgument(systemIncludeDirs, dirs);
      allIncludeDirs.insert(allIncludeDirs.end(), dirs.begin(), dirs.end());
    }

    std::vector<std::string>::const_iterator end =
      cmRemoveDuplicates(allIncludeDirs);

    for (std::vector<std::string>::const_iterator i = allIncludeDirs.begin();
         i != end; ++i) {
      xml.StartElement("Add");
      xml.Attribute("directory", *i);
      xml.EndElement();
    }

    xml.EndElement(); // Compiler
  } else              // e.g. all and the GLOBAL and UTILITY targets
  {
    xml.StartElement("Option");
    xml.Attribute("working_dir", lg->GetCurrentBinaryDirectory());
    xml.EndElement();

    xml.StartElement("Option");
    xml.Attribute("type", 4);
    xml.EndElement();
  }

  xml.StartElement("MakeCommands");

  xml.StartElement("Build");
  xml.Attribute("command", this->BuildMakeCommand(make, makefileName.c_str(),
                                                  targetName, makeFlags));
  xml.EndElement();

  xml.StartElement("CompileFile");
  xml.Attribute("command", this->BuildMakeCommand(make, makefileName.c_str(),
                                                  "\"$file\"", makeFlags));
  xml.EndElement();

  xml.StartElement("Clean");
  xml.Attribute("command", this->BuildMakeCommand(make, makefileName.c_str(),
                                                  "clean", makeFlags));
  xml.EndElement();

  xml.StartElement("DistClean");
  xml.Attribute("command", this->BuildMakeCommand(make, makefileName.c_str(),
                                                  "clean", makeFlags));
  xml.EndElement();

  xml.EndElement(); // MakeCommands
  xml.EndElement(); // Target
}

// Translate the cmake compiler id into the CodeBlocks compiler id
std::string cmExtraCodeBlocksGenerator::GetCBCompilerId(const cmMakefile* mf)
{
  // figure out which language to use
  // for now care only for C, C++, and Fortran

  // projects with C/C++ and Fortran are handled as C/C++ projects
  bool pureFortran = false;
  std::string compilerIdVar;
  if (this->GlobalGenerator->GetLanguageEnabled("CXX")) {
    compilerIdVar = "CMAKE_CXX_COMPILER_ID";
  } else if (this->GlobalGenerator->GetLanguageEnabled("C")) {
    compilerIdVar = "CMAKE_C_COMPILER_ID";
  } else if (this->GlobalGenerator->GetLanguageEnabled("Fortran")) {
    compilerIdVar = "CMAKE_Fortran_COMPILER_ID";
    pureFortran = true;
  }

  std::string compilerId = mf->GetSafeDefinition(compilerIdVar);
  std::string compiler = "gcc"; // default to gcc
  if (compilerId == "MSVC") {
    if (mf->IsDefinitionSet("MSVC10")) {
      compiler = "msvc10";
    } else {
      compiler = "msvc8";
    }
  } else if (compilerId == "Borland") {
    compiler = "bcc";
  } else if (compilerId == "SDCC") {
    compiler = "sdcc";
  } else if (compilerId == "Intel") {
    if (pureFortran && mf->IsDefinitionSet("WIN32")) {
      compiler = "ifcwin"; // Intel Fortran for Windows (known by cbFortran)
    } else {
      compiler = "icc";
    }
  } else if (compilerId == "Watcom" || compilerId == "OpenWatcom") {
    compiler = "ow";
  } else if (compilerId == "Clang") {
    compiler = "clang";
  } else if (compilerId == "PGI") {
    if (pureFortran) {
      compiler = "pgifortran";
    } else {
      compiler = "pgi"; // does not exist as default in CodeBlocks 16.01
    }
  } else if (compilerId == "GNU") {
    if (pureFortran) {
      compiler = "gfortran";
    } else {
      compiler = "gcc";
    }
  }
  return compiler;
}

// Translate the cmake target type into the CodeBlocks target type id
int cmExtraCodeBlocksGenerator::GetCBTargetType(cmGeneratorTarget* target)
{
  switch (target->GetType()) {
    case cmStateEnums::EXECUTABLE:
      if ((target->GetPropertyAsBool("WIN32_EXECUTABLE")) ||
          (target->GetPropertyAsBool("MACOSX_BUNDLE"))) {
        return 0;
      }
      return 1;
    case cmStateEnums::STATIC_LIBRARY:
    case cmStateEnums::OBJECT_LIBRARY:
      return 2;
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
      return 3;
    default:
      return 4;
  }
}

// Create the command line for building the given target using the selected
// make
std::string cmExtraCodeBlocksGenerator::BuildMakeCommand(
  const std::string& make, const char* makefile, const std::string& target,
  const std::string& makeFlags)
{
  std::string command = make;
  if (!makeFlags.empty()) {
    command += " ";
    command += makeFlags;
  }

  std::string generator = this->GlobalGenerator->GetName();
  if (generator == "NMake Makefiles" || generator == "NMake Makefiles JOM") {
    // For Windows ConvertToOutputPath already adds quotes when required.
    // These need to be escaped, see
    // https://gitlab.kitware.com/cmake/cmake/issues/13952
    std::string makefileName = cmSystemTools::ConvertToOutputPath(makefile);
    command += " /NOLOGO /f ";
    command += makefileName;
    command += " VERBOSE=1 ";
    command += target;
  } else if (generator == "MinGW Makefiles") {
    // no escaping of spaces in this case, see
    // https://gitlab.kitware.com/cmake/cmake/issues/10014
    std::string makefileName = makefile;
    command += " -f \"";
    command += makefileName;
    command += "\" ";
    command += " VERBOSE=1 ";
    command += target;
  } else if (generator == "Ninja") {
    command += " -v ";
    command += target;
  } else {
    std::string makefileName = cmSystemTools::ConvertToOutputPath(makefile);
    command += " -f \"";
    command += makefileName;
    command += "\" ";
    command += " VERBOSE=1 ";
    command += target;
  }
  return command;
}
