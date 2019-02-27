/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallExportAndroidMKGenerator_h
#define cmInstallExportAndroidMKGenerator_h

#include "cmInstallExportGenerator.h"

class cmExportInstallFileGenerator;
class cmInstallFilesGenerator;
class cmInstallTargetGenerator;
class cmExportSet;
class cmMakefile;

/** \class cmInstallExportAndroidMKGenerator
 * \brief Generate rules for creating an export files.
 */
class cmInstallExportAndroidMKGenerator : public cmInstallExportGenerator
{
public:
  cmInstallExportAndroidMKGenerator(
    cmExportSet* exportSet, const char* dest, const char* file_permissions,
    const std::vector<std::string>& configurations, const char* component,
    MessageLevel message, bool exclude_from_all, const char* filename,
    const char* name_space, bool exportOld);
  ~cmInstallExportAndroidMKGenerator();

  void Compute(cmLocalGenerator* lg);

protected:
  virtual void GenerateScript(std::ostream& os);
  virtual void GenerateScriptConfigs(std::ostream& os, Indent const& indent);
  virtual void GenerateScriptActions(std::ostream& os, Indent const& indent);
  void GenerateImportFile(cmExportSet const* exportSet);
  void GenerateImportFile(const char* config, cmExportSet const* exportSet);
};

#endif
