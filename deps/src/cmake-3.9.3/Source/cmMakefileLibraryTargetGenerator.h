/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmMakefileLibraryTargetGenerator_h
#define cmMakefileLibraryTargetGenerator_h

#include "cmConfigure.h"

#include "cmMakefileTargetGenerator.h"

#include <string>

class cmGeneratorTarget;

class cmMakefileLibraryTargetGenerator : public cmMakefileTargetGenerator
{
public:
  cmMakefileLibraryTargetGenerator(cmGeneratorTarget* target);
  ~cmMakefileLibraryTargetGenerator() CM_OVERRIDE;

  /* the main entry point for this class. Writes the Makefiles associated
     with this target */
  void WriteRuleFiles() CM_OVERRIDE;

protected:
  void WriteObjectLibraryRules();
  void WriteStaticLibraryRules();
  void WriteSharedLibraryRules(bool relink);
  void WriteModuleLibraryRules(bool relink);

  void WriteDeviceLibraryRules(const std::string& linkRule,
                               const std::string& extraFlags, bool relink);
  void WriteLibraryRules(const std::string& linkRule,
                         const std::string& extraFlags, bool relink);
  // MacOSX Framework support methods
  void WriteFrameworkRules(bool relink);

  // Store the computd framework version for OS X Frameworks.
  std::string FrameworkVersion;

private:
  std::string DeviceLinkObject;
};

#endif
