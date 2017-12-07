/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCommands.h"
#include "cmPolicies.h"
#include "cmState.h"

#include "cmAddCustomCommandCommand.h"
#include "cmAddCustomTargetCommand.h"
#include "cmAddDefinitionsCommand.h"
#include "cmAddDependenciesCommand.h"
#include "cmAddExecutableCommand.h"
#include "cmAddLibraryCommand.h"
#include "cmAddSubDirectoryCommand.h"
#include "cmAddTestCommand.h"
#include "cmBreakCommand.h"
#include "cmBuildCommand.h"
#include "cmCMakeMinimumRequired.h"
#include "cmCMakePolicyCommand.h"
#include "cmConfigureFileCommand.h"
#include "cmContinueCommand.h"
#include "cmCreateTestSourceList.h"
#include "cmDefinePropertyCommand.h"
#include "cmEnableLanguageCommand.h"
#include "cmEnableTestingCommand.h"
#include "cmExecProgramCommand.h"
#include "cmExecuteProcessCommand.h"
#include "cmFileCommand.h"
#include "cmFindFileCommand.h"
#include "cmFindLibraryCommand.h"
#include "cmFindPackageCommand.h"
#include "cmFindPathCommand.h"
#include "cmFindProgramCommand.h"
#include "cmForEachCommand.h"
#include "cmFunctionCommand.h"
#include "cmGetCMakePropertyCommand.h"
#include "cmGetDirectoryPropertyCommand.h"
#include "cmGetFilenameComponentCommand.h"
#include "cmGetPropertyCommand.h"
#include "cmGetSourceFilePropertyCommand.h"
#include "cmGetTargetPropertyCommand.h"
#include "cmGetTestPropertyCommand.h"
#include "cmIfCommand.h"
#include "cmIncludeCommand.h"
#include "cmIncludeDirectoryCommand.h"
#include "cmIncludeRegularExpressionCommand.h"
#include "cmInstallCommand.h"
#include "cmInstallFilesCommand.h"
#include "cmInstallTargetsCommand.h"
#include "cmLinkDirectoriesCommand.h"
#include "cmListCommand.h"
#include "cmMacroCommand.h"
#include "cmMakeDirectoryCommand.h"
#include "cmMarkAsAdvancedCommand.h"
#include "cmMathCommand.h"
#include "cmMessageCommand.h"
#include "cmOptionCommand.h"
#include "cmParseArgumentsCommand.h"
#include "cmProjectCommand.h"
#include "cmReturnCommand.h"
#include "cmSeparateArgumentsCommand.h"
#include "cmSetCommand.h"
#include "cmSetDirectoryPropertiesCommand.h"
#include "cmSetPropertyCommand.h"
#include "cmSetSourceFilesPropertiesCommand.h"
#include "cmSetTargetPropertiesCommand.h"
#include "cmSetTestsPropertiesCommand.h"
#include "cmSiteNameCommand.h"
#include "cmStringCommand.h"
#include "cmSubdirCommand.h"
#include "cmTargetLinkLibrariesCommand.h"
#include "cmTryCompileCommand.h"
#include "cmTryRunCommand.h"
#include "cmUnsetCommand.h"
#include "cmWhileCommand.h"

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmAddCompileOptionsCommand.h"
#include "cmAuxSourceDirectoryCommand.h"
#include "cmBuildNameCommand.h"
#include "cmCMakeHostSystemInformationCommand.h"
#include "cmExportCommand.h"
#include "cmExportLibraryDependenciesCommand.h"
#include "cmFLTKWrapUICommand.h"
#include "cmIncludeExternalMSProjectCommand.h"
#include "cmInstallProgramsCommand.h"
#include "cmLinkLibrariesCommand.h"
#include "cmLoadCacheCommand.h"
#include "cmLoadCommandCommand.h"
#include "cmOutputRequiredFilesCommand.h"
#include "cmQTWrapCPPCommand.h"
#include "cmQTWrapUICommand.h"
#include "cmRemoveCommand.h"
#include "cmRemoveDefinitionsCommand.h"
#include "cmSourceGroupCommand.h"
#include "cmSubdirDependsCommand.h"
#include "cmTargetCompileDefinitionsCommand.h"
#include "cmTargetCompileFeaturesCommand.h"
#include "cmTargetCompileOptionsCommand.h"
#include "cmTargetIncludeDirectoriesCommand.h"
#include "cmTargetSourcesCommand.h"
#include "cmUseMangledMesaCommand.h"
#include "cmUtilitySourceCommand.h"
#include "cmVariableRequiresCommand.h"
#include "cmVariableWatchCommand.h"
#include "cmWriteFileCommand.h"
#endif

void GetScriptingCommands(cmState* state)
{
  state->AddBuiltinCommand("break", new cmBreakCommand);
  state->AddBuiltinCommand("cmake_minimum_required",
                           new cmCMakeMinimumRequired);
  state->AddBuiltinCommand("cmake_policy", new cmCMakePolicyCommand);
  state->AddBuiltinCommand("configure_file", new cmConfigureFileCommand);
  state->AddBuiltinCommand("continue", new cmContinueCommand);
  state->AddBuiltinCommand("exec_program", new cmExecProgramCommand);
  state->AddBuiltinCommand("execute_process", new cmExecuteProcessCommand);
  state->AddBuiltinCommand("file", new cmFileCommand);
  state->AddBuiltinCommand("find_file", new cmFindFileCommand);
  state->AddBuiltinCommand("find_library", new cmFindLibraryCommand);
  state->AddBuiltinCommand("find_package", new cmFindPackageCommand);
  state->AddBuiltinCommand("find_path", new cmFindPathCommand);
  state->AddBuiltinCommand("find_program", new cmFindProgramCommand);
  state->AddBuiltinCommand("foreach", new cmForEachCommand);
  state->AddBuiltinCommand("function", new cmFunctionCommand);
  state->AddBuiltinCommand("get_cmake_property",
                           new cmGetCMakePropertyCommand);
  state->AddBuiltinCommand("get_directory_property",
                           new cmGetDirectoryPropertyCommand);
  state->AddBuiltinCommand("get_filename_component",
                           new cmGetFilenameComponentCommand);
  state->AddBuiltinCommand("get_property", new cmGetPropertyCommand);
  state->AddBuiltinCommand("if", new cmIfCommand);
  state->AddBuiltinCommand("include", new cmIncludeCommand);
  state->AddBuiltinCommand("list", new cmListCommand);
  state->AddBuiltinCommand("macro", new cmMacroCommand);
  state->AddBuiltinCommand("make_directory", new cmMakeDirectoryCommand);
  state->AddBuiltinCommand("mark_as_advanced", new cmMarkAsAdvancedCommand);
  state->AddBuiltinCommand("math", new cmMathCommand);
  state->AddBuiltinCommand("message", new cmMessageCommand);
  state->AddBuiltinCommand("option", new cmOptionCommand);
  state->AddBuiltinCommand("cmake_parse_arguments",
                           new cmParseArgumentsCommand);
  state->AddBuiltinCommand("return", new cmReturnCommand);
  state->AddBuiltinCommand("separate_arguments",
                           new cmSeparateArgumentsCommand);
  state->AddBuiltinCommand("set", new cmSetCommand);
  state->AddBuiltinCommand("set_directory_properties",
                           new cmSetDirectoryPropertiesCommand);
  state->AddBuiltinCommand("set_property", new cmSetPropertyCommand);
  state->AddBuiltinCommand("site_name", new cmSiteNameCommand);
  state->AddBuiltinCommand("string", new cmStringCommand);
  state->AddBuiltinCommand("unset", new cmUnsetCommand);
  state->AddBuiltinCommand("while", new cmWhileCommand);

  state->AddUnexpectedCommand(
    "else", "An ELSE command was found outside of a proper "
            "IF ENDIF structure. Or its arguments did not match "
            "the opening IF command.");
  state->AddUnexpectedCommand(
    "elseif", "An ELSEIF command was found outside of a proper "
              "IF ENDIF structure.");
  state->AddUnexpectedCommand(
    "endforeach", "An ENDFOREACH command was found outside of a proper "
                  "FOREACH ENDFOREACH structure. Or its arguments did "
                  "not match the opening FOREACH command.");
  state->AddUnexpectedCommand(
    "endfunction", "An ENDFUNCTION command was found outside of a proper "
                   "FUNCTION ENDFUNCTION structure. Or its arguments did not "
                   "match the opening FUNCTION command.");
  state->AddUnexpectedCommand(
    "endif", "An ENDIF command was found outside of a proper "
             "IF ENDIF structure. Or its arguments did not match "
             "the opening IF command.");
  state->AddUnexpectedCommand(
    "endmacro", "An ENDMACRO command was found outside of a proper "
                "MACRO ENDMACRO structure. Or its arguments did not "
                "match the opening MACRO command.");
  state->AddUnexpectedCommand(
    "endwhile", "An ENDWHILE command was found outside of a proper "
                "WHILE ENDWHILE structure. Or its arguments did not "
                "match the opening WHILE command.");

#if defined(CMAKE_BUILD_WITH_CMAKE)
  state->AddBuiltinCommand("cmake_host_system_information",
                           new cmCMakeHostSystemInformationCommand);
  state->AddBuiltinCommand("remove", new cmRemoveCommand);
  state->AddBuiltinCommand("variable_watch", new cmVariableWatchCommand);
  state->AddBuiltinCommand("write_file", new cmWriteFileCommand);

  state->AddDisallowedCommand(
    "build_name", new cmBuildNameCommand, cmPolicies::CMP0036,
    "The build_name command should not be called; see CMP0036.");
  state->AddDisallowedCommand(
    "use_mangled_mesa", new cmUseMangledMesaCommand, cmPolicies::CMP0030,
    "The use_mangled_mesa command should not be called; see CMP0030.");

#endif
}

void GetProjectCommands(cmState* state)
{
  state->AddBuiltinCommand("add_custom_command",
                           new cmAddCustomCommandCommand);
  state->AddBuiltinCommand("add_custom_target", new cmAddCustomTargetCommand);
  state->AddBuiltinCommand("add_definitions", new cmAddDefinitionsCommand);
  state->AddBuiltinCommand("add_dependencies", new cmAddDependenciesCommand);
  state->AddBuiltinCommand("add_executable", new cmAddExecutableCommand);
  state->AddBuiltinCommand("add_library", new cmAddLibraryCommand);
  state->AddBuiltinCommand("add_subdirectory", new cmAddSubDirectoryCommand);
  state->AddBuiltinCommand("add_test", new cmAddTestCommand);
  state->AddBuiltinCommand("build_command", new cmBuildCommand);
  state->AddBuiltinCommand("create_test_sourcelist",
                           new cmCreateTestSourceList);
  state->AddBuiltinCommand("define_property", new cmDefinePropertyCommand);
  state->AddBuiltinCommand("enable_language", new cmEnableLanguageCommand);
  state->AddBuiltinCommand("enable_testing", new cmEnableTestingCommand);
  state->AddBuiltinCommand("get_source_file_property",
                           new cmGetSourceFilePropertyCommand);
  state->AddBuiltinCommand("get_target_property",
                           new cmGetTargetPropertyCommand);
  state->AddBuiltinCommand("get_test_property", new cmGetTestPropertyCommand);
  state->AddBuiltinCommand("include_directories",
                           new cmIncludeDirectoryCommand);
  state->AddBuiltinCommand("include_regular_expression",
                           new cmIncludeRegularExpressionCommand);
  state->AddBuiltinCommand("install", new cmInstallCommand);
  state->AddBuiltinCommand("install_files", new cmInstallFilesCommand);
  state->AddBuiltinCommand("install_targets", new cmInstallTargetsCommand);
  state->AddBuiltinCommand("link_directories", new cmLinkDirectoriesCommand);
  state->AddBuiltinCommand("project", new cmProjectCommand);
  state->AddBuiltinCommand("set_source_files_properties",
                           new cmSetSourceFilesPropertiesCommand);
  state->AddBuiltinCommand("set_target_properties",
                           new cmSetTargetPropertiesCommand);
  state->AddBuiltinCommand("set_tests_properties",
                           new cmSetTestsPropertiesCommand);
  state->AddBuiltinCommand("subdirs", new cmSubdirCommand);
  state->AddBuiltinCommand("target_link_libraries",
                           new cmTargetLinkLibrariesCommand);
  state->AddBuiltinCommand("try_compile", new cmTryCompileCommand);
  state->AddBuiltinCommand("try_run", new cmTryRunCommand);

#if defined(CMAKE_BUILD_WITH_CMAKE)
  state->AddBuiltinCommand("add_compile_options",
                           new cmAddCompileOptionsCommand);
  state->AddBuiltinCommand("aux_source_directory",
                           new cmAuxSourceDirectoryCommand);
  state->AddBuiltinCommand("export", new cmExportCommand);
  state->AddBuiltinCommand("fltk_wrap_ui", new cmFLTKWrapUICommand);
  state->AddBuiltinCommand("include_external_msproject",
                           new cmIncludeExternalMSProjectCommand);
  state->AddBuiltinCommand("install_programs", new cmInstallProgramsCommand);
  state->AddBuiltinCommand("link_libraries", new cmLinkLibrariesCommand);
  state->AddBuiltinCommand("load_cache", new cmLoadCacheCommand);
  state->AddBuiltinCommand("qt_wrap_cpp", new cmQTWrapCPPCommand);
  state->AddBuiltinCommand("qt_wrap_ui", new cmQTWrapUICommand);
  state->AddBuiltinCommand("remove_definitions",
                           new cmRemoveDefinitionsCommand);
  state->AddBuiltinCommand("source_group", new cmSourceGroupCommand);
  state->AddBuiltinCommand("target_compile_definitions",
                           new cmTargetCompileDefinitionsCommand);
  state->AddBuiltinCommand("target_compile_features",
                           new cmTargetCompileFeaturesCommand);
  state->AddBuiltinCommand("target_compile_options",
                           new cmTargetCompileOptionsCommand);
  state->AddBuiltinCommand("target_include_directories",
                           new cmTargetIncludeDirectoriesCommand);
  state->AddBuiltinCommand("target_sources", new cmTargetSourcesCommand);

  state->AddDisallowedCommand(
    "export_library_dependencies", new cmExportLibraryDependenciesCommand,
    cmPolicies::CMP0033,
    "The export_library_dependencies command should not be called; "
    "see CMP0033.");
  state->AddDisallowedCommand(
    "load_command", new cmLoadCommandCommand, cmPolicies::CMP0031,
    "The load_command command should not be called; see CMP0031.");
  state->AddDisallowedCommand(
    "output_required_files", new cmOutputRequiredFilesCommand,
    cmPolicies::CMP0032,
    "The output_required_files command should not be called; see CMP0032.");
  state->AddDisallowedCommand(
    "subdir_depends", new cmSubdirDependsCommand, cmPolicies::CMP0029,
    "The subdir_depends command should not be called; see CMP0029.");
  state->AddDisallowedCommand(
    "utility_source", new cmUtilitySourceCommand, cmPolicies::CMP0034,
    "The utility_source command should not be called; see CMP0034.");
  state->AddDisallowedCommand(
    "variable_requires", new cmVariableRequiresCommand, cmPolicies::CMP0035,
    "The variable_requires command should not be called; see CMP0035.");
#endif
}

void GetProjectCommandsInScriptMode(cmState* state)
{
#define CM_UNEXPECTED_PROJECT_COMMAND(NAME)                                   \
  state->AddUnexpectedCommand(NAME, "command is not scriptable")

  CM_UNEXPECTED_PROJECT_COMMAND("add_compile_options");
  CM_UNEXPECTED_PROJECT_COMMAND("add_custom_command");
  CM_UNEXPECTED_PROJECT_COMMAND("add_custom_target");
  CM_UNEXPECTED_PROJECT_COMMAND("add_definitions");
  CM_UNEXPECTED_PROJECT_COMMAND("add_dependencies");
  CM_UNEXPECTED_PROJECT_COMMAND("add_executable");
  CM_UNEXPECTED_PROJECT_COMMAND("add_library");
  CM_UNEXPECTED_PROJECT_COMMAND("add_subdirectory");
  CM_UNEXPECTED_PROJECT_COMMAND("add_test");
  CM_UNEXPECTED_PROJECT_COMMAND("aux_source_directory");
  CM_UNEXPECTED_PROJECT_COMMAND("build_command");
  CM_UNEXPECTED_PROJECT_COMMAND("create_test_sourcelist");
  CM_UNEXPECTED_PROJECT_COMMAND("define_property");
  CM_UNEXPECTED_PROJECT_COMMAND("enable_language");
  CM_UNEXPECTED_PROJECT_COMMAND("enable_testing");
  CM_UNEXPECTED_PROJECT_COMMAND("export");
  CM_UNEXPECTED_PROJECT_COMMAND("fltk_wrap_ui");
  CM_UNEXPECTED_PROJECT_COMMAND("get_source_file_property");
  CM_UNEXPECTED_PROJECT_COMMAND("get_target_property");
  CM_UNEXPECTED_PROJECT_COMMAND("get_test_property");
  CM_UNEXPECTED_PROJECT_COMMAND("include_directories");
  CM_UNEXPECTED_PROJECT_COMMAND("include_external_msproject");
  CM_UNEXPECTED_PROJECT_COMMAND("include_regular_expression");
  CM_UNEXPECTED_PROJECT_COMMAND("install");
  CM_UNEXPECTED_PROJECT_COMMAND("link_directories");
  CM_UNEXPECTED_PROJECT_COMMAND("link_libraries");
  CM_UNEXPECTED_PROJECT_COMMAND("load_cache");
  CM_UNEXPECTED_PROJECT_COMMAND("project");
  CM_UNEXPECTED_PROJECT_COMMAND("qt_wrap_cpp");
  CM_UNEXPECTED_PROJECT_COMMAND("qt_wrap_ui");
  CM_UNEXPECTED_PROJECT_COMMAND("remove_definitions");
  CM_UNEXPECTED_PROJECT_COMMAND("set_source_files_properties");
  CM_UNEXPECTED_PROJECT_COMMAND("set_target_properties");
  CM_UNEXPECTED_PROJECT_COMMAND("set_tests_properties");
  CM_UNEXPECTED_PROJECT_COMMAND("source_group");
  CM_UNEXPECTED_PROJECT_COMMAND("target_compile_definitions");
  CM_UNEXPECTED_PROJECT_COMMAND("target_compile_features");
  CM_UNEXPECTED_PROJECT_COMMAND("target_compile_options");
  CM_UNEXPECTED_PROJECT_COMMAND("target_include_directories");
  CM_UNEXPECTED_PROJECT_COMMAND("target_link_libraries");
  CM_UNEXPECTED_PROJECT_COMMAND("target_sources");
  CM_UNEXPECTED_PROJECT_COMMAND("try_compile");
  CM_UNEXPECTED_PROJECT_COMMAND("try_run");

  // deprected commands
  CM_UNEXPECTED_PROJECT_COMMAND("export_library_dependencies");
  CM_UNEXPECTED_PROJECT_COMMAND("load_command");
  CM_UNEXPECTED_PROJECT_COMMAND("output_required_files");
  CM_UNEXPECTED_PROJECT_COMMAND("subdir_depends");
  CM_UNEXPECTED_PROJECT_COMMAND("utility_source");
  CM_UNEXPECTED_PROJECT_COMMAND("variable_requires");

#undef CM_UNEXPECTED_PROJECT_COMMAND
}
