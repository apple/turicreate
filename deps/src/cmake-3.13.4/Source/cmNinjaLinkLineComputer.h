/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmNinjaLinkLineComputer_h
#define cmNinjaLinkLineComputer_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmLinkLineComputer.h"

class cmGlobalNinjaGenerator;
class cmOutputConverter;
class cmStateDirectory;

class cmNinjaLinkLineComputer : public cmLinkLineComputer
{
  CM_DISABLE_COPY(cmNinjaLinkLineComputer)

public:
  cmNinjaLinkLineComputer(cmOutputConverter* outputConverter,
                          cmStateDirectory const& stateDir,
                          cmGlobalNinjaGenerator const* gg);

  std::string ConvertToLinkReference(std::string const& input) const override;

private:
  cmGlobalNinjaGenerator const* GG;
};

#endif
