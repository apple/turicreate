if(GENERATOR_TYPE STREQUAL "DEB" OR GENERATOR_TYPE STREQUAL "RPM")
  if(GENERATOR_TYPE STREQUAL "DEB")
    set(generator_type_suffix_ "IAN") # not entirely compatible...
  endif()

  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_FILE_NAME "${GENERATOR_TYPE}-DEFAULT")
  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_PKG_2_PACKAGE_NAME "second")
  string(TOLOWER "${GENERATOR_TYPE}" file_extension_)
  set(CPACK_${GENERATOR_TYPE}${generator_type_suffix_}_PKG_3_FILE_NAME "pkg_3_abc.${file_extension_}")
elseif(GENERATOR_TYPE STREQUAL "TGZ")
  set(CPACK_ARCHIVE_PKG_2_FILE_NAME "second")
  set(CPACK_ARCHIVE_PKG_3_FILE_NAME "pkg_3_abc")
endif()

install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_1)
install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_2)
install(FILES CMakeLists.txt DESTINATION foo COMPONENT pkg_3)
