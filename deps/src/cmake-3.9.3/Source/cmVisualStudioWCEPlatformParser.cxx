/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVisualStudioWCEPlatformParser.h"

#include "cmGlobalVisualStudioGenerator.h"
#include "cmXMLParser.h"

int cmVisualStudioWCEPlatformParser::ParseVersion(const char* version)
{
  const std::string registryBase =
    cmGlobalVisualStudioGenerator::GetRegistryBase(version);
  const std::string vckey = registryBase + "\\Setup\\VC;ProductDir";
  const std::string vskey = registryBase + "\\Setup\\VS;ProductDir";

  if (!cmSystemTools::ReadRegistryValue(vckey.c_str(), this->VcInstallDir,
                                        cmSystemTools::KeyWOW64_32) ||
      !cmSystemTools::ReadRegistryValue(vskey.c_str(), this->VsInstallDir,
                                        cmSystemTools::KeyWOW64_32)) {
    return 0;
  }
  cmSystemTools::ConvertToUnixSlashes(this->VcInstallDir);
  cmSystemTools::ConvertToUnixSlashes(this->VsInstallDir);
  this->VcInstallDir.append("/");
  this->VsInstallDir.append("/");

  const std::string configFilename =
    this->VcInstallDir + "vcpackages/WCE.VCPlatform.config";

  return this->ParseFile(configFilename.c_str());
}

std::string cmVisualStudioWCEPlatformParser::GetOSVersion() const
{
  if (this->OSMinorVersion.empty()) {
    return OSMajorVersion;
  }

  return OSMajorVersion + "." + OSMinorVersion;
}

const char* cmVisualStudioWCEPlatformParser::GetArchitectureFamily() const
{
  std::map<std::string, std::string>::const_iterator it =
    this->Macros.find("ARCHFAM");
  if (it != this->Macros.end()) {
    return it->second.c_str();
  }

  return 0;
}

void cmVisualStudioWCEPlatformParser::StartElement(const std::string& name,
                                                   const char** attributes)
{
  if (this->FoundRequiredName) {
    return;
  }

  this->CharacterData = "";

  if (name == "PlatformData") {
    this->PlatformName = "";
    this->OSMajorVersion = "";
    this->OSMinorVersion = "";
    this->Macros.clear();
  }

  if (name == "Macro") {
    std::string macroName;
    std::string macroValue;

    for (const char** attr = attributes; *attr; attr += 2) {
      if (strcmp(attr[0], "Name") == 0) {
        macroName = attr[1];
      } else if (strcmp(attr[0], "Value") == 0) {
        macroValue = attr[1];
      }
    }

    if (!macroName.empty()) {
      this->Macros[macroName] = macroValue;
    }
  } else if (name == "Directories") {
    for (const char** attr = attributes; *attr; attr += 2) {
      if (strcmp(attr[0], "Include") == 0) {
        this->Include = attr[1];
      } else if (strcmp(attr[0], "Library") == 0) {
        this->Library = attr[1];
      } else if (strcmp(attr[0], "Path") == 0) {
        this->Path = attr[1];
      }
    }
  }
}

void cmVisualStudioWCEPlatformParser::EndElement(const std::string& name)
{
  if (!this->RequiredName) {
    if (name == "PlatformName") {
      this->AvailablePlatforms.push_back(this->CharacterData);
    }
    return;
  }

  if (this->FoundRequiredName) {
    return;
  }

  if (name == "PlatformName") {
    this->PlatformName = this->CharacterData;
  } else if (name == "OSMajorVersion") {
    this->OSMajorVersion = this->CharacterData;
  } else if (name == "OSMinorVersion") {
    this->OSMinorVersion = this->CharacterData;
  } else if (name == "Platform") {
    if (this->PlatformName == this->RequiredName) {
      this->FoundRequiredName = true;
    }
  }
}

void cmVisualStudioWCEPlatformParser::CharacterDataHandler(const char* data,
                                                           int length)
{
  this->CharacterData.append(data, length);
}

std::string cmVisualStudioWCEPlatformParser::FixPaths(
  const std::string& paths) const
{
  std::string ret = paths;
  cmSystemTools::ReplaceString(ret, "$(PATH)", "%PATH%");
  cmSystemTools::ReplaceString(ret, "$(VCInstallDir)", VcInstallDir.c_str());
  cmSystemTools::ReplaceString(ret, "$(VSInstallDir)", VsInstallDir.c_str());
  std::replace(ret.begin(), ret.end(), '\\', '/');
  cmSystemTools::ReplaceString(ret, "//", "/");
  std::replace(ret.begin(), ret.end(), '/', '\\');
  return ret;
}
