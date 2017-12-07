/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmLinkLineDeviceComputer.h"

#include <set>
#include <sstream>
#include <vector>

#include "cmComputeLinkInformation.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalNinjaGenerator.h"
#include "cmStateTypes.h"

class cmOutputConverter;

cmLinkLineDeviceComputer::cmLinkLineDeviceComputer(
  cmOutputConverter* outputConverter, cmStateDirectory const& stateDir)
  : cmLinkLineComputer(outputConverter, stateDir)
{
}

cmLinkLineDeviceComputer::~cmLinkLineDeviceComputer()
{
}

std::string cmLinkLineDeviceComputer::ComputeLinkLibraries(
  cmComputeLinkInformation& cli, std::string const& stdLibString)
{
  // Write the library flags to the build rule.
  std::ostringstream fout;
  typedef cmComputeLinkInformation::ItemVector ItemVector;
  ItemVector const& items = cli.GetItems();
  std::string config = cli.GetConfig();
  for (ItemVector::const_iterator li = items.begin(); li != items.end();
       ++li) {
    if (!li->Target) {
      continue;
    }

    bool skippable = false;
    switch (li->Target->GetType()) {
      case cmStateEnums::SHARED_LIBRARY:
      case cmStateEnums::MODULE_LIBRARY:
      case cmStateEnums::INTERFACE_LIBRARY:
        skippable = true;
        break;
      case cmStateEnums::STATIC_LIBRARY:
        // If a static library is resolving its device linking, it should
        // be removed for other device linking
        skippable =
          li->Target->GetPropertyAsBool("CUDA_RESOLVE_DEVICE_SYMBOLS");
        break;
      default:
        break;
    }

    if (skippable) {
      continue;
    }

    std::set<std::string> langs;
    li->Target->GetLanguages(langs, config);
    if (langs.count("CUDA") == 0) {
      continue;
    }

    if (li->IsPath) {
      fout << this->ConvertToOutputFormat(
        this->ConvertToLinkReference(li->Value));
    } else {
      fout << li->Value;
    }
    fout << " ";
  }

  if (!stdLibString.empty()) {
    fout << stdLibString << " ";
  }

  return fout.str();
}

std::string cmLinkLineDeviceComputer::GetLinkerLanguage(cmGeneratorTarget*,
                                                        std::string const&)
{
  return "CUDA";
}

cmNinjaLinkLineDeviceComputer::cmNinjaLinkLineDeviceComputer(
  cmOutputConverter* outputConverter, cmStateDirectory const& stateDir,
  cmGlobalNinjaGenerator const* gg)
  : cmLinkLineDeviceComputer(outputConverter, stateDir)
  , GG(gg)
{
}

std::string cmNinjaLinkLineDeviceComputer::ConvertToLinkReference(
  std::string const& lib) const
{
  return GG->ConvertToNinjaPath(lib);
}
