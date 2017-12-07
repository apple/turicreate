/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackPKGGenerator.h"

#include <vector>

#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"

cmCPackPKGGenerator::cmCPackPKGGenerator()
{
  this->componentPackageMethod = ONE_PACKAGE;
}

cmCPackPKGGenerator::~cmCPackPKGGenerator()
{
}

bool cmCPackPKGGenerator::SupportsComponentInstallation() const
{
  return true;
}

int cmCPackPKGGenerator::InitializeInternal()
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "cmCPackPKGGenerator::Initialize()"
                  << std::endl);

  return this->Superclass::InitializeInternal();
}

std::string cmCPackPKGGenerator::GetPackageName(
  const cmCPackComponent& component)
{
  if (component.ArchiveFile.empty()) {
    std::string packagesDir = this->GetOption("CPACK_TEMPORARY_DIRECTORY");
    packagesDir += ".dummy";
    std::ostringstream out;
    out << cmSystemTools::GetFilenameWithoutLastExtension(packagesDir) << "-"
        << component.Name << ".pkg";
    return out.str();
  } else {
    return component.ArchiveFile + ".pkg";
  }
}

void cmCPackPKGGenerator::WriteDistributionFile(const char* metapackageFile)
{
  std::string distributionTemplate =
    this->FindTemplate("CPack.distribution.dist.in");
  if (distributionTemplate.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find input file: "
                    << distributionTemplate << std::endl);
    return;
  }

  std::string distributionFile = metapackageFile;
  distributionFile += "/Contents/distribution.dist";

  // Create the choice outline, which provides a tree-based view of
  // the components in their groups.
  std::ostringstream choiceOut;
  cmXMLWriter xout(choiceOut, 1);
  xout.StartElement("choices-outline");

  // Emit the outline for the groups
  std::map<std::string, cmCPackComponentGroup>::iterator groupIt;
  for (groupIt = this->ComponentGroups.begin();
       groupIt != this->ComponentGroups.end(); ++groupIt) {
    if (groupIt->second.ParentGroup == 0) {
      CreateChoiceOutline(groupIt->second, xout);
    }
  }

  // Emit the outline for the non-grouped components
  std::map<std::string, cmCPackComponent>::iterator compIt;
  for (compIt = this->Components.begin(); compIt != this->Components.end();
       ++compIt) {
    if (!compIt->second.Group) {
      xout.StartElement("line");
      xout.Attribute("choice", compIt->first + "Choice");
      xout.Content(""); // Avoid self-closing tag.
      xout.EndElement();
    }
  }
  if (!this->PostFlightComponent.Name.empty()) {
    xout.StartElement("line");
    xout.Attribute("choice", PostFlightComponent.Name + "Choice");
    xout.Content(""); // Avoid self-closing tag.
    xout.EndElement();
  }
  xout.EndElement(); // choices-outline>

  // Create the actual choices
  for (groupIt = this->ComponentGroups.begin();
       groupIt != this->ComponentGroups.end(); ++groupIt) {
    CreateChoice(groupIt->second, xout);
  }
  for (compIt = this->Components.begin(); compIt != this->Components.end();
       ++compIt) {
    CreateChoice(compIt->second, xout);
  }

  if (!this->PostFlightComponent.Name.empty()) {
    CreateChoice(PostFlightComponent, xout);
  }

  this->SetOption("CPACK_PACKAGEMAKER_CHOICES", choiceOut.str().c_str());

  // Create the distribution.dist file in the metapackage to turn it
  // into a distribution package.
  this->ConfigureFile(distributionTemplate.c_str(), distributionFile.c_str());
}

void cmCPackPKGGenerator::CreateChoiceOutline(
  const cmCPackComponentGroup& group, cmXMLWriter& xout)
{
  xout.StartElement("line");
  xout.Attribute("choice", group.Name + "Choice");
  std::vector<cmCPackComponentGroup*>::const_iterator groupIt;
  for (groupIt = group.Subgroups.begin(); groupIt != group.Subgroups.end();
       ++groupIt) {
    CreateChoiceOutline(**groupIt, xout);
  }

  std::vector<cmCPackComponent*>::const_iterator compIt;
  for (compIt = group.Components.begin(); compIt != group.Components.end();
       ++compIt) {
    xout.StartElement("line");
    xout.Attribute("choice", (*compIt)->Name + "Choice");
    xout.Content(""); // Avoid self-closing tag.
    xout.EndElement();
  }
  xout.EndElement();
}

void cmCPackPKGGenerator::CreateChoice(const cmCPackComponentGroup& group,
                                       cmXMLWriter& xout)
{
  xout.StartElement("choice");
  xout.Attribute("id", group.Name + "Choice");
  xout.Attribute("title", group.DisplayName);
  xout.Attribute("start_selected", "true");
  xout.Attribute("start_enabled", "true");
  xout.Attribute("start_visible", "true");
  if (!group.Description.empty()) {
    xout.Attribute("description", group.Description);
  }
  xout.EndElement();
}

void cmCPackPKGGenerator::CreateChoice(const cmCPackComponent& component,
                                       cmXMLWriter& xout)
{
  std::string packageId = "com.";
  packageId += this->GetOption("CPACK_PACKAGE_VENDOR");
  packageId += '.';
  packageId += this->GetOption("CPACK_PACKAGE_NAME");
  packageId += '.';
  packageId += component.Name;

  xout.StartElement("choice");
  xout.Attribute("id", component.Name + "Choice");
  xout.Attribute("title", component.DisplayName);
  xout.Attribute(
    "start_selected",
    component.IsDisabledByDefault && !component.IsRequired ? "false" : "true");
  xout.Attribute("start_enabled", component.IsRequired ? "false" : "true");
  xout.Attribute("start_visible", component.IsHidden ? "false" : "true");
  if (!component.Description.empty()) {
    xout.Attribute("description", component.Description);
  }
  if (!component.Dependencies.empty() ||
      !component.ReverseDependencies.empty()) {
    // The "selected" expression is evaluated each time any choice is
    // selected, for all choices *except* the one that the user
    // selected. A component is marked selected if it has been
    // selected (my.choice.selected in Javascript) and all of the
    // components it depends on have been selected (transitively) or
    // if any of the components that depend on it have been selected
    // (transitively). Assume that we have components A, B, C, D, and
    // E, where each component depends on the previous component (B
    // depends on A, C depends on B, D depends on C, and E depends on
    // D). The expression we build for the component C will be
    //   my.choice.selected && B && A || D || E
    // This way, selecting C will automatically select everything it depends
    // on (B and A), while selecting something that depends on C--either D
    // or E--will automatically cause C to get selected.
    std::ostringstream selected("my.choice.selected");
    std::set<const cmCPackComponent*> visited;
    AddDependencyAttributes(component, visited, selected);
    visited.clear();
    AddReverseDependencyAttributes(component, visited, selected);
    xout.Attribute("selected", selected.str());
  }
  xout.StartElement("pkg-ref");
  xout.Attribute("id", packageId);
  xout.EndElement(); // pkg-ref
  xout.EndElement(); // choice

  // Create a description of the package associated with this
  // component.
  std::string relativePackageLocation = "Contents/Packages/";
  relativePackageLocation += this->GetPackageName(component);

  // Determine the installed size of the package.
  std::string dirName = this->GetOption("CPACK_TEMPORARY_DIRECTORY");
  dirName += '/';
  dirName += component.Name;
  dirName += this->GetOption("CPACK_PACKAGING_INSTALL_PREFIX");
  unsigned long installedSize = component.GetInstalledSizeInKbytes(dirName);

  xout.StartElement("pkg-ref");
  xout.Attribute("id", packageId);
  xout.Attribute("version", this->GetOption("CPACK_PACKAGE_VERSION"));
  xout.Attribute("installKBytes", installedSize);
  xout.Attribute("auth", "Admin");
  xout.Attribute("onConclusion", "None");
  if (component.IsDownloaded) {
    xout.Content(this->GetOption("CPACK_DOWNLOAD_SITE"));
    xout.Content(this->GetPackageName(component));
  } else {
    xout.Content("file:./");
    xout.Content(relativePackageLocation);
  }
  xout.EndElement(); // pkg-ref
}

void cmCPackPKGGenerator::AddDependencyAttributes(
  const cmCPackComponent& component,
  std::set<const cmCPackComponent*>& visited, std::ostringstream& out)
{
  if (visited.find(&component) != visited.end()) {
    return;
  }
  visited.insert(&component);

  std::vector<cmCPackComponent*>::const_iterator dependIt;
  for (dependIt = component.Dependencies.begin();
       dependIt != component.Dependencies.end(); ++dependIt) {
    out << " && choices['" << (*dependIt)->Name << "Choice'].selected";
    AddDependencyAttributes(**dependIt, visited, out);
  }
}

void cmCPackPKGGenerator::AddReverseDependencyAttributes(
  const cmCPackComponent& component,
  std::set<const cmCPackComponent*>& visited, std::ostringstream& out)
{
  if (visited.find(&component) != visited.end()) {
    return;
  }
  visited.insert(&component);

  std::vector<cmCPackComponent*>::const_iterator dependIt;
  for (dependIt = component.ReverseDependencies.begin();
       dependIt != component.ReverseDependencies.end(); ++dependIt) {
    out << " || choices['" << (*dependIt)->Name << "Choice'].selected";
    AddReverseDependencyAttributes(**dependIt, visited, out);
  }
}

bool cmCPackPKGGenerator::CopyCreateResourceFile(const std::string& name,
                                                 const std::string& dirName)
{
  std::string uname = cmSystemTools::UpperCase(name);
  std::string cpackVar = "CPACK_RESOURCE_FILE_" + uname;
  const char* inFileName = this->GetOption(cpackVar);
  if (!inFileName) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "CPack option: "
                    << cpackVar.c_str()
                    << " not specified. It should point to "
                    << (!name.empty() ? name : "<empty>") << ".rtf, " << name
                    << ".html, or " << name << ".txt file" << std::endl);
    return false;
  }
  if (!cmSystemTools::FileExists(inFileName)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot find "
                    << (!name.empty() ? name : "<empty>")
                    << " resource file: " << inFileName << std::endl);
    return false;
  }
  std::string ext = cmSystemTools::GetFilenameLastExtension(inFileName);
  if (ext != ".rtfd" && ext != ".rtf" && ext != ".html" && ext != ".txt") {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR, "Bad file extension specified: "
        << ext
        << ". Currently only .rtfd, .rtf, .html, and .txt files allowed."
        << std::endl);
    return false;
  }

  std::string destFileName = dirName;
  destFileName += '/';
  destFileName += name + ext;

  // Set this so that distribution.dist gets the right name (without
  // the path).
  this->SetOption("CPACK_RESOURCE_FILE_" + uname + "_NOPATH",
                  (name + ext).c_str());

  cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                "Configure file: " << (inFileName ? inFileName : "(NULL)")
                                   << " to " << destFileName << std::endl);
  this->ConfigureFile(inFileName, destFileName.c_str());
  return true;
}

bool cmCPackPKGGenerator::CopyResourcePlistFile(const std::string& name,
                                                const char* outName)
{
  if (!outName) {
    outName = name.c_str();
  }

  std::string inFName = "CPack.";
  inFName += name;
  inFName += ".in";
  std::string inFileName = this->FindTemplate(inFName.c_str());
  if (inFileName.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot find input file: " << inFName << std::endl);
    return false;
  }

  std::string destFileName = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  destFileName += "/";
  destFileName += outName;

  cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Configure file: "
                  << inFileName << " to " << destFileName << std::endl);
  this->ConfigureFile(inFileName.c_str(), destFileName.c_str());
  return true;
}

int cmCPackPKGGenerator::CopyInstallScript(const std::string& resdir,
                                           const std::string& script,
                                           const std::string& name)
{
  std::string dst = resdir;
  dst += "/";
  dst += name;
  cmSystemTools::CopyFileAlways(script, dst);
  cmSystemTools::SetPermissions(dst.c_str(), 0777);
  cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                "copy script : " << script << "\ninto " << dst << std::endl);

  return 1;
}
