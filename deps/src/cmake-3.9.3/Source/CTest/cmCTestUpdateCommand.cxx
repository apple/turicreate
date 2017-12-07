/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestUpdateCommand.h"

#include "cmCTest.h"
#include "cmCTestGenericHandler.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

#include <vector>

cmCTestGenericHandler* cmCTestUpdateCommand::InitializeHandler()
{
  if (this->Values[ct_SOURCE]) {
    this->CTest->SetCTestConfiguration(
      "SourceDirectory",
      cmSystemTools::CollapseFullPath(this->Values[ct_SOURCE]).c_str(),
      this->Quiet);
  } else {
    this->CTest->SetCTestConfiguration(
      "SourceDirectory",
      cmSystemTools::CollapseFullPath(
        this->Makefile->GetDefinition("CTEST_SOURCE_DIRECTORY"))
        .c_str(),
      this->Quiet);
  }
  std::string source_dir =
    this->CTest->GetCTestConfiguration("SourceDirectory");

  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "UpdateCommand", "CTEST_UPDATE_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "UpdateOptions", "CTEST_UPDATE_OPTIONS", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "CVSCommand", "CTEST_CVS_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "CVSUpdateOptions", "CTEST_CVS_UPDATE_OPTIONS",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "SVNCommand", "CTEST_SVN_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "SVNUpdateOptions", "CTEST_SVN_UPDATE_OPTIONS",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "SVNOptions", "CTEST_SVN_OPTIONS", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "BZRCommand", "CTEST_BZR_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "BZRUpdateOptions", "CTEST_BZR_UPDATE_OPTIONS",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "GITCommand", "CTEST_GIT_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "GITUpdateOptions", "CTEST_GIT_UPDATE_OPTIONS",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "GITInitSubmodules", "CTEST_GIT_INIT_SUBMODULES",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "GITUpdateCustom", "CTEST_GIT_UPDATE_CUSTOM", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "UpdateVersionOnly", "CTEST_UPDATE_VERSION_ONLY",
    this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "HGCommand", "CTEST_HG_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "HGUpdateOptions", "CTEST_HG_UPDATE_OPTIONS", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "P4Command", "CTEST_P4_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "P4UpdateOptions", "CTEST_P4_UPDATE_OPTIONS", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "P4Client", "CTEST_P4_CLIENT", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "P4Options", "CTEST_P4_OPTIONS", this->Quiet);

  cmCTestGenericHandler* handler =
    this->CTest->GetInitializedHandler("update");
  if (!handler) {
    this->SetError("internal CTest error. Cannot instantiate update handler");
    return CM_NULLPTR;
  }
  handler->SetCommand(this);
  if (source_dir.empty()) {
    this->SetError("source directory not specified. Please use SOURCE tag");
    return CM_NULLPTR;
  }
  handler->SetOption("SourceDirectory", source_dir.c_str());
  handler->SetQuiet(this->Quiet);
  return handler;
}
