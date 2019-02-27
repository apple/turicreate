/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExtraSublimeTextGenerator_h
#define cmExtraSublimeTextGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmExternalMakefileProjectGenerator.h"

#include <map>
#include <string>
#include <vector>

class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmLocalGenerator;
class cmMakefile;
class cmSourceFile;

/** \class cmExtraSublimeTextGenerator
 * \brief Write Sublime Text 2 project files for Makefile based projects
 */
class cmExtraSublimeTextGenerator : public cmExternalMakefileProjectGenerator
{
public:
  static cmExternalMakefileProjectGeneratorFactory* GetFactory();
  typedef std::map<std::string, std::vector<std::string>> MapSourceFileFlags;
  cmExtraSublimeTextGenerator();

  void Generate() override;

private:
  void CreateProjectFile(const std::vector<cmLocalGenerator*>& lgs);

  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                            const std::string& filename);

  /** Appends all targets as build systems to the project file and get all
   * include directories and compiler definitions used.
   */
  void AppendAllTargets(const std::vector<cmLocalGenerator*>& lgs,
                        const cmMakefile* mf, cmGeneratedFileStream& fout,
                        MapSourceFileFlags& sourceFileFlags);
  /** Returns the build command that needs to be executed to build the
   *  specified target.
   */
  std::string BuildMakeCommand(const std::string& make, const char* makefile,
                               const std::string& target);
  /** Appends the specified target to the generated project file as a Sublime
   *  Text build system.
   */
  void AppendTarget(cmGeneratedFileStream& fout, const std::string& targetName,
                    cmLocalGenerator* lg, cmGeneratorTarget* target,
                    const char* make, const cmMakefile* makefile,
                    const char* compiler, MapSourceFileFlags& sourceFileFlags,
                    bool firstTarget);
  /**
   * Compute the flags for compilation of object files for a given @a language.
   * @note Generally it is the value of the variable whose name is computed
   *       by LanguageFlagsVarName().
   */
  std::string ComputeFlagsForObject(cmSourceFile* source, cmLocalGenerator* lg,
                                    cmGeneratorTarget* gtgt);

  std::string ComputeDefines(cmSourceFile* source, cmLocalGenerator* lg,
                             cmGeneratorTarget* gtgt);

  std::string ComputeIncludes(cmSourceFile* source, cmLocalGenerator* lg,
                              cmGeneratorTarget* gtgt);

  bool Open(const std::string& bindir, const std::string& projectName,
            bool dryRun) override;

  bool ExcludeBuildFolder;
  std::string EnvSettings;
};

#endif
