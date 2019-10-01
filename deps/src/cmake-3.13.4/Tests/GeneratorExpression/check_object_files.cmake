
if (NOT EXISTS ${OBJLIB_LISTFILE})
  message(SEND_ERROR "Object listing file \"${OBJLIB_LISTFILE}\" not found!")
endif()

file(STRINGS ${OBJLIB_LISTFILE} objlib_files ENCODING UTF-8)

list(LENGTH objlib_files num_objectfiles)
if (NOT EXPECTED_NUM_OBJECTFILES EQUAL num_objectfiles)
  message(SEND_ERROR "Unexpected number of entries in object list file (${num_objectfiles} instead of ${EXPECTED_NUM_OBJECTFILES})")
endif()

foreach(objlib_file ${objlib_files})
  set(file_exists False)
  if (EXISTS ${objlib_file})
    set(file_exists True)
  endif()

  if (NOT file_exists)
    if(attempts)
      list(REMOVE_DUPLICATES attempts)
      set(tried "  Tried ${attempts}")
    endif()
    message(SEND_ERROR "File \"${objlib_file}\" does not exist!${tried}")
  endif()
endforeach()
