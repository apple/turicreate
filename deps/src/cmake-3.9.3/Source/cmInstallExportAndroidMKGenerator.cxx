
/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallExportAndroidMKGenerator.h"

#include <stdio.h>

#include "cmExportInstallFileGenerator.h"
#include "cmExportSet.h"
#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmInstallFilesGenerator.h"
#include "cmInstallTargetGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmake.h"

cmInstallExportAndroidMKGenerator::cmInstallExportAndroidMKGenerator(
  cmExportSet* exportSet, const char* destination,
  const char* file_permissions, std::vector<std::string> const& configurations,
  const char* component, MessageLevel message, bool exclude_from_all,
  const char* filename, const char* name_space, bool exportOld)
  : cmInstallExportGenerator(exportSet, destination, file_permissions,
                             configurations, component, message,
                             exclude_from_all, filename, name_space, exportOld)
{
}

cmInstallExportAndroidMKGenerator::~cmInstallExportAndroidMKGenerator()
{
}

void cmInstallExportAndroidMKGenerator::Compute(cmLocalGenerator* lg)
{
  this->LocalGenerator = lg;
  this->ExportSet->Compute(lg);
}

void cmInstallExportAndroidMKGenerator::GenerateScript(std::ostream& os)
{
  // Skip empty sets.
  if (ExportSet->GetTargetExports()->empty()) {
    std::ostringstream e;
    e << "INSTALL(EXPORT) given unknown export \"" << ExportSet->GetName()
      << "\"";
    cmSystemTools::Error(e.str().c_str());
    return;
  }

  // Create the temporary directory in which to store the files.
  this->ComputeTempDir();
  cmSystemTools::MakeDirectory(this->TempDir.c_str());

  // Construct a temporary location for the file.
  this->MainImportFile = this->TempDir;
  this->MainImportFile += "/";
  this->MainImportFile += this->FileName;

  // Generate the import file for this export set.
  this->EFGen->SetExportFile(this->MainImportFile.c_str());
  this->EFGen->SetNamespace(this->Namespace);
  this->EFGen->SetExportOld(this->ExportOld);
  if (this->ConfigurationTypes->empty()) {
    if (!this->ConfigurationName.empty()) {
      this->EFGen->AddConfiguration(this->ConfigurationName);
    } else {
      this->EFGen->AddConfiguration("");
    }
  } else {
    for (std::vector<std::string>::const_iterator ci =
           this->ConfigurationTypes->begin();
         ci != this->ConfigurationTypes->end(); ++ci) {
      this->EFGen->AddConfiguration(*ci);
    }
  }
  this->EFGen->GenerateImportFile();

  // Perform the main install script generation.
  this->cmInstallGenerator::GenerateScript(os);
}

void cmInstallExportAndroidMKGenerator::GenerateScriptConfigs(
  std::ostream& os, Indent const& indent)
{
  // Create the main install rules first.
  this->cmInstallGenerator::GenerateScriptConfigs(os, indent);

  // Now create a configuration-specific install rule for the import
  // file of each configuration.
  std::vector<std::string> files;
  for (std::map<std::string, std::string>::const_iterator i =
         this->EFGen->GetConfigImportFiles().begin();
       i != this->EFGen->GetConfigImportFiles().end(); ++i) {
    files.push_back(i->second);
    std::string config_test = this->CreateConfigTest(i->first);
    os << indent << "if(" << config_test << ")\n";
    this->AddInstallRule(os, this->Destination, cmInstallType_FILES, files,
                         false, this->FilePermissions.c_str(), CM_NULLPTR,
                         CM_NULLPTR, CM_NULLPTR, indent.Next());
    os << indent << "endif()\n";
    files.clear();
  }
}

void cmInstallExportAndroidMKGenerator::GenerateScriptActions(
  std::ostream& os, Indent const& indent)
{
  // Remove old per-configuration export files if the main changes.
  std::string installedDir = "$ENV{DESTDIR}";
  installedDir += this->ConvertToAbsoluteDestination(this->Destination);
  installedDir += "/";
  std::string installedFile = installedDir;
  installedFile += this->FileName;
  os << indent << "if(EXISTS \"" << installedFile << "\")\n";
  Indent indentN = indent.Next();
  Indent indentNN = indentN.Next();
  Indent indentNNN = indentNN.Next();
  /* clang-format off */
  os << indentN << "file(DIFFERENT EXPORT_FILE_CHANGED FILES\n"
     << indentN << "     \"" << installedFile << "\"\n"
     << indentN << "     \"" << this->MainImportFile << "\")\n";
  os << indentN << "if(EXPORT_FILE_CHANGED)\n";
  os << indentNN << "file(GLOB OLD_CONFIG_FILES \"" << installedDir
     << this->EFGen->GetConfigImportFileGlob() << "\")\n";
  os << indentNN << "if(OLD_CONFIG_FILES)\n";
  os << indentNNN << "message(STATUS \"Old export file \\\"" << installedFile
     << "\\\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].\")\n";
  os << indentNNN << "file(REMOVE ${OLD_CONFIG_FILES})\n";
  os << indentNN << "endif()\n";
  os << indentN << "endif()\n";
  os << indent << "endif()\n";
  /* clang-format on */

  // Install the main export file.
  std::vector<std::string> files;
  files.push_back(this->MainImportFile);
  this->AddInstallRule(os, this->Destination, cmInstallType_FILES, files,
                       false, this->FilePermissions.c_str(), CM_NULLPTR,
                       CM_NULLPTR, CM_NULLPTR, indent);
}
