install(
  DIRECTORY pattern/
  DESTINATION dir1
  FILES_MATCHING
  PATTERN "*.h"
  REGEX "\\.c$"
  )

# FIXME: If/when CMake gains a good way to read file permissions, we should
# check that these permissions were set correctly.
install(
  DIRECTORY pattern
  DESTINATION dir2
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE
  DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  PATTERN "*.h" EXCLUDE
  REGEX "\\.c$" EXCLUDE
  )

install(
  DIRECTORY pattern/
  DESTINATION dir3
  PATTERN "*.h"
  PERMISSIONS OWNER_READ OWNER_WRITE
  )

install(
  DIRECTORY pattern/
  DESTINATION dir4
  USE_SOURCE_PERMISSIONS
  )

install(
  DIRECTORY
  DESTINATION empty
  )
