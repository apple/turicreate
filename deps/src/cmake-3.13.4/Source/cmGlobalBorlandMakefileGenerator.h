/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalBorlandMakefileGenerator_h
#define cmGlobalBorlandMakefileGenerator_h

#include "cmGlobalNMakeMakefileGenerator.h"

#include <iosfwd>

/** \class cmGlobalBorlandMakefileGenerator
 * \brief Write a Borland makefiles.
 *
 * cmGlobalBorlandMakefileGenerator manages nmake build process for a tree
 */
class cmGlobalBorlandMakefileGenerator : public cmGlobalUnixMakefileGenerator3
{
public:
  cmGlobalBorlandMakefileGenerator(cmake* cm);
  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<
      cmGlobalBorlandMakefileGenerator>();
  }

  ///! Get the name for the generator.
  std::string GetName() const override
  {
    return cmGlobalBorlandMakefileGenerator::GetActualName();
  }
  static std::string GetActualName() { return "Borland Makefiles"; }

  /** Get the documentation entry for this generator.  */
  static void GetDocumentation(cmDocumentationEntry& entry);

  ///! Create a local generator appropriate to this Global Generator
  cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf) override;

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  void EnableLanguage(std::vector<std::string> const& languages, cmMakefile*,
                      bool optional) override;

  bool AllowNotParallel() const override { return false; }
  bool AllowDeleteOnError() const override { return false; }

protected:
  void GenerateBuildCommand(std::vector<std::string>& makeCommand,
                            const std::string& makeProgram,
                            const std::string& projectName,
                            const std::string& projectDir,
                            const std::string& targetName,
                            const std::string& config, bool fast, int jobs,
                            bool verbose,
                            std::vector<std::string> const& makeOptions =
                              std::vector<std::string>()) override;

  void PrintBuildCommandAdvice(std::ostream& os, int jobs) const override;
};

#endif
