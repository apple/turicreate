/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio11Generator_h
#define cmGlobalVisualStudio11Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <set>
#include <string>

#include "cmGlobalVisualStudio10Generator.h"
#include "cmStateTypes.h"

class cmGlobalGeneratorFactory;
class cmMakefile;
class cmake;

/** \class cmGlobalVisualStudio11Generator  */
class cmGlobalVisualStudio11Generator : public cmGlobalVisualStudio10Generator
{
public:
  cmGlobalVisualStudio11Generator(cmake* cm, const std::string& name,
                                  const std::string& platformName);
  static cmGlobalGeneratorFactory* NewFactory();

  bool MatchesGeneratorName(const std::string& name) const override;

  void WriteSLNHeader(std::ostream& fout) override;

protected:
  bool InitializeWindowsPhone(cmMakefile* mf) override;
  bool InitializeWindowsStore(cmMakefile* mf) override;
  bool SelectWindowsPhoneToolset(std::string& toolset) const override;
  bool SelectWindowsStoreToolset(std::string& toolset) const override;

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  virtual bool IsWindowsDesktopToolsetInstalled() const;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsPhoneToolsetInstalled() const;
  bool IsWindowsStoreToolsetInstalled() const;

  const char* GetIDEVersion() override { return "11.0"; }
  bool UseFolderProperty();
  static std::set<std::string> GetInstalledWindowsCESDKs();

  /** Return true if the configuration needs to be deployed */
  bool NeedsDeploy(cmStateEnums::TargetType type) const override;

private:
  class Factory;
  friend class Factory;
};
#endif
