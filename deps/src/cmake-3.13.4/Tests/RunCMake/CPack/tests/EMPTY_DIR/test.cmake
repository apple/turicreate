if(GENERATOR_TYPE STREQUAL "DEB" OR GENERATOR_TYPE STREQUAL "RPM")
  if(GENERATOR_TYPE STREQUAL "DEB")
    set(generator_type_suffix_ "IAN") # not entirely compatible...
  endif()

  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_FILE_NAME "${GENERATOR_TYPE}-DEFAULT")
endif()

install(DIRECTORY DESTINATION empty
        COMPONENT test)

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(CPACK_COMPONENTS_ALL test)
endif()
