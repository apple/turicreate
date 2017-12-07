#include "cmVisualStudioGeneratorOptions.h"

#include "cmAlgorithms.h"
#include "cmLocalVisualStudioGenerator.h"
#include "cmOutputConverter.h"
#include "cmSystemTools.h"
#include "cmVisualStudio10TargetGenerator.h"

static std::string cmVisualStudio10GeneratorOptionsEscapeForXML(
  std::string ret)
{
  cmSystemTools::ReplaceString(ret, ";", "%3B");
  cmSystemTools::ReplaceString(ret, "&", "&amp;");
  cmSystemTools::ReplaceString(ret, "<", "&lt;");
  cmSystemTools::ReplaceString(ret, ">", "&gt;");
  return ret;
}

static std::string cmVisualStudioGeneratorOptionsEscapeForXML(std::string ret)
{
  cmSystemTools::ReplaceString(ret, "&", "&amp;");
  cmSystemTools::ReplaceString(ret, "\"", "&quot;");
  cmSystemTools::ReplaceString(ret, "<", "&lt;");
  cmSystemTools::ReplaceString(ret, ">", "&gt;");
  cmSystemTools::ReplaceString(ret, "\n", "&#x0D;&#x0A;");
  return ret;
}

cmVisualStudioGeneratorOptions::cmVisualStudioGeneratorOptions(
  cmLocalVisualStudioGenerator* lg, Tool tool,
  cmVisualStudio10TargetGenerator* g)
  : cmIDEOptions()
  , LocalGenerator(lg)
  , Version(lg->GetVersion())
  , CurrentTool(tool)
  , TargetGenerator(g)
{
  // Preprocessor definitions are not allowed for linker tools.
  this->AllowDefine = (tool != Linker);

  // Slash options are allowed for VS.
  this->AllowSlash = true;

  this->FortranRuntimeDebug = false;
  this->FortranRuntimeDLL = false;
  this->FortranRuntimeMT = false;

  this->UnknownFlagField = "AdditionalOptions";
}

cmVisualStudioGeneratorOptions::cmVisualStudioGeneratorOptions(
  cmLocalVisualStudioGenerator* lg, Tool tool, cmVS7FlagTable const* table,
  cmVS7FlagTable const* extraTable, cmVisualStudio10TargetGenerator* g)
  : cmIDEOptions()
  , LocalGenerator(lg)
  , Version(lg->GetVersion())
  , CurrentTool(tool)
  , TargetGenerator(g)
{
  // Store the given flag tables.
  this->AddTable(table);
  this->AddTable(extraTable);

  // Preprocessor definitions are not allowed for linker tools.
  this->AllowDefine = (tool != Linker);

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
    this->FlagTable[i] = CM_NULLPTR;
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
  // Look for the a _UNICODE definition.
  for (std::vector<std::string>::const_iterator di = this->Defines.begin();
       di != this->Defines.end(); ++di) {
    if (*di == "_UNICODE") {
      return true;
    }
  }
  return false;
}
bool cmVisualStudioGeneratorOptions::UsingSBCS() const
{
  // Look for the a _SBCS definition.
  for (std::vector<std::string>::const_iterator di = this->Defines.begin();
       di != this->Defines.end(); ++di) {
    if (*di == "_SBCS") {
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
    for (std::vector<std::string>::iterator ci = codes.begin();
         ci != codes.end(); ++ci) {
      std::string entry = arch_name + "," + *ci;
      result.push_back(entry);
    }
  }

  // Now add entries for the -gencode=<arch>,<code> pairs.
  for (std::vector<std::string>::iterator ei = gencode.begin();
       ei != gencode.end(); ++ei) {
    std::string entry = *ei;
    cmSystemTools::ReplaceString(entry, "arch=", "");
    cmSystemTools::ReplaceString(entry, "code=", "");
    result.push_back(entry);
  }
}

void cmVisualStudioGeneratorOptions::Parse(const char* flags)
{
  // Parse the input string as a windows command line since the string
  // is intended for writing directly into the build files.
  std::vector<std::string> args;
  cmSystemTools::ParseWindowsCommandLine(flags, args);

  // Process flags that need to be represented specially in the IDE
  // project file.
  for (std::vector<std::string>::iterator ai = args.begin(); ai != args.end();
       ++ai) {
    this->HandleFlag(ai->c_str());
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
  this->Parse(original.c_str());
}

void cmVisualStudioGeneratorOptions::StoreUnknownFlag(const char* flag)
{
  // Look for Intel Fortran flags that do not map well in the flag table.
  if (this->CurrentTool == FortranCompiler) {
    if (strcmp(flag, "/dbglibs") == 0) {
      this->FortranRuntimeDebug = true;
      return;
    }
    if (strcmp(flag, "/threads") == 0) {
      this->FortranRuntimeMT = true;
      return;
    }
    if (strcmp(flag, "/libs:dll") == 0) {
      this->FortranRuntimeDLL = true;
      return;
    }
    if (strcmp(flag, "/libs:static") == 0) {
      this->FortranRuntimeDLL = false;
      return;
    }
  }

  // This option is not known.  Store it in the output flags.
  std::string const opts = cmOutputConverter::EscapeWindowsShellArgument(
    flag, cmOutputConverter::Shell_Flag_AllowMakeVariables |
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

void cmVisualStudioGeneratorOptions::SetConfiguration(const char* config)
{
  this->Configuration = config;
}

void cmVisualStudioGeneratorOptions::OutputPreprocessorDefinitions(
  std::ostream& fout, const char* prefix, const char* suffix,
  const std::string& lang)
{
  if (this->Defines.empty()) {
    return;
  }
  const char* tag = "PreprocessorDefinitions";
  if (lang == "CUDA") {
    tag = "Defines";
  }
  if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
    // if there are configuration specific flags, then
    // use the configuration specific tag for PreprocessorDefinitions
    if (!this->Configuration.empty()) {
      fout << prefix;
      this->TargetGenerator->WritePlatformConfigTag(
        tag, this->Configuration.c_str(), 0, 0, 0, &fout);
    } else {
      fout << prefix << "<" << tag << ">";
    }
  } else {
    fout << prefix << tag << "=\"";
  }
  const char* sep = "";
  std::vector<std::string>::const_iterator de =
    cmRemoveDuplicates(this->Defines);
  for (std::vector<std::string>::const_iterator di = this->Defines.begin();
       di != de; ++di) {
    // Escape the definition for the compiler.
    std::string define;
    if (this->Version < cmGlobalVisualStudioGenerator::VS10) {
      define = this->LocalGenerator->EscapeForShell(di->c_str(), true);
    } else {
      define = *di;
    }
    // Escape this flag for the IDE.
    if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
      define = cmVisualStudio10GeneratorOptionsEscapeForXML(define);

      if (lang == "RC") {
        cmSystemTools::ReplaceString(define, "\"", "\\\"");
      }
    } else {
      define = cmVisualStudioGeneratorOptionsEscapeForXML(define);
    }
    // Store the flag in the project file.
    fout << sep << define;
    sep = ";";
  }
  if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
    fout << ";%(" << tag << ")</" << tag << ">" << suffix;
  } else {
    fout << "\"" << suffix;
  }
}

void cmVisualStudioGeneratorOptions::OutputFlagMap(std::ostream& fout,
                                                   const char* indent)
{
  if (this->Version >= cmGlobalVisualStudioGenerator::VS10) {
    for (std::map<std::string, FlagValue>::iterator m = this->FlagMap.begin();
         m != this->FlagMap.end(); ++m) {
      fout << indent;
      if (!this->Configuration.empty()) {
        this->TargetGenerator->WritePlatformConfigTag(
          m->first.c_str(), this->Configuration.c_str(), 0, 0, 0, &fout);
      } else {
        fout << "<" << m->first << ">";
      }
      const char* sep = "";
      for (std::vector<std::string>::iterator i = m->second.begin();
           i != m->second.end(); ++i) {
        fout << sep << cmVisualStudio10GeneratorOptionsEscapeForXML(*i);
        sep = ";";
      }
      fout << "</" << m->first << ">\n";
    }
  } else {
    for (std::map<std::string, FlagValue>::iterator m = this->FlagMap.begin();
         m != this->FlagMap.end(); ++m) {
      fout << indent << m->first << "=\"";
      const char* sep = "";
      for (std::vector<std::string>::iterator i = m->second.begin();
           i != m->second.end(); ++i) {
        fout << sep << cmVisualStudioGeneratorOptionsEscapeForXML(*i);
        sep = ";";
      }
      fout << "\"\n";
    }
  }
}
