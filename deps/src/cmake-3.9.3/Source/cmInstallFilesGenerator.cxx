/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallFilesGenerator.h"

#include "cmGeneratorExpression.h"
#include "cmInstallType.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"

class cmLocalGenerator;

cmInstallFilesGenerator::cmInstallFilesGenerator(
  std::vector<std::string> const& files, const char* dest, bool programs,
  const char* file_permissions, std::vector<std::string> const& configurations,
  const char* component, MessageLevel message, bool exclude_from_all,
  const char* rename, bool optional)
  : cmInstallGenerator(dest, configurations, component, message,
                       exclude_from_all)
  , LocalGenerator(CM_NULLPTR)
  , Files(files)
  , FilePermissions(file_permissions)
  , Rename(rename)
  , Programs(programs)
  , Optional(optional)
{
  // We need per-config actions if the destination has generator expressions.
  if (cmGeneratorExpression::Find(Destination) != std::string::npos) {
    this->ActionsPerConfig = true;
  }

  // We need per-config actions if any files have generator expressions.
  for (std::vector<std::string>::const_iterator i = files.begin();
       !this->ActionsPerConfig && i != files.end(); ++i) {
    if (cmGeneratorExpression::Find(*i) != std::string::npos) {
      this->ActionsPerConfig = true;
    }
  }
}

cmInstallFilesGenerator::~cmInstallFilesGenerator()
{
}

void cmInstallFilesGenerator::Compute(cmLocalGenerator* lg)
{
  this->LocalGenerator = lg;
}

std::string cmInstallFilesGenerator::GetDestination(
  std::string const& config) const
{
  cmGeneratorExpression ge;
  return ge.Parse(this->Destination)->Evaluate(this->LocalGenerator, config);
}

void cmInstallFilesGenerator::AddFilesInstallRule(
  std::ostream& os, std::string const& config, Indent indent,
  std::vector<std::string> const& files)
{
  // Write code to install the files.
  const char* no_dir_permissions = CM_NULLPTR;
  this->AddInstallRule(
    os, this->GetDestination(config),
    (this->Programs ? cmInstallType_PROGRAMS : cmInstallType_FILES), files,
    this->Optional, this->FilePermissions.c_str(), no_dir_permissions,
    this->Rename.c_str(), CM_NULLPTR, indent);
}

void cmInstallFilesGenerator::GenerateScriptActions(std::ostream& os,
                                                    Indent indent)
{
  if (this->ActionsPerConfig) {
    this->cmInstallGenerator::GenerateScriptActions(os, indent);
  } else {
    this->AddFilesInstallRule(os, "", indent, this->Files);
  }
}

void cmInstallFilesGenerator::GenerateScriptForConfig(
  std::ostream& os, const std::string& config, Indent indent)
{
  std::vector<std::string> files;
  cmGeneratorExpression ge;
  for (std::vector<std::string>::const_iterator i = this->Files.begin();
       i != this->Files.end(); ++i) {
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(*i);
    cmSystemTools::ExpandListArgument(
      cge->Evaluate(this->LocalGenerator, config), files);
  }
  this->AddFilesInstallRule(os, config, indent, files);
}
