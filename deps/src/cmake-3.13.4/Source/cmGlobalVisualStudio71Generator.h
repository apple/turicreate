/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio71Generator_h
#define cmGlobalVisualStudio71Generator_h

#include "cmGlobalVisualStudio7Generator.h"

/** \class cmGlobalVisualStudio71Generator
 * \brief Write a Unix makefiles.
 *
 * cmGlobalVisualStudio71Generator manages UNIX build process for a tree
 */
class cmGlobalVisualStudio71Generator : public cmGlobalVisualStudio7Generator
{
public:
  cmGlobalVisualStudio71Generator(cmake* cm,
                                  const std::string& platformName = "");

protected:
  void WriteSLNFile(std::ostream& fout, cmLocalGenerator* root,
                    std::vector<cmLocalGenerator*>& generators) override;
  virtual void WriteSolutionConfigurations(
    std::ostream& fout, std::vector<std::string> const& configs);
  void WriteProject(std::ostream& fout, const std::string& name,
                    const char* path, const cmGeneratorTarget* t) override;
  void WriteProjectDepends(std::ostream& fout, const std::string& name,
                           const char* path,
                           cmGeneratorTarget const* t) override;
  void WriteProjectConfigurations(
    std::ostream& fout, const std::string& name,
    cmGeneratorTarget const& target, std::vector<std::string> const& configs,
    const std::set<std::string>& configsPartOfDefaultBuild,
    const std::string& platformMapping = "") override;
  void WriteExternalProject(std::ostream& fout, const std::string& name,
                            const char* path, const char* typeGuid,
                            const std::set<std::string>& depends) override;
  void WriteSLNHeader(std::ostream& fout) override;

  // Folders are not supported by VS 7.1.
  virtual bool UseFolderProperty() { return false; }

  std::string ProjectConfigurationSectionName;
};
#endif
