#include "cmVisualStudioGeneratorOptions.h"

#include "cmAlgorithms.h"
#include "cmLocalVisualStudioGenerator.h"
#include "cmOutputConverter.h"
#include "cmSystemTools.h"

static void cmVS10EscapeForMSBuild(std::string& ret)
{
  cmSystemTools::ReplaceString(ret, ";", "%3B");
}

cmVisualStudioGeneratorOptions::cmVisualStudioGeneratorOptions(
  cmLocalVisualStudioGenerator* lg, Tool tool, cmVS7FlagTable const* table,
  cmVS7FlagTable const* extraTable)
  : cmIDEOptions()
  , LocalGenerator(lg)
  , Version(lg->GetVersion())
  , CurrentTool(tool)
{
  // Store the given flag tables.
  this->AddTable(table);
  this->AddTable(extraTable);

  // Preprocessor definitions are not allowed for linker tools.
  this->AllowDefine = (tool != Linker);

  // include directories are not allowed for linker tools.
  this->AllowInclude = (tool != Linker);

  // Slash options are allowed for VS.
  this->AllowSlash = true;

  this->FortranRuntimeDebug = false;
  this->FortranRuntimeDLL = false;
  this->FortranRuntimeMT = false;

  this->UnknownFlagField = "AdditionalOptions";
}

void cmVisualStudioGeneratorOptions::AddTable(cmVS7FlagTable const* table)
{
  if (table) {
    for (int i = 0; i < FlagTableCount; ++i) {
      if (!this->FlagTable[i]) {
        this->FlagTable[i] = table;
        break;
      }
    }
  }
}

void cmVisualStudioGeneratorOptions::ClearTables()
{
  for (int i = 0; i < FlagTableCount; ++i) {
    this->FlagTable[i] = nullptr;
  }
}

void cmVisualStudioGeneratorOptions::FixExceptionHandlingDefault()
{
  // Exception handling is on by default because the platform file has
  // "/EHsc" in the flags.  Normally, that will override this
  // initialization to off, but the user has the option of removing
  // the flag to disable exception handling.  When the user does
  // remove the flag we need to override the IDE default of on.
  switch (this->Version) {
    case cmGlobalVisualStudioGenerator::VS10:
    case cmGlobalVisualStudioGenerator::VS11:
    case cmGlobalVisualStudioGenerator::VS12:
    case cmGlobalVisualStudioGenerator::VS14:
    case cmGlobalVisualStudioGenerator::VS15:
      // by default VS puts <ExceptionHandling></ExceptionHandling> empty
      // for a project, to make our projects look the same put a new line
      // and space over for the closing </ExceptionHandling> as the default
      // value
      this->FlagMap["ExceptionHandling"] = "\n      ";
      break;
    default:
      this->FlagMap["ExceptionHandling"] = "0";
      break;
  }
}

void cmVisualStudioGeneratorOptions::SetVerboseMakefile(bool verbose)
{
  // If verbose makefiles have been requested and the /nologo option
  // was not given explicitly in the flags we want to add an attribute
  // to the generated project to disable logo suppression.  Otherwise
  // the GUI default is to enable suppression.
  //
  // On Visual Studio 10 (and later!), the value of this attribute should be
  // an empty string, instead of "FALSE", in order to avoid a warning:
  //   "cl ... warning D9035: option 'nologo-' has been deprecated"
  //
  if (verbose &&
      this->FlagMap.find("SuppressStartupBanner") == this->FlagMap.end()) {
    this->FlagMap["SuppressStartupBanner"] =
      this->Version < cmGlobalVisualStudioGenerator::VS10 ? "FALSE" : "";
  }
}

bool cmVisualStudioGeneratorOptions::IsDebug() const
{
  if (this->CurrentTool != CSharpCompiler) {
    return this->FlagMap.find("DebugInformationFormat") != this->FlagMap.end();
  }
  std::map<std::string, FlagValue>::const_iterator i =
    this->FlagMap.find("DebugType");
  if (i != this->FlagMap.end()) {
    if (i->second.size() == 1) {
      return i->second[0] != "none";
    }
  }
  return false;
}

bool cmVisualStudioGeneratorOptions::IsWinRt() const
{
  return this->FlagMap.find("CompileAsWinRT") != this->FlagMap.end();
}

bool cmVisualStudioGeneratorOptions::IsManaged() const
{
  return this->FlagMap.find("CompileAsManaged") != this->FlagMap.end();
}

bool cmVisualStudioGeneratorOptions::UsingUnicode() const
{
  // Look for a _UNICODE definition.
  for (std::string const& di : this->Defines) {
    if (di == "_UNICODE") {
      return true;
    }
  }
  return false;
}
bool cmVisualStudioGeneratorOptions::UsingSBCS() const
{
  // Look for a _SBCS definition.
  for (std::string const& di : this->Defines) {
    if (di == "_SBCS") {
      return true;
    }
  }
  return false;
}

cmVisualStudioGeneratorOptions::CudaRuntime
cmVisualStudioGeneratorOptions::GetCudaRuntime() const
{
  std::map<std::string, FlagValue>::const_iterator i =
    this->FlagMap.find("CudaRuntime");
  if (i != this->FlagMap.end() && i->second.size() == 1) {
    std::string const& cudaRuntime = i->second[0];
    if (cudaRuntime == "Static") {
      return CudaRuntimeStatic;
    }
    if (cudaRuntime == "Shared") {
      return CudaRuntimeShared;
    }
    if (cudaRuntime == "None") {
      return CudaRuntimeNone;
    }
  }
  // nvcc default is static
  return CudaRuntimeStatic;
}

void cmVisualStudioGeneratorOptions::FixCudaCodeGeneration()
{
  // Extract temporary values stored by our flag table.
  FlagValue arch = this->TakeFlag("cmake-temp-arch");
  FlagValue code = this->TakeFlag("cmake-temp-code");
  FlagValue gencode = this->TakeFlag("cmake-temp-gencode");

  // No -code allowed without -arch.
  if (arch.empty()) {
    code.clear();
  }

  if (arch.empty() && gencode.empty()) {
    return;
  }

  // Create a CodeGeneration field with [arch],[code] syntax in each entry.
  // CUDA will convert it to `-gencode=arch=[arch],code="[code],[arch]"`.
  FlagValue& result = this->FlagMap["CodeGeneration"];

  // First entries for the -arch=<arch> [-code=<code>,...] pair.
  if (!arch.empty()) {
    std::string arch_name = arch[0];
    std::vector<std::string> codes;
    if (!code.empty()) {
      codes = cmSystemTools::tokenize(code[0], ",");
    }
    if (codes.empty()) {
      codes.push_back(arch_name);
      // nvcc -arch=<arch> has a special case that allows a real
      // architecture to be specified instead of a virtual arch.
      // It translates to -arch=<virtual> -code=<real>.
      cmSystemTools::ReplaceString(arch_name, "sm_", "compute_");
    }
    for (std::string const& c : codes) {
      std::string entry = arch_name + "," + c;
      result.push_back(entry);
    }
  }

  // Now add entries for the following signatures:
  // -gencode=<arch>,<code>
  // -gencode=<arch>,[<code1>,<code2>]
  // -gencode=<arch>,"<code1>,<code2>"
  for (std::string const& e : gencode) {
    std::string entry = e;
    cmSystemTools::ReplaceString(entry, "arch=", "");
    cmSystemTools::ReplaceString(entry, "code=", "");
    cmSystemTools::ReplaceString(entry, "[", "");
    cmSystemTools::ReplaceString(entry, "]", "");
    cmSystemTools::ReplaceString(entry, "\"", "");

    std::vector<std::string> codes = cmSystemTools::tokenize(entry, ",");
    if (codes.size() >= 2) {
      auto gencode_arch = cm::cbegin(codes);
      for (auto ci = gencode_arch + 1; ci != cm::cend(codes); ++ci) {
        std::string code_entry = *gencode_arch + "," + *ci;
        result.push_back(code_entry);
      }
    }
  }
}

void cmVisualStudioGeneratorOptions::FixManifestUACFlags()
{
  static std::string const ENABLE_UAC = "EnableUAC";
  if (!HasFlag(ENABLE_UAC)) {
    return;
  }

  const std::string uacFlag = GetFlag(ENABLE_UAC);
  std::vector<std::string> subOptions;
  cmsys::SystemTools::Split(uacFlag, subOptions, ' ');
  if (subOptions.empty()) {
    AddFlag(ENABLE_UAC, "true");
    return;
  }

  if (subOptions.size() == 1 && subOptions[0] == "NO") {
    AddFlag(ENABLE_UAC, "false");
    return;
  }

  std::map<std::string, std::string> uacMap;
  uacMap["level"] = "UACExecutionLevel";
  uacMap["uiAccess"] = "UACUIAccess";

  std::map<std::string, std::string> uacExecuteLevelMap;
  uacExecuteLevelMap["asInvoker"] = "AsInvoker";
  uacExecuteLevelMap["highestAvailable"] = "HighestAvailable";
  uacExecuteLevelMap["requireAdministrator"] = "RequireAdministrator";

  for (std::string const& subopt : subOptions) {
    std::vector<std::string> keyValue;
    cmsys::SystemTools::Split(subopt, keyValue, '=');
    if (keyValue.size() != 2 || (uacMap.find(keyValue[0]) == uacMap.end())) {
      // ignore none key=value option or unknown flags
      continue;
    }

    if (keyValue[1].front() == '\'' && keyValue[1].back() == '\'') {
      keyValue[1] =
        keyValue[1].substr(1, std::max<int>(0, keyValue[1].size() - 2));
    }

    if (keyValue[0] == "level") {
      if (uacExecuteLevelMap.find(keyValue[1]) == uacExecuteLevelMap.end()) {
        // unknown level value
        continue;
      }

      AddFlag(uacMap[keyValue[0]], uacExecuteLevelMap[keyValue[1]]);
      continue;
    }

    if (keyValue[0] == "uiAccess") {
      if (keyValue[1] != "true" && keyValue[1] != "false") {
        // unknown uiAccess value
        continue;
      }
      AddFlag(uacMap[keyValue[0]], keyValue[1]);
      continue;
    }

    // unknown sub option
  }

  AddFlag(ENABLE_UAC, "true");
}

void cmVisualStudioGeneratorOptions::Parse(const std::string& flags)
{
  // Parse the input string as a windows command line since the string
  // is intended for writing directly into the build files.
  std::vector<std::string> args;
  cmSystemTools::ParseWindowsCommandLine(flags.c_str(), args);

  // Process flags that need to be represented specially in the IDE
  // project file.
  for (std::string const& ai : args) {
    this->HandleFlag(ai);
  }
}

void cmVisualStudioGeneratorOptions::ParseFinish()
{
  if (this->CurrentTool == FortranCompiler) {
    // "RuntimeLibrary" attribute values:
    //  "rtMultiThreaded", "0", /threads /libs:static
    //  "rtMultiThreadedDLL", "2", /threads /libs:dll
    //  "rtMultiThreadedDebug", "1", /threads /dbglibs /libs:static
    //  "rtMultiThreadedDebugDLL", "3", /threads /dbglibs /libs:dll
    // These seem unimplemented by the IDE:
    //  "rtSingleThreaded", "4", /libs:static
    //  "rtSingleThreadedDLL", "10", /libs:dll
    //  "rtSingleThreadedDebug", "5", /dbglibs /libs:static
    //  "rtSingleThreadedDebugDLL", "11", /dbglibs /libs:dll
    std::string rl = "rtMultiThreaded";
    rl += this->FortranRuntimeDebug ? "Debug" : "";
    rl += this->FortranRuntimeDLL ? "DLL" : "";
    this->FlagMap["RuntimeLibrary"] = rl;
  }

  if (this->CurrentTool == CudaCompiler) {
    std::map<std::string, FlagValue>::iterator i =
      this->FlagMap.find("CudaRuntime");
    if (i != this->FlagMap.end() && i->second.size() == 1) {
      std::string& cudaRuntime = i->second[0];
      if (cudaRuntime == "static") {
        cudaRuntime = "Static";
      } else if (cudaRuntime == "shared") {
        cudaRuntime = "Shared";
      } else if (cudaRuntime == "none") {
        cudaRuntime = "None";
      }
    }
  }
}

void cmVisualStudioGeneratorOptions::PrependInheritedString(
  std::string const& key)
{
  std::map<std::string, FlagValue>::iterator i = this->FlagMap.find(key);
  if (i == this->FlagMap.end() || i->second.size() != 1) {
    return;
  }
  std::string& value = i->second[0];
  value = "%(" + key + ") " + value;
}

void cmVisualStudioGeneratorOptions::Reparse(std::string const& key)
{
  std::map<std::string, FlagValue>::iterator i = this->FlagMap.find(key);
  if (i == this->FlagMap.end() || i->second.size() != 1) {
    return;
  }
  std::string const original = i->second[0];
  i->second[0] = "";
  this->UnknownFlagField = key;
  this->Parse(original);
}

void cmVisualStudioGeneratorOptions::StoreUnknownFlag(std::string const& flag)
{
  // Look for Intel Fortran flags that do not map well in the flag table.
  if (this->CurrentTool == FortranCompiler) {
    if (flag == "/dbglibs") {
      this->FortranRuntimeDebug = true;
      return;
    }
    if (flag == "/threads") {
      this->FortranRuntimeMT = true;
      return;
    }
    if (flag == "/libs:dll") {
      this->FortranRuntimeDLL = true;
      return;
    }
    if (flag == "/libs:static") {
      this->FortranRuntimeDLL = false;
      return;
    }
  }

  // This option is not known.  Store it in the output flags.
  std::string const opts = cmOutputConverter::EscapeWindowsShellArgument(
    flag.c_str(),
    cmOutputConverter::Shell_Flag_AllowMakeVariables |
      cmOutputConverter::Shell_Flag_VSIDE);
  this->AppendFlagString(this->UnknownFlagField, opts);
}

cmIDEOptions::FlagValue cmVisualStudioGeneratorOptions::TakeFlag(
  std::string const& key)
{
  FlagValue value;
  std::map<std::string, FlagValue>::iterator i = this->FlagMap.find(key);
  if (i != this->FlagMap.end()) {
    value = i->second;
    this->FlagMap.erase(i);
  }
  return value;
}

void cmVisualStudioGeneratorOptions::SetConfiguration(
  const std::string& config)
{
  this->Configuration = config;
}

const std::string& cmVisualStudioGeneratorOptions::GetConfiguration() const
{
  return this->Configuration;
}

void cmVisualStudioGeneratorOptions::OutputPreprocessorDefinitions(
  std::ostream& fout, int indent, const std::string& lang)
{
  if (this->Defines.empty()) {
    return;
  }
  const char* tag = "PreprocessorDefinitions";
  if (lang == "CUDA") {
    tag = "Defines";
  }

  std::ostringstream oss;
  const char* sep = "";
  std::vector<std::string>::const_iterator de =
    cmRemoveDuplicates(this->Defines);
  for (std::vector<std::string>::const_iterator di = this->Defines.begin();
       di != de; ++di) {
    // Escape the definition for the compiler.
    std::string define;
    if (this->Version < cmGlobalVisualStudioGenerator::VS10) {
      define = this->LocalGenerator->EscapeForShell(*di, true);
    } else {
      define = *di;
    }
    // Escape this flag for the MSBuild.
    if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
      cmVS10EscapeForMSBuild(define);
      if (lang == "RC") {
        cmSystemTools::ReplaceString(define, "\"", "\\\"");
      }
    }
    // Store the flag in the project file.
    oss << sep << define;
    sep = ";";
  }
  if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
    oss << ";%(" << tag << ")";
  }

  this->OutputFlag(fout, indent, tag, oss.str());
}

void cmVisualStudioGeneratorOptions::OutputAdditionalIncludeDirectories(
  std::ostream& fout, int indent, const std::string& lang)
{
  if (this->Includes.empty()) {
    return;
  }

  const char* tag = "AdditionalIncludeDirectories";
  if (lang == "CUDA") {
    tag = "Include";
  } else if (lang == "ASM_MASM" || lang == "ASM_NASM") {
    tag = "IncludePaths";
  }

  std::ostringstream oss;
  const char* sep = "";
  for (std::string include : this->Includes) {
    // first convert all of the slashes
    std::string::size_type pos = 0;
    while ((pos = include.find('/', pos)) != std::string::npos) {
      include[pos] = '\\';
      pos++;
    }

    if (lang == "ASM_NASM") {
      include += "\\";
    }

    // Escape this include for the MSBuild.
    if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
      cmVS10EscapeForMSBuild(include);
    }
    oss << sep << include;
    sep = ";";

    if (lang == "Fortran") {
      include += "/$(ConfigurationName)";
      oss << sep << include;
    }
  }

  if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
    oss << sep << "%(" << tag << ")";
  }

  this->OutputFlag(fout, indent, tag, oss.str());
}

void cmVisualStudioGeneratorOptions::OutputFlagMap(std::ostream& fout,
                                                   int indent)
{
  for (auto const& m : this->FlagMap) {
    std::ostringstream oss;
    const char* sep = "";
    for (std::string i : m.second) {
      if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
        cmVS10EscapeForMSBuild(i);
      }
      oss << sep << i;
      sep = ";";
    }

    this->OutputFlag(fout, indent, m.first.c_str(), oss.str());
  }
}
