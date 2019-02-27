set(CTEST_RUN_CURRENT_SCRIPT 0)

if(NOT DEFINED basefilename)
  message(FATAL_ERROR "pass -Dbasefilename=f1")
endif()

if(NOT DEFINED ext)
  set(ext "jkqvxz")
endif()

if(NOT DEFINED sleep_interval)
  set(sleep_interval 1)
endif()

get_filename_component(self_dir "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(filename "${self_dir}/${basefilename}.${ext}")

# count files
file(GLOB f1 *.${ext})
list(LENGTH f1 c1)
message("c='${c1}'")

# write a new file
message("Writing file: filename='${filename}'")
file(WRITE "${filename}" "${filename}")

# count files again
file(GLOB f2 *.${ext})
list(LENGTH f2 c2)
message("c='${c2}'")

# snooze
message("Sleeping: sleep_interval='${sleep_interval}'")
ctest_sleep(${sleep_interval})

# count files again
file(GLOB f3 *.${ext})
list(LENGTH f3 c3)
message("c='${c3}'")

# delete the file we wrote earlier
message("Removing file: filename='${filename}'")
file(REMOVE "${filename}")

# count files again
file(GLOB f4 *.${ext})
list(LENGTH f4 c4)
message("c='${c4}'")
