/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVisualStudioGeneratorOptions_h
#define cmVisualStudioGeneratorOptions_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

#include "cmGlobalVisualStudioGenerator.h"
#include "cmIDEFlagTable.h"
#include "cmIDEOptions.h"

class cmLocalVisualStudioGenerator;

typedef cmIDEFlagTable cmVS7FlagTable;

class cmVisualStudioGeneratorOptions : public cmIDEOptions
{
public:
  // Construct an options table for a given tool.
  enum Tool
  {
    Compiler,
    ResourceCompiler,
    CudaCompiler,
    MasmCompiler,
    NasmCompiler,
    Linker,
    FortranCompiler,
    CSharpCompiler
  };
  cmVisualStudioGeneratorOptions(cmLocalVisualStudioGenerator* lg, Tool tool,
                                 cmVS7FlagTable const* table = nullptr,
                                 cmVS7FlagTable const* extraTable = nullptr);

  // Add a table of flags.
  void AddTable(cmVS7FlagTable const* table);

  // Clear the flag tables.
  void ClearTables();

  // Store options from command line flags.
  void Parse(const std::string& flags);
  void ParseFinish();

  void PrependInheritedString(std::string const& key);

  // Parse the content of the given flag table entry again to extract
  // known flags and leave the rest in the original entry.
  void Reparse(std::string const& key);

  // Fix the ExceptionHandling option to default to off.
  void FixExceptionHandlingDefault();

  // Store options for verbose builds.
  void SetVerboseMakefile(bool verbose);

  // Check for specific options.
  bool UsingUnicode() const;
  bool UsingSBCS() const;

  enum CudaRuntime
  {
    CudaRuntimeStatic,
    CudaRuntimeShared,
    CudaRuntimeNone
  };
  CudaRuntime GetCudaRuntime() const;

  void FixCudaCodeGeneration();

  void FixManifestUACFlags();

  bool IsDebug() const;
  bool IsWinRt() const;
  bool IsManaged() const;
  // Write options to output.
  void OutputPreprocessorDefinitions(std::ostream& fout, int indent,
                                     const std::string& lang);
  void OutputAdditionalIncludeDirectories(std::ostream& fout, int indent,
                                          const std::string& lang);
  void OutputFlagMap(std::ostream& fout, int indent);
  void SetConfiguration(const std::string& config);
  const std::string& GetConfiguration() const;

protected:
  virtual void OutputFlag(std::ostream& fout, int indent, const char* tag,
                          const std::string& content) = 0;

private:
  cmLocalVisualStudioGenerator* LocalGenerator;
  cmGlobalVisualStudioGenerator::VSVersion Version;

  std::string Configuration;
  Tool CurrentTool;

  bool FortranRuntimeDebug;
  bool FortranRuntimeDLL;
  bool FortranRuntimeMT;

  std::string UnknownFlagField;

  void StoreUnknownFlag(std::string const& flag) override;

  FlagValue TakeFlag(std::string const& key);
};

#endif
