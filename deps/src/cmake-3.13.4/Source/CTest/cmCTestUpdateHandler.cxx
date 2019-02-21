/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestUpdateHandler.h"

#include "cmAlgorithms.h"
#include "cmCLocaleEnvironmentScope.h"
#include "cmCTest.h"
#include "cmCTestBZR.h"
#include "cmCTestCVS.h"
#include "cmCTestGIT.h"
#include "cmCTestHG.h"
#include "cmCTestP4.h"
#include "cmCTestSVN.h"
#include "cmCTestVC.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmXMLWriter.h"

#include <chrono>
#include <memory> // IWYU pragma: keep
#include <sstream>

static const char* cmCTestUpdateHandlerUpdateStrings[] = {
  "Unknown", "CVS", "SVN", "BZR", "GIT", "HG", "P4"
};

static const char* cmCTestUpdateHandlerUpdateToString(int type)
{
  if (type < cmCTestUpdateHandler::e_UNKNOWN ||
      type >= cmCTestUpdateHandler::e_LAST) {
    return cmCTestUpdateHandlerUpdateStrings[cmCTestUpdateHandler::e_UNKNOWN];
  }
  return cmCTestUpdateHandlerUpdateStrings[type];
}

cmCTestUpdateHandler::cmCTestUpdateHandler()
{
}

void cmCTestUpdateHandler::Initialize()
{
  this->Superclass::Initialize();
  this->UpdateCommand.clear();
  this->UpdateType = e_CVS;
}

int cmCTestUpdateHandler::DetermineType(const char* cmd, const char* type)
{
  cmCTestOptionalLog(this->CTest, DEBUG,
                     "Determine update type from command: "
                       << cmd << " and type: " << type << std::endl,
                     this->Quiet);
  if (type && *type) {
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Type specified: " << type << std::endl, this->Quiet);
    std::string stype = cmSystemTools::LowerCase(type);
    if (stype.find("cvs") != std::string::npos) {
      return cmCTestUpdateHandler::e_CVS;
    }
    if (stype.find("svn") != std::string::npos) {
      return cmCTestUpdateHandler::e_SVN;
    }
    if (stype.find("bzr") != std::string::npos) {
      return cmCTestUpdateHandler::e_BZR;
    }
    if (stype.find("git") != std::string::npos) {
      return cmCTestUpdateHandler::e_GIT;
    }
    if (stype.find("hg") != std::string::npos) {
      return cmCTestUpdateHandler::e_HG;
    }
    if (stype.find("p4") != std::string::npos) {
      return cmCTestUpdateHandler::e_P4;
    }
  } else {
    cmCTestOptionalLog(
      this->CTest, DEBUG,
      "Type not specified, check command: " << cmd << std::endl, this->Quiet);
    std::string stype = cmSystemTools::LowerCase(cmd);
    if (stype.find("cvs") != std::string::npos) {
      return cmCTestUpdateHandler::e_CVS;
    }
    if (stype.find("svn") != std::string::npos) {
      return cmCTestUpdateHandler::e_SVN;
    }
    if (stype.find("bzr") != std::string::npos) {
      return cmCTestUpdateHandler::e_BZR;
    }
    if (stype.find("git") != std::string::npos) {
      return cmCTestUpdateHandler::e_GIT;
    }
    if (stype.find("hg") != std::string::npos) {
      return cmCTestUpdateHandler::e_HG;
    }
    if (stype.find("p4") != std::string::npos) {
      return cmCTestUpdateHandler::e_P4;
    }
  }
  return cmCTestUpdateHandler::e_UNKNOWN;
}

// clearly it would be nice if this were broken up into a few smaller
// functions and commented...
int cmCTestUpdateHandler::ProcessHandler()
{
  // Make sure VCS tool messages are in English so we can parse them.
  cmCLocaleEnvironmentScope fixLocale;
  static_cast<void>(fixLocale);

  // Get source dir
  const char* sourceDirectory = this->GetOption("SourceDirectory");
  if (!sourceDirectory) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot find SourceDirectory  key in the DartConfiguration.tcl"
                 << std::endl);
    return -1;
  }

  cmGeneratedFileStream ofs;
  if (!this->CTest->GetShowOnly()) {
    this->StartLogFile("Update", ofs);
  }

  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                     "   Updating the repository: " << sourceDirectory
                                                    << std::endl,
                     this->Quiet);

  if (!this->SelectVCS()) {
    return -1;
  }

  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                     "   Use "
                       << cmCTestUpdateHandlerUpdateToString(this->UpdateType)
                       << " repository type" << std::endl;
                     , this->Quiet);

  // Create an object to interact with the VCS tool.
  std::unique_ptr<cmCTestVC> vc;
  switch (this->UpdateType) {
    case e_CVS:
      vc = cm::make_unique<cmCTestCVS>(this->CTest, ofs);
      break;
    case e_SVN:
      vc = cm::make_unique<cmCTestSVN>(this->CTest, ofs);
      break;
    case e_BZR:
      vc = cm::make_unique<cmCTestBZR>(this->CTest, ofs);
      break;
    case e_GIT:
      vc = cm::make_unique<cmCTestGIT>(this->CTest, ofs);
      break;
    case e_HG:
      vc = cm::make_unique<cmCTestHG>(this->CTest, ofs);
      break;
    case e_P4:
      vc = cm::make_unique<cmCTestP4>(this->CTest, ofs);
      break;
    default:
      vc = cm::make_unique<cmCTestVC>(this->CTest, ofs);
      break;
  }
  vc->SetCommandLineTool(this->UpdateCommand);
  vc->SetSourceDirectory(sourceDirectory);

  // Cleanup the working tree.
  vc->Cleanup();

  //
  // Now update repository and remember what files were updated
  //
  cmGeneratedFileStream os;
  if (!this->StartResultingXML(cmCTest::PartUpdate, "Update", os)) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot open log file" << std::endl);
    return -1;
  }
  std::string start_time = this->CTest->CurrentTime();
  auto start_time_time = std::chrono::system_clock::now();
  auto elapsed_time_start = std::chrono::steady_clock::now();

  bool updated = vc->Update();
  std::string buildname =
    cmCTest::SafeBuildIdField(this->CTest->GetCTestConfiguration("BuildName"));

  cmXMLWriter xml(os);
  xml.StartDocument();
  xml.StartElement("Update");
  xml.Attribute("mode", "Client");
  xml.Attribute("Generator",
                std::string("ctest-") + cmVersion::GetCMakeVersion());
  xml.Element("Site", this->CTest->GetCTestConfiguration("Site"));
  xml.Element("BuildName", buildname);
  xml.Element("BuildStamp",
              this->CTest->GetCurrentTag() + "-" +
                this->CTest->GetTestModelString());
  xml.Element("StartDateTime", start_time);
  xml.Element("StartTime", start_time_time);
  xml.Element("UpdateCommand", vc->GetUpdateCommandLine());
  xml.Element("UpdateType",
              cmCTestUpdateHandlerUpdateToString(this->UpdateType));

  bool loadedMods = vc->WriteXML(xml);

  int localModifications = 0;
  int numUpdated = vc->GetPathCount(cmCTestVC::PathUpdated);
  if (numUpdated) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "   Found " << numUpdated << " updated files\n",
                       this->Quiet);
  }
  if (int numModified = vc->GetPathCount(cmCTestVC::PathModified)) {
    cmCTestOptionalLog(
      this->CTest, HANDLER_OUTPUT,
      "   Found " << numModified << " locally modified files\n", this->Quiet);
    localModifications += numModified;
  }
  if (int numConflicting = vc->GetPathCount(cmCTestVC::PathConflicting)) {
    cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                       "   Found " << numConflicting << " conflicting files\n",
                       this->Quiet);
    localModifications += numConflicting;
  }

  cmCTestOptionalLog(this->CTest, DEBUG, "End" << std::endl, this->Quiet);
  std::string end_time = this->CTest->CurrentTime();
  xml.Element("EndDateTime", end_time);
  xml.Element("EndTime", std::chrono::system_clock::now());
  xml.Element("ElapsedMinutes",
              std::chrono::duration_cast<std::chrono::minutes>(
                std::chrono::steady_clock::now() - elapsed_time_start)
                .count());

  xml.StartElement("UpdateReturnStatus");
  if (localModifications) {
    xml.Content("Update error: "
                "There are modified or conflicting files in the repository");
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "   There are modified or conflicting files in the repository"
                 << std::endl);
  }
  if (!updated) {
    xml.Content("Update command failed:\n");
    xml.Content(vc->GetUpdateCommandLine());
    cmCTestLog(this->CTest, HANDLER_OUTPUT,
               "   Update command failed: " << vc->GetUpdateCommandLine()
                                            << "\n");
  }
  xml.EndElement(); // UpdateReturnStatus
  xml.EndElement(); // Update
  xml.EndDocument();
  return updated && loadedMods ? numUpdated : -1;
}

int cmCTestUpdateHandler::DetectVCS(const char* dir)
{
  std::string sourceDirectory = dir;
  cmCTestOptionalLog(this->CTest, DEBUG,
                     "Check directory: " << sourceDirectory << std::endl,
                     this->Quiet);
  sourceDirectory += "/.svn";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_SVN;
  }
  sourceDirectory = dir;
  sourceDirectory += "/CVS";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_CVS;
  }
  sourceDirectory = dir;
  sourceDirectory += "/.bzr";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_BZR;
  }
  sourceDirectory = dir;
  sourceDirectory += "/.git";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_GIT;
  }
  sourceDirectory = dir;
  sourceDirectory += "/.hg";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_HG;
  }
  sourceDirectory = dir;
  sourceDirectory += "/.p4";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_P4;
  }
  sourceDirectory = dir;
  sourceDirectory += "/.p4config";
  if (cmSystemTools::FileExists(sourceDirectory)) {
    return cmCTestUpdateHandler::e_P4;
  }
  return cmCTestUpdateHandler::e_UNKNOWN;
}

bool cmCTestUpdateHandler::SelectVCS()
{
  // Get update command
  this->UpdateCommand = this->CTest->GetCTestConfiguration("UpdateCommand");

  // Detect the VCS managing the source tree.
  this->UpdateType = this->DetectVCS(this->GetOption("SourceDirectory"));
  if (this->UpdateType == e_UNKNOWN) {
    // The source tree does not have a recognized VCS.  Check the
    // configuration value or command name.
    this->UpdateType = this->DetermineType(
      this->UpdateCommand.c_str(),
      this->CTest->GetCTestConfiguration("UpdateType").c_str());
  }

  // If no update command was specified, lookup one for this VCS tool.
  if (this->UpdateCommand.empty()) {
    const char* key = nullptr;
    switch (this->UpdateType) {
      case e_CVS:
        key = "CVSCommand";
        break;
      case e_SVN:
        key = "SVNCommand";
        break;
      case e_BZR:
        key = "BZRCommand";
        break;
      case e_GIT:
        key = "GITCommand";
        break;
      case e_HG:
        key = "HGCommand";
        break;
      case e_P4:
        key = "P4Command";
        break;
      default:
        break;
    }
    if (key) {
      this->UpdateCommand = this->CTest->GetCTestConfiguration(key);
    }
    if (this->UpdateCommand.empty()) {
      std::ostringstream e;
      e << "Cannot find UpdateCommand ";
      if (key) {
        e << "or " << key;
      }
      e << " configuration key.";
      cmCTestLog(this->CTest, ERROR_MESSAGE, e.str() << std::endl);
      return false;
    }
  }

  return true;
}
