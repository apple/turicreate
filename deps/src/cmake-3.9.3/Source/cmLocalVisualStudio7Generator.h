/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalVisualStudio7Generator_h
#define cmLocalVisualStudio7Generator_h

#include "cmConfigure.h"

#include <iosfwd>
#include <string>
#include <vector>

#include "cmLocalVisualStudioGenerator.h"
#include "cmVisualStudioGeneratorOptions.h"

class cmCustomCommand;
class cmGeneratorTarget;
class cmGlobalGenerator;
class cmLocalVisualStudio7GeneratorFCInfo;
class cmLocalVisualStudio7GeneratorInternals;
class cmMakefile;
class cmSourceFile;
class cmSourceGroup;

/** \class cmLocalVisualStudio7Generator
 * \brief Write Visual Studio .NET project files.
 *
 * cmLocalVisualStudio7Generator produces a Visual Studio .NET project
 * file for each target in its directory.
 */
class cmLocalVisualStudio7Generator : public cmLocalVisualStudioGenerator
{
public:
  ///! Set cache only and recurse to false by default.
  cmLocalVisualStudio7Generator(cmGlobalGenerator* gg, cmMakefile* mf);

  virtual ~cmLocalVisualStudio7Generator();

  virtual void AddHelperCommands();

  /**
   * Generate the makefile for this directory.
   */
  virtual void Generate();

  enum BuildType
  {
    STATIC_LIBRARY,
    DLL,
    EXECUTABLE,
    WIN32_EXECUTABLE,
    UTILITY
  };

  /**
   * Specify the type of the build: static, dll, or executable.
   */
  void SetBuildType(BuildType, const std::string& name);

  virtual std::string GetTargetDirectory(
    cmGeneratorTarget const* target) const;
  cmSourceFile* CreateVCProjBuildRule();
  void WriteStampFiles();
  virtual std::string ComputeLongestObjectDirectory(
    cmGeneratorTarget const*) const;

  virtual void ReadAndStoreExternalGUID(const std::string& name,
                                        const char* path);
  virtual void AddCMakeListsRules();

protected:
  void CreateSingleVCProj(const std::string& lname, cmGeneratorTarget* tgt);

private:
  typedef cmVisualStudioGeneratorOptions Options;
  typedef cmLocalVisualStudio7GeneratorFCInfo FCInfo;
  std::string GetBuildTypeLinkerFlags(std::string rootLinkerFlags,
                                      const std::string& configName);
  void FixGlobalTargets();
  void WriteProjectFiles();
  void WriteVCProjHeader(std::ostream& fout, const std::string& libName,
                         cmGeneratorTarget* tgt,
                         std::vector<cmSourceGroup>& sgs);
  void WriteVCProjFooter(std::ostream& fout, cmGeneratorTarget* target);
  void WriteVCProjFile(std::ostream& fout, const std::string& libName,
                       cmGeneratorTarget* tgt);
  void WriteConfigurations(std::ostream& fout,
                           std::vector<std::string> const& configs,
                           const std::string& libName, cmGeneratorTarget* tgt);
  void WriteConfiguration(std::ostream& fout, const std::string& configName,
                          const std::string& libName, cmGeneratorTarget* tgt);
  std::string EscapeForXML(const std::string& s);
  std::string ConvertToXMLOutputPath(const char* path);
  std::string ConvertToXMLOutputPathSingle(const char* path);
  void OutputTargetRules(std::ostream& fout, const std::string& configName,
                         cmGeneratorTarget* target,
                         const std::string& libName);
  void OutputBuildTool(std::ostream& fout, const std::string& configName,
                       cmGeneratorTarget* t, const Options& targetOptions);
  void OutputDeploymentDebuggerTool(std::ostream& fout,
                                    std::string const& config,
                                    cmGeneratorTarget* target);
  void OutputLibraryDirectories(std::ostream& fout,
                                std::vector<std::string> const& dirs);
  void WriteProjectSCC(std::ostream& fout, cmGeneratorTarget* target);
  void WriteProjectStart(std::ostream& fout, const std::string& libName,
                         cmGeneratorTarget* tgt,
                         std::vector<cmSourceGroup>& sgs);
  void WriteProjectStartFortran(std::ostream& fout, const std::string& libName,
                                cmGeneratorTarget* tgt);
  void WriteVCProjBeginGroup(std::ostream& fout, const char* group,
                             const char* filter);
  void WriteVCProjEndGroup(std::ostream& fout);

  void WriteCustomRule(std::ostream& fout,
                       std::vector<std::string> const& configs,
                       const char* source, const cmCustomCommand& command,
                       FCInfo& fcinfo);
  void WriteTargetVersionAttribute(std::ostream& fout, cmGeneratorTarget* gt);

  bool WriteGroup(const cmSourceGroup* sg, cmGeneratorTarget* target,
                  std::ostream& fout, const std::string& libName,
                  std::vector<std::string> const& configs,
                  std::map<cmSourceFile const*, size_t> const& sourcesIndex);

  friend class cmLocalVisualStudio7GeneratorFCInfo;
  friend class cmLocalVisualStudio7GeneratorInternals;

  class EventWriter;

  friend class EventWriter;

  bool FortranProject;
  bool WindowsCEProject;
  cmLocalVisualStudio7GeneratorInternals* Internal;
};

#endif
