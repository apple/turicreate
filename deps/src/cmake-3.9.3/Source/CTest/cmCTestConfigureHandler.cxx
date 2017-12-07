/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestConfigureHandler.h"

#include "cmCTest.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"

#include <ostream>
#include <string>

cmCTestConfigureHandler::cmCTestConfigureHandler()
{
}

void cmCTestConfigureHandler::Initialize()
{
  this->Superclass::Initialize();
}

// clearly it would be nice if this were broken up into a few smaller
// functions and commented...
int cmCTestConfigureHandler::ProcessHandler()
{
  cmCTestOptionalLog(this->CTest, HANDLER_OUTPUT,
                     "Configure project" << std::endl, this->Quiet);
  std::string cCommand =
    this->CTest->GetCTestConfiguration("ConfigureCommand");
  if (cCommand.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot find ConfigureCommand key in the DartConfiguration.tcl"
                 << std::endl);
    return -1;
  }

  std::string buildDirectory =
    this->CTest->GetCTestConfiguration("BuildDirectory");
  if (buildDirectory.empty()) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Cannot find BuildDirectory  key in the DartConfiguration.tcl"
                 << std::endl);
    return -1;
  }

  double elapsed_time_start = cmSystemTools::GetTime();
  std::string output;
  int retVal = 0;
  int res = 0;
  if (!this->CTest->GetShowOnly()) {
    cmGeneratedFileStream os;
    if (!this->StartResultingXML(cmCTest::PartConfigure, "Configure", os)) {
      cmCTestLog(this->CTest, ERROR_MESSAGE, "Cannot open configure file"
                   << std::endl);
      return 1;
    }
    std::string start_time = this->CTest->CurrentTime();
    unsigned int start_time_time =
      static_cast<unsigned int>(cmSystemTools::GetTime());

    cmGeneratedFileStream ofs;
    this->StartLogFile("Configure", ofs);
    cmCTestOptionalLog(this->CTest, HANDLER_VERBOSE_OUTPUT,
                       "Configure with command: " << cCommand << std::endl,
                       this->Quiet);
    res = this->CTest->RunMakeCommand(cCommand.c_str(), output, &retVal,
                                      buildDirectory.c_str(), 0, ofs);

    if (ofs) {
      ofs.close();
    }

    if (os) {
      cmXMLWriter xml(os);
      this->CTest->StartXML(xml, this->AppendXML);
      xml.StartElement("Configure");
      xml.Element("StartDateTime", start_time);
      xml.Element("StartConfigureTime", start_time_time);
      xml.Element("ConfigureCommand", cCommand);
      cmCTestOptionalLog(this->CTest, DEBUG, "End" << std::endl, this->Quiet);
      xml.Element("Log", output);
      xml.Element("ConfigureStatus", retVal);
      xml.Element("EndDateTime", this->CTest->CurrentTime());
      xml.Element("EndConfigureTime",
                  static_cast<unsigned int>(cmSystemTools::GetTime()));
      xml.Element(
        "ElapsedMinutes",
        static_cast<int>((cmSystemTools::GetTime() - elapsed_time_start) / 6) /
          10.0);
      xml.EndElement(); // Configure
      this->CTest->EndXML(xml);
    }
  } else {
    cmCTestOptionalLog(this->CTest, DEBUG,
                       "Configure with command: " << cCommand << std::endl,
                       this->Quiet);
  }
  if (!res || retVal) {
    cmCTestLog(this->CTest, ERROR_MESSAGE,
               "Error(s) when configuring the project" << std::endl);
    return -1;
  }
  return 0;
}
