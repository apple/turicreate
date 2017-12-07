/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFLTKWrapUICommand.h"

#include <stddef.h>

#include "cmCustomCommandLines.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;
class cmTarget;

// cmFLTKWrapUICommand
bool cmFLTKWrapUICommand::InitialPass(std::vector<std::string> const& args,
                                      cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // what is the current source dir
  std::string cdir = this->Makefile->GetCurrentSourceDirectory();
  const char* fluid_exe =
    this->Makefile->GetRequiredDefinition("FLTK_FLUID_EXECUTABLE");

  // get parameter for the command
  this->Target = args[0]; // Target that will use the generated files

  // get the list of GUI files from which .cxx and .h will be generated
  std::string outputDirectory = this->Makefile->GetCurrentBinaryDirectory();

  {
    // Some of the generated files are *.h so the directory "GUI"
    // where they are created have to be added to the include path
    std::vector<std::string> outputDirectories;
    outputDirectories.push_back(outputDirectory);
    this->Makefile->AddIncludeDirectories(outputDirectories);
  }

  for (std::vector<std::string>::const_iterator i = (args.begin() + 1);
       i != args.end(); i++) {
    cmSourceFile* curr = this->Makefile->GetSource(*i);
    // if we should use the source GUI
    // to generate .cxx and .h files
    if (!curr || !curr->GetPropertyAsBool("WRAP_EXCLUDE")) {
      std::string outName = outputDirectory;
      outName += "/";
      outName += cmSystemTools::GetFilenameWithoutExtension(*i);
      std::string hname = outName;
      hname += ".h";
      std::string origname = cdir + "/" + *i;
      // add starting depends
      std::vector<std::string> depends;
      depends.push_back(origname);
      depends.push_back(fluid_exe);
      std::string cxxres = outName;
      cxxres += ".cxx";

      cmCustomCommandLine commandLine;
      commandLine.push_back(fluid_exe);
      commandLine.push_back("-c"); // instructs Fluid to run in command line
      commandLine.push_back("-h"); // optionally rename .h files
      commandLine.push_back(hname);
      commandLine.push_back("-o"); // optionally rename .cxx files
      commandLine.push_back(cxxres);
      commandLine.push_back(origname); // name of the GUI fluid file
      cmCustomCommandLines commandLines;
      commandLines.push_back(commandLine);

      // Add command for generating the .h and .cxx files
      std::string no_main_dependency;
      const char* no_comment = CM_NULLPTR;
      const char* no_working_dir = CM_NULLPTR;
      this->Makefile->AddCustomCommandToOutput(
        cxxres, depends, no_main_dependency, commandLines, no_comment,
        no_working_dir);
      this->Makefile->AddCustomCommandToOutput(
        hname, depends, no_main_dependency, commandLines, no_comment,
        no_working_dir);

      cmSourceFile* sf = this->Makefile->GetSource(cxxres);
      sf->AddDepend(hname);
      sf->AddDepend(origname);
      this->GeneratedSourcesClasses.push_back(sf);
    }
  }

  // create the variable with the list of sources in it
  size_t lastHeadersClass = this->GeneratedSourcesClasses.size();
  std::string sourceListValue;
  for (size_t classNum = 0; classNum < lastHeadersClass; classNum++) {
    if (classNum) {
      sourceListValue += ";";
    }
    sourceListValue += this->GeneratedSourcesClasses[classNum]->GetFullPath();
  }
  std::string varName = this->Target;
  varName += "_FLTK_UI_SRCS";
  this->Makefile->AddDefinition(varName, sourceListValue.c_str());

  return true;
}

void cmFLTKWrapUICommand::FinalPass()
{
  // people should add the srcs to the target themselves, but the old command
  // didn't support that, so check and see if they added the files in and if
  // they didn;t then print a warning and add then anyhow
  cmTarget* target = this->Makefile->FindLocalNonAliasTarget(this->Target);
  if (!target) {
    std::string msg =
      "FLTK_WRAP_UI was called with a target that was never created: ";
    msg += this->Target;
    msg += ".  The problem was found while processing the source directory: ";
    msg += this->Makefile->GetCurrentSourceDirectory();
    msg += ".  This FLTK_WRAP_UI call will be ignored.";
    cmSystemTools::Message(msg.c_str(), "Warning");
    return;
  }
}
