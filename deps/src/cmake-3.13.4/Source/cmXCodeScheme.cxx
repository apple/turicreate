/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXCodeScheme.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmXMLSafe.h"

cmXCodeScheme::cmXCodeScheme(cmXCodeObject* xcObj, const TestObjects& tests,
                             const std::vector<std::string>& configList,
                             unsigned int xcVersion)
  : Target(xcObj)
  , Tests(tests)
  , TargetName(xcObj->GetTarget()->GetName())
  , ConfigList(configList)
  , XcodeVersion(xcVersion)
{
}

void cmXCodeScheme::WriteXCodeSharedScheme(const std::string& xcProjDir,
                                           const std::string& container)
{
  // Create shared scheme sub-directory tree
  //
  std::string xcodeSchemeDir = xcProjDir;
  xcodeSchemeDir += "/xcshareddata/xcschemes";
  cmSystemTools::MakeDirectory(xcodeSchemeDir.c_str());

  std::string xcodeSchemeFile = xcodeSchemeDir;
  xcodeSchemeFile += "/";
  xcodeSchemeFile += this->TargetName;
  xcodeSchemeFile += ".xcscheme";

  cmGeneratedFileStream fout(xcodeSchemeFile.c_str());
  fout.SetCopyIfDifferent(true);
  if (!fout) {
    return;
  }

  WriteXCodeXCScheme(fout, container);
}

void cmXCodeScheme::WriteXCodeXCScheme(std::ostream& fout,
                                       const std::string& container)
{
  cmXMLWriter xout(fout);
  xout.SetIndentationElement(std::string(3, ' '));
  xout.StartDocument();

  xout.StartElement("Scheme");
  xout.BreakAttributes();
  xout.Attribute("LastUpgradeVersion", WriteVersionString());
  xout.Attribute("version", "1.3");

  WriteBuildAction(xout, container);
  WriteTestAction(xout, FindConfiguration("Debug"), container);
  WriteLaunchAction(xout, FindConfiguration("Debug"), container);
  WriteProfileAction(xout, FindConfiguration("Release"));
  WriteAnalyzeAction(xout, FindConfiguration("Debug"));
  WriteArchiveAction(xout, FindConfiguration("Release"));

  xout.EndElement();
}

void cmXCodeScheme::WriteBuildAction(cmXMLWriter& xout,
                                     const std::string& container)
{
  xout.StartElement("BuildAction");
  xout.BreakAttributes();
  xout.Attribute("parallelizeBuildables", "YES");
  xout.Attribute("buildImplicitDependencies", "YES");

  xout.StartElement("BuildActionEntries");
  xout.StartElement("BuildActionEntry");
  xout.BreakAttributes();
  xout.Attribute("buildForTesting", "YES");
  xout.Attribute("buildForRunning", "YES");
  xout.Attribute("buildForProfiling", "YES");
  xout.Attribute("buildForArchiving", "YES");
  xout.Attribute("buildForAnalyzing", "YES");

  WriteBuildableReference(xout, this->Target, container);

  xout.EndElement(); // BuildActionEntry
  xout.EndElement(); // BuildActionEntries
  xout.EndElement(); // BuildAction
}

void cmXCodeScheme::WriteTestAction(cmXMLWriter& xout,
                                    const std::string& configuration,
                                    const std::string& container)
{
  xout.StartElement("TestAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.Attribute("selectedDebuggerIdentifier",
                 "Xcode.DebuggerFoundation.Debugger.LLDB");
  xout.Attribute("selectedLauncherIdentifier",
                 "Xcode.DebuggerFoundation.Launcher.LLDB");
  xout.Attribute("shouldUseLaunchSchemeArgsEnv", "YES");

  xout.StartElement("Testables");
  for (auto test : this->Tests) {
    xout.StartElement("TestableReference");
    xout.BreakAttributes();
    xout.Attribute("skipped", "NO");
    WriteBuildableReference(xout, test, container);
    xout.EndElement(); // TestableReference
  }
  xout.EndElement();

  if (IsTestable()) {
    xout.StartElement("MacroExpansion");
    WriteBuildableReference(xout, this->Target, container);
    xout.EndElement(); // MacroExpansion
  }

  xout.StartElement("AdditionalOptions");
  xout.EndElement();

  xout.EndElement(); // TestAction
}

void cmXCodeScheme::WriteLaunchAction(cmXMLWriter& xout,
                                      const std::string& configuration,
                                      const std::string& container)
{
  xout.StartElement("LaunchAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.Attribute("selectedDebuggerIdentifier",
                 "Xcode.DebuggerFoundation.Debugger.LLDB");
  xout.Attribute("selectedLauncherIdentifier",
                 "Xcode.DebuggerFoundation.Launcher.LLDB");
  xout.Attribute("launchStyle", "0");
  xout.Attribute("useCustomWorkingDirectory", "NO");
  xout.Attribute("ignoresPersistentStateOnLaunch", "NO");
  xout.Attribute("debugDocumentVersioning", "YES");
  xout.Attribute("debugServiceExtension", "internal");
  xout.Attribute("allowLocationSimulation", "YES");

  // Diagnostics tab begin

  bool useAddressSanitizer = WriteLaunchActionAttribute(
    xout, "enableAddressSanitizer",
    "XCODE_SCHEME_ADDRESS_SANITIZER"); // not allowed with
                                       // enableThreadSanitizer=YES
  WriteLaunchActionAttribute(
    xout, "enableASanStackUseAfterReturn",
    "XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN");

  bool useThreadSanitizer = false;
  if (!useAddressSanitizer) {
    useThreadSanitizer = WriteLaunchActionAttribute(
      xout, "enableThreadSanitizer",
      "XCODE_SCHEME_THREAD_SANITIZER"); // not allowed with
                                        // enableAddressSanitizer=YES
  }

  WriteLaunchActionAttribute(xout, "stopOnEveryThreadSanitizerIssue",
                             "XCODE_SCHEME_THREAD_SANITIZER_STOP");

  WriteLaunchActionAttribute(xout, "enableUBSanitizer",
                             "XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER");
  WriteLaunchActionAttribute(
    xout, "stopOnEveryUBSanitizerIssue",
    "XCODE_SCHEME_UNDEFINED_BEHAVIOUR_SANITIZER_STOP");

  WriteLaunchActionAttribute(
    xout, "disableMainThreadChecker",
    "XCODE_SCHEME_DISABLE_MAIN_THREAD_CHECKER"); // negative enabled!
  WriteLaunchActionAttribute(xout, "stopOnEveryMainThreadCheckerIssue",
                             "XCODE_SCHEME_MAIN_THREAD_CHECKER_STOP");

  // Diagnostics tab end

  if (IsExecutable(this->Target)) {
    xout.StartElement("BuildableProductRunnable");
    xout.BreakAttributes();
    xout.Attribute("runnableDebuggingMode", "0");

  } else {
    xout.StartElement("MacroExpansion");
  }

  WriteBuildableReference(xout, this->Target, container);

  xout.EndElement(); // MacroExpansion

  // Info tab begin

  if (const char* exe =
        this->Target->GetTarget()->GetProperty("XCODE_SCHEME_EXECUTABLE")) {

    xout.StartElement("PathRunnable");
    xout.BreakAttributes();

    xout.Attribute("runnableDebuggingMode", "0");
    xout.Attribute("FilePath", exe);

    xout.EndElement(); // PathRunnable
  }

  // Info tab end

  // Arguments tab begin

  if (const char* argList =
        this->Target->GetTarget()->GetProperty("XCODE_SCHEME_ARGUMENTS")) {
    std::vector<std::string> arguments;
    cmSystemTools::ExpandListArgument(argList, arguments);
    if (!arguments.empty()) {
      xout.StartElement("CommandLineArguments");

      for (auto argument : arguments) {
        xout.StartElement("CommandLineArgument");
        xout.BreakAttributes();

        xout.Attribute("argument", argument);
        xout.Attribute("isEnabled", "YES");

        xout.EndElement(); // CommandLineArgument
      }

      xout.EndElement(); // CommandLineArguments
    }
  }

  if (const char* envList =
        this->Target->GetTarget()->GetProperty("XCODE_SCHEME_ENVIRONMENT")) {
    std::vector<std::string> envs;
    cmSystemTools::ExpandListArgument(envList, envs);
    if (!envs.empty()) {
      xout.StartElement("EnvironmentVariables");

      for (auto env : envs) {

        xout.StartElement("EnvironmentVariable");
        xout.BreakAttributes();

        std::string envValue;
        const auto p = env.find_first_of('=');
        if (p != std::string::npos) {
          envValue = env.substr(p + 1);
          env.resize(p);
        }

        xout.Attribute("key", env);
        xout.Attribute("value", envValue);
        xout.Attribute("isEnabled", "YES");

        xout.EndElement(); // EnvironmentVariable
      }

      xout.EndElement(); // EnvironmentVariables
    }
  }

  // Arguments tab end

  xout.StartElement("AdditionalOptions");

  if (!useThreadSanitizer) {
    WriteLaunchActionAdditionalOption(xout, "MallocScribble", "",
                                      "XCODE_SCHEME_MALLOC_SCRIBBLE");
  }

  if (!useThreadSanitizer && !useAddressSanitizer) {
    WriteLaunchActionAdditionalOption(xout, "MallocGuardEdges", "",
                                      "XCODE_SCHEME_MALLOC_GUARD_EDGES");
  }

  if (!useThreadSanitizer && !useAddressSanitizer) {
    WriteLaunchActionAdditionalOption(xout, "DYLD_INSERT_LIBRARIES",
                                      "/usr/lib/libgmalloc.dylib",
                                      "XCODE_SCHEME_GUARD_MALLOC");
  }

  WriteLaunchActionAdditionalOption(xout, "NSZombieEnabled", "YES",
                                    "XCODE_SCHEME_ZOMBIE_OBJECTS");

  if (!useThreadSanitizer && !useAddressSanitizer) {
    WriteLaunchActionAdditionalOption(xout, "MallocStackLogging", "",
                                      "XCODE_SCHEME_MALLOC_STACK");
  }

  WriteLaunchActionAdditionalOption(xout, "DYLD_PRINT_APIS", "",
                                    "XCODE_SCHEME_DYNAMIC_LINKER_API_USAGE");

  WriteLaunchActionAdditionalOption(xout, "DYLD_PRINT_LIBRARIES", "",
                                    "XCODE_SCHEME_DYNAMIC_LIBRARY_LOADS");

  xout.EndElement();

  xout.EndElement(); // LaunchAction
}

bool cmXCodeScheme::WriteLaunchActionAttribute(cmXMLWriter& xout,
                                               const std::string& attrName,
                                               const std::string& varName)
{
  if (Target->GetTarget()->GetPropertyAsBool(varName)) {
    xout.Attribute(attrName.c_str(), "YES");
    return true;
  }
  return false;
}

bool cmXCodeScheme::WriteLaunchActionAdditionalOption(
  cmXMLWriter& xout, const std::string& key, const std::string& value,
  const std::string& varName)
{
  if (Target->GetTarget()->GetPropertyAsBool(varName)) {
    xout.StartElement("AdditionalOption");
    xout.BreakAttributes();

    xout.Attribute("key", key);
    xout.Attribute("value", value);
    xout.Attribute("isEnabled", "YES");

    xout.EndElement(); // AdditionalOption

    return true;
  }
  return false;
}

void cmXCodeScheme::WriteProfileAction(cmXMLWriter& xout,
                                       const std::string& configuration)
{
  xout.StartElement("ProfileAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.Attribute("shouldUseLaunchSchemeArgsEnv", "YES");
  xout.Attribute("savedToolIdentifier", "");
  xout.Attribute("useCustomWorkingDirectory", "NO");
  xout.Attribute("debugDocumentVersioning", "YES");
  xout.EndElement();
}

void cmXCodeScheme::WriteAnalyzeAction(cmXMLWriter& xout,
                                       const std::string& configuration)
{
  xout.StartElement("AnalyzeAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.EndElement();
}

void cmXCodeScheme::WriteArchiveAction(cmXMLWriter& xout,
                                       const std::string& configuration)
{
  xout.StartElement("ArchiveAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.Attribute("revealArchiveInOrganizer", "YES");
  xout.EndElement();
}

void cmXCodeScheme::WriteBuildableReference(cmXMLWriter& xout,
                                            const cmXCodeObject* xcObj,
                                            const std::string& container)
{
  xout.StartElement("BuildableReference");
  xout.BreakAttributes();
  xout.Attribute("BuildableIdentifier", "primary");
  xout.Attribute("BlueprintIdentifier", xcObj->GetId());
  xout.Attribute("BuildableName", xcObj->GetTarget()->GetFullName());
  xout.Attribute("BlueprintName", xcObj->GetTarget()->GetName());
  xout.Attribute("ReferencedContainer", "container:" + container);
  xout.EndElement();
}

std::string cmXCodeScheme::WriteVersionString()
{
  std::ostringstream v;
  v << std::setfill('0') << std::setw(4) << this->XcodeVersion * 10;
  return v.str();
}

std::string cmXCodeScheme::FindConfiguration(const std::string& name)
{
  // Try to find the desired configuration by name,
  // and if it's not found return first from the list
  //
  if (std::find(this->ConfigList.begin(), this->ConfigList.end(), name) ==
        this->ConfigList.end() &&
      !this->ConfigList.empty()) {
    return this->ConfigList[0];
  }

  return name;
}

bool cmXCodeScheme::IsTestable() const
{
  return !this->Tests.empty() || IsExecutable(this->Target);
}

bool cmXCodeScheme::IsExecutable(const cmXCodeObject* target)
{
  cmGeneratorTarget* gt = target->GetTarget();
  if (!gt) {
    cmSystemTools::Error("Error no target on xobject\n");
    return false;
  }

  return gt->GetType() == cmStateEnums::EXECUTABLE;
}
