/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudioGenerator_h
#define cmGlobalVisualStudioGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "cmGlobalGenerator.h"
#include "cmTargetDepend.h"

class cmCustomCommand;
class cmGeneratorTarget;
class cmLocalGenerator;
class cmMakefile;
class cmake;

/** \class cmGlobalVisualStudioGenerator
 * \brief Base class for global Visual Studio generators.
 *
 * cmGlobalVisualStudioGenerator provides functionality common to all
 * global Visual Studio generators.
 */
class cmGlobalVisualStudioGenerator : public cmGlobalGenerator
{
public:
  /** Known versions of Visual Studio.  */
  enum VSVersion
  {
    VS9 = 90,
    VS10 = 100,
    VS11 = 110,
    VS12 = 120,
    /* VS13 = 130 was skipped */
    VS14 = 140,
    VS15 = 150
  };

  cmGlobalVisualStudioGenerator(cmake* cm);
  virtual ~cmGlobalVisualStudioGenerator();

  VSVersion GetVersion() const;
  void SetVersion(VSVersion v);

  /**
   * Configure CMake's Visual Studio macros file into the user's Visual
   * Studio macros directory.
   */
  virtual void ConfigureCMakeVisualStudioMacros();

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

  enum MacroName
  {
    MacroReload,
    MacroStop
  };

  /**
   * Call the ReloadProjects macro if necessary based on
   * GetFilesReplacedDuringGenerate results.
   */
  void CallVisualStudioMacro(MacroName m, const char* vsSolutionFile = 0);

  // return true if target is fortran only
  bool TargetIsFortranOnly(const cmGeneratorTarget* gt);

  /** Get the top-level registry key for this VS version.  */
  std::string GetRegistryBase();

  /** Get the top-level registry key for the given VS version.  */
  static std::string GetRegistryBase(const char* version);

  /** Return true if the generated build tree may contain multiple builds.
      i.e. "Can I build Debug and Release in the same tree?" */
  bool IsMultiConfig() const override { return true; }

  /** Return true if building for Windows CE */
  virtual bool TargetsWindowsCE() const { return false; }

  bool IsIncludeExternalMSProjectSupported() const override { return true; }

  class TargetSet : public std::set<cmGeneratorTarget const*>
  {
  };
  class TargetCompare
  {
    std::string First;

  public:
    TargetCompare(std::string const& first)
      : First(first)
    {
    }
    bool operator()(cmGeneratorTarget const* l,
                    cmGeneratorTarget const* r) const;
  };
  class OrderedTargetDependSet;

  bool FindMakeProgram(cmMakefile*) override;

  std::string ExpandCFGIntDir(const std::string& str,
                              const std::string& config) const override;

  void ComputeTargetObjectDirectory(cmGeneratorTarget* gt) const;

  std::string GetStartupProjectName(cmLocalGenerator const* root) const;

  void AddSymbolExportCommand(cmGeneratorTarget*,
                              std::vector<cmCustomCommand>& commands,
                              std::string const& configName);

  bool Open(const std::string& bindir, const std::string& projectName,
            bool dryRun) override;

protected:
  void AddExtraIDETargets() override;

  // Does this VS version link targets to each other if there are
  // dependencies in the SLN file?  This was done for VS versions
  // below 8.
  virtual bool VSLinksDependencies() const { return true; }

  virtual const char* GetIDEVersion() = 0;

  bool ComputeTargetDepends() override;
  class VSDependSet : public std::set<std::string>
  {
  };
  class VSDependMap : public std::map<cmGeneratorTarget const*, VSDependSet>
  {
  };
  VSDependMap VSTargetDepends;
  void ComputeVSTargetDepends(cmGeneratorTarget*);

  bool CheckTargetLinks(cmGeneratorTarget& target, const std::string& name);
  std::string GetUtilityForTarget(cmGeneratorTarget& target,
                                  const std::string&);
  virtual std::string WriteUtilityDepend(cmGeneratorTarget const*) = 0;
  std::string GetUtilityDepend(const cmGeneratorTarget* target);
  typedef std::map<cmGeneratorTarget const*, std::string> UtilityDependsMap;
  UtilityDependsMap UtilityDepends;

protected:
  VSVersion Version;

private:
  virtual std::string GetVSMakeProgram() = 0;
  void PrintCompilerAdvice(std::ostream&, std::string const&,
                           const char*) const
  {
  }

  void FollowLinkDepends(cmGeneratorTarget const* target,
                         std::set<cmGeneratorTarget const*>& linked);

  class TargetSetMap : public std::map<cmGeneratorTarget*, TargetSet>
  {
  };
  TargetSetMap TargetLinkClosure;
  void FillLinkClosure(const cmGeneratorTarget* target, TargetSet& linked);
  TargetSet const& GetTargetLinkClosure(cmGeneratorTarget* target);
};

class cmGlobalVisualStudioGenerator::OrderedTargetDependSet
  : public std::multiset<cmTargetDepend,
                         cmGlobalVisualStudioGenerator::TargetCompare>
{
  typedef std::multiset<cmTargetDepend,
                        cmGlobalVisualStudioGenerator::TargetCompare>
    derived;

public:
  typedef cmGlobalGenerator::TargetDependSet TargetDependSet;
  typedef cmGlobalVisualStudioGenerator::TargetSet TargetSet;
  OrderedTargetDependSet(TargetDependSet const&, std::string const& first);
  OrderedTargetDependSet(TargetSet const&, std::string const& first);
};

#endif
