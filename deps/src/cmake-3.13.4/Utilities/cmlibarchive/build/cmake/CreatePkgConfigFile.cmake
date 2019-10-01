# - Generate a libarchive.pc like autotools for pkg-config
#

# Set the required variables (we use the same input file as autotools)
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix \${prefix})
SET(libdir \${exec_prefix}/lib)
SET(includedir \${prefix}/include)
# Now, this is not particularly pretty, nor is it terribly accurate...
# Loop over all our additional libs
FOREACH(mylib ${ADDITIONAL_LIBS})
	# Extract the filename from the absolute path
	GET_FILENAME_COMPONENT(mylib_name ${mylib} NAME_WE)
	# Strip the lib prefix
	STRING(REGEX REPLACE "^lib" "" mylib_name ${mylib_name})
	# Append it to our LIBS string
	SET(LIBS "${LIBS} -l${mylib_name}")
ENDFOREACH()
# libxml2 is easier, since it's already using pkg-config
FOREACH(mylib ${PC_LIBXML_STATIC_LDFLAGS})
	SET(LIBS "${LIBS} ${mylib}")
ENDFOREACH()
# FIXME: The order of the libraries doesn't take dependencies into account,
#	 thus there's a good chance it'll make some binutils versions unhappy...
#	 This only affects Libs.private (looked up for static builds) though.
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/build/pkgconfig/libarchive.pc.in
		${CMAKE_CURRENT_SOURCE_DIR}/build/pkgconfig/libarchive.pc
		@ONLY)
# And install it, of course ;).
IF(ENABLE_INSTALL)
  INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/build/pkgconfig/libarchive.pc
          DESTINATION "lib/pkgconfig")
ENDIF()
