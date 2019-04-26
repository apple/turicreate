/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio8Generator_h
#define cmGlobalVisualStudio8Generator_h

#include "cmGlobalVisualStudio71Generator.h"

/** \class cmGlobalVisualStudio8Generator
 * \brief Write a Unix makefiles.
 *
 * cmGlobalVisualStudio8Generator manages UNIX build process for a tree
 */
class cmGlobalVisualStudio8Generator : public cmGlobalVisualStudio71Generator
{
public:
  cmGlobalVisualStudio8Generator(cmake* cm, const std::string& name,
                                 const std::string& platformName);

  ///! Get the name for the generator.
  std::string GetName() const override { return this->Name; }

  /** Get the name of the main stamp list file. */
  static std::string GetGenerateStampList();

  void EnableLanguage(std::vector<std::string> const& languages, cmMakefile*,
                      bool optional) override;
  virtual void AddPlatformDefinitions(cmMakefile* mf);

  bool SetGeneratorPlatform(std::string const& p, cmMakefile* mf) override;

  /**
   * Override Configure and Generate to add the build-system check
   * target.
   */
  void Configure() override;

  /** Return true if the target project file should have the option
      LinkLibraryDependencies and link to .sln dependencies. */
  bool NeedLinkLibraryDependencies(cmGeneratorTarget* target) override;

  /** Return true if building for Windows CE */
  bool TargetsWindowsCE() const override
  {
    return !this->WindowsCEVersion.empty();
  }

  /** Is the installed VS an Express edition?  */
  bool IsExpressEdition() const { return this->ExpressEdition; }

protected:
  void AddExtraIDETargets() override;
  const char* GetIDEVersion() override { return "8.0"; }

  std::string FindDevEnvCommand() override;

  bool VSLinksDependencies() const override { return false; }

  bool AddCheckTarget();

  /** Return true if the configuration needs to be deployed */
  virtual bool NeedsDeploy(cmStateEnums::TargetType type) const;

  static cmIDEFlagTable const* GetExtraFlagTableVS8();
  void WriteSolutionConfigurations(
    std::ostream& fout, std::vector<std::string> const& configs) override;
  void WriteProjectConfigurations(
    std::ostream& fout, const std::string& name,
    cmGeneratorTarget const& target, std::vector<std::string> const& configs,
    const std::set<std::string>& configsPartOfDefaultBuild,
    const std::string& platformMapping = "") override;
  bool ComputeTargetDepends() override;
  void WriteProjectDepends(std::ostream& fout, const std::string& name,
                           const char* path,
                           const cmGeneratorTarget* t) override;

  bool UseFolderProperty();

  std::string Name;
  std::string WindowsCEVersion;
  bool ExpressEdition;
};
#endif
