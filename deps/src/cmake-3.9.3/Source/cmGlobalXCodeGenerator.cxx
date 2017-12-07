/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalXCodeGenerator.h"

#include "cmsys/RegularExpression.hxx"
#include <assert.h>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <string.h>

#include "cmComputeLinkInformation.h"
#include "cmCustomCommandGenerator.h"
#include "cmDocumentationEntry.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGeneratorFactory.h"
#include "cmLocalGenerator.h"
#include "cmLocalXCodeGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmSourceFile.h"
#include "cmSourceGroup.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmXCode21Object.h"
#include "cmXCodeObject.h"
#include "cmXCodeScheme.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

struct cmLinkImplementation;

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmXMLParser.h"

// parse the xml file storing the installed version of Xcode on
// the machine
class cmXcodeVersionParser : public cmXMLParser
{
public:
  cmXcodeVersionParser()
    : Version("1.5")
  {
  }
  void StartElement(const std::string&, const char**) { this->Data = ""; }
  void EndElement(const std::string& name)
  {
    if (name == "key") {
      this->Key = this->Data;
    } else if (name == "string") {
      if (this->Key == "CFBundleShortVersionString") {
        this->Version = this->Data;
      }
    }
  }
  void CharacterDataHandler(const char* data, int length)
  {
    this->Data.append(data, length);
  }
  std::string Version;
  std::string Key;
  std::string Data;
};
#endif

// Builds either an object list or a space-separated string from the
// given inputs.
class cmGlobalXCodeGenerator::BuildObjectListOrString
{
  cmGlobalXCodeGenerator* Generator;
  cmXCodeObject* Group;
  bool Empty;
  std::string String;

public:
  BuildObjectListOrString(cmGlobalXCodeGenerator* gen, bool buildObjectList)
    : Generator(gen)
    , Group(0)
    , Empty(true)
  {
    if (buildObjectList) {
      this->Group = this->Generator->CreateObject(cmXCodeObject::OBJECT_LIST);
    }
  }

  bool IsEmpty() const { return this->Empty; }

  void Add(const std::string& newString)
  {
    this->Empty = false;

    if (this->Group) {
      this->Group->AddObject(this->Generator->CreateString(newString));
    } else {
      this->String += newString;
      this->String += ' ';
    }
  }

  const std::string& GetString() const { return this->String; }

  cmXCodeObject* CreateList()
  {
    if (this->Group) {
      return this->Group;
    } else {
      return this->Generator->CreateString(this->String);
    }
  }
};

class cmGlobalXCodeGenerator::Factory : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE;

  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    cmGlobalXCodeGenerator::GetDocumentation(entry);
  }

  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(cmGlobalXCodeGenerator::GetActualName());
  }

  bool SupportsToolset() const CM_OVERRIDE { return true; }
  bool SupportsPlatform() const CM_OVERRIDE { return false; }
};

cmGlobalXCodeGenerator::cmGlobalXCodeGenerator(
  cmake* cm, std::string const& version_string, unsigned int version_number)
  : cmGlobalGenerator(cm)
{
  this->VersionString = version_string;
  this->XcodeVersion = version_number;

  this->RootObject = 0;
  this->MainGroupChildren = 0;
  this->CurrentMakefile = 0;
  this->CurrentLocalGenerator = 0;
  this->XcodeBuildCommandInitialized = false;

  this->ObjectDirArchDefault = "$(CURRENT_ARCH)";
  this->ObjectDirArch = this->ObjectDirArchDefault;

  cm->GetState()->SetIsGeneratorMultiConfig(true);
}

cmGlobalGeneratorFactory* cmGlobalXCodeGenerator::NewFactory()
{
  return new Factory;
}

cmGlobalGenerator* cmGlobalXCodeGenerator::Factory::CreateGlobalGenerator(
  const std::string& name, cmake* cm) const
{
  if (name != GetActualName())
    return 0;
#if defined(CMAKE_BUILD_WITH_CMAKE)
  cmXcodeVersionParser parser;
  std::string versionFile;
  {
    std::string out;
    std::string::size_type pos;
    if (cmSystemTools::RunSingleCommand("xcode-select --print-path", &out, 0,
                                        0, 0, cmSystemTools::OUTPUT_NONE) &&
        (pos = out.find(".app/"), pos != std::string::npos)) {
      versionFile = out.substr(0, pos + 5) + "Contents/version.plist";
    }
  }
  if (!versionFile.empty() && cmSystemTools::FileExists(versionFile.c_str())) {
    parser.ParseFile(versionFile.c_str());
  } else if (cmSystemTools::FileExists(
               "/Applications/Xcode.app/Contents/version.plist")) {
    parser.ParseFile("/Applications/Xcode.app/Contents/version.plist");
  } else {
    parser.ParseFile(
      "/Developer/Applications/Xcode.app/Contents/version.plist");
  }
  std::string const& version_string = parser.Version;

  // Compute an integer form of the version number.
  unsigned int v[2] = { 0, 0 };
  sscanf(version_string.c_str(), "%u.%u", &v[0], &v[1]);
  unsigned int version_number = 10 * v[0] + v[1];

  if (version_number < 30) {
    cm->IssueMessage(cmake::FATAL_ERROR,
                     "Xcode " + version_string + " not supported.");
    return CM_NULLPTR;
  }

  CM_AUTO_PTR<cmGlobalXCodeGenerator> gg(
    new cmGlobalXCodeGenerator(cm, version_string, version_number));
  return gg.release();
#else
  std::cerr << "CMake should be built with cmake to use Xcode, "
               "default to Xcode 1.5\n";
  return new cmGlobalXCodeGenerator(cm);
#endif
}

bool cmGlobalXCodeGenerator::FindMakeProgram(cmMakefile* mf)
{
  // The Xcode generator knows how to lookup its build tool
  // directly instead of needing a helper module to do it, so we
  // do not actually need to put CMAKE_MAKE_PROGRAM into the cache.
  if (cmSystemTools::IsOff(mf->GetDefinition("CMAKE_MAKE_PROGRAM"))) {
    mf->AddDefinition("CMAKE_MAKE_PROGRAM",
                      this->GetXcodeBuildCommand().c_str());
  }
  return true;
}

std::string const& cmGlobalXCodeGenerator::GetXcodeBuildCommand()
{
  if (!this->XcodeBuildCommandInitialized) {
    this->XcodeBuildCommandInitialized = true;
    this->XcodeBuildCommand = this->FindXcodeBuildCommand();
  }
  return this->XcodeBuildCommand;
}

std::string cmGlobalXCodeGenerator::FindXcodeBuildCommand()
{
  if (this->XcodeVersion >= 40) {
    std::string makeProgram = cmSystemTools::FindProgram("xcodebuild");
    if (makeProgram.empty()) {
      makeProgram = "xcodebuild";
    }
    return makeProgram;
  } else {
    // Use cmakexbuild wrapper to suppress environment dump from output.
    return cmSystemTools::GetCMakeCommand() + "xbuild";
  }
}

bool cmGlobalXCodeGenerator::SetGeneratorToolset(std::string const& ts,
                                                 cmMakefile* mf)
{
  if (ts.find_first_of(",=") != std::string::npos) {
    std::ostringstream e;
    /* clang-format off */
    e <<
      "Generator\n"
      "  " << this->GetName() << "\n"
      "does not recognize the toolset\n"
      "  " << ts << "\n"
      "that was specified.";
    /* clang-format on */
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }
  this->GeneratorToolset = ts;
  if (!this->GeneratorToolset.empty()) {
    mf->AddDefinition("CMAKE_XCODE_PLATFORM_TOOLSET",
                      this->GeneratorToolset.c_str());
  }
  return true;
}

void cmGlobalXCodeGenerator::EnableLanguage(
  std::vector<std::string> const& lang, cmMakefile* mf, bool optional)
{
  mf->AddDefinition("XCODE", "1");
  mf->AddDefinition("XCODE_VERSION", this->VersionString.c_str());
  if (!mf->GetDefinition("CMAKE_CONFIGURATION_TYPES")) {
    mf->AddCacheDefinition(
      "CMAKE_CONFIGURATION_TYPES", "Debug;Release;MinSizeRel;RelWithDebInfo",
      "Semicolon separated list of supported configuration types, "
      "only supports Debug, Release, MinSizeRel, and RelWithDebInfo, "
      "anything else will be ignored.",
      cmStateEnums::STRING);
  }
  mf->AddDefinition("CMAKE_GENERATOR_NO_COMPILER_ENV", "1");
  this->cmGlobalGenerator::EnableLanguage(lang, mf, optional);
  this->ComputeArchitectures(mf);
}

void cmGlobalXCodeGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& /*projectDir*/,
  const std::string& targetName, const std::string& config, bool /*fast*/,
  bool /*verbose*/, std::vector<std::string> const& makeOptions)
{
  // now build the test
  makeCommand.push_back(
    this->SelectMakeProgram(makeProgram, this->GetXcodeBuildCommand()));

  makeCommand.push_back("-project");
  std::string projectArg = projectName;
  projectArg += ".xcode";
  projectArg += "proj";
  makeCommand.push_back(projectArg);

  bool clean = false;
  std::string realTarget = targetName;
  if (realTarget == "clean") {
    clean = true;
    realTarget = "ALL_BUILD";
  }
  if (clean) {
    makeCommand.push_back("clean");
  } else {
    makeCommand.push_back("build");
  }
  makeCommand.push_back("-target");
  if (!realTarget.empty()) {
    makeCommand.push_back(realTarget);
  } else {
    makeCommand.push_back("ALL_BUILD");
  }
  makeCommand.push_back("-configuration");
  makeCommand.push_back(!config.empty() ? config : "Debug");
  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator* cmGlobalXCodeGenerator::CreateLocalGenerator(cmMakefile* mf)
{
  return new cmLocalXCodeGenerator(this, mf);
}

void cmGlobalXCodeGenerator::AddExtraIDETargets()
{
  std::map<std::string, std::vector<cmLocalGenerator*> >::iterator it;
  // make sure extra targets are added before calling
  // the parent generate which will call trace depends
  for (it = this->ProjectMap.begin(); it != this->ProjectMap.end(); ++it) {
    cmLocalGenerator* root = it->second[0];
    this->SetGenerationRoot(root);
    // add ALL_BUILD, INSTALL, etc
    this->AddExtraTargets(root, it->second);
  }
}

void cmGlobalXCodeGenerator::Generate()
{
  this->cmGlobalGenerator::Generate();
  if (cmSystemTools::GetErrorOccuredFlag()) {
    return;
  }
  std::map<std::string, std::vector<cmLocalGenerator*> >::iterator it;
  for (it = this->ProjectMap.begin(); it != this->ProjectMap.end(); ++it) {
    cmLocalGenerator* root = it->second[0];
    this->SetGenerationRoot(root);
    // now create the project
    this->OutputXCodeProject(root, it->second);
  }
}

void cmGlobalXCodeGenerator::SetGenerationRoot(cmLocalGenerator* root)
{
  this->CurrentProject = root->GetProjectName();
  this->SetCurrentLocalGenerator(root);
  cmSystemTools::SplitPath(
    this->CurrentLocalGenerator->GetCurrentSourceDirectory(),
    this->ProjectSourceDirectoryComponents);
  cmSystemTools::SplitPath(
    this->CurrentLocalGenerator->GetCurrentBinaryDirectory(),
    this->ProjectOutputDirectoryComponents);

  this->CurrentXCodeHackMakefile = root->GetCurrentBinaryDirectory();
  this->CurrentXCodeHackMakefile += "/CMakeScripts";
  cmSystemTools::MakeDirectory(this->CurrentXCodeHackMakefile.c_str());
  this->CurrentXCodeHackMakefile += "/XCODE_DEPEND_HELPER.make";
}

std::string cmGlobalXCodeGenerator::PostBuildMakeTarget(
  std::string const& tName, std::string const& configName)
{
  std::string target = tName;
  std::replace(target.begin(), target.end(), ' ', '_');
  std::string out = "PostBuild." + target;
  out += "." + configName;
  return out;
}

#define CMAKE_CHECK_BUILD_SYSTEM_TARGET "ZERO_CHECK"

void cmGlobalXCodeGenerator::AddExtraTargets(
  cmLocalGenerator* root, std::vector<cmLocalGenerator*>& gens)
{
  cmMakefile* mf = root->GetMakefile();

  // Add ALL_BUILD
  const char* no_working_directory = 0;
  std::vector<std::string> no_depends;
  cmTarget* allbuild =
    mf->AddUtilityCommand("ALL_BUILD", true, no_depends, no_working_directory,
                          "echo", "Build all projects");

  cmGeneratorTarget* allBuildGt = new cmGeneratorTarget(allbuild, root);
  root->AddGeneratorTarget(allBuildGt);

  // Refer to the main build configuration file for easy editing.
  std::string listfile = root->GetCurrentSourceDirectory();
  listfile += "/";
  listfile += "CMakeLists.txt";
  allBuildGt->AddSource(listfile);

  // Add XCODE depend helper
  std::string dir = root->GetCurrentBinaryDirectory();
  cmCustomCommandLine makeHelper;
  makeHelper.push_back("make");
  makeHelper.push_back("-C");
  makeHelper.push_back(dir);
  makeHelper.push_back("-f");
  makeHelper.push_back(this->CurrentXCodeHackMakefile);
  makeHelper.push_back(""); // placeholder, see below

  // Add ZERO_CHECK
  bool regenerate = !mf->IsOn("CMAKE_SUPPRESS_REGENERATION");
  if (regenerate) {
    this->CreateReRunCMakeFile(root, gens);
    std::string file =
      this->ConvertToRelativeForMake(this->CurrentReRunCMakeMakefile.c_str());
    cmSystemTools::ReplaceString(file, "\\ ", " ");
    cmTarget* check =
      mf->AddUtilityCommand(CMAKE_CHECK_BUILD_SYSTEM_TARGET, true, no_depends,
                            no_working_directory, "make", "-f", file.c_str());

    cmGeneratorTarget* checkGt = new cmGeneratorTarget(check, root);
    root->AddGeneratorTarget(checkGt);
  }

  // now make the allbuild depend on all the non-utility targets
  // in the project
  for (std::vector<cmLocalGenerator*>::iterator i = gens.begin();
       i != gens.end(); ++i) {
    cmLocalGenerator* lg = *i;
    if (this->IsExcluded(root, *i)) {
      continue;
    }

    std::vector<cmGeneratorTarget*> tgts = lg->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
         l != tgts.end(); l++) {
      cmGeneratorTarget* target = *l;

      if (target->GetType() == cmStateEnums::GLOBAL_TARGET) {
        continue;
      }

      std::string targetName = target->GetName();

      if (regenerate && (targetName != CMAKE_CHECK_BUILD_SYSTEM_TARGET)) {
        target->Target->AddUtility(CMAKE_CHECK_BUILD_SYSTEM_TARGET);
      }

      // make all exe, shared libs and modules
      // run the depend check makefile as a post build rule
      // this will make sure that when the next target is built
      // things are up-to-date
      if (target->GetType() == cmStateEnums::OBJECT_LIBRARY ||
          (this->XcodeVersion < 50 &&
           (target->GetType() == cmStateEnums::EXECUTABLE ||
            target->GetType() == cmStateEnums::STATIC_LIBRARY ||
            target->GetType() == cmStateEnums::SHARED_LIBRARY ||
            target->GetType() == cmStateEnums::MODULE_LIBRARY))) {
        makeHelper[makeHelper.size() - 1] = // fill placeholder
          this->PostBuildMakeTarget(target->GetName(), "$(CONFIGURATION)");
        cmCustomCommandLines commandLines;
        commandLines.push_back(makeHelper);
        std::vector<std::string> no_byproducts;
        lg->GetMakefile()->AddCustomCommandToTarget(
          target->GetName(), no_byproducts, no_depends, commandLines,
          cmTarget::POST_BUILD, "Depend check for xcode", dir.c_str(), true,
          false, "", false, cmMakefile::AcceptObjectLibraryCommands);
      }

      if (target->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
          !target->GetPropertyAsBool("EXCLUDE_FROM_ALL")) {
        allbuild->AddUtility(target->GetName());
      }

      // Refer to the build configuration file for easy editing.
      listfile = lg->GetCurrentSourceDirectory();
      listfile += "/";
      listfile += "CMakeLists.txt";
      target->AddSource(listfile);
    }
  }
}

void cmGlobalXCodeGenerator::CreateReRunCMakeFile(
  cmLocalGenerator* root, std::vector<cmLocalGenerator*> const& gens)
{
  std::vector<std::string> lfiles;
  for (std::vector<cmLocalGenerator*>::const_iterator gi = gens.begin();
       gi != gens.end(); ++gi) {
    std::vector<std::string> const& lf = (*gi)->GetMakefile()->GetListFiles();
    lfiles.insert(lfiles.end(), lf.begin(), lf.end());
  }

  // sort the array
  std::sort(lfiles.begin(), lfiles.end(), std::less<std::string>());
  std::vector<std::string>::iterator new_end =
    std::unique(lfiles.begin(), lfiles.end());
  lfiles.erase(new_end, lfiles.end());
  this->CurrentReRunCMakeMakefile = root->GetCurrentBinaryDirectory();
  this->CurrentReRunCMakeMakefile += "/CMakeScripts";
  cmSystemTools::MakeDirectory(this->CurrentReRunCMakeMakefile.c_str());
  this->CurrentReRunCMakeMakefile += "/ReRunCMake.make";
  cmGeneratedFileStream makefileStream(
    this->CurrentReRunCMakeMakefile.c_str());
  makefileStream.SetCopyIfDifferent(true);
  makefileStream << "# Generated by CMake, DO NOT EDIT\n\n";

  makefileStream << "empty:= \n";
  makefileStream << "space:= $(empty) $(empty)\n";
  makefileStream << "spaceplus:= $(empty)\\ $(empty)\n\n";

  for (std::vector<std::string>::const_iterator i = lfiles.begin();
       i != lfiles.end(); ++i) {
    makefileStream << "TARGETS += $(subst $(space),$(spaceplus),$(wildcard "
                   << this->ConvertToRelativeForMake(i->c_str()) << "))\n";
  }

  std::string checkCache = root->GetBinaryDirectory();
  checkCache += "/";
  checkCache += cmake::GetCMakeFilesDirectoryPostSlash();
  checkCache += "cmake.check_cache";

  makefileStream << "\n"
                 << this->ConvertToRelativeForMake(checkCache.c_str())
                 << ": $(TARGETS)\n";
  makefileStream << "\t"
                 << this->ConvertToRelativeForMake(
                      cmSystemTools::GetCMakeCommand().c_str())
                 << " -H"
                 << this->ConvertToRelativeForMake(root->GetSourceDirectory())
                 << " -B"
                 << this->ConvertToRelativeForMake(root->GetBinaryDirectory())
                 << "\n";
}

static bool objectIdLessThan(cmXCodeObject* l, cmXCodeObject* r)
{
  return l->GetId() < r->GetId();
}

void cmGlobalXCodeGenerator::SortXCodeObjects()
{
  std::sort(this->XCodeObjects.begin(), this->XCodeObjects.end(),
            objectIdLessThan);
}

void cmGlobalXCodeGenerator::ClearXCodeObjects()
{
  this->TargetDoneSet.clear();
  for (unsigned int i = 0; i < this->XCodeObjects.size(); ++i) {
    delete this->XCodeObjects[i];
  }
  this->XCodeObjects.clear();
  this->XCodeObjectIDs.clear();
  this->XCodeObjectMap.clear();
  this->GroupMap.clear();
  this->GroupNameMap.clear();
  this->TargetGroup.clear();
  this->FileRefs.clear();
}

void cmGlobalXCodeGenerator::addObject(cmXCodeObject* obj)
{
  if (obj->GetType() == cmXCodeObject::OBJECT) {
    std::string id = obj->GetId();

    // If this is a duplicate id, it's an error:
    //
    if (this->XCodeObjectIDs.count(id)) {
      cmSystemTools::Error(
        "Xcode generator: duplicate object ids not allowed");
    }

    this->XCodeObjectIDs.insert(id);
  }

  this->XCodeObjects.push_back(obj);
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateObject(
  cmXCodeObject::PBXType ptype)
{
  cmXCodeObject* obj = new cmXCode21Object(ptype, cmXCodeObject::OBJECT);
  this->addObject(obj);
  return obj;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateObject(cmXCodeObject::Type type)
{
  cmXCodeObject* obj = new cmXCodeObject(cmXCodeObject::None, type);
  this->addObject(obj);
  return obj;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateString(const std::string& s)
{
  cmXCodeObject* obj = this->CreateObject(cmXCodeObject::STRING);
  obj->SetString(s);
  return obj;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateObjectReference(
  cmXCodeObject* ref)
{
  cmXCodeObject* obj = this->CreateObject(cmXCodeObject::OBJECT_REF);
  obj->SetObject(ref);
  return obj;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateFlatClone(cmXCodeObject* orig)
{
  cmXCodeObject* obj = this->CreateObject(orig->GetType());
  obj->CopyAttributes(orig);
  return obj;
}

std::string GetGroupMapKeyFromPath(cmGeneratorTarget* target,
                                   const std::string& fullpath)
{
  std::string key(target->GetName());
  key += "-";
  key += fullpath;
  return key;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateXCodeSourceFileFromPath(
  const std::string& fullpath, cmGeneratorTarget* target,
  const std::string& lang, cmSourceFile* sf)
{
  // Using a map and the full path guarantees that we will always get the same
  // fileRef object for any given full path.
  //
  cmXCodeObject* fileRef =
    this->CreateXCodeFileReferenceFromPath(fullpath, target, lang, sf);

  cmXCodeObject* buildFile = this->CreateObject(cmXCodeObject::PBXBuildFile);
  buildFile->SetComment(fileRef->GetComment());
  buildFile->AddAttribute("fileRef", this->CreateObjectReference(fileRef));

  return buildFile;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateXCodeSourceFile(
  cmLocalGenerator* lg, cmSourceFile* sf, cmGeneratorTarget* gtgt)
{
  // Add flags from target and source file properties.
  std::string flags;
  const char* srcfmt = sf->GetProperty("Fortran_FORMAT");
  switch (cmOutputConverter::GetFortranFormat(srcfmt)) {
    case cmOutputConverter::FortranFormatFixed:
      flags = "-fixed " + flags;
      break;
    case cmOutputConverter::FortranFormatFree:
      flags = "-free " + flags;
      break;
    default:
      break;
  }
  if (const char* cflags = sf->GetProperty("COMPILE_FLAGS")) {
    cmGeneratorExpression ge;
    std::string configName = "NO-PER-CONFIG-SUPPORT-IN-XCODE";
    CM_AUTO_PTR<cmCompiledGeneratorExpression> compiledExpr = ge.Parse(cflags);
    const char* processed = compiledExpr->Evaluate(lg, configName);
    if (compiledExpr->GetHadContextSensitiveCondition()) {
      std::ostringstream e;
      /* clang-format off */
      e <<
        "Xcode does not support per-config per-source COMPILE_FLAGS:\n"
        "  " << cflags << "\n"
        "specified for source:\n"
        "  " << sf->GetFullPath() << "\n";
      /* clang-format on */
      lg->IssueMessage(cmake::FATAL_ERROR, e.str());
    }
    lg->AppendFlags(flags, processed);
  }

  // Add per-source definitions.
  BuildObjectListOrString flagsBuild(this, false);
  this->AppendDefines(flagsBuild, sf->GetProperty("COMPILE_DEFINITIONS"),
                      true);
  if (!flagsBuild.IsEmpty()) {
    if (!flags.empty()) {
      flags += ' ';
    }
    flags += flagsBuild.GetString();
  }

  std::string lang = this->CurrentLocalGenerator->GetSourceFileLanguage(*sf);

  cmXCodeObject* buildFile =
    this->CreateXCodeSourceFileFromPath(sf->GetFullPath(), gtgt, lang, sf);

  cmXCodeObject* settings = this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
  settings->AddAttributeIfNotEmpty("COMPILER_FLAGS",
                                   this->CreateString(flags));

  cmGeneratorTarget::SourceFileFlags tsFlags =
    gtgt->GetTargetSourceFileFlags(sf);

  cmXCodeObject* attrs = this->CreateObject(cmXCodeObject::OBJECT_LIST);

  // Is this a "private" or "public" framework header file?
  // Set the ATTRIBUTES attribute appropriately...
  //
  if (gtgt->IsFrameworkOnApple()) {
    if (tsFlags.Type == cmGeneratorTarget::SourceFileTypePrivateHeader) {
      attrs->AddObject(this->CreateString("Private"));
    } else if (tsFlags.Type == cmGeneratorTarget::SourceFileTypePublicHeader) {
      attrs->AddObject(this->CreateString("Public"));
    }
  }

  // Add user-specified file attributes.
  const char* extraFileAttributes = sf->GetProperty("XCODE_FILE_ATTRIBUTES");
  if (extraFileAttributes) {
    // Expand the list of attributes.
    std::vector<std::string> attributes;
    cmSystemTools::ExpandListArgument(extraFileAttributes, attributes);

    // Store the attributes.
    for (std::vector<std::string>::const_iterator ai = attributes.begin();
         ai != attributes.end(); ++ai) {
      attrs->AddObject(this->CreateString(*ai));
    }
  }

  settings->AddAttributeIfNotEmpty("ATTRIBUTES", attrs);

  buildFile->AddAttributeIfNotEmpty("settings", settings);
  return buildFile;
}

std::string GetSourcecodeValueFromFileExtension(const std::string& _ext,
                                                const std::string& lang,
                                                bool& keepLastKnownFileType)
{
  std::string ext = cmSystemTools::LowerCase(_ext);
  std::string sourcecode = "sourcecode";

  if (ext == "o") {
    sourcecode = "compiled.mach-o.objfile";
  } else if (ext == "xctest") {
    sourcecode = "wrapper.cfbundle";
  } else if (ext == "xib") {
    keepLastKnownFileType = true;
    sourcecode = "file.xib";
  } else if (ext == "storyboard") {
    keepLastKnownFileType = true;
    sourcecode = "file.storyboard";
  } else if (ext == "mm") {
    sourcecode += ".cpp.objcpp";
  } else if (ext == "m") {
    sourcecode += ".c.objc";
  } else if (ext == "swift") {
    sourcecode += ".swift";
  } else if (ext == "plist") {
    sourcecode += ".text.plist";
  } else if (ext == "h") {
    sourcecode += ".c.h";
  } else if (ext == "hxx" || ext == "hpp" || ext == "txx" || ext == "pch" ||
             ext == "hh") {
    sourcecode += ".cpp.h";
  } else if (ext == "png" || ext == "gif" || ext == "jpg") {
    keepLastKnownFileType = true;
    sourcecode = "image";
  } else if (ext == "txt") {
    sourcecode += ".text";
  } else if (lang == "CXX") {
    sourcecode += ".cpp.cpp";
  } else if (lang == "C") {
    sourcecode += ".c.c";
  } else if (lang == "Fortran") {
    sourcecode += ".fortran.f90";
  } else if (lang == "ASM") {
    sourcecode += ".asm";
  } else if (ext == "metal") {
    sourcecode += ".metal";
  } else if (ext == "mig") {
    sourcecode += ".mig";
  }
  // else
  //  {
  //  // Already specialized above or we leave sourcecode == "sourcecode"
  //  // which is probably the most correct choice. Extensionless headers,
  //  // for example... Or file types unknown to Xcode that do not map to a
  //  // valid explicitFileType value.
  //  }

  return sourcecode;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateXCodeFileReferenceFromPath(
  const std::string& fullpath, cmGeneratorTarget* target,
  const std::string& lang, cmSourceFile* sf)
{
  std::string key = GetGroupMapKeyFromPath(target, fullpath);
  cmXCodeObject* fileRef = this->FileRefs[key];
  if (!fileRef) {
    fileRef = this->CreateObject(cmXCodeObject::PBXFileReference);
    fileRef->SetComment(fullpath);
    this->FileRefs[key] = fileRef;
  }
  cmXCodeObject* group = this->GroupMap[key];
  cmXCodeObject* children = group->GetObject("children");
  if (!children->HasObject(fileRef)) {
    children->AddObject(fileRef);
  }
  fileRef->AddAttribute("fileEncoding", this->CreateString("4"));

  bool useLastKnownFileType = false;
  std::string fileType;
  if (sf) {
    if (const char* e = sf->GetProperty("XCODE_EXPLICIT_FILE_TYPE")) {
      fileType = e;
    } else if (const char* l = sf->GetProperty("XCODE_LAST_KNOWN_FILE_TYPE")) {
      useLastKnownFileType = true;
      fileType = l;
    }
  }
  if (fileType.empty()) {
    // Compute the extension without leading '.'.
    std::string ext = cmSystemTools::GetFilenameLastExtension(fullpath);
    if (!ext.empty()) {
      ext = ext.substr(1);
    }

    // If fullpath references a directory, then we need to specify
    // lastKnownFileType as folder in order for Xcode to be able to
    // open the contents of the folder.
    // (Xcode 4.6 does not like explicitFileType=folder).
    if (cmSystemTools::FileIsDirectory(fullpath)) {
      fileType = (ext == "xcassets" ? "folder.assetcatalog" : "folder");
      useLastKnownFileType = true;
    } else {
      fileType =
        GetSourcecodeValueFromFileExtension(ext, lang, useLastKnownFileType);
    }
  }

  fileRef->AddAttribute(useLastKnownFileType ? "lastKnownFileType"
                                             : "explicitFileType",
                        this->CreateString(fileType));

  // Store the file path relative to the top of the source tree.
  std::string path = this->RelativeToSource(fullpath.c_str());
  std::string name = cmSystemTools::GetFilenameName(path);
  const char* sourceTree =
    (cmSystemTools::FileIsFullPath(path.c_str()) ? "<absolute>"
                                                 : "SOURCE_ROOT");
  fileRef->AddAttribute("name", this->CreateString(name));
  fileRef->AddAttribute("path", this->CreateString(path));
  fileRef->AddAttribute("sourceTree", this->CreateString(sourceTree));
  return fileRef;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateXCodeFileReference(
  cmSourceFile* sf, cmGeneratorTarget* target)
{
  std::string lang = this->CurrentLocalGenerator->GetSourceFileLanguage(*sf);

  return this->CreateXCodeFileReferenceFromPath(sf->GetFullPath(), target,
                                                lang, sf);
}

bool cmGlobalXCodeGenerator::SpecialTargetEmitted(std::string const& tname)
{
  if (tname == "ALL_BUILD" || tname == "XCODE_DEPEND_HELPER" ||
      tname == "install" || tname == "package" || tname == "RUN_TESTS" ||
      tname == CMAKE_CHECK_BUILD_SYSTEM_TARGET) {
    if (this->TargetDoneSet.find(tname) != this->TargetDoneSet.end()) {
      return true;
    }
    this->TargetDoneSet.insert(tname);
    return false;
  }
  return false;
}

void cmGlobalXCodeGenerator::SetCurrentLocalGenerator(cmLocalGenerator* gen)
{
  this->CurrentLocalGenerator = gen;
  this->CurrentMakefile = gen->GetMakefile();

  // Select the current set of configuration types.
  this->CurrentConfigurationTypes.clear();
  this->CurrentMakefile->GetConfigurations(this->CurrentConfigurationTypes);
  if (this->CurrentConfigurationTypes.empty()) {
    this->CurrentConfigurationTypes.push_back("");
  }
}

struct cmSourceFilePathCompare
{
  bool operator()(cmSourceFile* l, cmSourceFile* r)
  {
    return l->GetFullPath() < r->GetFullPath();
  }
};

struct cmCompareTargets
{
  bool operator()(std::string const& a, std::string const& b) const
  {
    if (a == "ALL_BUILD") {
      return true;
    }
    if (b == "ALL_BUILD") {
      return false;
    }
    return strcmp(a.c_str(), b.c_str()) < 0;
  }
};

bool cmGlobalXCodeGenerator::CreateXCodeTargets(
  cmLocalGenerator* gen, std::vector<cmXCodeObject*>& targets)
{
  this->SetCurrentLocalGenerator(gen);
  std::vector<cmGeneratorTarget*> tgts =
    this->CurrentLocalGenerator->GetGeneratorTargets();
  typedef std::map<std::string, cmGeneratorTarget*, cmCompareTargets>
    cmSortedTargets;
  cmSortedTargets sortedTargets;
  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); l++) {
    sortedTargets[(*l)->GetName()] = *l;
  }
  for (cmSortedTargets::iterator l = sortedTargets.begin();
       l != sortedTargets.end(); l++) {
    cmGeneratorTarget* gtgt = l->second;

    std::string targetName = gtgt->GetName();

    // make sure ALL_BUILD, INSTALL, etc are only done once
    if (this->SpecialTargetEmitted(targetName)) {
      continue;
    }

    if (gtgt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }

    if (gtgt->GetType() == cmStateEnums::UTILITY ||
        gtgt->GetType() == cmStateEnums::GLOBAL_TARGET) {
      cmXCodeObject* t = this->CreateUtilityTarget(gtgt);
      if (!t) {
        return false;
      }
      targets.push_back(t);
      continue;
    }

    // organize the sources
    std::vector<cmSourceFile*> classes;
    if (!gtgt->GetConfigCommonSourceFiles(classes)) {
      return false;
    }
    std::sort(classes.begin(), classes.end(), cmSourceFilePathCompare());

    gtgt->ComputeObjectMapping();

    std::vector<cmXCodeObject*> externalObjFiles;
    std::vector<cmXCodeObject*> headerFiles;
    std::vector<cmXCodeObject*> resourceFiles;
    std::vector<cmXCodeObject*> sourceFiles;
    for (std::vector<cmSourceFile*>::const_iterator i = classes.begin();
         i != classes.end(); ++i) {
      cmXCodeObject* xsf =
        this->CreateXCodeSourceFile(this->CurrentLocalGenerator, *i, gtgt);
      cmXCodeObject* fr = xsf->GetObject("fileRef");
      cmXCodeObject* filetype = fr->GetObject()->GetObject("explicitFileType");

      cmGeneratorTarget::SourceFileFlags tsFlags =
        gtgt->GetTargetSourceFileFlags(*i);

      if (filetype && filetype->GetString() == "compiled.mach-o.objfile") {
        if ((*i)->GetObjectLibrary().empty()) {
          externalObjFiles.push_back(xsf);
        }
      } else if (this->IsHeaderFile(*i) ||
                 (tsFlags.Type ==
                  cmGeneratorTarget::SourceFileTypePrivateHeader) ||
                 (tsFlags.Type ==
                  cmGeneratorTarget::SourceFileTypePublicHeader)) {
        headerFiles.push_back(xsf);
      } else if (tsFlags.Type == cmGeneratorTarget::SourceFileTypeResource) {
        resourceFiles.push_back(xsf);
      } else if (!(*i)->GetPropertyAsBool("HEADER_FILE_ONLY")) {
        // Include this file in the build if it has a known language
        // and has not been listed as an ignored extension for this
        // generator.
        if (!this->CurrentLocalGenerator->GetSourceFileLanguage(**i).empty() &&
            !this->IgnoreFile((*i)->GetExtension().c_str())) {
          sourceFiles.push_back(xsf);
        }
      }
    }

    if (this->XcodeVersion < 50) {
      // Add object library contents as external objects. (Equivalent to
      // the externalObjFiles above, except each one is not a cmSourceFile
      // within the target.)
      std::vector<cmSourceFile const*> objs;
      gtgt->GetExternalObjects(objs, "");
      for (std::vector<cmSourceFile const*>::const_iterator oi = objs.begin();
           oi != objs.end(); ++oi) {
        if ((*oi)->GetObjectLibrary().empty()) {
          continue;
        }
        std::string const& obj = (*oi)->GetFullPath();
        cmXCodeObject* xsf =
          this->CreateXCodeSourceFileFromPath(obj, gtgt, "", 0);
        externalObjFiles.push_back(xsf);
      }
    }

    // some build phases only apply to bundles and/or frameworks
    bool isFrameworkTarget = gtgt->IsFrameworkOnApple();
    bool isBundleTarget = gtgt->GetPropertyAsBool("MACOSX_BUNDLE");
    bool isCFBundleTarget = gtgt->IsCFBundleOnApple();

    cmXCodeObject* buildFiles = 0;

    // create source build phase
    cmXCodeObject* sourceBuildPhase = 0;
    if (!sourceFiles.empty()) {
      sourceBuildPhase =
        this->CreateObject(cmXCodeObject::PBXSourcesBuildPhase);
      sourceBuildPhase->SetComment("Sources");
      sourceBuildPhase->AddAttribute("buildActionMask",
                                     this->CreateString("2147483647"));
      buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
      for (std::vector<cmXCodeObject*>::iterator i = sourceFiles.begin();
           i != sourceFiles.end(); ++i) {
        buildFiles->AddObject(*i);
      }
      sourceBuildPhase->AddAttribute("files", buildFiles);
      sourceBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                     this->CreateString("0"));
    }

    // create header build phase - only for framework targets
    cmXCodeObject* headerBuildPhase = 0;
    if (!headerFiles.empty() && isFrameworkTarget) {
      headerBuildPhase =
        this->CreateObject(cmXCodeObject::PBXHeadersBuildPhase);
      headerBuildPhase->SetComment("Headers");
      headerBuildPhase->AddAttribute("buildActionMask",
                                     this->CreateString("2147483647"));
      buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
      for (std::vector<cmXCodeObject*>::iterator i = headerFiles.begin();
           i != headerFiles.end(); ++i) {
        buildFiles->AddObject(*i);
      }
      headerBuildPhase->AddAttribute("files", buildFiles);
      headerBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                     this->CreateString("0"));
    }

    // create resource build phase - only for framework or bundle targets
    cmXCodeObject* resourceBuildPhase = 0;
    if (!resourceFiles.empty() &&
        (isFrameworkTarget || isBundleTarget || isCFBundleTarget)) {
      resourceBuildPhase =
        this->CreateObject(cmXCodeObject::PBXResourcesBuildPhase);
      resourceBuildPhase->SetComment("Resources");
      resourceBuildPhase->AddAttribute("buildActionMask",
                                       this->CreateString("2147483647"));
      buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
      for (std::vector<cmXCodeObject*>::iterator i = resourceFiles.begin();
           i != resourceFiles.end(); ++i) {
        buildFiles->AddObject(*i);
      }
      resourceBuildPhase->AddAttribute("files", buildFiles);
      resourceBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                       this->CreateString("0"));
    }

    // create vector of "non-resource content file" build phases - only for
    // framework or bundle targets
    std::vector<cmXCodeObject*> contentBuildPhases;
    if (isFrameworkTarget || isBundleTarget || isCFBundleTarget) {
      typedef std::map<std::string, std::vector<cmSourceFile*> >
        mapOfVectorOfSourceFiles;
      mapOfVectorOfSourceFiles bundleFiles;
      for (std::vector<cmSourceFile*>::const_iterator i = classes.begin();
           i != classes.end(); ++i) {
        cmGeneratorTarget::SourceFileFlags tsFlags =
          gtgt->GetTargetSourceFileFlags(*i);
        if (tsFlags.Type == cmGeneratorTarget::SourceFileTypeMacContent) {
          bundleFiles[tsFlags.MacFolder].push_back(*i);
        }
      }
      mapOfVectorOfSourceFiles::iterator mit;
      for (mit = bundleFiles.begin(); mit != bundleFiles.end(); ++mit) {
        cmXCodeObject* copyFilesBuildPhase =
          this->CreateObject(cmXCodeObject::PBXCopyFilesBuildPhase);
        copyFilesBuildPhase->SetComment("Copy files");
        copyFilesBuildPhase->AddAttribute("buildActionMask",
                                          this->CreateString("2147483647"));
        copyFilesBuildPhase->AddAttribute("dstSubfolderSpec",
                                          this->CreateString("6"));
        std::ostringstream ostr;
        if (gtgt->IsFrameworkOnApple()) {
          // dstPath in frameworks is relative to Versions/<version>
          ostr << mit->first;
        } else if (mit->first != "MacOS") {
          if (gtgt->Target->GetMakefile()->PlatformIsAppleIos()) {
            ostr << mit->first;
          } else {
            // dstPath in bundles is relative to Contents/MacOS
            ostr << "../" << mit->first;
          }
        }
        copyFilesBuildPhase->AddAttribute("dstPath",
                                          this->CreateString(ostr.str()));
        copyFilesBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                          this->CreateString("0"));
        buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
        copyFilesBuildPhase->AddAttribute("files", buildFiles);
        std::vector<cmSourceFile*>::iterator sfIt;
        for (sfIt = mit->second.begin(); sfIt != mit->second.end(); ++sfIt) {
          cmXCodeObject* xsf = this->CreateXCodeSourceFile(
            this->CurrentLocalGenerator, *sfIt, gtgt);
          buildFiles->AddObject(xsf);
        }
        contentBuildPhases.push_back(copyFilesBuildPhase);
      }
    }

    // create vector of "resource content file" build phases - only for
    // framework or bundle targets
    if (isFrameworkTarget || isBundleTarget || isCFBundleTarget) {
      typedef std::map<std::string, std::vector<cmSourceFile*> >
        mapOfVectorOfSourceFiles;
      mapOfVectorOfSourceFiles bundleFiles;
      for (std::vector<cmSourceFile*>::const_iterator i = classes.begin();
           i != classes.end(); ++i) {
        cmGeneratorTarget::SourceFileFlags tsFlags =
          gtgt->GetTargetSourceFileFlags(*i);
        if (tsFlags.Type == cmGeneratorTarget::SourceFileTypeDeepResource) {
          bundleFiles[tsFlags.MacFolder].push_back(*i);
        }
      }
      mapOfVectorOfSourceFiles::iterator mit;
      for (mit = bundleFiles.begin(); mit != bundleFiles.end(); ++mit) {
        cmXCodeObject* copyFilesBuildPhase =
          this->CreateObject(cmXCodeObject::PBXCopyFilesBuildPhase);
        copyFilesBuildPhase->SetComment("Copy files");
        copyFilesBuildPhase->AddAttribute("buildActionMask",
                                          this->CreateString("2147483647"));
        copyFilesBuildPhase->AddAttribute("dstSubfolderSpec",
                                          this->CreateString("7"));
        copyFilesBuildPhase->AddAttribute("dstPath",
                                          this->CreateString(mit->first));
        copyFilesBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                          this->CreateString("0"));
        buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
        copyFilesBuildPhase->AddAttribute("files", buildFiles);
        std::vector<cmSourceFile*>::iterator sfIt;
        for (sfIt = mit->second.begin(); sfIt != mit->second.end(); ++sfIt) {
          cmXCodeObject* xsf = this->CreateXCodeSourceFile(
            this->CurrentLocalGenerator, *sfIt, gtgt);
          buildFiles->AddObject(xsf);
        }
        contentBuildPhases.push_back(copyFilesBuildPhase);
      }
    }

    // create framework build phase
    cmXCodeObject* frameworkBuildPhase = 0;
    if (!externalObjFiles.empty()) {
      frameworkBuildPhase =
        this->CreateObject(cmXCodeObject::PBXFrameworksBuildPhase);
      frameworkBuildPhase->SetComment("Frameworks");
      frameworkBuildPhase->AddAttribute("buildActionMask",
                                        this->CreateString("2147483647"));
      buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
      frameworkBuildPhase->AddAttribute("files", buildFiles);
      for (std::vector<cmXCodeObject*>::iterator i = externalObjFiles.begin();
           i != externalObjFiles.end(); ++i) {
        buildFiles->AddObject(*i);
      }
      frameworkBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                        this->CreateString("0"));
    }

    // create list of build phases and create the Xcode target
    cmXCodeObject* buildPhases =
      this->CreateObject(cmXCodeObject::OBJECT_LIST);

    this->CreateCustomCommands(buildPhases, sourceBuildPhase, headerBuildPhase,
                               resourceBuildPhase, contentBuildPhases,
                               frameworkBuildPhase, gtgt);

    targets.push_back(this->CreateXCodeTarget(gtgt, buildPhases));
  }
  return true;
}

void cmGlobalXCodeGenerator::ForceLinkerLanguages()
{
  for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
    std::vector<cmGeneratorTarget*> tgts =
      this->LocalGenerators[i]->GetGeneratorTargets();
    // All targets depend on the build-system check target.
    for (std::vector<cmGeneratorTarget*>::const_iterator ti = tgts.begin();
         ti != tgts.end(); ++ti) {
      // This makes sure all targets link using the proper language.
      this->ForceLinkerLanguage(*ti);
    }
  }
}

void cmGlobalXCodeGenerator::ForceLinkerLanguage(cmGeneratorTarget* gtgt)
{
  // This matters only for targets that link.
  if (gtgt->GetType() != cmStateEnums::EXECUTABLE &&
      gtgt->GetType() != cmStateEnums::SHARED_LIBRARY &&
      gtgt->GetType() != cmStateEnums::MODULE_LIBRARY) {
    return;
  }

  std::string llang = gtgt->GetLinkerLanguage("NOCONFIG");
  if (llang.empty()) {
    return;
  }

  // If the language is compiled as a source trust Xcode to link with it.
  cmLinkImplementation const* impl = gtgt->GetLinkImplementation("NOCONFIG");
  for (std::vector<std::string>::const_iterator li = impl->Languages.begin();
       li != impl->Languages.end(); ++li) {
    if (*li == llang) {
      return;
    }
  }

  // Add an empty source file to the target that compiles with the
  // linker language.  This should convince Xcode to choose the proper
  // language.
  cmMakefile* mf = gtgt->Target->GetMakefile();
  std::string fname = gtgt->GetLocalGenerator()->GetCurrentBinaryDirectory();
  fname += cmake::GetCMakeFilesDirectory();
  fname += "/";
  fname += gtgt->GetName();
  fname += "-CMakeForceLinker";
  fname += ".";
  fname += cmSystemTools::LowerCase(llang);
  {
    cmGeneratedFileStream fout(fname.c_str());
    fout << "\n";
  }
  if (cmSourceFile* sf = mf->GetOrCreateSource(fname)) {
    sf->SetProperty("LANGUAGE", llang.c_str());
    gtgt->AddSource(fname);
  }
}

bool cmGlobalXCodeGenerator::IsHeaderFile(cmSourceFile* sf)
{
  const std::vector<std::string>& hdrExts =
    this->CMakeInstance->GetHeaderExtensions();
  return (std::find(hdrExts.begin(), hdrExts.end(), sf->GetExtension()) !=
          hdrExts.end());
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateBuildPhase(
  const char* name, const char* name2, cmGeneratorTarget* target,
  const std::vector<cmCustomCommand>& commands)
{
  if (commands.size() == 0 && strcmp(name, "CMake ReRun") != 0) {
    return 0;
  }
  cmXCodeObject* buildPhase =
    this->CreateObject(cmXCodeObject::PBXShellScriptBuildPhase);
  buildPhase->AddAttribute("buildActionMask",
                           this->CreateString("2147483647"));
  cmXCodeObject* buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  buildPhase->AddAttribute("files", buildFiles);
  buildPhase->AddAttribute("name", this->CreateString(name));
  buildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                           this->CreateString("0"));
  buildPhase->AddAttribute("shellPath", this->CreateString("/bin/sh"));
  this->AddCommandsToBuildPhase(buildPhase, target, commands, name2);
  return buildPhase;
}

void cmGlobalXCodeGenerator::CreateCustomCommands(
  cmXCodeObject* buildPhases, cmXCodeObject* sourceBuildPhase,
  cmXCodeObject* headerBuildPhase, cmXCodeObject* resourceBuildPhase,
  std::vector<cmXCodeObject*> contentBuildPhases,
  cmXCodeObject* frameworkBuildPhase, cmGeneratorTarget* gtgt)
{
  std::vector<cmCustomCommand> const& prebuild = gtgt->GetPreBuildCommands();
  std::vector<cmCustomCommand> const& prelink = gtgt->GetPreLinkCommands();
  std::vector<cmCustomCommand> postbuild = gtgt->GetPostBuildCommands();

  if (gtgt->GetType() == cmStateEnums::SHARED_LIBRARY &&
      !gtgt->IsFrameworkOnApple()) {
    cmCustomCommandLines cmd;
    cmd.resize(1);
    cmd[0].push_back(cmSystemTools::GetCMakeCommand());
    cmd[0].push_back("-E");
    cmd[0].push_back("cmake_symlink_library");
    std::string str_file = "$<TARGET_FILE:";
    str_file += gtgt->GetName();
    str_file += ">";
    std::string str_so_file = "$<TARGET_SONAME_FILE:";
    str_so_file += gtgt->GetName();
    str_so_file += ">";
    std::string str_link_file = "$<TARGET_LINKER_FILE:";
    str_link_file += gtgt->GetName();
    str_link_file += ">";
    cmd[0].push_back(str_file);
    cmd[0].push_back(str_so_file);
    cmd[0].push_back(str_link_file);

    cmCustomCommand command(this->CurrentMakefile, std::vector<std::string>(),
                            std::vector<std::string>(),
                            std::vector<std::string>(), cmd,
                            "Creating symlinks", "");

    postbuild.push_back(command);
  }

  std::vector<cmSourceFile*> classes;
  if (!gtgt->GetConfigCommonSourceFiles(classes)) {
    return;
  }
  // add all the sources
  std::vector<cmCustomCommand> commands;
  for (std::vector<cmSourceFile*>::const_iterator i = classes.begin();
       i != classes.end(); ++i) {
    if ((*i)->GetCustomCommand()) {
      commands.push_back(*(*i)->GetCustomCommand());
    }
  }
  // create prebuild phase
  cmXCodeObject* cmakeRulesBuildPhase = this->CreateBuildPhase(
    "CMake Rules", "cmakeRulesBuildPhase", gtgt, commands);
  // create prebuild phase
  cmXCodeObject* preBuildPhase = this->CreateBuildPhase(
    "CMake PreBuild Rules", "preBuildCommands", gtgt, prebuild);
  // create prelink phase
  cmXCodeObject* preLinkPhase = this->CreateBuildPhase(
    "CMake PreLink Rules", "preLinkCommands", gtgt, prelink);
  // create postbuild phase
  cmXCodeObject* postBuildPhase = this->CreateBuildPhase(
    "CMake PostBuild Rules", "postBuildPhase", gtgt, postbuild);

  // The order here is the order they will be built in.
  // The order "headers, resources, sources" mimics a native project generated
  // from an xcode template...
  //
  if (preBuildPhase) {
    buildPhases->AddObject(preBuildPhase);
  }
  if (cmakeRulesBuildPhase) {
    buildPhases->AddObject(cmakeRulesBuildPhase);
  }
  if (headerBuildPhase) {
    buildPhases->AddObject(headerBuildPhase);
  }
  if (resourceBuildPhase) {
    buildPhases->AddObject(resourceBuildPhase);
  }
  std::vector<cmXCodeObject*>::iterator cit;
  for (cit = contentBuildPhases.begin(); cit != contentBuildPhases.end();
       ++cit) {
    buildPhases->AddObject(*cit);
  }
  if (sourceBuildPhase) {
    buildPhases->AddObject(sourceBuildPhase);
  }
  if (preLinkPhase) {
    buildPhases->AddObject(preLinkPhase);
  }
  if (frameworkBuildPhase) {
    buildPhases->AddObject(frameworkBuildPhase);
  }
  if (postBuildPhase) {
    buildPhases->AddObject(postBuildPhase);
  }
}

// This function removes each occurrence of the flag and returns the last one
// (i.e., the dominant flag in GCC)
std::string cmGlobalXCodeGenerator::ExtractFlag(const char* flag,
                                                std::string& flags)
{
  std::string retFlag;
  std::string::size_type lastOccurancePos = flags.rfind(flag);
  bool saved = false;
  while (lastOccurancePos != std::string::npos) {
    // increment pos, we use lastOccurancePos to reduce search space on next
    // inc
    std::string::size_type pos = lastOccurancePos;
    if (pos == 0 || flags[pos - 1] == ' ') {
      while (pos < flags.size() && flags[pos] != ' ') {
        if (!saved) {
          retFlag += flags[pos];
        }
        flags[pos] = ' ';
        pos++;
      }
      saved = true;
    }
    // decrement lastOccurancePos while making sure we don't loop around
    // and become a very large positive number since size_type is unsigned
    lastOccurancePos = lastOccurancePos == 0 ? 0 : lastOccurancePos - 1;
    lastOccurancePos = flags.rfind(flag, lastOccurancePos);
  }
  return retFlag;
}

// This function removes each matching occurrence of the expression and
// returns the last one (i.e., the dominant flag in GCC)
std::string cmGlobalXCodeGenerator::ExtractFlagRegex(const char* exp,
                                                     int matchIndex,
                                                     std::string& flags)
{
  std::string retFlag;

  cmsys::RegularExpression regex(exp);
  assert(regex.is_valid());
  if (!regex.is_valid()) {
    return retFlag;
  }

  std::string::size_type offset = 0;

  while (regex.find(flags.c_str() + offset)) {
    const std::string::size_type startPos = offset + regex.start(matchIndex);
    const std::string::size_type endPos = offset + regex.end(matchIndex);
    const std::string::size_type size = endPos - startPos;

    offset = startPos + 1;

    retFlag.assign(flags, startPos, size);
    flags.replace(startPos, size, size, ' ');
  }

  return retFlag;
}

//----------------------------------------------------------------------------
// This function strips off Xcode attributes that do not target the current
// configuration
void cmGlobalXCodeGenerator::FilterConfigurationAttribute(
  std::string const& configName, std::string& attribute)
{
  // Handle [variant=<config>] condition explicitly here.
  std::string::size_type beginVariant = attribute.find("[variant=");
  if (beginVariant == std::string::npos) {
    // There is no variant in this attribute.
    return;
  }

  std::string::size_type endVariant = attribute.find("]", beginVariant + 9);
  if (endVariant == std::string::npos) {
    // There is no terminating bracket.
    return;
  }

  // Compare the variant to the configuration.
  std::string variant =
    attribute.substr(beginVariant + 9, endVariant - beginVariant - 9);
  if (variant == configName) {
    // The variant matches the configuration so use this
    // attribute but drop the [variant=<config>] condition.
    attribute.erase(beginVariant, endVariant - beginVariant + 1);
  } else {
    // The variant does not match the configuration so
    // do not use this attribute.
    attribute.clear();
  }
}

void cmGlobalXCodeGenerator::AddCommandsToBuildPhase(
  cmXCodeObject* buildphase, cmGeneratorTarget* target,
  std::vector<cmCustomCommand> const& commands, const char* name)
{
  std::string dir = this->CurrentLocalGenerator->GetCurrentBinaryDirectory();
  dir += "/CMakeScripts";
  cmSystemTools::MakeDirectory(dir.c_str());
  std::string makefile = dir;
  makefile += "/";
  makefile += target->GetName();
  makefile += "_";
  makefile += name;
  makefile += ".make";

  for (std::vector<std::string>::const_iterator currentConfig =
         this->CurrentConfigurationTypes.begin();
       currentConfig != this->CurrentConfigurationTypes.end();
       currentConfig++) {
    this->CreateCustomRulesMakefile(makefile.c_str(), target, commands,
                                    *currentConfig);
  }

  std::string cdir = this->CurrentLocalGenerator->GetCurrentBinaryDirectory();
  cdir = this->ConvertToRelativeForMake(cdir.c_str());
  std::string makecmd = "make -C ";
  makecmd += cdir;
  makecmd += " -f ";
  makecmd +=
    this->ConvertToRelativeForMake((makefile + "$CONFIGURATION").c_str());
  makecmd += " all";
  buildphase->AddAttribute("shellScript", this->CreateString(makecmd));
  buildphase->AddAttribute("showEnvVarsInLog", this->CreateString("0"));
}

void cmGlobalXCodeGenerator::CreateCustomRulesMakefile(
  const char* makefileBasename, cmGeneratorTarget* target,
  std::vector<cmCustomCommand> const& commands, const std::string& configName)
{
  std::string makefileName = makefileBasename;
  makefileName += configName;
  cmGeneratedFileStream makefileStream(makefileName.c_str());
  if (!makefileStream) {
    return;
  }
  makefileStream.SetCopyIfDifferent(true);
  makefileStream << "# Generated by CMake, DO NOT EDIT\n";
  makefileStream << "# Custom rules for " << target->GetName() << "\n";

  // disable the implicit rules
  makefileStream << ".SUFFIXES: "
                 << "\n";

  // have all depend on all outputs
  makefileStream << "all: ";
  std::map<const cmCustomCommand*, std::string> tname;
  int count = 0;
  for (std::vector<cmCustomCommand>::const_iterator i = commands.begin();
       i != commands.end(); ++i) {
    cmCustomCommandGenerator ccg(*i, configName, this->CurrentLocalGenerator);
    if (ccg.GetNumberOfCommands() > 0) {
      const std::vector<std::string>& outputs = ccg.GetOutputs();
      if (!outputs.empty()) {
        for (std::vector<std::string>::const_iterator o = outputs.begin();
             o != outputs.end(); ++o) {
          makefileStream << "\\\n\t"
                         << this->ConvertToRelativeForMake(o->c_str());
        }
      } else {
        std::ostringstream str;
        str << "_buildpart_" << count++;
        tname[&ccg.GetCC()] = std::string(target->GetName()) + str.str();
        makefileStream << "\\\n\t" << tname[&ccg.GetCC()];
      }
    }
  }
  makefileStream << "\n\n";
  for (std::vector<cmCustomCommand>::const_iterator i = commands.begin();
       i != commands.end(); ++i) {
    cmCustomCommandGenerator ccg(*i, configName, this->CurrentLocalGenerator);
    if (ccg.GetNumberOfCommands() > 0) {
      makefileStream << "\n";
      const std::vector<std::string>& outputs = ccg.GetOutputs();
      if (!outputs.empty()) {
        // There is at least one output, start the rule for it
        const char* sep = "";
        for (std::vector<std::string>::const_iterator oi = outputs.begin();
             oi != outputs.end(); ++oi) {
          makefileStream << sep << this->ConvertToRelativeForMake(oi->c_str());
          sep = " ";
        }
        makefileStream << ": ";
      } else {
        // There are no outputs.  Use the generated force rule name.
        makefileStream << tname[&ccg.GetCC()] << ": ";
      }
      for (std::vector<std::string>::const_iterator d =
             ccg.GetDepends().begin();
           d != ccg.GetDepends().end(); ++d) {
        std::string dep;
        if (this->CurrentLocalGenerator->GetRealDependency(*d, configName,
                                                           dep)) {
          makefileStream << "\\\n"
                         << this->ConvertToRelativeForMake(dep.c_str());
        }
      }
      makefileStream << "\n";

      if (const char* comment = ccg.GetComment()) {
        std::string echo_cmd = "echo ";
        echo_cmd += (this->CurrentLocalGenerator->EscapeForShell(
          comment, ccg.GetCC().GetEscapeAllowMakeVars()));
        makefileStream << "\t" << echo_cmd << "\n";
      }

      // Add each command line to the set of commands.
      for (unsigned int c = 0; c < ccg.GetNumberOfCommands(); ++c) {
        // Build the command line in a single string.
        std::string cmd2 = ccg.GetCommand(c);
        cmSystemTools::ReplaceString(cmd2, "/./", "/");
        cmd2 = this->ConvertToRelativeForMake(cmd2.c_str());
        std::string cmd;
        std::string wd = ccg.GetWorkingDirectory();
        if (!wd.empty()) {
          cmd += "cd ";
          cmd += this->ConvertToRelativeForMake(wd.c_str());
          cmd += " && ";
        }
        cmd += cmd2;
        ccg.AppendArguments(c, cmd);
        makefileStream << "\t" << cmd << "\n";
      }
    }
  }
}

void cmGlobalXCodeGenerator::CreateBuildSettings(cmGeneratorTarget* gtgt,
                                                 cmXCodeObject* buildSettings,
                                                 const std::string& configName)
{
  if (gtgt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return;
  }

  std::string defFlags;
  bool shared = ((gtgt->GetType() == cmStateEnums::SHARED_LIBRARY) ||
                 (gtgt->GetType() == cmStateEnums::MODULE_LIBRARY));
  bool binary = ((gtgt->GetType() == cmStateEnums::OBJECT_LIBRARY) ||
                 (gtgt->GetType() == cmStateEnums::STATIC_LIBRARY) ||
                 (gtgt->GetType() == cmStateEnums::EXECUTABLE) || shared);

  // Compute the compilation flags for each language.
  std::set<std::string> languages;
  gtgt->GetLanguages(languages, configName);
  std::map<std::string, std::string> cflags;
  for (std::set<std::string>::iterator li = languages.begin();
       li != languages.end(); ++li) {
    std::string const& lang = *li;
    std::string& flags = cflags[lang];

    // Add language-specific flags.
    this->CurrentLocalGenerator->AddLanguageFlags(flags, gtgt, lang,
                                                  configName);

    // Add shared-library flags if needed.
    this->CurrentLocalGenerator->AddCMP0018Flags(flags, gtgt, lang,
                                                 configName);

    this->CurrentLocalGenerator->AddVisibilityPresetFlags(flags, gtgt, lang);

    this->CurrentLocalGenerator->AddCompileOptions(flags, gtgt, lang,
                                                   configName);
  }

  std::string llang = gtgt->GetLinkerLanguage(configName);
  if (binary && llang.empty()) {
    cmSystemTools::Error(
      "CMake can not determine linker language for target: ",
      gtgt->GetName().c_str());
    return;
  }

  if (gtgt->IsIPOEnabled(llang, configName)) {
    const char* ltoValue =
      this->CurrentMakefile->IsOn("_CMAKE_LTO_THIN") ? "YES_THIN" : "YES";
    buildSettings->AddAttribute("LLVM_LTO", this->CreateString(ltoValue));
  }

  // Add define flags
  this->CurrentLocalGenerator->AppendFlags(
    defFlags, this->CurrentMakefile->GetDefineFlags());

  // Add preprocessor definitions for this target and configuration.
  BuildObjectListOrString ppDefs(this, true);
  this->AppendDefines(
    ppDefs, "CMAKE_INTDIR=\"$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)\"");
  if (const char* exportMacro = gtgt->GetExportMacro()) {
    // Add the export symbol definition for shared library objects.
    this->AppendDefines(ppDefs, exportMacro);
  }
  std::vector<std::string> targetDefines;
  gtgt->GetCompileDefinitions(targetDefines, configName, "C");
  this->AppendDefines(ppDefs, targetDefines);
  buildSettings->AddAttribute("GCC_PREPROCESSOR_DEFINITIONS",
                              ppDefs.CreateList());

  std::string extraLinkOptionsVar;
  std::string extraLinkOptions;
  if (gtgt->GetType() == cmStateEnums::EXECUTABLE) {
    extraLinkOptionsVar = "CMAKE_EXE_LINKER_FLAGS";
  } else if (gtgt->GetType() == cmStateEnums::SHARED_LIBRARY) {
    extraLinkOptionsVar = "CMAKE_SHARED_LINKER_FLAGS";
  } else if (gtgt->GetType() == cmStateEnums::MODULE_LIBRARY) {
    extraLinkOptionsVar = "CMAKE_MODULE_LINKER_FLAGS";
  }
  if (!extraLinkOptionsVar.empty()) {
    this->CurrentLocalGenerator->AddConfigVariableFlags(
      extraLinkOptions, extraLinkOptionsVar, configName);
  }

  if (gtgt->GetType() == cmStateEnums::OBJECT_LIBRARY ||
      gtgt->GetType() == cmStateEnums::STATIC_LIBRARY) {
    this->CurrentLocalGenerator->GetStaticLibraryFlags(
      extraLinkOptions, cmSystemTools::UpperCase(configName), gtgt);
  } else {
    const char* targetLinkFlags = gtgt->GetProperty("LINK_FLAGS");
    if (targetLinkFlags) {
      this->CurrentLocalGenerator->AppendFlags(extraLinkOptions,
                                               targetLinkFlags);
    }
    if (!configName.empty()) {
      std::string linkFlagsVar = "LINK_FLAGS_";
      linkFlagsVar += cmSystemTools::UpperCase(configName);
      if (const char* linkFlags = gtgt->GetProperty(linkFlagsVar)) {
        this->CurrentLocalGenerator->AppendFlags(extraLinkOptions, linkFlags);
      }
    }
  }

  // Set target-specific architectures.
  std::vector<std::string> archs;
  gtgt->GetAppleArchs(configName, archs);

  if (!archs.empty()) {
    // Enable ARCHS attribute.
    buildSettings->AddAttribute("ONLY_ACTIVE_ARCH", this->CreateString("NO"));

    // Store ARCHS value.
    if (archs.size() == 1) {
      buildSettings->AddAttribute("ARCHS", this->CreateString(archs[0]));
    } else {
      cmXCodeObject* archObjects =
        this->CreateObject(cmXCodeObject::OBJECT_LIST);
      for (std::vector<std::string>::iterator i = archs.begin();
           i != archs.end(); i++) {
        archObjects->AddObject(this->CreateString(*i));
      }
      buildSettings->AddAttribute("ARCHS", archObjects);
    }
  }

  // Get the product name components.
  std::string pnprefix;
  std::string pnbase;
  std::string pnsuffix;
  gtgt->GetFullNameComponents(pnprefix, pnbase, pnsuffix, configName);

  const char* version = gtgt->GetProperty("VERSION");
  const char* soversion = gtgt->GetProperty("SOVERSION");
  if (!gtgt->HasSOName(configName) || gtgt->IsFrameworkOnApple()) {
    version = 0;
    soversion = 0;
  }
  if (version && !soversion) {
    soversion = version;
  }
  if (!version && soversion) {
    version = soversion;
  }

  std::string realName = pnbase;
  std::string soName = pnbase;
  if (version && soversion) {
    realName += ".";
    realName += version;
    soName += ".";
    soName += soversion;
  }

  // Set attributes to specify the proper name for the target.
  std::string pndir = this->CurrentLocalGenerator->GetCurrentBinaryDirectory();
  if (gtgt->GetType() == cmStateEnums::STATIC_LIBRARY ||
      gtgt->GetType() == cmStateEnums::SHARED_LIBRARY ||
      gtgt->GetType() == cmStateEnums::MODULE_LIBRARY ||
      gtgt->GetType() == cmStateEnums::EXECUTABLE) {
    if (!gtgt->UsesDefaultOutputDir(configName,
                                    cmStateEnums::RuntimeBinaryArtifact)) {
      std::string pncdir = gtgt->GetDirectory(configName);
      buildSettings->AddAttribute("CONFIGURATION_BUILD_DIR",
                                  this->CreateString(pncdir));
    }

    if (gtgt->IsFrameworkOnApple() || gtgt->IsCFBundleOnApple()) {
      pnprefix = "";
    }

    buildSettings->AddAttribute("EXECUTABLE_PREFIX",
                                this->CreateString(pnprefix));
    buildSettings->AddAttribute("EXECUTABLE_SUFFIX",
                                this->CreateString(pnsuffix));
  } else if (gtgt->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    pnprefix = "lib";
    pnbase = gtgt->GetName();
    pnsuffix = ".a";

    std::string pncdir =
      this->GetObjectsNormalDirectory(this->CurrentProject, configName, gtgt);
    buildSettings->AddAttribute("CONFIGURATION_BUILD_DIR",
                                this->CreateString(pncdir));
  }

  // Store the product name for all target types.
  buildSettings->AddAttribute("PRODUCT_NAME", this->CreateString(realName));
  buildSettings->AddAttribute("SYMROOT", this->CreateString(pndir));

  // Handle settings for each target type.
  switch (gtgt->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      if (gtgt->GetPropertyAsBool("FRAMEWORK")) {
        std::string fw_version = gtgt->GetFrameworkVersion();
        buildSettings->AddAttribute("FRAMEWORK_VERSION",
                                    this->CreateString(fw_version));
        const char* ext = gtgt->GetProperty("BUNDLE_EXTENSION");
        if (ext) {
          buildSettings->AddAttribute("WRAPPER_EXTENSION",
                                      this->CreateString(ext));
        }

        std::string plist = this->ComputeInfoPListLocation(gtgt);
        // Xcode will create the final version of Info.plist at build time,
        // so let it replace the framework name. This avoids creating
        // a per-configuration Info.plist file.
        this->CurrentLocalGenerator->GenerateFrameworkInfoPList(
          gtgt, "$(EXECUTABLE_NAME)", plist.c_str());
        buildSettings->AddAttribute("INFOPLIST_FILE",
                                    this->CreateString(plist));
        buildSettings->AddAttribute("MACH_O_TYPE",
                                    this->CreateString("staticlib"));
      } else {
        buildSettings->AddAttribute("LIBRARY_STYLE",
                                    this->CreateString("STATIC"));
      }
      break;

    case cmStateEnums::OBJECT_LIBRARY: {
      buildSettings->AddAttribute("LIBRARY_STYLE",
                                  this->CreateString("STATIC"));
      break;
    }

    case cmStateEnums::MODULE_LIBRARY: {
      buildSettings->AddAttribute("LIBRARY_STYLE",
                                  this->CreateString("BUNDLE"));
      if (gtgt->IsCFBundleOnApple()) {
        // It turns out that a BUNDLE is basically the same
        // in many ways as an application bundle, as far as
        // link flags go
        std::string createFlags = this->LookupFlags(
          "CMAKE_SHARED_MODULE_CREATE_", llang, "_FLAGS", "-bundle");
        if (!createFlags.empty()) {
          extraLinkOptions += " ";
          extraLinkOptions += createFlags;
        }
        const char* ext = gtgt->GetProperty("BUNDLE_EXTENSION");
        if (ext) {
          buildSettings->AddAttribute("WRAPPER_EXTENSION",
                                      this->CreateString(ext));
        }
        std::string plist = this->ComputeInfoPListLocation(gtgt);
        // Xcode will create the final version of Info.plist at build time,
        // so let it replace the cfbundle name. This avoids creating
        // a per-configuration Info.plist file. The cfbundle plist
        // is very similar to the application bundle plist
        this->CurrentLocalGenerator->GenerateAppleInfoPList(
          gtgt, "$(EXECUTABLE_NAME)", plist.c_str());
        buildSettings->AddAttribute("INFOPLIST_FILE",
                                    this->CreateString(plist));
      } else {
        buildSettings->AddAttribute("MACH_O_TYPE",
                                    this->CreateString("mh_bundle"));
        buildSettings->AddAttribute("GCC_DYNAMIC_NO_PIC",
                                    this->CreateString("NO"));
        // Add the flags to create an executable.
        std::string createFlags =
          this->LookupFlags("CMAKE_", llang, "_LINK_FLAGS", "");
        if (!createFlags.empty()) {
          extraLinkOptions += " ";
          extraLinkOptions += createFlags;
        }
      }
      break;
    }
    case cmStateEnums::SHARED_LIBRARY: {
      if (gtgt->GetPropertyAsBool("FRAMEWORK")) {
        std::string fw_version = gtgt->GetFrameworkVersion();
        buildSettings->AddAttribute("FRAMEWORK_VERSION",
                                    this->CreateString(fw_version));
        const char* ext = gtgt->GetProperty("BUNDLE_EXTENSION");
        if (ext) {
          buildSettings->AddAttribute("WRAPPER_EXTENSION",
                                      this->CreateString(ext));
        }

        std::string plist = this->ComputeInfoPListLocation(gtgt);
        // Xcode will create the final version of Info.plist at build time,
        // so let it replace the framework name. This avoids creating
        // a per-configuration Info.plist file.
        this->CurrentLocalGenerator->GenerateFrameworkInfoPList(
          gtgt, "$(EXECUTABLE_NAME)", plist.c_str());
        buildSettings->AddAttribute("INFOPLIST_FILE",
                                    this->CreateString(plist));
      } else {
        // Add the flags to create a shared library.
        std::string createFlags = this->LookupFlags(
          "CMAKE_SHARED_LIBRARY_CREATE_", llang, "_FLAGS", "-dynamiclib");
        if (!createFlags.empty()) {
          extraLinkOptions += " ";
          extraLinkOptions += createFlags;
        }
      }

      buildSettings->AddAttribute("LIBRARY_STYLE",
                                  this->CreateString("DYNAMIC"));
      break;
    }
    case cmStateEnums::EXECUTABLE: {
      // Add the flags to create an executable.
      std::string createFlags =
        this->LookupFlags("CMAKE_", llang, "_LINK_FLAGS", "");
      if (!createFlags.empty()) {
        extraLinkOptions += " ";
        extraLinkOptions += createFlags;
      }

      // Handle bundles and normal executables separately.
      if (gtgt->GetPropertyAsBool("MACOSX_BUNDLE")) {
        const char* ext = gtgt->GetProperty("BUNDLE_EXTENSION");
        if (ext) {
          buildSettings->AddAttribute("WRAPPER_EXTENSION",
                                      this->CreateString(ext));
        }
        std::string plist = this->ComputeInfoPListLocation(gtgt);
        // Xcode will create the final version of Info.plist at build time,
        // so let it replace the executable name.  This avoids creating
        // a per-configuration Info.plist file.
        this->CurrentLocalGenerator->GenerateAppleInfoPList(
          gtgt, "$(EXECUTABLE_NAME)", plist.c_str());
        buildSettings->AddAttribute("INFOPLIST_FILE",
                                    this->CreateString(plist));
      }
    } break;
    default:
      break;
  }
  if (this->XcodeVersion < 40) {
    buildSettings->AddAttribute("PREBINDING", this->CreateString("NO"));
  }

  BuildObjectListOrString dirs(this, true);
  BuildObjectListOrString fdirs(this, true);
  BuildObjectListOrString sysdirs(this, true);
  BuildObjectListOrString sysfdirs(this, true);
  const bool emitSystemIncludes = this->XcodeVersion >= 83;

  std::vector<std::string> includes;
  this->CurrentLocalGenerator->GetIncludeDirectories(includes, gtgt, "C",
                                                     configName);
  std::set<std::string> emitted;
  emitted.insert("/System/Library/Frameworks");

  for (std::vector<std::string>::iterator i = includes.begin();
       i != includes.end(); ++i) {
    if (this->NameResolvesToFramework(*i)) {
      std::string frameworkDir = *i;
      frameworkDir += "/../";
      frameworkDir = cmSystemTools::CollapseFullPath(frameworkDir);
      if (emitted.insert(frameworkDir).second) {
        std::string incpath = this->XCodeEscapePath(frameworkDir);
        if (emitSystemIncludes &&
            gtgt->IsSystemIncludeDirectory(frameworkDir, configName)) {
          sysfdirs.Add(incpath);
        } else {
          fdirs.Add(incpath);
        }
      }
    } else {
      std::string incpath = this->XCodeEscapePath(*i);
      if (emitSystemIncludes &&
          gtgt->IsSystemIncludeDirectory(*i, configName)) {
        sysdirs.Add(incpath);
      } else {
        dirs.Add(incpath);
      }
    }
  }
  // Add framework search paths needed for linking.
  if (cmComputeLinkInformation* cli = gtgt->GetLinkInformation(configName)) {
    std::vector<std::string> const& fwDirs = cli->GetFrameworkPaths();
    for (std::vector<std::string>::const_iterator fdi = fwDirs.begin();
         fdi != fwDirs.end(); ++fdi) {
      if (emitted.insert(*fdi).second) {
        std::string incpath = this->XCodeEscapePath(*fdi);
        if (emitSystemIncludes &&
            gtgt->IsSystemIncludeDirectory(*fdi, configName)) {
          sysfdirs.Add(incpath);
        } else {
          fdirs.Add(incpath);
        }
      }
    }
  }
  if (!fdirs.IsEmpty()) {
    buildSettings->AddAttribute("FRAMEWORK_SEARCH_PATHS", fdirs.CreateList());
  }
  if (!dirs.IsEmpty()) {
    buildSettings->AddAttribute("HEADER_SEARCH_PATHS", dirs.CreateList());
  }
  if (!sysfdirs.IsEmpty()) {
    buildSettings->AddAttribute("SYSTEM_FRAMEWORK_SEARCH_PATHS",
                                sysfdirs.CreateList());
  }
  if (!sysdirs.IsEmpty()) {
    buildSettings->AddAttribute("SYSTEM_HEADER_SEARCH_PATHS",
                                sysdirs.CreateList());
  }

  if (this->XcodeVersion >= 60 && !emitSystemIncludes) {
    // Add those per-language flags in addition to HEADER_SEARCH_PATHS to gain
    // system include directory awareness. We need to also keep on setting
    // HEADER_SEARCH_PATHS to work around a missing compile options flag for
    // GNU assembly files (#16449)
    for (std::set<std::string>::iterator li = languages.begin();
         li != languages.end(); ++li) {
      std::string includeFlags = this->CurrentLocalGenerator->GetIncludeFlags(
        includes, gtgt, *li, true, false, configName);

      if (!includeFlags.empty()) {
        cflags[*li] += " " + includeFlags;
      }
    }
  }

  bool same_gflags = true;
  std::map<std::string, std::string> gflags;
  std::string const* last_gflag = 0;
  std::string optLevel = "0";

  // Minimal map of flags to build settings.
  for (std::set<std::string>::iterator li = languages.begin();
       li != languages.end(); ++li) {
    std::string& flags = cflags[*li];
    std::string& gflag = gflags[*li];
    std::string oflag =
      this->ExtractFlagRegex("(^| )(-Ofast|-Os|-O[0-9]*)( |$)", 2, flags);
    if (oflag.size() == 2) {
      optLevel = "1";
    } else if (oflag.size() > 2) {
      optLevel = oflag.substr(2);
    }
    gflag = this->ExtractFlag("-g", flags);
    // put back gdwarf-2 if used since there is no way
    // to represent it in the gui, but we still want debug yes
    if (gflag == "-gdwarf-2") {
      flags += " ";
      flags += gflag;
    }
    if (last_gflag && *last_gflag != gflag) {
      same_gflags = false;
    }
    last_gflag = &gflag;
  }

  const char* debugStr = "YES";
  if (!same_gflags) {
    // We can't set the Xcode flag differently depending on the language,
    // so put them back in this case.
    for (std::set<std::string>::iterator li = languages.begin();
         li != languages.end(); ++li) {
      cflags[*li] += " ";
      cflags[*li] += gflags[*li];
    }
    debugStr = "NO";
  } else if (last_gflag && (last_gflag->empty() || *last_gflag == "-g0")) {
    debugStr = "NO";
  }

  buildSettings->AddAttribute("COMBINE_HIDPI_IMAGES",
                              this->CreateString("YES"));
  buildSettings->AddAttribute("GCC_GENERATE_DEBUGGING_SYMBOLS",
                              this->CreateString(debugStr));
  buildSettings->AddAttribute("GCC_OPTIMIZATION_LEVEL",
                              this->CreateString(optLevel));
  buildSettings->AddAttribute("GCC_SYMBOLS_PRIVATE_EXTERN",
                              this->CreateString("NO"));
  buildSettings->AddAttribute("GCC_INLINES_ARE_PRIVATE_EXTERN",
                              this->CreateString("NO"));
  for (std::set<std::string>::iterator li = languages.begin();
       li != languages.end(); ++li) {
    std::string flags = cflags[*li] + " " + defFlags;
    if (*li == "CXX") {
      buildSettings->AddAttribute("OTHER_CPLUSPLUSFLAGS",
                                  this->CreateString(flags));
    } else if (*li == "Fortran") {
      buildSettings->AddAttribute("IFORT_OTHER_FLAGS",
                                  this->CreateString(flags));
    } else if (*li == "C") {
      buildSettings->AddAttribute("OTHER_CFLAGS", this->CreateString(flags));
    } else if (*li == "Swift") {
      buildSettings->AddAttribute("OTHER_SWIFT_FLAGS",
                                  this->CreateString(flags));
    }
  }

  // Add Fortran source format attribute if property is set.
  const char* format = 0;
  const char* tgtfmt = gtgt->GetProperty("Fortran_FORMAT");
  switch (cmOutputConverter::GetFortranFormat(tgtfmt)) {
    case cmOutputConverter::FortranFormatFixed:
      format = "fixed";
      break;
    case cmOutputConverter::FortranFormatFree:
      format = "free";
      break;
    default:
      break;
  }
  if (format) {
    buildSettings->AddAttribute("IFORT_LANG_SRCFMT",
                                this->CreateString(format));
  }

  // Create the INSTALL_PATH attribute.
  std::string install_name_dir;
  if (gtgt->GetType() == cmStateEnums::SHARED_LIBRARY) {
    // Get the install_name directory for the build tree.
    install_name_dir = gtgt->GetInstallNameDirForBuildTree(configName);
    // Xcode doesn't create the correct install_name in some cases.
    // That is, if the INSTALL_PATH is empty, or if we have versioning
    // of dylib libraries, we want to specify the install_name.
    // This is done by adding a link flag to create an install_name
    // with just the library soname.
    std::string install_name;
    if (!install_name_dir.empty()) {
      // Convert to a path for the native build tool.
      cmSystemTools::ConvertToUnixSlashes(install_name_dir);
      install_name += install_name_dir;
      install_name += "/";
    }
    install_name += gtgt->GetSOName(configName);

    if ((realName != soName) || install_name_dir.empty()) {
      install_name_dir = "";
      extraLinkOptions += " -install_name ";
      extraLinkOptions += XCodeEscapePath(install_name);
    }
  }
  buildSettings->AddAttribute("INSTALL_PATH",
                              this->CreateString(install_name_dir));

  // Create the LD_RUNPATH_SEARCH_PATHS
  cmComputeLinkInformation* pcli = gtgt->GetLinkInformation(configName);
  if (pcli) {
    std::string search_paths;
    std::vector<std::string> runtimeDirs;
    pcli->GetRPath(runtimeDirs, false);
    // runpath dirs needs to be unique to prevent corruption
    std::set<std::string> unique_dirs;

    for (std::vector<std::string>::const_iterator i = runtimeDirs.begin();
         i != runtimeDirs.end(); ++i) {
      std::string runpath = *i;
      runpath = this->ExpandCFGIntDir(runpath, configName);

      if (unique_dirs.find(runpath) == unique_dirs.end()) {
        unique_dirs.insert(runpath);
        if (!search_paths.empty()) {
          search_paths += " ";
        }
        search_paths += this->XCodeEscapePath(runpath);
      }
    }
    if (!search_paths.empty()) {
      buildSettings->AddAttribute("LD_RUNPATH_SEARCH_PATHS",
                                  this->CreateString(search_paths));
    }
  }

  buildSettings->AddAttribute(this->GetTargetLinkFlagsVar(gtgt),
                              this->CreateString(extraLinkOptions));
  buildSettings->AddAttribute("OTHER_REZFLAGS", this->CreateString(""));
  buildSettings->AddAttribute("SECTORDER_FLAGS", this->CreateString(""));
  buildSettings->AddAttribute("USE_HEADERMAP", this->CreateString("NO"));
  cmXCodeObject* group = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  group->AddObject(this->CreateString("-Wmost"));
  group->AddObject(this->CreateString("-Wno-four-char-constants"));
  group->AddObject(this->CreateString("-Wno-unknown-pragmas"));
  group->AddObject(this->CreateString("$(inherited)"));
  buildSettings->AddAttribute("WARNING_CFLAGS", group);

  // Runtime version information.
  if (gtgt->GetType() == cmStateEnums::SHARED_LIBRARY) {
    int major;
    int minor;
    int patch;

    // VERSION -> current_version
    gtgt->GetTargetVersion(false, major, minor, patch);
    std::ostringstream v;

    // Xcode always wants at least 1.0.0 or nothing
    if (!(major == 0 && minor == 0 && patch == 0)) {
      v << major << "." << minor << "." << patch;
    }
    buildSettings->AddAttribute("DYLIB_CURRENT_VERSION",
                                this->CreateString(v.str()));

    // SOVERSION -> compatibility_version
    gtgt->GetTargetVersion(true, major, minor, patch);
    std::ostringstream vso;

    // Xcode always wants at least 1.0.0 or nothing
    if (!(major == 0 && minor == 0 && patch == 0)) {
      vso << major << "." << minor << "." << patch;
    }
    buildSettings->AddAttribute("DYLIB_COMPATIBILITY_VERSION",
                                this->CreateString(vso.str()));
  }
  // put this last so it can override existing settings
  // Convert "XCODE_ATTRIBUTE_*" properties directly.
  {
    std::vector<std::string> const& props = gtgt->GetPropertyKeys();
    for (std::vector<std::string>::const_iterator i = props.begin();
         i != props.end(); ++i) {
      if (i->find("XCODE_ATTRIBUTE_") == 0) {
        std::string attribute = i->substr(16);
        this->FilterConfigurationAttribute(configName, attribute);
        if (!attribute.empty()) {
          cmGeneratorExpression ge;
          std::string processed =
            ge.Parse(gtgt->GetProperty(*i))
              ->Evaluate(this->CurrentLocalGenerator, configName);
          buildSettings->AddAttribute(attribute,
                                      this->CreateString(processed));
        }
      }
    }
  }
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateUtilityTarget(
  cmGeneratorTarget* gtgt)
{
  cmXCodeObject* shellBuildPhase =
    this->CreateObject(cmXCodeObject::PBXShellScriptBuildPhase);
  shellBuildPhase->AddAttribute("buildActionMask",
                                this->CreateString("2147483647"));
  cmXCodeObject* buildFiles = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  shellBuildPhase->AddAttribute("files", buildFiles);
  cmXCodeObject* inputPaths = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  shellBuildPhase->AddAttribute("inputPaths", inputPaths);
  cmXCodeObject* outputPaths = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  shellBuildPhase->AddAttribute("outputPaths", outputPaths);
  shellBuildPhase->AddAttribute("runOnlyForDeploymentPostprocessing",
                                this->CreateString("0"));
  shellBuildPhase->AddAttribute("shellPath", this->CreateString("/bin/sh"));
  shellBuildPhase->AddAttribute(
    "shellScript", this->CreateString("# shell script goes here\nexit 0"));
  shellBuildPhase->AddAttribute("showEnvVarsInLog", this->CreateString("0"));

  cmXCodeObject* target =
    this->CreateObject(cmXCodeObject::PBXAggregateTarget);
  target->SetComment(gtgt->GetName());
  cmXCodeObject* buildPhases = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  std::vector<cmXCodeObject*> emptyContentVector;
  this->CreateCustomCommands(buildPhases, 0, 0, 0, emptyContentVector, 0,
                             gtgt);
  target->AddAttribute("buildPhases", buildPhases);
  this->AddConfigurations(target, gtgt);
  cmXCodeObject* dependencies = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  target->AddAttribute("dependencies", dependencies);
  target->AddAttribute("name", this->CreateString(gtgt->GetName()));
  target->AddAttribute("productName", this->CreateString(gtgt->GetName()));
  target->SetTarget(gtgt);
  this->XCodeObjectMap[gtgt] = target;

  // Add source files without build rules for editing convenience.
  if (gtgt->GetType() == cmStateEnums::UTILITY) {
    std::vector<cmSourceFile*> sources;
    if (!gtgt->GetConfigCommonSourceFiles(sources)) {
      return 0;
    }

    for (std::vector<cmSourceFile*>::const_iterator i = sources.begin();
         i != sources.end(); ++i) {
      if (!(*i)->GetPropertyAsBool("GENERATED")) {
        this->CreateXCodeFileReference(*i, gtgt);
      }
    }
  }

  target->SetId(this->GetOrCreateId(gtgt->GetName(), target->GetId()));

  return target;
}

std::string cmGlobalXCodeGenerator::AddConfigurations(cmXCodeObject* target,
                                                      cmGeneratorTarget* gtgt)
{
  std::string configTypes =
    this->CurrentMakefile->GetRequiredDefinition("CMAKE_CONFIGURATION_TYPES");
  std::vector<std::string> configVectorIn;
  std::vector<std::string> configVector;
  configVectorIn.push_back(configTypes);
  cmSystemTools::ExpandList(configVectorIn, configVector);
  cmXCodeObject* configlist =
    this->CreateObject(cmXCodeObject::XCConfigurationList);
  cmXCodeObject* buildConfigurations =
    this->CreateObject(cmXCodeObject::OBJECT_LIST);
  configlist->AddAttribute("buildConfigurations", buildConfigurations);
  std::string comment = "Build configuration list for ";
  comment += cmXCodeObject::PBXTypeNames[target->GetIsA()];
  comment += " \"";
  comment += gtgt->GetName();
  comment += "\"";
  configlist->SetComment(comment);
  target->AddAttribute("buildConfigurationList",
                       this->CreateObjectReference(configlist));
  for (unsigned int i = 0; i < configVector.size(); ++i) {
    cmXCodeObject* config =
      this->CreateObject(cmXCodeObject::XCBuildConfiguration);
    buildConfigurations->AddObject(config);
    cmXCodeObject* buildSettings =
      this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
    this->CreateBuildSettings(gtgt, buildSettings, configVector[i]);
    config->AddAttribute("name", this->CreateString(configVector[i]));
    config->SetComment(configVector[i]);
    config->AddAttribute("buildSettings", buildSettings);
  }
  if (!configVector.empty()) {
    configlist->AddAttribute("defaultConfigurationName",
                             this->CreateString(configVector[0]));
    configlist->AddAttribute("defaultConfigurationIsVisible",
                             this->CreateString("0"));
    return configVector[0];
  }
  return "";
}

const char* cmGlobalXCodeGenerator::GetTargetLinkFlagsVar(
  cmGeneratorTarget const* target) const
{
  if (this->XcodeVersion >= 60 &&
      (target->GetType() == cmStateEnums::STATIC_LIBRARY ||
       target->GetType() == cmStateEnums::OBJECT_LIBRARY)) {
    return "OTHER_LIBTOOLFLAGS";
  } else {
    return "OTHER_LDFLAGS";
  }
}

const char* cmGlobalXCodeGenerator::GetTargetFileType(
  cmGeneratorTarget* target)
{
  if (const char* e = target->GetProperty("XCODE_EXPLICIT_FILE_TYPE")) {
    return e;
  }

  switch (target->GetType()) {
    case cmStateEnums::OBJECT_LIBRARY:
      return "archive.ar";
    case cmStateEnums::STATIC_LIBRARY:
      return (target->GetPropertyAsBool("FRAMEWORK") ? "wrapper.framework"
                                                     : "archive.ar");
    case cmStateEnums::MODULE_LIBRARY:
      if (target->IsXCTestOnApple())
        return "wrapper.cfbundle";
      else if (target->IsCFBundleOnApple())
        return "wrapper.plug-in";
      else
        return "compiled.mach-o.executable";
    case cmStateEnums::SHARED_LIBRARY:
      return (target->GetPropertyAsBool("FRAMEWORK")
                ? "wrapper.framework"
                : "compiled.mach-o.dylib");
    case cmStateEnums::EXECUTABLE:
      return "compiled.mach-o.executable";
    default:
      break;
  }
  return 0;
}

const char* cmGlobalXCodeGenerator::GetTargetProductType(
  cmGeneratorTarget* target)
{
  if (const char* e = target->GetProperty("XCODE_PRODUCT_TYPE")) {
    return e;
  }

  switch (target->GetType()) {
    case cmStateEnums::OBJECT_LIBRARY:
      return "com.apple.product-type.library.static";
    case cmStateEnums::STATIC_LIBRARY:
      return (target->GetPropertyAsBool("FRAMEWORK")
                ? "com.apple.product-type.framework"
                : "com.apple.product-type.library.static");
    case cmStateEnums::MODULE_LIBRARY:
      if (target->IsXCTestOnApple())
        return "com.apple.product-type.bundle.unit-test";
      else if (target->IsCFBundleOnApple())
        return "com.apple.product-type.bundle";
      else
        return "com.apple.product-type.tool";
    case cmStateEnums::SHARED_LIBRARY:
      return (target->GetPropertyAsBool("FRAMEWORK")
                ? "com.apple.product-type.framework"
                : "com.apple.product-type.library.dynamic");
    case cmStateEnums::EXECUTABLE:
      return (target->GetPropertyAsBool("MACOSX_BUNDLE")
                ? "com.apple.product-type.application"
                : "com.apple.product-type.tool");
    default:
      break;
  }
  return 0;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateXCodeTarget(
  cmGeneratorTarget* gtgt, cmXCodeObject* buildPhases)
{
  if (gtgt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return 0;
  }
  cmXCodeObject* target = this->CreateObject(cmXCodeObject::PBXNativeTarget);
  target->AddAttribute("buildPhases", buildPhases);
  cmXCodeObject* buildRules = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  target->AddAttribute("buildRules", buildRules);
  std::string defConfig;
  defConfig = this->AddConfigurations(target, gtgt);
  cmXCodeObject* dependencies = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  target->AddAttribute("dependencies", dependencies);
  target->AddAttribute("name", this->CreateString(gtgt->GetName()));
  target->AddAttribute("productName", this->CreateString(gtgt->GetName()));

  cmXCodeObject* fileRef = this->CreateObject(cmXCodeObject::PBXFileReference);
  if (const char* fileType = this->GetTargetFileType(gtgt)) {
    fileRef->AddAttribute("explicitFileType", this->CreateString(fileType));
  }
  std::string fullName;
  if (gtgt->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    fullName = "lib";
    fullName += gtgt->GetName();
    fullName += ".a";
  } else {
    fullName = gtgt->GetFullName(defConfig);
  }
  fileRef->AddAttribute("path", this->CreateString(fullName));
  fileRef->AddAttribute("sourceTree",
                        this->CreateString("BUILT_PRODUCTS_DIR"));
  fileRef->SetComment(gtgt->GetName());
  target->AddAttribute("productReference",
                       this->CreateObjectReference(fileRef));
  if (const char* productType = this->GetTargetProductType(gtgt)) {
    target->AddAttribute("productType", this->CreateString(productType));
  }
  target->SetTarget(gtgt);
  this->XCodeObjectMap[gtgt] = target;
  target->SetId(this->GetOrCreateId(gtgt->GetName(), target->GetId()));
  return target;
}

cmXCodeObject* cmGlobalXCodeGenerator::FindXCodeTarget(
  cmGeneratorTarget const* t)
{
  if (!t) {
    return 0;
  }

  std::map<cmGeneratorTarget const*, cmXCodeObject*>::const_iterator const i =
    this->XCodeObjectMap.find(t);
  if (i == this->XCodeObjectMap.end()) {
    return 0;
  }
  return i->second;
}

std::string cmGlobalXCodeGenerator::GetOrCreateId(const std::string& name,
                                                  const std::string& id)
{
  std::string guidStoreName = name;
  guidStoreName += "_GUID_CMAKE";
  const char* storedGUID =
    this->CMakeInstance->GetCacheDefinition(guidStoreName);

  if (storedGUID) {
    return storedGUID;
  }

  this->CMakeInstance->AddCacheEntry(guidStoreName, id.c_str(),
                                     "Stored Xcode object GUID",
                                     cmStateEnums::INTERNAL);

  return id;
}

void cmGlobalXCodeGenerator::AddDependTarget(cmXCodeObject* target,
                                             cmXCodeObject* dependTarget)
{
  // This is called once for every edge in the target dependency graph.
  cmXCodeObject* container =
    this->CreateObject(cmXCodeObject::PBXContainerItemProxy);
  container->SetComment("PBXContainerItemProxy");
  container->AddAttribute("containerPortal",
                          this->CreateObjectReference(this->RootObject));
  container->AddAttribute("proxyType", this->CreateString("1"));
  container->AddAttribute("remoteGlobalIDString",
                          this->CreateObjectReference(dependTarget));
  container->AddAttribute(
    "remoteInfo", this->CreateString(dependTarget->GetTarget()->GetName()));
  cmXCodeObject* targetdep =
    this->CreateObject(cmXCodeObject::PBXTargetDependency);
  targetdep->SetComment("PBXTargetDependency");
  targetdep->AddAttribute("target", this->CreateObjectReference(dependTarget));
  targetdep->AddAttribute("targetProxy",
                          this->CreateObjectReference(container));

  cmXCodeObject* depends = target->GetObject("dependencies");
  if (!depends) {
    cmSystemTools::Error(
      "target does not have dependencies attribute error..");

  } else {
    depends->AddUniqueObject(targetdep);
  }
}

void cmGlobalXCodeGenerator::AppendOrAddBuildSetting(cmXCodeObject* settings,
                                                     const char* attribute,
                                                     const char* value)
{
  if (settings) {
    cmXCodeObject* attr = settings->GetObject(attribute);
    if (!attr) {
      settings->AddAttribute(attribute, this->CreateString(value));
    } else {
      std::string oldValue = attr->GetString();
      oldValue += " ";
      oldValue += value;
      attr->SetString(oldValue);
    }
  }
}

void cmGlobalXCodeGenerator::AppendBuildSettingAttribute(
  cmXCodeObject* target, const char* attribute, const char* value,
  const std::string& configName)
{
  // There are multiple configurations.  Add the setting to the
  // buildSettings of the configuration name given.
  cmXCodeObject* configurationList =
    target->GetObject("buildConfigurationList")->GetObject();
  cmXCodeObject* buildConfigs =
    configurationList->GetObject("buildConfigurations");
  std::vector<cmXCodeObject*> list = buildConfigs->GetObjectList();
  // each configuration and the target itself has a buildSettings in it
  // list.push_back(target);
  for (std::vector<cmXCodeObject*>::iterator i = list.begin(); i != list.end();
       ++i) {
    if (!configName.empty()) {
      if ((*i)->GetObject("name")->GetString() == configName) {
        cmXCodeObject* settings = (*i)->GetObject("buildSettings");
        this->AppendOrAddBuildSetting(settings, attribute, value);
      }
    } else {
      cmXCodeObject* settings = (*i)->GetObject("buildSettings");
      this->AppendOrAddBuildSetting(settings, attribute, value);
    }
  }
}

void cmGlobalXCodeGenerator::AddDependAndLinkInformation(cmXCodeObject* target)
{
  cmGeneratorTarget* gt = target->GetTarget();
  if (!gt) {
    cmSystemTools::Error("Error no target on xobject\n");
    return;
  }
  if (gt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return;
  }

  // Add dependencies on other CMake targets.
  TargetDependSet const& deps = this->GetTargetDirectDepends(gt);
  for (TargetDependSet::const_iterator i = deps.begin(); i != deps.end();
       ++i) {
    if (cmXCodeObject* dptarget = this->FindXCodeTarget(*i)) {
      this->AddDependTarget(target, dptarget);
    }
  }

  // Loop over configuration types and set per-configuration info.
  for (std::vector<std::string>::iterator i =
         this->CurrentConfigurationTypes.begin();
       i != this->CurrentConfigurationTypes.end(); ++i) {
    // Get the current configuration name.
    std::string configName = *i;

    if (this->XcodeVersion >= 50) {
      // Add object library contents as link flags.
      std::string linkObjs;
      const char* sep = "";
      std::vector<cmSourceFile const*> objs;
      gt->GetExternalObjects(objs, configName);
      for (std::vector<cmSourceFile const*>::const_iterator oi = objs.begin();
           oi != objs.end(); ++oi) {
        if ((*oi)->GetObjectLibrary().empty()) {
          continue;
        }
        linkObjs += sep;
        sep = " ";
        linkObjs += this->XCodeEscapePath((*oi)->GetFullPath());
      }
      this->AppendBuildSettingAttribute(
        target, this->GetTargetLinkFlagsVar(gt), linkObjs.c_str(), configName);
    }

    // Skip link information for object libraries.
    if (gt->GetType() == cmStateEnums::OBJECT_LIBRARY ||
        gt->GetType() == cmStateEnums::STATIC_LIBRARY) {
      continue;
    }

    // Compute the link library and directory information.
    cmComputeLinkInformation* pcli = gt->GetLinkInformation(configName);
    if (!pcli) {
      continue;
    }
    cmComputeLinkInformation& cli = *pcli;

    // Add dependencies directly on library files.
    {
      std::vector<std::string> const& libDeps = cli.GetDepends();
      for (std::vector<std::string>::const_iterator j = libDeps.begin();
           j != libDeps.end(); ++j) {
        target->AddDependLibrary(configName, *j);
      }
    }

    // add the library search paths
    {
      std::vector<std::string> const& libDirs = cli.GetDirectories();
      std::string linkDirs;
      for (std::vector<std::string>::const_iterator libDir = libDirs.begin();
           libDir != libDirs.end(); ++libDir) {
        if (libDir->size() && *libDir != "/usr/lib") {
          // Now add the same one but append
          // $(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME) to it:
          linkDirs += " ";
          linkDirs += this->XCodeEscapePath(
            *libDir + "/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)");
          linkDirs += " ";
          linkDirs += this->XCodeEscapePath(*libDir);
        }
      }
      this->AppendBuildSettingAttribute(target, "LIBRARY_SEARCH_PATHS",
                                        linkDirs.c_str(), configName);
    }

    // now add the link libraries
    {
      std::string linkLibs;
      const char* sep = "";
      typedef cmComputeLinkInformation::ItemVector ItemVector;
      ItemVector const& libNames = cli.GetItems();
      for (ItemVector::const_iterator li = libNames.begin();
           li != libNames.end(); ++li) {
        linkLibs += sep;
        sep = " ";
        if (li->IsPath) {
          linkLibs += this->XCodeEscapePath(li->Value);
        } else if (!li->Target ||
                   li->Target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
          linkLibs += li->Value;
        }
        if (li->Target && !li->Target->IsImported()) {
          target->AddDependTarget(configName, li->Target->GetName());
        }
      }
      this->AppendBuildSettingAttribute(
        target, this->GetTargetLinkFlagsVar(gt), linkLibs.c_str(), configName);
    }
  }
}

bool cmGlobalXCodeGenerator::CreateGroups(
  std::vector<cmLocalGenerator*>& generators)
{
  for (std::vector<cmLocalGenerator*>::iterator i = generators.begin();
       i != generators.end(); ++i) {
    cmMakefile* mf = (*i)->GetMakefile();
    std::vector<cmSourceGroup> sourceGroups = mf->GetSourceGroups();
    std::vector<cmGeneratorTarget*> tgts = (*i)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
         l != tgts.end(); l++) {
      cmGeneratorTarget* gtgt = *l;

      // Same skipping logic here as in CreateXCodeTargets so that we do not
      // end up with (empty anyhow) ALL_BUILD and XCODE_DEPEND_HELPER source
      // groups:
      //
      if (gtgt->GetType() == cmStateEnums::GLOBAL_TARGET) {
        continue;
      }
      if (gtgt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        continue;
      }

      // add the soon to be generated Info.plist file as a source for a
      // MACOSX_BUNDLE file
      if (gtgt->GetPropertyAsBool("MACOSX_BUNDLE")) {
        std::string plist = this->ComputeInfoPListLocation(gtgt);
        mf->GetOrCreateSource(plist, true);
        gtgt->AddSource(plist);
      }

      std::vector<cmGeneratorTarget::AllConfigSource> const& sources =
        gtgt->GetAllConfigSources();

      // Put cmSourceFile instances in proper groups:
      for (std::vector<cmGeneratorTarget::AllConfigSource>::const_iterator si =
             sources.begin();
           si != sources.end(); ++si) {
        cmSourceFile const* sf = si->Source;
        if (this->XcodeVersion >= 50 && !sf->GetObjectLibrary().empty()) {
          // Object library files go on the link line instead.
          continue;
        }
        // Add the file to the list of sources.
        std::string const& source = sf->GetFullPath();
        cmSourceGroup* sourceGroup =
          mf->FindSourceGroup(source.c_str(), sourceGroups);
        cmXCodeObject* pbxgroup = this->CreateOrGetPBXGroup(gtgt, sourceGroup);
        std::string key = GetGroupMapKeyFromPath(gtgt, source);
        this->GroupMap[key] = pbxgroup;
      }
    }
  }
  return true;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreatePBXGroup(cmXCodeObject* parent,
                                                      std::string name)
{
  cmXCodeObject* parentChildren = NULL;
  if (parent)
    parentChildren = parent->GetObject("children");
  cmXCodeObject* group = this->CreateObject(cmXCodeObject::PBXGroup);
  cmXCodeObject* groupChildren =
    this->CreateObject(cmXCodeObject::OBJECT_LIST);
  group->AddAttribute("name", this->CreateString(name));
  group->AddAttribute("children", groupChildren);
  group->AddAttribute("sourceTree", this->CreateString("<group>"));
  if (parentChildren)
    parentChildren->AddObject(group);
  return group;
}

cmXCodeObject* cmGlobalXCodeGenerator::CreateOrGetPBXGroup(
  cmGeneratorTarget* gtgt, cmSourceGroup* sg)
{
  std::string s;
  std::string target;
  const std::string targetFolder = gtgt->GetEffectiveFolderName();
  if (!targetFolder.empty()) {
    target = targetFolder;
    target += "/";
  }
  target += gtgt->GetName();
  s = target + "/";
  s += sg->GetFullName();
  std::map<std::string, cmXCodeObject*>::iterator it =
    this->GroupNameMap.find(s);
  if (it != this->GroupNameMap.end()) {
    return it->second;
  }

  it = this->TargetGroup.find(target);
  cmXCodeObject* tgroup = 0;
  if (it != this->TargetGroup.end()) {
    tgroup = it->second;
  } else {
    std::vector<std::string> tgt_folders =
      cmSystemTools::tokenize(target, "/");
    std::string curr_tgt_folder;
    for (std::vector<std::string>::size_type i = 0; i < tgt_folders.size();
         i++) {
      if (i != 0) {
        curr_tgt_folder += "/";
      }
      curr_tgt_folder += tgt_folders[i];
      it = this->TargetGroup.find(curr_tgt_folder);
      if (it != this->TargetGroup.end()) {
        tgroup = it->second;
        continue;
      }
      tgroup = this->CreatePBXGroup(tgroup, tgt_folders[i]);
      this->TargetGroup[curr_tgt_folder] = tgroup;
      if (i == 0) {
        this->MainGroupChildren->AddObject(tgroup);
      }
    }
  }
  this->TargetGroup[target] = tgroup;

  // If it's the default source group (empty name) then put the source file
  // directly in the tgroup...
  //
  if (std::string(sg->GetFullName()) == "") {
    this->GroupNameMap[s] = tgroup;
    return tgroup;
  }

  // It's a recursive folder structure, let's find the real parent group
  if (std::string(sg->GetFullName()) != std::string(sg->GetName())) {
    std::vector<std::string> folders =
      cmSystemTools::tokenize(sg->GetFullName(), "\\");
    std::string curr_folder = target;
    curr_folder += "/";
    for (std::vector<std::string>::size_type i = 0; i < folders.size(); i++) {
      curr_folder += folders[i];
      std::map<std::string, cmXCodeObject*>::iterator i_folder =
        this->GroupNameMap.find(curr_folder);
      // Create new folder
      if (i_folder == this->GroupNameMap.end()) {
        cmXCodeObject* group = this->CreatePBXGroup(tgroup, folders[i]);
        this->GroupNameMap[curr_folder] = group;
        tgroup = group;
      } else {
        tgroup = i_folder->second;
      }
      curr_folder = curr_folder + "\\";
    }
    return tgroup;
  }
  cmXCodeObject* group = this->CreatePBXGroup(tgroup, sg->GetName());
  this->GroupNameMap[s] = group;
  return group;
}

bool cmGlobalXCodeGenerator::CreateXCodeObjects(
  cmLocalGenerator* root, std::vector<cmLocalGenerator*>& generators)
{
  this->ClearXCodeObjects();
  this->RootObject = 0;
  this->MainGroupChildren = 0;
  cmXCodeObject* group = this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
  group->AddAttribute("COPY_PHASE_STRIP", this->CreateString("NO"));
  cmXCodeObject* listObjs = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  for (unsigned int i = 0; i < this->CurrentConfigurationTypes.size(); ++i) {
    cmXCodeObject* buildStyle =
      this->CreateObject(cmXCodeObject::PBXBuildStyle);
    const char* name = this->CurrentConfigurationTypes[i].c_str();
    buildStyle->AddAttribute("name", this->CreateString(name));
    buildStyle->SetComment(name);
    cmXCodeObject* sgroup = this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
    sgroup->AddAttribute("COPY_PHASE_STRIP", this->CreateString("NO"));
    buildStyle->AddAttribute("buildSettings", sgroup);
    listObjs->AddObject(buildStyle);
  }

  cmXCodeObject* mainGroup = this->CreateObject(cmXCodeObject::PBXGroup);
  this->MainGroupChildren = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  mainGroup->AddAttribute("children", this->MainGroupChildren);
  mainGroup->AddAttribute("sourceTree", this->CreateString("<group>"));

  // now create the cmake groups
  if (!this->CreateGroups(generators)) {
    return false;
  }

  cmXCodeObject* productGroup = this->CreateObject(cmXCodeObject::PBXGroup);
  productGroup->AddAttribute("name", this->CreateString("Products"));
  productGroup->AddAttribute("sourceTree", this->CreateString("<group>"));
  cmXCodeObject* productGroupChildren =
    this->CreateObject(cmXCodeObject::OBJECT_LIST);
  productGroup->AddAttribute("children", productGroupChildren);
  this->MainGroupChildren->AddObject(productGroup);

  this->RootObject = this->CreateObject(cmXCodeObject::PBXProject);
  this->RootObject->SetComment("Project object");

  std::string project_id = "PROJECT_";
  project_id += root->GetProjectName();
  this->RootObject->SetId(
    this->GetOrCreateId(project_id.c_str(), this->RootObject->GetId()));

  group = this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
  this->RootObject->AddAttribute("mainGroup",
                                 this->CreateObjectReference(mainGroup));
  this->RootObject->AddAttribute("buildSettings", group);
  this->RootObject->AddAttribute("buildStyles", listObjs);
  this->RootObject->AddAttribute("hasScannedForEncodings",
                                 this->CreateString("0"));
  group = this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
  group->AddAttribute("BuildIndependentTargetsInParallel",
                      this->CreateString("YES"));
  std::ostringstream v;
  v << std::setfill('0') << std::setw(4) << XcodeVersion * 10;
  group->AddAttribute("LastUpgradeCheck", this->CreateString(v.str()));
  this->RootObject->AddAttribute("attributes", group);
  if (this->XcodeVersion >= 32)
    this->RootObject->AddAttribute("compatibilityVersion",
                                   this->CreateString("Xcode 3.2"));
  else if (this->XcodeVersion >= 31)
    this->RootObject->AddAttribute("compatibilityVersion",
                                   this->CreateString("Xcode 3.1"));
  else
    this->RootObject->AddAttribute("compatibilityVersion",
                                   this->CreateString("Xcode 3.0"));
  // Point Xcode at the top of the source tree.
  {
    std::string pdir =
      this->RelativeToBinary(root->GetCurrentSourceDirectory());
    this->RootObject->AddAttribute("projectDirPath", this->CreateString(pdir));
    this->RootObject->AddAttribute("projectRoot", this->CreateString(""));
  }
  cmXCodeObject* configlist =
    this->CreateObject(cmXCodeObject::XCConfigurationList);
  cmXCodeObject* buildConfigurations =
    this->CreateObject(cmXCodeObject::OBJECT_LIST);
  typedef std::vector<std::pair<std::string, cmXCodeObject*> > Configs;
  Configs configs;
  const char* defaultConfigName = "Debug";
  for (unsigned int i = 0; i < this->CurrentConfigurationTypes.size(); ++i) {
    const char* name = this->CurrentConfigurationTypes[i].c_str();
    if (0 == i) {
      defaultConfigName = name;
    }
    cmXCodeObject* config =
      this->CreateObject(cmXCodeObject::XCBuildConfiguration);
    config->AddAttribute("name", this->CreateString(name));
    configs.push_back(std::make_pair(name, config));
  }
  for (Configs::iterator c = configs.begin(); c != configs.end(); ++c) {
    buildConfigurations->AddObject(c->second);
  }
  configlist->AddAttribute("buildConfigurations", buildConfigurations);

  std::string comment = "Build configuration list for PBXProject";
  comment += " \"";
  comment += this->CurrentProject;
  comment += "\"";
  configlist->SetComment(comment);
  configlist->AddAttribute("defaultConfigurationIsVisible",
                           this->CreateString("0"));
  configlist->AddAttribute("defaultConfigurationName",
                           this->CreateString(defaultConfigName));
  cmXCodeObject* buildSettings =
    this->CreateObject(cmXCodeObject::ATTRIBUTE_GROUP);
  const char* sysroot =
    this->CurrentMakefile->GetDefinition("CMAKE_OSX_SYSROOT");
  const char* deploymentTarget =
    this->CurrentMakefile->GetDefinition("CMAKE_OSX_DEPLOYMENT_TARGET");
  if (sysroot) {
    buildSettings->AddAttribute("SDKROOT", this->CreateString(sysroot));
  }
  // recompute this as it may have been changed since enable language
  this->ComputeArchitectures(this->CurrentMakefile);
  std::string const archs = cmJoin(this->Architectures, " ");
  if (archs.empty()) {
    // Tell Xcode to use NATIVE_ARCH instead of ARCHS.
    buildSettings->AddAttribute("ONLY_ACTIVE_ARCH", this->CreateString("YES"));
  } else {
    // Tell Xcode to use ARCHS (ONLY_ACTIVE_ARCH defaults to NO).
    buildSettings->AddAttribute("ARCHS", this->CreateString(archs));
  }
  if (deploymentTarget && *deploymentTarget) {
    buildSettings->AddAttribute("MACOSX_DEPLOYMENT_TARGET",
                                this->CreateString(deploymentTarget));
  }
  if (!this->GeneratorToolset.empty()) {
    buildSettings->AddAttribute("GCC_VERSION",
                                this->CreateString(this->GeneratorToolset));
  }
  if (this->GetLanguageEnabled("Swift")) {
    std::string swiftVersion;
    if (const char* vers = this->CurrentMakefile->GetDefinition(
          "CMAKE_Swift_LANGUAGE_VERSION")) {
      swiftVersion = vers;
    } else if (this->XcodeVersion >= 83) {
      swiftVersion = "3.0";
    } else {
      swiftVersion = "2.3";
    }
    buildSettings->AddAttribute("SWIFT_VERSION",
                                this->CreateString(swiftVersion));
  }

  std::string symroot = root->GetCurrentBinaryDirectory();
  symroot += "/build";
  buildSettings->AddAttribute("SYMROOT", this->CreateString(symroot));

  for (Configs::iterator i = configs.begin(); i != configs.end(); ++i) {
    cmXCodeObject* buildSettingsForCfg = this->CreateFlatClone(buildSettings);

    // Put this last so it can override existing settings
    // Convert "CMAKE_XCODE_ATTRIBUTE_*" variables directly.
    std::vector<std::string> vars = this->CurrentMakefile->GetDefinitions();
    for (std::vector<std::string>::const_iterator d = vars.begin();
         d != vars.end(); ++d) {
      if (d->find("CMAKE_XCODE_ATTRIBUTE_") == 0) {
        std::string attribute = d->substr(22);
        this->FilterConfigurationAttribute(i->first, attribute);
        if (!attribute.empty()) {
          cmGeneratorExpression ge;
          std::string processed =
            ge.Parse(this->CurrentMakefile->GetDefinition(*d))
              ->Evaluate(this->CurrentLocalGenerator, i->first);
          buildSettingsForCfg->AddAttribute(attribute,
                                            this->CreateString(processed));
        }
      }
    }
    // store per-config buildSettings into configuration object
    i->second->AddAttribute("buildSettings", buildSettingsForCfg);
  }

  this->RootObject->AddAttribute("buildConfigurationList",
                                 this->CreateObjectReference(configlist));

  std::vector<cmXCodeObject*> targets;
  for (std::vector<cmLocalGenerator*>::iterator i = generators.begin();
       i != generators.end(); ++i) {
    if (!this->CreateXCodeTargets(*i, targets)) {
      return false;
    }
  }
  // loop over all targets and add link and depend info
  for (std::vector<cmXCodeObject*>::iterator i = targets.begin();
       i != targets.end(); ++i) {
    cmXCodeObject* t = *i;
    this->AddDependAndLinkInformation(t);
  }
  this->CreateXCodeDependHackTarget(targets);
  // now add all targets to the root object
  cmXCodeObject* allTargets = this->CreateObject(cmXCodeObject::OBJECT_LIST);
  for (std::vector<cmXCodeObject*>::iterator i = targets.begin();
       i != targets.end(); ++i) {
    cmXCodeObject* t = *i;
    allTargets->AddObject(t);
    cmXCodeObject* productRef = t->GetObject("productReference");
    if (productRef) {
      productGroupChildren->AddObject(productRef->GetObject());
    }
  }
  this->RootObject->AddAttribute("targets", allTargets);
  return true;
}

std::string cmGlobalXCodeGenerator::GetObjectsNormalDirectory(
  const std::string& projName, const std::string& configName,
  const cmGeneratorTarget* t) const
{
  std::string dir = t->GetLocalGenerator()->GetCurrentBinaryDirectory();
  dir += "/";
  dir += projName;
  dir += ".build/";
  dir += configName;
  dir += "/";
  dir += t->GetName();
  dir += ".build/Objects-normal/";

  return dir;
}

void cmGlobalXCodeGenerator::ComputeArchitectures(cmMakefile* mf)
{
  this->Architectures.clear();
  const char* osxArch = mf->GetDefinition("CMAKE_OSX_ARCHITECTURES");
  const char* sysroot = mf->GetDefinition("CMAKE_OSX_SYSROOT");
  if (osxArch && sysroot) {
    cmSystemTools::ExpandListArgument(std::string(osxArch),
                                      this->Architectures);
  }

  if (this->Architectures.empty()) {
    // With no ARCHS we use ONLY_ACTIVE_ARCH.
    // Look up the arch that Xcode chooses in this case.
    if (const char* arch = mf->GetDefinition("CMAKE_XCODE_CURRENT_ARCH")) {
      this->ObjectDirArchDefault = arch;
    }
  }

  this->ComputeObjectDirArch(mf);
}

void cmGlobalXCodeGenerator::ComputeObjectDirArch(cmMakefile* mf)
{
  if (this->Architectures.size() > 1 || this->UseEffectivePlatformName(mf)) {
    this->ObjectDirArch = "$(CURRENT_ARCH)";
  } else if (!this->Architectures.empty()) {
    this->ObjectDirArch = this->Architectures[0];
  } else {
    this->ObjectDirArch = this->ObjectDirArchDefault;
  }
}

void cmGlobalXCodeGenerator::CreateXCodeDependHackTarget(
  std::vector<cmXCodeObject*>& targets)
{
  cmGeneratedFileStream makefileStream(this->CurrentXCodeHackMakefile.c_str());
  if (!makefileStream) {
    cmSystemTools::Error("Could not create",
                         this->CurrentXCodeHackMakefile.c_str());
    return;
  }
  makefileStream.SetCopyIfDifferent(true);
  // one more pass for external depend information not handled
  // correctly by xcode
  /* clang-format off */
  makefileStream << "# DO NOT EDIT\n";
  makefileStream << "# This makefile makes sure all linkable targets are\n";
  makefileStream << "# up-to-date with anything they link to\n"
    "default:\n"
    "\techo \"Do not invoke directly\"\n"
    "\n";
  /* clang-format on */

  std::set<std::string> dummyRules;

  // Write rules to help Xcode relink things at the right time.
  /* clang-format off */
  makefileStream <<
    "# Rules to remove targets that are older than anything to which they\n"
    "# link.  This forces Xcode to relink the targets from scratch.  It\n"
    "# does not seem to check these dependencies itself.\n";
  /* clang-format on */
  for (std::vector<std::string>::const_iterator ct =
         this->CurrentConfigurationTypes.begin();
       ct != this->CurrentConfigurationTypes.end(); ++ct) {
    std::string configName = *ct;
    for (std::vector<cmXCodeObject*>::iterator i = targets.begin();
         i != targets.end(); ++i) {
      cmXCodeObject* target = *i;
      cmGeneratorTarget* gt = target->GetTarget();

      if (gt->GetType() == cmStateEnums::EXECUTABLE ||
          gt->GetType() == cmStateEnums::OBJECT_LIBRARY ||
          gt->GetType() == cmStateEnums::STATIC_LIBRARY ||
          gt->GetType() == cmStateEnums::SHARED_LIBRARY ||
          gt->GetType() == cmStateEnums::MODULE_LIBRARY) {
        // Declare an entry point for the target post-build phase.
        makefileStream << this->PostBuildMakeTarget(gt->GetName(), *ct)
                       << ":\n";
      }

      if (gt->GetType() == cmStateEnums::EXECUTABLE ||
          gt->GetType() == cmStateEnums::STATIC_LIBRARY ||
          gt->GetType() == cmStateEnums::SHARED_LIBRARY ||
          gt->GetType() == cmStateEnums::MODULE_LIBRARY) {
        std::string tfull = gt->GetFullPath(configName);
        std::string trel = this->ConvertToRelativeForMake(tfull.c_str());

        // Add this target to the post-build phases of its dependencies.
        std::map<std::string, cmXCodeObject::StringVec>::const_iterator y =
          target->GetDependTargets().find(*ct);
        if (y != target->GetDependTargets().end()) {
          std::vector<std::string> const& deptgts = y->second;
          for (std::vector<std::string>::const_iterator d = deptgts.begin();
               d != deptgts.end(); ++d) {
            makefileStream << this->PostBuildMakeTarget(*d, *ct) << ": "
                           << trel << "\n";
          }
        }

        std::vector<cmGeneratorTarget*> objlibs;
        gt->GetObjectLibrariesCMP0026(objlibs);
        for (std::vector<cmGeneratorTarget*>::const_iterator it =
               objlibs.begin();
             it != objlibs.end(); ++it) {
          makefileStream << this->PostBuildMakeTarget((*it)->GetName(), *ct)
                         << ": " << trel << "\n";
        }

        // Create a rule for this target.
        makefileStream << trel << ":";

        // List dependencies if any exist.
        std::map<std::string, cmXCodeObject::StringVec>::const_iterator x =
          target->GetDependLibraries().find(*ct);
        if (x != target->GetDependLibraries().end()) {
          std::vector<std::string> const& deplibs = x->second;
          for (std::vector<std::string>::const_iterator d = deplibs.begin();
               d != deplibs.end(); ++d) {
            std::string file = this->ConvertToRelativeForMake(d->c_str());
            makefileStream << "\\\n\t" << file;
            dummyRules.insert(file);
          }
        }

        for (std::vector<cmGeneratorTarget*>::const_iterator it =
               objlibs.begin();
             it != objlibs.end(); ++it) {

          const std::string objLibName = (*it)->GetName();
          std::string d = this->GetObjectsNormalDirectory(this->CurrentProject,
                                                          configName, *it);
          d += "lib";
          d += objLibName;
          d += ".a";

          std::string dependency = this->ConvertToRelativeForMake(d.c_str());
          makefileStream << "\\\n\t" << dependency;
          dummyRules.insert(dependency);
        }

        // Write the action to remove the target if it is out of date.
        makefileStream << "\n";
        makefileStream << "\t/bin/rm -f "
                       << this->ConvertToRelativeForMake(tfull.c_str())
                       << "\n";
        // if building for more than one architecture
        // then remove those exectuables as well
        if (this->Architectures.size() > 1) {
          std::string universal = this->GetObjectsNormalDirectory(
            this->CurrentProject, configName, gt);
          for (std::vector<std::string>::iterator arch =
                 this->Architectures.begin();
               arch != this->Architectures.end(); ++arch) {
            std::string universalFile = universal;
            universalFile += *arch;
            universalFile += "/";
            universalFile += gt->GetFullName(configName);
            makefileStream << "\t/bin/rm -f "
                           << this->ConvertToRelativeForMake(
                                universalFile.c_str())
                           << "\n";
          }
        }
        makefileStream << "\n\n";
      }
    }
  }

  makefileStream << "\n\n"
                 << "# For each target create a dummy rule"
                 << "so the target does not have to exist\n";
  for (std::set<std::string>::const_iterator it = dummyRules.begin();
       it != dummyRules.end(); ++it) {
    makefileStream << *it << ":\n";
  }
}

void cmGlobalXCodeGenerator::OutputXCodeProject(
  cmLocalGenerator* root, std::vector<cmLocalGenerator*>& generators)
{
  if (generators.size() == 0) {
    return;
  }
  // Skip local generators that are excluded from this project.
  for (std::vector<cmLocalGenerator*>::iterator g = generators.begin();
       g != generators.end(); ++g) {
    if (this->IsExcluded(root, *g)) {
      continue;
    }
  }

  if (!this->CreateXCodeObjects(root, generators)) {
    return;
  }
  std::string xcodeDir = root->GetCurrentBinaryDirectory();
  xcodeDir += "/";
  xcodeDir += root->GetProjectName();
  xcodeDir += ".xcodeproj";
  cmSystemTools::MakeDirectory(xcodeDir.c_str());
  std::string xcodeProjFile = xcodeDir + "/project.pbxproj";
  cmGeneratedFileStream fout(xcodeProjFile.c_str());
  fout.SetCopyIfDifferent(true);
  if (!fout) {
    return;
  }
  this->WriteXCodePBXProj(fout, root, generators);

  // Since the lowest available Xcode version for testing was 7.0,
  // I'm setting this as a limit then
  if (this->XcodeVersion >= 70) {
    if (root->GetMakefile()->GetCMakeInstance()->GetIsInTryCompile() ||
        root->GetMakefile()->IsOn("CMAKE_XCODE_GENERATE_SCHEME")) {
      this->OutputXCodeSharedSchemes(xcodeDir);
      this->OutputXCodeWorkspaceSettings(xcodeDir);
    }
  }

  this->ClearXCodeObjects();

  // Since this call may have created new cache entries, save the cache:
  //
  root->GetMakefile()->GetCMakeInstance()->SaveCache(
    root->GetBinaryDirectory());
}

void cmGlobalXCodeGenerator::OutputXCodeSharedSchemes(
  const std::string& xcProjDir)
{
  for (std::vector<cmXCodeObject*>::const_iterator i =
         this->XCodeObjects.begin();
       i != this->XCodeObjects.end(); ++i) {
    cmXCodeObject* obj = *i;
    if (obj->GetType() == cmXCodeObject::OBJECT &&
        (obj->GetIsA() == cmXCodeObject::PBXNativeTarget ||
         obj->GetIsA() == cmXCodeObject::PBXAggregateTarget)) {
      cmXCodeScheme schm(obj, this->CurrentConfigurationTypes,
                         this->XcodeVersion);
      schm.WriteXCodeSharedScheme(xcProjDir,
                                  this->RelativeToSource(xcProjDir.c_str()));
    }
  }
}

void cmGlobalXCodeGenerator::OutputXCodeWorkspaceSettings(
  const std::string& xcProjDir)
{
  std::string xcodeSharedDataDir = xcProjDir;
  xcodeSharedDataDir += "/project.xcworkspace/xcshareddata";
  cmSystemTools::MakeDirectory(xcodeSharedDataDir);

  std::string workspaceSettingsFile = xcodeSharedDataDir;
  workspaceSettingsFile += "/WorkspaceSettings.xcsettings";

  cmGeneratedFileStream fout(workspaceSettingsFile.c_str());
  fout.SetCopyIfDifferent(true);
  if (!fout) {
    return;
  }

  cmXMLWriter xout(fout);
  xout.StartDocument();
  xout.Doctype("plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\""
               "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
  xout.StartElement("plist");
  xout.Attribute("version", "1.0");
  xout.StartElement("dict");
  xout.Element("key", "IDEWorkspaceSharedSettings_AutocreateContextsIfNeeded");
  xout.Element("false");
  xout.EndElement(); // dict
  xout.EndElement(); // plist
  xout.EndDocument();
}

void cmGlobalXCodeGenerator::WriteXCodePBXProj(std::ostream& fout,
                                               cmLocalGenerator*,
                                               std::vector<cmLocalGenerator*>&)
{
  SortXCodeObjects();

  fout << "// !$*UTF8*$!\n";
  fout << "{\n";
  cmXCodeObject::Indent(1, fout);
  fout << "archiveVersion = 1;\n";
  cmXCodeObject::Indent(1, fout);
  fout << "classes = {\n";
  cmXCodeObject::Indent(1, fout);
  fout << "};\n";
  cmXCodeObject::Indent(1, fout);
  if (this->XcodeVersion >= 32)
    fout << "objectVersion = 46;\n";
  else if (this->XcodeVersion >= 31)
    fout << "objectVersion = 45;\n";
  else
    fout << "objectVersion = 44;\n";
  cmXCode21Object::PrintList(this->XCodeObjects, fout);
  cmXCodeObject::Indent(1, fout);
  fout << "rootObject = " << this->RootObject->GetId()
       << " /* Project object */;\n";
  fout << "}\n";
}

const char* cmGlobalXCodeGenerator::GetCMakeCFGIntDir() const
{
  return "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)";
}

std::string cmGlobalXCodeGenerator::ExpandCFGIntDir(
  const std::string& str, const std::string& config) const
{
  std::string replace1 = "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)";
  std::string replace2 = "$(CONFIGURATION)";

  std::string tmp = str;
  for (std::string::size_type i = tmp.find(replace1); i != std::string::npos;
       i = tmp.find(replace1, i)) {
    tmp.replace(i, replace1.size(), config);
    i += config.size();
  }
  for (std::string::size_type i = tmp.find(replace2); i != std::string::npos;
       i = tmp.find(replace2, i)) {
    tmp.replace(i, replace2.size(), config);
    i += config.size();
  }
  return tmp;
}

void cmGlobalXCodeGenerator::GetDocumentation(cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalXCodeGenerator::GetActualName();
  entry.Brief = "Generate Xcode project files.";
}

std::string cmGlobalXCodeGenerator::ConvertToRelativeForMake(const char* p)
{
  return cmSystemTools::ConvertToOutputPath(p);
}

std::string cmGlobalXCodeGenerator::RelativeToSource(const char* p)
{
  // We force conversion because Xcode breakpoints do not work unless
  // they are in a file named relative to the source tree.
  return cmOutputConverter::ForceToRelativePath(
    cmSystemTools::JoinPath(this->ProjectSourceDirectoryComponents), p);
}

std::string cmGlobalXCodeGenerator::RelativeToBinary(const char* p)
{
  return this->CurrentLocalGenerator->ConvertToRelativePath(
    cmSystemTools::JoinPath(this->ProjectOutputDirectoryComponents), p);
}

std::string cmGlobalXCodeGenerator::XCodeEscapePath(const std::string& p)
{
  if (p.find(' ') != std::string::npos) {
    std::string t = "\"";
    t += p;
    t += "\"";
    return t;
  }
  return p;
}

void cmGlobalXCodeGenerator::AppendDirectoryForConfig(
  const std::string& prefix, const std::string& config,
  const std::string& suffix, std::string& dir)
{
  if (!config.empty()) {
    dir += prefix;
    dir += config;
    dir += suffix;
  }
}

std::string cmGlobalXCodeGenerator::LookupFlags(
  const std::string& varNamePrefix, const std::string& varNameLang,
  const std::string& varNameSuffix, const std::string& default_flags)
{
  if (!varNameLang.empty()) {
    std::string varName = varNamePrefix;
    varName += varNameLang;
    varName += varNameSuffix;
    if (const char* varValue = this->CurrentMakefile->GetDefinition(varName)) {
      if (*varValue) {
        return varValue;
      }
    }
  }
  return default_flags;
}

void cmGlobalXCodeGenerator::AppendDefines(BuildObjectListOrString& defs,
                                           const char* defines_list,
                                           bool dflag)
{
  // Skip this if there are no definitions.
  if (!defines_list) {
    return;
  }

  // Expand the list of definitions.
  std::vector<std::string> defines;
  cmSystemTools::ExpandListArgument(defines_list, defines);

  // Store the definitions in the string.
  this->AppendDefines(defs, defines, dflag);
}

void cmGlobalXCodeGenerator::AppendDefines(
  BuildObjectListOrString& defs, std::vector<std::string> const& defines,
  bool dflag)
{
  // GCC_PREPROCESSOR_DEFINITIONS is a space-separated list of definitions.
  std::string def;
  for (std::vector<std::string>::const_iterator di = defines.begin();
       di != defines.end(); ++di) {
    // Start with -D if requested.
    def = dflag ? "-D" : "";
    def += *di;

    // Append the flag with needed escapes.
    std::string tmp;
    this->AppendFlag(tmp, def);
    defs.Add(tmp);
  }
}

void cmGlobalXCodeGenerator::AppendFlag(std::string& flags,
                                        std::string const& flag)
{
  // Short-circuit for an empty flag.
  if (flag.empty()) {
    return;
  }

  // Separate from previous flags.
  if (!flags.empty()) {
    flags += " ";
  }

  // Check if the flag needs quoting.
  bool quoteFlag =
    flag.find_first_of("`~!@#$%^&*()+={}[]|:;\"'<>,.? ") != std::string::npos;

  // We escape a flag as follows:
  //   - Place each flag in single quotes ''
  //   - Escape a single quote as \'
  //   - Escape a backslash as \\ since it itself is an escape
  // Note that in the code below we need one more level of escapes for
  // C string syntax in this source file.
  //
  // The final level of escaping is done when the string is stored
  // into the project file by cmXCodeObject::PrintString.

  if (quoteFlag) {
    // Open single quote.
    flags += "'";
  }

  // Flag value with escaped quotes and backslashes.
  for (const char* c = flag.c_str(); *c; ++c) {
    if (*c == '\'') {
      if (this->XcodeVersion >= 40) {
        flags += "'\\''";
      } else {
        flags += "\\'";
      }
    } else if (*c == '\\') {
      flags += "\\\\";
    } else {
      flags += *c;
    }
  }

  if (quoteFlag) {
    // Close single quote.
    flags += "'";
  }
}

std::string cmGlobalXCodeGenerator::ComputeInfoPListLocation(
  cmGeneratorTarget* target)
{
  std::string plist = target->GetLocalGenerator()->GetCurrentBinaryDirectory();
  plist += cmake::GetCMakeFilesDirectory();
  plist += "/";
  plist += target->GetName();
  plist += ".dir/Info.plist";
  return plist;
}

// Return true if the generated build tree may contain multiple builds.
// i.e. "Can I build Debug and Release in the same tree?"
bool cmGlobalXCodeGenerator::IsMultiConfig() const
{
  // Newer Xcode versions are multi config:
  return true;
}

bool cmGlobalXCodeGenerator::HasKnownObjectFileLocation(
  std::string* reason) const
{
  if (this->ObjectDirArch.find('$') != std::string::npos) {
    if (reason != CM_NULLPTR) {
      *reason = " under Xcode with multiple architectures";
    }
    return false;
  }
  return true;
}

bool cmGlobalXCodeGenerator::UseEffectivePlatformName(cmMakefile* mf) const
{
  const char* epnValue =
    this->GetCMakeInstance()->GetState()->GetGlobalProperty(
      "XCODE_EMIT_EFFECTIVE_PLATFORM_NAME");

  if (!epnValue) {
    return mf->PlatformIsAppleIos();
  }

  return cmSystemTools::IsOn(epnValue);
}

bool cmGlobalXCodeGenerator::ShouldStripResourcePath(cmMakefile*) const
{
  // Xcode determines Resource location itself
  return true;
}

void cmGlobalXCodeGenerator::ComputeTargetObjectDirectory(
  cmGeneratorTarget* gt) const
{
  std::string configName = this->GetCMakeCFGIntDir();
  std::string dir =
    this->GetObjectsNormalDirectory("$(PROJECT_NAME)", configName, gt);
  dir += this->ObjectDirArch;
  dir += "/";
  gt->ObjectDirectory = dir;
}
