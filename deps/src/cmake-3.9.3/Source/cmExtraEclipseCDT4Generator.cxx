/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExtraEclipseCDT4Generator.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <assert.h>
#include <map>
#include <sstream>
#include <stdio.h>
#include <utility>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSourceGroup.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"
#include "cmake.h"

static void AppendAttribute(cmXMLWriter& xml, const char* keyval)
{
  xml.StartElement("attribute");
  xml.Attribute("key", keyval);
  xml.Attribute("value", keyval);
  xml.EndElement();
}

template <typename T>
void AppendDictionary(cmXMLWriter& xml, const char* key, T const& value)
{
  xml.StartElement("dictionary");
  xml.Element("key", key);
  xml.Element("value", value);
  xml.EndElement();
}

cmExtraEclipseCDT4Generator::cmExtraEclipseCDT4Generator()
  : cmExternalMakefileProjectGenerator()
{
  this->SupportsVirtualFolders = true;
  this->GenerateLinkedResources = true;
  this->SupportsGmakeErrorParser = true;
  this->SupportsMachO64Parser = true;
  this->CEnabled = false;
  this->CXXEnabled = false;
}

cmExternalMakefileProjectGeneratorFactory*
cmExtraEclipseCDT4Generator::GetFactory()
{
  static cmExternalMakefileProjectGeneratorSimpleFactory<
    cmExtraEclipseCDT4Generator>
    factory("Eclipse CDT4", "Generates Eclipse CDT 4.0 project files.");

  if (factory.GetSupportedGlobalGenerators().empty()) {
// TODO: Verify if __CYGWIN__ should be checked.
//#if defined(_WIN32) && !defined(__CYGWIN__)
#if defined(_WIN32)
    factory.AddSupportedGlobalGenerator("NMake Makefiles");
    factory.AddSupportedGlobalGenerator("MinGW Makefiles");
// factory.AddSupportedGlobalGenerator("MSYS Makefiles");
#endif
    factory.AddSupportedGlobalGenerator("Ninja");
    factory.AddSupportedGlobalGenerator("Unix Makefiles");
  }

  return &factory;
}

void cmExtraEclipseCDT4Generator::EnableLanguage(
  std::vector<std::string> const& languages, cmMakefile* /*unused*/,
  bool /*optional*/)
{
  for (std::vector<std::string>::const_iterator lit = languages.begin();
       lit != languages.end(); ++lit) {
    if (*lit == "CXX") {
      this->Natures.insert("org.eclipse.cdt.core.ccnature");
      this->Natures.insert("org.eclipse.cdt.core.cnature");
      this->CXXEnabled = true;
    } else if (*lit == "C") {
      this->Natures.insert("org.eclipse.cdt.core.cnature");
      this->CEnabled = true;
    } else if (*lit == "Java") {
      this->Natures.insert("org.eclipse.jdt.core.javanature");
    }
  }
}

void cmExtraEclipseCDT4Generator::Generate()
{
  cmLocalGenerator* lg = this->GlobalGenerator->GetLocalGenerators()[0];
  const cmMakefile* mf = lg->GetMakefile();

  std::string eclipseVersion = mf->GetSafeDefinition("CMAKE_ECLIPSE_VERSION");
  cmsys::RegularExpression regex(".*([0-9]+\\.[0-9]+).*");
  if (regex.find(eclipseVersion.c_str())) {
    unsigned int majorVersion = 0;
    unsigned int minorVersion = 0;
    int res =
      sscanf(regex.match(1).c_str(), "%u.%u", &majorVersion, &minorVersion);
    if (res == 2) {
      int version = majorVersion * 1000 + minorVersion;
      if (version < 3006) // 3.6 is Helios
      {
        this->SupportsVirtualFolders = false;
        this->SupportsMachO64Parser = false;
      }
      if (version < 3007) // 3.7 is Indigo
      {
        this->SupportsGmakeErrorParser = false;
      }
    }
  }

  // TODO: Decide if these are local or member variables
  this->HomeDirectory = lg->GetSourceDirectory();
  this->HomeOutputDirectory = lg->GetBinaryDirectory();

  this->GenerateLinkedResources =
    mf->IsOn("CMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES");

  this->IsOutOfSourceBuild =
    (this->HomeDirectory != this->HomeOutputDirectory);

  this->GenerateSourceProject =
    (this->IsOutOfSourceBuild &&
     mf->IsOn("CMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT"));

  if (!this->GenerateSourceProject &&
      (mf->IsOn("ECLIPSE_CDT4_GENERATE_SOURCE_PROJECT"))) {
    mf->IssueMessage(
      cmake::WARNING,
      "ECLIPSE_CDT4_GENERATE_SOURCE_PROJECT is set to TRUE, "
      "but this variable is not supported anymore since CMake 2.8.7.\n"
      "Enable CMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT instead.");
  }

  if (cmSystemTools::IsSubDirectory(this->HomeOutputDirectory,
                                    this->HomeDirectory)) {
    mf->IssueMessage(cmake::WARNING,
                     "The build directory is a subdirectory "
                     "of the source directory.\n"
                     "This is not supported well by Eclipse. It is strongly "
                     "recommended to use a build directory which is a "
                     "sibling of the source directory.");
  }

  // NOTE: This is not good, since it pollutes the source tree. However,
  //       Eclipse doesn't allow CVS/SVN to work when the .project is not in
  //       the cvs/svn root directory. Hence, this is provided as an option.
  if (this->GenerateSourceProject) {
    // create .project file in the source tree
    this->CreateSourceProjectFile();
  }

  // create a .project file
  this->CreateProjectFile();

  // create a .cproject file
  this->CreateCProjectFile();
}

void cmExtraEclipseCDT4Generator::CreateSourceProjectFile()
{
  assert(this->HomeDirectory != this->HomeOutputDirectory);

  // set up the project name: <project>-Source@<baseSourcePathName>
  cmLocalGenerator* lg = this->GlobalGenerator->GetLocalGenerators()[0];
  std::string name =
    this->GenerateProjectName(lg->GetProjectName(), "Source",
                              this->GetPathBasename(this->HomeDirectory));

  const std::string filename = this->HomeDirectory + "/.project";
  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  cmXMLWriter xml(fout);
  xml.StartDocument("UTF-8");
  xml.StartElement("projectDescription");
  xml.Element("name", name);
  xml.Element("comment", "");
  xml.Element("projects", "");
  xml.Element("buildSpec", "");
  xml.Element("natures", "");
  xml.StartElement("linkedResources");

  if (this->SupportsVirtualFolders) {
    this->CreateLinksToSubprojects(xml, this->HomeDirectory);
    this->SrcLinkedResources.clear();
  }

  xml.EndElement(); // linkedResources
  xml.EndElement(); // projectDescription
  xml.EndDocument();
}

void cmExtraEclipseCDT4Generator::AddEnvVar(std::ostream& out,
                                            const char* envVar,
                                            cmLocalGenerator* lg)
{
  cmMakefile* mf = lg->GetMakefile();

  // get the variables from the environment and from the cache and then
  // figure out which one to use:

  std::string envVarValue;
  const bool envVarSet = cmSystemTools::GetEnv(envVar, envVarValue);

  std::string cacheEntryName = "CMAKE_ECLIPSE_ENVVAR_";
  cacheEntryName += envVar;
  const char* cacheValue =
    lg->GetState()->GetInitializedCacheValue(cacheEntryName);

  // now we have both, decide which one to use
  std::string valueToUse;
  if (!envVarSet && cacheValue == CM_NULLPTR) {
    // nothing known, do nothing
    valueToUse = "";
  } else if (envVarSet && cacheValue == CM_NULLPTR) {
    // The variable is in the env, but not in the cache. Use it and put it
    // in the cache
    valueToUse = envVarValue;
    mf->AddCacheDefinition(cacheEntryName, valueToUse.c_str(),
                           cacheEntryName.c_str(), cmStateEnums::STRING, true);
    mf->GetCMakeInstance()->SaveCache(lg->GetBinaryDirectory());
  } else if (!envVarSet && cacheValue != CM_NULLPTR) {
    // It is already in the cache, but not in the env, so use it from the cache
    valueToUse = cacheValue;
  } else {
    // It is both in the cache and in the env.
    // Use the version from the env. except if the value from the env is
    // completely contained in the value from the cache (for the case that we
    // now have a PATH without MSVC dirs in the env. but had the full PATH with
    // all MSVC dirs during the cmake run which stored the var in the cache:
    valueToUse = cacheValue;
    if (valueToUse.find(envVarValue) == std::string::npos) {
      valueToUse = envVarValue;
      mf->AddCacheDefinition(cacheEntryName, valueToUse.c_str(),
                             cacheEntryName.c_str(), cmStateEnums::STRING,
                             true);
      mf->GetCMakeInstance()->SaveCache(lg->GetBinaryDirectory());
    }
  }

  if (!valueToUse.empty()) {
    out << envVar << "=" << valueToUse << "|";
  }
}

void cmExtraEclipseCDT4Generator::CreateProjectFile()
{
  cmLocalGenerator* lg = this->GlobalGenerator->GetLocalGenerators()[0];
  cmMakefile* mf = lg->GetMakefile();

  const std::string filename = this->HomeOutputDirectory + "/.project";

  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  std::string compilerId = mf->GetSafeDefinition("CMAKE_C_COMPILER_ID");
  if (compilerId.empty()) // no C compiler, try the C++ compiler:
  {
    compilerId = mf->GetSafeDefinition("CMAKE_CXX_COMPILER_ID");
  }

  cmXMLWriter xml(fout);

  xml.StartDocument("UTF-8");
  xml.StartElement("projectDescription");

  xml.Element("name", this->GenerateProjectName(
                        lg->GetProjectName(),
                        mf->GetSafeDefinition("CMAKE_BUILD_TYPE"),
                        this->GetPathBasename(this->HomeOutputDirectory)));

  xml.Element("comment", "");
  xml.Element("projects", "");

  xml.StartElement("buildSpec");
  xml.StartElement("buildCommand");
  xml.Element("name", "org.eclipse.cdt.make.core.makeBuilder");
  xml.Element("triggers", "clean,full,incremental,");
  xml.StartElement("arguments");

  // use clean target
  AppendDictionary(xml, "org.eclipse.cdt.make.core.cleanBuildTarget", "clean");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.enableCleanBuild", "true");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.append_environment",
                   "true");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.stopOnError", "true");

  // set the make command
  AppendDictionary(xml, "org.eclipse.cdt.make.core.enabledIncrementalBuild",
                   "true");
  AppendDictionary(
    xml, "org.eclipse.cdt.make.core.build.command",
    this->GetEclipsePath(mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM")));
  AppendDictionary(xml, "org.eclipse.cdt.make.core.contents",
                   "org.eclipse.cdt.make.core.activeConfigSettings");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.build.target.inc", "all");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.build.arguments",
                   mf->GetSafeDefinition("CMAKE_ECLIPSE_MAKE_ARGUMENTS"));
  AppendDictionary(xml, "org.eclipse.cdt.make.core.buildLocation",
                   this->GetEclipsePath(this->HomeOutputDirectory));
  AppendDictionary(xml, "org.eclipse.cdt.make.core.useDefaultBuildCmd",
                   "false");

  // set project specific environment
  std::ostringstream environment;
  environment << "VERBOSE=1|CMAKE_NO_VERBOSE=1|"; // verbose Makefile output
  // set vsvars32.bat environment available at CMake time,
  //   but not necessarily when eclipse is open
  if (compilerId == "MSVC") {
    AddEnvVar(environment, "PATH", lg);
    AddEnvVar(environment, "INCLUDE", lg);
    AddEnvVar(environment, "LIB", lg);
    AddEnvVar(environment, "LIBPATH", lg);
  } else if (compilerId == "Intel") {
    // if the env.var is set, use this one and put it in the cache
    // if the env.var is not set, but the value is in the cache,
    // use it from the cache:
    AddEnvVar(environment, "INTEL_LICENSE_FILE", lg);
  }
  AppendDictionary(xml, "org.eclipse.cdt.make.core.environment",
                   environment.str());

  AppendDictionary(xml, "org.eclipse.cdt.make.core.enableFullBuild", "true");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.build.target.auto", "all");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.enableAutoBuild", "false");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.build.target.clean",
                   "clean");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.fullBuildTarget", "all");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.buildArguments", "");
  AppendDictionary(xml, "org.eclipse.cdt.make.core.build.location",
                   this->GetEclipsePath(this->HomeOutputDirectory));
  AppendDictionary(xml, "org.eclipse.cdt.make.core.autoBuildTarget", "all");

  // set error parsers
  std::ostringstream errorOutputParser;

  if (compilerId == "MSVC") {
    errorOutputParser << "org.eclipse.cdt.core.VCErrorParser;";
  } else if (compilerId == "Intel") {
    errorOutputParser << "org.eclipse.cdt.core.ICCErrorParser;";
  }

  if (this->SupportsGmakeErrorParser) {
    errorOutputParser << "org.eclipse.cdt.core.GmakeErrorParser;";
  } else {
    errorOutputParser << "org.eclipse.cdt.core.MakeErrorParser;";
  }

  errorOutputParser << "org.eclipse.cdt.core.GCCErrorParser;"
                       "org.eclipse.cdt.core.GASErrorParser;"
                       "org.eclipse.cdt.core.GLDErrorParser;";
  AppendDictionary(xml, "org.eclipse.cdt.core.errorOutputParser",
                   errorOutputParser.str());

  xml.EndElement(); // arguments
  xml.EndElement(); // buildCommand
  xml.StartElement("buildCommand");
  xml.Element("name", "org.eclipse.cdt.make.core.ScannerConfigBuilder");
  xml.StartElement("arguments");
  xml.EndElement(); // arguments
  xml.EndElement(); // buildCommand
  xml.EndElement(); // buildSpec

  // set natures for c/c++ projects
  xml.StartElement("natures");
  xml.Element("nature", "org.eclipse.cdt.make.core.makeNature");
  xml.Element("nature", "org.eclipse.cdt.make.core.ScannerConfigNature");
  ;

  for (std::set<std::string>::const_iterator nit = this->Natures.begin();
       nit != this->Natures.end(); ++nit) {
    xml.Element("nature", *nit);
  }

  if (const char* extraNaturesProp =
        mf->GetState()->GetGlobalProperty("ECLIPSE_EXTRA_NATURES")) {
    std::vector<std::string> extraNatures;
    cmSystemTools::ExpandListArgument(extraNaturesProp, extraNatures);
    for (std::vector<std::string>::const_iterator nit = extraNatures.begin();
         nit != extraNatures.end(); ++nit) {
      xml.Element("nature", *nit);
    }
  }

  xml.EndElement(); // natures

  xml.StartElement("linkedResources");
  // create linked resources
  if (this->IsOutOfSourceBuild) {
    // create a linked resource to CMAKE_SOURCE_DIR
    // (this is not done anymore for each project because of
    // https://gitlab.kitware.com/cmake/cmake/issues/9978 and because I found
    // it actually quite confusing in bigger projects with many directories and
    // projects, Alex

    std::string sourceLinkedResourceName = "[Source directory]";
    std::string linkSourceDirectory =
      this->GetEclipsePath(lg->GetCurrentSourceDirectory());
    // .project dir can't be subdir of a linked resource dir
    if (!cmSystemTools::IsSubDirectory(this->HomeOutputDirectory,
                                       linkSourceDirectory)) {
      this->AppendLinkedResource(xml, sourceLinkedResourceName,
                                 this->GetEclipsePath(linkSourceDirectory),
                                 LinkToFolder);
      this->SrcLinkedResources.push_back(sourceLinkedResourceName);
    }
  }

  if (this->SupportsVirtualFolders) {
    this->CreateLinksToSubprojects(xml, this->HomeOutputDirectory);

    this->CreateLinksForTargets(xml);
  }

  xml.EndElement(); // linkedResources
  xml.EndElement(); // projectDescription
}

void cmExtraEclipseCDT4Generator::WriteGroups(
  std::vector<cmSourceGroup> const& sourceGroups, std::string& linkName,
  cmXMLWriter& xml)
{
  for (std::vector<cmSourceGroup>::const_iterator sgIt = sourceGroups.begin();
       sgIt != sourceGroups.end(); ++sgIt) {
    std::string linkName3 = linkName;
    linkName3 += "/";
    linkName3 += sgIt->GetFullName();

    std::replace(linkName3.begin(), linkName3.end(), '\\', '/');

    this->AppendLinkedResource(xml, linkName3, "virtual:/virtual",
                               VirtualFolder);
    std::vector<cmSourceGroup> const& children = sgIt->GetGroupChildren();
    if (!children.empty()) {
      this->WriteGroups(children, linkName, xml);
    }
    std::vector<const cmSourceFile*> sFiles = sgIt->GetSourceFiles();
    for (std::vector<const cmSourceFile*>::const_iterator fileIt =
           sFiles.begin();
         fileIt != sFiles.end(); ++fileIt) {
      std::string const& fullPath = (*fileIt)->GetFullPath();

      if (!cmSystemTools::FileIsDirectory(fullPath)) {
        std::string linkName4 = linkName3;
        linkName4 += "/";
        linkName4 += cmSystemTools::GetFilenameName(fullPath);
        this->AppendLinkedResource(xml, linkName4,
                                   this->GetEclipsePath(fullPath), LinkToFile);
      }
    }
  }
}

void cmExtraEclipseCDT4Generator::CreateLinksForTargets(cmXMLWriter& xml)
{
  std::string linkName = "[Targets]";
  this->AppendLinkedResource(xml, linkName, "virtual:/virtual", VirtualFolder);

  for (std::vector<cmLocalGenerator*>::const_iterator lgIt =
         this->GlobalGenerator->GetLocalGenerators().begin();
       lgIt != this->GlobalGenerator->GetLocalGenerators().end(); ++lgIt) {
    cmMakefile* makefile = (*lgIt)->GetMakefile();
    const std::vector<cmGeneratorTarget*> targets =
      (*lgIt)->GetGeneratorTargets();

    for (std::vector<cmGeneratorTarget*>::const_iterator ti = targets.begin();
         ti != targets.end(); ++ti) {
      std::string linkName2 = linkName;
      linkName2 += "/";
      switch ((*ti)->GetType()) {
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY: {
          const char* prefix =
            ((*ti)->GetType() == cmStateEnums::EXECUTABLE ? "[exe] "
                                                          : "[lib] ");
          linkName2 += prefix;
          linkName2 += (*ti)->GetName();
          this->AppendLinkedResource(xml, linkName2, "virtual:/virtual",
                                     VirtualFolder);
          if (!this->GenerateLinkedResources) {
            break; // skip generating the linked resources to the source files
          }
          std::vector<cmSourceGroup> sourceGroups =
            makefile->GetSourceGroups();
          // get the files from the source lists then add them to the groups
          cmGeneratorTarget* gt = const_cast<cmGeneratorTarget*>(*ti);
          std::vector<cmSourceFile*> files;
          gt->GetSourceFiles(files,
                             makefile->GetSafeDefinition("CMAKE_BUILD_TYPE"));
          for (std::vector<cmSourceFile*>::const_iterator sfIt = files.begin();
               sfIt != files.end(); sfIt++) {
            // Add the file to the list of sources.
            std::string const& source = (*sfIt)->GetFullPath();
            cmSourceGroup* sourceGroup =
              makefile->FindSourceGroup(source.c_str(), sourceGroups);
            sourceGroup->AssignSource(*sfIt);
          }

          this->WriteGroups(sourceGroups, linkName2, xml);
        } break;
        // ignore all others:
        default:
          break;
      }
    }
  }
}

void cmExtraEclipseCDT4Generator::CreateLinksToSubprojects(
  cmXMLWriter& xml, const std::string& baseDir)
{
  if (!this->GenerateLinkedResources) {
    return;
  }

  // for each sub project create a linked resource to the source dir
  // - only if it is an out-of-source build
  this->AppendLinkedResource(xml, "[Subprojects]", "virtual:/virtual",
                             VirtualFolder);

  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
         it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end(); ++it) {
    std::string linkSourceDirectory =
      this->GetEclipsePath(it->second[0]->GetCurrentSourceDirectory());
    // a linked resource must not point to a parent directory of .project or
    // .project itself
    if ((baseDir != linkSourceDirectory) &&
        !cmSystemTools::IsSubDirectory(baseDir, linkSourceDirectory)) {
      std::string linkName = "[Subprojects]/";
      linkName += it->first;
      this->AppendLinkedResource(xml, linkName,
                                 this->GetEclipsePath(linkSourceDirectory),
                                 LinkToFolder);
      // Don't add it to the srcLinkedResources, because listing multiple
      // directories confuses the Eclipse indexer (#13596).
    }
  }
}

void cmExtraEclipseCDT4Generator::AppendIncludeDirectories(
  cmXMLWriter& xml, const std::vector<std::string>& includeDirs,
  std::set<std::string>& emittedDirs)
{
  for (std::vector<std::string>::const_iterator inc = includeDirs.begin();
       inc != includeDirs.end(); ++inc) {
    if (!inc->empty()) {
      std::string dir = cmSystemTools::CollapseFullPath(*inc);

      // handle framework include dirs on OSX, the remainder after the
      // Frameworks/ part has to be stripped
      //   /System/Library/Frameworks/GLUT.framework/Headers
      cmsys::RegularExpression frameworkRx("(.+/Frameworks)/.+\\.framework/");
      if (frameworkRx.find(dir.c_str())) {
        dir = frameworkRx.match(1);
      }

      if (emittedDirs.find(dir) == emittedDirs.end()) {
        emittedDirs.insert(dir);
        xml.StartElement("pathentry");
        xml.Attribute("include",
                      cmExtraEclipseCDT4Generator::GetEclipsePath(dir));
        xml.Attribute("kind", "inc");
        xml.Attribute("path", "");
        xml.Attribute("system", "true");
        xml.EndElement();
      }
    }
  }
}

void cmExtraEclipseCDT4Generator::CreateCProjectFile() const
{
  std::set<std::string> emmited;

  cmLocalGenerator* lg = this->GlobalGenerator->GetLocalGenerators()[0];
  const cmMakefile* mf = lg->GetMakefile();

  const std::string filename = this->HomeOutputDirectory + "/.cproject";

  cmGeneratedFileStream fout(filename.c_str());
  if (!fout) {
    return;
  }

  cmXMLWriter xml(fout);

  // add header
  xml.StartDocument("UTF-8");
  xml.ProcessingInstruction("fileVersion", "4.0.0");
  xml.StartElement("cproject");
  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "org.eclipse.cdt.core.settings");

  xml.StartElement("cconfiguration");
  xml.Attribute("id", "org.eclipse.cdt.core.default.config.1");

  // Configuration settings...
  xml.StartElement("storageModule");
  xml.Attribute("buildSystemId",
                "org.eclipse.cdt.core.defaultConfigDataProvider");
  xml.Attribute("id", "org.eclipse.cdt.core.default.config.1");
  xml.Attribute("moduleId", "org.eclipse.cdt.core.settings");
  xml.Attribute("name", "Configuration");
  xml.Element("externalSettings");
  xml.StartElement("extensions");

  // TODO: refactor this out...
  std::string executableFormat =
    mf->GetSafeDefinition("CMAKE_EXECUTABLE_FORMAT");
  if (executableFormat == "ELF") {
    xml.StartElement("extension");
    xml.Attribute("id", "org.eclipse.cdt.core.ELF");
    xml.Attribute("point", "org.eclipse.cdt.core.BinaryParser");
    xml.EndElement(); // extension

    xml.StartElement("extension");
    xml.Attribute("id", "org.eclipse.cdt.core.GNU_ELF");
    xml.Attribute("point", "org.eclipse.cdt.core.BinaryParser");
    AppendAttribute(xml, "addr2line");
    AppendAttribute(xml, "c++filt");
    xml.EndElement(); // extension
  } else {
    std::string systemName = mf->GetSafeDefinition("CMAKE_SYSTEM_NAME");
    if (systemName == "CYGWIN") {
      xml.StartElement("extension");
      xml.Attribute("id", "org.eclipse.cdt.core.Cygwin_PE");
      xml.Attribute("point", "org.eclipse.cdt.core.BinaryParser");
      AppendAttribute(xml, "addr2line");
      AppendAttribute(xml, "c++filt");
      AppendAttribute(xml, "cygpath");
      AppendAttribute(xml, "nm");
      xml.EndElement(); // extension
    } else if (systemName == "Windows") {
      xml.StartElement("extension");
      xml.Attribute("id", "org.eclipse.cdt.core.PE");
      xml.Attribute("point", "org.eclipse.cdt.core.BinaryParser");
      xml.EndElement(); // extension
    } else if (systemName == "Darwin") {
      xml.StartElement("extension");
      xml.Attribute("id", this->SupportsMachO64Parser
                      ? "org.eclipse.cdt.core.MachO64"
                      : "org.eclipse.cdt.core.MachO");
      xml.Attribute("point", "org.eclipse.cdt.core.BinaryParser");
      AppendAttribute(xml, "c++filt");
      xml.EndElement(); // extension
    } else {
      // *** Should never get here ***
      xml.Element("error_toolchain_type");
    }
  }

  xml.EndElement(); // extensions
  xml.EndElement(); // storageModule

  // ???
  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "org.eclipse.cdt.core.language.mapping");
  xml.Element("project-mappings");
  xml.EndElement(); // storageModule

  // ???
  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "org.eclipse.cdt.core.externalSettings");
  xml.EndElement(); // storageModule

  // set the path entries (includes, libs, source dirs, etc.)
  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "org.eclipse.cdt.core.pathentry");

  // for each sub project with a linked resource to the source dir:
  // - make it type 'src'
  // - and exclude it from type 'out'
  std::string excludeFromOut;
  /* I don't know what the pathentry kind="src" are good for, e.g.
   * autocompletion
   * works also without them. Done wrong, the indexer complains, see #12417
   * and #12213.
   * According to #13596, this entry at least limits the directories the
   * indexer is searching for files. So now the "src" entry contains only
   * the linked resource to CMAKE_SOURCE_DIR.
   * The CDT documentation is very terse on that:
   * "CDT_SOURCE: Entry kind constant describing a path entry identifying a
   * folder containing source code to be compiled."
   * Also on the cdt-dev list didn't bring any information:
   * http://web.archiveorange.com/archive/v/B4NlJDNIpYoOS1SbxFNy
   * Alex */
  // include subprojects directory to the src pathentry
  // eclipse cdt indexer uses this entries as reference to index source files
  if (this->GenerateLinkedResources) {
    xml.StartElement("pathentry");
    xml.Attribute("kind", "src");
    xml.Attribute("path", "[Subprojects]");
    xml.EndElement();
  }

  for (std::vector<std::string>::const_iterator it =
         this->SrcLinkedResources.begin();
       it != this->SrcLinkedResources.end(); ++it) {
    xml.StartElement("pathentry");
    xml.Attribute("kind", "src");
    xml.Attribute("path", *it);
    xml.EndElement();

    // exlude source directory from output search path
    // - only if not named the same as an output directory
    if (!cmSystemTools::FileIsDirectory(
          std::string(this->HomeOutputDirectory + "/" + *it))) {
      excludeFromOut += *it + "/|";
    }
  }

  excludeFromOut += "**/CMakeFiles/";

  xml.StartElement("pathentry");
  xml.Attribute("excluding", excludeFromOut);
  xml.Attribute("kind", "out");
  xml.Attribute("path", "");
  xml.EndElement();

  // add pre-processor definitions to allow eclipse to gray out sections
  emmited.clear();
  for (std::vector<cmLocalGenerator*>::const_iterator it =
         this->GlobalGenerator->GetLocalGenerators().begin();
       it != this->GlobalGenerator->GetLocalGenerators().end(); ++it) {

    if (const char* cdefs =
          (*it)->GetMakefile()->GetProperty("COMPILE_DEFINITIONS")) {
      // Expand the list.
      std::vector<std::string> defs;
      cmGeneratorExpression::Split(cdefs, defs);

      for (std::vector<std::string>::const_iterator di = defs.begin();
           di != defs.end(); ++di) {
        if (cmGeneratorExpression::Find(*di) != std::string::npos) {
          continue;
        }

        std::string::size_type equals = di->find('=', 0);
        std::string::size_type enddef = di->length();

        std::string def;
        std::string val;
        if (equals != std::string::npos && equals < enddef) {
          // we have -DFOO=BAR
          def = di->substr(0, equals);
          val = di->substr(equals + 1, enddef - equals + 1);
        } else {
          // we have -DFOO
          def = *di;
        }

        // insert the definition if not already added.
        if (emmited.find(def) == emmited.end()) {
          emmited.insert(def);
          xml.StartElement("pathentry");
          xml.Attribute("kind", "mac");
          xml.Attribute("name", def);
          xml.Attribute("path", "");
          xml.Attribute("value", val);
          xml.EndElement();
        }
      }
    }
  }
  // add system defined c macros
  const char* cDefs =
    mf->GetDefinition("CMAKE_EXTRA_GENERATOR_C_SYSTEM_DEFINED_MACROS");
  if (this->CEnabled && cDefs) {
    // Expand the list.
    std::vector<std::string> defs;
    cmSystemTools::ExpandListArgument(cDefs, defs, true);

    // the list must contain only definition-value pairs:
    if ((defs.size() % 2) == 0) {
      std::vector<std::string>::const_iterator di = defs.begin();
      while (di != defs.end()) {
        std::string def = *di;
        ++di;
        std::string val;
        if (di != defs.end()) {
          val = *di;
          ++di;
        }

        // insert the definition if not already added.
        if (emmited.find(def) == emmited.end()) {
          emmited.insert(def);
          xml.StartElement("pathentry");
          xml.Attribute("kind", "mac");
          xml.Attribute("name", def);
          xml.Attribute("path", "");
          xml.Attribute("value", val);
          xml.EndElement();
        }
      }
    }
  }
  // add system defined c++ macros
  const char* cxxDefs =
    mf->GetDefinition("CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_DEFINED_MACROS");
  if (this->CXXEnabled && cxxDefs) {
    // Expand the list.
    std::vector<std::string> defs;
    cmSystemTools::ExpandListArgument(cxxDefs, defs, true);

    // the list must contain only definition-value pairs:
    if ((defs.size() % 2) == 0) {
      std::vector<std::string>::const_iterator di = defs.begin();
      while (di != defs.end()) {
        std::string def = *di;
        ++di;
        std::string val;
        if (di != defs.end()) {
          val = *di;
          ++di;
        }

        // insert the definition if not already added.
        if (emmited.find(def) == emmited.end()) {
          emmited.insert(def);
          xml.StartElement("pathentry");
          xml.Attribute("kind", "mac");
          xml.Attribute("name", def);
          xml.Attribute("path", "");
          xml.Attribute("value", val);
          xml.EndElement();
        }
      }
    }
  }

  // include dirs
  emmited.clear();
  for (std::vector<cmLocalGenerator*>::const_iterator it =
         this->GlobalGenerator->GetLocalGenerators().begin();
       it != this->GlobalGenerator->GetLocalGenerators().end(); ++it) {
    std::vector<cmGeneratorTarget*> targets = (*it)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator l = targets.begin();
         l != targets.end(); ++l) {
      std::vector<std::string> includeDirs;
      std::string config = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");
      (*it)->GetIncludeDirectories(includeDirs, *l, "C", config);
      this->AppendIncludeDirectories(xml, includeDirs, emmited);
    }
  }
  // now also the system include directories, in case we found them in
  // CMakeSystemSpecificInformation.cmake. This makes Eclipse find the
  // standard headers.
  std::string compiler = mf->GetSafeDefinition("CMAKE_C_COMPILER");
  if (this->CEnabled && !compiler.empty()) {
    std::string systemIncludeDirs =
      mf->GetSafeDefinition("CMAKE_EXTRA_GENERATOR_C_SYSTEM_INCLUDE_DIRS");
    std::vector<std::string> dirs;
    cmSystemTools::ExpandListArgument(systemIncludeDirs, dirs);
    this->AppendIncludeDirectories(xml, dirs, emmited);
  }
  compiler = mf->GetSafeDefinition("CMAKE_CXX_COMPILER");
  if (this->CXXEnabled && !compiler.empty()) {
    std::string systemIncludeDirs =
      mf->GetSafeDefinition("CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS");
    std::vector<std::string> dirs;
    cmSystemTools::ExpandListArgument(systemIncludeDirs, dirs);
    this->AppendIncludeDirectories(xml, dirs, emmited);
  }

  xml.EndElement(); // storageModule

  // add build targets
  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "org.eclipse.cdt.make.core.buildtargets");
  xml.StartElement("buildTargets");
  emmited.clear();
  const std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  const std::string makeArgs =
    mf->GetSafeDefinition("CMAKE_ECLIPSE_MAKE_ARGUMENTS");

  cmGlobalGenerator* generator =
    const_cast<cmGlobalGenerator*>(this->GlobalGenerator);

  std::string allTarget;
  std::string cleanTarget;
  if (generator->GetAllTargetName()) {
    allTarget = generator->GetAllTargetName();
  }
  if (generator->GetCleanTargetName()) {
    cleanTarget = generator->GetCleanTargetName();
  }

  // add all executable and library targets and some of the GLOBAL
  // and UTILITY targets
  for (std::vector<cmLocalGenerator*>::const_iterator it =
         this->GlobalGenerator->GetLocalGenerators().begin();
       it != this->GlobalGenerator->GetLocalGenerators().end(); ++it) {
    const std::vector<cmGeneratorTarget*> targets =
      (*it)->GetGeneratorTargets();
    std::string subdir = (*it)->ConvertToRelativePath(
      this->HomeOutputDirectory, (*it)->GetCurrentBinaryDirectory());
    if (subdir == ".") {
      subdir = "";
    }

    for (std::vector<cmGeneratorTarget*>::const_iterator ti = targets.begin();
         ti != targets.end(); ++ti) {
      std::string targetName = (*ti)->GetName();
      switch ((*ti)->GetType()) {
        case cmStateEnums::GLOBAL_TARGET: {
          // Only add the global targets from CMAKE_BINARY_DIR,
          // not from the subdirs
          if (subdir.empty()) {
            this->AppendTarget(xml, targetName, make, makeArgs, subdir, ": ");
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

          this->AppendTarget(xml, targetName, make, makeArgs, subdir, ": ");
          break;
        case cmStateEnums::EXECUTABLE:
        case cmStateEnums::STATIC_LIBRARY:
        case cmStateEnums::SHARED_LIBRARY:
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::OBJECT_LIBRARY: {
          const char* prefix =
            ((*ti)->GetType() == cmStateEnums::EXECUTABLE ? "[exe] "
                                                          : "[lib] ");
          this->AppendTarget(xml, targetName, make, makeArgs, subdir, prefix);
          std::string fastTarget = targetName;
          fastTarget += "/fast";
          this->AppendTarget(xml, fastTarget, make, makeArgs, subdir, prefix);

          // Add Build and Clean targets in the virtual folder of targets:
          if (this->SupportsVirtualFolders) {
            std::string virtDir = "[Targets]/";
            virtDir += prefix;
            virtDir += targetName;
            std::string buildArgs = "-C \"";
            buildArgs += (*it)->GetBinaryDirectory();
            buildArgs += "\" ";
            buildArgs += makeArgs;
            this->AppendTarget(xml, "Build", make, buildArgs, virtDir, "",
                               targetName.c_str());

            std::string cleanArgs = "-E chdir \"";
            cleanArgs += (*it)->GetCurrentBinaryDirectory();
            cleanArgs += "\" \"";
            cleanArgs += cmSystemTools::GetCMakeCommand();
            cleanArgs += "\" -P \"";
            cmGeneratorTarget* gt = *ti;
            cleanArgs += (*it)->GetTargetDirectory(gt);
            cleanArgs += "/cmake_clean.cmake\"";
            this->AppendTarget(xml, "Clean", cmSystemTools::GetCMakeCommand(),
                               cleanArgs, virtDir, "", "");
          }
        } break;
        default:
          break;
      }
    }

    // insert the all and clean targets in every subdir
    if (!allTarget.empty()) {
      this->AppendTarget(xml, allTarget, make, makeArgs, subdir, ": ");
    }
    if (!cleanTarget.empty()) {
      this->AppendTarget(xml, cleanTarget, make, makeArgs, subdir, ": ");
    }

    // insert rules for compiling, preprocessing and assembling individual
    // files
    std::vector<std::string> objectFileTargets;
    (*it)->GetIndividualFileTargets(objectFileTargets);
    for (std::vector<std::string>::const_iterator fit =
           objectFileTargets.begin();
         fit != objectFileTargets.end(); ++fit) {
      const char* prefix = "[obj] ";
      if ((*fit)[fit->length() - 1] == 's') {
        prefix = "[to asm] ";
      } else if ((*fit)[fit->length() - 1] == 'i') {
        prefix = "[pre] ";
      }
      this->AppendTarget(xml, *fit, make, makeArgs, subdir, prefix);
    }
  }

  xml.EndElement(); // buildTargets
  xml.EndElement(); // storageModule

  this->AppendStorageScanners(xml, *mf);

  xml.EndElement(); // cconfiguration
  xml.EndElement(); // storageModule

  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "cdtBuildSystem");
  xml.Attribute("version", "4.0.0");

  xml.StartElement("project");
  xml.Attribute("id", std::string(lg->GetProjectName()) + ".null.1");
  xml.Attribute("name", lg->GetProjectName());
  xml.EndElement(); // project

  xml.EndElement(); // storageModule
  xml.EndElement(); // cproject
}

std::string cmExtraEclipseCDT4Generator::GetEclipsePath(
  const std::string& path)
{
#if defined(__CYGWIN__)
  std::string cmd = "cygpath -m " + path;
  std::string out;
  if (!cmSystemTools::RunSingleCommand(cmd.c_str(), &out, &out)) {
    return path;
  } else {
    out.erase(out.find_last_of('\n'));
    return out;
  }
#else
  return path;
#endif
}

std::string cmExtraEclipseCDT4Generator::GetPathBasename(
  const std::string& path)
{
  std::string outputBasename = path;
  while (!outputBasename.empty() &&
         (outputBasename[outputBasename.size() - 1] == '/' ||
          outputBasename[outputBasename.size() - 1] == '\\')) {
    outputBasename.resize(outputBasename.size() - 1);
  }
  std::string::size_type loc = outputBasename.find_last_of("/\\");
  if (loc != std::string::npos) {
    outputBasename = outputBasename.substr(loc + 1);
  }

  return outputBasename;
}

std::string cmExtraEclipseCDT4Generator::GenerateProjectName(
  const std::string& name, const std::string& type, const std::string& path)
{
  return name + (type.empty() ? "" : "-") + type + "@" + path;
}

// Helper functions
void cmExtraEclipseCDT4Generator::AppendStorageScanners(
  cmXMLWriter& xml, const cmMakefile& makefile)
{
  // we need the "make" and the C (or C++) compiler which are used, Alex
  std::string make = makefile.GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string compiler = makefile.GetSafeDefinition("CMAKE_C_COMPILER");
  std::string arg1 = makefile.GetSafeDefinition("CMAKE_C_COMPILER_ARG1");
  if (compiler.empty()) {
    compiler = makefile.GetSafeDefinition("CMAKE_CXX_COMPILER");
    arg1 = makefile.GetSafeDefinition("CMAKE_CXX_COMPILER_ARG1");
  }
  if (compiler.empty()) // Hmm, what to do now ?
  {
    compiler = "gcc";
  }

  // the following right now hardcodes gcc behaviour :-/
  std::string compilerArgs =
    "-E -P -v -dD ${plugin_state_location}/${specs_file}";
  if (!arg1.empty()) {
    arg1 += " ";
    compilerArgs = arg1 + compilerArgs;
  }

  xml.StartElement("storageModule");
  xml.Attribute("moduleId", "scannerConfiguration");

  xml.StartElement("autodiscovery");
  xml.Attribute("enabled", "true");
  xml.Attribute("problemReportingEnabled", "true");
  xml.Attribute("selectedProfileId",
                "org.eclipse.cdt.make.core.GCCStandardMakePerProjectProfile");
  xml.EndElement(); // autodiscovery

  cmExtraEclipseCDT4Generator::AppendScannerProfile(
    xml, "org.eclipse.cdt.make.core.GCCStandardMakePerProjectProfile", true,
    "", true, "specsFile", compilerArgs, compiler, true, true);
  cmExtraEclipseCDT4Generator::AppendScannerProfile(
    xml, "org.eclipse.cdt.make.core.GCCStandardMakePerFileProfile", true, "",
    true, "makefileGenerator", "-f ${project_name}_scd.mk", make, true, true);

  xml.EndElement(); // storageModule
}

// The prefix is prepended before the actual name of the target. The purpose
// of that is to sort the targets in the view of Eclipse, so that at first
// the global/utility/all/clean targets appear ": ", then the executable
// targets "[exe] ", then the libraries "[lib]", then the rules for the
// object files "[obj]", then for preprocessing only "[pre] " and
// finally the assembly files "[to asm] ". Note the "to" in "to asm",
// without it, "asm" would be the first targets in the list, with the "to"
// they are the last targets, which makes more sense.
void cmExtraEclipseCDT4Generator::AppendTarget(
  cmXMLWriter& xml, const std::string& target, const std::string& make,
  const std::string& makeArgs, const std::string& path, const char* prefix,
  const char* makeTarget)
{
  xml.StartElement("target");
  xml.Attribute("name", prefix + target);
  xml.Attribute("path", path);
  xml.Attribute("targetID", "org.eclipse.cdt.make.MakeTargetBuilder");
  xml.Element("buildCommand",
              cmExtraEclipseCDT4Generator::GetEclipsePath(make));
  xml.Element("buildArguments", makeArgs);
  xml.Element("buildTarget", makeTarget ? makeTarget : target.c_str());
  xml.Element("stopOnError", "true");
  xml.Element("useDefaultCommand", "false");
  xml.EndElement();
}

void cmExtraEclipseCDT4Generator::AppendScannerProfile(
  cmXMLWriter& xml, const std::string& profileID, bool openActionEnabled,
  const std::string& openActionFilePath, bool pParserEnabled,
  const std::string& scannerInfoProviderID,
  const std::string& runActionArguments, const std::string& runActionCommand,
  bool runActionUseDefault, bool sipParserEnabled)
{
  xml.StartElement("profile");
  xml.Attribute("id", profileID);

  xml.StartElement("buildOutputProvider");
  xml.StartElement("openAction");
  xml.Attribute("enabled", openActionEnabled ? "true" : "false");
  xml.Attribute("filePath", openActionFilePath);
  xml.EndElement(); // openAction
  xml.StartElement("parser");
  xml.Attribute("enabled", pParserEnabled ? "true" : "false");
  xml.EndElement(); // parser
  xml.EndElement(); // buildOutputProvider

  xml.StartElement("scannerInfoProvider");
  xml.Attribute("id", scannerInfoProviderID);
  xml.StartElement("runAction");
  xml.Attribute("arguments", runActionArguments);
  xml.Attribute("command", runActionCommand);
  xml.Attribute("useDefault", runActionUseDefault ? "true" : "false");
  xml.EndElement(); // runAction
  xml.StartElement("parser");
  xml.Attribute("enabled", sipParserEnabled ? "true" : "false");
  xml.EndElement(); // parser
  xml.EndElement(); // scannerInfoProvider

  xml.EndElement(); // profile
}

void cmExtraEclipseCDT4Generator::AppendLinkedResource(cmXMLWriter& xml,
                                                       const std::string& name,
                                                       const std::string& path,
                                                       LinkType linkType)
{
  const char* locationTag = "location";
  int typeTag = 2;
  if (linkType == VirtualFolder) // ... and not a linked folder
  {
    locationTag = "locationURI";
  }
  if (linkType == LinkToFile) {
    typeTag = 1;
  }

  xml.StartElement("link");
  xml.Element("name", name);
  xml.Element("type", typeTag);
  xml.Element(locationTag, path);
  xml.EndElement();
}
