install(FILES CMakeLists.txt DESTINATION foo COMPONENT test)

if(GENERATOR_TYPE STREQUAL "DEB")
  set(package_type_ "DEBIAN")
  set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
else()
  set(package_type_ "${GENERATOR_TYPE}")
endif()

set(CPACK_${package_type_}_PACKAGE_EPOCH "3")

if(PACKAGING_TYPE STREQUAL "COMPONENT")
  set(CPACK_COMPONENTS_ALL test)
endif()
