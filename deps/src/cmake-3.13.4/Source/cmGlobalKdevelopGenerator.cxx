/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalKdevelopGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmXMLWriter.h"
#include "cmake.h"

#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include <map>
#include <set>
#include <string.h>
#include <utility>

cmGlobalKdevelopGenerator::cmGlobalKdevelopGenerator()
  : cmExternalMakefileProjectGenerator()
{
}

cmExternalMakefileProjectGeneratorFactory*
cmGlobalKdevelopGenerator::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmGlobalKdevelopGenerator>
    factory("KDevelop3", "Generates KDevelop 3 project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
#ifdef CMAKE_USE_NINJA
    factory.AddSupportedGlobalGenerator("Ninja");
#endif

    factory.Aliases.push_back("KDevelop3");
  }

  return &factory;
}

void cmGlobalKdevelopGenerator::Generate()
{
  // for each sub project in the project create
  // a kdevelop project
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); ++it) {
    std::string outputDir = it->second[0]->GetCurrentBinaryDirectory();
    std::string projectDir = it->second[0]->GetSourceDirectory();
    std::string projectName = it->second[0]->GetProjectName();
    std::string cmakeFilePattern("CMakeLists.txt;*.cmake;");
    std::string fileToOpen;
    const std::vector<cmLocalGenerator*>& lgs = it->second;
    // create the project.kdevelop.filelist file
    if (!this->CreateFilelistFile(lgs, outputDir, projectDir, projectName,
                                  cmakeFilePattern, fileToOpen)) {
      cmSystemTools::Error("Can not create filelist file");
      return;
    }
    // try to find the name of an executable so we have something to
    // run from kdevelop for now just pick the first executable found
    std::string executable;
    for (std::vector<cmLocalGenerator*>::const_iterator lg = lgs.begin();
         lg != lgs.end(); lg++) {
      std::vector<cmGeneratorTarget*> const& targets =
        (*lg)->GetGeneratorTargets();
      for (std::vector<cmGeneratorTarget*>::const_iterator ti =
             targets.begin();
           ti != targets.end(); ti++) {
        if ((*ti)->GetType() == cmStateEnums::EXECUTABLE) {
          executable = (*ti)->GetLocation("");
          break;
        }
      }
      if (!executable.empty()) {
        break;
      }
    }

    // now create a project file
    this->CreateProjectFile(outputDir, projectDir, projectName, executable,
                            cmakeFilePattern, fileToOpen);
  }
}

bool cmGlobalKdevelopGenerator::CreateFilelistFile(
  const std::vector<cmLocalGenerator*>& lgs, const std::string& outputDir,
  const std::string& projectDirIn, const std::string& projectname,
  std::string& cmakeFilePattern, std::string& fileToOpen)
{
  std::string projectDir = projectDirIn + "/";
  std::string filename = outputDir + "/" + projectname + ".kdevelop.filelist";

  std::set<std::string> files;
  std::string tmp;

  std::vector<std::string> const& hdrExts =
    this->GlobalGenerator->GetCMakeInstance()->GetHeaderExtensions();

  for (std::vector<cmLocalGenerator*>::const_iterator it = lgs.begin();
       it != lgs.end(); it++) {
    cmMakefile* makefile = (*it)->GetMakefile();
    const std::vector<std::string>& listFiles = makefile->GetListFiles();
    for (std::vector<std::string>::const_iterator lt = listFiles.begin();
         lt != listFiles.end(); lt++) {
      tmp = *lt;
      cmSystemTools::ReplaceString(tmp, projectDir.c_str(), "");
      // make sure the file is part of this source tree
      if ((tmp[0] != '/') &&
          (strstr(tmp.c_str(), cmake::GetCMakeFilesDirectoryPostSlash()) ==
           CM_NULLPTR)) {
        files.insert(tmp);
        tmp = cmSystemTools::GetFilenameName(tmp);
        // add all files which dont match the default
        // */CMakeLists.txt;*cmake; to the file pattern
        if ((tmp != "CMakeLists.txt") &&
            (strstr(tmp.c_str(), ".cmake") == CM_NULLPTR)) {
          cmakeFilePattern += tmp + ";";
        }
      }
    }

    // get all sources
    std::vector<cmGeneratorTarget*> targets = (*it)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator ti = targets.begin();
         ti != targets.end(); ti++) {
      std::vector<cmSourceFile*> sources;
      cmGeneratorTarget* gt = *ti;
      gt->GetSourceFiles(sources, gt->Target->GetMakefile()->GetSafeDefinition(
                                    "CMAKE_BUILD_TYPE"));
      for (std::vector<cmSourceFile*>::const_iterator si = sources.begin();
           si != sources.end(); si++) {
        tmp = (*si)->GetFullPath();
        std::string headerBasename = cmSystemTools::GetFilenamePath(tmp);
        headerBasename += "/";
        headerBasename += cmSystemTools::GetFilenameWithoutExtension(tmp);

        cmSystemTools::ReplaceString(tmp, projectDir.c_str(), "");

        if ((tmp[0] != '/') &&
            (strstr(tmp.c_str(), cmake::GetCMakeFilesDirectoryPostSlash()) ==
             CM_NULLPTR) &&
            (cmSystemTools::GetFilenameExtension(tmp) != ".moc")) {
          files.insert(tmp);

          // check if there's a matching header around
          for (std::vector<std::string>::const_iterator ext = hdrExts.begin();
               ext != hdrExts.end(); ++ext) {
            std::string hname = headerBasename;
            hname += ".";
            hname += *ext;
            if (cmSystemTools::FileExists(hname.c_str())) {
              cmSystemTools::ReplaceString(hname, projectDir.c_str(), "");
              files.insert(hname);
              break;
            }
          }
        }
      }
      for (std::vector<std::string>::const_iterator lt = listFiles.begin();
           lt != listFiles.end(); lt++) {
        tmp = *lt;
        cmSystemTools::ReplaceString(tmp, projectDir.c_str(), "");
        if ((tmp[0] != '/') &&
            (strstr(tmp.c_str(), cmake::GetCMakeFilesDirectoryPostSlash()) ==
             CM_NULLPTR)) {
          files.insert(tmp);
        }
      }
    }
  }

  // check if the output file already exists and read it
  // insert all files which exist into the set of files
  cmsys::ifstream oldFilelist(filename.c_str());
  if (oldFilelist) {
    while (cmSystemTools::GetLineFromStream(oldFilelist, tmp)) {
      if (tmp[0] == '/') {
        continue;
      }
      std::string completePath = projectDir + tmp;
      if (cmSystemTools::FileExists(completePath.c_str())) {
        files.insert(tmp);
      }
    }
    oldFilelist.close();
  }

  // now write the new filename
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return false;
  }

  fileToOpen = "";
  for (std::set<std::string>::const_iterator it = files.begin();
       it != files.end(); it++) {
    // get the full path to the file
    tmp = cmSystemTools::CollapseFullPath(*it, projectDir.c_str());
    // just select the first source file
    if (fileToOpen.empty()) {
      std::string ext = cmSystemTools::GetFilenameExtension(tmp);
      if ((ext == ".c") || (ext == ".cc") || (ext == ".cpp") ||
          (ext == ".cxx") || (ext == ".C") || (ext == ".h") ||
          (ext == ".hpp")) {
        fileToOpen = tmp;
      }
    }
    // make it relative to the project dir
    cmSystemTools::ReplaceString(tmp, projectDir.c_str(), "");
    // only put relative paths
    if (!tmp.empty() && tmp[0] != '/') {
      fout << tmp << "\n";
    }
  }
  return true;
}

/* create the project file, if it already exists, merge it with the
existing one, otherwise create a new one */
void cmGlobalKdevelopGenerator::CreateProjectFile(
  const std::string& outputDir, const std::string& projectDir,
  const std::string& projectname, const std::string& executable,
  const std::string& cmakeFilePattern, const std::string& fileToOpen)
{
  this->Blacklist.clear();

  std::string filename = outputDir + "/";
  filename += projectname + ".kdevelop";
  std::string sessionFilename = outputDir + "/";
  sessionFilename += projectname + ".kdevses";

  if (cmSystemTools::FileExists(filename.c_str())) {
    this->MergeProjectFiles(outputDir, projectDir, filename, executable,
                            cmakeFilePattern, fileToOpen, sessionFilename);
  } else {
    // add all subdirectories which are cmake build directories to the
    // kdevelop blacklist so they are not monitored for added or removed files
    // since this is handled by adding files to the cmake files
    cmsys::Directory d;
    if (d.Load(projectDir)) {
      size_t numf = d.GetNumberOfFiles();
      for (unsigned int i = 0; i < numf; i++) {
        std::string nextFile = d.GetFile(i);
        if ((nextFile != ".") && (nextFile != "..")) {
          std::string tmp = projectDir;
          tmp += "/";
          tmp += nextFile;
          if (cmSystemTools::FileIsDirectory(tmp)) {
            tmp += "/CMakeCache.txt";
            if ((nextFile == "CMakeFiles") ||
                (cmSystemTools::FileExists(tmp.c_str()))) {
              this->Blacklist.push_back(nextFile);
            }
          }
        }
      }
    }
    this->CreateNewProjectFile(outputDir, projectDir, filename, executable,
                               cmakeFilePattern, fileToOpen, sessionFilename);
  }
}

void cmGlobalKdevelopGenerator::MergeProjectFiles(
  const std::string& outputDir, const std::string& projectDir,
  const std::string& filename, const std::string& executable,
  const std::string& cmakeFilePattern, const std::string& fileToOpen,
  const std::string& sessionFilename)
{
  cmsys::ifstream oldProjectFile(filename.c_str());
  if (!oldProjectFile) {
    this->CreateNewProjectFile(outputDir, projectDir, filename, executable,
                               cmakeFilePattern, fileToOpen, sessionFilename);
    return;
  }

  /* Read the existing project file (line by line), copy all lines
    into the new project file, except the ones which can be reliably
    set from contents of the CMakeLists.txt */
  std::string tmp;
  std::vector<std::string> lines;
  while (cmSystemTools::GetLineFromStream(oldProjectFile, tmp)) {
    lines.push_back(tmp);
  }
  oldProjectFile.close();

  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  for (std::vector<std::string>::const_iterator it = lines.begin();
       it != lines.end(); it++) {
    const char* line = (*it).c_str();
    // skip these tags as they are always replaced
    if ((strstr(line, "<projectdirectory>") != CM_NULLPTR) ||
        (strstr(line, "<projectmanagement>") != CM_NULLPTR) ||
        (strstr(line, "<absoluteprojectpath>") != CM_NULLPTR) ||
        (strstr(line, "<filelistdirectory>") != CM_NULLPTR) ||
        (strstr(line, "<buildtool>") != CM_NULLPTR) ||
        (strstr(line, "<builddir>") != CM_NULLPTR)) {
      continue;
    }

    // output the line from the file if it is not one of the above tags
    fout << *it << "\n";
    // if this is the <general> tag output the stuff that goes in the
    // general tag
    if (strstr(line, "<general>")) {
      fout << "  <projectmanagement>KDevCustomProject</projectmanagement>\n";
      fout << "  <projectdirectory>" << projectDir
           << "</projectdirectory>\n"; // this one is important
      fout << "  <absoluteprojectpath>true</absoluteprojectpath>\n";
      // and this one
    }
    // inside kdevcustomproject the <filelistdirectory> must be put
    if (strstr(line, "<kdevcustomproject>")) {
      fout << "    <filelistdirectory>" << outputDir
           << "</filelistdirectory>\n";
    }
    // buildtool and builddir go inside <build>
    if (strstr(line, "<build>")) {
      fout << "      <buildtool>make</buildtool>\n";
      fout << "      <builddir>" << outputDir << "</builddir>\n";
    }
  }
}

void cmGlobalKdevelopGenerator::CreateNewProjectFile(
  const std::string& outputDir, const std::string& projectDir,
  const std::string& filename, const std::string& executable,
  const std::string& cmakeFilePattern, const std::string& fileToOpen,
  const std::string& sessionFilename)
{
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }
  cmXMLWriter xml(fout);

  // check for a version control system
  bool hasSvn = cmSystemTools::FileExists((projectDir + "/.svn").c_str());
  bool hasCvs = cmSystemTools::FileExists((projectDir + "/CVS").c_str());

  bool enableCxx = (this->GlobalGenerator->GetLanguageEnabled("C") ||
                    this->GlobalGenerator->GetLanguageEnabled("CXX"));
  bool enableFortran = this->GlobalGenerator->GetLanguageEnabled("Fortran");
  std::string primaryLanguage = "C++";
  if (enableFortran && !enableCxx) {
    primaryLanguage = "Fortran77";
  }

  xml.StartDocument();
  xml.StartElement("kdevelop");
  xml.StartElement("general");

  xml.Element("author", "");
  xml.Element("email", "");
  xml.Element("version", "$VERSION$");
  xml.Element("projectmanagement", "KDevCustomProject");
  xml.Element("primarylanguage", primaryLanguage);
  xml.Element("ignoreparts");
  xml.Element("projectdirectory", projectDir); // this one is important
  xml.Element("absoluteprojectpath", "true");  // and this one

  // setup additional languages
  xml.StartElement("secondaryLanguages");
  if (enableFortran && enableCxx) {
    xml.Element("language", "Fortran");
  }
  if (enableCxx) {
    xml.Element("language", "C");
  }
  xml.EndElement();

  if (hasSvn) {
    xml.Element("versioncontrol", "kdevsubversion");
  } else if (hasCvs) {
    xml.Element("versioncontrol", "kdevcvsservice");
  }

  xml.EndElement(); // general
  xml.StartElement("kdevcustomproject");

  xml.Element("filelistdirectory", outputDir);

  xml.StartElement("run");
  xml.Element("mainprogram", executable);
  xml.Element("directoryradio", "custom");
  xml.Element("customdirectory", outputDir);
  xml.Element("programargs", "");
  xml.Element("terminal", "false");
  xml.Element("autocompile", "true");
  xml.Element("envvars");
  xml.EndElement();

  xml.StartElement("build");
  xml.Element("buildtool", "make");   // this one is important
  xml.Element("builddir", outputDir); // and this one
  xml.EndElement();

  xml.StartElement("make");
  xml.Element("abortonerror", "false");
  xml.Element("numberofjobs", 1);
  xml.Element("dontact", "false");
  xml.Element("makebin", this->GlobalGenerator->GetLocalGenerators()[0]
                           ->GetMakefile()
                           ->GetRequiredDefinition("CMAKE_MAKE_PROGRAM"));
  xml.Element("selectedenvironment", "default");

  xml.StartElement("environments");
  xml.StartElement("default");

  xml.StartElement("envvar");
  xml.Attribute("value", 1);
  xml.Attribute("name", "VERBOSE");
  xml.EndElement();

  xml.StartElement("envvar");
  xml.Attribute("value", 1);
  xml.Attribute("name", "CMAKE_NO_VERBOSE");
  xml.EndElement();

  xml.EndElement(); // default
  xml.EndElement(); // environments
  xml.EndElement(); // make

  xml.StartElement("blacklist");
  for (std::vector<std::string>::const_iterator dirIt =
         this->Blacklist.begin();
       dirIt != this->Blacklist.end(); ++dirIt) {
    xml.Element("path", *dirIt);
  }
  xml.EndElement();

  xml.EndElement(); // kdevcustomproject

  xml.StartElement("kdevfilecreate");
  xml.Element("filetypes");
  xml.StartElement("useglobaltypes");

  xml.StartElement("type");
  xml.Attribute("ext", "ui");
  xml.EndElement();

  xml.StartElement("type");
  xml.Attribute("ext", "cpp");
  xml.EndElement();

  xml.StartElement("type");
  xml.Attribute("ext", "h");
  xml.EndElement();

  xml.EndElement(); // useglobaltypes
  xml.EndElement(); // kdevfilecreate

  xml.StartElement("kdevdoctreeview");
  xml.StartElement("projectdoc");
  xml.Element("userdocDir", "html/");
  xml.Element("apidocDir", "html/");
  xml.EndElement(); // projectdoc
  xml.Element("ignoreqt_xml");
  xml.Element("ignoredoxygen");
  xml.Element("ignorekdocs");
  xml.Element("ignoretocs");
  xml.Element("ignoredevhelp");
  xml.EndElement(); // kdevdoctreeview;

  if (enableCxx) {
    xml.StartElement("cppsupportpart");
    xml.StartElement("filetemplates");
    xml.Element("interfacesuffix", ".h");
    xml.Element("implementationsuffix", ".cpp");
    xml.EndElement(); // filetemplates
    xml.EndElement(); // cppsupportpart

    xml.StartElement("kdevcppsupport");
    xml.StartElement("codecompletion");
    xml.Element("includeGlobalFunctions", "true");
    xml.Element("includeTypes", "true");
    xml.Element("includeEnums", "true");
    xml.Element("includeTypedefs", "false");
    xml.Element("automaticCodeCompletion", "true");
    xml.Element("automaticArgumentsHint", "true");
    xml.Element("automaticHeaderCompletion", "true");
    xml.Element("codeCompletionDelay", 250);
    xml.Element("argumentsHintDelay", 400);
    xml.Element("headerCompletionDelay", 250);
    xml.EndElement(); // codecompletion
    xml.Element("references");
    xml.EndElement(); // kdevcppsupport;
  }

  if (enableFortran) {
    xml.StartElement("kdevfortransupport");
    xml.StartElement("ftnchek");
    xml.Element("division", "false");
    xml.Element("extern", "false");
    xml.Element("declare", "false");
    xml.Element("pure", "false");
    xml.Element("argumentsall", "false");
    xml.Element("commonall", "false");
    xml.Element("truncationall", "false");
    xml.Element("usageall", "false");
    xml.Element("f77all", "false");
    xml.Element("portabilityall", "false");
    xml.Element("argumentsonly");
    xml.Element("commononly");
    xml.Element("truncationonly");
    xml.Element("usageonly");
    xml.Element("f77only");
    xml.Element("portabilityonly");
    xml.EndElement(); // ftnchek
    xml.EndElement(); // kdevfortransupport;
  }

  // set up file groups. maybe this can be used with the CMake SOURCE_GROUP()
  // command
  xml.StartElement("kdevfileview");
  xml.StartElement("groups");

  xml.StartElement("group");
  xml.Attribute("pattern", cmakeFilePattern);
  xml.Attribute("name", "CMake");
  xml.EndElement();

  if (enableCxx) {
    xml.StartElement("group");
    xml.Attribute("pattern", "*.h;*.hxx;*.hpp");
    xml.Attribute("name", "Header");
    xml.EndElement();

    xml.StartElement("group");
    xml.Attribute("pattern", "*.c");
    xml.Attribute("name", "C Sources");
    xml.EndElement();

    xml.StartElement("group");
    xml.Attribute("pattern", "*.cpp;*.C;*.cxx;*.cc");
    xml.Attribute("name", "C++ Sources");
    xml.EndElement();
  }

  if (enableFortran) {
    xml.StartElement("group");
    xml.Attribute("pattern",
                  "*.f;*.F;*.f77;*.F77;*.f90;*.F90;*.for;*.f95;*.F95");
    xml.Attribute("name", "Fortran Sources");
    xml.EndElement();
  }

  xml.StartElement("group");
  xml.Attribute("pattern", "*.ui");
  xml.Attribute("name", "Qt Designer files");
  xml.EndElement();

  xml.Element("hidenonprojectfiles", "true");
  xml.EndElement(); // groups

  xml.StartElement("tree");
  xml.Element("hidepatterns", "*.o,*.lo,CVS,*~,cmake*");
  xml.Element("hidenonprojectfiles", "true");
  xml.EndElement(); // tree

  xml.EndElement(); // kdevfileview
  xml.EndElement(); // kdevelop;
  xml.EndDocument();

  if (sessionFilename.empty()) {
    return;
  }

  // and a session file, so that kdevelop opens a file if it opens the
  // project the first time
  cmGeneratedFileStream devses(sessionFilename.c_str());
  if (!devses) {
    return;
  }
  cmXMLWriter sesxml(devses);
  sesxml.StartDocument("UTF-8");
  sesxml.Doctype("KDevPrjSession");
  sesxml.StartElement("KDevPrjSession");

  sesxml.StartElement("DocsAndViews");
  sesxml.Attribute("NumberOfDocuments", 1);

  sesxml.StartElement("Doc0");
  sesxml.Attribute("NumberOfViews", 1);
  sesxml.Attribute("URL", "file://" + fileToOpen);

  sesxml.StartElement("View0");
  sesxml.Attribute("line", 0);
  sesxml.Attribute("Type", "Source");
  sesxml.EndElement(); // View0

  sesxml.EndElement(); // Doc0
  sesxml.EndElement(); // DocsAndViews
  sesxml.EndElement(); // KDevPrjSession;
}
