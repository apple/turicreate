/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCommonTargetGenerator_h
#define cmCommonTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

class cmGeneratorTarget;
class cmGlobalCommonGenerator;
class cmLinkLineComputer;
class cmLocalCommonGenerator;
class cmMakefile;
class cmSourceFile;

/** \class cmCommonTargetGenerator
 * \brief Common infrastructure for Makefile and Ninja per-target generators
 */
class cmCommonTargetGenerator
{
public:
  cmCommonTargetGenerator(cmGeneratorTarget* gt);
  virtual ~cmCommonTargetGenerator();

  std::string const& GetConfigName() const;

protected:
  // Feature query methods.
  const char* GetFeature(const std::string& feature);

  // Helper to add flag for windows .def file.
  void AddModuleDefinitionFlag(cmLinkLineComputer* linkLineComputer,
                               std::string& flags);

  cmGeneratorTarget* GeneratorTarget;
  cmMakefile* Makefile;
  cmLocalCommonGenerator* LocalGenerator;
  cmGlobalCommonGenerator* GlobalGenerator;
  std::string ConfigName;

  void AppendFortranFormatFlags(std::string& flags,
                                cmSourceFile const& source);

  virtual void AddIncludeFlags(std::string& flags,
                               std::string const& lang) = 0;

  void AppendOSXVerFlag(std::string& flags, const std::string& lang,
                        const char* name, bool so);

  typedef std::map<std::string, std::string> ByLanguageMap;
  std::string GetFlags(const std::string& l);
  ByLanguageMap FlagsByLanguage;
  std::string GetDefines(const std::string& l);
  ByLanguageMap DefinesByLanguage;
  std::string GetIncludes(std::string const& l);
  ByLanguageMap IncludesByLanguage;
  std::string GetManifests();

  std::vector<std::string> GetLinkedTargetDirectories() const;
  std::string ComputeTargetCompilePDB() const;
};

#endif
