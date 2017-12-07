/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVariableWatchCommand.h"

#include <sstream>

#include "cmExecutionStatus.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmVariableWatch.h"
#include "cmake.h"

struct cmVariableWatchCallbackData
{
  bool InCallback;
  std::string Command;
};

static void cmVariableWatchCommandVariableAccessed(const std::string& variable,
                                                   int access_type,
                                                   void* client_data,
                                                   const char* newValue,
                                                   const cmMakefile* mf)
{
  cmVariableWatchCallbackData* data =
    static_cast<cmVariableWatchCallbackData*>(client_data);

  if (data->InCallback) {
    return;
  }
  data->InCallback = true;

  cmListFileFunction newLFF;
  cmListFileArgument arg;
  bool processed = false;
  const char* accessString = cmVariableWatch::GetAccessAsString(access_type);
  const char* currentListFile = mf->GetDefinition("CMAKE_CURRENT_LIST_FILE");

  /// Ultra bad!!
  cmMakefile* makefile = const_cast<cmMakefile*>(mf);

  std::string stack = makefile->GetProperty("LISTFILE_STACK");
  if (!data->Command.empty()) {
    newLFF.Arguments.clear();
    newLFF.Arguments.push_back(
      cmListFileArgument(variable, cmListFileArgument::Quoted, 9999));
    newLFF.Arguments.push_back(
      cmListFileArgument(accessString, cmListFileArgument::Quoted, 9999));
    newLFF.Arguments.push_back(cmListFileArgument(
      newValue ? newValue : "", cmListFileArgument::Quoted, 9999));
    newLFF.Arguments.push_back(
      cmListFileArgument(currentListFile, cmListFileArgument::Quoted, 9999));
    newLFF.Arguments.push_back(
      cmListFileArgument(stack, cmListFileArgument::Quoted, 9999));
    newLFF.Name = data->Command;
    newLFF.Line = 9999;
    cmExecutionStatus status;
    if (!makefile->ExecuteCommand(newLFF, status)) {
      std::ostringstream error;
      error << "Error in cmake code at\nUnknown:0:\n"
            << "A command failed during the invocation of callback \""
            << data->Command << "\".";
      cmSystemTools::Error(error.str().c_str());
      data->InCallback = false;
      return;
    }
    processed = true;
  }
  if (!processed) {
    std::ostringstream msg;
    msg << "Variable \"" << variable << "\" was accessed using "
        << accessString << " with value \"" << (newValue ? newValue : "")
        << "\".";
    makefile->IssueMessage(cmake::LOG, msg.str());
  }

  data->InCallback = false;
}

static void deleteVariableWatchCallbackData(void* client_data)
{
  cmVariableWatchCallbackData* data =
    static_cast<cmVariableWatchCallbackData*>(client_data);
  delete data;
}

cmVariableWatchCommand::cmVariableWatchCommand()
{
}

cmVariableWatchCommand::~cmVariableWatchCommand()
{
  std::set<std::string>::const_iterator it;
  for (it = this->WatchedVariables.begin(); it != this->WatchedVariables.end();
       ++it) {
    this->Makefile->GetCMakeInstance()->GetVariableWatch()->RemoveWatch(
      *it, cmVariableWatchCommandVariableAccessed);
  }
}

bool cmVariableWatchCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("must be called with at least one argument.");
    return false;
  }
  std::string const& variable = args[0];
  std::string command;
  if (args.size() > 1) {
    command = args[1];
  }
  if (variable == "CMAKE_CURRENT_LIST_FILE") {
    std::ostringstream ostr;
    ostr << "cannot be set on the variable: " << variable;
    this->SetError(ostr.str());
    return false;
  }

  cmVariableWatchCallbackData* data = new cmVariableWatchCallbackData;

  data->InCallback = false;
  data->Command = command;

  this->WatchedVariables.insert(variable);
  if (!this->Makefile->GetCMakeInstance()->GetVariableWatch()->AddWatch(
        variable, cmVariableWatchCommandVariableAccessed, data,
        deleteVariableWatchCallbackData)) {
    deleteVariableWatchCallbackData(data);
    return false;
  }

  return true;
}
