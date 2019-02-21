/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalVisualStudio10Generator_h
#define cmLocalVisualStudio10Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmLocalVisualStudio7Generator.h"

class cmGlobalGenerator;
class cmMakefile;

/** \class cmLocalVisualStudio10Generator
 * \brief Write Visual Studio 10 project files.
 *
 * cmLocalVisualStudio10Generator produces a Visual Studio 10 project
 * file for each target in its directory.
 */
class cmLocalVisualStudio10Generator : public cmLocalVisualStudio7Generator
{
public:
  ///! Set cache only and recurse to false by default.
  cmLocalVisualStudio10Generator(cmGlobalGenerator* gg, cmMakefile* mf);

  virtual ~cmLocalVisualStudio10Generator();

  /**
   * Generate the makefile for this directory.
   */
  void Generate() override;
  void ReadAndStoreExternalGUID(const std::string& name,
                                const char* path) override;

  std::set<cmSourceFile const*>& GetSourcesVisited(cmGeneratorTarget* target)
  {
    return SourcesVisited[target];
  };

protected:
  const char* ReportErrorLabel() const override;
  bool CustomCommandUseLocal() const override { return true; }

private:
  void GenerateTargetsDepthFirst(cmGeneratorTarget* target,
                                 std::vector<cmGeneratorTarget*>& remaining);

  std::map<cmGeneratorTarget*, std::set<cmSourceFile const*>> SourcesVisited;
};
#endif
