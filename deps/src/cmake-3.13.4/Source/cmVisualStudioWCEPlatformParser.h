/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVisualStudioWCEPlatformParser_h
#define cmVisualStudioWCEPlatformParser_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <stddef.h>
#include <string>
#include <vector>

#include "cmXMLParser.h"

// This class is used to parse XML with configuration
// of installed SDKs in system
class cmVisualStudioWCEPlatformParser : public cmXMLParser
{
public:
  cmVisualStudioWCEPlatformParser(const char* name = NULL)
    : RequiredName(name)
    , FoundRequiredName(false)
  {
  }

  int ParseVersion(const char* version);

  bool Found() const { return this->FoundRequiredName; }
  const char* GetArchitectureFamily() const;
  std::string GetOSVersion() const;
  std::string GetIncludeDirectories() const
  {
    return this->FixPaths(this->Include);
  }
  std::string GetLibraryDirectories() const
  {
    return this->FixPaths(this->Library);
  }
  std::string GetPathDirectories() const { return this->FixPaths(this->Path); }
  const std::vector<std::string>& GetAvailablePlatforms() const
  {
    return this->AvailablePlatforms;
  }

protected:
  virtual void StartElement(const std::string& name, const char** attributes);
  void EndElement(const std::string& name);
  void CharacterDataHandler(const char* data, int length);

private:
  std::string FixPaths(const std::string& paths) const;

  std::string CharacterData;

  std::string Include;
  std::string Library;
  std::string Path;
  std::string PlatformName;
  std::string OSMajorVersion;
  std::string OSMinorVersion;
  std::map<std::string, std::string> Macros;
  std::vector<std::string> AvailablePlatforms;

  const char* RequiredName;
  bool FoundRequiredName;
  std::string VcInstallDir;
  std::string VsInstallDir;
};

#endif
