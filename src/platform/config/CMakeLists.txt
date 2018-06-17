project(config)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(APPLE)

    #find_library(foundation_config NAMES Foundation REQUIRED)
    #message("Foundation found at ${foundation_config}.")

  # We need to get the proper SDK paths in the build
  execute_process(
    COMMAND bash -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE _macosx_sdk_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(_FRAMEWORK_SEARCH_PATH "${_macosx_sdk_path}/System/Library/Frameworks/;/System/Library/Frameworks/")

  find_library(foundation_config NAMES Foundation
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
   message("Foundation found at ${foundation_config}.")


  make_library(platform_config
    SOURCES
      apple_config.mm
    REQUIRES
    ${foundation_config}
    EXTERNAL_VISIBILITY
  )

else()

  make_empty_library(platform_config)

endif()
