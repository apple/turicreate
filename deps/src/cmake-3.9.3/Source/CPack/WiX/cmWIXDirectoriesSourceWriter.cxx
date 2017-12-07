/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXDirectoriesSourceWriter.h"

cmWIXDirectoriesSourceWriter::cmWIXDirectoriesSourceWriter(
  cmCPackLog* logger, std::string const& filename, GuidType componentGuidType)
  : cmWIXSourceWriter(logger, filename, componentGuidType)
{
}

void cmWIXDirectoriesSourceWriter::EmitStartMenuFolder(
  std::string const& startMenuFolder)
{
  BeginElement("Directory");
  AddAttribute("Id", "ProgramMenuFolder");

  BeginElement("Directory");
  AddAttribute("Id", "PROGRAM_MENU_FOLDER");
  AddAttribute("Name", startMenuFolder);
  EndElement("Directory");

  EndElement("Directory");
}

void cmWIXDirectoriesSourceWriter::EmitDesktopFolder()
{
  BeginElement("Directory");
  AddAttribute("Id", "DesktopFolder");
  AddAttribute("Name", "Desktop");
  EndElement("Directory");
}

void cmWIXDirectoriesSourceWriter::EmitStartupFolder()
{
  BeginElement("Directory");
  AddAttribute("Id", "StartupFolder");
  AddAttribute("Name", "Startup");
  EndElement("Directory");
}

size_t cmWIXDirectoriesSourceWriter::BeginInstallationPrefixDirectory(
  std::string const& programFilesFolderId,
  std::string const& installRootString)
{
  size_t offset = 1;
  if (!programFilesFolderId.empty()) {
    BeginElement("Directory");
    AddAttribute("Id", programFilesFolderId);
    offset = 0;
  }

  std::vector<std::string> installRoot;

  cmSystemTools::SplitPath(installRootString.c_str(), installRoot);

  if (!installRoot.empty() && installRoot.back().empty()) {
    installRoot.pop_back();
  }

  for (size_t i = 1; i < installRoot.size(); ++i) {
    BeginElement("Directory");

    if (i == installRoot.size() - 1) {
      AddAttribute("Id", "INSTALL_ROOT");
    } else {
      std::ostringstream tmp;
      tmp << "INSTALL_PREFIX_" << i;
      AddAttribute("Id", tmp.str());
    }

    AddAttribute("Name", installRoot[i]);
  }

  return installRoot.size() - offset;
}

void cmWIXDirectoriesSourceWriter::EndInstallationPrefixDirectory(size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    EndElement("Directory");
  }
}
