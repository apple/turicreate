/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmLinkLineDeviceComputer_h
#define cmLinkLineDeviceComputer_h

#include "cmConfigure.h"

#include <string>

#include "cmLinkLineComputer.h"

class cmComputeLinkInformation;
class cmGeneratorTarget;
class cmGlobalNinjaGenerator;
class cmOutputConverter;
class cmStateDirectory;

class cmLinkLineDeviceComputer : public cmLinkLineComputer
{
  CM_DISABLE_COPY(cmLinkLineDeviceComputer)

public:
  cmLinkLineDeviceComputer(cmOutputConverter* outputConverter,
                           cmStateDirectory const& stateDir);
  ~cmLinkLineDeviceComputer() CM_OVERRIDE;

  std::string ComputeLinkLibraries(cmComputeLinkInformation& cli,
                                   std::string const& stdLibString)
    CM_OVERRIDE;

  std::string GetLinkerLanguage(cmGeneratorTarget* target,
                                std::string const& config) CM_OVERRIDE;
};

class cmNinjaLinkLineDeviceComputer : public cmLinkLineDeviceComputer
{
  CM_DISABLE_COPY(cmNinjaLinkLineDeviceComputer)

public:
  cmNinjaLinkLineDeviceComputer(cmOutputConverter* outputConverter,
                                cmStateDirectory const& stateDir,
                                cmGlobalNinjaGenerator const* gg);

  std::string ConvertToLinkReference(std::string const& input) const
    CM_OVERRIDE;

private:
  cmGlobalNinjaGenerator const* GG;
};

#endif
