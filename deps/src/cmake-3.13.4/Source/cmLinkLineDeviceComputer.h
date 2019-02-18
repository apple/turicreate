/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmLinkLineDeviceComputer_h
#define cmLinkLineDeviceComputer_h

#include "cmConfigure.h" // IWYU pragma: keep

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
  ~cmLinkLineDeviceComputer() override;

  std::string ComputeLinkLibraries(cmComputeLinkInformation& cli,
                                   std::string const& stdLibString) override;

  std::string GetLinkerLanguage(cmGeneratorTarget* target,
                                std::string const& config) override;
};

class cmNinjaLinkLineDeviceComputer : public cmLinkLineDeviceComputer
{
  CM_DISABLE_COPY(cmNinjaLinkLineDeviceComputer)

public:
  cmNinjaLinkLineDeviceComputer(cmOutputConverter* outputConverter,
                                cmStateDirectory const& stateDir,
                                cmGlobalNinjaGenerator const* gg);

  std::string ConvertToLinkReference(std::string const& input) const override;

private:
  cmGlobalNinjaGenerator const* GG;
};

#endif
