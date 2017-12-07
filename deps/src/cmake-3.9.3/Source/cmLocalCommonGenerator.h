/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalCommonGenerator_h
#define cmLocalCommonGenerator_h

#include "cmConfigure.h"

#include <string>

#include "cmLocalGenerator.h"

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;

/** \class cmLocalCommonGenerator
 * \brief Common infrastructure for Makefile and Ninja local generators.
 */
class cmLocalCommonGenerator : public cmLocalGenerator
{
public:
  cmLocalCommonGenerator(cmGlobalGenerator* gg, cmMakefile* mf,
                         std::string const& wd);
  ~cmLocalCommonGenerator() CM_OVERRIDE;

  std::string const& GetConfigName() { return this->ConfigName; }

  std::string GetWorkingDirectory() const { return this->WorkingDirectory; }

  std::string GetTargetFortranFlags(cmGeneratorTarget const* target,
                                    std::string const& config) CM_OVERRIDE;

protected:
  std::string WorkingDirectory;

  std::string ConfigName;

  friend class cmCommonTargetGenerator;
};

#endif
