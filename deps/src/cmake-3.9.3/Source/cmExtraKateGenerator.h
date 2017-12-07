/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExtraKateGenerator_h
#define cmExtraKateGenerator_h

#include "cmConfigure.h"

#include "cmExternalMakefileProjectGenerator.h"

#include <string>

class cmGeneratedFileStream;
class cmLocalGenerator;

/** \class cmExtraKateGenerator
 * \brief Write Kate project files for Makefile or ninja based projects
 */
class cmExtraKateGenerator : public cmExternalMakefileProjectGenerator
{
public:
  cmExtraKateGenerator();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  void Generate() CM_OVERRIDE;

private:
  void CreateKateProjectFile(const cmLocalGenerator* lg) const;
  void CreateDummyKateProjectFile(const cmLocalGenerator* lg) const;
  void WriteTargets(const cmLocalGenerator* lg,
                    cmGeneratedFileStream& fout) const;
  void AppendTarget(cmGeneratedFileStream& fout, const std::string& target,
                    const std::string& make, const std::string& makeArgs,
                    const std::string& path, const char* homeOutputDir) const;

  std::string GenerateFilesString(const cmLocalGenerator* lg) const;
  std::string GetPathBasename(const std::string& path) const;
  std::string GenerateProjectName(const std::string& name,
                                  const std::string& type,
                                  const std::string& path) const;

  std::string ProjectName;
  bool UseNinja;
};

#endif
