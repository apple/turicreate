/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmLinkLineComputer_h
#define cmLinkLineComputer_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmStateDirectory.h"

class cmComputeLinkInformation;
class cmGeneratorTarget;
class cmOutputConverter;

class cmLinkLineComputer
{
  CM_DISABLE_COPY(cmLinkLineComputer)

public:
  cmLinkLineComputer(cmOutputConverter* outputConverter,
                     cmStateDirectory const& stateDir);
  virtual ~cmLinkLineComputer();

  void SetUseWatcomQuote(bool useWatcomQuote);
  void SetForResponse(bool forResponse);
  void SetRelink(bool relink);

  virtual std::string ConvertToLinkReference(std::string const& input) const;

  std::string ComputeLinkPath(cmComputeLinkInformation& cli,
                              std::string const& libPathFlag,
                              std::string const& libPathTerminator);

  std::string ComputeFrameworkPath(cmComputeLinkInformation& cli,
                                   std::string const& fwSearchFlag);

  virtual std::string ComputeLinkLibraries(cmComputeLinkInformation& cli,
                                           std::string const& stdLibString);

  virtual std::string GetLinkerLanguage(cmGeneratorTarget* target,
                                        std::string const& config);

protected:
  std::string ComputeLinkLibs(cmComputeLinkInformation& cli);
  std::string ComputeRPath(cmComputeLinkInformation& cli);

  std::string ConvertToOutputFormat(std::string const& input);
  std::string ConvertToOutputForExisting(std::string const& input);

  cmStateDirectory StateDir;
  cmOutputConverter* OutputConverter;

  bool ForResponse;
  bool UseWatcomQuote;
  bool Relink;
};

#endif
