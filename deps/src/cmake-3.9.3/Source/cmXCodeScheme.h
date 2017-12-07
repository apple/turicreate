/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmXCodeScheme_h
#define cmXCodeScheme_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmGlobalXCodeGenerator.h"
#include "cmSystemTools.h"
#include "cmXCodeObject.h"
#include "cmXMLWriter.h"

/** \class cmXCodeScheme
 * \brief Write shared schemes for native targets in Xcode project.
 */
class cmXCodeScheme
{
public:
  cmXCodeScheme(cmXCodeObject* xcObj,
                const std::vector<std::string>& configList,
                unsigned int xcVersion);

  void WriteXCodeSharedScheme(const std::string& xcProjDir,
                              const std::string& container);

private:
  const cmXCodeObject* const Target;
  const std::string& TargetName;
  const std::string BuildableName;
  const std::string& TargetId;
  const std::vector<std::string>& ConfigList;
  const unsigned int XcodeVersion;

  void WriteXCodeXCScheme(std::ostream& fout, const std::string& container);

  void WriteBuildAction(cmXMLWriter& xout, const std::string& container);
  void WriteTestAction(cmXMLWriter& xout, std::string configuration);
  void WriteLaunchAction(cmXMLWriter& xout, std::string configuration,
                         const std::string& container);
  void WriteProfileAction(cmXMLWriter& xout, std::string configuration);
  void WriteAnalyzeAction(cmXMLWriter& xout, std::string configuration);
  void WriteArchiveAction(cmXMLWriter& xout, std::string configuration);

  std::string WriteVersionString();
  std::string FindConfiguration(const std::string& name);

  static bool IsExecutable(const cmXCodeObject* target);
};

#endif
