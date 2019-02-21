/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio14Generator_h
#define cmGlobalVisualStudio14Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

#include "cmGlobalVisualStudio12Generator.h"

class cmGlobalGeneratorFactory;
class cmMakefile;
class cmake;

/** \class cmGlobalVisualStudio14Generator  */
class cmGlobalVisualStudio14Generator : public cmGlobalVisualStudio12Generator
{
public:
  cmGlobalVisualStudio14Generator(cmake* cm, const std::string& name,
                                  const std::string& platformName);
  static cmGlobalGeneratorFactory* NewFactory();

  bool MatchesGeneratorName(const std::string& name) const override;

  void WriteSLNHeader(std::ostream& fout) override;

  const char* GetToolsVersion() override { return "14.0"; }

protected:
  bool InitializeWindows(cmMakefile* mf) override;
  bool InitializeWindowsStore(cmMakefile* mf) override;
  bool SelectWindowsStoreToolset(std::string& toolset) const override;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsStoreToolsetInstalled() const;

  // Used to make sure that the Windows 10 SDK selected can work with the
  // version of the toolset.
  virtual std::string GetWindows10SDKMaxVersion() const;

  const char* GetIDEVersion() override { return "14.0"; }
  virtual bool SelectWindows10SDK(cmMakefile* mf, bool required);

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  bool IsWindowsDesktopToolsetInstalled() const override;

  std::string GetWindows10SDKVersion();

private:
  class Factory;
};
#endif
