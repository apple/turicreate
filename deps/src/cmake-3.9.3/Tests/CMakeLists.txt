# a macro for tests that have a simple format where the name matches the
# directory and project
macro(ADD_TEST_MACRO NAME)
  if(${ARGC} GREATER 1)
    set(_test_command --test-command ${ARGN})
  endif()
  string(REPLACE "." "/" dir "${NAME}")
  string(REGEX REPLACE "[^.]*\\." "" proj "${NAME}")
  add_test(NAME "${NAME}" COMMAND "${CMAKE_CTEST_COMMAND}"
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/${dir}"
    "${CMake_BINARY_DIR}/Tests/${dir}"
    --build-two-config
    ${build_generator_args}
    --build-project ${proj}
    ${${NAME}_CTEST_OPTIONS}
    --build-options ${build_options}
    ${${NAME}_BUILD_OPTIONS}
    ${_test_command})
  unset(_test_command)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${dir}")
endmacro()

include(${CMAKE_CURRENT_SOURCE_DIR}/CheckFortran.cmake)

# Fake a user home directory to avoid polluting the real one.
if(DEFINED ENV{HOME} AND NOT CTEST_NO_TEST_HOME)
  set(TEST_HOME "${CMake_BINARY_DIR}/Tests/CMakeFiles/TestHome")
  file(MAKE_DIRECTORY "${TEST_HOME}")
  file(WRITE "${TEST_HOME}/.cvspass" ":pserver:anoncvs@www.cmake.org:/cvsroot/KWSys A\n")
  set(TEST_HOME_ENV_CODE "# Fake a user home directory to avoid polluting the real one.
# But provide original ENV{HOME} value in ENV{CTEST_REAL_HOME} for tests that
# need access to the real HOME directory.
set(ENV{CTEST_REAL_HOME} \"\$ENV{HOME}\")
set(ENV{HOME} \"${TEST_HOME}\")
")
endif()

# Choose a default configuration for CTest tests.
set(CTestTest_CONFIG Debug)
if(NOT CMAKE_CONFIGURATION_TYPES AND CMAKE_BUILD_TYPE)
  set(CTestTest_CONFIG ${CMAKE_BUILD_TYPE})
endif()

configure_file(${CMake_SOURCE_DIR}/Tests/EnforceConfig.cmake.in
               ${CMake_BINARY_DIR}/Tests/EnforceConfig.cmake @ONLY)

# Testing
if(BUILD_TESTING)
  set(CMake_TEST_DEVENV "")
  if(CMAKE_VS_DEVENV_COMMAND)
    set(CMake_TEST_DEVENV "${CMAKE_VS_DEVENV_COMMAND}")
  elseif(CMAKE_GENERATOR MATCHES "Visual Studio [89] " AND
      NOT CMAKE_MAKE_PROGRAM MATCHES "[mM][sS][bB][uU][iI][lL][dD]\\.[eE][xX][eE]")
    set(CMake_TEST_DEVENV "${CMAKE_MAKE_PROGRAM}")
  endif()

  if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
    set(CMake_TEST_EXPLICIT_MAKE_PROGRAM "")
  else()
    set(CMake_TEST_EXPLICIT_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM}")
  endif()

  if(NOT CMake_TEST_EXTERNAL_CMAKE)
    if("${CMAKE_GENERATOR}" MATCHES "Unix Makefiles" OR ("${CMAKE_GENERATOR}" MATCHES Ninja AND NOT WIN32))
      set(TEST_CompileCommandOutput 1)
    endif()
  endif()

  set(MAKE_IS_GNU )
  if(CMAKE_MAKE_PROGRAM MATCHES make)
    execute_process(COMMAND ${CMAKE_MAKE_PROGRAM} no_such_target --version
      RESULT_VARIABLE res OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if("${res}" STREQUAL "0")
      if("${out}" MATCHES "GNU")
        set(MAKE_IS_GNU 1)
      endif()
    endif()
  endif()

  # some old versions of make simply cannot handle spaces in paths
  if (MAKE_IS_GNU OR
      CMAKE_MAKE_PROGRAM MATCHES "nmake|gmake|wmake" OR
      CMAKE_GENERATOR MATCHES "Visual Studio|Xcode|Borland")
    set(MAKE_SUPPORTS_SPACES 1)
  else()
    set(MAKE_SUPPORTS_SPACES 0)
  endif()

  # assume no resources building to test
  set(CMake_TEST_RESOURCES FALSE)
  # for windows and cygwin assume we have resources
  if(WIN32 OR CYGWIN)
    set(CMake_TEST_RESOURCES TRUE)
  endif()
  # for borland and watcom there is no resource support
  if(WATCOM OR BORLAND)
    set(CMake_TEST_RESOURCES FALSE)
  endif()

  set(build_generator_args
    --build-generator ${CMAKE_GENERATOR}
    )
  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND build_generator_args
      --build-generator-platform ${CMAKE_GENERATOR_PLATFORM}
      )
  endif()
  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND build_generator_args
      --build-generator-toolset ${CMAKE_GENERATOR_TOOLSET}
      )
  endif()

  set(build_options)
  if(CMake_TEST_EXPLICIT_MAKE_PROGRAM)
    list(APPEND build_options -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM})
  endif()

  # Look for rpmbuild to use for tests.
  # The tool does not work with spaces in the path.
  if(NOT CMAKE_CURRENT_BINARY_DIR MATCHES " ")
    find_program(RPMBUILD_EXECUTABLE NAMES rpmbuild)
  else()
    set(RPMBUILD_EXECUTABLE "RPMBUILD_EXECUTABLE-NOTFOUND")
  endif()

  if(RPMBUILD_EXECUTABLE)
    set(CPACK_BINARY_RPM ON)
  else()
    set(CPACK_BINARY_RPM OFF)
  endif()

  # Look for rpmbuild to use for tests.
  # The tool does not work with spaces in the path.
  find_program(DPKG_EXECUTABLE NAMES dpkg)

  if(DPKG_EXECUTABLE)
    set(CPACK_BINARY_DEB ON)
  else()
    set(CPACK_BINARY_DEB OFF)
  endif()

  #---------------------------------------------------------------------------
  # Add tests below here.

  if(NOT CMake_TEST_EXTERNAL_CMAKE)
    add_subdirectory(CMakeLib)
  endif()
  add_subdirectory(CMakeOnly)
  add_subdirectory(RunCMake)

  add_subdirectory(FindPackageModeMakefileTest)

  # Collect a list of all test build directories.
  set(TEST_BUILD_DIRS)

  # Should the long tests be run?
  option(CMAKE_RUN_LONG_TESTS
    "Should the long tests be run (such as Bootstrap)." ON)
  mark_as_advanced(CMAKE_RUN_LONG_TESTS)

  if (CMAKE_RUN_LONG_TESTS)
    option(CTEST_TEST_CTEST
      "Should the tests that run a full sub ctest process be run?"
      OFF)
    mark_as_advanced(CTEST_TEST_CTEST)
  endif ()

  # Should tests that use CVS be run?
  #
  set(do_cvs_tests 0)

  if(EXISTS ${CMAKE_ROOT}/Modules/FindCVS.cmake)
    find_package(CVS QUIET)
  else()
    find_program(CVS_EXECUTABLE NAMES cvs)
  endif()

  if(CVS_EXECUTABLE)
    set(do_cvs_tests 1)
  endif()

  if(do_cvs_tests AND NOT UNIX)
    if("${CVS_EXECUTABLE}" MATCHES "cygwin")
      set(do_cvs_tests 0)
    endif()
  endif()

  # Should CPack tests be run? By default, yes, but...
  #
  # Disable packaging test on Apple 10.3 and below. PackageMaker starts
  # DiskManagementTool as root and disowns it
  # (http://lists.apple.com/archives/installer-dev/2005/Jul/msg00005.html).
  # It is left holding open pipe handles and preventing ProcessUNIX from
  # detecting end-of-data even after its immediate child exits. Then
  # the test hangs until it times out and is killed. This is a
  # well-known bug in kwsys process execution that I would love to get
  # time to fix.
  #
  option(CTEST_TEST_CPACK
    "Should the tests that use '--build-target package' be run?"
    ON)
  mark_as_advanced(CTEST_TEST_CPACK)
  set(CTEST_TEST_OSX_ARCH 0)
  set(CMake_TEST_XCODE_VERSION 0)
  if(APPLE)
    execute_process(
      COMMAND sw_vers -productVersion
      OUTPUT_VARIABLE OSX_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    if(OSX_VERSION VERSION_LESS 10.4)
      message(STATUS "Forcing CTEST_TEST_CPACK=OFF on OSX < 10.4")
      message(STATUS "OSX_VERSION='${OSX_VERSION}'")
      set(CTEST_TEST_CPACK OFF)
    else()
      set(CTEST_TEST_OSX_ARCH 1)
    endif()
    if(XCODE_VERSION)
      set(CMake_TEST_XCODE_VERSION "${XCODE_VERSION}")
    else()
      execute_process(
        COMMAND xcodebuild -version
        OUTPUT_VARIABLE _version ERROR_VARIABLE _version
        )
      if(_version MATCHES "^Xcode ([0-9]+(\\.[0-9]+)*)")
        set(CMake_TEST_XCODE_VERSION "${CMAKE_MATCH_1}")
      endif()
    endif()
    if(CMAKE_OSX_SYSROOT)
      execute_process(
        COMMAND xcodebuild -sdk ${CMAKE_OSX_SYSROOT} -version ProductName
        OUTPUT_VARIABLE _stdout
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE _stderr
        RESULT_VARIABLE _failed
        )
      if(NOT _failed)
        set(CMAKE_OSX_SDKPRODUCT "${_stdout}")
      endif()

      execute_process(
        COMMAND xcodebuild -sdk ${CMAKE_OSX_SYSROOT} -version SDKVersion
        OUTPUT_VARIABLE _stdout
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE _stderr
        RESULT_VARIABLE _failed
        )
      if(NOT _failed)
        set(CMAKE_OSX_SDKVERSION "${_stdout}")
      endif()
    endif()
  endif()

  # Use 1500 or CTEST_TEST_TIMEOUT for long test timeout value,
  # whichever is greater.
  set(CMAKE_LONG_TEST_TIMEOUT 1500)
  if(CTEST_TEST_TIMEOUT)
    set(CMAKE_LONG_TEST_TIMEOUT ${CTEST_TEST_TIMEOUT})
  endif()
  if(CMAKE_LONG_TEST_TIMEOUT LESS 1500)
    set(CMAKE_LONG_TEST_TIMEOUT 1500)
  endif()

  add_test(NAME CMake.Copyright
    COMMAND cmake -P ${CMAKE_CURRENT_SOURCE_DIR}/CMakeCopyright.cmake)

  # add a bunch of standard build-and-test style tests
  ADD_TEST_MACRO(CommandLineTest CommandLineTest)
  ADD_TEST_MACRO(FindPackageTest FindPackageTest)
  ADD_TEST_MACRO(FindModulesExecuteAll FindModulesExecuteAll)
  ADD_TEST_MACRO(StringFileTest StringFileTest)
  ADD_TEST_MACRO(TryCompile TryCompile)
  ADD_TEST_MACRO(SystemInformation SystemInformation)
  ADD_TEST_MACRO(MathTest MathTest)
  ADD_TEST_MACRO(CompileFeatures CompileFeatures)
  ADD_TEST_MACRO(CMakeCommands.target_compile_features)

  if(CMake_TEST_RESOURCES)
    ADD_TEST_MACRO(VSResource VSResource)
    if (CMAKE_GENERATOR MATCHES "Ninja")
      add_test_macro(VSResourceNinjaForceRSP VSResourceNinjaForceRSP)
    endif ()
  endif()
  ADD_TEST_MACRO(MSManifest MSManifest)
  ADD_TEST_MACRO(Simple Simple)
  ADD_TEST_MACRO(PreOrder PreOrder)
  ADD_TEST_MACRO(MissingSourceFile MissingSourceFile)
  set_tests_properties(MissingSourceFile PROPERTIES
    PASS_REGULAR_EXPRESSION "CMake Error at CMakeLists.txt:3 \\(add_executable\\):[ \r\n]*Cannot find source file:[ \r\n]*DoesNotExist/MissingSourceFile.c")
  if(CMake_TEST_XCODE_VERSION AND CMAKE_OSX_SDKVERSION AND CMAKE_OSX_SDKPRODUCT)
    if((NOT CMake_TEST_XCODE_VERSION VERSION_LESS 6.1) AND
       ((NOT CMAKE_OSX_SDKPRODUCT STREQUAL "Mac OS X") OR
        (NOT CMAKE_OSX_SDKVERSION VERSION_LESS 10.10)))
      if(CMAKE_GENERATOR STREQUAL "Xcode")
        ADD_TEST_MACRO(SwiftMix SwiftMix)
        ADD_TEST_MACRO(SwiftOnly SwiftOnly)
      endif()
    endif()
  endif()
  if(CMAKE_Fortran_COMPILER)
    ADD_TEST_MACRO(FortranOnly FortranOnly)
  endif()
  # test Visual Studio GNU Fortran mixing with cmake_add_fortran_subdirectory
  # run this project if we have a working fortran compiler or
  # the test is enabled with CMAKE_TEST_CMAKE_ADD_FORTRAN cache variable.
  # If you enable the test, CMake should find the MinGW fortran install,
  # or in some cases you might need to set the PATH so that cmake can find
  # the gfortran from mingw.
  if(CMAKE_Fortran_COMPILER OR CMAKE_TEST_CMAKE_ADD_FORTRAN)
    set(CMAKE_SKIP_VSGNUFortran FALSE)
    # disable test for apple builds using ifort if they are building
    # more than one architecture, as ifort does not support that.
    if(APPLE AND (CMAKE_Fortran_COMPILER MATCHES ifort))
      list(LENGTH CMAKE_OSX_ARCHITECTURES len)
      if("${len}" GREATER 1)
        message(STATUS "Skip VSGNUFortran for ifort dual cpu mac build")
        set(CMAKE_SKIP_VSGNUFortran TRUE)
      endif()
    endif()
    if((CMAKE_C_COMPILER MATCHES lsb)
        AND (CMAKE_Fortran_COMPILER MATCHES ifort))
      message(STATUS "Skip VSGNUFortran for ifort and lsb compilers")
      set(CMAKE_SKIP_VSGNUFortran TRUE)
    endif()
    if(NOT CMAKE_SKIP_VSGNUFortran)
      ADD_TEST_MACRO(VSGNUFortran ${CMAKE_COMMAND} -P runtest.cmake)
    endif()
  endif()

  if(${CMAKE_GENERATOR} MATCHES "Visual Studio ([^89]|[89][0-9])")
    ADD_TEST_MACRO(CSharpOnly CSharpOnly)
    ADD_TEST_MACRO(CSharpLinkToCxx CSharpLinkToCxx)
  endif()

  ADD_TEST_MACRO(COnly COnly)
  ADD_TEST_MACRO(CxxOnly CxxOnly)
  ADD_TEST_MACRO(CxxSubdirC CxxSubdirC)
  ADD_TEST_MACRO(IPO COnly/COnly)
  ADD_TEST_MACRO(OutDir runtime/OutDir)
  ADD_TEST_MACRO(ObjectLibrary UseCshared)
  ADD_TEST_MACRO(NewlineArgs NewlineArgs)
  ADD_TEST_MACRO(SetLang SetLang)
  ADD_TEST_MACRO(EmptyProperty EmptyProperty)
  ADD_TEST_MACRO(ExternalOBJ ExternalOBJ)
  if(NOT CMake_TEST_EXTERNAL_CMAKE)
    ADD_TEST_MACRO(LoadCommand LoadedCommand)
  endif()
  ADD_TEST_MACRO(LinkDirectory bin/LinkDirectory)
  ADD_TEST_MACRO(LinkLanguage LinkLanguage)
  ADD_TEST_MACRO(LinkLine LinkLine)
  ADD_TEST_MACRO(MacroTest miniMacroTest)
  ADD_TEST_MACRO(FunctionTest miniFunctionTest)
  ADD_TEST_MACRO(ReturnTest ReturnTest)
  ADD_TEST_MACRO(Properties Properties)
  ADD_TEST_MACRO(Assembler HelloAsm)
  ADD_TEST_MACRO(SourceGroups SourceGroups)
  ADD_TEST_MACRO(Preprocess Preprocess)
  set(ExportImport_BUILD_OPTIONS -DCMake_TEST_NESTED_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM})
  ADD_TEST_MACRO(ExportImport ExportImport)
  ADD_TEST_MACRO(Unset Unset)
  ADD_TEST_MACRO(PolicyScope PolicyScope)
  ADD_TEST_MACRO(EmptyLibrary EmptyLibrary)
  ADD_TEST_MACRO(CompileDefinitions CompileDefinitions)
  ADD_TEST_MACRO(CompileOptions CompileOptions)
  ADD_TEST_MACRO(CompatibleInterface CompatibleInterface)
  ADD_TEST_MACRO(AliasTarget AliasTarget)
  ADD_TEST_MACRO(StagingPrefix StagingPrefix)
  ADD_TEST_MACRO(InterfaceLibrary InterfaceLibrary)
  if (CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    set(ConfigSources_BUILD_OPTIONS -DCMAKE_BUILD_TYPE=Debug)
    ADD_TEST_MACRO(ConfigSources ConfigSources)
  endif()
  ADD_TEST_MACRO(SourcesProperty SourcesProperty)
  ADD_TEST_MACRO(SourceFileProperty SourceFileProperty)
  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU
      AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
    set(runCxxDialectTest 1)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang
        AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
    if(NOT APPLE OR POLICY CMP0025)
      set(runCxxDialectTest 1)
    endif()
  endif()
  if(runCxxDialectTest)
    ADD_TEST_MACRO(CxxDialect CxxDialect)
  endif()
  set_tests_properties(EmptyLibrary PROPERTIES
    PASS_REGULAR_EXPRESSION "CMake Error: CMake can not determine linker language for target: test")
  ADD_TEST_MACRO(CrossCompile CrossCompile)
  set_tests_properties(CrossCompile PROPERTIES
    PASS_REGULAR_EXPRESSION "TRY_RUN.. invoked in cross-compiling mode")
  if("${CMAKE_GENERATOR}" MATCHES "Make")
    ADD_TEST_MACRO(Policy0002 Policy0002)
  endif()
  if(CTEST_TEST_OSX_ARCH)
    ADD_TEST_MACRO(Architecture Architecture)
    set_tests_properties(Architecture PROPERTIES
      PASS_REGULAR_EXPRESSION "(file is not of required architecture|does not match cputype|not the architecture being linked)")
  endif()

  list(APPEND TEST_BUILD_DIRS ${CMake_TEST_INSTALL_PREFIX})

  if(NOT QT4_FOUND)
    find_package(Qt4 QUIET)
  endif()

  if(QT4_FOUND)
    # test whether the Qt4 which has been found works, on some machines
    # which run nightly builds there were errors like "wrong file format"
    # for libQtCore.so. So first check it works, and only if it does add
    # the automoc test.
    include(CheckCXXSourceCompiles)
    set(_save_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
    set(_save_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")

    set(CMAKE_REQUIRED_INCLUDES ${QT_INCLUDES})
    set(CMAKE_REQUIRED_LIBRARIES ${QT_QTCORE_LIBRARIES})

    CHECK_CXX_SOURCE_COMPILES("#include <QCoreApplication>\n int main() {return (qApp == 0 ? 0 : 1); }\n"
                              QT4_WORKS)

    set(CMAKE_REQUIRED_INCLUDES "${_save_CMAKE_REQUIRED_INCLUDES}")
    set(CMAKE_REQUIRED_LIBRARIES "${_save_CMAKE_REQUIRED_LIBRARIES}")
  endif()

  # run test for BundleUtilities on supported platforms/compilers
  if(MSVC OR
     MINGW OR
     CMAKE_SYSTEM_NAME MATCHES "Linux" OR
     CMAKE_SYSTEM_NAME MATCHES "Darwin")
  if(NOT "${CMAKE_GENERATOR}" STREQUAL "Watcom WMake")

    add_test(BundleUtilities ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/BundleUtilities"
      "${CMake_BINARY_DIR}/Tests/BundleUtilities"
      ${build_generator_args}
      --build-project BundleUtilities
      --build-options ${build_options}
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/BundleUtilities")

    # run test for DeployQt4 on supported platforms/compilers (which depends on BundleUtilities)
    # this test also depends on the existence of the standard qtiff plugin
    if(QT4_WORKS AND QT_QTSQL_FOUND)
      add_test(Qt4Deploy ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/Qt4Deploy"
        "${CMake_BINARY_DIR}/Tests/Qt4Deploy"
        ${build_generator_args}
        --build-project Qt4Deploy
        --build-options ${build_options}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4Deploy")
    endif()

  endif()
  endif()

  set(CMAKE_BUILD_TEST_SOURCE_DIR "${CMake_SOURCE_DIR}/Tests/COnly")
  set(CMAKE_BUILD_TEST_BINARY_DIR "${CMake_BINARY_DIR}/Tests/CMakeBuildCOnly")
  set(CMAKE_BUILD_TEST_EXE COnly)
  configure_file("${CMake_SOURCE_DIR}/Tests/CMakeBuildTest.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CMakeBuildTest.cmake" @ONLY)
  add_test(CMakeBuildTest ${CMAKE_CMAKE_COMMAND} -P
    "${CMake_BINARY_DIR}/Tests/CMakeBuildTest.cmake")
  list(APPEND TEST_BUILD_DIRS ${CMAKE_BUILD_TEST_BINARY_DIR})
  # now do it again for a project that has two project commands
  set(CMAKE_BUILD_TEST_SOURCE_DIR "${CMake_SOURCE_DIR}/Tests/DoubleProject")
  set(CMAKE_BUILD_TEST_BINARY_DIR "${CMake_BINARY_DIR}/Tests/DoubleProject")
  set(CMAKE_BUILD_TEST_EXE just_silly)
  configure_file("${CMake_SOURCE_DIR}/Tests/CMakeBuildTest.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CMakeBuildDoubleProjectTest.cmake" @ONLY)
  add_test(CMakeDoubleProject ${CMAKE_CMAKE_COMMAND} -P
    "${CMake_BINARY_DIR}/Tests/CMakeBuildDoubleProjectTest.cmake")
  list(APPEND TEST_BUILD_DIRS ${CMAKE_BUILD_TEST_BINARY_DIR})

  ADD_TEST_MACRO(Module.CheckTypeSize CheckTypeSize)

  set(Module.CheckIPOSupported-C_BUILD_OPTIONS -DCMake_TEST_IPO_WORKS_C=${CMake_TEST_IPO_WORKS_C})
  ADD_TEST_MACRO(Module.CheckIPOSupported-C CheckIPOSupported-C)

  set(Module.CheckIPOSupported-CXX_BUILD_OPTIONS -DCMake_TEST_IPO_WORKS_CXX=${CMake_TEST_IPO_WORKS_CXX})
  ADD_TEST_MACRO(Module.CheckIPOSupported-CXX CheckIPOSupported-CXX)

  if(CMAKE_Fortran_COMPILER)
    set(Module.CheckIPOSupported-Fortran_BUILD_OPTIONS -DCMake_TEST_IPO_WORKS_Fortran=${CMake_TEST_IPO_WORKS_Fortran})
    ADD_TEST_MACRO(Module.CheckIPOSupported-Fortran CheckIPOSupported-Fortran)
  endif()

  add_test(Module.ExternalData ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Module/ExternalData"
    "${CMake_BINARY_DIR}/Tests/Module/ExternalData"
    ${build_generator_args}
    --build-project ExternalDataTest
    --build-noclean
    --force-new-ctest-process
    --build-options ${build_options}
      -DMAKE_SUPPORTS_SPACES=${MAKE_SUPPORTS_SPACES}
    --test-command ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Module/ExternalData")

  ADD_TEST_MACRO(Module.FindDependency FindDependency)

  ADD_TEST_MACRO(Module.WriteCompilerDetectionHeader WriteCompilerDetectionHeader)

  if (APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fPIE run_pic_test)
  else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "PGI"
        OR CMAKE_CXX_COMPILER_ID MATCHES "PathScale"
        OR CMAKE_SYSTEM_NAME MATCHES "IRIX64"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
      set(run_pic_test 0)
    else()
      set(run_pic_test 1)
    endif()
  endif()

  if (run_pic_test)
    ADD_TEST_MACRO(PositionIndependentTargets PositionIndependentTargets)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND
    (NOT "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 4.2) AND
    (CMAKE_SYSTEM_NAME MATCHES "Linux"))

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(
      -fvisibility-inlines-hidden run_inlines_hidden_test)
  endif()

  if(run_inlines_hidden_test)
    add_test(Visibility ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Visibility"
      "${CMake_BINARY_DIR}/Tests/Visibility"
      ${build_generator_args}
      --build-project Visibility
      --build-options ${build_options}
    )
    list(APPEND TEST_BUILD_DIRS
      "${CMake_BINARY_DIR}/Tests/Visibility"
    )
  endif()

  add_test(LinkFlags-prepare
    ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/LinkFlags"
    "${CMake_BINARY_DIR}/Tests/LinkFlags"
    ${build_generator_args}
    --build-project LinkFlags
    --build-target LinkFlags
    --build-options ${build_options}
      -DTEST_CONFIG=\${CTEST_CONFIGURATION_TYPE}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/LinkFlags")

  macro(ADD_LINK_FLAGS_TEST name depends)
    add_test(LinkFlags-${name}
      ${CMAKE_CMAKE_COMMAND} --build "${CMake_BINARY_DIR}/Tests/LinkFlags"
      --target LinkFlags_${name} --config \${CTEST_CONFIGURATION_TYPE}
      )
    set_tests_properties(LinkFlags-${name} PROPERTIES
      PASS_REGULAR_EXPRESSION "BADFLAG" DEPENDS LinkFlags-${depends})
  endmacro()
  ADD_LINK_FLAGS_TEST(lib prepare)
  ADD_LINK_FLAGS_TEST(dll lib)
  ADD_LINK_FLAGS_TEST(mod dll)
  ADD_LINK_FLAGS_TEST(exe mod)
  ADD_LINK_FLAGS_TEST(lib_config exe)
  ADD_LINK_FLAGS_TEST(dll_config lib_config)
  ADD_LINK_FLAGS_TEST(mod_config dll_config)
  ADD_LINK_FLAGS_TEST(exe_config mod_config)
  ADD_LINK_FLAGS_TEST(lib_flags exe_config)
  ADD_LINK_FLAGS_TEST(dll_flags lib_flags)
  ADD_LINK_FLAGS_TEST(mod_flags dll_flags)
  ADD_LINK_FLAGS_TEST(exe_flags mod_flags)
  ADD_LINK_FLAGS_TEST(lib_flags_config exe_flags)
  ADD_LINK_FLAGS_TEST(dll_flags_config lib_flags_config)
  ADD_LINK_FLAGS_TEST(mod_flags_config dll_flags_config)
  ADD_LINK_FLAGS_TEST(exe_flags_config mod_flags_config)

  # If we are running right now with a Unix Makefiles or Ninja based generator,
  # build the "Simple" test with the ExtraGenerators, if available
  # This doesn't test whether the generated project files work (unfortunately),
  # mainly it tests that cmake doesn't crash when generating these project files.
  if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles" OR ${CMAKE_GENERATOR} MATCHES "Ninja")

    # check which generators we have
    execute_process(COMMAND ${CMAKE_CMAKE_COMMAND} --help
      OUTPUT_VARIABLE cmakeOutput ERROR_VARIABLE cmakeOutput)

    set(extraGenerators
      "CodeBlocks"
      "CodeLite"
      "Eclipse CDT4"
      "Kate"
      "KDevelop3"
      "Sublime Text 2")

    foreach(extraGenerator ${extraGenerators})
      if ("${cmakeOutput}" MATCHES "${extraGenerator} - ${CMAKE_GENERATOR}")
        set(extraGeneratorTestName "Simple_${extraGenerator}Generator")
        string(REPLACE " " "" extraGeneratorTestName ${extraGeneratorTestName})

        add_test(${extraGeneratorTestName} ${CMAKE_CTEST_COMMAND}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/Simple"
          "${CMake_BINARY_DIR}/Tests/${extraGeneratorTestName}"
          --build-two-config
          --build-generator "${extraGenerator} - ${CMAKE_GENERATOR}"
          --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
          --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
          --build-project Simple
          --build-options ${build_options}
          --test-command Simple)
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${extraGeneratorTestName}")
      endif ()
    endforeach(extraGenerator)

  endif()

  # test for correct sub-project generation
  # not implemented in Xcode or Ninja
  if(NOT CMAKE_GENERATOR MATCHES "Xcode|Ninja")
    # run cmake and configure all of SubProject
    # but only build the independent executable car
    add_test(SubProject ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/SubProject"
      "${CMake_BINARY_DIR}/Tests/SubProject"
      --build-project SubProject
      ${build_generator_args}
      --build-target car
      --build-options ${build_options}
      --test-command car
      )

    # For stage 2, do not run cmake again.
    # Then build the foo sub project which should build
    # the bar library which should be referenced because
    # foo links to the static library bar, but bar is not
    # directly in the foo sub project
    if(CMake_TEST_EXPLICIT_MAKE_PROGRAM)
      set(SubProject-Stage2_BUILD_MAKEPROGRAM
        --build-makeprogram ${CMake_TEST_EXPLICIT_MAKE_PROGRAM}
        )
    endif()
    add_test(SubProject-Stage2  ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/SubProject/foo"
      "${CMake_BINARY_DIR}/Tests/SubProject/foo"
      --build-generator ${CMAKE_GENERATOR}
      --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
      ${SubProject-Stage2_BUILD_MAKEPROGRAM}
      --build-nocmake
      --build-project foo
      --build-target foo
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/SubProject/foo"
      --build-options ${build_options}
      --test-command foo
      )
    set_tests_properties ( SubProject-Stage2 PROPERTIES DEPENDS SubProject)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SubProject")
  endif()

  # macro to add a test that will build a nightly release
  # of CMake for given platform using the release scripts
  macro(ADD_NIGHTLY_BUILD_TEST name script)
    set(_TEST_DIR "${CMake_BINARY_DIR}/Tests/${name}")
    file(MAKE_DIRECTORY "${_TEST_DIR}")
    file(WRITE "${_TEST_DIR}/nightly-cmake.sh"
      "cd ${_TEST_DIR}
${CMake_BINARY_DIR}/bin/cmake -DCMAKE_CREATE_VERSION=nightly -P ${CMake_SOURCE_DIR}/Utilities/Release/${script}
${CMake_BINARY_DIR}/bin/cmake -DDIR=dev -P ${CMake_SOURCE_DIR}/Utilities/Release/upload_release.cmake
    ")
    add_test(${name} /bin/sh ${_TEST_DIR}/nightly-cmake.sh)
    if(COMMAND SET_TESTS_PROPERTIES AND COMMAND GET_TEST_PROPERTY)
      set_tests_properties (${name} PROPERTIES TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})
    endif()
  endmacro()
  if(CMAKE_BUILD_NIGHTLY_RELEASES)
    ADD_NIGHTLY_BUILD_TEST(CMakeNightlyWin32
      win32_release.cmake)
    ADD_NIGHTLY_BUILD_TEST(CMakeNightlyWin64
      win64_release.cmake)
    ADD_NIGHTLY_BUILD_TEST(CMakeNightlyOSX
      osx_release.cmake)
    ADD_NIGHTLY_BUILD_TEST(CMakeNightlyLinux64
      linux64_release.cmake)
    set_property(TEST CMakeNightlyWin64 PROPERTY DEPENDS CMakeNightlyWin32)
  endif()

  # add tests with more complex invocations
  add_test(Framework ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Framework"
    "${CMake_BINARY_DIR}/Tests/Framework"
    --build-two-config
    ${build_generator_args}
    --build-project Framework
    --build-options ${build_options}
    "-DCMAKE_INSTALL_PREFIX:PATH=${CMake_BINARY_DIR}/Tests/Framework/Install"
    --test-command bar)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Framework")

  add_test(TargetName ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/TargetName"
    "${CMake_BINARY_DIR}/Tests/TargetName"
    --build-two-config
    ${build_generator_args}
    --build-project TargetName
    --build-options ${build_options}
    --test-command ${CMAKE_CMAKE_COMMAND} -E compare_files
    ${CMake_SOURCE_DIR}/Tests/TargetName/scripts/hello_world
    ${CMake_BINARY_DIR}/Tests/TargetName/scripts/hello_world)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/TargetName")

  add_test(LibName ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/LibName"
    "${CMake_BINARY_DIR}/Tests/LibName"
    --build-two-config
    ${build_generator_args}
    --build-project LibName
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/LibName/lib"
    --build-options ${build_options}
    --test-command foobar
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/LibName")

  add_test(CustComDepend ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/CustComDepend"
    "${CMake_BINARY_DIR}/Tests/CustComDepend"
    --build-two-config
    ${build_generator_args}
    --build-project CustComDepend
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/CustComDepend/bin"
    --build-options ${build_options}
    --test-command foo bar.c
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CustComDepend")

  add_test(ArgumentExpansion  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ArgumentExpansion"
    "${CMake_BINARY_DIR}/Tests/ArgumentExpansion"
    ${build_generator_args}
    --build-project ArgumentExpansion
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/ArgumentExpansion/bin"
    --build-options ${build_options}
    )
  set_tests_properties(ArgumentExpansion PROPERTIES
    FAIL_REGULAR_EXPRESSION "Unexpected: ")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ArgumentExpansion")

  add_test(GeneratorExpression
    ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/GeneratorExpression"
    "${CMake_BINARY_DIR}/Tests/GeneratorExpression"
    ${build_generator_args}
    --build-project GeneratorExpression
    --build-options ${build_options}
      -DCMAKE_BUILD_TYPE=\${CTEST_CONFIGURATION_TYPE}
    --test-command ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/GeneratorExpression")

  add_test(CustomCommand  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/CustomCommand"
    "${CMake_BINARY_DIR}/Tests/CustomCommand"
    --build-two-config
    ${build_generator_args}
    --build-project CustomCommand
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/CustomCommand/bin"
    --build-options ${build_options}
    --test-command CustomCommand
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CustomCommand")

  ADD_TEST_MACRO(CustomCommandByproducts CustomCommandByproducts)

  ADD_TEST_MACRO(EmptyDepends ${CMAKE_CTEST_COMMAND})

  add_test(CustomCommandWorkingDirectory  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/CustomCommandWorkingDirectory"
    "${CMake_BINARY_DIR}/Tests/CustomCommandWorkingDirectory"
    --build-two-config
    ${build_generator_args}
    --build-project TestWorkingDir
    --build-options ${build_options}
    --test-command working
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CustomCommandWorkingDirectory")

   #add_test(SimpleExclude ${CMAKE_CTEST_COMMAND}
   #  --build-and-test
   #  "${CMake_SOURCE_DIR}/Tests/SimpleExclude"
   #  "${CMake_BINARY_DIR}/Tests/SimpleExclude"
   #  ${build_generator_args}
   #  --build-project SimpleExclude
   #  --build-two-config
   #  --build-options ${build_options}
   #  --test-command t4
   #--test-command "${CMAKE_COMMAND}"
   #"-DCONFIGURATION=\${CTEST_CONFIGURATION_TYPE}"
   #-P "${CMake_BINARY_DIR}/Tests/SimpleExclude/run.cmake"
   #)

#  add_test(SameName  ${CMAKE_CTEST_COMMAND}
#    --build-and-test
#    "${CMake_SOURCE_DIR}/Tests/SameName"
#    "${CMake_BINARY_DIR}/Tests/SameName"
#    ${build_generator_args}
#    --build-project SameName
#    --build-two-config
#    --build-options ${build_options}
#    --test-command
#    "${CMake_BINARY_DIR}/Tests/SameName/Exe1/mytest2")

  add_test(OutOfSource ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/OutOfSource"
    "${CMake_BINARY_DIR}/Tests/OutOfSource"
    ${build_generator_args}
    --build-project OutOfSource
    --build-two-config
    --build-options ${build_options}
    --test-command
    "${CMake_BINARY_DIR}/Tests/OutOfSource/SubDir/OutOfSourceSubdir/simple")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/OutOfSource")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/OutOfSourceDeep")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/OutOfBinary")

  add_test(BuildDepends ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/BuildDepends"
    "${CMake_BINARY_DIR}/Tests/BuildDepends"
    ${build_generator_args}
    --build-project BuildDepends
    --build-options ${build_options}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/BuildDepends")

  set(SimpleInstallInstallDir
    "${CMake_BINARY_DIR}/Tests/SimpleInstall/InstallDirectory")
  add_test(SimpleInstall ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/SimpleInstall"
    "${CMake_BINARY_DIR}/Tests/SimpleInstall"
    ${build_generator_args}
    --build-project TestSimpleInstall
    --build-two-config
    --build-options ${build_options}
    "-DCMAKE_INSTALL_PREFIX:PATH=${SimpleInstallInstallDir}"
    "-DCTEST_TEST_CPACK:BOOL=${CTEST_TEST_CPACK}"
    --test-command   ${SimpleInstallInstallDir}/MyTest/bin/SimpleInstExe)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SimpleInstall")
  add_test(SimpleInstall-Stage2 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/SimpleInstallS2"
    "${CMake_BINARY_DIR}/Tests/SimpleInstallS2"
    ${build_generator_args}
    --build-project TestSimpleInstall
    --build-two-config
    --build-options ${build_options}
    "-DCMAKE_INSTALL_PREFIX:PATH=${SimpleInstallInstallDir}"
    "-DSTAGE2:BOOL=1"
    --test-command   ${SimpleInstallInstallDir}/MyTest/bin/SimpleInstExeS2)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SimpleInstallS2")

  set(MissingInstallInstallDir
    "${CMake_BINARY_DIR}/Tests/MissingInstall/InstallDirectory")
  add_test(MissingInstall ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/MissingInstall"
    "${CMake_BINARY_DIR}/Tests/MissingInstall"
    ${build_generator_args}
    --build-project TestMissingInstall
    --build-two-config
    --build-options ${build_options}
    "-DCMAKE_INSTALL_PREFIX:PATH=${MissingInstallInstallDir}")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/MissingInstall")

  # By default, run the CPackComponents test if the CTEST_TEST_CPACK
  # option is ON:
  #
  set(CTEST_RUN_CPackComponents ${CTEST_TEST_CPACK})
  set(CTEST_package_X11_TEST ${CTEST_TEST_CPACK})
  set(CTEST_RUN_CPackComponentsForAll ${CTEST_TEST_CPACK})
  set(CTEST_RUN_CPackComponentsPrefix ${CTEST_TEST_CPACK})

  find_program(NSIS_MAKENSIS_EXECUTABLE NAMES makensis
    PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS]
    DOC "makensis program location"
    )

  # But on Windows, only run the CPackComponents test if the NSIS
  # installer builder is available:
  #
  if(WIN32)
    if(NSIS_MAKENSIS_EXECUTABLE)
      set(CTEST_RUN_CPackComponents ON)
    else()
      set(CTEST_RUN_CPackComponents OFF)
      set(CTEST_package_X11_TEST OFF)
    endif()
  endif()

  # On Windows run the CPackWiXGenerator test
  # if the WiX Toolset seems to be available
  if(WIN32)
    file(TO_CMAKE_PATH "$ENV{WIX}" WIX_ROOT)

    find_program(WIX_LIGHT_EXECUTABLE light
      PATHS "${WIX_ROOT}/bin"
      DOC "WiX Toolset light.exe location")

    if(WIX_LIGHT_EXECUTABLE)
      add_test(CPackWiXGenerator ${CMAKE_CTEST_COMMAND}
        -C \${CTEST_CONFIGURATION_TYPE}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/CPackWiXGenerator"
        "${CMake_BINARY_DIR}/Tests/CPackWiXGenerator"
        ${build_generator_args}
        --build-project CPackWiXGenerator
        --build-options ${build_options}
        --test-command ${CMAKE_CMAKE_COMMAND}
          "-DCPackWiXGenerator_BINARY_DIR:PATH=${CMake_BINARY_DIR}/Tests/CPackWiXGenerator"
          "-Dconfig=\${CTEST_CONFIGURATION_TYPE}"
          -P "${CMake_SOURCE_DIR}/Tests/CPackWiXGenerator/RunCPackVerifyResult.cmake")

      set_property(TEST CPackWiXGenerator PROPERTY
        ATTACHED_FILES_ON_FAIL
        "${CMake_BINARY_DIR}/Tests/CPackWiXGenerator/_CPack_Packages/win32/WIX/wix.log")
    endif()
  endif()

  if(CTEST_RUN_CPackComponents)
    set(CPackComponents_BUILD_OPTIONS)
    if(APPLE)
      set(CPackComponents_BUILD_OPTIONS -DCPACK_BINARY_DRAGNDROP:BOOL=ON)
      if(CMake_TEST_XCODE_VERSION VERSION_GREATER "4.6")
        set(CPackComponents_BUILD_OPTIONS ${CPackComponents_BUILD_OPTIONS}
          -DCPACK_BINARY_PRODUCTBUILD:BOOL=ON)
      endif()
    endif()
    if(NSIS_MAKENSIS_EXECUTABLE)
      set(CPackComponents_BUILD_OPTIONS ${CPackComponents_BUILD_OPTIONS}
        -DCPACK_BINARY_NSIS:BOOL=ON)
    endif()

    add_test(CPackComponents ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/CPackComponents"
      "${CMake_BINARY_DIR}/Tests/CPackComponents"
      ${build_generator_args}
      --build-project CPackComponents
      --build-two-config
      --build-target package
      --build-options ${build_options}
        -DCPACK_BINARY_DEB:BOOL=${CPACK_BINARY_DEB}
        -DCPACK_BINARY_RPM:BOOL=${CPACK_BINARY_RPM}
        ${CPackComponents_BUILD_OPTIONS}
        --graphviz=CPackComponents.dot
      --test-command ${CMAKE_CMAKE_COMMAND}
        "-DCPackComponents_BINARY_DIR:PATH=${CMake_BINARY_DIR}/Tests/CPackComponents"
        -P "${CMake_SOURCE_DIR}/Tests/CPackComponents/VerifyResult.cmake")
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CPackComponents")
  endif()

  if(CTEST_RUN_CPackComponentsForAll)
    # Check whether if rpmbuild command is found
    # before adding RPM tests
    if(CPACK_BINARY_RPM)
      list(APPEND ACTIVE_CPACK_GENERATORS RPM)
    endif()
    # Check whether if dpkg command is found
    # before adding DEB tests
    if(CPACK_BINARY_DEB)
      list(APPEND ACTIVE_CPACK_GENERATORS DEB)
    endif()

    # ACTIVE_CPACK_GENERATORS variable
    # now contains the list of 'active generators'
    set(CPackComponentsForAll_BUILD_OPTIONS)
    # set up list of CPack generators
    list(APPEND ACTIVE_CPACK_GENERATORS "ZIP")
    if(APPLE)
      list(APPEND ACTIVE_CPACK_GENERATORS "DragNDrop")
      if(CMake_TEST_XCODE_VERSION VERSION_GREATER "4.6")
        list(APPEND ACTIVE_CPACK_GENERATORS "productbuild")
      endif()
    endif()

    # set up list of component packaging ways
    list(APPEND CWAYLST "default")
    list(APPEND CWAYLST "OnePackPerGroup")
    list(APPEND CWAYLST "IgnoreGroup")
    list(APPEND CWAYLST "AllInOne")
    foreach(CPackGen IN LISTS ACTIVE_CPACK_GENERATORS)
      set(CPackRun_CPackGen  "-DCPackGen=${CPackGen}")
      foreach(CPackComponentWay ${CWAYLST})
        set(CPackRun_CPackComponentWay "-DCPackComponentWay=${CPackComponentWay}")
        add_test(CPackComponentsForAll-${CPackGen}-${CPackComponentWay}
          ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/CPackComponentsForAll"
          "${CMake_BINARY_DIR}/Tests/CPackComponentsForAll/build${CPackGen}-${CPackComponentWay}"
          ${build_generator_args}
          --build-project CPackComponentsForAll
          --build-options ${build_options}
             -DCPACK_GENERATOR:STRING=${CPackGen}
             -DCPACK_BINARY_${CPackGen}:BOOL=ON
             ${CPackRun_CPackComponentWay}
             ${CPackComponentsForAll_BUILD_OPTIONS}
             --graphviz=CPackComponentsForAll.dot
          --test-command ${CMAKE_CMAKE_COMMAND}
            "-DCPackComponentsForAll_BINARY_DIR:PATH=${CMake_BINARY_DIR}/Tests/CPackComponentsForAll/build${CPackGen}-${CPackComponentWay}"
            "${CPackRun_CPackGen}"
            "${CPackRun_CPackComponentWay}"
            -P "${CMake_SOURCE_DIR}/Tests/CPackComponentsForAll/RunCPackVerifyResult.cmake")
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CPackComponentsForAll/build${CPackGen}-${CPackComponentWay}")
      endforeach()
    endforeach()

    # debian specific
    if(DPKG_EXECUTABLE)
      unset(CPackRun_CPackDEBConfiguration_ALL_CONFIGS)
      set(DEB_TEST_NAMES "CPackComponentsDEB")
      set(DEB_CONFIGURATIONS_TO_TEST "components-lintian-dpkgdeb-checks"
                                     "components-description1"
                                     "components-description2"
                                     "components-source"
                                     "components-shlibdeps1"
                                     "components-depend1"
                                     "components-depend2"
                                     "compression")
      set(CPackGen "DEB")
      set(CPackRun_CPackGen "-DCPackGen=${CPackGen}")

      foreach(CPackDEBConfiguration IN LISTS DEB_CONFIGURATIONS_TO_TEST)
        set(CPackRun_CPackDEBConfiguration "-DCPackDEBConfiguration=${CPackDEBConfiguration}")
        add_test(${DEB_TEST_NAMES}-${CPackDEBConfiguration}
          ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
          --build-and-test
              "${CMake_SOURCE_DIR}/Tests/${DEB_TEST_NAMES}"
              "${CMake_BINARY_DIR}/Tests/${DEB_TEST_NAMES}/build${CPackGen}-${CPackDEBConfiguration}"
              ${build_generator_args}
          --build-project CPackComponentsDEB
          --build-options ${build_options}
              -DCPACK_GENERATOR:STRING=${CPackGen}
              -DCPACK_BINARY_${CPackGen}:BOOL=ON
              ${CPackRun_CPackDEBConfiguration}
              ${CPackRun_CPackDEBConfiguration_ALL_CONFIGS}
          --graphviz=${DEB_TEST_NAMES}.dot
          --test-command ${CMAKE_CMAKE_COMMAND}
              "-D${DEB_TEST_NAMES}_SOURCE_DIR:PATH=${CMake_SOURCE_DIR}/Tests/${DEB_TEST_NAMES}"
              "-D${DEB_TEST_NAMES}_BINARY_DIR:PATH=${CMake_BINARY_DIR}/Tests/${DEB_TEST_NAMES}/build${CPackGen}-${CPackDEBConfiguration}"
              "${CPackRun_CPackGen}"
              "${CPackRun_CPackDEBConfiguration}"
              -P "${CMake_SOURCE_DIR}/Tests/${DEB_TEST_NAMES}/RunCPackVerifyResult-${CPackDEBConfiguration}.cmake")
          list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${DEB_TEST_NAMES}/build${CPackGen}-${CPackDEBConfiguration}")
      endforeach()
    endif()

  endif()

  # By default, turn this test off (because it takes a long time...)
  #
  if(NOT DEFINED CTEST_RUN_CPackTestAllGenerators)
    set(CTEST_RUN_CPackTestAllGenerators OFF)

    # ...but: if it appears to be a coverage dashboard, or long tests are
    # on, then set it to the generic CTEST_TEST_CPACK setting.
    #
    if(CMAKE_CXX_FLAGS MATCHES "-ftest-coverage" OR
      NOT "$ENV{COVFILE}" STREQUAL "" OR
      CMAKE_RUN_LONG_TESTS)
      set(CTEST_RUN_CPackTestAllGenerators ${CTEST_TEST_CPACK})
    endif()
  endif()

  if(CTEST_RUN_CPackTestAllGenerators)
    add_test(CPackTestAllGenerators ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/CPackTestAllGenerators"
      "${CMake_BINARY_DIR}/Tests/CPackTestAllGenerators"
      ${build_generator_args}
      --build-project CPackTestAllGenerators
      --build-options ${build_options}
      --test-command
      ${CMAKE_CMAKE_COMMAND}
        -D dir=${CMake_BINARY_DIR}/Tests/CPackTestAllGenerators
        -P ${CMake_SOURCE_DIR}/Tests/CPackTestAllGenerators/RunCPack.cmake
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CPackTestAllGenerators")
  endif()

  if(CTEST_RUN_CPackComponentsPrefix)
    set(CPackComponents_BUILD_OPTIONS)
    if(APPLE)
      set(CPackComponents_BUILD_OPTIONS -DCPACK_BINARY_DRAGNDROP:BOOL=ON)
      if(CMake_TEST_XCODE_VERSION VERSION_GREATER "4.6")
        set(CPackComponents_BUILD_OPTIONS ${CPackComponents_BUILD_OPTIONS}
          -DCPACK_BINARY_PRODUCTBUILD:BOOL=ON)
      endif()
    endif()
    if(NOT NSIS_MAKENSIS_EXECUTABLE)
      set(CPackComponents_BUILD_OPTIONS ${CPackComponents_BUILD_OPTIONS}
        -DCPACK_BINARY_NSIS:BOOL=OFF)
    endif()

    add_test(CPackComponentsPrefix ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/CPackComponentsPrefix"
      "${CMake_BINARY_DIR}/Tests/CPackComponentsPrefix"
      ${build_generator_args}
      --build-project CPackComponentsPrefix
      --build-two-config
      --build-target package
      --build-options ${build_options}
        -DCPACK_BINARY_DEB:BOOL=${CPACK_BINARY_DEB}
        -DCPACK_BINARY_RPM:BOOL=${CPACK_BINARY_RPM}
        -DCPACK_BINARY_ZIP:BOOL=ON
        ${CPackComponents_BUILD_OPTIONS}
        )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CPackComponentsPrefix")
  endif()

  if(CTEST_package_X11_TEST)
    set(X11_build_target_arg --build-target package)
  else()
    set(X11_build_target_arg)
  endif()

  add_test(X11 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/X11"
    "${CMake_BINARY_DIR}/Tests/X11"
    ${build_generator_args}
    --build-project UseX11
    --build-two-config
    ${X11_build_target_arg}
    --build-options ${build_options}
    --test-command  UseX11)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/X11")

  if(NOT DEFINED CTEST_RUN_CMakeTestAllGenerators)
    set(CTEST_RUN_CMakeTestAllGenerators ON)
  endif()

  if(CTEST_RUN_CMakeTestAllGenerators)
    add_test(CMakeTestAllGenerators ${CMAKE_CMAKE_COMMAND}
        -D dir=${CMake_BINARY_DIR}/Tests/CMakeTestAllGenerators
        -D CMake_SOURCE_DIR=${CMake_SOURCE_DIR}
        -P ${CMake_SOURCE_DIR}/Tests/CMakeTestAllGenerators/RunCMake.cmake
      )
    list(APPEND TEST_BUILD_DIRS
      "${CMake_BINARY_DIR}/Tests/CMakeTestAllGenerators")
    # This test runs a lot of processes.  Do not make them compete
    # for resources with other tests.
    set_property(TEST CMakeTestAllGenerators PROPERTY RUN_SERIAL 1)
  endif()

  if(NOT DEFINED CTEST_RUN_CMakeTestMultipleConfigures)
    set(CTEST_RUN_CMakeTestMultipleConfigures ON)
  endif()

  if(CTEST_RUN_CMakeTestMultipleConfigures)
    add_test(CMakeTestMultipleConfigures ${CMAKE_CMAKE_COMMAND}
        -D dir=${CMake_BINARY_DIR}/Tests/CMakeTestMultipleConfigures
        -D gen=${CMAKE_GENERATOR}
        -D CMake_SOURCE_DIR=${CMake_SOURCE_DIR}
        -P ${CMake_SOURCE_DIR}/Tests/CMakeTestMultipleConfigures/RunCMake.cmake
      )
    list(APPEND TEST_BUILD_DIRS
      "${CMake_BINARY_DIR}/Tests/CMakeTestMultipleConfigures")
  endif()

  if(NOT CMake_TEST_EXTERNAL_CMAKE)
    add_test(LoadedCommandOneConfig  ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/LoadCommandOneConfig"
      "${CMake_BINARY_DIR}/Tests/LoadCommandOneConfig"
      ${build_generator_args}
      --build-project LoadCommand
      --build-options ${build_options}
      --test-command  LoadedCommand
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/LoadCommandOneConfig")
  endif()

  add_test(complex  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Complex"
    "${CMake_BINARY_DIR}/Tests/Complex"
    --build-two-config
    --build-config-sample "${CMAKE_CTEST_COMMAND}"
    ${build_generator_args}
    --build-project Complex
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Complex/bin"
    --build-options ${build_options}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    --test-command complex
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Complex")

  add_test(complexOneConfig  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ComplexOneConfig"
    "${CMake_BINARY_DIR}/Tests/ComplexOneConfig"
    ${build_generator_args}
    --build-project Complex
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/ComplexOneConfig/bin"
    --build-options ${build_options}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    --test-command complex)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ComplexOneConfig")
  # because of the registry write these tests depend on each other
  set_tests_properties ( complex PROPERTIES DEPENDS complexOneConfig)

  add_test(Environment ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Environment"
    "${CMake_BINARY_DIR}/Tests/Environment"
    ${build_generator_args}
    --build-project EnvironmentProj
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Environment"
    --force-new-ctest-process
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Environment")

  add_test(QtAutomocNoQt  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/QtAutomocNoQt"
    "${CMake_BINARY_DIR}/Tests/QtAutomocNoQt"
    ${build_generator_args}
    --build-project QtAutomocNoQt
    --build-options ${build_options}
      -DCMAKE_BUILD_TYPE=\${CTEST_CONFIGURATION_TYPE}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/QtAutomocNoQt")

  # On Windows there is no RPATH, so while Qt might be available for building,
  # the required dlls may not be in the PATH, so we can't run the executables
  # on that platform.
  if(WIN32)
    set(run_autogen_test ${CMAKE_CTEST_COMMAND} -V)
    set(run_autouic_test ${CMAKE_CTEST_COMMAND} -V)
  else()
    set(run_autogen_test complex/QtAutogen)
    set(run_autouic_test QtAutoUicInterface)
  endif()
  if(NOT CMAKE_CONFIGURATION_TYPES)
    set(QtAutogen_BUILD_OPTIONS -DCMAKE_BUILD_TYPE=$<CONFIGURATION>)
  endif()

  find_package(Qt5Widgets QUIET NO_MODULE)
  if(Qt5Widgets_FOUND)
    add_test(NAME Qt5Autogen COMMAND ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/QtAutogen"
      "${CMake_BINARY_DIR}/Tests/Qt5Autogen"
      ${build_generator_args}
      --build-project QtAutogen
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt5Autogen"
      --force-new-ctest-process
      --build-options ${build_options}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE} -DQT_TEST_VERSION=5
        ${QtAutogen_BUILD_OPTIONS}
      --test-command ${run_autogen_test}
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt5Autogen")

    add_test(Qt5AutoUicInterface ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/QtAutoUicInterface"
      "${CMake_BINARY_DIR}/Tests/Qt5AutoUicInterface"
      ${build_generator_args}
      --build-project QtAutoUicInterface
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt5AutoUicInterface"
      --force-new-ctest-process
      --build-options ${build_options}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE} -DQT_TEST_VERSION=5
      --test-command ${run_autouic_test}
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt5AutoUicInterface")
  endif()
  if(QT4_WORKS AND QT_QTGUI_FOUND)
    add_test(NAME Qt4Autogen COMMAND ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/QtAutogen"
      "${CMake_BINARY_DIR}/Tests/Qt4Autogen"
      ${build_generator_args}
      --build-project QtAutogen
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt4Autogen"
      --force-new-ctest-process
      --build-options ${build_options}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE} -DQT_TEST_VERSION=4
        ${QtAutogen_BUILD_OPTIONS}
      --test-command ${run_autogen_test}
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4Autogen")

    add_test(Qt4AutoUicInterface ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/QtAutoUicInterface"
      "${CMake_BINARY_DIR}/Tests/Qt4AutoUicInterface"
      ${build_generator_args}
      --build-project QtAutoUicInterface
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt4AutoUicInterface"
      --force-new-ctest-process
      --build-options ${build_options}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE} -DQT_TEST_VERSION=4
      --test-command ${run_autouic_test}
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4AutoUicInterface")

    add_test(Qt4Targets ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Qt4Targets"
      "${CMake_BINARY_DIR}/Tests/Qt4Targets"
      ${build_generator_args}
      --build-project Qt4Targets
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt4Targets"
      --force-new-ctest-process
      --build-options ${build_options}
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
      --test-command ${CMAKE_CTEST_COMMAND} -V
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4Targets")

    if(Qt5Widgets_FOUND AND NOT Qt5Widgets_VERSION VERSION_LESS 5.1.0)
      add_test(Qt4And5Automoc ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/Qt4And5Automoc"
        "${CMake_BINARY_DIR}/Tests/Qt4And5Automoc"
        ${build_generator_args}
        --build-project Qt4And5Automoc
        --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt4And5Automoc"
        --force-new-ctest-process
        --build-options ${build_options}
        --test-command ${CMAKE_CTEST_COMMAND} -V
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4And5Automoc")
      add_test(Qt4And5AutomocReverse ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/Qt4And5Automoc"
        "${CMake_BINARY_DIR}/Tests/Qt4And5AutomocReverse"
        ${build_generator_args}
        --build-project Qt4And5Automoc
        --build-exe-dir "${CMake_BINARY_DIR}/Tests/Qt4And5AutomocReverse"
        --force-new-ctest-process
        --build-options ${build_options} -DQT_REVERSE_FIND_ORDER=1
        --test-command ${CMAKE_CTEST_COMMAND} -V
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Qt4And5AutomocReverse")
    endif()
  endif()

  if(CMake_TEST_CUDA)
    add_subdirectory(Cuda)
    add_subdirectory(CudaOnly)
  endif()

  if(CMake_TEST_FindBoost)
    add_subdirectory(FindBoost)
  endif()

  if(CMake_TEST_FindBZip2)
    add_subdirectory(FindBZip2)
  endif()

  if(CMake_TEST_FindDoxygen)
    add_subdirectory(FindDoxygen)
  endif()

  if(CMake_TEST_FindGSL)
    add_subdirectory(FindGSL)
  endif()

  if(CMake_TEST_FindGTest)
    add_subdirectory(FindGTest)
    add_subdirectory(GoogleTest)
  endif()

  if(CMake_TEST_FindICU)
    add_subdirectory(FindICU)
  endif()

  if(CMake_TEST_FindJsonCpp)
    add_subdirectory(FindJsonCpp)
  endif()

  if(CMake_TEST_FindLibRHash)
    add_subdirectory(FindLibRHash)
  endif()

  if(CMake_TEST_FindLibUV)
    add_subdirectory(FindLibUV)
  endif()

  if(CMake_TEST_FindLTTngUST)
    add_subdirectory(FindLTTngUST)
  endif()

  if(CMake_TEST_FindOpenCL)
    add_subdirectory(FindOpenCL)
  endif()

  if(CMake_TEST_FindOpenGL)
    add_subdirectory(FindOpenGL)
  endif()

  if(CMake_TEST_FindOpenMP)
    add_subdirectory(FindOpenMP)
  endif()

  if(CMake_TEST_FindOpenSSL)
    add_subdirectory(FindOpenSSL)
  endif()

  if(CMake_TEST_FindMPI)
    add_subdirectory(FindMPI)
  endif()

  if(CMake_TEST_FindPNG)
    add_subdirectory(FindPNG)
  endif()

  if(CMake_TEST_FindProtobuf)
    add_subdirectory(FindProtobuf)
  endif()

  if(CMake_TEST_FindTIFF)
    add_subdirectory(FindTIFF)
  endif()

  if(CMake_TEST_FindVulkan)
    add_subdirectory(FindVulkan)
  endif()

  if(CMake_TEST_FindXalanC)
    add_subdirectory(FindXalanC)
  endif()

  if(CMake_TEST_FindXercesC)
    add_subdirectory(FindXercesC)
  endif()

  add_subdirectory(FindThreads)

  # Matlab module
  if(CMake_TEST_FindMatlab)
    ADD_TEST_MACRO(FindMatlab.basic_checks      ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>)
    ADD_TEST_MACRO(FindMatlab.versions_checks   ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>)
    ADD_TEST_MACRO(FindMatlab.components_checks ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>)
  endif()

  find_package(GTK2 QUIET)
  if(GTK2_FOUND)
    add_subdirectory(FindGTK2)
  endif()

  add_test(ExternalProject ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ExternalProject"
    "${CMake_BINARY_DIR}/Tests/ExternalProject"
    ${build_generator_args}
    --build-project ExternalProjectTest
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/ExternalProject"
    --force-new-ctest-process
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProject")
  set_tests_properties(ExternalProject PROPERTIES
    TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})

  add_test(NAME ExternalProjectSubdir
    COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ExternalProjectSubdir"
    "${CMake_BINARY_DIR}/Tests/ExternalProjectSubdir"
    ${build_generator_args}
    --build-project ExternalProjectSubdir
    --force-new-ctest-process
    --build-options ${build_options}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProjectSubdir")

  add_test(NAME ExternalProjectSourceSubdir
    COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ExternalProjectSourceSubdir"
    "${CMake_BINARY_DIR}/Tests/ExternalProjectSourceSubdir"
    ${build_generator_args}
    --build-project ExternalProjectSourceSubdir
    --force-new-ctest-process
    --build-options ${build_options}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProjectSourceSubdir")

  add_test(ExternalProjectLocal ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ExternalProjectLocal"
    "${CMake_BINARY_DIR}/Tests/ExternalProjectLocal"
    ${build_generator_args}
    --build-project ExternalProjectLocalTest
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/ExternalProjectLocal"
    --force-new-ctest-process
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProjectLocal")
  set_tests_properties(ExternalProjectLocal PROPERTIES
    TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})

  add_test(ExternalProjectUpdateSetup ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/ExternalProjectUpdate"
    "${CMake_BINARY_DIR}/Tests/ExternalProjectUpdate"
    ${build_generator_args}
    --build-project ExternalProjectUpdateTest
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/ExternalProjectUpdate"
    --force-new-ctest-process
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProjectUpdate")
  set_tests_properties(ExternalProjectUpdateSetup PROPERTIES
    TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})

  add_test(NAME ExternalProjectUpdate
    COMMAND ${CMAKE_CMAKE_COMMAND}
    -DExternalProjectUpdate_SOURCE_DIR:PATH=${CMake_SOURCE_DIR}/Tests/ExternalProjectUpdate
    -DExternalProjectUpdate_BINARY_DIR:PATH=${CMake_BINARY_DIR}/Tests/ExternalProjectUpdate
    -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
    -DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
    -DCMAKE_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
    -DCMAKE_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}
    -P ${CMake_SOURCE_DIR}/Tests/ExternalProjectUpdate/ExternalProjectUpdateTest.cmake
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/ExternalProjectUpdate")
  set_tests_properties(ExternalProjectUpdate PROPERTIES
    TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT}
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}/Tests/ExternalProjectUpdate
    DEPENDS ExternalProjectUpdateSetup )

  # do each of the tutorial steps
  foreach(STP RANGE 1 7)
    add_test(TutorialStep${STP} ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Tutorial/Step${STP}"
      "${CMake_BINARY_DIR}/Tests/Tutorial/Step${STP}"
      --build-two-config
      ${build_generator_args}
      --build-project Tutorial
      --build-options ${build_options}
      --test-command Tutorial 25.0)
  endforeach()
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Tutorial")

  add_test(testing ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Testing"
    "${CMake_BINARY_DIR}/Tests/Testing"
    ${build_generator_args}
    --build-project Testing
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
    )
  set_tests_properties(testing PROPERTIES PASS_REGULAR_EXPRESSION "Passed")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Testing")

  add_test(wrapping  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Wrapping"
    "${CMake_BINARY_DIR}/Tests/Wrapping"
    ${build_generator_args}
    --build-project Wrapping
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Wrapping/bin"
    --build-options ${build_options}
    --test-command wrapping
    )
  add_test(qtwrapping  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Wrapping"
    "${CMake_BINARY_DIR}/Tests/Wrapping"
    ${build_generator_args}
    --build-project Wrapping
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Wrapping/bin"
    --build-options ${build_options}
      --test-command qtwrapping
      )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Wrapping")

  add_test(testdriver1 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/TestDriver"
    "${CMake_BINARY_DIR}/Tests/TestDriver"
    ${build_generator_args}
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Wrapping/bin"
    --build-project TestDriverTest
    --build-options ${build_options}
    --test-command TestDriverTest test1
    )

  add_test(testdriver2 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/TestDriver"
    "${CMake_BINARY_DIR}/Tests/TestDriver"
    ${build_generator_args}
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Wrapping/bin"
    --build-project TestDriverTest
    --build-options ${build_options}
    --test-command TestDriverTest test2
    )

  add_test(testdriver3  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/TestDriver"
    "${CMake_BINARY_DIR}/Tests/TestDriver"
    ${build_generator_args}
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Wrapping/bin"
    --build-project TestDriverTest
    --build-options ${build_options}
    --test-command TestDriverTest subdir/test3
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/TestDriver")

  add_test(Dependency ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/Dependency"
    "${CMake_BINARY_DIR}/Tests/Dependency"
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/Dependency/Exec"
    ${build_generator_args}
    --build-project Dependency
    --build-options ${build_options}
    --test-command exec
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Dependency")

  if(CMAKE_SYSTEM_NAME MATCHES syllable)

# RPATH isn't supported under Syllable, so the tests don't
# find their libraries. In order to fix that LIBRARY_OUTPUT_DIR
# in the tests would have to be adjusted to ${EXECUTABLE_OUTPUT_DIR}/lib .
# For now we just require on Syllable that the user adjusts the DLL_PATH
# environment variable, so except the two tests below all other tests will succeed.

    set(_DLL_PATH "$ENV{DLL_PATH}")
    if(NOT "${_DLL_PATH}" MATCHES "^(.*:)?\\@bindir\\@/\\.(:.*)?$")
      message(FATAL_ERROR "In order to successfully run the CMake test suite on Syllable you need to add \"\\@bindir\\@/.\" to the DLL_PATH environment variable")
    endif()
    if(NOT "${_DLL_PATH}" MATCHES "^(.*:)?\\@bindir\\@/\\.\\./lib(:.*)?$")
      message(FATAL_ERROR "In order to successfully run the CMake test suite on Syllable you need to add \"\\@bindir\\@/../lib\" to the DLL_PATH environment variable")
    endif()

  else()

    add_test(JumpWithLibOut  ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Jump"
      "${CMake_BINARY_DIR}/Tests/Jump/WithLibOut"
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Jump/WithLibOut/Executable"
      --build-project Jump
      ${build_generator_args}
      --build-options ${build_options}
      -DLIBRARY_OUTPUT_PATH:PATH=${CMake_BINARY_DIR}/Tests/Jump/WithLibOut/Lib
      --test-command jumpExecutable
      )

    add_test(JumpNoLibOut  ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Jump"
      "${CMake_BINARY_DIR}/Tests/Jump/NoLibOut"
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/Jump/NoLibOut/Executable"
      --build-run-dir "${CMake_BINARY_DIR}/Tests/Jump/NoLibOut/Executable"
      --build-project Jump
      ${build_generator_args}
      --build-options ${build_options}
      --test-command jumpExecutable
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Jump")

    add_test(Plugin ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Plugin"
      "${CMake_BINARY_DIR}/Tests/Plugin"
      ${build_generator_args}
      --build-project Plugin
      --build-two-config
      --build-options ${build_options}
      --test-command bin/example)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Plugin")

    if(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG)
      ADD_TEST_MACRO(RuntimePath RuntimePath)
    endif()
  endif()

  if(APPLE AND "${DARWIN_MAJOR_VERSION}" GREATER 9)
    add_test(MacRuntimePath ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/MacRuntimePath"
      "${CMake_BINARY_DIR}/Tests/MacRuntimePath"
      ${build_generator_args}
      --build-project MacRuntimePath
      --build-options ${build_options}
        -DCMake_TEST_NESTED_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM}
      )
  endif()

  if(CMake_TEST_XCODE_VERSION AND NOT CMake_TEST_XCODE_VERSION VERSION_LESS 5
     AND OSX_VERSION MATCHES "^([0-9]+\\.[0-9]+)")
    set(XCTest_CTEST_OPTIONS --build-config $<CONFIGURATION>)
    set(XCTest_BUILD_OPTIONS -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_MATCH_1} -DCMAKE_OSX_SYSROOT=macosx)
    ADD_TEST_MACRO(XCTest ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -V)
  endif()

  add_test(linkorder1 ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/LinkLineOrder"
    "${CMake_BINARY_DIR}/Tests/LinkLineOrder"
    ${build_generator_args}
    --build-project LinkLineOrder
    --build-options ${build_options}
    --test-command Exec1
    )

  add_test(linkorder2  ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/LinkLineOrder"
    "${CMake_BINARY_DIR}/Tests/LinkLineOrder"
    ${build_generator_args}
    --build-project LinkLineOrder
    --build-options ${build_options}
    --test-command Exec2
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/LinkLineOrder")
  set_tests_properties ( qtwrapping PROPERTIES DEPENDS wrapping)
  set_tests_properties ( testdriver1 PROPERTIES DEPENDS qtwrapping)
  set_tests_properties ( testdriver2 PROPERTIES DEPENDS testdriver1)
  set_tests_properties ( testdriver3 PROPERTIES DEPENDS testdriver2)
  set_tests_properties ( linkorder2 PROPERTIES DEPENDS linkorder1)
  set_tests_properties ( SimpleInstall-Stage2 PROPERTIES DEPENDS SimpleInstall)

  # Test static linking on toolchains known to support it.
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU"
      AND NOT APPLE AND NOT WIN32 AND NOT CYGWIN
      AND EXISTS "/usr/lib/libm.a")
    add_test(LinkStatic  ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/LinkStatic"
      "${CMake_BINARY_DIR}/Tests/LinkStatic"
      ${build_generator_args}
      --build-project LinkStatic
      --build-options ${build_options}
        -DMATH_LIBRARY:FILEPATH=/usr/lib/libm.a
      --test-command LinkStatic
      )
  endif()

  if(MAKE_SUPPORTS_SPACES AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
    add_test(SubDirSpaces ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/SubDirSpaces"
      "${CMake_BINARY_DIR}/Tests/SubDirSpaces"
      --build-exe-dir
      "${CMake_BINARY_DIR}/Tests/SubDirSpaces/Executable Sources"
      ${build_generator_args}
      --build-project SUBDIR
      --build-options ${build_options}
      --test-command test
      "${CMake_BINARY_DIR}/Tests/SubDirSpaces/ShouldBeHere"
      "${CMake_BINARY_DIR}/Tests/SubDirSpaces/testfromsubdir.obj"
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SubDirSpaces")
  endif ()

  if (WIN32)
    add_test(SubDir ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/SubDir"
      "${CMake_BINARY_DIR}/Tests/SubDir"
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/SubDir/Executable"
      ${build_generator_args}
      --build-project SUBDIR
      --build-options ${build_options}
      --test-command test
      "${CMake_BINARY_DIR}/Tests/SubDir/ShouldBeHere"
      "${CMake_BINARY_DIR}/Tests/SubDir/testfromsubdir.obj"
      )
  else ()
    add_test(SubDir ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/SubDir"
      "${CMake_BINARY_DIR}/Tests/SubDir"
      --build-exe-dir "${CMake_BINARY_DIR}/Tests/SubDir/Executable"
      ${build_generator_args}
      --build-project SUBDIR
      --build-options ${build_options}
      --test-command test
      "${CMake_BINARY_DIR}/Tests/SubDir/ShouldBeHere"
      "${CMake_BINARY_DIR}/Tests/SubDir/testfromsubdir.o"
      )
  endif ()
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SubDir")

  if(MSVC)
    ADD_TEST_MACRO(ForceInclude foo)
    ADD_TEST_MACRO(PDBDirectoryAndName myexe)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
      ADD_TEST_MACRO(PrecompiledHeader foo)
    endif()
  endif()
  if(MSVC OR
      "${CMAKE_GENERATOR}" MATCHES "(MSYS|MinGW) Makefiles")
    ADD_TEST_MACRO(ModuleDefinition example_exe)
  endif()

  ADD_TEST_MACRO(CheckCompilerRelatedVariables CheckCompilerRelatedVariables)

  if("${CMAKE_GENERATOR}" MATCHES "Makefile")
    add_test(MakeClean ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/MakeClean"
      "${CMake_BINARY_DIR}/Tests/MakeClean"
      ${build_generator_args}
      --build-project MakeClean
      --build-exe-dir "${CMake_BINARY_DIR}/MakeClean"
      --build-options ${build_options}
      --test-command check_clean
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/MakeClean")
  endif()

  if(NOT DEFINED CTEST_RUN_MFC)
    set(CTEST_RUN_MFC OFF)

    if(MSVC)
      set(CTEST_RUN_MFC ON)

      # Look for evidence that this is a VCExpress build. If so, avoid
      # the MFC test by default.
      string(TOLOWER "${CMAKE_MAKE_PROGRAM}" mkprog)
      if(mkprog MATCHES "vcexpress")
        message(STATUS
          "CMAKE_MAKE_PROGRAM indicates vcexpress, avoiding MFC test")
        set(CTEST_RUN_MFC OFF)
      endif()

      # Since MSBuild might also be the "makeprogram" for a VCExpress
      # build tree, use one more heuristic, too. The string representing
      # the .vcproj file type contains "VCExpress" on machines where an
      # express edition of VS was installed last:
      if(CTEST_RUN_MFC)
        execute_process(COMMAND cmd /c assoc .vcproj
          OUTPUT_STRIP_TRAILING_WHITESPACE
          OUTPUT_VARIABLE ov)
        if(ov MATCHES "VCExpress")
          message(STATUS
            ".vcproj file association indicates VCExpress, avoiding MFC test")
          set(CTEST_RUN_MFC OFF)
        endif()
      endif()

      # For the Watcom WMake generator, avoid the MFC test by default.
      if(CTEST_RUN_MFC)
        if("${CMAKE_GENERATOR}" MATCHES "WMake")
          message(STATUS
            "using the Watcom WMake generator, avoiding MFC test")
          set(CTEST_RUN_MFC OFF)
        endif()
      endif()

      # Last resort, after quick checks are done. Do a try_compile, and avoid
      # the MFC test if the simplest possible MFC app cannot be compiled.
      if(CTEST_RUN_MFC AND NOT DEFINED HAVE_MFC)
        configure_file(
          ${CMAKE_CURRENT_SOURCE_DIR}/MFC/try_compile/CMakeLists.txt
          ${CMAKE_CURRENT_BINARY_DIR}/MFC/try_compile/CMakeLists.txt
          COPYONLY
          )
        configure_file(
          ${CMAKE_CURRENT_SOURCE_DIR}/MFC/mfc1/stdafx.cpp
          ${CMAKE_CURRENT_BINARY_DIR}/MFC/try_compile/stdafx.cpp
          COPYONLY
          )
        configure_file(
          ${CMAKE_CURRENT_SOURCE_DIR}/MFC/mfc1/stdafx.h
          ${CMAKE_CURRENT_BINARY_DIR}/MFC/try_compile/stdafx.h
          COPYONLY
          )

        message(STATUS "Looking for MFC")

        try_compile(HAVE_MFC
          ${CMAKE_CURRENT_BINARY_DIR}/MFC/try_compile/build
          ${CMAKE_CURRENT_BINARY_DIR}/MFC/try_compile
          try_compile_mfc
          OUTPUT_VARIABLE HAVE_MFC_OUTPUT)

        if(HAVE_MFC)
          message(STATUS "Looking for MFC - found")
          set(HAVE_MFC 1 CACHE INTERNAL "Have MFC")
          file(APPEND
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
            "Determining if MFC exists passed with the following output:\n"
            "${HAVE_MFC_OUTPUT}\n\n")
        else()
          message(STATUS "Looking for MFC - not found")
          set(HAVE_MFC "" CACHE INTERNAL "Have MFC")
          file(APPEND
            ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
            "Determining if MFC exists failed with the following output:\n"
            "${HAVE_MFC_OUTPUT}\n\n")
        endif()
      endif()

      if(CTEST_RUN_MFC AND NOT HAVE_MFC)
        message(STATUS
          "cannot compile simplest ever MFC app, avoiding MFC test")
        set(CTEST_RUN_MFC OFF)
      endif()
    endif()
  endif()

  if(CTEST_RUN_MFC)
    add_test(MFC ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/MFC"
      "${CMake_BINARY_DIR}/Tests/MFC"
      --build-two-config
      ${build_generator_args}
      --build-project mfc_driver
      --build-options ${build_options}
      --test-command ${CMAKE_CTEST_COMMAND}
        -C \${CTEST_CONFIGURATION_TYPE} -VV)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/MFC")
  endif()

  if(MSVC AND NOT MSVC_VERSION LESS 1310
     AND (NOT CMAKE_GENERATOR MATCHES "Visual Studio [89]( |$)"
          OR CMAKE_SIZEOF_VOID_P EQUAL 4)
      )
    ADD_TEST_MACRO(VSMASM VSMASM)
  endif()

  if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    if(NOT MSVC60)
      ADD_TEST_MACRO(SBCS SBCS)
    endif()

    if(NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio [89]( |$)"
        AND NOT CMAKE_GENERATOR_TOOLSET)
      ADD_TEST_MACRO(VSWindowsFormsResx VSWindowsFormsResx)
    endif()

    add_test(VSExternalInclude ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/VSExternalInclude"
      "${CMake_BINARY_DIR}/Tests/VSExternalInclude"
      --build-two-config
      ${build_generator_args}
      --build-project VSExternalInclude
      --build-options ${build_options}
      --test-command VSExternalInclude)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSExternalInclude")

    add_test(VSMidl ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/VSMidl"
      "${CMake_BINARY_DIR}/Tests/VSMidl"
      --build-two-config
      ${build_generator_args}
      --build-project VSMidl
      --build-options ${build_options}
      --test-command VSMidl)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSMidl")

    if(CMake_TEST_DEVENV)
      # The test (and tested property) works with .sln files, so it's skipped when:
      # * cmake --build is set up to use MSBuild, since the MSBuild invocation does not use the .sln file
      set(_last_test "")
      foreach(config ${CMAKE_CONFIGURATION_TYPES})
        add_test(NAME VSExcludeFromDefaultBuild-${config} COMMAND ${CMAKE_CTEST_COMMAND}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/VSExcludeFromDefaultBuild"
          "${CMake_BINARY_DIR}/Tests/VSExcludeFromDefaultBuild"
          --build-config ${config}
          --build-two-config
          --build-generator ${CMAKE_GENERATOR}
          --build-makeprogram ${CMake_TEST_DEVENV}
          --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
          --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
          --build-project VSExcludeFromDefaultBuild
          --build-target install
          --test-command ${CMAKE_COMMAND}
             -D "activeConfig=${config}"
             -D "allConfigs=${CMAKE_CONFIGURATION_TYPES}"
             -D "dir=${CMake_BINARY_DIR}/Tests/VSExcludeFromDefaultBuild"
             -P "${CMake_SOURCE_DIR}/Tests/VSExcludeFromDefaultBuild/ResultTest.cmake")
        if(_last_test)
          set_property(TEST VSExcludeFromDefaultBuild-${config} PROPERTY DEPENDS ${_last_test})
        endif()
        set(_last_test "VSExcludeFromDefaultBuild-${config}")
      endforeach()
      unset(_last_test)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSExcludeFromDefaultBuild")
    endif()

    if(CMAKE_GENERATOR MATCHES "Visual Studio ([0-5]|[6-9][0-9])")
      # This is Visual Studio 10 or above, so the default build tool is MSBuild.
      add_test(NAME VSProjectInSubdir COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/VSProjectInSubdir"
        "${CMake_BINARY_DIR}/Tests/VSProjectInSubdir"
        --build-two-config
        --build-generator ${CMAKE_GENERATOR}
        --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
        --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
        --build-project VSProjectInSubdir
        --build-target test)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSProjectInSubdir")
    endif()
  endif()

  if(WIN32)
    # Macro to search for available Windows CE SDKs in the windows Registry
    macro(select_wince_sdk selected_reg selected_sdk)
      if(CMAKE_HOST_WIN32)
        execute_process(COMMAND reg QUERY "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows CE Tools\\SDKs"
                    OUTPUT_VARIABLE sdk_reg
                    ERROR_VARIABLE my_err)
        string(REGEX REPLACE "HKEY_LOCAL_MACHINE\\\\SOFTWARE\\\\Wow6432Node\\\\Microsoft\\\\Windows CE Tools\\\\SDKs\\\\"  ";" sdk_list "${sdk_reg}")
        list(LENGTH sdk_list sdk_list_len)
        if (${sdk_list_len} GREATER 1)
          list(GET sdk_list 1 _sdk) # The first entry is always empty due to the regex replace above
          string(STRIP ${_sdk} _sdk) # Make sure there is no newline in the SDK name
        endif()
        # Build a key to be used by get_filename_component that is pointing to the SDK directory
        set(_reg "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows CE Tools\\SDKs\\${_sdk}]")

        # Set return values
        set(${selected_reg} ${_reg})
        set(${selected_sdk} ${_sdk})
      endif(CMAKE_HOST_WIN32)
    endmacro(select_wince_sdk)

    set(reg_vs10 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0;InstallDir]")
    set(reg_vs11 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\11.0;InstallDir]")
    set(reg_vs12 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\12.0;InstallDir]")
    set(reg_vs14 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]")
    set(reg_ws80 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]")
    set(reg_ws81 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]")
    set(reg_ws10_0 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\14.0\\Setup\\Build Tools for Windows 10;srcPath]")
    set(reg_wp80 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\WindowsPhone\\v8.0;InstallationFolder]")
    set(reg_wp81 "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\WindowsPhone\\v8.1;InstallationFolder]")
    select_wince_sdk(reg_wince wince_sdk)
    set(reg_tegra "[HKEY_LOCAL_MACHINE\\SOFTWARE\\NVIDIA Corporation\\Nsight Tegra;sdkRoot]")
    set(reg_nasm "[HKEY_CURRENT_USER\\SOFTWARE\\nasm]")
    foreach(reg vs10 vs11 vs12 vs14 ws80 ws81 ws10_0 wp80 wp81 wince tegra nasm)
      get_filename_component(r "${reg_${reg}}" ABSOLUTE)
      if(IS_DIRECTORY "${r}")
        set(${reg} 1)
      else()
        set(${reg} 0)
      endif()
    endforeach()
  endif()

  get_filename_component(ntver "[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion;CurrentVersion]" NAME)
  if(WIN32 AND ntver VERSION_GREATER 6.1) # Windows >= 8.0
    macro(add_test_VSWinStorePhone name generator systemName systemVersion)
      add_test(NAME VSWinStorePhone.${name} COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/VSWinStorePhone"
        "${CMake_BINARY_DIR}/Tests/VSWinStorePhone/${name}"
        --build-generator "${generator}"
        --build-project VSWinStorePhone
        --build-config $<CONFIGURATION>
        --build-options -DCMAKE_SYSTEM_NAME=${systemName}
                        -DCMAKE_SYSTEM_VERSION=${systemVersion}
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSWinStorePhone/${name}")
    endmacro()

    if(vs11 AND ws80)
      add_test_VSWinStorePhone(vs11-store80-X86 "Visual Studio 11 2012" WindowsStore 8.0)
      add_test_VSWinStorePhone(vs11-store80-ARM "Visual Studio 11 2012 ARM" WindowsStore 8.0)
      add_test_VSWinStorePhone(vs11-store80-X64 "Visual Studio 11 2012 Win64" WindowsStore 8.0)
    endif()
    if(vs12 AND ws81)
      add_test_VSWinStorePhone(vs12-store81-X86 "Visual Studio 12 2013" WindowsStore 8.1)
      add_test_VSWinStorePhone(vs12-store81-ARM "Visual Studio 12 2013 ARM" WindowsStore 8.1)
      add_test_VSWinStorePhone(vs12-store81-X64 "Visual Studio 12 2013 Win64" WindowsStore 8.1)

      add_test(NAME VSXaml COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
          "${CMake_SOURCE_DIR}/Tests/VSXaml"
          "${CMake_BINARY_DIR}/Tests/VSXaml"
          --build-generator "Visual Studio 12 2013"
          --build-project VSXaml
          --build-config $<CONFIGURATION>
          --build-options -DCMAKE_SYSTEM_NAME=WindowsStore
                          -DCMAKE_SYSTEM_VERSION=8.1
        )
    endif()
    if(vs14 AND ws10_0)
      add_test_VSWinStorePhone(vs14-store10_0-X86 "Visual Studio 14 2015" WindowsStore 10.0)
      add_test_VSWinStorePhone(vs14-store10_0-ARM "Visual Studio 14 2015 ARM" WindowsStore 10.0)
      add_test_VSWinStorePhone(vs14-store10_0-X64 "Visual Studio 14 2015 Win64" WindowsStore 10.0)
    endif()
    if(vs11 AND wp80)
      add_test_VSWinStorePhone(vs11-phone80-X86 "Visual Studio 11 2012" WindowsPhone 8.0)
      add_test_VSWinStorePhone(vs11-phone80-ARM "Visual Studio 11 2012 ARM" WindowsPhone 8.0)
    endif()
    if(vs12 AND wp81)
      add_test_VSWinStorePhone(vs12-phone81-X86 "Visual Studio 12 2013" WindowsPhone 8.1)
      add_test_VSWinStorePhone(vs12-phone81-ARM "Visual Studio 12 2013 ARM" WindowsPhone 8.1)
    endif()
  endif()

  if(WIN32 AND wince)
    macro(add_test_VSWinCE name generator systemName systemVersion generatorPlatform)
      # TODO: Fix the tutorial to make it work in cross compile
      # currently the MakeTable is build for target and can not be used on the host
      # This happens in part 5 so we build only part 1-4 of the tutorial
      foreach(STP RANGE 1 4)
        add_test(NAME "TutorialStep${STP}.${name}" COMMAND ${CMAKE_CTEST_COMMAND}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/Tutorial/Step${STP}"
          "${CMake_BINARY_DIR}/Tests/Tutorial/Step${STP}_${name}"
          --build-generator "${generator}"
          --build-project Tutorial
          --build-config $<CONFIGURATION>
          --build-options -DCMAKE_SYSTEM_NAME=${systemName}
                          -DCMAKE_SYSTEM_VERSION=${systemVersion}
                          -DCMAKE_GENERATOR_PLATFORM=${generatorPlatform})
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Tutorial/Step${STP}_${name}")
      endforeach()
    endmacro()

    if(vs11)
      add_test_VSWinCE(vs11-ce80-ARM "Visual Studio 11 2012" WindowsCE 8.0 ${wince_sdk})
    endif()

    if(vs12)
      add_test_VSWinCE(vs12-ce80-ARM "Visual Studio 12 2013" WindowsCE 8.0 ${wince_sdk})
    endif()
  endif()

  if(CMAKE_GENERATOR MATCHES "Visual Studio ([^89]|[89][0-9])" AND nasm)
    ADD_TEST_MACRO(VSNASM VSNASM)
  endif()

  if (CMake_TEST_GreenHillsMULTI)
    macro(add_test_GhsMulti name primaryTarget bspName)
      add_test(NAME GhsMulti.${name} COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/GhsMulti"
        "${CMake_BINARY_DIR}/Tests/GhsMulti/${name}"
        --build-generator "Green Hills MULTI"
        --build-project ReturnNum
        --build-config $<CONFIGURATION>
        --build-options -DGHS_PRIMARY_TARGET=${primaryTarget}
        -DGHS_BSP_NAME=${bspName}
        )
    endmacro ()
    add_test_GhsMulti("arm_integrity_simarm" "arm_integrity.tgt" "simarm")
    add_test_GhsMulti("arm64_integrity_simarm" "arm64_integrity.tgt" "simarm")
    add_test(NAME GhsMulti.duplicate_source_filenames
        COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/GhsMultiDuplicateSourceFilenames"
        "${CMake_BINARY_DIR}/Tests/GhsMultiDuplicateSourceFilenames"
        --build-generator "Green Hills MULTI"
        --build-project ReturnNum
        --build-config $<CONFIGURATION>
        --build-options -DGHS_PRIMARY_TARGET="arm_integrity.tgt"
        -DGHS_BSP_NAME="simarm"
        )
  endif ()

  if(tegra AND NOT "${CMake_SOURCE_DIR};${CMake_BINARY_DIR}" MATCHES " ")
    macro(add_test_VSNsightTegra name generator)
      add_test(NAME VSNsightTegra.${name} COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/VSNsightTegra"
        "${CMake_BINARY_DIR}/Tests/VSNsightTegra/${name}"
        --build-generator "${generator}"
        --build-project VSNsightTegra
        --build-config $<CONFIGURATION>
        --build-options -DCMAKE_SYSTEM_NAME=Android
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/VSNsightTegra/${name}")
    endmacro()
    if(vs10)
      add_test_VSNsightTegra(vs10 "Visual Studio 10 2010")
    endif()
    if(vs11)
      add_test_VSNsightTegra(vs11 "Visual Studio 11 2012")
    endif()
    if(vs12)
      add_test_VSNsightTegra(vs12 "Visual Studio 12 2013")
    endif()
    if(vs14)
      add_test_VSNsightTegra(vs14 "Visual Studio 14 2015")
    endif()
  endif()

  if (APPLE)
    if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(BundleTestInstallDir
        "${CMake_BINARY_DIR}/Tests/BundleTest/InstallDirectory")
      add_test(BundleTest ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/BundleTest"
        "${CMake_BINARY_DIR}/Tests/BundleTest"
        --build-two-config
        ${build_generator_args}
        --build-project BundleTest
        --build-target install
#       --build-target package
        --build-options ${build_options}
        "-DCMAKE_INSTALL_PREFIX:PATH=${BundleTestInstallDir}"
        "-DCMake_SOURCE_DIR:PATH=${CMake_SOURCE_DIR}"
        --test-command
        ${BundleTestInstallDir}/Applications/SecondBundleExe.app/Contents/MacOS/SecondBundleExe)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/BundleTest")

      add_test(NAME CFBundleTest COMMAND ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/CFBundleTest"
        "${CMake_BINARY_DIR}/Tests/CFBundleTest"
        --build-two-config
        ${build_generator_args}
        --build-project CFBundleTest
        --build-config $<CONFIGURATION>
        --build-options ${build_options}
        --test-command
        ${CMAKE_CMAKE_COMMAND} -DCTEST_CONFIGURATION_TYPE=$<CONFIGURATION>
        -Ddir=${CMake_BINARY_DIR}/Tests/CFBundleTest
        -Dgen=${CMAKE_GENERATOR}
        -P ${CMake_SOURCE_DIR}/Tests/CFBundleTest/VerifyResult.cmake)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CFBundleTest")

      ADD_TEST_MACRO(ObjC++ ObjC++)
    endif ()
  endif ()

  if(APPLE AND CTEST_TEST_CPACK)
    add_test(BundleGeneratorTest ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/BundleGeneratorTest"
      "${CMake_BINARY_DIR}/Tests/BundleGeneratorTest"
      --build-two-config
      ${build_generator_args}
      --build-project BundleGeneratorTest
      --build-target package
      --build-options ${build_options}
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMake_BINARY_DIR}/Tests/BundleGeneratorTest/InstallDirectory"
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/BundleGeneratorTest")
  endif()

  add_test(WarnUnusedUnusedViaSet ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/VariableUnusedViaSet"
    "${CMake_BINARY_DIR}/Tests/WarnUnusedUnusedViaSet"
    ${build_generator_args}
    --build-noclean
    --build-project WarnUnusedUnusedViaSet
    --build-options ${build_options}
      "--warn-unused-vars")
  set_tests_properties(WarnUnusedUnusedViaSet PROPERTIES
    PASS_REGULAR_EXPRESSION "unused variable \\(changing definition\\) 'UNUSED_VARIABLE'")
  set_tests_properties(WarnUnusedUnusedViaSet PROPERTIES
    FAIL_REGULAR_EXPRESSION "unused variable \\(unsetting\\) 'UNUSED_VARIABLE'")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/WarnUnusedUnusedViaSet")

  add_test(WarnUnusedUnusedViaUnset ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/VariableUnusedViaUnset"
    "${CMake_BINARY_DIR}/Tests/WarnUnusedUnusedViaUnset"
    ${build_generator_args}
    --build-noclean
    --build-project WarnUnusedUnusedViaUnset
    --build-options ${build_options}
      "--warn-unused-vars")
  set_tests_properties(WarnUnusedUnusedViaUnset PROPERTIES
    PASS_REGULAR_EXPRESSION "CMake Warning \\(dev\\) at CMakeLists.txt:7 \\(set\\):")
  set_tests_properties(WarnUnusedUnusedViaUnset PROPERTIES
    FAIL_REGULAR_EXPRESSION "CMakeLists.txt:5 \\(set\\):")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/WarnUnusedUnusedViaUnset")

  if("${CMAKE_GENERATOR}" MATCHES "Makefile" AND NOT WIN32)
    # Ninja does not support ADDITIONAL_MAKE_CLEAN_FILES and therefore fails
    # this test. (See #13371)
    # Apparently Visual Studio does not support it either. As the MakeClean
    # test above is only run with the Makefiles generator, only run this
    # test with the Makefiles generator also.
    add_test(WarnUnusedCliUnused ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/WarnUnusedCliUnused"
      "${CMake_BINARY_DIR}/Tests/WarnUnusedCliUnused"
      ${build_generator_args}
      --build-project WarnUnusedCliUnused
      --build-options ${build_options}
        "-DUNUSED_CLI_VARIABLE=Unused")
    set_tests_properties(WarnUnusedCliUnused PROPERTIES
      PASS_REGULAR_EXPRESSION "CMake Warning:.*Manually-specified variables were not used by the project:.*  UNUSED_CLI_VARIABLE")
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/WarnUnusedCliUnused")
  endif()

  add_test(WarnUnusedCliUsed ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/VariableUsage"
    "${CMake_BINARY_DIR}/Tests/WarnUnusedCliUsed"
    ${build_generator_args}
    --build-noclean
    --build-project WarnUnusedCliUsed
    --build-options ${build_options}
      "-DUSED_VARIABLE=Usage proven")
  set_tests_properties(WarnUnusedCliUsed PROPERTIES
    PASS_REGULAR_EXPRESSION "Usage proven")
  set_tests_properties(WarnUnusedCliUsed PROPERTIES
    FAIL_REGULAR_EXPRESSION "CMake Warning: The variable, 'USED_VARIABLE'")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/WarnUnusedCliUsed")

  add_test(WarnUninitialized ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/VariableUsage"
    "${CMake_BINARY_DIR}/Tests/WarnUninitialized"
    ${build_generator_args}
    --build-noclean
    --build-project WarnUninitialized
    --build-options ${build_options}
      "--warn-uninitialized")
  set_tests_properties(WarnUninitialized PROPERTIES
    PASS_REGULAR_EXPRESSION "uninitialized variable 'USED_VARIABLE'")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/WarnUninitialized")

  add_test(TestsWorkingDirectory
    ${CMAKE_CTEST_COMMAND} -C \${CTEST_CONFIGURATION_TYPE}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/TestsWorkingDirectory"
    "${CMake_BINARY_DIR}/Tests/TestsWorkingDirectory"
    ${build_generator_args}
    --build-project TestsWorkingDirectoryProj
    --build-exe-dir "${CMake_BINARY_DIR}/Tests/TestsWorkingDirectory"
    --force-new-ctest-process
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V -C \${CTEST_CONFIGURATION_TYPE}
    )
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/TestsWorkingDirectory")

  # Make sure CTest can handle a test with no newline in output.
  add_test(CTest.NoNewline
    ${CMAKE_CMAKE_COMMAND} -E echo_append "This line has no newline!")

  # A simple test for ctest in script mode
  configure_file("${CMake_SOURCE_DIR}/Tests/CTestScriptMode/CTestTestScriptMode.cmake.in"
          "${CMake_BINARY_DIR}/Tests/CTestScriptMode/CTestTestScriptMode.cmake" @ONLY)
#  add_test(CTest.ScriptMode ${CMAKE_CTEST_COMMAND}
#        -S "${CMake_BINARY_DIR}/Tests/CTestScriptMode/CTestTestScriptMode.cmake"
#        )

  # A test for ctest_build() with targets in subdirectories
  set(ctest_configure_options)
  if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND ctest_configure_options -A ${CMAKE_GENERATOR_PLATFORM})
  endif()
  if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND ctest_configure_options -T ${CMAKE_GENERATOR_TOOLSET})
  endif()
  if(CMake_TEST_EXPLICIT_MAKE_PROGRAM)
    list(APPEND ctest_configure_options -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM})
  endif()
  configure_file("${CMake_SOURCE_DIR}/Tests/CTestBuildCommandProjectInSubdir/CTestBuildCommandProjectInSubdir.cmake.in"
                 "${CMake_BINARY_DIR}/Tests/CTestBuildCommandProjectInSubdir/CTestBuildCommandProjectInSubdir.cmake" @ONLY)
  unset(ctest_configure_options)
  add_test(CTest.BuildCommand.ProjectInSubdir
    ${CMAKE_CTEST_COMMAND} -S "${CMake_BINARY_DIR}/Tests/CTestBuildCommandProjectInSubdir/CTestBuildCommandProjectInSubdir.cmake")
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/CTestBuildCommandProjectInSubdir/Nested")

  set(CTEST_TEST_UPDATE 1)
  if(CTEST_TEST_UPDATE)
    # Test CTest Update with Subversion
    find_package(Subversion QUIET)
    if(Subversion_FOUND)
      get_filename_component(_Subversion_BIN_DIR
        ${Subversion_SVN_EXECUTABLE} PATH)
      find_program(Subversion_SVNADMIN_EXECUTABLE svnadmin
        HINTS ${_Subversion_BIN_DIR}
        )
      mark_as_advanced(Subversion_SVNADMIN_EXECUTABLE)
      if(NOT Subversion_SVNADMIN_EXECUTABLE)
        set(Subversion_FOUND FALSE)
      endif()
    endif()
    if(Subversion_FOUND)
      set(CTestUpdateSVN_DIR "CTest UpdateSVN")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateSVN.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestUpdateSVN.cmake" @ONLY)
      add_test(CTest.UpdateSVN ${CMAKE_CMAKE_COMMAND}
        -P "${CMake_BINARY_DIR}/Tests/CTestUpdateSVN.cmake"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateSVN_DIR}")
    endif()

    # Test CTest Update with CVS
    if(EXISTS ${CMAKE_ROOT}/Modules/FindCVS.cmake)
      find_package(CVS QUIET)
    else()
      find_program(CVS_EXECUTABLE NAMES cvs)
      set(CVS_FOUND ${CVS_EXECUTABLE})
    endif()
    set(CTEST_TEST_UPDATE_CVS ${CVS_FOUND})
    if(CTEST_TEST_UPDATE_CVS AND NOT UNIX)
      if("${CVS_EXECUTABLE}" MATCHES "cygwin")
        message(STATUS "No CTest.UpdateCVS test with cygwin cvs.exe outside cygwin!")
        set(CTEST_TEST_UPDATE_CVS 0)
      endif()
    endif()
    if(CTEST_TEST_UPDATE_CVS)
      set(CTestUpdateCVS_DIR "CTest UpdateCVS")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateCVS.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestUpdateCVS.cmake" @ONLY)
      add_test(CTest.UpdateCVS ${CMAKE_CMAKE_COMMAND}
        -P "${CMake_BINARY_DIR}/Tests/CTestUpdateCVS.cmake"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateCVS_DIR}")
    endif()

    # Test CTest Update with BZR
    find_program(BZR_EXECUTABLE NAMES bzr)
    mark_as_advanced(BZR_EXECUTABLE)
    set(CTEST_TEST_UPDATE_BZR 0)
    if(BZR_EXECUTABLE)
      if(NOT "${BZR_EXECUTABLE}" MATCHES "cygwin" OR UNIX)
        set(CTEST_TEST_UPDATE_BZR 1)
      endif()
    endif()
    if(CTEST_TEST_UPDATE_BZR)
      # Check if xmloutput plugin is there
      execute_process(COMMAND ${BZR_EXECUTABLE} xmlplugins RESULT_VARIABLE xmlplugres
        OUTPUT_QUIET ERROR_QUIET)
      if( NOT ${xmlplugres} )
        set(CTestUpdateBZR_DIR "CTest UpdateBZR")
        configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateBZR.cmake.in"
          "${CMake_BINARY_DIR}/Tests/CTestUpdateBZR.cmake" @ONLY)
        add_test(CTest.UpdateBZR ${CMAKE_CMAKE_COMMAND}
          -P "${CMake_BINARY_DIR}/Tests/CTestUpdateBZR.cmake"
          )
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateBZR_DIR}")
        set(CTestUpdateBZR_DIR "CTest UpdateBZR_CLocale")
        configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateBZR.cmake.in"
          "${CMake_BINARY_DIR}/Tests/CTestUpdateBZR_CLocale.cmake" @ONLY)
        add_test(CTest.UpdateBZR.CLocale ${CMAKE_CMAKE_COMMAND}
          -P "${CMake_BINARY_DIR}/Tests/CTestUpdateBZR_CLocale.cmake"
          )
        set_tests_properties(CTest.UpdateBZR.CLocale PROPERTIES ENVIRONMENT LC_ALL=C)
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateBZR_DIR}")
      endif()
    endif()

    # Test CTest Update with GIT
    find_program(GIT_EXECUTABLE NAMES git)
    mark_as_advanced(GIT_EXECUTABLE)
    set(CTEST_TEST_UPDATE_GIT 0)
    if(GIT_EXECUTABLE)
      if(NOT "${GIT_EXECUTABLE}" MATCHES "cygwin" OR UNIX)
        set(CTEST_TEST_UPDATE_GIT 1)
      endif()
    endif()
    if(CTEST_TEST_UPDATE_GIT)
      set(CTestUpdateGIT_DIR "CTest UpdateGIT")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateGIT.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestUpdateGIT.cmake" @ONLY)
      add_test(CTest.UpdateGIT ${CMAKE_CMAKE_COMMAND}
        -P "${CMake_BINARY_DIR}/Tests/CTestUpdateGIT.cmake"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateGIT_DIR}")
    endif()

    # Test CTest Update with HG
    find_program(HG_EXECUTABLE NAMES hg)
    mark_as_advanced(HG_EXECUTABLE)
    set(CTEST_TEST_UPDATE_HG 0)
    if(HG_EXECUTABLE)
      if(NOT "${HG_EXECUTABLE}" MATCHES "cygwin" OR UNIX)
        set(CTEST_TEST_UPDATE_HG 1)
      endif()
    endif()
    if(CTEST_TEST_UPDATE_HG)
      set(CTestUpdateHG_DIR "CTest UpdateHG")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateHG.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestUpdateHG.cmake" @ONLY)
      add_test(CTest.UpdateHG ${CMAKE_CMAKE_COMMAND}
        -P "${CMake_BINARY_DIR}/Tests/CTestUpdateHG.cmake"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateHG_DIR}")
    endif()

    # Test CTest Update with P4
    find_program(P4_EXECUTABLE NAMES p4)
    find_program(P4D_EXECUTABLE NAMES p4d)
    mark_as_advanced(P4_EXECUTABLE P4D_EXECUTABLE)
    set(CTEST_TEST_UPDATE_P4 0)
    if(P4_EXECUTABLE AND P4D_EXECUTABLE)
      if(NOT "${P4_EXECUTABLE};${P4D_EXECUTABLE}" MATCHES "cygwin" OR UNIX)
        set(CTEST_TEST_UPDATE_P4 1)
      endif()
    endif()
    if(CTEST_TEST_UPDATE_P4)
      set(CTestUpdateP4_DIR "CTest UpdateP4")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestUpdateP4.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestUpdateP4.cmake" @ONLY)
      add_test(CTest.UpdateP4 ${CMAKE_CMAKE_COMMAND}
        -P "${CMake_BINARY_DIR}/Tests/CTestUpdateP4.cmake"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/${CTestUpdateP4_DIR}")
    endif()
  endif()

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestFailure/testNoBuild.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testNoBuild.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestNoBuild ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testNoBuild.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testOut1.log"
    )
  set_tests_properties(CTestTestNoBuild PROPERTIES
    FAIL_REGULAR_EXPRESSION "Error" WILL_FAIL true)

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestFailure/testNoExe.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testNoExe.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestNoExe ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testNoExe.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestFailure/testOut2.log"
    )
  set_tests_properties(CTestTestNoExe PROPERTIES DEPENDS CTestTestNoBuild
    PASS_REGULAR_EXPRESSION "Could not find executable"
    FAIL_REGULAR_EXPRESSION "SegFault")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestUpload/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestUpload/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestUpload ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestUpload/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestUpload/testOut.log"
    )
  set_tests_properties(CTestTestUpload PROPERTIES
    PASS_REGULAR_EXPRESSION "Upload\\.xml")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestCoverageCollectGCOV/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestCoverageCollectGCOV/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestCoverageCollectGCOV ${CMAKE_CTEST_COMMAND}
    -C \${CTEST_CONFIGURATION_TYPE}
    -S "${CMake_BINARY_DIR}/Tests/CTestCoverageCollectGCOV/test.cmake" -VV
    --output-log "${CMake_BINARY_DIR}/Tests/CTestCoverageCollectGCOV/testOut.log"
    )
  set_tests_properties(CTestCoverageCollectGCOV PROPERTIES
    PASS_REGULAR_EXPRESSION
    "PASSED with correct output.*Testing/CoverageInfo/main.cpp.gcov")
  set_property(TEST CTestCoverageCollectGCOV PROPERTY ENVIRONMENT CTEST_PARALLEL_LEVEL=)

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestEmptyBinaryDirectory/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestEmptyBinaryDirectory/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestEmptyBinaryDirectory ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestEmptyBinaryDirectory/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestEmptyBinaryDirectory/testOut.log"
    )
  set_tests_properties(CTestTestEmptyBinaryDirectory PROPERTIES
    PASS_REGULAR_EXPRESSION "TEST_SUCCESS")

  # test coverage for mumps
  # create a MumpsCoverage dir in the binary tree under Testing to
  # avoid the .NoDartCoverage files in the cmake testing tree
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/MumpsCoverage/DartConfiguration.tcl")
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/gtm_coverage.mcov.in"
     "${CMake_BINARY_DIR}/Testing/MumpsCoverage/gtm_coverage.mcov")
  file(REMOVE_RECURSE "${CMake_BINARY_DIR}/Testing/MumpsCoverage/VistA-FOIA")
  file(COPY "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/VistA-FOIA"
    DESTINATION "${CMake_BINARY_DIR}/Testing/MumpsCoverage")
  add_test(NAME CTestGTMCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/MumpsCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestGTMCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*ZZCOVTST.m.*Total LOC:.*30.*Percentage Coverage: 80.00*"
      ENVIRONMENT COVFILE=)

  configure_file(
     "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/DartConfiguration.cache.tcl.in"
     "${CMake_BINARY_DIR}/Testing/MumpsCacheCoverage/DartConfiguration.tcl")
  configure_file(
    "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/cache_coverage.cmcov.in"
    "${CMake_BINARY_DIR}/Testing/MumpsCacheCoverage/cache_coverage.cmcov")
  file(REMOVE_RECURSE "${CMake_BINARY_DIR}/Testing/MumpsCacheCoverage/VistA-FOIA")
  file(COPY "${CMake_SOURCE_DIR}/Tests/MumpsCoverage/VistA-FOIA"
    DESTINATION "${CMake_BINARY_DIR}/Testing/MumpsCacheCoverage")
  add_test(NAME CTestCacheCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/MumpsCacheCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestCacheCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*ZZCOVTST.m.*Total LOC:.*29.*Percentage Coverage: 86.21.*"
      ENVIRONMENT COVFILE=)

  # Adding a test case for Python Coverage
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/PythonCoverage/coverage.xml.in"
     "${CMake_BINARY_DIR}/Testing/PythonCoverage/coverage.xml")
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/PythonCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/PythonCoverage/DartConfiguration.tcl")
  file(COPY "${CMake_SOURCE_DIR}/Tests/PythonCoverage/coveragetest"
    DESTINATION "${CMake_BINARY_DIR}/Testing/PythonCoverage")
  add_test(NAME CTestPythonCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/PythonCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestPythonCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*foo.py.*Total LOC:.*13.*Percentage Coverage: 84.62.*"
      ENVIRONMENT COVFILE=)

  # Adding a test case for non-python Cobertura Coverage
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/CoberturaCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/CoberturaCoverage/DartConfiguration.tcl")
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/CoberturaCoverage/coverage.xml.in"
     "${CMake_BINARY_DIR}/Testing/CoberturaCoverage/coverage.xml")
  file(COPY "${CMake_SOURCE_DIR}/Tests/CoberturaCoverage/src"
    DESTINATION "${CMake_BINARY_DIR}/Testing/CoberturaCoverage")
  add_test(NAME CTestCoberturaCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/CoberturaCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestCoberturaCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*CoverageTest.java.*Total LOC:.*18.*Percentage Coverage: 72.22.*"
      ENVIRONMENT COBERTURADIR=${CMake_BINARY_DIR}/Testing/CoberturaCoverage
      ENVIRONMENT COVFILE=)


  # Adding a test case for JaCoCo Coverage
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/JacocoCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/JacocoCoverage/DartConfiguration.tcl")
  file(COPY "${CMake_SOURCE_DIR}/Tests/JacocoCoverage/Coverage"
    DESTINATION "${CMake_BINARY_DIR}/Testing/JacocoCoverage")
  configure_file("${CMake_BINARY_DIR}/Testing/JacocoCoverage/Coverage/target/site/jacoco.xml.in"
    "${CMake_BINARY_DIR}/Testing/JacocoCoverage/Coverage/target/site/jacoco.xml")
  add_test(NAME CTestJacocoCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/JacocoCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestJacocoCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*CoverageTest.java.*Total LOC:.*17.*Percentage Coverage: 76.47*"
      ENVIRONMENT COVFILE=)

  # Adding a test case for Javascript Coverage
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/JavascriptCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/JavascriptCoverage/DartConfiguration.tcl")
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/JavascriptCoverage/output.json.in"
     "${CMake_BINARY_DIR}/Testing/JavascriptCoverage/output.json")
  file(COPY "${CMake_SOURCE_DIR}/Tests/JavascriptCoverage/"
    DESTINATION "${CMake_BINARY_DIR}/Testing/JavascriptCoverage"
    FILES_MATCHING PATTERN "*.js")
  add_test(NAME CTestJavascriptCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/JavascriptCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestJavascriptCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*test3.js.*Total LOC:.*49.*Percentage Coverage: 79.59*"
      ENVIRONMENT COVFILE=)

  # test coverage for Delphi-code-Coverage
  configure_file(
     "${CMake_SOURCE_DIR}/Tests/DelphiCoverage/DartConfiguration.tcl.in"
     "${CMake_BINARY_DIR}/Testing/DelphiCoverage/DartConfiguration.tcl")
  file(COPY "${CMake_SOURCE_DIR}/Tests/DelphiCoverage/src"
    DESTINATION "${CMake_BINARY_DIR}/Testing/DelphiCoverage")
  configure_file(
    "${CMake_SOURCE_DIR}/Tests/DelphiCoverage/UTCovTest(UTCovTest.pas).html.in"
    "${CMake_BINARY_DIR}/Testing/DelphiCoverage/UTCovTest(UTCovTest.pas).html")
  add_test(NAME CTestDelphiCoverage
    COMMAND cmake -E chdir
    ${CMake_BINARY_DIR}/Testing/DelphiCoverage
    $<TARGET_FILE:ctest> -T Coverage --debug)
  set_tests_properties(CTestDelphiCoverage PROPERTIES
      PASS_REGULAR_EXPRESSION
      "Process file.*UTCovTest.pas.*Total LOC:.*20.*Percentage Coverage: 95.*"
      ENVIRONMENT COVFILE=)

  function(add_config_tests cfg)
    set(base "${CMake_BINARY_DIR}/Tests/CTestConfig")

    # Test -S script with a -C config arg to ctest:
    configure_file(
      "${CMake_SOURCE_DIR}/Tests/CTestConfig/script.cmake.in"
      "${base}/${cfg}-script.cmake"
      @ONLY ESCAPE_QUOTES)
    add_test(CTestConfig.Script.${cfg} ${CMAKE_CTEST_COMMAND}
      -C ${cfg}
      -S "${base}/${cfg}-script.cmake" -VV
      --output-log "${base}/${cfg}-script.log"
      )

    # Test -D dashboard with a -C config arg to ctest.
    # (Actual commands inside a cmake -P script because we need to be able to set
    #  the working directory reliably...)
    configure_file(
      "${CMake_SOURCE_DIR}/Tests/CTestConfig/dashboard.cmake.in"
      "${base}/${cfg}-dashboard.cmake"
      @ONLY ESCAPE_QUOTES)
    add_test(CTestConfig.Dashboard.${cfg} ${CMAKE_CMAKE_COMMAND}
      -P "${base}/${cfg}-dashboard.cmake" -VV
      )
  endfunction()

  add_config_tests(Debug)
  add_config_tests(MinSizeRel)
  add_config_tests(Release)
  add_config_tests(RelWithDebInfo)

  # Test -S script with some -D variable definition args to ctest:
  add_test(CTestConfig.ScriptWithArgs ${CMAKE_CTEST_COMMAND}
    -C "Release"
    -D arg1=this
    -D arg2=that
    -D "arg3=the other"
    "-Darg4=this is the fourth"
    -Darg5=the_fifth
    -Darg6:STRING=value-with-type
    -S "${CMake_SOURCE_DIR}/Tests/CTestConfig/ScriptWithArgs.cmake" -VV
    --output-log "${CMake_BINARY_DIR}/Tests/CTestConfig/ScriptWithArgs.log"
    )

  ADD_TEST_MACRO(CMakeCommands.add_compile_options add_compile_options)
  ADD_TEST_MACRO(CMakeCommands.target_link_libraries target_link_libraries)
  ADD_TEST_MACRO(CMakeCommands.target_include_directories target_include_directories)
  ADD_TEST_MACRO(CMakeCommands.target_compile_definitions target_compile_definitions)
  ADD_TEST_MACRO(CMakeCommands.target_compile_options target_compile_options)

  if(CMake_TEST_SERVER_MODE)
    # The cmake server-mode test requires python for a simple client.
    find_package(PythonInterp QUIET)
    if(PYTHON_EXECUTABLE)
      set(Server_BUILD_OPTIONS -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE})
      ADD_TEST_MACRO(Server Server)
    endif()
  endif()

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestCrash/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestCrash/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestCrash ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestCrash/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestCrash/testOutput.log"
    )
  # with watcom the SEGFAULT is not found, it just fails
  if(CMAKE_GENERATOR MATCHES "Watcom WMake")
    set_tests_properties(CTestTestCrash PROPERTIES
      PASS_REGULAR_EXPRESSION "Failed")
  else()
    set_tests_properties(CTestTestCrash PROPERTIES
      PASS_REGULAR_EXPRESSION "(Illegal|SegFault)")
  endif()

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestBadExe/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestBadExe/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestBadExe ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestBadExe/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestBadExe/testOutput.log"
    )
  set(CTestTestBadExe_REGEX "BAD_COMMAND")
  # some cygwin can not be made to produce a BAD_COMMAND error
  # the best we can get from it is a failed test
  if(CYGWIN)
    set(CTestTestBadExe_REGEX "(\\*\\*\\*Failed)|BAD_COMMAND")
  endif()
  set_tests_properties(CTestTestBadExe PROPERTIES
    PASS_REGULAR_EXPRESSION "${CTestTestBadExe_REGEX}")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestBadGenerator/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestBadGenerator/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestBadGenerator ${CMAKE_CTEST_COMMAND}
    -C "\${CTestTest_CONFIG}"
    -S "${CMake_BINARY_DIR}/Tests/CTestTestBadGenerator/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestBadGenerator/testOutput.log"
    )
  set_property(TEST CTestTestBadGenerator PROPERTY
    PASS_REGULAR_EXPRESSION "could not create generator named \"Bad Generator\"")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestParallel/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestParallel/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestParallel ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestParallel/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestParallel/testOutput.log"
    )

  configure_file("${CMake_SOURCE_DIR}/Tests/CTestTestVerboseOutput/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestVerboseOutput/test.cmake" @ONLY ESCAPE_QUOTES)
  add_test(CTestTestVerboseOutput ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestVerboseOutput/test.cmake" -VV
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestVerboseOutput/testOutput.log"
    -C "\${CTestTest_CONFIG}"
    )
  set_property(TEST CTestTestVerboseOutput PROPERTY PASS_REGULAR_EXPRESSION
    "Environment variables:.*foo=bar.*this=that"
  )

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestSkipReturnCode/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestSkipReturnCode/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestSkipReturnCode ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestSkipReturnCode/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestSkipReturnCode/testOutput.log"
    -C \${CTEST_CONFIGURATION_TYPE}
    )
  set_tests_properties(CTestTestSkipReturnCode PROPERTIES
    PASS_REGULAR_EXPRESSION "CMakeV1 \\.* +Passed.*CMakeV2 \\.+\\*+Skipped")
  set_property(TEST CTestTestSkipReturnCode PROPERTY ENVIRONMENT CTEST_PARALLEL_LEVEL=)

  ADD_TEST_MACRO(CTestTestSerialInDepends ${CMAKE_CTEST_COMMAND} -j 4
    --output-on-failure -C "\${CTestTest_CONFIG}")

  ADD_TEST_MACRO(CTestTestMissingDependsExe ${CMAKE_CTEST_COMMAND}
    --output-on-failure -C "\${CTestTest_CONFIG}")
  set_tests_properties(CTestTestMissingDependsExe PROPERTIES
    PASS_REGULAR_EXPRESSION "\\*\\*\\*Not Run"
  )

  ADD_TEST_MACRO(CTestTestSerialOrder ${CMAKE_CTEST_COMMAND}
    --output-on-failure -C "\${CTestTest_CONFIG}")
  set_property(TEST CTestTestSerialOrder PROPERTY ENVIRONMENT CTEST_PARALLEL_LEVEL=)

  if(NOT BORLAND)
    set(CTestLimitDashJ_CTEST_OPTIONS --force-new-ctest-process)
    add_test_macro(CTestLimitDashJ ${CMAKE_CTEST_COMMAND} -j 4
      --output-on-failure -C "\${CTestTest_CONFIG}")
  endif()

  add_test(CTestTestPrintLabels ${CMAKE_CTEST_COMMAND} --print-labels)
  set_tests_properties(CTestTestPrintLabels PROPERTIES LABELS "Label1;Label2")
  set_tests_properties(CTestTestPrintLabels PROPERTIES PASS_REGULAR_EXPRESSION
    "All Labels:.*  Label1.*  Label2")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestLabelRegExp/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestLabelRegExp/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(NAME CTestTestLabelRegExp
    COMMAND ${CMAKE_CMAKE_COMMAND}
      -DSOURCE_DIR=${CMAKE_SOURCE_DIR}/Tests/CTestTestLabelRegExp
      -P ${CMAKE_BINARY_DIR}/Tests/CTestTestLabelRegExp/test.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Tests/CTestTestLabelRegExp
  )

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestResourceLock/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestResourceLock/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestResourceLock ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestResourceLock/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestResourceLock/output.log"
    )

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestScheduler/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestScheduler/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestScheduler ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestScheduler/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestScheduler/testOutput.log"
    )
  set_tests_properties(CTestTestScheduler PROPERTIES
    PASS_REGULAR_EXPRESSION "Start 1.*Start 2.*Start 3.*Start 4.*Start 4.*Start 3.*Start 2.*Start 1"
    RESOURCE_LOCK "CostData")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestCostSerial/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestCostSerial/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestCostSerial ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestCostSerial/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestCostSerial/testOutput.log"
    )
  set_tests_properties(CTestTestCostSerial PROPERTIES
    PASS_REGULAR_EXPRESSION "Start 2.*Start 3.*Start 1.*Start 2.*Start 3.*Start 1"
    RESOURCE_LOCK "CostData")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestStopTime/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestStopTime/test.cmake"
    @ONLY ESCAPE_QUOTES)
  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestStopTime/GetDate.cmake"
    "${CMake_BINARY_DIR}/Tests/CTestTestStopTime/GetDate.cmake"
    COPYONLY)
  add_test(CTestTestStopTime ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestStopTime/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestStopTime/testOutput.log"
    )
  set_tests_properties(CTestTestStopTime PROPERTIES
    PASS_REGULAR_EXPRESSION "The stop time has been passed")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestSubdir/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestSubdir/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestSubdir ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestSubdir/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestSubdir/testOutput.log"
    )
  #make sure all 3 subdirs were added
  set_tests_properties(CTestTestSubdir PROPERTIES
    PASS_REGULAR_EXPRESSION "0 tests failed out of 3")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestTimeout/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestTimeout/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestTimeout ${CMAKE_CTEST_COMMAND}
    -C "\${CTestTest_CONFIG}"
    -S "${CMake_BINARY_DIR}/Tests/CTestTestTimeout/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestTimeout/testOutput.log"
    )
  set_tests_properties(CTestTestTimeout PROPERTIES
    PASS_REGULAR_EXPRESSION "TestTimeout *\\.+ *\\*\\*\\*Timeout.*TestSleep *\\.+ *Passed.*timeout correctly killed child")

  add_test(
    NAME CTestTestRerunFailed
    COMMAND ${CMAKE_CTEST_COMMAND} --rerun-failed)
  set_tests_properties(CTestTestRerunFailed PROPERTIES
    PASS_REGULAR_EXPRESSION "1/1 Test #1: TestTimeout" DEPENDS CTestTestTimeout
    WORKING_DIRECTORY ${CMake_BINARY_DIR}/Tests/CTestTestTimeout)

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestZeroTimeout/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestZeroTimeout/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestZeroTimeout ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestZeroTimeout/test.cmake" -V
    --output-log
    "${CMake_BINARY_DIR}/Tests/CTestTestZeroTimeout/testOutput.log")
  set_tests_properties(CTestTestZeroTimeout PROPERTIES
    FAIL_REGULAR_EXPRESSION "\\*\\*\\*Timeout")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestDepends/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestDepends/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestDepends ${CMAKE_CTEST_COMMAND}
    -C "\${CTestTest_CONFIG}"
    -S "${CMake_BINARY_DIR}/Tests/CTestTestDepends/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestDepends/testOutput.log"
    )

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestCycle/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestCycle/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestCycle ${CMAKE_CTEST_COMMAND}
    -C "\${CTestTest_CONFIG}"
    -S "${CMake_BINARY_DIR}/Tests/CTestTestCycle/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestCycle/testOutput.log"
    )
  set_tests_properties(CTestTestCycle PROPERTIES
    PASS_REGULAR_EXPRESSION "a cycle exists in the test dependency graph")

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestRunScript/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestRunScript/test.cmake"
    @ONLY ESCAPE_QUOTES)
  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestRunScript/hello.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestRunScript/hello.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestRunScript ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestRunScript/test.cmake" -V
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestRunScript/testOutput.log"
    )

  add_test(CTestTestShowOnly ${CMAKE_CTEST_COMMAND} -N)

  add_test(CTestBatchTest ${CMAKE_CTEST_COMMAND} -B)

  configure_file(
    "${CMake_SOURCE_DIR}/Tests/CTestTestFdSetSize/test.cmake.in"
    "${CMake_BINARY_DIR}/Tests/CTestTestFdSetSize/test.cmake"
    @ONLY ESCAPE_QUOTES)
  add_test(CTestTestFdSetSize ${CMAKE_CTEST_COMMAND}
    -S "${CMake_BINARY_DIR}/Tests/CTestTestFdSetSize/test.cmake" -j20 -V --timeout 120
    --output-log "${CMake_BINARY_DIR}/Tests/CTestTestFdSetSize/testOutput.log"
    )

  if (CMAKE_TESTS_CDASH_SERVER)
    set(regex "^([^:]+)://([^/]+)(.*)$")

    if ("${CMAKE_TESTS_CDASH_SERVER}" MATCHES "${regex}")
      set(protocol "${CMAKE_MATCH_1}")
      set(server "${CMAKE_MATCH_2}")
      set(path "${CMAKE_MATCH_3}")
    else ()
      set(protocol "http")
      set(server "open.cdash.org")
      set(path "")
      message("warning: CMAKE_TESTS_CDASH_SERVER does not match expected regex...")
      message("         ...using default url='${protocol}://${server}${path}' for CTestTest[23]")
    endif ()
  endif ()


  if (CTEST_TEST_CTEST AND CMAKE_RUN_LONG_TESTS AND CMAKE_TESTS_CDASH_SERVER)
    configure_file("${CMake_SOURCE_DIR}/Tests/CTestTest/test.cmake.in"
      "${CMake_BINARY_DIR}/Tests/CTestTest/test.cmake" @ONLY ESCAPE_QUOTES)
    add_test(CTestTest ${CMAKE_CTEST_COMMAND}
      -S "${CMake_BINARY_DIR}/Tests/CTestTest/test.cmake" -V
      --output-log "${CMake_BINARY_DIR}/Tests/CTestTest/testOutput.log"
      )

    configure_file("${CMake_SOURCE_DIR}/Tests/CTestTest2/test.cmake.in"
      "${CMake_BINARY_DIR}/Tests/CTestTest2/test.cmake" @ONLY ESCAPE_QUOTES)
    add_test(CTestTest2 ${CMAKE_CTEST_COMMAND}
      -S "${CMake_BINARY_DIR}/Tests/CTestTest2/test.cmake" -V
      --output-log "${CMake_BINARY_DIR}/Tests/CTestTest2/testOutput.log"
      )

    if("${CMAKE_GENERATOR}" MATCHES "Makefiles" OR "${CMAKE_GENERATOR}" MATCHES "Ninja")
      configure_file("${CMake_SOURCE_DIR}/Tests/CTestTestLaunchers/test.cmake.in"
        "${CMake_BINARY_DIR}/Tests/CTestTestLaunchers/test.cmake" @ONLY ESCAPE_QUOTES)
      add_test(CTestTestLaunchers ${CMAKE_CTEST_COMMAND}
        -S "${CMake_BINARY_DIR}/Tests/CTestTestLaunchers/test.cmake" -V
        --output-log "${CMake_BINARY_DIR}/Tests/CTestTestLaunchers/testOutput.log"
        )
      set_tests_properties(CTestTestLaunchers PROPERTIES
        PASS_REGULAR_EXPRESSION "CTEST_TEST_LAUNCHER_SUCCESS")
    endif()

    configure_file("${CMake_SOURCE_DIR}/Tests/CTestTestChecksum/test.cmake.in"
      "${CMake_BINARY_DIR}/Tests/CTestTestChecksum/test.cmake" @ONLY
      ESCAPE_QUOTES)
    add_test(CTestTestChecksum ${CMAKE_CTEST_COMMAND}
      -S "${CMake_BINARY_DIR}/Tests/CTestTestChecksum/test.cmake" -V
      --output-log
      "${CMake_BINARY_DIR}/Tests/CTestTestChecksum/testOutput.log"
      )
    set_tests_properties(CTestTestChecksum PROPERTIES PASS_REGULAR_EXPRESSION
      "Submission failed: Checksum failed for file")

    # these tests take a long time, make sure they have it
    # if timeouts have not already been set
    get_test_property(CTestTest TIMEOUT PREVIOUS_TIMEOUT)
    if ("${PREVIOUS_TIMEOUT}" MATCHES NOTFOUND)
      set_tests_properties ( CTestTest
        PROPERTIES TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})
    endif ()

    get_test_property(CTestTest2 TIMEOUT PREVIOUS_TIMEOUT)
    if ("${PREVIOUS_TIMEOUT}" MATCHES NOTFOUND)
      set_tests_properties ( CTestTest2
        PROPERTIES TIMEOUT ${CMAKE_LONG_TEST_TIMEOUT})
    endif ()
  endif ()

  if(CMake_TEST_EXTERNAL_CMAKE)
    set(CMAKE_SKIP_BOOTSTRAP_TEST 1)
  endif()
  if("${CMAKE_GENERATOR}" MATCHES Xcode)
    set(CMAKE_SKIP_BOOTSTRAP_TEST 1)
  endif()
  if(EXISTS "${CMake_BINARY_DIR}/CMakeLists.txt")
    # If there is CMakeLists.txt in the binary tree, assume in-source build
    set(CMAKE_SKIP_BOOTSTRAP_TEST 1)
  endif()
  set(bootstrap "")
  if(CMAKE_RUN_LONG_TESTS AND NOT CMAKE_SKIP_BOOTSTRAP_TEST)
    if(UNIX)
      set(bootstrap ${CMake_SOURCE_DIR}/bootstrap)
    elseif(MSYS)
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.bat.in
                     ${CMAKE_CURRENT_BINARY_DIR}/bootstrap.bat @ONLY)
      set(bootstrap ${CMAKE_CURRENT_BINARY_DIR}/bootstrap.bat)
    endif()
  endif()
  if(bootstrap)
    add_test(NAME BootstrapTest
      COMMAND ${CMAKE_CMAKE_COMMAND}
        -D "bootstrap=${bootstrap}"
        -D "bin_dir=${CMake_BINARY_DIR}/Tests/BootstrapTest"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/BootstrapTest.cmake
      )
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/BootstrapTest")
    # This test will use all processors.
    set_tests_properties(BootstrapTest PROPERTIES RUN_SERIAL 1)

    # provide more time for the bootstrap test
    get_test_property(BootstrapTest TIMEOUT PREVIOUS_TIMEOUT)
    if ("${PREVIOUS_TIMEOUT}" MATCHES NOTFOUND)
      set_tests_properties ( BootstrapTest
        PROPERTIES TIMEOUT 5400)
    endif ()
  endif()

  if(CMAKE_Fortran_COMPILER)
    add_test(Fortran ${CMAKE_CTEST_COMMAND}
      --build-and-test
      "${CMake_SOURCE_DIR}/Tests/Fortran"
      "${CMake_BINARY_DIR}/Tests/Fortran"
      ${build_generator_args}
      --build-project testf
      --build-two-config
      --build-options ${build_options}
      --test-command testf)
    list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Fortran")

    if(CMAKE_Fortran_COMPILER_SUPPORTS_F90)
      add_test(FortranModules ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/FortranModules"
        "${CMake_BINARY_DIR}/Tests/FortranModules"
        ${build_generator_args}
        --build-project FortranModules
        --build-options ${build_options}
          -DCMake_TEST_NESTED_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM}
          -DCMake_TEST_Fortran_SUBMODULES:BOOL=${CMake_TEST_Fortran_SUBMODULES}
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/FortranModules")
    endif()

    # FortranCInterface tests.
    if(UNIX)
      configure_file(${CMAKE_CURRENT_SOURCE_DIR}/FortranC/Flags.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/FortranC/Flags.cmake @ONLY)
      add_test(FortranC.Flags ${CMAKE_CMAKE_COMMAND} -P
        ${CMAKE_CURRENT_BINARY_DIR}/FortranC/Flags.cmake)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/FortranC/Flags")
    else()
      add_test(FortranC ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/FortranC"
        "${CMake_BINARY_DIR}/Tests/FortranC"
        ${build_generator_args}
        --build-project FortranC
        --build-two-config
        --build-options ${build_options}
        --test-command CMakeFiles/FortranCInterface/FortranCInterface)
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/FortranC")
    endif()
  endif()

  find_package(Java COMPONENTS Development QUIET)
  if(Java_JAVA_EXECUTABLE AND Java_JAVAC_EXECUTABLE AND Java_JAR_EXECUTABLE AND NOT MINGW
      AND NOT "${CMAKE_GENERATOR}" MATCHES "Xcode")

    set(JavaExportImport_BUILD_OPTIONS -DCMake_TEST_NESTED_MAKE_PROGRAM:FILEPATH=${CMake_TEST_EXPLICIT_MAKE_PROGRAM})
    ADD_TEST_MACRO(JavaExportImport JavaExportImport)

    get_filename_component(JNIPATH ${JAVA_COMPILE} PATH)
    find_file(JNI_H jni.h
      "${JNIPATH}/../include"
      "${JNIPATH}/../java/include")
    if(JNI_H AND EXISTS "${JNI_H}") # in case jni.h is a broken symlink
      file(READ "${JNI_H}" JNI_FILE)
      if("${JNI_FILE}" MATCHES "JDK1_2")
        add_test(Java.Jar ${CMAKE_CTEST_COMMAND}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/Java"
          "${CMake_BINARY_DIR}/Tests/JavaJar"
          ${build_generator_args}
          --build-project hello
          --build-target hello
          --build-two-config
          --build-run-dir "${CMake_BINARY_DIR}/Tests/JavaJar/"
          --build-options ${build_options}
          --test-command ${JAVA_RUNTIME} -classpath hello.jar HelloWorld)
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/JavaJar")
        add_test(Java.JarSourceList ${CMAKE_CTEST_COMMAND}
          --build-and-test
          "${CMake_SOURCE_DIR}/Tests/Java"
          "${CMake_BINARY_DIR}/Tests/JavaJarSourceList"
          ${build_generator_args}
          --build-project hello
          --build-target hello2
          --build-two-config
          --build-run-dir "${CMake_BINARY_DIR}/Tests/JavaJarSourceList/"
          --build-options ${build_options}
          --test-command ${JAVA_RUNTIME} -classpath hello2.jar HelloWorld)
        list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/JavaJarSourceList")

        # For next test, java tool must have same architecture as toolchain
        math(EXPR _object_mode "${CMAKE_SIZEOF_VOID_P} * 8")
        execute_process(
          COMMAND "${Java_JAVA_EXECUTABLE}" -d${_object_mode} -version
          OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE _result
          )
        if(_result EQUAL 0)
          if(CMAKE_CONFIGURATION_TYPES)
            set (JAVAH_LIBRARY_PATH ${CMake_BINARY_DIR}/Tests/JavaJavah/$<CONFIGURATION>)
          else()
            set (JAVAH_LIBRARY_PATH ${CMake_BINARY_DIR}/Tests/JavaJavah)
          endif()
          add_test(NAME Java.Javah COMMAND ${CMAKE_CTEST_COMMAND}
            --build-and-test
            "${CMake_SOURCE_DIR}/Tests/JavaJavah"
            "${CMake_BINARY_DIR}/Tests/JavaJavah"
            ${build_generator_args}
            --build-project helloJavah
            --build-two-config
            --build-run-dir "${CMake_BINARY_DIR}/Tests/JavaJavah/"
            --build-options ${build_options}
            --test-command ${JAVA_RUNTIME} -Djava.library.path=${JAVAH_LIBRARY_PATH} -classpath hello3.jar HelloWorld2)
          list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/JavaJavah")
        endif()
      endif()
    endif()
  endif()

  # add some cross compiler tests, for now only with makefile based generators
  if(CMAKE_GENERATOR MATCHES "Makefiles" OR CMAKE_GENERATOR MATCHES "KDevelop")

    # if sdcc is found, build the SimpleCOnly project with sdcc
    find_program(SDCC_EXECUTABLE sdcc)
    mark_as_advanced(SDCC_EXECUTABLE)
    if(SDCC_EXECUTABLE)
      add_test(SimpleCOnly_sdcc ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/SimpleCOnly"
        "${CMake_BINARY_DIR}/Tests/SimpleCOnly_sdcc"
        ${build_generator_args}
        --build-project SimpleC
        --build-options ${build_options}
        "-DCMAKE_SYSTEM_NAME=Generic"
        "-DCMAKE_C_COMPILER=${SDCC_EXECUTABLE}")
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/SimpleCOnly_sdcc")
    endif()

    # If a Linux -> MinGW cross compiler is found then try it
    find_program(MINGW_CC_LINUX2WIN_EXECUTABLE i586-mingw32msvc-gcc)
    find_program(MINGW_CXX_LINUX2WIN_EXECUTABLE i586-mingw32msvc-g++)
    find_program(MINGW_RC_LINUX2WIN_EXECUTABLE i586-mingw32msvc-windres)
    mark_as_advanced(MINGW_CC_LINUX2WIN_EXECUTABLE MINGW_CXX_LINUX2WIN_EXECUTABLE MINGW_RC_LINUX2WIN_EXECUTABLE)
    if(MINGW_CC_LINUX2WIN_EXECUTABLE AND MINGW_CXX_LINUX2WIN_EXECUTABLE AND MINGW_RC_LINUX2WIN_EXECUTABLE)
      add_test(Simple_Mingw_Linux2Win ${CMAKE_CTEST_COMMAND}
        --build-and-test
        "${CMake_SOURCE_DIR}/Tests/Simple"
        "${CMake_BINARY_DIR}/Tests/Simple_Mingw_Linux2Win"
        ${build_generator_args}
        --build-project Simple
        --build-options ${build_options}
        "-DCMAKE_SYSTEM_NAME=Windows"
        "-DCMAKE_C_COMPILER=${MINGW_CC_LINUX2WIN_EXECUTABLE}"
        "-DCMAKE_CXX_COMPILER=${MINGW_CXX_LINUX2WIN_EXECUTABLE}"
        "-DCMAKE_RC_COMPILER=${MINGW_RC_LINUX2WIN_EXECUTABLE}"
        )
      list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/Simple_Mingw_Linux2Win")
    endif()
  endif()

  if(CMAKE_TEST_PROJECT_CSE_DIR)
    set(script "${CMAKE_TEST_PROJECT_CSE_DIR}/BuildProjectCSE.cmake")
    if(NOT EXISTS "${script}")
      set(script "${CMAKE_TEST_PROJECT_CSE_DIR}/cse_build.cmake")
    endif()
    if(NOT EXISTS "${script}")
      message("warning: CMAKE_TEST_PROJECT_CSE_DIR set, but no build script found...")
    endif()

    add_test(BuildCSE ${CMAKE_CTEST_COMMAND} -V -S "${script}")
    set_tests_properties(BuildCSE PROPERTIES TIMEOUT 5400)
  endif()

  if(CMAKE_TEST_PLPLOT_DIR)
    add_test(plplot ${CMAKE_CTEST_COMMAND} -V -S ${CMAKE_TEST_PLPLOT_DIR}/../../EasyDashboardScripts/plplot.cmake )
    set_tests_properties ( plplot PROPERTIES TIMEOUT 5400)
  endif()

  if(CMAKE_TEST_CHICKEN_DIR)
    add_test(Chicken ${CMAKE_CTEST_COMMAND} -V -S ${CMAKE_TEST_CHICKEN_DIR}/../../EasyDashboardScripts/Chicken.cmake )
    set_tests_properties ( Chicken PROPERTIES TIMEOUT 5400)
  endif()

  if(CMAKE_TEST_KDELIBS_ALPHA_1_DIR)
    add_test(KDELibsAlpha1 ${CMAKE_CTEST_COMMAND} -V -S ${CMAKE_TEST_KDELIBS_ALPHA_1_DIR}/../../EasyDashboardScripts/kdelibs.cmake )
    set_tests_properties ( KDELibsAlpha1 PROPERTIES TIMEOUT 5400)
  endif()

  # If this is not an in-source build, provide a target to wipe out
  # all the test build directories.
  if(NOT EXISTS "${CMake_BINARY_DIR}/CMakeLists.txt")
    configure_file(${CMake_SOURCE_DIR}/Tests/test_clean.cmake.in
                   ${CMake_BINARY_DIR}/Tests/test_clean.cmake @ONLY)
    add_custom_target(test_clean
      COMMAND ${CMAKE_COMMAND} -P ${CMake_BINARY_DIR}/Tests/test_clean.cmake
      COMMENT "Removing test build directories."
      )
  endif()

  add_test(NAME CMakeWizardTest COMMAND cmake -i)
  set_property(TEST CMakeWizardTest PROPERTY PASS_REGULAR_EXPRESSION
    "The \"cmake -i\" wizard mode is no longer supported.")

  # If the cache variable CMAKE_CONTRACT_PROJECTS is set
  # then the dashboard will run a contract with CMake test of that
  # name.  For example CMAKE_CONTRACT_PROJECTS = vtk542 would run
  # the vtk542 contract test.
  # For each Contract test, the project should provide a directory
  # with at least one CMakeLists.txt file that uses ExternalProject
  # to download and configure the project. The directory should also
  # contain a RunTest.cmake file that has a single set of the format:
  # set(project_RUN_TEST testToRun)
  # The testToRun should be a test executable that can be run to
  # smoke test the build.
  foreach(project ${CMAKE_CONTRACT_PROJECTS})
    include(Contracts/${project}/RunTest.cmake)
    ADD_TEST_MACRO(Contracts.${project}
      ${${project}_RUN_TEST})
    # Contract test timeout in seconds.
    # Default to 6 hours.
    if(DEFINED ${project}_TEST_TIMEOUT)
      set(timeout ${${project}_TEST_TIMEOUT})
    elseif(CMAKE_CONTRACT_TEST_TIMEOUT_DEFAULT)
      set(timeout ${CMAKE_CONTRACT_TEST_TIMEOUT_DEFAULT})
    else()
      set(timeout 21600)
    endif()
    set_tests_properties(Contracts.${project} PROPERTIES TIMEOUT ${timeout})
  endforeach()

  if(TEST_CompileCommandOutput)
    set(CompileCommandOutput_BUILD_OPTIONS
      -DMAKE_SUPPORTS_SPACES=${MAKE_SUPPORTS_SPACES})
    ADD_TEST_MACRO(CompileCommandOutput
      "${CMake_BINARY_DIR}/Tests/CMakeLib/runcompilecommands")
  endif()

  add_test(IncludeDirectories ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/IncludeDirectories"
    "${CMake_BINARY_DIR}/Tests/IncludeDirectories"
    --build-two-config
    ${build_generator_args}
    --build-project IncludeDirectories
    --build-options ${build_options}
    --test-command IncludeDirectories)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/IncludeDirectories")

  add_test(InterfaceLinkLibraries ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/InterfaceLinkLibraries"
    "${CMake_BINARY_DIR}/Tests/InterfaceLinkLibraries"
    --build-two-config
    ${build_generator_args}
    --build-project InterfaceLinkLibraries
    --build-options ${build_options}
    --test-command InterfaceLinkLibraries)
  list(APPEND TEST_BUILD_DIRS "${CMake_BINARY_DIR}/Tests/InterfaceLinkLibraries")

  if(NOT CMake_TEST_EXTERNAL_CMAKE)
    add_subdirectory(CMakeTests)
  endif()
endif()
