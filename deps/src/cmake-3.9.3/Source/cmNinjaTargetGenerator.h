/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmNinjaTargetGenerator_h
#define cmNinjaTargetGenerator_h

#include "cmConfigure.h"

#include "cmCommonTargetGenerator.h"
#include "cmGlobalNinjaGenerator.h"
#include "cmNinjaTypes.h"
#include "cmOSXBundleGenerator.h"

#include <set>
#include <string>
#include <vector>

class cmCustomCommand;
class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmLocalNinjaGenerator;
class cmMakefile;
class cmSourceFile;

class cmNinjaTargetGenerator : public cmCommonTargetGenerator
{
public:
  /// Create a cmNinjaTargetGenerator according to the @a target's type.
  static cmNinjaTargetGenerator* New(cmGeneratorTarget* target);

  /// Build a NinjaTargetGenerator.
  cmNinjaTargetGenerator(cmGeneratorTarget* target);

  /// Destructor.
  ~cmNinjaTargetGenerator() CM_OVERRIDE;

  virtual void Generate() = 0;

  std::string GetTargetName() const;

  bool NeedDepTypeMSVC(const std::string& lang) const;

protected:
  bool SetMsvcTargetPdbVariable(cmNinjaVars&) const;

  cmGeneratedFileStream& GetBuildFileStream() const;
  cmGeneratedFileStream& GetRulesFileStream() const;

  cmGeneratorTarget* GetGeneratorTarget() const
  {
    return this->GeneratorTarget;
  }

  cmLocalNinjaGenerator* GetLocalGenerator() const
  {
    return this->LocalGenerator;
  }

  cmGlobalNinjaGenerator* GetGlobalGenerator() const;

  cmMakefile* GetMakefile() const { return this->Makefile; }

  std::string LanguageCompilerRule(const std::string& lang) const;
  std::string LanguagePreprocessRule(std::string const& lang) const;
  bool NeedExplicitPreprocessing(std::string const& lang) const;
  std::string LanguageDyndepRule(std::string const& lang) const;
  bool NeedDyndep(std::string const& lang) const;

  std::string OrderDependsTargetForTarget();

  std::string ComputeOrderDependsForTarget();

  /**
   * Compute the flags for compilation of object files for a given @a language.
   * @note Generally it is the value of the variable whose name is computed
   *       by LanguageFlagsVarName().
   */
  std::string ComputeFlagsForObject(cmSourceFile const* source,
                                    const std::string& language);

  void AddIncludeFlags(std::string& flags,
                       std::string const& lang) CM_OVERRIDE;

  std::string ComputeDefines(cmSourceFile const* source,
                             const std::string& language);

  std::string ConvertToNinjaPath(const std::string& path) const
  {
    return this->GetGlobalGenerator()->ConvertToNinjaPath(path);
  }
  cmGlobalNinjaGenerator::MapToNinjaPathImpl MapToNinjaPath() const
  {
    return this->GetGlobalGenerator()->MapToNinjaPath();
  }

  /// @return the list of link dependency for the given target @a target.
  cmNinjaDeps ComputeLinkDeps() const;

  /// @return the source file path for the given @a source.
  std::string GetSourceFilePath(cmSourceFile const* source) const;

  /// @return the object file path for the given @a source.
  std::string GetObjectFilePath(cmSourceFile const* source) const;

  /// @return the preprocessed source file path for the given @a source.
  std::string GetPreprocessedFilePath(cmSourceFile const* source) const;

  /// @return the dyndep file path for this target.
  std::string GetDyndepFilePath(std::string const& lang) const;

  /// @return the target dependency scanner info file path
  std::string GetTargetDependInfoPath(std::string const& lang) const;

  /// @return the file path where the target named @a name is generated.
  std::string GetTargetFilePath(const std::string& name) const;

  /// @return the output path for the target.
  virtual std::string GetTargetOutputDir() const;

  void WriteLanguageRules(const std::string& language);
  void WriteCompileRule(const std::string& language);
  void WriteObjectBuildStatements();
  void WriteObjectBuildStatement(cmSourceFile const* source);
  void WriteTargetDependInfo(std::string const& lang);

  void ExportObjectCompileCommand(
    std::string const& language, std::string const& sourceFileName,
    std::string const& objectDir, std::string const& objectFileName,
    std::string const& objectFileDir, std::string const& flags,
    std::string const& defines, std::string const& includes);

  cmNinjaDeps GetObjects() const { return this->Objects; }

  void EnsureDirectoryExists(const std::string& dir) const;
  void EnsureParentDirectoryExists(const std::string& path) const;

  // write rules for Mac OS X Application Bundle content.
  struct MacOSXContentGeneratorType
    : cmOSXBundleGenerator::MacOSXContentGeneratorType
  {
    MacOSXContentGeneratorType(cmNinjaTargetGenerator* g)
      : Generator(g)
    {
    }

    void operator()(cmSourceFile const& source,
                    const char* pkgloc) CM_OVERRIDE;

  private:
    cmNinjaTargetGenerator* Generator;
  };
  friend struct MacOSXContentGeneratorType;

  MacOSXContentGeneratorType* MacOSXContentGenerator;
  // Properly initialized by sub-classes.
  cmOSXBundleGenerator* OSXBundleGenerator;
  std::set<std::string> MacContentFolders;

  void addPoolNinjaVariable(const std::string& pool_property,
                            cmGeneratorTarget* target, cmNinjaVars& vars);

  bool ForceResponseFile();

private:
  cmLocalNinjaGenerator* LocalGenerator;
  /// List of object files for this target.
  cmNinjaDeps Objects;
  cmNinjaDeps DDIFiles; // TODO: Make per-language.
  std::vector<cmCustomCommand const*> CustomCommands;
  cmNinjaDeps ExtraFiles;
};

#endif // ! cmNinjaTargetGenerator_h
