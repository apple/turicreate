if(GENERATOR_TYPE STREQUAL "DEB" OR GENERATOR_TYPE STREQUAL "RPM")
  if(GENERATOR_TYPE STREQUAL "DEB")
    set(generator_type_suffix_ "IAN") # not entirely compatible...
    set(group_ "SECTION")
  else()
    set(group_ "GROUP")
  endif()

  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_FILE_NAME "${GENERATOR_TYPE}-DEFAULT")

  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_PACKAGE_${group_} "default")
  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_PKG_2_PACKAGE_NAME "second")
  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_PKG_2_PACKAGE_${group_} "second_group")
endif()

install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_1)
install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_2)
install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_3)
