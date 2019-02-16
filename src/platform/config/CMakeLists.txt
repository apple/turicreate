project(config)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(APPLE)

  make_library(platform_config OBJECT
    SOURCES
      apple_config.mm
    REQUIRES
      ${FOUNDATION}
    EXTERNAL_VISIBILITY
  )

endif()
