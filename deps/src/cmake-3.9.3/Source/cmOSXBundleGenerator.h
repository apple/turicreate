/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmOSXBundleGenerator_h
#define cmOSXBundleGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <set>
#include <string>
#include <vector>

class cmGeneratorTarget;
class cmLocalGenerator;
class cmMakefile;
class cmSourceFile;

class cmOSXBundleGenerator
{
public:
  cmOSXBundleGenerator(cmGeneratorTarget* target,
                       const std::string& configName);

  // create an app bundle at a given root, and return
  // the directory within the bundle that contains the executable
  void CreateAppBundle(const std::string& targetName, std::string& root);

  // create a framework at a given root
  void CreateFramework(const std::string& targetName, const std::string& root);

  // create a cf bundle at a given root
  void CreateCFBundle(const std::string& targetName, const std::string& root);

  struct MacOSXContentGeneratorType
  {
    virtual ~MacOSXContentGeneratorType() {}
    virtual void operator()(cmSourceFile const& source,
                            const char* pkgloc) = 0;
  };

  void GenerateMacOSXContentStatements(
    std::vector<cmSourceFile const*> const& sources,
    MacOSXContentGeneratorType* generator);
  std::string InitMacOSXContentDirectory(const char* pkgloc);

  void SetMacContentFolders(std::set<std::string>* macContentFolders)
  {
    this->MacContentFolders = macContentFolders;
  }

private:
  bool MustSkip();

private:
  cmGeneratorTarget* GT;
  cmMakefile* Makefile;
  cmLocalGenerator* LocalGenerator;
  std::string ConfigName;
  std::set<std::string>* MacContentFolders;
};

#endif
