/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCustomCommandGenerator.h"

#include "cmCustomCommand.h"
#include "cmCustomCommandLines.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"

#include "cmConfigure.h"

cmCustomCommandGenerator::cmCustomCommandGenerator(cmCustomCommand const& cc,
                                                   const std::string& config,
                                                   cmLocalGenerator* lg)
  : CC(cc)
  , Config(config)
  , LG(lg)
  , OldStyle(cc.GetEscapeOldStyle())
  , MakeVars(cc.GetEscapeAllowMakeVars())
  , GE(new cmGeneratorExpression(cc.GetBacktrace()))
  , DependsDone(false)
{
  const cmCustomCommandLines& cmdlines = this->CC.GetCommandLines();
  for (cmCustomCommandLines::const_iterator cmdline = cmdlines.begin();
       cmdline != cmdlines.end(); ++cmdline) {
    cmCustomCommandLine argv;
    for (cmCustomCommandLine::const_iterator clarg = cmdline->begin();
         clarg != cmdline->end(); ++clarg) {
      CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = this->GE->Parse(*clarg);
      std::string parsed_arg = cge->Evaluate(this->LG, this->Config);
      if (this->CC.GetCommandExpandLists()) {
        std::vector<std::string> ExpandedArg;
        cmSystemTools::ExpandListArgument(parsed_arg, ExpandedArg);
        argv.insert(argv.end(), ExpandedArg.begin(), ExpandedArg.end());
      } else {
        argv.push_back(parsed_arg);
      }
    }
    this->CommandLines.push_back(argv);
  }
}

cmCustomCommandGenerator::~cmCustomCommandGenerator()
{
  delete this->GE;
}

unsigned int cmCustomCommandGenerator::GetNumberOfCommands() const
{
  return static_cast<unsigned int>(this->CC.GetCommandLines().size());
}

const char* cmCustomCommandGenerator::GetCrossCompilingEmulator(
  unsigned int c) const
{
  if (!this->LG->GetMakefile()->IsOn("CMAKE_CROSSCOMPILING")) {
    return CM_NULLPTR;
  }
  std::string const& argv0 = this->CommandLines[c][0];
  cmGeneratorTarget* target = this->LG->FindGeneratorTargetToUse(argv0);
  if (target && target->GetType() == cmStateEnums::EXECUTABLE &&
      !target->IsImported()) {
    return target->GetProperty("CROSSCOMPILING_EMULATOR");
  }
  return CM_NULLPTR;
}

const char* cmCustomCommandGenerator::GetArgv0Location(unsigned int c) const
{
  std::string const& argv0 = this->CommandLines[c][0];
  cmGeneratorTarget* target = this->LG->FindGeneratorTargetToUse(argv0);
  if (target && target->GetType() == cmStateEnums::EXECUTABLE &&
      (target->IsImported() ||
       target->GetProperty("CROSSCOMPILING_EMULATOR") ||
       !this->LG->GetMakefile()->IsOn("CMAKE_CROSSCOMPILING"))) {
    return target->GetLocation(this->Config);
  }
  return CM_NULLPTR;
}

std::string cmCustomCommandGenerator::GetCommand(unsigned int c) const
{
  if (const char* emulator = this->GetCrossCompilingEmulator(c)) {
    return std::string(emulator);
  }
  if (const char* location = this->GetArgv0Location(c)) {
    return std::string(location);
  }

  return this->CommandLines[c][0];
}

std::string escapeForShellOldStyle(const std::string& str)
{
  std::string result;
#if defined(_WIN32) && !defined(__CYGWIN__)
  // if there are spaces
  std::string temp = str;
  if (temp.find(" ") != std::string::npos &&
      temp.find("\"") == std::string::npos) {
    result = "\"";
    result += str;
    result += "\"";
    return result;
  }
  return str;
#else
  for (const char* ch = str.c_str(); *ch != '\0'; ++ch) {
    if (*ch == ' ') {
      result += '\\';
    }
    result += *ch;
  }
  return result;
#endif
}

void cmCustomCommandGenerator::AppendArguments(unsigned int c,
                                               std::string& cmd) const
{
  unsigned int offset = 1;
  if (this->GetCrossCompilingEmulator(c) != CM_NULLPTR) {
    offset = 0;
  }
  cmCustomCommandLine const& commandLine = this->CommandLines[c];
  for (unsigned int j = offset; j < commandLine.size(); ++j) {
    std::string arg;
    if (const char* location =
          j == 0 ? this->GetArgv0Location(c) : CM_NULLPTR) {
      // GetCommand returned the emulator instead of the argv0 location,
      // so transform the latter now.
      arg = location;
    } else {
      arg = commandLine[j];
    }
    cmd += " ";
    if (this->OldStyle) {
      cmd += escapeForShellOldStyle(arg);
    } else {
      cmOutputConverter converter(this->LG->GetStateSnapshot());
      cmd += converter.EscapeForShell(arg, this->MakeVars);
    }
  }
}

const char* cmCustomCommandGenerator::GetComment() const
{
  return this->CC.GetComment();
}

std::string cmCustomCommandGenerator::GetWorkingDirectory() const
{
  return this->CC.GetWorkingDirectory();
}

std::vector<std::string> const& cmCustomCommandGenerator::GetOutputs() const
{
  return this->CC.GetOutputs();
}

std::vector<std::string> const& cmCustomCommandGenerator::GetByproducts() const
{
  return this->CC.GetByproducts();
}

std::vector<std::string> const& cmCustomCommandGenerator::GetDepends() const
{
  if (!this->DependsDone) {
    this->DependsDone = true;
    std::vector<std::string> depends = this->CC.GetDepends();
    for (std::vector<std::string>::const_iterator i = depends.begin();
         i != depends.end(); ++i) {
      CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = this->GE->Parse(*i);
      std::vector<std::string> result;
      cmSystemTools::ExpandListArgument(cge->Evaluate(this->LG, this->Config),
                                        result);
      for (std::vector<std::string>::iterator it = result.begin();
           it != result.end(); ++it) {
        if (cmSystemTools::FileIsFullPath(it->c_str())) {
          *it = cmSystemTools::CollapseFullPath(*it);
        }
      }
      this->Depends.insert(this->Depends.end(), result.begin(), result.end());
    }
  }
  return this->Depends;
}
