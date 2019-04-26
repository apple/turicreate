/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmOSXBundleGenerator.h"

#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"

#include <cassert>

class cmSourceFile;

cmOSXBundleGenerator::cmOSXBundleGenerator(cmGeneratorTarget* target,
                                           const std::string& configName)
  : GT(target)
  , Makefile(target->Target->GetMakefile())
  , LocalGenerator(target->GetLocalGenerator())
  , ConfigName(configName)
  , MacContentFolders(nullptr)
{
  if (this->MustSkip()) {
    return;
  }
}

bool cmOSXBundleGenerator::MustSkip()
{
  return !this->GT->HaveWellDefinedOutputFiles();
}

void cmOSXBundleGenerator::CreateAppBundle(const std::string& targetName,
                                           std::string& outpath)
{
  if (this->MustSkip()) {
    return;
  }

  // Compute bundle directory names.
  std::string out = outpath;
  out += "/";
  out += this->GT->GetAppBundleDirectory(this->ConfigName,
                                         cmGeneratorTarget::FullLevel);
  cmSystemTools::MakeDirectory(out);
  this->Makefile->AddCMakeOutputFile(out);

  // Configure the Info.plist file.  Note that it needs the executable name
  // to be set.
  std::string plist = outpath;
  plist += "/";
  plist += this->GT->GetAppBundleDirectory(this->ConfigName,
                                           cmGeneratorTarget::ContentLevel);
  plist += "/Info.plist";
  this->LocalGenerator->GenerateAppleInfoPList(this->GT, targetName,
                                               plist.c_str());
  this->Makefile->AddCMakeOutputFile(plist);
  outpath = out;
}

void cmOSXBundleGenerator::CreateFramework(const std::string& targetName,
                                           const std::string& outpath)
{
  if (this->MustSkip()) {
    return;
  }

  assert(this->MacContentFolders);

  // Compute the location of the top-level foo.framework directory.
  std::string contentdir = outpath + "/" +
    this->GT->GetFrameworkDirectory(this->ConfigName,
                                    cmGeneratorTarget::ContentLevel);
  contentdir += "/";

  std::string newoutpath = outpath + "/" +
    this->GT->GetFrameworkDirectory(this->ConfigName,
                                    cmGeneratorTarget::FullLevel);

  std::string frameworkVersion = this->GT->GetFrameworkVersion();

  // Configure the Info.plist file
  std::string plist = newoutpath;
  if (!this->Makefile->PlatformIsAppleEmbedded()) {
    // Put the Info.plist file into the Resources directory.
    this->MacContentFolders->insert("Resources");
    plist += "/Resources";
  }
  plist += "/Info.plist";
  std::string name = cmSystemTools::GetFilenameName(targetName);
  this->LocalGenerator->GenerateFrameworkInfoPList(this->GT, name,
                                                   plist.c_str());

  // Generate Versions directory only for MacOSX frameworks
  if (this->Makefile->PlatformIsAppleEmbedded()) {
    return;
  }

  // TODO: Use the cmMakefileTargetGenerator::ExtraFiles vector to
  // drive rules to create these files at build time.
  std::string oldName;
  std::string newName;

  // Make foo.framework/Versions
  std::string versions = contentdir;
  versions += "Versions";
  cmSystemTools::MakeDirectory(versions);

  // Make foo.framework/Versions/version
  cmSystemTools::MakeDirectory(newoutpath);

  // Current -> version
  oldName = frameworkVersion;
  newName = versions;
  newName += "/Current";
  cmSystemTools::RemoveFile(newName);
  cmSystemTools::CreateSymlink(oldName, newName);
  this->Makefile->AddCMakeOutputFile(newName);

  // foo -> Versions/Current/foo
  oldName = "Versions/Current/";
  oldName += name;
  newName = contentdir;
  newName += name;
  cmSystemTools::RemoveFile(newName);
  cmSystemTools::CreateSymlink(oldName, newName);
  this->Makefile->AddCMakeOutputFile(newName);

  // Resources -> Versions/Current/Resources
  if (this->MacContentFolders->find("Resources") !=
      this->MacContentFolders->end()) {
    oldName = "Versions/Current/Resources";
    newName = contentdir;
    newName += "Resources";
    cmSystemTools::RemoveFile(newName);
    cmSystemTools::CreateSymlink(oldName, newName);
    this->Makefile->AddCMakeOutputFile(newName);
  }

  // Headers -> Versions/Current/Headers
  if (this->MacContentFolders->find("Headers") !=
      this->MacContentFolders->end()) {
    oldName = "Versions/Current/Headers";
    newName = contentdir;
    newName += "Headers";
    cmSystemTools::RemoveFile(newName);
    cmSystemTools::CreateSymlink(oldName, newName);
    this->Makefile->AddCMakeOutputFile(newName);
  }

  // PrivateHeaders -> Versions/Current/PrivateHeaders
  if (this->MacContentFolders->find("PrivateHeaders") !=
      this->MacContentFolders->end()) {
    oldName = "Versions/Current/PrivateHeaders";
    newName = contentdir;
    newName += "PrivateHeaders";
    cmSystemTools::RemoveFile(newName);
    cmSystemTools::CreateSymlink(oldName, newName);
    this->Makefile->AddCMakeOutputFile(newName);
  }
}

void cmOSXBundleGenerator::CreateCFBundle(const std::string& targetName,
                                          const std::string& root)
{
  if (this->MustSkip()) {
    return;
  }

  // Compute bundle directory names.
  std::string out = root;
  out += "/";
  out += this->GT->GetCFBundleDirectory(this->ConfigName,
                                        cmGeneratorTarget::FullLevel);
  cmSystemTools::MakeDirectory(out);
  this->Makefile->AddCMakeOutputFile(out);

  // Configure the Info.plist file.  Note that it needs the executable name
  // to be set.
  std::string plist = root + "/" +
    this->GT->GetCFBundleDirectory(this->ConfigName,
                                   cmGeneratorTarget::ContentLevel);
  plist += "/Info.plist";
  std::string name = cmSystemTools::GetFilenameName(targetName);
  this->LocalGenerator->GenerateAppleInfoPList(this->GT, name, plist.c_str());
  this->Makefile->AddCMakeOutputFile(plist);
}

void cmOSXBundleGenerator::GenerateMacOSXContentStatements(
  std::vector<cmSourceFile const*> const& sources,
  MacOSXContentGeneratorType* generator)
{
  if (this->MustSkip()) {
    return;
  }

  for (cmSourceFile const* source : sources) {
    cmGeneratorTarget::SourceFileFlags tsFlags =
      this->GT->GetTargetSourceFileFlags(source);
    if (tsFlags.Type != cmGeneratorTarget::SourceFileTypeNormal) {
      (*generator)(*source, tsFlags.MacFolder);
    }
  }
}

std::string cmOSXBundleGenerator::InitMacOSXContentDirectory(
  const char* pkgloc)
{
  // Construct the full path to the content subdirectory.

  std::string macdir = this->GT->GetMacContentDirectory(
    this->ConfigName, cmStateEnums::RuntimeBinaryArtifact);
  macdir += "/";
  macdir += pkgloc;
  cmSystemTools::MakeDirectory(macdir);

  // Record use of this content location.  Only the first level
  // directory is needed.
  {
    std::string loc = pkgloc;
    loc = loc.substr(0, loc.find('/'));
    this->MacContentFolders->insert(loc);
  }

  return macdir;
}
