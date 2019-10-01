execute_process(COMMAND ${RPM_EXECUTABLE} -qpd ${FOUND_FILE_1}
  WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
  OUTPUT_VARIABLE DOC_FILES_
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REPLACE "\n" ";" DOC_FILES_ "${DOC_FILES_}")

set(DOC_FILES_WANTED_ "/usr/one/foo.txt;/usr/one/two/bar.txt;/usr/three/baz.txt")
if (NOT "${DOC_FILES_}" STREQUAL "${DOC_FILES_WANTED_}")
  message(FATAL_ERROR "USER_FILELIST handling error: Check filelist in spec file. Doc files were: ${DOC_FILES_}. Should have been ${DOC_FILES_WANTED_}")
endif()
