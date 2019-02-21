/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmMakefileExecutableTargetGenerator_h
#define cmMakefileExecutableTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmMakefileTargetGenerator.h"

class cmGeneratorTarget;

class cmMakefileExecutableTargetGenerator : public cmMakefileTargetGenerator
{
public:
  cmMakefileExecutableTargetGenerator(cmGeneratorTarget* target);
  ~cmMakefileExecutableTargetGenerator() override;

  /* the main entry point for this class. Writes the Makefiles associated
     with this target */
  void WriteRuleFiles() override;

protected:
  virtual void WriteExecutableRule(bool relink);
  virtual void WriteDeviceExecutableRule(bool relink);

private:
  std::string DeviceLinkObject;
};

#endif
