/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmTestGenerator.h"

#include <map>
#include <ostream>
#include <utility>

#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmOutputConverter.h"
#include "cmProperty.h"
#include "cmPropertyMap.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTest.h"

cmTestGenerator::cmTestGenerator(
  cmTest* test, std::vector<std::string> const& configurations)
  : cmScriptGenerator("CTEST_CONFIGURATION_TYPE", configurations)
  , Test(test)
{
  this->ActionsPerConfig = !test->GetOldStyle();
  this->TestGenerated = false;
  this->LG = CM_NULLPTR;
}

cmTestGenerator::~cmTestGenerator()
{
}

void cmTestGenerator::Compute(cmLocalGenerator* lg)
{
  this->LG = lg;
}

void cmTestGenerator::GenerateScriptConfigs(std::ostream& os, Indent indent)
{
  // Create the tests.
  this->cmScriptGenerator::GenerateScriptConfigs(os, indent);
}

void cmTestGenerator::GenerateScriptActions(std::ostream& os, Indent indent)
{
  if (this->ActionsPerConfig) {
    // This is the per-config generation in a single-configuration
    // build generator case.  The superclass will call our per-config
    // method.
    this->cmScriptGenerator::GenerateScriptActions(os, indent);
  } else {
    // This is an old-style test, so there is only one config.
    // assert(this->Test->GetOldStyle());
    this->GenerateOldStyle(os, indent);
  }
}

void cmTestGenerator::GenerateScriptForConfig(std::ostream& os,
                                              const std::string& config,
                                              Indent indent)
{
  this->TestGenerated = true;

  // Set up generator expression evaluation context.
  cmGeneratorExpression ge(this->Test->GetBacktrace());

  // Start the test command.
  os << indent << "add_test(" << this->Test->GetName() << " ";

  // Get the test command line to be executed.
  std::vector<std::string> const& command = this->Test->GetCommand();

  // Check whether the command executable is a target whose name is to
  // be translated.
  std::string exe = command[0];
  cmGeneratorTarget* target = this->LG->FindGeneratorTargetToUse(exe);
  if (target && target->GetType() == cmStateEnums::EXECUTABLE) {
    // Use the target file on disk.
    exe = target->GetFullPath(config);

    // Prepend with the emulator when cross compiling if required.
    const char* emulator = target->GetProperty("CROSSCOMPILING_EMULATOR");
    if (emulator != CM_NULLPTR) {
      std::vector<std::string> emulatorWithArgs;
      cmSystemTools::ExpandListArgument(emulator, emulatorWithArgs);
      std::string emulatorExe(emulatorWithArgs[0]);
      cmSystemTools::ConvertToUnixSlashes(emulatorExe);
      os << cmOutputConverter::EscapeForCMake(emulatorExe) << " ";
      for (std::vector<std::string>::const_iterator ei =
             emulatorWithArgs.begin() + 1;
           ei != emulatorWithArgs.end(); ++ei) {
        os << cmOutputConverter::EscapeForCMake(*ei) << " ";
      }
    }
  } else {
    // Use the command name given.
    exe = ge.Parse(exe.c_str())->Evaluate(this->LG, config);
    cmSystemTools::ConvertToUnixSlashes(exe);
  }

  // Generate the command line with full escapes.
  os << cmOutputConverter::EscapeForCMake(exe);
  for (std::vector<std::string>::const_iterator ci = command.begin() + 1;
       ci != command.end(); ++ci) {
    os << " " << cmOutputConverter::EscapeForCMake(
                   ge.Parse(*ci)->Evaluate(this->LG, config));
  }

  // Finish the test command.
  os << ")\n";

  // Output properties for the test.
  cmPropertyMap& pm = this->Test->GetProperties();
  if (!pm.empty()) {
    os << indent << "set_tests_properties(" << this->Test->GetName()
       << " PROPERTIES ";
    for (cmPropertyMap::const_iterator i = pm.begin(); i != pm.end(); ++i) {
      os << " " << i->first << " "
         << cmOutputConverter::EscapeForCMake(
              ge.Parse(i->second.GetValue())->Evaluate(this->LG, config));
    }
    os << ")" << std::endl;
  }
}

void cmTestGenerator::GenerateScriptNoConfig(std::ostream& os, Indent indent)
{
  os << indent << "add_test(" << this->Test->GetName() << " NOT_AVAILABLE)\n";
}

bool cmTestGenerator::NeedsScriptNoConfig() const
{
  return (this->TestGenerated &&    // test generated for at least one config
          this->ActionsPerConfig && // test is config-aware
          this->Configurations.empty() &&      // test runs in all configs
          !this->ConfigurationTypes->empty()); // config-dependent command
}

void cmTestGenerator::GenerateOldStyle(std::ostream& fout, Indent indent)
{
  this->TestGenerated = true;

  // Get the test command line to be executed.
  std::vector<std::string> const& command = this->Test->GetCommand();

  std::string exe = command[0];
  cmSystemTools::ConvertToUnixSlashes(exe);
  fout << indent;
  fout << "add_test(";
  fout << this->Test->GetName() << " \"" << exe << "\"";

  for (std::vector<std::string>::const_iterator argit = command.begin() + 1;
       argit != command.end(); ++argit) {
    // Just double-quote all arguments so they are re-parsed
    // correctly by the test system.
    fout << " \"";
    for (std::string::const_iterator c = argit->begin(); c != argit->end();
         ++c) {
      // Escape quotes within arguments.  We should escape
      // backslashes too but we cannot because it makes the result
      // inconsistent with previous behavior of this command.
      if ((*c == '"')) {
        fout << '\\';
      }
      fout << *c;
    }
    fout << "\"";
  }
  fout << ")" << std::endl;

  // Output properties for the test.
  cmPropertyMap& pm = this->Test->GetProperties();
  if (!pm.empty()) {
    fout << indent << "set_tests_properties(" << this->Test->GetName()
         << " PROPERTIES ";
    for (cmPropertyMap::const_iterator i = pm.begin(); i != pm.end(); ++i) {
      fout << " " << i->first << " "
           << cmOutputConverter::EscapeForCMake(i->second.GetValue());
    }
    fout << ")" << std::endl;
  }
}
