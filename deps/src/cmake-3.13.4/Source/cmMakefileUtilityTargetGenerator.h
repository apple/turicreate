/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmMakefileUtilityTargetGenerator_h
#define cmMakefileUtilityTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmMakefileTargetGenerator.h"

class cmGeneratorTarget;

class cmMakefileUtilityTargetGenerator : public cmMakefileTargetGenerator
{
public:
  cmMakefileUtilityTargetGenerator(cmGeneratorTarget* target);
  ~cmMakefileUtilityTargetGenerator() override;

  /* the main entry point for this class. Writes the Makefiles associated
     with this target */
  void WriteRuleFiles() override;

protected:
};

#endif
