project(config)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(APPLE)

  find_library(foundation_config NAMES Foundation REQUIRED)
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
