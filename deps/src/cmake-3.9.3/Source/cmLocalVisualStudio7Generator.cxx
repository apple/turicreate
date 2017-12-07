/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalVisualStudio7Generator.h"

#include "cmCustomCommandGenerator.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalVisualStudio7Generator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"
#include "cm_expat.h"
#include "cmake.h"

#include "cmComputeLinkInformation.h"
#include "cmGeneratedFileStream.h"

#include <ctype.h> // for isspace
#include <windows.h>

static bool cmLVS7G_IsFAT(const char* dir);

class cmLocalVisualStudio7GeneratorInternals
{
public:
  cmLocalVisualStudio7GeneratorInternals(cmLocalVisualStudio7Generator* e)
    : LocalGenerator(e)
  {
  }
  typedef cmComputeLinkInformation::ItemVector ItemVector;
  void OutputLibraries(std::ostream& fout, ItemVector const& libs);
  void OutputObjects(std::ostream& fout, cmGeneratorTarget* t,
                     std::string const& config, const char* isep = 0);

private:
  cmLocalVisualStudio7Generator* LocalGenerator;
};

extern cmVS7FlagTable cmLocalVisualStudio7GeneratorFlagTable[];

static void cmConvertToWindowsSlash(std::string& s)
{
  std::string::size_type pos = 0;
  while ((pos = s.find('/', pos)) != std::string::npos) {
    s[pos] = '\\';
    pos++;
  }
}

cmLocalVisualStudio7Generator::cmLocalVisualStudio7Generator(
  cmGlobalGenerator* gg, cmMakefile* mf)
  : cmLocalVisualStudioGenerator(gg, mf)
{
  this->Internal = new cmLocalVisualStudio7GeneratorInternals(this);
}

cmLocalVisualStudio7Generator::~cmLocalVisualStudio7Generator()
{
  delete this->Internal;
}

void cmLocalVisualStudio7Generator::AddHelperCommands()
{
  // Now create GUIDs for targets
  std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); ++l) {
    if ((*l)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    const char* path = (*l)->GetProperty("EXTERNAL_MSPROJECT");
    if (path) {
      this->ReadAndStoreExternalGUID((*l)->GetName().c_str(), path);
    }
  }

  this->FixGlobalTargets();
}

void cmLocalVisualStudio7Generator::Generate()
{
  this->WriteProjectFiles();
  this->WriteStampFiles();
}

void cmLocalVisualStudio7Generator::AddCMakeListsRules()
{
  // Create the regeneration custom rule.
  if (!this->Makefile->IsOn("CMAKE_SUPPRESS_REGENERATION")) {
    // Create a rule to regenerate the build system when the target
    // specification source changes.
    if (cmSourceFile* sf = this->CreateVCProjBuildRule()) {
      // Add the rule to targets that need it.
      std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();
      for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
           l != tgts.end(); ++l) {
        if ((*l)->GetType() == cmStateEnums::GLOBAL_TARGET) {
          continue;
        }
        if ((*l)->GetName() != CMAKE_CHECK_BUILD_SYSTEM_TARGET) {
          (*l)->AddSource(sf->GetFullPath());
        }
      }
    }
  }
}

void cmLocalVisualStudio7Generator::FixGlobalTargets()
{
  // Visual Studio .NET 2003 Service Pack 1 will not run post-build
  // commands for targets in which no sources are built.  Add dummy
  // rules to force these targets to build.
  std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); l++) {
    if ((*l)->GetType() == cmStateEnums::GLOBAL_TARGET) {
      std::vector<std::string> no_depends;
      cmCustomCommandLine force_command;
      force_command.push_back("cd");
      force_command.push_back(".");
      cmCustomCommandLines force_commands;
      force_commands.push_back(force_command);
      std::string no_main_dependency = "";
      std::string force = this->GetCurrentBinaryDirectory();
      force += cmake::GetCMakeFilesDirectory();
      force += "/";
      force += (*l)->GetName();
      force += "_force";
      if (cmSourceFile* file = this->Makefile->AddCustomCommandToOutput(
            force.c_str(), no_depends, no_main_dependency, force_commands, " ",
            0, true)) {
        (*l)->AddSource(file->GetFullPath());
      }
    }
  }
}

// TODO
// for CommandLine= need to repleace quotes with &quot
// write out configurations
void cmLocalVisualStudio7Generator::WriteProjectFiles()
{
  // If not an in source build, then create the output directory
  if (strcmp(this->GetCurrentBinaryDirectory(), this->GetSourceDirectory()) !=
      0) {
    if (!cmSystemTools::MakeDirectory(this->GetCurrentBinaryDirectory())) {
      cmSystemTools::Error("Error creating directory ",
                           this->GetCurrentBinaryDirectory());
    }
  }

  // Get the set of targets in this directory.
  std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();

  // Create the project file for each target.
  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); l++) {
    if ((*l)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    // INCLUDE_EXTERNAL_MSPROJECT command only affects the workspace
    // so don't build a projectfile for it
    if (!(*l)->GetProperty("EXTERNAL_MSPROJECT")) {
      this->CreateSingleVCProj((*l)->GetName().c_str(), *l);
    }
  }
}

void cmLocalVisualStudio7Generator::WriteStampFiles()
{
  // Touch a timestamp file used to determine when the project file is
  // out of date.
  std::string stampName = this->GetCurrentBinaryDirectory();
  stampName += cmake::GetCMakeFilesDirectory();
  cmSystemTools::MakeDirectory(stampName.c_str());
  stampName += "/";
  stampName += "generate.stamp";
  cmsys::ofstream stamp(stampName.c_str());
  stamp << "# CMake generation timestamp file for this directory.\n";

  // Create a helper file so CMake can determine when it is run
  // through the rule created by CreateVCProjBuildRule whether it
  // really needs to regenerate the project.  This file lists its own
  // dependencies.  If any file listed in it is newer than itself then
  // CMake must rerun.  Otherwise the project files are up to date and
  // the stamp file can just be touched.
  std::string depName = stampName;
  depName += ".depend";
  cmsys::ofstream depFile(depName.c_str());
  depFile << "# CMake generation dependency list for this directory.\n";
  std::vector<std::string> const& listFiles = this->Makefile->GetListFiles();
  for (std::vector<std::string>::const_iterator lf = listFiles.begin();
       lf != listFiles.end(); ++lf) {
    depFile << *lf << std::endl;
  }
}

void cmLocalVisualStudio7Generator::CreateSingleVCProj(
  const std::string& lname, cmGeneratorTarget* target)
{
  cmGlobalVisualStudioGenerator* gg =
    static_cast<cmGlobalVisualStudioGenerator*>(this->GlobalGenerator);
  this->FortranProject = gg->TargetIsFortranOnly(target);
  this->WindowsCEProject = gg->TargetsWindowsCE();

  // Intel Fortran for VS10 uses VS9 format ".vfproj" files.
  cmGlobalVisualStudioGenerator::VSVersion realVersion = gg->GetVersion();
  if (this->FortranProject &&
      gg->GetVersion() >= cmGlobalVisualStudioGenerator::VS10) {
    gg->SetVersion(cmGlobalVisualStudioGenerator::VS9);
  }

  // add to the list of projects
  target->Target->SetProperty("GENERATOR_FILE_NAME", lname.c_str());
  // create the dsp.cmake file
  std::string fname;
  fname = this->GetCurrentBinaryDirectory();
  fname += "/";
  fname += lname;
  if (this->FortranProject) {
    fname += ".vfproj";
  } else {
    fname += ".vcproj";
  }

  // Generate the project file and replace it atomically with
  // copy-if-different.  We use a separate timestamp so that the IDE
  // does not reload project files unnecessarily.
  cmGeneratedFileStream fout(fname.c_str());
  fout.SetCopyIfDifferent(true);
  this->WriteVCProjFile(fout, lname, target);
  if (fout.Close()) {
    this->GlobalGenerator->FileReplacedDuringGenerate(fname);
  }

  gg->SetVersion(realVersion);
}

cmSourceFile* cmLocalVisualStudio7Generator::CreateVCProjBuildRule()
{
  std::string stampName = this->GetCurrentBinaryDirectory();
  stampName += "/";
  stampName += cmake::GetCMakeFilesDirectoryPostSlash();
  stampName += "generate.stamp";
  cmCustomCommandLine commandLine;
  commandLine.push_back(cmSystemTools::GetCMakeCommand());
  std::string makefileIn = this->GetCurrentSourceDirectory();
  makefileIn += "/";
  makefileIn += "CMakeLists.txt";
  makefileIn = cmSystemTools::CollapseFullPath(makefileIn.c_str());
  if (!cmSystemTools::FileExists(makefileIn.c_str())) {
    return 0;
  }
  std::string comment = "Building Custom Rule ";
  comment += makefileIn;
  std::string args;
  args = "-H";
  args += this->GetSourceDirectory();
  commandLine.push_back(args);
  args = "-B";
  args += this->GetBinaryDirectory();
  commandLine.push_back(args);
  commandLine.push_back("--check-stamp-file");
  commandLine.push_back(stampName);

  std::vector<std::string> const& listFiles = this->Makefile->GetListFiles();

  cmCustomCommandLines commandLines;
  commandLines.push_back(commandLine);
  const char* no_working_directory = 0;
  std::string fullpathStampName =
    cmSystemTools::CollapseFullPath(stampName.c_str());
  this->Makefile->AddCustomCommandToOutput(
    fullpathStampName.c_str(), listFiles, makefileIn.c_str(), commandLines,
    comment.c_str(), no_working_directory, true, false);
  if (cmSourceFile* file = this->Makefile->GetSource(makefileIn.c_str())) {
    return file;
  } else {
    cmSystemTools::Error("Error adding rule for ", makefileIn.c_str());
    return 0;
  }
}

void cmLocalVisualStudio7Generator::WriteConfigurations(
  std::ostream& fout, std::vector<std::string> const& configs,
  const std::string& libName, cmGeneratorTarget* target)
{
  fout << "\t<Configurations>\n";
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    this->WriteConfiguration(fout, i->c_str(), libName, target);
  }
  fout << "\t</Configurations>\n";
}
cmVS7FlagTable cmLocalVisualStudio7GeneratorFortranFlagTable[] = {
  { "Preprocess", "fpp", "Run Preprocessor on files", "preprocessYes", 0 },
  { "SuppressStartupBanner", "nologo", "SuppressStartupBanner", "true", 0 },
  { "SourceFileFormat", "fixed", "Use Fixed Format", "fileFormatFixed", 0 },
  { "SourceFileFormat", "free", "Use Free Format", "fileFormatFree", 0 },
  { "DebugInformationFormat", "Zi", "full debug", "debugEnabled", 0 },
  { "DebugInformationFormat", "debug:full", "full debug", "debugEnabled", 0 },
  { "DebugInformationFormat", "Z7", "c7 compat", "debugOldStyleInfo", 0 },
  { "DebugInformationFormat", "Zd", "line numbers", "debugLineInfoOnly", 0 },
  { "Optimization", "Od", "disable optimization", "optimizeDisabled", 0 },
  { "Optimization", "O1", "min space", "optimizeMinSpace", 0 },
  { "Optimization", "O3", "full optimize", "optimizeFull", 0 },
  { "GlobalOptimizations", "Og", "global optimize", "true", 0 },
  { "InlineFunctionExpansion", "Ob0", "", "expandDisable", 0 },
  { "InlineFunctionExpansion", "Ob1", "", "expandOnlyInline", 0 },
  { "FavorSizeOrSpeed", "Os", "", "favorSize", 0 },
  { "OmitFramePointers", "Oy-", "", "false", 0 },
  { "OptimizeForProcessor", "GB", "", "procOptimizeBlended", 0 },
  { "OptimizeForProcessor", "G5", "", "procOptimizePentium", 0 },
  { "OptimizeForProcessor", "G6", "", "procOptimizePentiumProThruIII", 0 },
  { "UseProcessorExtensions", "QzxK", "", "codeForStreamingSIMD", 0 },
  { "OptimizeForProcessor", "QaxN", "", "codeForPentium4", 0 },
  { "OptimizeForProcessor", "QaxB", "", "codeForPentiumM", 0 },
  { "OptimizeForProcessor", "QaxP", "", "codeForCodeNamedPrescott", 0 },
  { "OptimizeForProcessor", "QaxT", "", "codeForCore2Duo", 0 },
  { "OptimizeForProcessor", "QxK", "", "codeExclusivelyStreamingSIMD", 0 },
  { "OptimizeForProcessor", "QxN", "", "codeExclusivelyPentium4", 0 },
  { "OptimizeForProcessor", "QxB", "", "codeExclusivelyPentiumM", 0 },
  { "OptimizeForProcessor", "QxP", "", "codeExclusivelyCodeNamedPrescott", 0 },
  { "OptimizeForProcessor", "QxT", "", "codeExclusivelyCore2Duo", 0 },
  { "OptimizeForProcessor", "QxO", "", "codeExclusivelyCore2StreamingSIMD",
    0 },
  { "OptimizeForProcessor", "QxS", "", "codeExclusivelyCore2StreamingSIMD4",
    0 },
  { "OpenMP", "Qopenmp", "", "OpenMPParallelCode", 0 },
  { "OpenMP", "Qopenmp-stubs", "", "OpenMPSequentialCode", 0 },
  { "Traceback", "traceback", "", "true", 0 },
  { "Traceback", "notraceback", "", "false", 0 },
  { "FloatingPointExceptionHandling", "fpe:0", "", "fpe0", 0 },
  { "FloatingPointExceptionHandling", "fpe:1", "", "fpe1", 0 },
  { "FloatingPointExceptionHandling", "fpe:3", "", "fpe3", 0 },

  { "MultiProcessorCompilation", "MP", "", "true",
    cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue },
  { "ProcessorNumber", "MP", "Multi-processor Compilation", "",
    cmVS7FlagTable::UserValueRequired },

  { "ModulePath", "module:", "", "", cmVS7FlagTable::UserValueRequired },
  { "LoopUnrolling", "Qunroll:", "", "", cmVS7FlagTable::UserValueRequired },
  { "AutoParallelThreshold", "Qpar-threshold:", "", "",
    cmVS7FlagTable::UserValueRequired },
  { "HeapArrays", "heap-arrays:", "", "", cmVS7FlagTable::UserValueRequired },
  { "ObjectText", "bintext:", "", "", cmVS7FlagTable::UserValueRequired },
  { "Parallelization", "Qparallel", "", "true", 0 },
  { "PrefetchInsertion", "Qprefetch-", "", "false", 0 },
  { "BufferedIO", "assume:buffered_io", "", "true", 0 },
  { "CallingConvention", "iface:stdcall", "", "callConventionStdCall", 0 },
  { "CallingConvention", "iface:cref", "", "callConventionCRef", 0 },
  { "CallingConvention", "iface:stdref", "", "callConventionStdRef", 0 },
  { "CallingConvention", "iface:stdcall", "", "callConventionStdCall", 0 },
  { "CallingConvention", "iface:cvf", "", "callConventionCVF", 0 },
  { "EnableRecursion", "recursive", "", "true", 0 },
  { "ReentrantCode", "reentrancy", "", "true", 0 },
  // done up to Language
  { 0, 0, 0, 0, 0 }
};
// fill the table here currently the comment field is not used for
// anything other than documentation NOTE: Make sure the longer
// commandFlag comes FIRST!
cmVS7FlagTable cmLocalVisualStudio7GeneratorFlagTable[] = {
  // option flags (some flags map to the same option)
  { "BasicRuntimeChecks", "GZ", "Stack frame checks", "1", 0 },
  { "BasicRuntimeChecks", "RTCsu", "Both stack and uninitialized checks", "3",
    0 },
  { "BasicRuntimeChecks", "RTCs", "Stack frame checks", "1", 0 },
  { "BasicRuntimeChecks", "RTCu", "Uninitialized Variables ", "2", 0 },
  { "BasicRuntimeChecks", "RTC1", "Both stack and uninitialized checks", "3",
    0 },
  { "DebugInformationFormat", "Z7", "debug format", "1", 0 },
  { "DebugInformationFormat", "Zd", "debug format", "2", 0 },
  { "DebugInformationFormat", "Zi", "debug format", "3", 0 },
  { "DebugInformationFormat", "ZI", "debug format", "4", 0 },
  { "EnableEnhancedInstructionSet", "arch:SSE2", "Use sse2 instructions", "2",
    0 },
  { "EnableEnhancedInstructionSet", "arch:SSE", "Use sse instructions", "1",
    0 },
  { "FloatingPointModel", "fp:precise", "Use precise floating point model",
    "0", 0 },
  { "FloatingPointModel", "fp:strict", "Use strict floating point model", "1",
    0 },
  { "FloatingPointModel", "fp:fast", "Use fast floating point model", "2", 0 },
  { "FavorSizeOrSpeed", "Ot", "Favor fast code", "1", 0 },
  { "FavorSizeOrSpeed", "Os", "Favor small code", "2", 0 },
  { "CompileAs", "TC", "Compile as c code", "1", 0 },
  { "CompileAs", "TP", "Compile as c++ code", "2", 0 },
  { "Optimization", "Od", "Non Debug", "0", 0 },
  { "Optimization", "O1", "Min Size", "1", 0 },
  { "Optimization", "O2", "Max Speed", "2", 0 },
  { "Optimization", "Ox", "Max Optimization", "3", 0 },
  { "OptimizeForProcessor", "GB", "Blended processor mode", "0", 0 },
  { "OptimizeForProcessor", "G5", "Pentium", "1", 0 },
  { "OptimizeForProcessor", "G6", "PPro PII PIII", "2", 0 },
  { "OptimizeForProcessor", "G7", "Pentium 4 or Athlon", "3", 0 },
  { "InlineFunctionExpansion", "Ob0", "no inlines", "0", 0 },
  { "InlineFunctionExpansion", "Ob1", "when inline keyword", "1", 0 },
  { "InlineFunctionExpansion", "Ob2", "any time you can inline", "2", 0 },
  { "RuntimeLibrary", "MTd", "Multithreaded debug", "1", 0 },
  { "RuntimeLibrary", "MT", "Multithreaded", "0", 0 },
  { "RuntimeLibrary", "MDd", "Multithreaded dll debug", "3", 0 },
  { "RuntimeLibrary", "MD", "Multithreaded dll", "2", 0 },
  { "RuntimeLibrary", "MLd", "Single Thread debug", "5", 0 },
  { "RuntimeLibrary", "ML", "Single Thread", "4", 0 },
  { "StructMemberAlignment", "Zp16", "struct align 16 byte ", "5", 0 },
  { "StructMemberAlignment", "Zp1", "struct align 1 byte ", "1", 0 },
  { "StructMemberAlignment", "Zp2", "struct align 2 byte ", "2", 0 },
  { "StructMemberAlignment", "Zp4", "struct align 4 byte ", "3", 0 },
  { "StructMemberAlignment", "Zp8", "struct align 8 byte ", "4", 0 },
  { "WarningLevel", "W0", "Warning level", "0", 0 },
  { "WarningLevel", "W1", "Warning level", "1", 0 },
  { "WarningLevel", "W2", "Warning level", "2", 0 },
  { "WarningLevel", "W3", "Warning level", "3", 0 },
  { "WarningLevel", "W4", "Warning level", "4", 0 },
  { "DisableSpecificWarnings", "wd", "Disable specific warnings", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },

  // Precompiled header and related options.  Note that the
  // UsePrecompiledHeader entries are marked as "Continue" so that the
  // corresponding PrecompiledHeaderThrough entry can be found.
  { "UsePrecompiledHeader", "Yc", "Create Precompiled Header", "1",
    cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue },
  { "PrecompiledHeaderThrough", "Yc", "Precompiled Header Name", "",
    cmVS7FlagTable::UserValueRequired },
  { "PrecompiledHeaderFile", "Fp", "Generated Precompiled Header", "",
    cmVS7FlagTable::UserValue },
  // The YX and Yu options are in a per-global-generator table because
  // their values differ based on the VS IDE version.
  { "ForcedIncludeFiles", "FI", "Forced include files", "",
    cmVS7FlagTable::UserValueRequired | cmVS7FlagTable::SemicolonAppendable },

  { "AssemblerListingLocation", "Fa", "ASM List Location", "",
    cmVS7FlagTable::UserValue },
  { "ProgramDataBaseFileName", "Fd", "Program Database File Name", "",
    cmVS7FlagTable::UserValue },

  // boolean flags
  { "BufferSecurityCheck", "GS", "Buffer security check", "true", 0 },
  { "BufferSecurityCheck", "GS-", "Turn off Buffer security check", "false",
    0 },
  { "Detect64BitPortabilityProblems", "Wp64",
    "Detect 64-bit Portability Problems", "true", 0 },
  { "EnableFiberSafeOptimizations", "GT", "Enable Fiber-safe Optimizations",
    "true", 0 },
  { "EnableFunctionLevelLinking", "Gy", "EnableFunctionLevelLinking", "true",
    0 },
  { "EnableIntrinsicFunctions", "Oi", "EnableIntrinsicFunctions", "true", 0 },
  { "GlobalOptimizations", "Og", "Global Optimize", "true", 0 },
  { "ImproveFloatingPointConsistency", "Op", "ImproveFloatingPointConsistency",
    "true", 0 },
  { "MinimalRebuild", "Gm", "minimal rebuild", "true", 0 },
  { "OmitFramePointers", "Oy", "OmitFramePointers", "true", 0 },
  { "OptimizeForWindowsApplication", "GA", "Optimize for windows", "true", 0 },
  { "RuntimeTypeInfo", "GR", "Turn on Run time type information for c++",
    "true", 0 },
  { "RuntimeTypeInfo", "GR-", "Turn off Run time type information for c++",
    "false", 0 },
  { "SmallerTypeCheck", "RTCc", "smaller type check", "true", 0 },
  { "SuppressStartupBanner", "nologo", "SuppressStartupBanner", "true", 0 },
  { "WholeProgramOptimization", "GL", "Enables whole program optimization",
    "true", 0 },
  { "WholeProgramOptimization", "GL-", "Disables whole program optimization",
    "false", 0 },
  { "WarnAsError", "WX", "Treat warnings as errors", "true", 0 },
  { "BrowseInformation", "FR", "Generate browse information", "1", 0 },
  { "StringPooling", "GF", "Enable StringPooling", "true", 0 },
  { 0, 0, 0, 0, 0 }
};

cmVS7FlagTable cmLocalVisualStudio7GeneratorLinkFlagTable[] = {
  // option flags (some flags map to the same option)
  { "GenerateManifest", "MANIFEST:NO", "disable manifest generation", "false",
    0 },
  { "GenerateManifest", "MANIFEST", "enable manifest generation", "true", 0 },
  { "LinkIncremental", "INCREMENTAL:NO", "link incremental", "1", 0 },
  { "LinkIncremental", "INCREMENTAL:YES", "link incremental", "2", 0 },
  { "CLRUnmanagedCodeCheck", "CLRUNMANAGEDCODECHECK:NO", "", "false", 0 },
  { "CLRUnmanagedCodeCheck", "CLRUNMANAGEDCODECHECK", "", "true", 0 },
  { "DataExecutionPrevention", "NXCOMPAT:NO",
    "Not known to work with Windows Data Execution Prevention", "1", 0 },
  { "DataExecutionPrevention", "NXCOMPAT",
    "Known to work with Windows Data Execution Prevention", "2", 0 },
  { "DelaySign", "DELAYSIGN:NO", "", "false", 0 },
  { "DelaySign", "DELAYSIGN", "", "true", 0 },
  { "EntryPointSymbol", "ENTRY:", "sets the starting address", "",
    cmVS7FlagTable::UserValue },
  { "IgnoreDefaultLibraryNames", "NODEFAULTLIB:", "default libs to ignore", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "IgnoreAllDefaultLibraries", "NODEFAULTLIB", "ignore all default libs",
    "true", 0 },
  { "FixedBaseAddress", "FIXED:NO", "Generate a relocation section", "1", 0 },
  { "FixedBaseAddress", "FIXED", "Image must be loaded at a fixed address",
    "2", 0 },
  { "EnableCOMDATFolding", "OPT:NOICF", "Do not remove redundant COMDATs", "1",
    0 },
  { "EnableCOMDATFolding", "OPT:ICF", "Remove redundant COMDATs", "2", 0 },
  { "ResourceOnlyDLL", "NOENTRY", "Create DLL with no entry point", "true",
    0 },
  { "OptimizeReferences", "OPT:NOREF", "Keep unreferenced data", "1", 0 },
  { "OptimizeReferences", "OPT:REF", "Eliminate unreferenced data", "2", 0 },
  { "Profile", "PROFILE", "", "true", 0 },
  { "RandomizedBaseAddress", "DYNAMICBASE:NO",
    "Image may not be rebased at load-time", "1", 0 },
  { "RandomizedBaseAddress", "DYNAMICBASE",
    "Image may be rebased at load-time", "2", 0 },
  { "SetChecksum", "RELEASE", "Enable setting checksum in header", "true", 0 },
  { "SupportUnloadOfDelayLoadedDLL", "DELAY:UNLOAD", "", "true", 0 },
  { "TargetMachine", "MACHINE:I386", "Machine x86", "1", 0 },
  { "TargetMachine", "MACHINE:X86", "Machine x86", "1", 0 },
  { "TargetMachine", "MACHINE:AM33", "Machine AM33", "2", 0 },
  { "TargetMachine", "MACHINE:ARM", "Machine ARM", "3", 0 },
  { "TargetMachine", "MACHINE:EBC", "Machine EBC", "4", 0 },
  { "TargetMachine", "MACHINE:IA64", "Machine IA64", "5", 0 },
  { "TargetMachine", "MACHINE:M32R", "Machine M32R", "6", 0 },
  { "TargetMachine", "MACHINE:MIPS", "Machine MIPS", "7", 0 },
  { "TargetMachine", "MACHINE:MIPS16", "Machine MIPS16", "8", 0 },
  { "TargetMachine", "MACHINE:MIPSFPU)", "Machine MIPSFPU", "9", 0 },
  { "TargetMachine", "MACHINE:MIPSFPU16", "Machine MIPSFPU16", "10", 0 },
  { "TargetMachine", "MACHINE:MIPSR41XX", "Machine MIPSR41XX", "11", 0 },
  { "TargetMachine", "MACHINE:SH3", "Machine SH3", "12", 0 },
  { "TargetMachine", "MACHINE:SH3DSP", "Machine SH3DSP", "13", 0 },
  { "TargetMachine", "MACHINE:SH4", "Machine SH4", "14", 0 },
  { "TargetMachine", "MACHINE:SH5", "Machine SH5", "15", 0 },
  { "TargetMachine", "MACHINE:THUMB", "Machine THUMB", "16", 0 },
  { "TargetMachine", "MACHINE:X64", "Machine x64", "17", 0 },
  { "TurnOffAssemblyGeneration", "NOASSEMBLY",
    "No assembly even if CLR information is present in objects.", "true", 0 },
  { "ModuleDefinitionFile", "DEF:", "add an export def file", "",
    cmVS7FlagTable::UserValue },
  { "GenerateMapFile", "MAP", "enable generation of map file", "true", 0 },
  { 0, 0, 0, 0, 0 }
};

cmVS7FlagTable cmLocalVisualStudio7GeneratorFortranLinkFlagTable[] = {
  { "LinkIncremental", "INCREMENTAL:NO", "link incremental",
    "linkIncrementalNo", 0 },
  { "LinkIncremental", "INCREMENTAL:YES", "link incremental",
    "linkIncrementalYes", 0 },
  { 0, 0, 0, 0, 0 }
};

// Helper class to write build event <Tool .../> elements.
class cmLocalVisualStudio7Generator::EventWriter
{
public:
  EventWriter(cmLocalVisualStudio7Generator* lg, const std::string& config,
              std::ostream& os)
    : LG(lg)
    , Config(config)
    , Stream(os)
    , First(true)
  {
  }
  void Start(const char* tool)
  {
    this->First = true;
    this->Stream << "\t\t\t<Tool\n\t\t\t\tName=\"" << tool << "\"";
  }
  void Finish() { this->Stream << (this->First ? "" : "\"") << "/>\n"; }
  void Write(std::vector<cmCustomCommand> const& ccs)
  {
    for (std::vector<cmCustomCommand>::const_iterator ci = ccs.begin();
         ci != ccs.end(); ++ci) {
      this->Write(*ci);
    }
  }
  void Write(cmCustomCommand const& cc)
  {
    cmCustomCommandGenerator ccg(cc, this->Config, this->LG);
    if (this->First) {
      const char* comment = ccg.GetComment();
      if (comment && *comment) {
        this->Stream << "\nDescription=\"" << this->LG->EscapeForXML(comment)
                     << "\"";
      }
      this->Stream << "\nCommandLine=\"";
      this->First = false;
    } else {
      this->Stream << this->LG->EscapeForXML("\n");
    }
    std::string script = this->LG->ConstructScript(ccg);
    this->Stream << this->LG->EscapeForXML(script.c_str());
  }

private:
  cmLocalVisualStudio7Generator* LG;
  std::string Config;
  std::ostream& Stream;
  bool First;
};

void cmLocalVisualStudio7Generator::WriteConfiguration(
  std::ostream& fout, const std::string& configName,
  const std::string& libName, cmGeneratorTarget* target)
{
  const char* mfcFlag = this->Makefile->GetDefinition("CMAKE_MFC_FLAG");
  if (!mfcFlag) {
    mfcFlag = "0";
  }
  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);
  fout << "\t\t<Configuration\n"
       << "\t\t\tName=\"" << configName << "|" << gg->GetPlatformName()
       << "\"\n";
  // This is an internal type to Visual Studio, it seems that:
  // 4 == static library
  // 2 == dll
  // 1 == executable
  // 10 == utility
  const char* configType = "10";
  const char* projectType = 0;
  bool targetBuilds = true;

  switch (target->GetType()) {
    case cmStateEnums::OBJECT_LIBRARY:
      targetBuilds = false; // no manifest tool for object library
    case cmStateEnums::STATIC_LIBRARY:
      projectType = "typeStaticLibrary";
      configType = "4";
      break;
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
      projectType = "typeDynamicLibrary";
      configType = "2";
      break;
    case cmStateEnums::EXECUTABLE:
      configType = "1";
      break;
    case cmStateEnums::UTILITY:
    case cmStateEnums::GLOBAL_TARGET:
      configType = "10";
    default:
      targetBuilds = false;
      break;
  }
  if (this->FortranProject && projectType) {
    configType = projectType;
  }
  std::string flags;
  if (strcmp(configType, "10") != 0) {
    const std::string& linkLanguage =
      (this->FortranProject ? std::string("Fortran")
                            : target->GetLinkerLanguage(configName));
    if (linkLanguage.empty()) {
      cmSystemTools::Error(
        "CMake can not determine linker language for target: ",
        target->GetName().c_str());
      return;
    }
    if (linkLanguage == "C" || linkLanguage == "CXX" ||
        linkLanguage == "Fortran") {
      std::string baseFlagVar = "CMAKE_";
      baseFlagVar += linkLanguage;
      baseFlagVar += "_FLAGS";
      flags = this->Makefile->GetRequiredDefinition(baseFlagVar.c_str());
      std::string flagVar =
        baseFlagVar + std::string("_") + cmSystemTools::UpperCase(configName);
      flags += " ";
      flags += this->Makefile->GetRequiredDefinition(flagVar.c_str());
    }
    // set the correct language
    if (linkLanguage == "C") {
      flags += " /TC ";
    }
    if (linkLanguage == "CXX") {
      flags += " /TP ";
    }

    // Add the target-specific flags.
    this->AddCompileOptions(flags, target, linkLanguage, configName);

    // Check IPO related warning/error.
    target->IsIPOEnabled(linkLanguage, configName);
  }

  if (this->FortranProject) {
    switch (cmOutputConverter::GetFortranFormat(
      target->GetProperty("Fortran_FORMAT"))) {
      case cmOutputConverter::FortranFormatFixed:
        flags += " -fixed";
        break;
      case cmOutputConverter::FortranFormatFree:
        flags += " -free";
        break;
      default:
        break;
    }
  }

  // Get preprocessor definitions for this directory.
  std::string defineFlags = this->Makefile->GetDefineFlags();
  Options::Tool t = Options::Compiler;
  cmVS7FlagTable const* table = cmLocalVisualStudio7GeneratorFlagTable;
  if (this->FortranProject) {
    t = Options::FortranCompiler;
    table = cmLocalVisualStudio7GeneratorFortranFlagTable;
  }
  Options targetOptions(this, t, table, gg->ExtraFlagTable);
  targetOptions.FixExceptionHandlingDefault();
  std::string asmLocation = configName + "/";
  targetOptions.AddFlag("AssemblerListingLocation", asmLocation.c_str());
  targetOptions.Parse(flags.c_str());
  targetOptions.Parse(defineFlags.c_str());
  targetOptions.ParseFinish();
  std::vector<std::string> targetDefines;
  target->GetCompileDefinitions(targetDefines, configName, "CXX");
  targetOptions.AddDefines(targetDefines);
  targetOptions.SetVerboseMakefile(
    this->Makefile->IsOn("CMAKE_VERBOSE_MAKEFILE"));

  // Add a definition for the configuration name.
  std::string configDefine = "CMAKE_INTDIR=\"";
  configDefine += configName;
  configDefine += "\"";
  targetOptions.AddDefine(configDefine);

  // Add the export symbol definition for shared library objects.
  if (const char* exportMacro = target->GetExportMacro()) {
    targetOptions.AddDefine(exportMacro);
  }

  // The intermediate directory name consists of a directory for the
  // target and a subdirectory for the configuration name.
  std::string intermediateDir = this->GetTargetDirectory(target);
  intermediateDir += "/";
  intermediateDir += configName;

  if (target->GetType() < cmStateEnums::UTILITY) {
    std::string const& outDir =
      target->GetType() == cmStateEnums::OBJECT_LIBRARY
      ? intermediateDir
      : target->GetDirectory(configName);
    /* clang-format off */
    fout << "\t\t\tOutputDirectory=\""
         << this->ConvertToXMLOutputPathSingle(outDir.c_str()) << "\"\n";
    /* clang-format on */
  }

  /* clang-format off */
  fout << "\t\t\tIntermediateDirectory=\""
       << this->ConvertToXMLOutputPath(intermediateDir.c_str())
       << "\"\n"
       << "\t\t\tConfigurationType=\"" << configType << "\"\n"
       << "\t\t\tUseOfMFC=\"" << mfcFlag << "\"\n"
       << "\t\t\tATLMinimizesCRunTimeLibraryUsage=\"false\"\n";
  /* clang-format on */

  if (this->FortranProject) {
    // Intel Fortran >= 15.0 uses TargetName property.
    std::string const targetNameFull = target->GetFullName(configName);
    std::string const targetName =
      cmSystemTools::GetFilenameWithoutLastExtension(targetNameFull);
    std::string const targetExt =
      target->GetType() == cmStateEnums::OBJECT_LIBRARY
      ? ".lib"
      : cmSystemTools::GetFilenameLastExtension(targetNameFull);
    /* clang-format off */
    fout <<
      "\t\t\tTargetName=\"" << this->EscapeForXML(targetName) << "\"\n"
      "\t\t\tTargetExt=\"" << this->EscapeForXML(targetExt) << "\"\n"
      ;
    /* clang-format on */
  }

  // If unicode is enabled change the character set to unicode, if not
  // then default to MBCS.
  if (targetOptions.UsingUnicode()) {
    fout << "\t\t\tCharacterSet=\"1\">\n";
  } else if (targetOptions.UsingSBCS()) {
    fout << "\t\t\tCharacterSet=\"0\">\n";
  } else {
    fout << "\t\t\tCharacterSet=\"2\">\n";
  }
  const char* tool = "VCCLCompilerTool";
  if (this->FortranProject) {
    tool = "VFFortranCompilerTool";
  }
  fout << "\t\t\t<Tool\n"
       << "\t\t\t\tName=\"" << tool << "\"\n";
  if (this->FortranProject) {
    const char* target_mod_dir =
      target->GetProperty("Fortran_MODULE_DIRECTORY");
    std::string modDir;
    if (target_mod_dir) {
      modDir = this->ConvertToRelativePath(this->GetCurrentBinaryDirectory(),
                                           target_mod_dir);
    } else {
      modDir = ".";
    }
    fout << "\t\t\t\tModulePath=\""
         << this->ConvertToXMLOutputPath(modDir.c_str())
         << "\\$(ConfigurationName)\"\n";
  }
  fout << "\t\t\t\tAdditionalIncludeDirectories=\"";
  std::vector<std::string> includes;
  this->GetIncludeDirectories(includes, target, "C", configName);
  std::vector<std::string>::iterator i = includes.begin();
  for (; i != includes.end(); ++i) {
    // output the include path
    std::string ipath = this->ConvertToXMLOutputPath(i->c_str());
    fout << ipath << ";";
    // if this is fortran then output the include with
    // a ConfigurationName on the end of it.
    if (this->FortranProject) {
      ipath = i->c_str();
      ipath += "/$(ConfigurationName)";
      ipath = this->ConvertToXMLOutputPath(ipath.c_str());
      fout << ipath << ";";
    }
  }
  fout << "\"\n";
  targetOptions.OutputFlagMap(fout, "\t\t\t\t");
  targetOptions.OutputPreprocessorDefinitions(fout, "\t\t\t\t", "\n", "CXX");
  fout << "\t\t\t\tObjectFile=\"$(IntDir)\\\"\n";
  if (target->GetType() <= cmStateEnums::OBJECT_LIBRARY) {
    // Specify the compiler program database file if configured.
    std::string pdb = target->GetCompilePDBPath(configName);
    if (!pdb.empty()) {
      fout << "\t\t\t\tProgramDataBaseFileName=\""
           << this->ConvertToXMLOutputPathSingle(pdb.c_str()) << "\"\n";
    }
  }
  fout << "/>\n"; // end of <Tool Name=VCCLCompilerTool
  if (gg->IsMasmEnabled() && !this->FortranProject) {
    Options masmOptions(this, Options::MasmCompiler, 0, 0);
    /* clang-format off */
    fout <<
      "\t\t\t<Tool\n"
      "\t\t\t\tName=\"MASM\"\n"
      "\t\t\t\tIncludePaths=\""
      ;
    /* clang-format on */
    const char* sep = "";
    for (i = includes.begin(); i != includes.end(); ++i) {
      std::string inc = *i;
      cmConvertToWindowsSlash(inc);
      fout << sep << this->EscapeForXML(inc);
      sep = ";";
    }
    fout << "\"\n";
    // Use same preprocessor definitions as VCCLCompilerTool.
    targetOptions.OutputPreprocessorDefinitions(fout, "\t\t\t\t", "\n",
                                                "ASM_MASM");
    masmOptions.OutputFlagMap(fout, "\t\t\t\t");
    /* clang-format off */
    fout <<
      "\t\t\t\tObjectFile=\"$(IntDir)\\\"\n"
      "\t\t\t/>\n";
    /* clang-format on */
  }
  tool = "VCCustomBuildTool";
  if (this->FortranProject) {
    tool = "VFCustomBuildTool";
  }
  fout << "\t\t\t<Tool\n\t\t\t\tName=\"" << tool << "\"/>\n";
  tool = "VCResourceCompilerTool";
  if (this->FortranProject) {
    tool = "VFResourceCompilerTool";
  }
  fout << "\t\t\t<Tool\n\t\t\t\tName=\"" << tool << "\"\n"
       << "\t\t\t\tAdditionalIncludeDirectories=\"";
  for (i = includes.begin(); i != includes.end(); ++i) {
    std::string ipath = this->ConvertToXMLOutputPath(i->c_str());
    fout << ipath << ";";
  }
  // add the -D flags to the RC tool
  fout << "\"";
  targetOptions.OutputPreprocessorDefinitions(fout, "\n\t\t\t\t", "", "RC");
  fout << "/>\n";
  tool = "VCMIDLTool";
  if (this->FortranProject) {
    tool = "VFMIDLTool";
  }
  fout << "\t\t\t<Tool\n\t\t\t\tName=\"" << tool << "\"\n";
  fout << "\t\t\t\tAdditionalIncludeDirectories=\"";
  for (i = includes.begin(); i != includes.end(); ++i) {
    std::string ipath = this->ConvertToXMLOutputPath(i->c_str());
    fout << ipath << ";";
  }
  fout << "\"\n";
  fout << "\t\t\t\tMkTypLibCompatible=\"false\"\n";
  if (gg->GetPlatformName() == "x64") {
    fout << "\t\t\t\tTargetEnvironment=\"3\"\n";
  } else if (gg->GetPlatformName() == "ia64") {
    fout << "\t\t\t\tTargetEnvironment=\"2\"\n";
  } else {
    fout << "\t\t\t\tTargetEnvironment=\"1\"\n";
  }
  fout << "\t\t\t\tGenerateStublessProxies=\"true\"\n";
  fout << "\t\t\t\tTypeLibraryName=\"$(InputName).tlb\"\n";
  fout << "\t\t\t\tOutputDirectory=\"$(IntDir)\"\n";
  fout << "\t\t\t\tHeaderFileName=\"$(InputName).h\"\n";
  fout << "\t\t\t\tDLLDataFileName=\"\"\n";
  fout << "\t\t\t\tInterfaceIdentifierFileName=\"$(InputName)_i.c\"\n";
  fout << "\t\t\t\tProxyFileName=\"$(InputName)_p.c\"/>\n";
  // end of <Tool Name=VCMIDLTool

  // Add manifest tool settings.
  if (targetBuilds) {
    const char* manifestTool = "VCManifestTool";
    if (this->FortranProject) {
      manifestTool = "VFManifestTool";
    }
    /* clang-format off */
    fout <<
      "\t\t\t<Tool\n"
      "\t\t\t\tName=\"" << manifestTool << "\"";
    /* clang-format on */

    std::vector<cmSourceFile const*> manifest_srcs;
    target->GetManifests(manifest_srcs, configName);
    if (!manifest_srcs.empty()) {
      fout << "\n\t\t\t\tAdditionalManifestFiles=\"";
      for (std::vector<cmSourceFile const*>::const_iterator mi =
             manifest_srcs.begin();
           mi != manifest_srcs.end(); ++mi) {
        std::string m = (*mi)->GetFullPath();
        fout << this->ConvertToXMLOutputPath(m.c_str()) << ";";
      }
      fout << "\"";
    }

    // Check if we need the FAT32 workaround.
    // Check the filesystem type where the target will be written.
    if (cmLVS7G_IsFAT(target->GetDirectory(configName).c_str())) {
      // Add a flag telling the manifest tool to use a workaround
      // for FAT32 file systems, which can cause an empty manifest
      // to be embedded into the resulting executable.  See CMake
      // bug #2617.
      fout << "\n\t\t\t\tUseFAT32Workaround=\"true\"";
    }
    fout << "/>\n";
  }

  this->OutputTargetRules(fout, configName, target, libName);
  this->OutputBuildTool(fout, configName, target, targetOptions);
  this->OutputDeploymentDebuggerTool(fout, configName, target);
  fout << "\t\t</Configuration>\n";
}

std::string cmLocalVisualStudio7Generator::GetBuildTypeLinkerFlags(
  std::string rootLinkerFlags, const std::string& configName)
{
  std::string configTypeUpper = cmSystemTools::UpperCase(configName);
  std::string extraLinkOptionsBuildTypeDef =
    rootLinkerFlags + "_" + configTypeUpper;

  std::string extraLinkOptionsBuildType =
    this->Makefile->GetRequiredDefinition(
      extraLinkOptionsBuildTypeDef.c_str());

  return extraLinkOptionsBuildType;
}

void cmLocalVisualStudio7Generator::OutputBuildTool(
  std::ostream& fout, const std::string& configName, cmGeneratorTarget* target,
  const Options& targetOptions)
{
  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);
  std::string temp;
  std::string extraLinkOptions;
  if (target->GetType() == cmStateEnums::EXECUTABLE) {
    extraLinkOptions =
      this->Makefile->GetRequiredDefinition("CMAKE_EXE_LINKER_FLAGS") +
      std::string(" ") +
      GetBuildTypeLinkerFlags("CMAKE_EXE_LINKER_FLAGS", configName);
  }
  if (target->GetType() == cmStateEnums::SHARED_LIBRARY) {
    extraLinkOptions =
      this->Makefile->GetRequiredDefinition("CMAKE_SHARED_LINKER_FLAGS") +
      std::string(" ") +
      GetBuildTypeLinkerFlags("CMAKE_SHARED_LINKER_FLAGS", configName);
  }
  if (target->GetType() == cmStateEnums::MODULE_LIBRARY) {
    extraLinkOptions =
      this->Makefile->GetRequiredDefinition("CMAKE_MODULE_LINKER_FLAGS") +
      std::string(" ") +
      GetBuildTypeLinkerFlags("CMAKE_MODULE_LINKER_FLAGS", configName);
  }

  const char* targetLinkFlags = target->GetProperty("LINK_FLAGS");
  if (targetLinkFlags) {
    extraLinkOptions += " ";
    extraLinkOptions += targetLinkFlags;
  }
  std::string configTypeUpper = cmSystemTools::UpperCase(configName);
  std::string linkFlagsConfig = "LINK_FLAGS_";
  linkFlagsConfig += configTypeUpper;
  targetLinkFlags = target->GetProperty(linkFlagsConfig.c_str());
  if (targetLinkFlags) {
    extraLinkOptions += " ";
    extraLinkOptions += targetLinkFlags;
  }
  Options linkOptions(this, Options::Linker);
  if (this->FortranProject) {
    linkOptions.AddTable(cmLocalVisualStudio7GeneratorFortranLinkFlagTable);
  }
  linkOptions.AddTable(cmLocalVisualStudio7GeneratorLinkFlagTable);

  linkOptions.Parse(extraLinkOptions.c_str());
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    target->GetModuleDefinitionInfo(configName);
  if (mdi && !mdi->DefFile.empty()) {
    std::string defFile =
      this->ConvertToOutputFormat(mdi->DefFile, cmOutputConverter::SHELL);
    linkOptions.AddFlag("ModuleDefinitionFile", defFile.c_str());
  }

  switch (target->GetType()) {
    case cmStateEnums::UNKNOWN_LIBRARY:
      break;
    case cmStateEnums::OBJECT_LIBRARY: {
      std::string libpath = this->GetTargetDirectory(target);
      libpath += "/";
      libpath += configName;
      libpath += "/";
      libpath += target->GetName();
      libpath += ".lib";
      const char* tool =
        this->FortranProject ? "VFLibrarianTool" : "VCLibrarianTool";
      fout << "\t\t\t<Tool\n"
           << "\t\t\t\tName=\"" << tool << "\"\n";
      fout << "\t\t\t\tOutputFile=\""
           << this->ConvertToXMLOutputPathSingle(libpath.c_str()) << "\"/>\n";
      break;
    }
    case cmStateEnums::STATIC_LIBRARY: {
      std::string targetNameFull = target->GetFullName(configName);
      std::string libpath = target->GetDirectory(configName);
      libpath += "/";
      libpath += targetNameFull;
      const char* tool = "VCLibrarianTool";
      if (this->FortranProject) {
        tool = "VFLibrarianTool";
      }
      fout << "\t\t\t<Tool\n"
           << "\t\t\t\tName=\"" << tool << "\"\n";

      if (this->FortranProject) {
        std::ostringstream libdeps;
        this->Internal->OutputObjects(libdeps, target, configName);
        if (!libdeps.str().empty()) {
          fout << "\t\t\t\tAdditionalDependencies=\"" << libdeps.str()
               << "\"\n";
        }
      }
      std::string libflags;
      this->GetStaticLibraryFlags(libflags, configTypeUpper, target);
      if (!libflags.empty()) {
        fout << "\t\t\t\tAdditionalOptions=\"" << libflags << "\"\n";
      }
      fout << "\t\t\t\tOutputFile=\""
           << this->ConvertToXMLOutputPathSingle(libpath.c_str()) << "\"/>\n";
      break;
    }
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY: {
      std::string targetName;
      std::string targetNameSO;
      std::string targetNameFull;
      std::string targetNameImport;
      std::string targetNamePDB;
      target->GetLibraryNames(targetName, targetNameSO, targetNameFull,
                              targetNameImport, targetNamePDB, configName);

      // Compute the link library and directory information.
      cmComputeLinkInformation* pcli = target->GetLinkInformation(configName);
      if (!pcli) {
        return;
      }
      cmComputeLinkInformation& cli = *pcli;
      std::string linkLanguage = cli.GetLinkLanguage();

      // Compute the variable name to lookup standard libraries for this
      // language.
      std::string standardLibsVar = "CMAKE_";
      standardLibsVar += linkLanguage;
      standardLibsVar += "_STANDARD_LIBRARIES";
      const char* tool = "VCLinkerTool";
      if (this->FortranProject) {
        tool = "VFLinkerTool";
      }
      fout << "\t\t\t<Tool\n"
           << "\t\t\t\tName=\"" << tool << "\"\n";
      if (!gg->NeedLinkLibraryDependencies(target)) {
        fout << "\t\t\t\tLinkLibraryDependencies=\"false\"\n";
      }
      // Use the NOINHERIT macro to avoid getting VS project default
      // libraries which may be set by the user to something bad.
      fout << "\t\t\t\tAdditionalDependencies=\"$(NOINHERIT) "
           << this->Makefile->GetSafeDefinition(standardLibsVar.c_str());
      if (this->FortranProject) {
        this->Internal->OutputObjects(fout, target, configName, " ");
      }
      fout << " ";
      this->Internal->OutputLibraries(fout, cli.GetItems());
      fout << "\"\n";
      temp = target->GetDirectory(configName);
      temp += "/";
      temp += targetNameFull;
      fout << "\t\t\t\tOutputFile=\""
           << this->ConvertToXMLOutputPathSingle(temp.c_str()) << "\"\n";
      this->WriteTargetVersionAttribute(fout, target);
      linkOptions.OutputFlagMap(fout, "\t\t\t\t");
      fout << "\t\t\t\tAdditionalLibraryDirectories=\"";
      this->OutputLibraryDirectories(fout, cli.GetDirectories());
      fout << "\"\n";
      temp = target->GetPDBDirectory(configName);
      temp += "/";
      temp += targetNamePDB;
      fout << "\t\t\t\tProgramDatabaseFile=\""
           << this->ConvertToXMLOutputPathSingle(temp.c_str()) << "\"\n";
      if (targetOptions.IsDebug()) {
        fout << "\t\t\t\tGenerateDebugInformation=\"true\"\n";
      }
      if (this->WindowsCEProject) {
        if (this->GetVersion() < cmGlobalVisualStudioGenerator::VS9) {
          fout << "\t\t\t\tSubSystem=\"9\"\n";
        } else {
          fout << "\t\t\t\tSubSystem=\"8\"\n";
        }
      }
      std::string stackVar = "CMAKE_";
      stackVar += linkLanguage;
      stackVar += "_STACK_SIZE";
      const char* stackVal = this->Makefile->GetDefinition(stackVar.c_str());
      if (stackVal) {
        fout << "\t\t\t\tStackReserveSize=\"" << stackVal << "\"\n";
      }
      temp =
        target->GetDirectory(configName, cmStateEnums::ImportLibraryArtifact);
      temp += "/";
      temp += targetNameImport;
      fout << "\t\t\t\tImportLibrary=\""
           << this->ConvertToXMLOutputPathSingle(temp.c_str()) << "\"";
      if (this->FortranProject) {
        fout << "\n\t\t\t\tLinkDLL=\"true\"";
      }
      fout << "/>\n";
    } break;
    case cmStateEnums::EXECUTABLE: {
      std::string targetName;
      std::string targetNameFull;
      std::string targetNameImport;
      std::string targetNamePDB;
      target->GetExecutableNames(targetName, targetNameFull, targetNameImport,
                                 targetNamePDB, configName);

      // Compute the link library and directory information.
      cmComputeLinkInformation* pcli = target->GetLinkInformation(configName);
      if (!pcli) {
        return;
      }
      cmComputeLinkInformation& cli = *pcli;
      std::string linkLanguage = cli.GetLinkLanguage();

      bool isWin32Executable = target->GetPropertyAsBool("WIN32_EXECUTABLE");

      // Compute the variable name to lookup standard libraries for this
      // language.
      std::string standardLibsVar = "CMAKE_";
      standardLibsVar += linkLanguage;
      standardLibsVar += "_STANDARD_LIBRARIES";
      const char* tool = "VCLinkerTool";
      if (this->FortranProject) {
        tool = "VFLinkerTool";
      }
      fout << "\t\t\t<Tool\n"
           << "\t\t\t\tName=\"" << tool << "\"\n";
      if (!gg->NeedLinkLibraryDependencies(target)) {
        fout << "\t\t\t\tLinkLibraryDependencies=\"false\"\n";
      }
      // Use the NOINHERIT macro to avoid getting VS project default
      // libraries which may be set by the user to something bad.
      fout << "\t\t\t\tAdditionalDependencies=\"$(NOINHERIT) "
           << this->Makefile->GetSafeDefinition(standardLibsVar.c_str());
      if (this->FortranProject) {
        this->Internal->OutputObjects(fout, target, configName, " ");
      }
      fout << " ";
      this->Internal->OutputLibraries(fout, cli.GetItems());
      fout << "\"\n";
      temp = target->GetDirectory(configName);
      temp += "/";
      temp += targetNameFull;
      fout << "\t\t\t\tOutputFile=\""
           << this->ConvertToXMLOutputPathSingle(temp.c_str()) << "\"\n";
      this->WriteTargetVersionAttribute(fout, target);
      linkOptions.OutputFlagMap(fout, "\t\t\t\t");
      fout << "\t\t\t\tAdditionalLibraryDirectories=\"";
      this->OutputLibraryDirectories(fout, cli.GetDirectories());
      fout << "\"\n";
      std::string path = this->ConvertToXMLOutputPathSingle(
        target->GetPDBDirectory(configName).c_str());
      fout << "\t\t\t\tProgramDatabaseFile=\"" << path << "/" << targetNamePDB
           << "\"\n";
      if (targetOptions.IsDebug()) {
        fout << "\t\t\t\tGenerateDebugInformation=\"true\"\n";
      }
      if (this->WindowsCEProject) {
        if (this->GetVersion() < cmGlobalVisualStudioGenerator::VS9) {
          fout << "\t\t\t\tSubSystem=\"9\"\n";
        } else {
          fout << "\t\t\t\tSubSystem=\"8\"\n";
        }

        if (!linkOptions.GetFlag("EntryPointSymbol")) {
          const char* entryPointSymbol = targetOptions.UsingUnicode()
            ? (isWin32Executable ? "wWinMainCRTStartup" : "mainWCRTStartup")
            : (isWin32Executable ? "WinMainCRTStartup" : "mainACRTStartup");
          fout << "\t\t\t\tEntryPointSymbol=\"" << entryPointSymbol << "\"\n";
        }
      } else if (this->FortranProject) {
        fout << "\t\t\t\tSubSystem=\""
             << (isWin32Executable ? "subSystemWindows" : "subSystemConsole")
             << "\"\n";
      } else {
        fout << "\t\t\t\tSubSystem=\"" << (isWin32Executable ? "2" : "1")
             << "\"\n";
      }
      std::string stackVar = "CMAKE_";
      stackVar += linkLanguage;
      stackVar += "_STACK_SIZE";
      const char* stackVal = this->Makefile->GetDefinition(stackVar.c_str());
      if (stackVal) {
        fout << "\t\t\t\tStackReserveSize=\"" << stackVal << "\"";
      }
      temp =
        target->GetDirectory(configName, cmStateEnums::ImportLibraryArtifact);
      temp += "/";
      temp += targetNameImport;
      fout << "\t\t\t\tImportLibrary=\""
           << this->ConvertToXMLOutputPathSingle(temp.c_str()) << "\"/>\n";
      break;
    }
    case cmStateEnums::UTILITY:
    case cmStateEnums::GLOBAL_TARGET:
    case cmStateEnums::INTERFACE_LIBRARY:
      break;
  }
}

void cmLocalVisualStudio7Generator::OutputDeploymentDebuggerTool(
  std::ostream& fout, std::string const& config, cmGeneratorTarget* target)
{
  if (this->WindowsCEProject) {
    if (const char* dir = target->GetProperty("DEPLOYMENT_REMOTE_DIRECTORY")) {
      /* clang-format off */
      fout <<
        "\t\t\t<DeploymentTool\n"
        "\t\t\t\tForceDirty=\"-1\"\n"
        "\t\t\t\tRemoteDirectory=\"" << this->EscapeForXML(dir) << "\"\n"
        "\t\t\t\tRegisterOutput=\"0\"\n"
        "\t\t\t\tAdditionalFiles=\"\"/>\n"
        ;
      /* clang-format on */
      std::string const exe =
        dir + std::string("\\") + target->GetFullName(config);
      /* clang-format off */
      fout <<
        "\t\t\t<DebuggerTool\n"
        "\t\t\t\tRemoteExecutable=\"" << this->EscapeForXML(exe) << "\"\n"
        "\t\t\t\tArguments=\"\"\n"
        "\t\t\t/>\n"
        ;
      /* clang-format on */
    }
  }
}

void cmLocalVisualStudio7Generator::WriteTargetVersionAttribute(
  std::ostream& fout, cmGeneratorTarget* gt)
{
  int major;
  int minor;
  gt->GetTargetVersion(major, minor);
  fout << "\t\t\t\tVersion=\"" << major << "." << minor << "\"\n";
}

void cmLocalVisualStudio7GeneratorInternals::OutputLibraries(
  std::ostream& fout, ItemVector const& libs)
{
  cmLocalVisualStudio7Generator* lg = this->LocalGenerator;
  std::string currentBinDir = lg->GetCurrentBinaryDirectory();
  for (ItemVector::const_iterator l = libs.begin(); l != libs.end(); ++l) {
    if (l->IsPath) {
      std::string rel =
        lg->ConvertToRelativePath(currentBinDir, l->Value.c_str());
      fout << lg->ConvertToXMLOutputPath(rel.c_str()) << " ";
    } else if (!l->Target ||
               l->Target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
      fout << l->Value << " ";
    }
  }
}

void cmLocalVisualStudio7GeneratorInternals::OutputObjects(
  std::ostream& fout, cmGeneratorTarget* gt, std::string const& configName,
  const char* isep)
{
  // VS < 8 does not support per-config source locations so we
  // list object library content on the link line instead.
  cmLocalVisualStudio7Generator* lg = this->LocalGenerator;
  std::string currentBinDir = lg->GetCurrentBinaryDirectory();

  std::vector<cmSourceFile const*> objs;
  gt->GetExternalObjects(objs, configName);

  const char* sep = isep ? isep : "";
  for (std::vector<cmSourceFile const*>::const_iterator i = objs.begin();
       i != objs.end(); ++i) {
    if (!(*i)->GetObjectLibrary().empty()) {
      std::string const& objFile = (*i)->GetFullPath();
      std::string rel = lg->ConvertToRelativePath(currentBinDir, objFile);
      fout << sep << lg->ConvertToXMLOutputPath(rel.c_str());
      sep = " ";
    }
  }
}

void cmLocalVisualStudio7Generator::OutputLibraryDirectories(
  std::ostream& fout, std::vector<std::string> const& dirs)
{
  const char* comma = "";
  std::string currentBinDir = this->GetCurrentBinaryDirectory();
  for (std::vector<std::string>::const_iterator d = dirs.begin();
       d != dirs.end(); ++d) {
    // Remove any trailing slash and skip empty paths.
    std::string dir = *d;
    if (dir[dir.size() - 1] == '/') {
      dir = dir.substr(0, dir.size() - 1);
    }
    if (dir.empty()) {
      continue;
    }

    // Switch to a relative path specification if it is shorter.
    if (cmSystemTools::FileIsFullPath(dir.c_str())) {
      std::string rel =
        this->ConvertToRelativePath(currentBinDir, dir.c_str());
      if (rel.size() < dir.size()) {
        dir = rel;
      }
    }

    // First search a configuration-specific subdirectory and then the
    // original directory.
    fout << comma
         << this->ConvertToXMLOutputPath(
              (dir + "/$(ConfigurationName)").c_str())
         << "," << this->ConvertToXMLOutputPath(dir.c_str());
    comma = ",";
  }
}

void cmLocalVisualStudio7Generator::WriteVCProjFile(std::ostream& fout,
                                                    const std::string& libName,
                                                    cmGeneratorTarget* target)
{
  std::vector<std::string> configs;
  this->Makefile->GetConfigurations(configs);

  // We may be modifying the source groups temporarily, so make a copy.
  std::vector<cmSourceGroup> sourceGroups = this->Makefile->GetSourceGroups();

  std::vector<cmGeneratorTarget::AllConfigSource> const& sources =
    target->GetAllConfigSources();
  std::map<cmSourceFile const*, size_t> sourcesIndex;

  for (size_t si = 0; si < sources.size(); ++si) {
    cmSourceFile const* sf = sources[si].Source;
    sourcesIndex[sf] = si;
    if (!sf->GetObjectLibrary().empty()) {
      if (this->FortranProject) {
        // Intel Fortran does not support per-config source locations
        // so we list object library content on the link line instead.
        // See OutputObjects.
        continue;
      }
    }
    // Add the file to the list of sources.
    std::string const source = sf->GetFullPath();
    cmSourceGroup* sourceGroup =
      this->Makefile->FindSourceGroup(source.c_str(), sourceGroups);
    sourceGroup->AssignSource(sf);
  }

  // open the project
  this->WriteProjectStart(fout, libName, target, sourceGroups);
  // write the configuration information
  this->WriteConfigurations(fout, configs, libName, target);

  fout << "\t<Files>\n";

  // Loop through every source group.
  for (unsigned int i = 0; i < sourceGroups.size(); ++i) {
    cmSourceGroup sg = sourceGroups[i];
    this->WriteGroup(&sg, target, fout, libName, configs, sourcesIndex);
  }

  fout << "\t</Files>\n";

  // Write the VCProj file's footer.
  this->WriteVCProjFooter(fout, target);
}

struct cmLVS7GFileConfig
{
  std::string ObjectName;
  std::string CompileFlags;
  std::string CompileDefs;
  std::string CompileDefsConfig;
  std::string AdditionalDeps;
  bool ExcludedFromBuild;
};

class cmLocalVisualStudio7GeneratorFCInfo
{
public:
  cmLocalVisualStudio7GeneratorFCInfo(
    cmLocalVisualStudio7Generator* lg, cmGeneratorTarget* target,
    cmGeneratorTarget::AllConfigSource const& acs,
    std::vector<std::string> const& configs);
  std::map<std::string, cmLVS7GFileConfig> FileConfigMap;
};

cmLocalVisualStudio7GeneratorFCInfo::cmLocalVisualStudio7GeneratorFCInfo(
  cmLocalVisualStudio7Generator* lg, cmGeneratorTarget* gt,
  cmGeneratorTarget::AllConfigSource const& acs,
  std::vector<std::string> const& configs)
{
  cmSourceFile const& sf = *acs.Source;
  std::string objectName;
  if (gt->HasExplicitObjectName(&sf)) {
    objectName = gt->GetObjectName(&sf);
  }

  // Compute per-source, per-config information.
  size_t ci = 0;
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i, ++ci) {
    std::string configUpper = cmSystemTools::UpperCase(*i);
    cmLVS7GFileConfig fc;
    bool needfc = false;
    if (!objectName.empty()) {
      fc.ObjectName = objectName;
      needfc = true;
    }
    if (const char* cflags = sf.GetProperty("COMPILE_FLAGS")) {
      cmGeneratorExpression ge;
      CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(cflags);
      fc.CompileFlags = cge->Evaluate(lg, *i);
      needfc = true;
    }
    if (lg->FortranProject) {
      switch (cmOutputConverter::GetFortranFormat(
        sf.GetProperty("Fortran_FORMAT"))) {
        case cmOutputConverter::FortranFormatFixed:
          fc.CompileFlags = "-fixed " + fc.CompileFlags;
          needfc = true;
          break;
        case cmOutputConverter::FortranFormatFree:
          fc.CompileFlags = "-free " + fc.CompileFlags;
          needfc = true;
          break;
        default:
          break;
      }
    }
    if (const char* cdefs = sf.GetProperty("COMPILE_DEFINITIONS")) {
      fc.CompileDefs = cdefs;
      needfc = true;
    }
    std::string defPropName = "COMPILE_DEFINITIONS_";
    defPropName += configUpper;
    if (const char* ccdefs = sf.GetProperty(defPropName.c_str())) {
      fc.CompileDefsConfig = ccdefs;
      needfc = true;
    }

    // Check for extra object-file dependencies.
    if (const char* deps = sf.GetProperty("OBJECT_DEPENDS")) {
      std::vector<std::string> depends;
      cmSystemTools::ExpandListArgument(deps, depends);
      const char* sep = "";
      for (std::vector<std::string>::iterator j = depends.begin();
           j != depends.end(); ++j) {
        fc.AdditionalDeps += sep;
        fc.AdditionalDeps += lg->ConvertToXMLOutputPath(j->c_str());
        sep = ";";
        needfc = true;
      }
    }

    std::string lang =
      lg->GlobalGenerator->GetLanguageFromExtension(sf.GetExtension().c_str());
    const std::string& sourceLang = lg->GetSourceFileLanguage(sf);
    const std::string& linkLanguage = gt->GetLinkerLanguage(i->c_str());
    bool needForceLang = false;
    // source file does not match its extension language
    if (lang != sourceLang) {
      needForceLang = true;
      lang = sourceLang;
    }
    // If HEADER_FILE_ONLY is set, we must suppress this generation in
    // the project file
    fc.ExcludedFromBuild = sf.GetPropertyAsBool("HEADER_FILE_ONLY") ||
      std::find(acs.Configs.begin(), acs.Configs.end(), ci) ==
        acs.Configs.end();
    if (fc.ExcludedFromBuild) {
      needfc = true;
    }

    // if the source file does not match the linker language
    // then force c or c++
    if (needForceLang || (linkLanguage != lang)) {
      if (lang == "CXX") {
        // force a C++ file type
        fc.CompileFlags += " /TP ";
        needfc = true;
      } else if (lang == "C") {
        // force to c
        fc.CompileFlags += " /TC ";
        needfc = true;
      }
    }

    if (needfc) {
      this->FileConfigMap[*i] = fc;
    }
  }
}

std::string cmLocalVisualStudio7Generator::ComputeLongestObjectDirectory(
  cmGeneratorTarget const* target) const
{
  std::vector<std::string> configs;
  target->Target->GetMakefile()->GetConfigurations(configs);

  // Compute the maximum length configuration name.
  std::string config_max;
  for (std::vector<std::string>::iterator i = configs.begin();
       i != configs.end(); ++i) {
    if (i->size() > config_max.size()) {
      config_max = *i;
    }
  }

  // Compute the maximum length full path to the intermediate
  // files directory for any configuration.  This is used to construct
  // object file names that do not produce paths that are too long.
  std::string dir_max;
  dir_max += this->GetCurrentBinaryDirectory();
  dir_max += "/";
  dir_max += this->GetTargetDirectory(target);
  dir_max += "/";
  dir_max += config_max;
  dir_max += "/";
  return dir_max;
}

bool cmLocalVisualStudio7Generator::WriteGroup(
  const cmSourceGroup* sg, cmGeneratorTarget* target, std::ostream& fout,
  const std::string& libName, std::vector<std::string> const& configs,
  std::map<cmSourceFile const*, size_t> const& sourcesIndex)
{
  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);
  const std::vector<const cmSourceFile*>& sourceFiles = sg->GetSourceFiles();
  std::vector<cmSourceGroup> const& children = sg->GetGroupChildren();

  // Write the children to temporary output.
  bool hasChildrenWithSources = false;
  std::ostringstream tmpOut;
  for (unsigned int i = 0; i < children.size(); ++i) {
    if (this->WriteGroup(&children[i], target, tmpOut, libName, configs,
                         sourcesIndex)) {
      hasChildrenWithSources = true;
    }
  }

  // If the group is empty, don't write it at all.
  if (sourceFiles.empty() && !hasChildrenWithSources) {
    return false;
  }

  // If the group has a name, write the header.
  std::string name = sg->GetName();
  if (name != "") {
    this->WriteVCProjBeginGroup(fout, name.c_str(), "");
  }

  std::vector<cmGeneratorTarget::AllConfigSource> const& sources =
    target->GetAllConfigSources();

  // Loop through each source in the source group.
  for (std::vector<const cmSourceFile*>::const_iterator sf =
         sourceFiles.begin();
       sf != sourceFiles.end(); ++sf) {
    std::string source = (*sf)->GetFullPath();

    if (source != libName || target->GetType() == cmStateEnums::UTILITY ||
        target->GetType() == cmStateEnums::GLOBAL_TARGET) {
      // Look up the source kind and configs.
      std::map<cmSourceFile const*, size_t>::const_iterator map_it =
        sourcesIndex.find(*sf);
      // The map entry must exist because we populated it earlier.
      assert(map_it != sourcesIndex.end());
      cmGeneratorTarget::AllConfigSource const& acs = sources[map_it->second];

      FCInfo fcinfo(this, target, acs, configs);

      fout << "\t\t\t<File\n";
      std::string d = this->ConvertToXMLOutputPathSingle(source.c_str());
      // Tell MS-Dev what the source is.  If the compiler knows how to
      // build it, then it will.
      fout << "\t\t\t\tRelativePath=\"" << d << "\">\n";
      if (cmCustomCommand const* command = (*sf)->GetCustomCommand()) {
        this->WriteCustomRule(fout, configs, source.c_str(), *command, fcinfo);
      } else if (!fcinfo.FileConfigMap.empty()) {
        const char* aCompilerTool = "VCCLCompilerTool";
        const char* ppLang = "CXX";
        if (this->FortranProject) {
          aCompilerTool = "VFFortranCompilerTool";
        }
        std::string const& lang = (*sf)->GetLanguage();
        std::string ext = (*sf)->GetExtension();
        ext = cmSystemTools::LowerCase(ext);
        if (ext == "idl") {
          aCompilerTool = "VCMIDLTool";
          if (this->FortranProject) {
            aCompilerTool = "VFMIDLTool";
          }
        }
        if (ext == "rc") {
          aCompilerTool = "VCResourceCompilerTool";
          ppLang = "RC";
          if (this->FortranProject) {
            aCompilerTool = "VFResourceCompilerTool";
          }
        }
        if (ext == "def") {
          aCompilerTool = "VCCustomBuildTool";
          if (this->FortranProject) {
            aCompilerTool = "VFCustomBuildTool";
          }
        }
        if (gg->IsMasmEnabled() && !this->FortranProject &&
            lang == "ASM_MASM") {
          aCompilerTool = "MASM";
        }
        if (acs.Kind == cmGeneratorTarget::SourceKindExternalObject) {
          aCompilerTool = "VCCustomBuildTool";
        }
        for (std::map<std::string, cmLVS7GFileConfig>::const_iterator fci =
               fcinfo.FileConfigMap.begin();
             fci != fcinfo.FileConfigMap.end(); ++fci) {
          cmLVS7GFileConfig const& fc = fci->second;
          fout << "\t\t\t\t<FileConfiguration\n"
               << "\t\t\t\t\tName=\"" << fci->first << "|"
               << gg->GetPlatformName() << "\"";
          if (fc.ExcludedFromBuild) {
            fout << " ExcludedFromBuild=\"true\"";
          }
          fout << ">\n";
          fout << "\t\t\t\t\t<Tool\n"
               << "\t\t\t\t\tName=\"" << aCompilerTool << "\"\n";
          if (!fc.CompileFlags.empty() || !fc.CompileDefs.empty() ||
              !fc.CompileDefsConfig.empty()) {
            Options::Tool tool = Options::Compiler;
            cmVS7FlagTable const* table =
              cmLocalVisualStudio7GeneratorFlagTable;
            if (this->FortranProject) {
              tool = Options::FortranCompiler;
              table = cmLocalVisualStudio7GeneratorFortranFlagTable;
            }
            Options fileOptions(this, tool, table, gg->ExtraFlagTable);
            fileOptions.Parse(fc.CompileFlags.c_str());
            fileOptions.AddDefines(fc.CompileDefs.c_str());
            fileOptions.AddDefines(fc.CompileDefsConfig.c_str());
            fileOptions.OutputFlagMap(fout, "\t\t\t\t\t");
            fileOptions.OutputPreprocessorDefinitions(fout, "\t\t\t\t\t", "\n",
                                                      ppLang);
          }
          if (!fc.AdditionalDeps.empty()) {
            fout << "\t\t\t\t\tAdditionalDependencies=\"" << fc.AdditionalDeps
                 << "\"\n";
          }
          if (!fc.ObjectName.empty()) {
            fout << "\t\t\t\t\tObjectFile=\"$(IntDir)/" << fc.ObjectName
                 << "\"\n";
          }
          fout << "\t\t\t\t\t/>\n"
               << "\t\t\t\t</FileConfiguration>\n";
        }
      }
      fout << "\t\t\t</File>\n";
    }
  }

  // If the group has children with source files, write the children.
  if (hasChildrenWithSources) {
    fout << tmpOut.str();
  }

  // If the group has a name, write the footer.
  if (name != "") {
    this->WriteVCProjEndGroup(fout);
  }

  return true;
}

void cmLocalVisualStudio7Generator::WriteCustomRule(
  std::ostream& fout, std::vector<std::string> const& configs,
  const char* source, const cmCustomCommand& command, FCInfo& fcinfo)
{
  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);

  // Write the rule for each configuration.
  const char* compileTool = "VCCLCompilerTool";
  if (this->FortranProject) {
    compileTool = "VFCLCompilerTool";
  }
  const char* customTool = "VCCustomBuildTool";
  if (this->FortranProject) {
    customTool = "VFCustomBuildTool";
  }
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    cmCustomCommandGenerator ccg(command, *i, this);
    cmLVS7GFileConfig const& fc = fcinfo.FileConfigMap[*i];
    fout << "\t\t\t\t<FileConfiguration\n";
    fout << "\t\t\t\t\tName=\"" << *i << "|" << gg->GetPlatformName()
         << "\">\n";
    if (!fc.CompileFlags.empty()) {
      fout << "\t\t\t\t\t<Tool\n"
           << "\t\t\t\t\tName=\"" << compileTool << "\"\n"
           << "\t\t\t\t\tAdditionalOptions=\""
           << this->EscapeForXML(fc.CompileFlags.c_str()) << "\"/>\n";
    }

    std::string comment = this->ConstructComment(ccg);
    std::string script = this->ConstructScript(ccg);
    if (this->FortranProject) {
      cmSystemTools::ReplaceString(script, "$(Configuration)", i->c_str());
    }
    /* clang-format off */
    fout << "\t\t\t\t\t<Tool\n"
         << "\t\t\t\t\tName=\"" << customTool << "\"\n"
         << "\t\t\t\t\tDescription=\""
         << this->EscapeForXML(comment.c_str()) << "\"\n"
         << "\t\t\t\t\tCommandLine=\""
         << this->EscapeForXML(script.c_str()) << "\"\n"
         << "\t\t\t\t\tAdditionalDependencies=\"";
    /* clang-format on */
    if (ccg.GetDepends().empty()) {
      // There are no real dependencies.  Produce an artificial one to
      // make sure the rule runs reliably.
      if (!cmSystemTools::FileExists(source)) {
        cmsys::ofstream depout(source);
        depout << "Artificial dependency for a custom command.\n";
      }
      fout << this->ConvertToXMLOutputPath(source);
    } else {
      // Write out the dependencies for the rule.
      for (std::vector<std::string>::const_iterator d =
             ccg.GetDepends().begin();
           d != ccg.GetDepends().end(); ++d) {
        // Get the real name of the dependency in case it is a CMake target.
        std::string dep;
        if (this->GetRealDependency(d->c_str(), i->c_str(), dep)) {
          fout << this->ConvertToXMLOutputPath(dep.c_str()) << ";";
        }
      }
    }
    fout << "\"\n";
    fout << "\t\t\t\t\tOutputs=\"";
    if (ccg.GetOutputs().empty()) {
      fout << source << "_force";
    } else {
      // Write a rule for the output generated by this command.
      const char* sep = "";
      for (std::vector<std::string>::const_iterator o =
             ccg.GetOutputs().begin();
           o != ccg.GetOutputs().end(); ++o) {
        fout << sep << this->ConvertToXMLOutputPathSingle(o->c_str());
        sep = ";";
      }
    }
    fout << "\"/>\n";
    fout << "\t\t\t\t</FileConfiguration>\n";
  }
}

void cmLocalVisualStudio7Generator::WriteVCProjBeginGroup(std::ostream& fout,
                                                          const char* group,
                                                          const char*)
{
  /* clang-format off */
  fout << "\t\t<Filter\n"
       << "\t\t\tName=\"" << group << "\"\n"
       << "\t\t\tFilter=\"\">\n";
  /* clang-format on */
}

void cmLocalVisualStudio7Generator::WriteVCProjEndGroup(std::ostream& fout)
{
  fout << "\t\t</Filter>\n";
}

// look for custom rules on a target and collect them together
void cmLocalVisualStudio7Generator::OutputTargetRules(
  std::ostream& fout, const std::string& configName, cmGeneratorTarget* target,
  const std::string& /*libName*/)
{
  if (target->GetType() > cmStateEnums::GLOBAL_TARGET) {
    return;
  }
  EventWriter event(this, configName, fout);

  // Add pre-build event.
  const char* tool =
    this->FortranProject ? "VFPreBuildEventTool" : "VCPreBuildEventTool";
  event.Start(tool);
  event.Write(target->GetPreBuildCommands());
  event.Finish();

  // Add pre-link event.
  tool = this->FortranProject ? "VFPreLinkEventTool" : "VCPreLinkEventTool";
  event.Start(tool);
  bool addedPrelink = false;
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    target->GetModuleDefinitionInfo(configName);
  if (mdi && mdi->DefFileGenerated) {
    addedPrelink = true;
    std::vector<cmCustomCommand> commands = target->GetPreLinkCommands();
    cmGlobalVisualStudioGenerator* gg =
      static_cast<cmGlobalVisualStudioGenerator*>(this->GlobalGenerator);
    gg->AddSymbolExportCommand(target, commands, configName);
    event.Write(commands);
  }
  if (!addedPrelink) {
    event.Write(target->GetPreLinkCommands());
  }
  CM_AUTO_PTR<cmCustomCommand> pcc(
    this->MaybeCreateImplibDir(target, configName, this->FortranProject));
  if (pcc.get()) {
    event.Write(*pcc);
  }
  event.Finish();

  // Add post-build event.
  tool =
    this->FortranProject ? "VFPostBuildEventTool" : "VCPostBuildEventTool";
  event.Start(tool);
  event.Write(target->GetPostBuildCommands());
  event.Finish();
}

void cmLocalVisualStudio7Generator::WriteProjectSCC(std::ostream& fout,
                                                    cmGeneratorTarget* target)
{
  // if we have all the required Source code control tags
  // then add that to the project
  const char* vsProjectname = target->GetProperty("VS_SCC_PROJECTNAME");
  const char* vsLocalpath = target->GetProperty("VS_SCC_LOCALPATH");
  const char* vsProvider = target->GetProperty("VS_SCC_PROVIDER");

  if (vsProvider && vsLocalpath && vsProjectname) {
    /* clang-format off */
    fout << "\tSccProjectName=\"" << vsProjectname << "\"\n"
         << "\tSccLocalPath=\"" << vsLocalpath << "\"\n"
         << "\tSccProvider=\"" << vsProvider << "\"\n";
    /* clang-format on */

    const char* vsAuxPath = target->GetProperty("VS_SCC_AUXPATH");
    if (vsAuxPath) {
      fout << "\tSccAuxPath=\"" << vsAuxPath << "\"\n";
    }
  }
}

void cmLocalVisualStudio7Generator::WriteProjectStartFortran(
  std::ostream& fout, const std::string& libName, cmGeneratorTarget* target)
{

  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);
  /* clang-format off */
  fout << "<?xml version=\"1.0\" encoding = \""
       << gg->Encoding() << "\"?>\n"
       << "<VisualStudioProject\n"
       << "\tProjectCreator=\"Intel Fortran\"\n"
       << "\tVersion=\"" << gg->GetIntelProjectVersion() << "\"\n";
  /* clang-format on */
  const char* keyword = target->GetProperty("VS_KEYWORD");
  if (!keyword) {
    keyword = "Console Application";
  }
  const char* projectType = 0;
  switch (target->GetType()) {
    case cmStateEnums::STATIC_LIBRARY:
      projectType = "typeStaticLibrary";
      if (keyword) {
        keyword = "Static Library";
      }
      break;
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
      projectType = "typeDynamicLibrary";
      if (!keyword) {
        keyword = "Dll";
      }
      break;
    case cmStateEnums::EXECUTABLE:
      if (!keyword) {
        keyword = "Console Application";
      }
      projectType = 0;
      break;
    case cmStateEnums::UTILITY:
    case cmStateEnums::GLOBAL_TARGET:
    default:
      break;
  }
  if (projectType) {
    fout << "\tProjectType=\"" << projectType << "\"\n";
  }
  this->WriteProjectSCC(fout, target);
  /* clang-format off */
  fout<< "\tKeyword=\"" << keyword << "\">\n"
       << "\tProjectGUID=\"{" << gg->GetGUID(libName.c_str()) << "}\">\n"
       << "\t<Platforms>\n"
       << "\t\t<Platform\n\t\t\tName=\"" << gg->GetPlatformName() << "\"/>\n"
       << "\t</Platforms>\n";
  /* clang-format on */
}

void cmLocalVisualStudio7Generator::WriteProjectStart(
  std::ostream& fout, const std::string& libName, cmGeneratorTarget* target,
  std::vector<cmSourceGroup>&)
{
  if (this->FortranProject) {
    this->WriteProjectStartFortran(fout, libName, target);
    return;
  }

  cmGlobalVisualStudio7Generator* gg =
    static_cast<cmGlobalVisualStudio7Generator*>(this->GlobalGenerator);

  /* clang-format off */
  fout << "<?xml version=\"1.0\" encoding = \""
       << gg->Encoding() << "\"?>\n"
       << "<VisualStudioProject\n"
       << "\tProjectType=\"Visual C++\"\n";
  /* clang-format on */
  fout << "\tVersion=\"" << (gg->GetVersion() / 10) << ".00\"\n";
  const char* projLabel = target->GetProperty("PROJECT_LABEL");
  if (!projLabel) {
    projLabel = libName.c_str();
  }
  const char* keyword = target->GetProperty("VS_KEYWORD");
  if (!keyword) {
    keyword = "Win32Proj";
  }
  fout << "\tName=\"" << projLabel << "\"\n";
  fout << "\tProjectGUID=\"{" << gg->GetGUID(libName.c_str()) << "}\"\n";
  this->WriteProjectSCC(fout, target);
  if (const char* targetFrameworkVersion =
        target->GetProperty("VS_DOTNET_TARGET_FRAMEWORK_VERSION")) {
    fout << "\tTargetFrameworkVersion=\"" << targetFrameworkVersion << "\"\n";
  }
  /* clang-format off */
  fout << "\tKeyword=\"" << keyword << "\">\n"
       << "\t<Platforms>\n"
       << "\t\t<Platform\n\t\t\tName=\"" << gg->GetPlatformName() << "\"/>\n"
       << "\t</Platforms>\n";
  /* clang-format on */
  if (gg->IsMasmEnabled()) {
    /* clang-format off */
    fout <<
      "\t<ToolFiles>\n"
      "\t\t<DefaultToolFile\n"
      "\t\t\tFileName=\"masm.rules\"\n"
      "\t\t/>\n"
      "\t</ToolFiles>\n"
      ;
    /* clang-format on */
  }
}

void cmLocalVisualStudio7Generator::WriteVCProjFooter(
  std::ostream& fout, cmGeneratorTarget* target)
{
  fout << "\t<Globals>\n";

  std::vector<std::string> const& props = target->GetPropertyKeys();
  for (std::vector<std::string>::const_iterator i = props.begin();
       i != props.end(); ++i) {
    if (i->find("VS_GLOBAL_") == 0) {
      std::string name = i->substr(10);
      if (name != "") {
        /* clang-format off */
        fout << "\t\t<Global\n"
             << "\t\t\tName=\"" << name << "\"\n"
             << "\t\t\tValue=\"" << target->GetProperty(*i) << "\"\n"
             << "\t\t/>\n";
        /* clang-format on */
      }
    }
  }

  fout << "\t</Globals>\n"
       << "</VisualStudioProject>\n";
}

std::string cmLocalVisualStudio7GeneratorEscapeForXML(const std::string& s)
{
  std::string ret = s;
  cmSystemTools::ReplaceString(ret, "&", "&amp;");
  cmSystemTools::ReplaceString(ret, "\"", "&quot;");
  cmSystemTools::ReplaceString(ret, "<", "&lt;");
  cmSystemTools::ReplaceString(ret, ">", "&gt;");
  cmSystemTools::ReplaceString(ret, "\n", "&#x0D;&#x0A;");
  return ret;
}

std::string cmLocalVisualStudio7Generator::EscapeForXML(const std::string& s)
{
  return cmLocalVisualStudio7GeneratorEscapeForXML(s);
}

std::string cmLocalVisualStudio7Generator::ConvertToXMLOutputPath(
  const char* path)
{
  std::string ret =
    this->ConvertToOutputFormat(path, cmOutputConverter::SHELL);
  cmSystemTools::ReplaceString(ret, "&", "&amp;");
  cmSystemTools::ReplaceString(ret, "\"", "&quot;");
  cmSystemTools::ReplaceString(ret, "<", "&lt;");
  cmSystemTools::ReplaceString(ret, ">", "&gt;");
  return ret;
}

std::string cmLocalVisualStudio7Generator::ConvertToXMLOutputPathSingle(
  const char* path)
{
  std::string ret =
    this->ConvertToOutputFormat(path, cmOutputConverter::SHELL);
  cmSystemTools::ReplaceString(ret, "\"", "");
  cmSystemTools::ReplaceString(ret, "&", "&amp;");
  cmSystemTools::ReplaceString(ret, "<", "&lt;");
  cmSystemTools::ReplaceString(ret, ">", "&gt;");
  return ret;
}

// This class is used to parse an existing vs 7 project
// and extract the GUID
class cmVS7XMLParser : public cmXMLParser
{
public:
  virtual void EndElement(const std::string& /* name */) {}
  virtual void StartElement(const std::string& name, const char** atts)
  {
    // once the GUID is found do nothing
    if (!this->GUID.empty()) {
      return;
    }
    int i = 0;
    if ("VisualStudioProject" == name) {
      while (atts[i]) {
        if (strcmp(atts[i], "ProjectGUID") == 0) {
          if (atts[i + 1]) {
            this->GUID = atts[i + 1];
            this->GUID = this->GUID.substr(1, this->GUID.size() - 2);
          } else {
            this->GUID = "";
          }
          return;
        }
        ++i;
      }
    }
  }
  int InitializeParser()
  {
    int ret = cmXMLParser::InitializeParser();
    if (ret == 0) {
      return ret;
    }
    // visual studio projects have a strange encoding, but it is
    // really utf-8
    XML_SetEncoding(static_cast<XML_Parser>(this->Parser), "utf-8");
    return 1;
  }
  std::string GUID;
};

void cmLocalVisualStudio7Generator::ReadAndStoreExternalGUID(
  const std::string& name, const char* path)
{
  cmVS7XMLParser parser;
  parser.ParseFile(path);
  // if we can not find a GUID then we will generate one later
  if (parser.GUID.empty()) {
    return;
  }
  std::string guidStoreName = name;
  guidStoreName += "_GUID_CMAKE";
  // save the GUID in the cache
  this->GlobalGenerator->GetCMakeInstance()->AddCacheEntry(
    guidStoreName.c_str(), parser.GUID.c_str(), "Stored GUID",
    cmStateEnums::INTERNAL);
}

std::string cmLocalVisualStudio7Generator::GetTargetDirectory(
  cmGeneratorTarget const* target) const
{
  std::string dir;
  dir += target->GetName();
  dir += ".dir";
  return dir;
}

static bool cmLVS7G_IsFAT(const char* dir)
{
  if (dir[0] && dir[1] == ':') {
    char volRoot[4] = "_:/";
    volRoot[0] = dir[0];
    char fsName[16];
    if (GetVolumeInformationA(volRoot, 0, 0, 0, 0, 0, fsName, 16) &&
        strstr(fsName, "FAT") != 0) {
      return true;
    }
  }
  return false;
}
