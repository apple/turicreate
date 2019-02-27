/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalCommonGenerator_h
#define cmLocalCommonGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

#include "cmLocalGenerator.h"

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;
class cmSourceFile;

/** \class cmLocalCommonGenerator
 * \brief Common infrastructure for Makefile and Ninja local generators.
 */
class cmLocalCommonGenerator : public cmLocalGenerator
{
public:
  cmLocalCommonGenerator(cmGlobalGenerator* gg, cmMakefile* mf,
                         std::string const& wd);
  ~cmLocalCommonGenerator() override;

  std::string const& GetConfigName() { return this->ConfigName; }

  std::string GetWorkingDirectory() const { return this->WorkingDirectory; }

  std::string GetTargetFortranFlags(cmGeneratorTarget const* target,
                                    std::string const& config) override;

  void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt = nullptr) override;

protected:
  std::string WorkingDirectory;

  std::string ConfigName;

  friend class cmCommonTargetGenerator;
};

#endif
