/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmNinjaLinkLineComputer.h"

#include "cmGlobalNinjaGenerator.h"

class cmOutputConverter;

cmNinjaLinkLineComputer::cmNinjaLinkLineComputer(
  cmOutputConverter* outputConverter, cmStateDirectory const& stateDir,
  cmGlobalNinjaGenerator const* gg)
  : cmLinkLineComputer(outputConverter, stateDir)
  , GG(gg)
{
}

std::string cmNinjaLinkLineComputer::ConvertToLinkReference(
  std::string const& lib) const
{
  return GG->ConvertToNinjaPath(lib);
}
