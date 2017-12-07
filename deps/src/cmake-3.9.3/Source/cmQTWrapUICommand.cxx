/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQTWrapUICommand.h"

#include "cmCustomCommandLines.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmQTWrapUICommand
bool cmQTWrapUICommand::InitialPass(std::vector<std::string> const& args,
                                    cmExecutionStatus&)
{
  if (args.size() < 4) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Get the uic and moc executables to run in the custom commands.
  const char* uic_exe =
    this->Makefile->GetRequiredDefinition("QT_UIC_EXECUTABLE");
  const char* moc_exe =
    this->Makefile->GetRequiredDefinition("QT_MOC_EXECUTABLE");

  // Get the variable holding the list of sources.
  std::string const& headerList = args[1];
  std::string const& sourceList = args[2];
  std::string headerListValue = this->Makefile->GetSafeDefinition(headerList);
  std::string sourceListValue = this->Makefile->GetSafeDefinition(sourceList);

  // Create rules for all sources listed.
  for (std::vector<std::string>::const_iterator j = (args.begin() + 3);
       j != args.end(); ++j) {
    cmSourceFile* curr = this->Makefile->GetSource(*j);
    // if we should wrap the class
    if (!(curr && curr->GetPropertyAsBool("WRAP_EXCLUDE"))) {
      // Compute the name of the files to generate.
      std::string srcName = cmSystemTools::GetFilenameWithoutLastExtension(*j);
      std::string hName = this->Makefile->GetCurrentBinaryDirectory();
      hName += "/";
      hName += srcName;
      hName += ".h";
      std::string cxxName = this->Makefile->GetCurrentBinaryDirectory();
      cxxName += "/";
      cxxName += srcName;
      cxxName += ".cxx";
      std::string mocName = this->Makefile->GetCurrentBinaryDirectory();
      mocName += "/moc_";
      mocName += srcName;
      mocName += ".cxx";

      // Compute the name of the ui file from which to generate others.
      std::string uiName;
      if (cmSystemTools::FileIsFullPath(j->c_str())) {
        uiName = *j;
      } else {
        if (curr && curr->GetPropertyAsBool("GENERATED")) {
          uiName = this->Makefile->GetCurrentBinaryDirectory();
        } else {
          uiName = this->Makefile->GetCurrentSourceDirectory();
        }
        uiName += "/";
        uiName += *j;
      }

      // create the list of headers
      if (!headerListValue.empty()) {
        headerListValue += ";";
      }
      headerListValue += hName;

      // create the list of sources
      if (!sourceListValue.empty()) {
        sourceListValue += ";";
      }
      sourceListValue += cxxName;
      sourceListValue += ";";
      sourceListValue += mocName;

      // set up .ui to .h and .cxx command
      cmCustomCommandLine hCommand;
      hCommand.push_back(uic_exe);
      hCommand.push_back("-o");
      hCommand.push_back(hName);
      hCommand.push_back(uiName);
      cmCustomCommandLines hCommandLines;
      hCommandLines.push_back(hCommand);

      cmCustomCommandLine cxxCommand;
      cxxCommand.push_back(uic_exe);
      cxxCommand.push_back("-impl");
      cxxCommand.push_back(hName);
      cxxCommand.push_back("-o");
      cxxCommand.push_back(cxxName);
      cxxCommand.push_back(uiName);
      cmCustomCommandLines cxxCommandLines;
      cxxCommandLines.push_back(cxxCommand);

      cmCustomCommandLine mocCommand;
      mocCommand.push_back(moc_exe);
      mocCommand.push_back("-o");
      mocCommand.push_back(mocName);
      mocCommand.push_back(hName);
      cmCustomCommandLines mocCommandLines;
      mocCommandLines.push_back(mocCommand);

      std::vector<std::string> depends;
      depends.push_back(uiName);
      std::string no_main_dependency;
      const char* no_comment = CM_NULLPTR;
      const char* no_working_dir = CM_NULLPTR;
      this->Makefile->AddCustomCommandToOutput(
        hName, depends, no_main_dependency, hCommandLines, no_comment,
        no_working_dir);

      depends.push_back(hName);
      this->Makefile->AddCustomCommandToOutput(
        cxxName, depends, no_main_dependency, cxxCommandLines, no_comment,
        no_working_dir);

      depends.clear();
      depends.push_back(hName);
      this->Makefile->AddCustomCommandToOutput(
        mocName, depends, no_main_dependency, mocCommandLines, no_comment,
        no_working_dir);
    }
  }

  // Store the final list of source files and headers.
  this->Makefile->AddDefinition(sourceList, sourceListValue.c_str());
  this->Makefile->AddDefinition(headerList, headerListValue.c_str());
  return true;
}
