/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQTWrapCPPCommand.h"

#include "cmCustomCommandLines.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmQTWrapCPPCommand
bool cmQTWrapCPPCommand::InitialPass(std::vector<std::string> const& args,
                                     cmExecutionStatus&)
{
  if (args.size() < 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Get the moc executable to run in the custom command.
  const char* moc_exe =
    this->Makefile->GetRequiredDefinition("QT_MOC_EXECUTABLE");

  // Get the variable holding the list of sources.
  std::string const& sourceList = args[1];
  std::string sourceListValue = this->Makefile->GetSafeDefinition(sourceList);

  // Create a rule for all sources listed.
  for (std::vector<std::string>::const_iterator j = (args.begin() + 2);
       j != args.end(); ++j) {
    cmSourceFile* curr = this->Makefile->GetSource(*j);
    // if we should wrap the class
    if (!(curr && curr->GetPropertyAsBool("WRAP_EXCLUDE"))) {
      // Compute the name of the file to generate.
      std::string srcName = cmSystemTools::GetFilenameWithoutLastExtension(*j);
      std::string newName = this->Makefile->GetCurrentBinaryDirectory();
      newName += "/moc_";
      newName += srcName;
      newName += ".cxx";
      cmSourceFile* sf = this->Makefile->GetOrCreateSource(newName, true);
      if (curr) {
        sf->SetProperty("ABSTRACT", curr->GetProperty("ABSTRACT"));
      }

      // Compute the name of the header from which to generate the file.
      std::string hname;
      if (cmSystemTools::FileIsFullPath(j->c_str())) {
        hname = *j;
      } else {
        if (curr && curr->GetPropertyAsBool("GENERATED")) {
          hname = this->Makefile->GetCurrentBinaryDirectory();
        } else {
          hname = this->Makefile->GetCurrentSourceDirectory();
        }
        hname += "/";
        hname += *j;
      }

      // Append the generated source file to the list.
      if (!sourceListValue.empty()) {
        sourceListValue += ";";
      }
      sourceListValue += newName;

      // Create the custom command to generate the file.
      cmCustomCommandLine commandLine;
      commandLine.push_back(moc_exe);
      commandLine.push_back("-o");
      commandLine.push_back(newName);
      commandLine.push_back(hname);

      cmCustomCommandLines commandLines;
      commandLines.push_back(commandLine);

      std::vector<std::string> depends;
      depends.push_back(moc_exe);
      depends.push_back(hname);

      std::string no_main_dependency;
      const char* no_working_dir = CM_NULLPTR;
      this->Makefile->AddCustomCommandToOutput(
        newName, depends, no_main_dependency, commandLines, "Qt Wrapped File",
        no_working_dir);
    }
  }

  // Store the final list of source files.
  this->Makefile->AddDefinition(sourceList, sourceListValue.c_str());
  return true;
}
