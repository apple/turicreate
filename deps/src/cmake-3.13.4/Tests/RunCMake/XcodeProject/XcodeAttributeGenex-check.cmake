# per target attribute with genex

set(expect "TEST_HOST = \"[^;\"]*Tests/RunCMake/XcodeProject/XcodeAttributeGenex-build/[^;\"/]*/some\"")
file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodeAttributeGenex.xcodeproj/project.pbxproj actual
     REGEX "TEST_HOST = .*;" LIMIT_COUNT 1)
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()

# per target attribute with variant

file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodeAttributeGenex.xcodeproj/project.pbxproj actual
     REGEX "CONFIG_SPECIFIC = .*;")
list(REMOVE_DUPLICATES actual)

set(expect "CONFIG_SPECIFIC = general")
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()

set(expect "CONFIG_SPECIFIC = release")
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()

# global attribute with genex

set(expect "ANOTHER_GLOBAL = \"[^;\"]*Tests/RunCMake/XcodeProject/XcodeAttributeGenex-build/[^;\"/]*/another\"")
file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodeAttributeGenex.xcodeproj/project.pbxproj actual
     REGEX "ANOTHER_GLOBAL = .*;" LIMIT_COUNT 1)
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()

# global attribute with variant

file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodeAttributeGenex.xcodeproj/project.pbxproj actual
     REGEX "ANOTHER_CONFIG = .*;" LIMIT_COUNT 4)
list(REMOVE_DUPLICATES actual)

set(expect "ANOTHER_CONFIG = general")
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()

set(expect "ANOTHER_CONFIG = debug")
if(NOT "${actual}" MATCHES "${expect}")
  message(SEND_ERROR "The actual project contains the line:\n ${actual}\n"
    "which does not match expected regex:\n ${expect}\n")
endif()
