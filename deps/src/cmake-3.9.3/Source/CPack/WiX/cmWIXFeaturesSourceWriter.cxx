/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXFeaturesSourceWriter.h"

cmWIXFeaturesSourceWriter::cmWIXFeaturesSourceWriter(
  cmCPackLog* logger, std::string const& filename, GuidType componentGuidType)
  : cmWIXSourceWriter(logger, filename, componentGuidType)
{
}

void cmWIXFeaturesSourceWriter::CreateCMakePackageRegistryEntry(
  std::string const& package, std::string const& upgradeGuid)
{
  BeginElement("Component");
  AddAttribute("Id", "CM_PACKAGE_REGISTRY");
  AddAttribute("Directory", "TARGETDIR");
  AddAttribute("Guid", CreateGuidFromComponentId("CM_PACKAGE_REGISTRY"));

  std::string registryKey =
    std::string("Software\\Kitware\\CMake\\Packages\\") + package;

  BeginElement("RegistryValue");
  AddAttribute("Root", "HKLM");
  AddAttribute("Key", registryKey);
  AddAttribute("Name", upgradeGuid);
  AddAttribute("Type", "string");
  AddAttribute("Value", "[INSTALL_ROOT]");
  AddAttribute("KeyPath", "yes");
  EndElement("RegistryValue");

  EndElement("Component");
}

void cmWIXFeaturesSourceWriter::EmitFeatureForComponentGroup(
  cmCPackComponentGroup const& group, cmWIXPatch& patch)
{
  BeginElement("Feature");
  AddAttribute("Id", "CM_G_" + group.Name);

  if (group.IsExpandedByDefault) {
    AddAttribute("Display", "expand");
  }

  AddAttributeUnlessEmpty("Title", group.DisplayName);
  AddAttributeUnlessEmpty("Description", group.Description);

  patch.ApplyFragment("CM_G_" + group.Name, *this);

  for (std::vector<cmCPackComponentGroup*>::const_iterator i =
         group.Subgroups.begin();
       i != group.Subgroups.end(); ++i) {
    EmitFeatureForComponentGroup(**i, patch);
  }

  for (std::vector<cmCPackComponent*>::const_iterator i =
         group.Components.begin();
       i != group.Components.end(); ++i) {
    EmitFeatureForComponent(**i, patch);
  }

  EndElement("Feature");
}

void cmWIXFeaturesSourceWriter::EmitFeatureForComponent(
  cmCPackComponent const& component, cmWIXPatch& patch)
{
  BeginElement("Feature");
  AddAttribute("Id", "CM_C_" + component.Name);

  AddAttributeUnlessEmpty("Title", component.DisplayName);
  AddAttributeUnlessEmpty("Description", component.Description);

  if (component.IsRequired) {
    AddAttribute("Absent", "disallow");
  }

  if (component.IsHidden) {
    AddAttribute("Display", "hidden");
  }

  if (component.IsDisabledByDefault) {
    AddAttribute("Level", "2");
  }

  patch.ApplyFragment("CM_C_" + component.Name, *this);

  EndElement("Feature");
}

void cmWIXFeaturesSourceWriter::EmitComponentRef(std::string const& id)
{
  BeginElement("ComponentRef");
  AddAttribute("Id", id);
  EndElement("ComponentRef");
}
