/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio9Generator.h"

#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio7Generator.h"
#include "cmMakefile.h"
#include "cmVisualStudioWCEPlatformParser.h"
#include "cmake.h"

static const char vs9generatorName[] = "Visual Studio 9 2008";

class cmGlobalVisualStudio9Generator::Factory : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const override
  {
    if (strncmp(name.c_str(), vs9generatorName,
                sizeof(vs9generatorName) - 1) != 0) {
      return 0;
    }

    const char* p = name.c_str() + sizeof(vs9generatorName) - 1;
    if (p[0] == '\0') {
      return new cmGlobalVisualStudio9Generator(cm, name, "");
    }

    if (p[0] != ' ') {
      return 0;
    }

    ++p;

    if (!strcmp(p, "IA64")) {
      return new cmGlobalVisualStudio9Generator(cm, name, "Itanium");
    }

    if (!strcmp(p, "Win64")) {
      return new cmGlobalVisualStudio9Generator(cm, name, "x64");
    }

    cmVisualStudioWCEPlatformParser parser(p);
    parser.ParseVersion("9.0");
    if (!parser.Found()) {
      return 0;
    }

    cmGlobalVisualStudio9Generator* ret =
      new cmGlobalVisualStudio9Generator(cm, name, p);
    ret->WindowsCEVersion = parser.GetOSVersion();
    return ret;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const override
  {
    entry.Name = std::string(vs9generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2008 project files.  "
                  "Optional [arch] can be \"Win64\" or \"IA64\".";
  }

  void GetGenerators(std::vector<std::string>& names) const override
  {
    names.push_back(vs9generatorName);
    names.push_back(vs9generatorName + std::string(" Win64"));
    names.push_back(vs9generatorName + std::string(" IA64"));
    cmVisualStudioWCEPlatformParser parser;
    parser.ParseVersion("9.0");
    const std::vector<std::string>& availablePlatforms =
      parser.GetAvailablePlatforms();
    for (std::string const& i : availablePlatforms) {
      names.push_back("Visual Studio 9 2008 " + i);
    }
  }

  bool SupportsToolset() const override { return false; }
  bool SupportsPlatform() const override { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio9Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio9Generator::cmGlobalVisualStudio9Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio8Generator(cm, name, platformName)
{
  this->Version = VS9;
  std::string vc9Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\9.0\\Setup\\VC;"
    "ProductDir",
    vc9Express, cmSystemTools::KeyWOW64_32);
}

void cmGlobalVisualStudio9Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 10.00\n";
  fout << "# Visual Studio 2008\n";
}

std::string cmGlobalVisualStudio9Generator::GetUserMacrosDirectory()
{
  std::string base;
  std::string path;

  // base begins with the VisualStudioProjectsLocation reg value...
  if (cmSystemTools::ReadRegistryValue(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\VisualStudio\\9.0;"
        "VisualStudioProjectsLocation",
        base)) {
    cmSystemTools::ConvertToUnixSlashes(base);

    // 9.0 macros folder:
    path = base + "/VSMacros80";
    // *NOT* a typo; right now in Visual Studio 2008 beta the macros
    // folder is VSMacros80... They may change it to 90 before final
    // release of 2008 or they may not... we'll have to keep our eyes
    // on it
  }

  // path is (correctly) still empty if we did not read the base value from
  // the Registry value
  return path;
}

std::string cmGlobalVisualStudio9Generator::GetUserMacrosRegKeyBase()
{
  return "Software\\Microsoft\\VisualStudio\\9.0\\vsmacros";
}
