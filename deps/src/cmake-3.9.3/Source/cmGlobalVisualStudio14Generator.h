/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio14Generator_h
#define cmGlobalVisualStudio14Generator_h

#include "cmConfigure.h"

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

  virtual bool MatchesGeneratorName(const std::string& name) const;

  virtual void WriteSLNHeader(std::ostream& fout);

  virtual const char* GetToolsVersion() { return "14.0"; }
protected:
  virtual bool InitializeWindows(cmMakefile* mf);
  virtual bool InitializeWindowsStore(cmMakefile* mf);
  virtual bool SelectWindowsStoreToolset(std::string& toolset) const;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsStoreToolsetInstalled() const;

  virtual const char* GetIDEVersion() { return "14.0"; }
  virtual bool SelectWindows10SDK(cmMakefile* mf, bool required);

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  virtual bool IsWindowsDesktopToolsetInstalled() const;

  std::string GetWindows10SDKVersion();

private:
  class Factory;
};
#endif
