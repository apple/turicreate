/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalXCodeGenerator_h
#define cmLocalXCodeGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

#include "cmLocalGenerator.h"

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;
class cmSourceFile;

/** \class cmLocalXCodeGenerator
 * \brief Write a local Xcode project
 *
 * cmLocalXCodeGenerator produces a LocalUnix makefile from its
 * member Makefile.
 */
class cmLocalXCodeGenerator : public cmLocalGenerator
{
public:
  ///! Set cache only and recurse to false by default.
  cmLocalXCodeGenerator(cmGlobalGenerator* gg, cmMakefile* mf);

  virtual ~cmLocalXCodeGenerator();
  virtual std::string GetTargetDirectory(
    cmGeneratorTarget const* target) const;
  virtual void AppendFlagEscape(std::string& flags,
                                const std::string& rawFlag);
  virtual void Generate();
  virtual void GenerateInstallRules();
  virtual void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt = 0);

private:
};

#endif
