/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXCodeScheme.h"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmXMLSafe.h"

cmXCodeScheme::cmXCodeScheme(cmXCodeObject* xcObj,
                             const std::vector<std::string>& configList,
                             unsigned int xcVersion)
  : Target(xcObj)
  , TargetName(xcObj->GetTarget()->GetName())
  , BuildableName(xcObj->GetTarget()->GetFullName())
  , TargetId(xcObj->GetId())
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
  WriteTestAction(xout, FindConfiguration("Debug"));
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

  xout.StartElement("BuildableReference");
  xout.BreakAttributes();
  xout.Attribute("BuildableIdentifier", "primary");
  xout.Attribute("BlueprintIdentifier", this->TargetId);
  xout.Attribute("BuildableName", this->BuildableName);
  xout.Attribute("BlueprintName", this->TargetName);
  xout.Attribute("ReferencedContainer", "container:" + container);
  xout.EndElement();

  xout.EndElement(); // BuildActionEntry
  xout.EndElement(); // BuildActionEntries
  xout.EndElement(); // BuildAction
}

void cmXCodeScheme::WriteTestAction(cmXMLWriter& xout,
                                    std::string configuration)
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
  xout.EndElement();

  xout.StartElement("AdditionalOptions");
  xout.EndElement();

  xout.EndElement(); // TestAction
}

void cmXCodeScheme::WriteLaunchAction(cmXMLWriter& xout,
                                      std::string configuration,
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

  if (IsExecutable(this->Target)) {
    xout.StartElement("BuildableProductRunnable");
    xout.BreakAttributes();
    xout.Attribute("runnableDebuggingMode", "0");

  } else {
    xout.StartElement("MacroExpansion");
  }

  xout.StartElement("BuildableReference");
  xout.BreakAttributes();
  xout.Attribute("BuildableIdentifier", "primary");
  xout.Attribute("BlueprintIdentifier", this->TargetId);
  xout.Attribute("BuildableName", this->BuildableName);
  xout.Attribute("BlueprintName", this->TargetName);
  xout.Attribute("ReferencedContainer", "container:" + container);
  xout.EndElement();

  xout.EndElement(); // MacroExpansion

  xout.StartElement("AdditionalOptions");
  xout.EndElement();

  xout.EndElement(); // LaunchAction
}

void cmXCodeScheme::WriteProfileAction(cmXMLWriter& xout,
                                       std::string configuration)
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
                                       std::string configuration)
{
  xout.StartElement("AnalyzeAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.EndElement();
}

void cmXCodeScheme::WriteArchiveAction(cmXMLWriter& xout,
                                       std::string configuration)
{
  xout.StartElement("ArchiveAction");
  xout.BreakAttributes();
  xout.Attribute("buildConfiguration", configuration);
  xout.Attribute("revealArchiveInOrganizer", "YES");
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
      this->ConfigList.size() > 0)
    return this->ConfigList[0];

  return name;
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
