/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmsys/CommandLineArguments.hxx"
#include "cmsys/Encoding.hxx"
#include <iostream>
#include <map>
#include <memory> // IWYU pragma: keep
#include <sstream>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
#  include "cmsys/ConsoleBuf.hxx"
#endif

#include "cmCPackGenerator.h"
#include "cmCPackGeneratorFactory.h"
#include "cmCPackLog.h"
#include "cmDocumentation.h"
#include "cmDocumentationEntry.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmake.h"

static const char* cmDocumentationName[][2] = {
  { nullptr, "  cpack - Packaging driver provided by CMake." },
  { nullptr, nullptr }
};

static const char* cmDocumentationUsage[][2] = {
  // clang-format off
  { nullptr, "  cpack [options]" },
  { nullptr, nullptr }
  // clang-format on
};

static const char* cmDocumentationOptions[][2] = {
  { "-G <generators>", "Override/define CPACK_GENERATOR" },
  { "-C <Configuration>", "Specify the project configuration" },
  { "-D <var>=<value>", "Set a CPack variable." },
  { "--config <configFile>", "Specify the config file." },
  { "--verbose,-V", "Enable verbose output" },
  { "--trace", "Put underlying cmake scripts in trace mode." },
  { "--trace-expand", "Put underlying cmake scripts in expanded trace mode." },
  { "--debug", "Enable debug output (for CPack developers)" },
  { "-P <packageName>", "Override/define CPACK_PACKAGE_NAME" },
  { "-R <packageVersion>", "Override/define CPACK_PACKAGE_VERSION" },
  { "-B <packageDirectory>", "Override/define CPACK_PACKAGE_DIRECTORY" },
  { "--vendor <vendorName>", "Override/define CPACK_PACKAGE_VENDOR" },
  { nullptr, nullptr }
};

int cpackUnknownArgument(const char* /*unused*/, void* /*unused*/)
{
  return 1;
}

struct cpackDefinitions
{
  typedef std::map<std::string, std::string> MapType;
  MapType Map;
  cmCPackLog* Log;
};

int cpackDefinitionArgument(const char* argument, const char* cValue,
                            void* call_data)
{
  (void)argument;
  cpackDefinitions* def = static_cast<cpackDefinitions*>(call_data);
  std::string value = cValue;
  size_t pos = value.find_first_of('=');
  if (pos == std::string::npos) {
    cmCPack_Log(def->Log, cmCPackLog::LOG_ERROR,
                "Please specify CPack definitions as: KEY=VALUE" << std::endl);
    return 0;
  }
  std::string key = value.substr(0, pos);
  value = value.c_str() + pos + 1;
  def->Map[key] = value;
  cmCPack_Log(def->Log, cmCPackLog::LOG_DEBUG,
              "Set CPack variable: " << key << " to \"" << value << "\""
                                     << std::endl);
  return 1;
}

static void cpackProgressCallback(const char* message, float progress,
                                  void* clientdata)
{
  (void)progress;
  (void)clientdata;

  std::cout << "-- " << message << std::endl;
}

// this is CPack.
int main(int argc, char const* const* argv)
{
#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
  // Replace streambuf so we can output Unicode to console
  cmsys::ConsoleBuf::Manager consoleOut(std::cout);
  consoleOut.SetUTF8Pipes();
  cmsys::ConsoleBuf::Manager consoleErr(std::cerr, true);
  consoleErr.SetUTF8Pipes();
#endif
  cmsys::Encoding::CommandLineArguments args =
    cmsys::Encoding::CommandLineArguments::Main(argc, argv);
  argc = args.argc();
  argv = args.argv();

  cmSystemTools::EnableMSVCDebugHook();
  cmSystemTools::InitializeLibUV();
  cmSystemTools::FindCMakeResources(argv[0]);
  cmCPackLog log;

  log.SetErrorPrefix("CPack Error: ");
  log.SetWarningPrefix("CPack Warning: ");
  log.SetOutputPrefix("CPack: ");
  log.SetVerbosePrefix("CPack Verbose: ");

  if (cmSystemTools::GetCurrentWorkingDirectory().empty()) {
    cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                "Current working directory cannot be established."
                  << std::endl);
    return 1;
  }

  std::string generator;
  bool help = false;
  bool helpVersion = false;
  bool verbose = false;
  bool trace = false;
  bool traceExpand = false;
  bool debug = false;
  std::string helpFull;
  std::string helpMAN;
  std::string helpHTML;

  std::string cpackProjectName;
  std::string cpackProjectDirectory;
  std::string cpackBuildConfig;
  std::string cpackProjectVersion;
  std::string cpackProjectPatch;
  std::string cpackProjectVendor;
  std::string cpackConfigFile;

  cpackDefinitions definitions;
  definitions.Log = &log;

  cpackConfigFile.clear();

  cmsys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  typedef cmsys::CommandLineArguments argT;
  // Help arguments
  arg.AddArgument("--help", argT::NO_ARGUMENT, &help, "CPack help");
  arg.AddArgument("--help-full", argT::SPACE_ARGUMENT, &helpFull,
                  "CPack help");
  arg.AddArgument("--help-html", argT::SPACE_ARGUMENT, &helpHTML,
                  "CPack help");
  arg.AddArgument("--help-man", argT::SPACE_ARGUMENT, &helpMAN, "CPack help");
  arg.AddArgument("--version", argT::NO_ARGUMENT, &helpVersion, "CPack help");

  arg.AddArgument("-V", argT::NO_ARGUMENT, &verbose, "CPack verbose");
  arg.AddArgument("--verbose", argT::NO_ARGUMENT, &verbose, "-V");
  arg.AddArgument("--debug", argT::NO_ARGUMENT, &debug, "-V");
  arg.AddArgument("--config", argT::SPACE_ARGUMENT, &cpackConfigFile,
                  "CPack configuration file");
  arg.AddArgument("--trace", argT::NO_ARGUMENT, &trace,
                  "Put underlying cmake scripts in trace mode.");
  arg.AddArgument("--trace-expand", argT::NO_ARGUMENT, &traceExpand,
                  "Put underlying cmake scripts in expanded trace mode.");
  arg.AddArgument("-C", argT::SPACE_ARGUMENT, &cpackBuildConfig,
                  "CPack build configuration");
  arg.AddArgument("-G", argT::SPACE_ARGUMENT, &generator, "CPack generator");
  arg.AddArgument("-P", argT::SPACE_ARGUMENT, &cpackProjectName,
                  "CPack project name");
  arg.AddArgument("-R", argT::SPACE_ARGUMENT, &cpackProjectVersion,
                  "CPack project version");
  arg.AddArgument("-B", argT::SPACE_ARGUMENT, &cpackProjectDirectory,
                  "CPack project directory");
  arg.AddArgument("--patch", argT::SPACE_ARGUMENT, &cpackProjectPatch,
                  "CPack project patch");
  arg.AddArgument("--vendor", argT::SPACE_ARGUMENT, &cpackProjectVendor,
                  "CPack project vendor");
  arg.AddCallback("-D", argT::SPACE_ARGUMENT, cpackDefinitionArgument,
                  &definitions, "CPack Definitions");
  arg.SetUnknownArgumentCallback(cpackUnknownArgument);

  // Parse command line
  int parsed = arg.Parse();

  // Setup logging
  if (verbose) {
    log.SetVerbose(verbose);
    cmCPack_Log(&log, cmCPackLog::LOG_OUTPUT, "Enable Verbose" << std::endl);
  }
  if (debug) {
    log.SetDebug(debug);
    cmCPack_Log(&log, cmCPackLog::LOG_OUTPUT, "Enable Debug" << std::endl);
  }

  cmCPack_Log(&log, cmCPackLog::LOG_VERBOSE,
              "Read CPack config file: " << cpackConfigFile << std::endl);

  cmake cminst(cmake::RoleScript);
  cminst.SetHomeDirectory("");
  cminst.SetHomeOutputDirectory("");
  cminst.SetProgressCallback(cpackProgressCallback, nullptr);
  cminst.GetCurrentSnapshot().SetDefaultDefinitions();
  cmGlobalGenerator cmgg(&cminst);
  cmMakefile globalMF(&cmgg, cminst.GetCurrentSnapshot());
#if defined(__CYGWIN__)
  globalMF.AddDefinition("CMAKE_LEGACY_CYGWIN_WIN32", "0");
#endif

  if (trace) {
    cminst.SetTrace(true);
  }
  if (traceExpand) {
    cminst.SetTrace(true);
    cminst.SetTraceExpand(true);
  }

  bool cpackConfigFileSpecified = true;
  if (cpackConfigFile.empty()) {
    cpackConfigFile = cmSystemTools::GetCurrentWorkingDirectory();
    cpackConfigFile += "/CPackConfig.cmake";
    cpackConfigFileSpecified = false;
  }

  cmCPackGeneratorFactory generators;
  generators.SetLogger(&log);
  cmCPackGenerator* cpackGenerator = nullptr;

  cmDocumentation doc;
  doc.addCPackStandardDocSections();
  /* Were we invoked to display doc or to do some work ?
   * Unlike cmake launching cpack with zero argument
   * should launch cpack using "cpackConfigFile" if it exists
   * in the current directory.
   */
  help = doc.CheckOptions(argc, argv, "-G") && argc != 1;

  // This part is used for cpack documentation lookup as well.
  cminst.AddCMakePaths();

  if (parsed && !help) {
    // find out which system cpack is running on, so it can setup the search
    // paths, so FIND_XXX() commands can be used in scripts
    std::string systemFile =
      globalMF.GetModulesFile("CMakeDetermineSystem.cmake");
    if (!globalMF.ReadListFile(systemFile.c_str())) {
      cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                  "Error reading CMakeDetermineSystem.cmake" << std::endl);
      return 1;
    }

    systemFile =
      globalMF.GetModulesFile("CMakeSystemSpecificInformation.cmake");
    if (!globalMF.ReadListFile(systemFile.c_str())) {
      cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                  "Error reading CMakeSystemSpecificInformation.cmake"
                    << std::endl);
      return 1;
    }

    if (!cpackBuildConfig.empty()) {
      globalMF.AddDefinition("CPACK_BUILD_CONFIG", cpackBuildConfig.c_str());
    }

    if (cmSystemTools::FileExists(cpackConfigFile)) {
      cpackConfigFile = cmSystemTools::CollapseFullPath(cpackConfigFile);
      cmCPack_Log(&log, cmCPackLog::LOG_VERBOSE,
                  "Read CPack configuration file: " << cpackConfigFile
                                                    << std::endl);
      if (!globalMF.ReadListFile(cpackConfigFile.c_str())) {
        cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                    "Problem reading CPack config file: \""
                      << cpackConfigFile << "\"" << std::endl);
        return 1;
      }
    } else if (cpackConfigFileSpecified) {
      cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                  "Cannot find CPack config file: \"" << cpackConfigFile
                                                      << "\"" << std::endl);
      return 1;
    }

    if (!generator.empty()) {
      globalMF.AddDefinition("CPACK_GENERATOR", generator.c_str());
    }
    if (!cpackProjectName.empty()) {
      globalMF.AddDefinition("CPACK_PACKAGE_NAME", cpackProjectName.c_str());
    }
    if (!cpackProjectVersion.empty()) {
      globalMF.AddDefinition("CPACK_PACKAGE_VERSION",
                             cpackProjectVersion.c_str());
    }
    if (!cpackProjectVendor.empty()) {
      globalMF.AddDefinition("CPACK_PACKAGE_VENDOR",
                             cpackProjectVendor.c_str());
    }
    // if this is not empty it has been set on the command line
    // go for it. Command line override values set in config file.
    if (!cpackProjectDirectory.empty()) {
      globalMF.AddDefinition("CPACK_PACKAGE_DIRECTORY",
                             cpackProjectDirectory.c_str());
    }
    // The value has not been set on the command line
    else {
      // get a default value (current working directory)
      cpackProjectDirectory = cmsys::SystemTools::GetCurrentWorkingDirectory();
      // use default value iff no value has been provided by the config file
      if (!globalMF.IsSet("CPACK_PACKAGE_DIRECTORY")) {
        globalMF.AddDefinition("CPACK_PACKAGE_DIRECTORY",
                               cpackProjectDirectory.c_str());
      }
    }
    for (auto const& cd : definitions.Map) {
      globalMF.AddDefinition(cd.first, cd.second.c_str());
    }

    const char* cpackModulesPath = globalMF.GetDefinition("CPACK_MODULE_PATH");
    if (cpackModulesPath) {
      globalMF.AddDefinition("CMAKE_MODULE_PATH", cpackModulesPath);
    }
    const char* genList = globalMF.GetDefinition("CPACK_GENERATOR");
    if (!genList) {
      cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                  "CPack generator not specified" << std::endl);
    } else {
      std::vector<std::string> generatorsVector;
      cmSystemTools::ExpandListArgument(genList, generatorsVector);
      for (std::string const& gen : generatorsVector) {
        cmMakefile::ScopePushPop raii(&globalMF);
        cmMakefile* mf = &globalMF;
        cmCPack_Log(&log, cmCPackLog::LOG_VERBOSE,
                    "Specified generator: " << gen << std::endl);
        if (parsed && !mf->GetDefinition("CPACK_PACKAGE_NAME")) {
          cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                      "CPack project name not specified" << std::endl);
          parsed = 0;
        }
        if (parsed &&
            !(mf->GetDefinition("CPACK_PACKAGE_VERSION") ||
              (mf->GetDefinition("CPACK_PACKAGE_VERSION_MAJOR") &&
               mf->GetDefinition("CPACK_PACKAGE_VERSION_MINOR") &&
               mf->GetDefinition("CPACK_PACKAGE_VERSION_PATCH")))) {
          cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                      "CPack project version not specified"
                        << std::endl
                        << "Specify CPACK_PACKAGE_VERSION, or "
                           "CPACK_PACKAGE_VERSION_MAJOR, "
                           "CPACK_PACKAGE_VERSION_MINOR, and "
                           "CPACK_PACKAGE_VERSION_PATCH."
                        << std::endl);
          parsed = 0;
        }
        if (parsed) {
          cpackGenerator = generators.NewGenerator(gen);
          if (cpackGenerator) {
            cpackGenerator->SetTrace(trace);
            cpackGenerator->SetTraceExpand(traceExpand);
          } else {
            cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                        "Cannot initialize CPack generator: " << gen
                                                              << std::endl);
            parsed = 0;
          }

          if (parsed && !cpackGenerator->Initialize(gen, mf)) {
            cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                        "Cannot initialize the generator " << gen
                                                           << std::endl);
            parsed = 0;
          }

          if (!mf->GetDefinition("CPACK_INSTALL_COMMANDS") &&
              !mf->GetDefinition("CPACK_INSTALL_SCRIPT") &&
              !mf->GetDefinition("CPACK_INSTALLED_DIRECTORIES") &&
              !mf->GetDefinition("CPACK_INSTALL_CMAKE_PROJECTS")) {
            cmCPack_Log(
              &log, cmCPackLog::LOG_ERROR,
              "Please specify build tree of the project that uses CMake "
              "using CPACK_INSTALL_CMAKE_PROJECTS, specify "
              "CPACK_INSTALL_COMMANDS, CPACK_INSTALL_SCRIPT, or "
              "CPACK_INSTALLED_DIRECTORIES."
                << std::endl);
            parsed = 0;
          }
          if (parsed) {
            const char* projName = mf->GetDefinition("CPACK_PACKAGE_NAME");
            cmCPack_Log(&log, cmCPackLog::LOG_VERBOSE,
                        "Use generator: " << cpackGenerator->GetNameOfClass()
                                          << std::endl);
            cmCPack_Log(&log, cmCPackLog::LOG_VERBOSE,
                        "For project: " << projName << std::endl);

            const char* projVersion =
              mf->GetDefinition("CPACK_PACKAGE_VERSION");
            if (!projVersion) {
              const char* projVersionMajor =
                mf->GetDefinition("CPACK_PACKAGE_VERSION_MAJOR");
              const char* projVersionMinor =
                mf->GetDefinition("CPACK_PACKAGE_VERSION_MINOR");
              const char* projVersionPatch =
                mf->GetDefinition("CPACK_PACKAGE_VERSION_PATCH");
              std::ostringstream ostr;
              ostr << projVersionMajor << "." << projVersionMinor << "."
                   << projVersionPatch;
              mf->AddDefinition("CPACK_PACKAGE_VERSION", ostr.str().c_str());
            }

            int res = cpackGenerator->DoPackage();
            if (!res) {
              cmCPack_Log(&log, cmCPackLog::LOG_ERROR,
                          "Error when generating package: " << projName
                                                            << std::endl);
              return 1;
            }
          }
        }
      }
    }
  }

  /* In this case we are building the documentation object
   * instance in order to create appropriate structure
   * in order to satisfy the appropriate --help-xxx request
   */
  if (help) {
    // Construct and print requested documentation.

    doc.SetName("cpack");
    doc.SetSection("Name", cmDocumentationName);
    doc.SetSection("Usage", cmDocumentationUsage);
    doc.PrependSection("Options", cmDocumentationOptions);

    std::vector<cmDocumentationEntry> v;
    for (auto const& g : generators.GetGeneratorsList()) {
      cmDocumentationEntry e;
      e.Name = g.first;
      e.Brief = g.second;
      v.push_back(std::move(e));
    }
    doc.SetSection("Generators", v);

    return doc.PrintRequestedDocumentation(std::cout) ? 0 : 1;
  }

  if (cmSystemTools::GetErrorOccuredFlag()) {
    return 1;
  }

  return 0;
}
