/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"
#include "cmcmd.h"

#ifdef CMAKE_BUILD_WITH_CMAKE
#  include "cmDocumentation.h"
#  include "cmDynamicLoader.h"
#endif

#include "cm_uv.h"

#include "cmsys/Encoding.hxx"
#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
#  include "cmsys/ConsoleBuf.hxx"
#endif

#include <ctype.h>
#include <iostream>
#include <string.h>
#include <string>
#include <vector>

#ifdef CMAKE_BUILD_WITH_CMAKE
static const char* cmDocumentationName[][2] = {
  { nullptr, "  cmake - Cross-Platform Makefile Generator." },
  { nullptr, nullptr }
};

static const char* cmDocumentationUsage[][2] = {
  { nullptr,
    "  cmake [options] <path-to-source>\n"
    "  cmake [options] <path-to-existing-build>\n"
    "  cmake [options] -S <path-to-source> -B <path-to-build>" },
  { nullptr,
    "Specify a source directory to (re-)generate a build system for "
    "it in the current working directory.  Specify an existing build "
    "directory to re-generate its build system." },
  { nullptr, nullptr }
};

static const char* cmDocumentationUsageNote[][2] = {
  { nullptr, "Run 'cmake --help' for more information." },
  { nullptr, nullptr }
};

#  define CMAKE_BUILD_OPTIONS                                                 \
    "  <dir>          = Project binary directory to be built.\n"              \
    "  -j [<jobs>] --parallel [<jobs>] = Build in parallel using\n"           \
    "                   the given number of jobs. If <jobs> is omitted\n"     \
    "                   the native build tool's default number is used.\n"    \
    "                   The CMAKE_BUILD_PARALLEL_LEVEL environment "          \
    "variable\n"                                                              \
    "                   specifies a default parallel level when this "        \
    "option\n"                                                                \
    "                   is not given.\n"                                      \
    "  --target <tgt> = Build <tgt> instead of default targets.\n"            \
    "                   May only be specified once.\n"                        \
    "  --config <cfg> = For multi-configuration tools, choose <cfg>.\n"       \
    "  --clean-first  = Build target 'clean' first, then build.\n"            \
    "                   (To clean only, use --target 'clean'.)\n"             \
    "  --use-stderr   = Ignored.  Behavior is default in CMake >= 3.0.\n"     \
    "  --             = Pass remaining options to the native tool.\n"

static const char* cmDocumentationOptions[][2] = {
  CMAKE_STANDARD_OPTIONS_TABLE,
  { "-E", "CMake command mode." },
  { "-L[A][H]", "List non-advanced cached variables." },
  { "--build <dir>", "Build a CMake-generated project binary tree." },
  { "--open <dir>", "Open generated project in the associated application." },
  { "-N", "View mode only." },
  { "-P <file>", "Process script mode." },
  { "--find-package", "Run in pkg-config like mode." },
  { "--graphviz=[file]",
    "Generate graphviz of dependencies, see "
    "CMakeGraphVizOptions.cmake for more." },
  { "--system-information [file]", "Dump information about this system." },
  { "--debug-trycompile",
    "Do not delete the try_compile build tree. Only "
    "useful on one try_compile at a time." },
  { "--debug-output", "Put cmake in a debug mode." },
  { "--trace", "Put cmake in trace mode." },
  { "--trace-expand", "Put cmake in trace mode with variable expansion." },
  { "--trace-source=<file>",
    "Trace only this CMake file/module. Multiple options allowed." },
  { "--warn-uninitialized", "Warn about uninitialized values." },
  { "--warn-unused-vars", "Warn about unused variables." },
  { "--no-warn-unused-cli", "Don't warn about command line options." },
  { "--check-system-vars",
    "Find problems with variable usage in system "
    "files." },
  { nullptr, nullptr }
};

#endif

static int do_command(int ac, char const* const* av)
{
  std::vector<std::string> args;
  args.reserve(ac - 1);
  args.push_back(av[0]);
  args.insert(args.end(), av + 2, av + ac);
  return cmcmd::ExecuteCMakeCommand(args);
}

int do_cmake(int ac, char const* const* av);
static int do_build(int ac, char const* const* av);
static int do_open(int ac, char const* const* av);

static cmMakefile* cmakemainGetMakefile(void* clientdata)
{
  cmake* cm = static_cast<cmake*>(clientdata);
  if (cm && cm->GetDebugOutput()) {
    cmGlobalGenerator* gg = cm->GetGlobalGenerator();
    if (gg) {
      return gg->GetCurrentMakefile();
    }
  }
  return nullptr;
}

static std::string cmakemainGetStack(void* clientdata)
{
  std::string msg;
  cmMakefile* mf = cmakemainGetMakefile(clientdata);
  if (mf) {
    msg = mf->FormatListFileStack();
    if (!msg.empty()) {
      msg = "\n   Called from: " + msg;
    }
  }

  return msg;
}

static void cmakemainMessageCallback(const char* m, const char* /*unused*/,
                                     bool& /*unused*/, void* clientdata)
{
  std::cerr << m << cmakemainGetStack(clientdata) << std::endl << std::flush;
}

static void cmakemainProgressCallback(const char* m, float prog,
                                      void* clientdata)
{
  cmMakefile* mf = cmakemainGetMakefile(clientdata);
  std::string dir;
  if ((mf) && (strstr(m, "Configuring") == m) && (prog < 0)) {
    dir = " ";
    dir += mf->GetCurrentSourceDirectory();
  } else if ((mf) && (strstr(m, "Generating") == m)) {
    dir = " ";
    dir += mf->GetCurrentBinaryDirectory();
  }

  if ((prog < 0) || (!dir.empty())) {
    std::cout << "-- " << m << dir << cmakemainGetStack(clientdata)
              << std::endl;
  }

  std::cout.flush();
}

int main(int ac, char const* const* av)
{
#if defined(_WIN32) && defined(CMAKE_BUILD_WITH_CMAKE)
  // Replace streambuf so we can output Unicode to console
  cmsys::ConsoleBuf::Manager consoleOut(std::cout);
  consoleOut.SetUTF8Pipes();
  cmsys::ConsoleBuf::Manager consoleErr(std::cerr, true);
  consoleErr.SetUTF8Pipes();
#endif
  cmsys::Encoding::CommandLineArguments args =
    cmsys::Encoding::CommandLineArguments::Main(ac, av);
  ac = args.argc();
  av = args.argv();

  cmSystemTools::EnableMSVCDebugHook();
  cmSystemTools::InitializeLibUV();
  cmSystemTools::FindCMakeResources(av[0]);
  if (ac > 1) {
    if (strcmp(av[1], "--build") == 0) {
      return do_build(ac, av);
    }
    if (strcmp(av[1], "--open") == 0) {
      return do_open(ac, av);
    }
    if (strcmp(av[1], "-E") == 0) {
      return do_command(ac, av);
    }
  }
  int ret = do_cmake(ac, av);
#ifdef CMAKE_BUILD_WITH_CMAKE
  cmDynamicLoader::FlushCache();
#endif
  uv_loop_close(uv_default_loop());
  return ret;
}

int do_cmake(int ac, char const* const* av)
{
  if (cmSystemTools::GetCurrentWorkingDirectory().empty()) {
    std::cerr << "Current working directory cannot be established."
              << std::endl;
    return 1;
  }

#ifdef CMAKE_BUILD_WITH_CMAKE
  cmDocumentation doc;
  doc.addCMakeStandardDocSections();
  if (doc.CheckOptions(ac, av)) {
    // Construct and print requested documentation.
    cmake hcm(cmake::RoleInternal);
    hcm.SetHomeDirectory("");
    hcm.SetHomeOutputDirectory("");
    hcm.AddCMakePaths();

    // the command line args are processed here so that you can do
    // -DCMAKE_MODULE_PATH=/some/path and have this value accessible here
    std::vector<std::string> args(av, av + ac);
    hcm.SetCacheArgs(args);

    std::vector<cmDocumentationEntry> generators;

    hcm.GetGeneratorDocumentation(generators);

    doc.SetName("cmake");
    doc.SetSection("Name", cmDocumentationName);
    doc.SetSection("Usage", cmDocumentationUsage);
    if (ac == 1) {
      doc.AppendSection("Usage", cmDocumentationUsageNote);
    }
    doc.AppendSection("Generators", generators);
    doc.PrependSection("Options", cmDocumentationOptions);

    return doc.PrintRequestedDocumentation(std::cout) ? 0 : 1;
  }
#else
  if (ac == 1) {
    std::cout
      << "Bootstrap CMake should not be used outside CMake build process."
      << std::endl;
    return 0;
  }
#endif

  bool sysinfo = false;
  bool list_cached = false;
  bool list_all_cached = false;
  bool list_help = false;
  bool view_only = false;
  cmake::WorkingMode workingMode = cmake::NORMAL_MODE;
  std::vector<std::string> args;
  for (int i = 0; i < ac; ++i) {
    if (strcmp(av[i], "-i") == 0) {
      /* clang-format off */
      std::cerr <<
        "The \"cmake -i\" wizard mode is no longer supported.\n"
        "Use the -D option to set cache values on the command line.\n"
        "Use cmake-gui or ccmake for an interactive dialog.\n";
      /* clang-format on */
      return 1;
    }
    if (strcmp(av[i], "--system-information") == 0) {
      sysinfo = true;
    } else if (strcmp(av[i], "-N") == 0) {
      view_only = true;
    } else if (strcmp(av[i], "-L") == 0) {
      list_cached = true;
    } else if (strcmp(av[i], "-LA") == 0) {
      list_all_cached = true;
    } else if (strcmp(av[i], "-LH") == 0) {
      list_cached = true;
      list_help = true;
    } else if (strcmp(av[i], "-LAH") == 0) {
      list_all_cached = true;
      list_help = true;
    } else if (cmHasLiteralPrefix(av[i], "-P")) {
      if (i == ac - 1) {
        cmSystemTools::Error("No script specified for argument -P");
        return 1;
      }
      workingMode = cmake::SCRIPT_MODE;
      args.push_back(av[i]);
      i++;
      args.push_back(av[i]);
    } else if (cmHasLiteralPrefix(av[i], "--find-package")) {
      workingMode = cmake::FIND_PACKAGE_MODE;
      args.push_back(av[i]);
    } else {
      args.push_back(av[i]);
    }
  }
  if (sysinfo) {
    cmake cm(cmake::RoleProject);
    cm.SetHomeDirectory("");
    cm.SetHomeOutputDirectory("");
    int ret = cm.GetSystemInformation(args);
    return ret;
  }
  cmake::Role const role =
    workingMode == cmake::SCRIPT_MODE ? cmake::RoleScript : cmake::RoleProject;
  cmake cm(role);
  cm.SetHomeDirectory("");
  cm.SetHomeOutputDirectory("");
  cmSystemTools::SetMessageCallback(cmakemainMessageCallback, &cm);
  cm.SetProgressCallback(cmakemainProgressCallback, &cm);
  cm.SetWorkingMode(workingMode);

  int res = cm.Run(args, view_only);
  if (list_cached || list_all_cached) {
    std::cout << "-- Cache values" << std::endl;
    std::vector<std::string> keys = cm.GetState()->GetCacheEntryKeys();
    for (std::string const& k : keys) {
      cmStateEnums::CacheEntryType t = cm.GetState()->GetCacheEntryType(k);
      if (t != cmStateEnums::INTERNAL && t != cmStateEnums::STATIC &&
          t != cmStateEnums::UNINITIALIZED) {
        const char* advancedProp =
          cm.GetState()->GetCacheEntryProperty(k, "ADVANCED");
        if (list_all_cached || !advancedProp) {
          if (list_help) {
            std::cout << "// "
                      << cm.GetState()->GetCacheEntryProperty(k, "HELPSTRING")
                      << std::endl;
          }
          std::cout << k << ":" << cmState::CacheEntryTypeToString(t) << "="
                    << cm.GetState()->GetCacheEntryValue(k) << std::endl;
          if (list_help) {
            std::cout << std::endl;
          }
        }
      }
    }
  }

  // Always return a non-negative value.  Windows tools do not always
  // interpret negative return values as errors.
  if (res != 0) {
    return 1;
  }
  return 0;
}

static int do_build(int ac, char const* const* av)
{
#ifndef CMAKE_BUILD_WITH_CMAKE
  std::cerr << "This cmake does not support --build\n";
  return -1;
#else
  int jobs = cmake::NO_BUILD_PARALLEL_LEVEL;
  std::string target;
  std::string config = "Debug";
  std::string dir;
  std::vector<std::string> nativeOptions;
  bool clean = false;
  bool hasTarget = false;

  enum Doing
  {
    DoingNone,
    DoingJobs,
    DoingDir,
    DoingTarget,
    DoingConfig,
    DoingNative
  };
  Doing doing = DoingDir;
  for (int i = 2; i < ac; ++i) {
    if (doing == DoingNative) {
      nativeOptions.push_back(av[i]);
    } else if ((strcmp(av[i], "-j") == 0) ||
               (strcmp(av[i], "--parallel") == 0)) {
      jobs = cmake::DEFAULT_BUILD_PARALLEL_LEVEL;
      /* does the next argument start with a number? */
      if ((i + 1 < ac) && (isdigit(*av[i + 1]))) {
        doing = DoingJobs;
      }
    } else if (strcmp(av[i], "--target") == 0) {
      if (!hasTarget) {
        doing = DoingTarget;
        hasTarget = true;
      } else {
        std::cerr << "'--target' may not be specified more than once.\n\n";
        dir.clear();
        break;
      }
    } else if (strcmp(av[i], "--config") == 0) {
      doing = DoingConfig;
    } else if (strcmp(av[i], "--clean-first") == 0) {
      clean = true;
      doing = DoingNone;
    } else if (strcmp(av[i], "--use-stderr") == 0) {
      /* tolerate legacy option */
    } else if (strcmp(av[i], "--") == 0) {
      doing = DoingNative;
    } else {
      switch (doing) {
        case DoingJobs: {
          unsigned long numJobs = 0;
          if (cmSystemTools::StringToULong(av[i], &numJobs)) {
            jobs = int(numJobs);
            doing = DoingNone;
          } else {
            std::cerr << "'" << av[i - 1] << "' invalid number '" << av[i]
                      << "' given.\n\n";
            dir.clear();
            break;
          }
        } break;
        case DoingDir:
          dir = cmSystemTools::CollapseFullPath(av[i]);
          doing = DoingNone;
          break;
        case DoingTarget:
          target = av[i];
          doing = DoingNone;
          break;
        case DoingConfig:
          config = av[i];
          doing = DoingNone;
          break;
        default:
          std::cerr << "Unknown argument " << av[i] << std::endl;
          dir.clear();
          break;
      }
    }
  }

  if (jobs == cmake::NO_BUILD_PARALLEL_LEVEL) {
    std::string parallel;
    if (cmSystemTools::GetEnv("CMAKE_BUILD_PARALLEL_LEVEL", parallel)) {
      if (parallel.empty()) {
        jobs = cmake::DEFAULT_BUILD_PARALLEL_LEVEL;
      } else {
        unsigned long numJobs = 0;
        if (cmSystemTools::StringToULong(parallel.c_str(), &numJobs)) {
          jobs = int(numJobs);
        } else {
          std::cerr << "'CMAKE_BUILD_PARALLEL_LEVEL' environment variable\n"
                    << "invalid number '" << parallel << "' given.\n\n";
          dir.clear();
        }
      }
    }
  }

  if (dir.empty()) {
    /* clang-format off */
    std::cerr <<
      "Usage: cmake --build <dir> [options] [-- [native-options]]\n"
      "Options:\n"
      CMAKE_BUILD_OPTIONS
      ;
    /* clang-format on */
    return 1;
  }

  cmake cm(cmake::RoleInternal);
  cmSystemTools::SetMessageCallback(cmakemainMessageCallback, &cm);
  cm.SetProgressCallback(cmakemainProgressCallback, &cm);
  return cm.Build(jobs, dir, target, config, nativeOptions, clean);
#endif
}

static int do_open(int ac, char const* const* av)
{
#ifndef CMAKE_BUILD_WITH_CMAKE
  std::cerr << "This cmake does not support --open\n";
  return -1;
#else
  std::string dir;

  enum Doing
  {
    DoingNone,
    DoingDir,
  };
  Doing doing = DoingDir;
  for (int i = 2; i < ac; ++i) {
    switch (doing) {
      case DoingDir:
        dir = cmSystemTools::CollapseFullPath(av[i]);
        doing = DoingNone;
        break;
      default:
        std::cerr << "Unknown argument " << av[i] << std::endl;
        dir.clear();
        break;
    }
  }
  if (dir.empty()) {
    std::cerr << "Usage: cmake --open <dir>\n";
    return 1;
  }

  cmake cm(cmake::RoleInternal);
  cmSystemTools::SetMessageCallback(cmakemainMessageCallback, &cm);
  cm.SetProgressCallback(cmakemainProgressCallback, &cm);
  return cm.Open(dir, false) ? 0 : 1;
#endif
}
