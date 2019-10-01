set(CMAKE_EXECUTABLE "${CMake_BIN_DIR}/cmake")


macro(AddCMakeTest TestName PreArgs)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${TestName}Test.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${TestName}Test.cmake" @ONLY)
  add_test(NAME CMake.${TestName}
    COMMAND ${CMAKE_EXECUTABLE} ${PreArgs}
    -P "${CMAKE_CURRENT_BINARY_DIR}/${TestName}Test.cmake" ${ARGN})
endmacro()


AddCMakeTest(List "")
AddCMakeTest(VariableWatch "")
AddCMakeTest(Include "")
AddCMakeTest(FindBase "")
AddCMakeTest(Toolchain "")
AddCMakeTest(GetFilenameComponentRealpath "")
AddCMakeTest(Version "")
AddCMakeTest(Message "")
AddCMakeTest(File "")
AddCMakeTest(ImplicitLinkInfo "")
AddCMakeTest(ModuleNotices "")
AddCMakeTest(GetProperty "")
AddCMakeTest(If "")
AddCMakeTest(String "")
AddCMakeTest(Math "")
AddCMakeTest(CMakeMinimumRequired "")
AddCMakeTest(CompilerIdVendor "")
AddCMakeTest(ProcessorCount "-DKWSYS_TEST_EXE=$<TARGET_FILE:cmsysTestsCxx>")
AddCMakeTest(PushCheckState "")
AddCMakeTest(While "")
AddCMakeTest(CMakeHostSystemInformation "")

AddCMakeTest(FileDownload "")
set_property(TEST CMake.FileDownload PROPERTY
  PASS_REGULAR_EXPRESSION "file already exists with expected MD5 sum"
  )
AddCMakeTest(FileDownloadBadHash "")
set_property(TEST CMake.FileDownloadBadHash PROPERTY
  WILL_FAIL TRUE
  )

AddCMakeTest(FileUpload "")

if(HAVE_ELF_H)
  AddCMakeTest(ELF "")
endif()

set(EndStuff_PreArgs
  "-Ddir:STRING=${CMAKE_CURRENT_BINARY_DIR}/EndStuffTest"
  )
AddCMakeTest(EndStuff "${EndStuff_PreArgs}")

AddCMakeTest(GetPrerequisites "-DConfiguration:STRING=$<CONFIGURATION>")

if(GIT_EXECUTABLE)
  set(PolicyCheck_PreArgs
    "-DCMake_BINARY_DIR:PATH=${CMake_BINARY_DIR}"
    "-DCMake_SOURCE_DIR:PATH=${CMake_SOURCE_DIR}"
    "-DGIT_EXECUTABLE:STRING=${GIT_EXECUTABLE}"
    )
  AddCMakeTest(PolicyCheck "${PolicyCheck_PreArgs}")
endif()

# Run CheckSourceTree as the very last test in the CMake/CTest/CPack test
# suite. It detects if any changes have been made to the CMake source tree
# by any previous configure, build or test steps.
#
if(GIT_EXECUTABLE)
  string(REPLACE "\\" "/" ENV_HOME "$ENV{HOME}")
  set(CheckSourceTree_PreArgs
    "-DCMake_BINARY_DIR:PATH=${CMake_BINARY_DIR}"
    "-DCMake_SOURCE_DIR:PATH=${CMake_SOURCE_DIR}"
    "-DGIT_EXECUTABLE:STRING=${GIT_EXECUTABLE}"
    "-DHOME:STRING=${ENV_HOME}"
    )
  AddCMakeTest(CheckSourceTree "${CheckSourceTree_PreArgs}")
endif()
