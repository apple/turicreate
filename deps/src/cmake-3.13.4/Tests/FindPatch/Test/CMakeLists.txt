cmake_minimum_required(VERSION 3.8)
project(TestFindPatch VERSION 1.0 LANGUAGES NONE)

macro(_check)
  if(NOT EXISTS "${Patch_EXECUTABLE}")
    message(FATAL_ERROR "Failed to lookup Patch_EXECUTABLE [${Patch_EXECUTABLE}]")
  endif()

  if(NOT DEFINED PATCH_FOUND)
    message(FATAL_ERROR "Variable PATCH_FOUND is not defined")
  endif()

  # Is import target available ?
  if(NOT TARGET Patch::patch)
    message(FATAL_ERROR "Target Patch::patch is not defined")
  endif()

  # Check Patch::patch imported location
  get_property(_imported_location TARGET Patch::patch PROPERTY IMPORTED_LOCATION)
  if(NOT "${Patch_EXECUTABLE}" STREQUAL "${_imported_location}")
    message(FATAL_ERROR "\
Patch_EXECUTABLE is expected to be equal to Patch::patch IMPORTED_LOCATION
  Patch_EXECUTABLE [${Patch_EXECUTABLE}]
  Patch::patch IMPORTED_LOCATION [${_imported_location}]
")
  endif()

endmacro()

find_package(Patch REQUIRED)
_check()

# Calling twice should not fail
find_package(Patch REQUIRED)
_check()

add_custom_target(TestPatchVersion ALL
  COMMAND ${Patch_EXECUTABLE} -v
  )

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/QUOTE.txt.baseline"
[=[Because it's there.
- George Mallory, 1923
]=]
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/QUOTE.txt" "Because it's there.\n")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/quote-add-author.patch"
[=[diff --git a/QUOTE.txt b/QUOTE.txt
index b36681d..68059b3 100644
--- a/QUOTE.txt
+++ b/QUOTE.txt
@@ -1 +1,2 @@
 Because it's there.
+- George Mallory
]=]
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/quote-add-date.patch"
[=[diff --git a/QUOTE.txt b/QUOTE.txt
index 68059b3..c6f30c2 100644
--- a/QUOTE.txt
+++ b/QUOTE.txt
@@ -1,2 +1,2 @@
 Because it's there.
-- George Mallory
+- George Mallory, 1923
]=]
)

add_custom_target(TestPatch ALL
  COMMAND ${Patch_EXECUTABLE} -p1 -i quote-add-author.patch --binary
  COMMAND Patch::patch -p1 -i quote-add-date.patch --binary
  COMMAND ${CMAKE_COMMAND} -E compare_files QUOTE.txt QUOTE.txt.baseline
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
