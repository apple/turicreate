/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio11Generator_h
#define cmGlobalVisualStudio11Generator_h

#include "cmConfigure.h"

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

  virtual bool MatchesGeneratorName(const std::string& name) const;

  virtual void WriteSLNHeader(std::ostream& fout);

protected:
  virtual bool InitializeWindowsPhone(cmMakefile* mf);
  virtual bool InitializeWindowsStore(cmMakefile* mf);
  virtual bool SelectWindowsPhoneToolset(std::string& toolset) const;
  virtual bool SelectWindowsStoreToolset(std::string& toolset) const;

  // Used to verify that the Desktop toolset for the current generator is
  // installed on the machine.
  virtual bool IsWindowsDesktopToolsetInstalled() const;

  // These aren't virtual because we need to check if the selected version
  // of the toolset is installed
  bool IsWindowsPhoneToolsetInstalled() const;
  bool IsWindowsStoreToolsetInstalled() const;

  virtual const char* GetIDEVersion() { return "11.0"; }
  bool UseFolderProperty();
  static std::set<std::string> GetInstalledWindowsCESDKs();

  /** Return true if the configuration needs to be deployed */
  virtual bool NeedsDeploy(cmStateEnums::TargetType type) const;

private:
  class Factory;
  friend class Factory;
};
#endif
