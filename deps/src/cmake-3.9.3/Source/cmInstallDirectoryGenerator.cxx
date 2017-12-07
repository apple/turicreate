/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallDirectoryGenerator.h"

#include "cmGeneratorExpression.h"
#include "cmInstallType.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"

cmInstallDirectoryGenerator::cmInstallDirectoryGenerator(
  std::vector<std::string> const& dirs, const char* dest,
  const char* file_permissions, const char* dir_permissions,
  std::vector<std::string> const& configurations, const char* component,
  MessageLevel message, bool exclude_from_all, const char* literal_args,
  bool optional)
  : cmInstallGenerator(dest, configurations, component, message,
                       exclude_from_all)
  , LocalGenerator(CM_NULLPTR)
  , Directories(dirs)
  , FilePermissions(file_permissions)
  , DirPermissions(dir_permissions)
  , LiteralArguments(literal_args)
  , Optional(optional)
{
  // We need per-config actions if destination have generator expressions.
  if (cmGeneratorExpression::Find(Destination) != std::string::npos) {
    this->ActionsPerConfig = true;
  }

  // We need per-config actions if any directories have generator expressions.
  for (std::vector<std::string>::const_iterator i = dirs.begin();
       !this->ActionsPerConfig && i != dirs.end(); ++i) {
    if (cmGeneratorExpression::Find(*i) != std::string::npos) {
      this->ActionsPerConfig = true;
    }
  }
}

cmInstallDirectoryGenerator::~cmInstallDirectoryGenerator()
{
}

void cmInstallDirectoryGenerator::Compute(cmLocalGenerator* lg)
{
  LocalGenerator = lg;
}

void cmInstallDirectoryGenerator::GenerateScriptActions(std::ostream& os,
                                                        Indent indent)
{
  if (this->ActionsPerConfig) {
    this->cmInstallGenerator::GenerateScriptActions(os, indent);
  } else {
    this->AddDirectoryInstallRule(os, "", indent, this->Directories);
  }
}

void cmInstallDirectoryGenerator::GenerateScriptForConfig(
  std::ostream& os, const std::string& config, Indent indent)
{
  std::vector<std::string> dirs;
  cmGeneratorExpression ge;
  for (std::vector<std::string>::const_iterator i = this->Directories.begin();
       i != this->Directories.end(); ++i) {
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(*i);
    cmSystemTools::ExpandListArgument(
      cge->Evaluate(this->LocalGenerator, config), dirs);
  }

  // Make sure all dirs have absolute paths.
  cmMakefile const& mf = *this->LocalGenerator->GetMakefile();
  for (std::vector<std::string>::iterator i = dirs.begin(); i != dirs.end();
       ++i) {
    if (!cmSystemTools::FileIsFullPath(i->c_str())) {
      *i = std::string(mf.GetCurrentSourceDirectory()) + "/" + *i;
    }
  }

  this->AddDirectoryInstallRule(os, config, indent, dirs);
}

void cmInstallDirectoryGenerator::AddDirectoryInstallRule(
  std::ostream& os, const std::string& config, Indent indent,
  std::vector<std::string> const& dirs)
{
  // Write code to install the directories.
  const char* no_rename = CM_NULLPTR;
  this->AddInstallRule(os, this->GetDestination(config),
                       cmInstallType_DIRECTORY, dirs, this->Optional,
                       this->FilePermissions.c_str(),
                       this->DirPermissions.c_str(), no_rename,
                       this->LiteralArguments.c_str(), indent);
}

std::string cmInstallDirectoryGenerator::GetDestination(
  std::string const& config) const
{
  cmGeneratorExpression ge;
  return ge.Parse(this->Destination)->Evaluate(this->LocalGenerator, config);
}
