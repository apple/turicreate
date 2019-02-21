/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio12Generator_h
#define cmGlobalVisualStudio12Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

#include "cmGlobalVisualStudio11Generator.h"

class cmGlobalGeneratorFactory;
class cmMakefile;
class cmake;

/** \class cmGlobalVisualStudio12Generator  */
class cmGlobalVisualStudio12Generator : public cmGlobalVisualStudio11Generator
{
public:
  cmGlobalVisualStudio12Generator(cmake* cm, const std::string& name,
                                  const std::string& platformName);
  static cmGlobalGeneratorFactory* NewFactory();

  bool MatchesGeneratorName(const std::string& name) const override;

  void WriteSLNHeader(std::ostream& fout) override;

  // in Visual Studio 2013 they detached the MSBuild tools version
  // from the .Net Framework version and instead made it have it's own
  // version number
  const char* GetToolsVersion() override { return "12.0"; }

protected:
  bool ProcessGeneratorToolsetField(std::string const& key,
                                    std::string const& value) override;

  bool InitializeWindowsPhone(cmMakefile* mf) override;
  bool InitializeWindowsStore(cmMakefile* mf) override;
  bool SelectWindowsPhoneToolset(std::string& toolset) const override;
  bool SelectWindowsStoreToolset(std::string& toolset) const override;

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  bool IsWindowsDesktopToolsetInstalled() const override;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsPhoneToolsetInstalled() const;
  bool IsWindowsStoreToolsetInstalled() const;
  const char* GetIDEVersion() override { return "12.0"; }

private:
  class Factory;
};
#endif
