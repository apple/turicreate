/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExtraCodeBlocksGenerator_h
#define cmExtraCodeBlocksGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmExternalMakefileProjectGenerator.h"

#include <string>
#include <vector>

class cmGeneratorTarget;
class cmLocalGenerator;
class cmMakefile;
class cmXMLWriter;

/** \class cmExtraCodeBlocksGenerator
 * \brief Write CodeBlocks project files for Makefile based projects
 */
class cmExtraCodeBlocksGenerator : public cmExternalMakefileProjectGenerator
{
public:
  cmExtraCodeBlocksGenerator();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  void Generate() override;

private:
  struct CbpUnit
  {
    std::vector<const cmGeneratorTarget*> Targets;
  };

  void CreateProjectFile(const std::vector<cmLocalGenerator*>& lgs);

  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                            const std::string& filename);
  std::string CreateDummyTargetFile(cmLocalGenerator* lg,
                                    cmGeneratorTarget* target) const;

  std::string GetCBCompilerId(const cmMakefile* mf);
  int GetCBTargetType(cmGeneratorTarget* target);
  std::string BuildMakeCommand(const std::string& make, const char* makefile,
                               const std::string& target,
                               const std::string& makeFlags);
  void AppendTarget(cmXMLWriter& xml, const std::string& targetName,
                    cmGeneratorTarget* target, const char* make,
                    const cmLocalGenerator* lg, const char* compiler,
                    const std::string& makeFlags);
};

#endif
