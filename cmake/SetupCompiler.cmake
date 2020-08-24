macro(SetupCompiler)

#**************************************************************************/
#*                                                                        */
#*                     Step 1: Identify the compiler                      */
#*                                                                        */
#**************************************************************************/



if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CLANG false)
elseif(WIN32)
# for whatever reason on windows the compiler identification is an empty string
  set(CLANG false)
else()
  set(CLANG true)
endif()

if(CMAKE_GENERATOR MATCHES "MinGW Makefiles")
  SET(MINGW_MAKEFILES true)
else()
  SET(MINGW_MAKEFILES false)
endif()

if(CMAKE_GENERATOR MATCHES "MSYS Makefiles")
  SET(MSYS_MAKEFILES true)
else()
  SET(MSYS_MAKEFILES false)
endif()


if(WIN32 AND MINGW)
  SET(COMPILER_FLAGS "${COMPILER_FLAGS} -Wa,-mbig-obj")
endif()

set(MINGW_ROOT "/mingw64/bin")
# Separate variable so that cython's CMakeLists.txt can use it too
if (WIN32)
        set(INSTALLATION_SYSTEM_BINARY_FILES
        ${CMAKE_SOURCE_DIR}/deps/local/bin/libsodium-13.dll
        ${MINGW_ROOT}/libiconv-2.dll
        ${MINGW_ROOT}/libssh2-1.dll
        ${MINGW_ROOT}/zlib1.dll
        ${MINGW_ROOT}/libwinpthread-1.dll
        ${MINGW_ROOT}/libgcc_s_seh-1.dll
        ${MINGW_ROOT}/libstdc++-6.dll
        ${MINGW_ROOT}/libeay32.dll)
endif()

#**************************************************************************/
#*                                                                        */
#*                     Step 2: Set up SDK stuff.                          */
#*                                                                        */
#**************************************************************************/


if(APPLE)  
  if(NOT TC_BASE_SDK) 
    # Assume that we're building for macOSX
    set(TC_BASE_SDK macosx)
  endif()

  if(${TC_BASE_SDK} MATCHES "iphoneos.*")

    if(${TC_BUILD_REMOTEFS})
      message(FATAL_ERROR "RemoteFS must be disabled for building iOS.")
    endif()

    set(TC_BUILD_IOS 1)
    message(STATUS "Configuration targets iOS.")

    if(NOT CLANG)
      message(FATAL_ERROR "Must be using Clang compiler system to target ios.")
    endif()

    # Add compiler flags 
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target arm64-apple-darwin-eabi")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target arm64-apple-darwin-eabi")

  elseif(${TC_BASE_SDK} MATCHES "macos.*")
  else()
    message(FATAL_ERROR "Platform ${TC_BASE_SDK} not recognized.") 
  endif()

  # Find and set the SDK path.
  EXEC_PROGRAM(xcrun ARGS --sdk ${TC_BASE_SDK} 
    --show-sdk-path OUTPUT_VARIABLE TC_BASE_SDK_PATH RETURN_VALUE _xcrun_ret)

  if(NOT ${_xcrun_ret} EQUAL 0)
    message(FATAL_ERROR "xcrun command failed with return code ${_xcrun_ret}.")
  endif()

  # Set the base root.
  SET(CMAKE_OSX_SYSROOT "${TC_BASE_SDK_PATH}")
  
  message("Using SDK ${CMAKE_OSX_SYSROOT}.")
  
  # Get the sdk version.
  EXEC_PROGRAM(xcrun ARGS --sdk ${TC_BASE_SDK} --show-sdk-version OUTPUT_VARIABLE TC_BASE_SDK_VERSION RETURN_VALUE _xcrun_ret)
  if(NOT ${_xcrun_ret} EQUAL 0)
    message(FATAL_ERROR "xcrun command failed with return code ${_xcrun_ret}.")
  endif()
  
  # TODO: replace all these with in-build macros based on standard macros in Availability.h

  # Core ML is only present on macOS 10.13 or higher.
  # Logic reversed to get around what seems to be a CMake bug.
  if(NOT TC_BASE_SDK_VERSION VERSION_LESS 10.13)
    add_definitions(-DHAS_CORE_ML)
    set(HAS_CORE_ML TRUE)
  endif()
  
  # MLCompute is only present on macOS 10.16 or higher.
  if(TC_BASE_SDK_VERSION VERSION_GREATER_EQUAL 10.16)
    add_definitions(-DHAS_ML_COMPUTE)
    set(HAS_ML_COMPUTE TRUE)
  endif()

  # Core ML only supports batch inference on macOS 10.14 or higher
  # Logic reversed to get around what seems to be a CMake bug.
  if(NOT TC_BASE_SDK_VERSION VERSION_LESS 10.14)
    add_definitions(-DHAS_CORE_ML_BATCH_INFERENCE)

    # GPU-accelerated training with MPS backend requires macOS 10.14 or higher
    add_definitions(-DHAS_MPS)
    set(HAS_MPS TRUE)

    # CoreML MLCustomModel requires macOS 10.14 or higher
    add_definitions(-DHAS_MLCUSTOM_MODEL)
    set(HAS_COREML_CUSTOM_MODEL TRUE)
  endif()
 
  if(NOT TC_BASE_SDK_VERSION VERSION_LESS 10.15)
    add_definitions(-DHAS_MACOS_10_15)
  endif()

endif() 

endmacro()
