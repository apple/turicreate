/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGeneratorInitializer_h
#define cmQtAutoGeneratorInitializer_h

#include "cmConfigure.h" // IWYU pragma: keep

class cmGeneratorTarget;
class cmLocalGenerator;

class cmQtAutoGeneratorInitializer
{
public:
  static void InitializeAutogenSources(cmGeneratorTarget* target);
  static void InitializeAutogenTarget(cmLocalGenerator* lg,
                                      cmGeneratorTarget* target);
  static void SetupAutoGenerateTarget(cmGeneratorTarget const* target);
};

#endif
