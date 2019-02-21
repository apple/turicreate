/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmNinjaUtilityTargetGenerator_h
#define cmNinjaUtilityTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmNinjaTargetGenerator.h"

class cmGeneratorTarget;

class cmNinjaUtilityTargetGenerator : public cmNinjaTargetGenerator
{
public:
  cmNinjaUtilityTargetGenerator(cmGeneratorTarget* target);
  ~cmNinjaUtilityTargetGenerator() override;

  void Generate() override;
};

#endif // ! cmNinjaUtilityTargetGenerator_h
