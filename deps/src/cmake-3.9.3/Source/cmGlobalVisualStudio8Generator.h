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
  static cmGlobalGeneratorFactory* NewFactory();

  ///! Get the name for the generator.
  virtual std::string GetName() const { return this->Name; }

  /** Get the name of the main stamp list file. */
  static std::string GetGenerateStampList();

  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);
  virtual void AddPlatformDefinitions(cmMakefile* mf);

  virtual bool SetGeneratorPlatform(std::string const& p, cmMakefile* mf);

  /**
   * Override Configure and Generate to add the build-system check
   * target.
   */
  virtual void Configure();

  /**
   * Where does this version of Visual Studio look for macros for the
   * current user? Returns the empty string if this version of Visual
   * Studio does not implement support for VB macros.
   */
  virtual std::string GetUserMacrosDirectory();

  /**
   * What is the reg key path to "vsmacros" for this version of Visual
   * Studio?
   */
  virtual std::string GetUserMacrosRegKeyBase();

  /** Return true if the target project file should have the option
      LinkLibraryDependencies and link to .sln dependencies. */
  virtual bool NeedLinkLibraryDependencies(cmGeneratorTarget* target);

  /** Return true if building for Windows CE */
  virtual bool TargetsWindowsCE() const
  {
    return !this->WindowsCEVersion.empty();
  }

  /** Is the installed VS an Express edition?  */
  bool IsExpressEdition() const { return this->ExpressEdition; }

protected:
  virtual void AddExtraIDETargets();
  virtual const char* GetIDEVersion() { return "8.0"; }

  virtual std::string FindDevEnvCommand();

  virtual bool VSLinksDependencies() const { return false; }

  bool AddCheckTarget();

  /** Return true if the configuration needs to be deployed */
  virtual bool NeedsDeploy(cmStateEnums::TargetType type) const;

  static cmIDEFlagTable const* GetExtraFlagTableVS8();
  virtual void WriteSLNHeader(std::ostream& fout);
  virtual void WriteSolutionConfigurations(
    std::ostream& fout, std::vector<std::string> const& configs);
  virtual void WriteProjectConfigurations(
    std::ostream& fout, const std::string& name,
    cmGeneratorTarget const& target, std::vector<std::string> const& configs,
    const std::set<std::string>& configsPartOfDefaultBuild,
    const std::string& platformMapping = "");
  virtual bool ComputeTargetDepends();
  virtual void WriteProjectDepends(std::ostream& fout, const std::string& name,
                                   const char* path,
                                   const cmGeneratorTarget* t);

  bool UseFolderProperty();

  std::string Name;
  std::string WindowsCEVersion;
  bool ExpressEdition;

private:
  class Factory;
  friend class Factory;
};
#endif
