/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCustomCommandGenerator.h"

#include "cmCustomCommand.h"
#include "cmCustomCommandLines.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

#include <memory> // IWYU pragma: keep
#include <stddef.h>
#include <utility>

cmCustomCommandGenerator::cmCustomCommandGenerator(cmCustomCommand const& cc,
                                                   const std::string& config,
                                                   cmLocalGenerator* lg)
  : CC(cc)
  , Config(config)
  , LG(lg)
  , OldStyle(cc.GetEscapeOldStyle())
  , MakeVars(cc.GetEscapeAllowMakeVars())
  , GE(new cmGeneratorExpression(cc.GetBacktrace()))
{
  const cmCustomCommandLines& cmdlines = this->CC.GetCommandLines();
  for (cmCustomCommandLine const& cmdline : cmdlines) {
    cmCustomCommandLine argv;
    for (std::string const& clarg : cmdline) {
      std::unique_ptr<cmCompiledGeneratorExpression> cge =
        this->GE->Parse(clarg);
      std::string parsed_arg = cge->Evaluate(this->LG, this->Config);
      if (this->CC.GetCommandExpandLists()) {
        std::vector<std::string> ExpandedArg;
        cmSystemTools::ExpandListArgument(parsed_arg, ExpandedArg);
        argv.insert(argv.end(), ExpandedArg.begin(), ExpandedArg.end());
      } else {
        argv.push_back(std::move(parsed_arg));
      }
    }

    // Later code assumes at least one entry exists, but expanding
    // lists on an empty command may have left this empty.
    // FIXME: Should we define behavior for removing empty commands?
    if (argv.empty()) {
      argv.push_back(std::string());
    }

    this->CommandLines.push_back(std::move(argv));
  }

  std::vector<std::string> depends = this->CC.GetDepends();
  for (std::string const& d : depends) {
    std::unique_ptr<cmCompiledGeneratorExpression> cge = this->GE->Parse(d);
    std::vector<std::string> result;
    cmSystemTools::ExpandListArgument(cge->Evaluate(this->LG, this->Config),
                                      result);
    for (std::string& it : result) {
      if (cmSystemTools::FileIsFullPath(it)) {
        it = cmSystemTools::CollapseFullPath(it);
      }
    }
    this->Depends.insert(this->Depends.end(), result.begin(), result.end());
  }

  const std::string& workingdirectory = this->CC.GetWorkingDirectory();
  if (!workingdirectory.empty()) {
    std::unique_ptr<cmCompiledGeneratorExpression> cge =
      this->GE->Parse(workingdirectory);
    this->WorkingDirectory = cge->Evaluate(this->LG, this->Config);
    // Convert working directory to a full path.
    if (!this->WorkingDirectory.empty()) {
      std::string const& build_dir = this->LG->GetCurrentBinaryDirectory();
      this->WorkingDirectory =
        cmSystemTools::CollapseFullPath(this->WorkingDirectory, build_dir);
    }
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
    return nullptr;
  }
  std::string const& argv0 = this->CommandLines[c][0];
  cmGeneratorTarget* target = this->LG->FindGeneratorTargetToUse(argv0);
  if (target && target->GetType() == cmStateEnums::EXECUTABLE &&
      !target->IsImported()) {
    return target->GetProperty("CROSSCOMPILING_EMULATOR");
  }
  return nullptr;
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
  return nullptr;
}

bool cmCustomCommandGenerator::HasOnlyEmptyCommandLines() const
{
  for (size_t i = 0; i < this->CommandLines.size(); ++i) {
    for (size_t j = 0; j < this->CommandLines[i].size(); ++j) {
      if (!this->CommandLines[i][j].empty()) {
        return false;
      }
    }
  }
  return true;
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
  if (this->GetCrossCompilingEmulator(c) != nullptr) {
    offset = 0;
  }
  cmCustomCommandLine const& commandLine = this->CommandLines[c];
  for (unsigned int j = offset; j < commandLine.size(); ++j) {
    std::string arg;
    if (const char* location = j == 0 ? this->GetArgv0Location(c) : nullptr) {
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
      cmd += this->LG->EscapeForShell(arg, this->MakeVars);
    }
  }
}

const char* cmCustomCommandGenerator::GetComment() const
{
  return this->CC.GetComment();
}

std::string cmCustomCommandGenerator::GetWorkingDirectory() const
{
  return this->WorkingDirectory;
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
  return this->Depends;
}
