/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmLinkLineDeviceComputer.h"

#include <set>
#include <sstream>
#include <utility>

#include "cmAlgorithms.h"
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

static bool cmLinkItemValidForDevice(std::string const& item)
{
  // Valid items are:
  // * Non-flags (does not start in '-')
  // * Specific flags --library, --library-path, -l, -L
  // For example:
  // * 'cublas_device' => pass-along
  // * '--library pthread' => pass-along
  // * '-lpthread' => pass-along
  // * '-pthread' => drop
  // * '-a' => drop
  return (!cmHasLiteralPrefix(item, "-") || //
          cmHasLiteralPrefix(item, "-l") || //
          cmHasLiteralPrefix(item, "-L") || //
          cmHasLiteralPrefix(item, "--library"));
}

std::string cmLinkLineDeviceComputer::ComputeLinkLibraries(
  cmComputeLinkInformation& cli, std::string const& stdLibString)
{
  // Write the library flags to the build rule.
  std::ostringstream fout;

  // Generate the unique set of link items when device linking.
  // The nvcc device linker is designed so that each static library
  // with device symbols only needs to be listed once as it doesn't
  // care about link order.
  std::set<std::string> emitted;
  typedef cmComputeLinkInformation::ItemVector ItemVector;
  ItemVector const& items = cli.GetItems();
  std::string config = cli.GetConfig();
  for (auto const& item : items) {
    if (item.Target) {
      bool skip = false;
      switch (item.Target->GetType()) {
        case cmStateEnums::MODULE_LIBRARY:
        case cmStateEnums::INTERFACE_LIBRARY:
          skip = true;
          break;
        case cmStateEnums::STATIC_LIBRARY:
          skip = item.Target->GetPropertyAsBool("CUDA_RESOLVE_DEVICE_SYMBOLS");
          break;
        default:
          break;
      }
      if (skip) {
        continue;
      }
    }

    std::string out;
    if (item.IsPath) {
      // nvcc understands absolute paths to libraries ending in '.a' or '.lib'.
      // These should be passed to nvlink.  Other extensions need to be left
      // out because nvlink may not understand or need them.  Even though it
      // can tolerate '.so' or '.dylib' it cannot tolerate '.so.1'.
      if (cmHasLiteralSuffix(item.Value, ".a") ||
          cmHasLiteralSuffix(item.Value, ".lib")) {
        out += this->ConvertToOutputFormat(
          this->ConvertToLinkReference(item.Value));
      }
    } else if (cmLinkItemValidForDevice(item.Value)) {
      out += item.Value;
    }

    if (emitted.insert(out).second) {
      fout << out << " ";
    }
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
