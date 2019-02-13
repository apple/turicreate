project(config)

#/**************************************************************************/
#/*                                                                        */
#/*                             Random Library                             */
#/*                                                                        */
#/**************************************************************************/

if(APPLE)

  add_library(platform_config OBJECT
    apple_config.mm
  )

endif()
