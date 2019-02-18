cmake_minimum_required(VERSION 3.4)
enable_language(C)

set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE INTERNAL "Supported configuration types")
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}) # get rid of ${EFFECTIVE_PLATFORM_NAME}

add_library(Framework ${FRAMEWORK_TYPE}
            foo.c
            foo.h
            res.txt
            flatresource.txt
            deepresource.txt
            some.txt)
set_target_properties(Framework PROPERTIES
                      FRAMEWORK TRUE
                      PUBLIC_HEADER foo.h
                      RESOURCE "res.txt")
set_source_files_properties(flatresource.txt PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set_source_files_properties(deepresource.txt PROPERTIES MACOSX_PACKAGE_LOCATION Resources/deep)
set_source_files_properties(some.txt PROPERTIES MACOSX_PACKAGE_LOCATION somedir)

add_custom_command(TARGET Framework POST_BUILD
                   COMMAND /usr/bin/file $<TARGET_FILE:Framework>)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/FrameworkName.cmake CONTENT "set(framework-dir \"$<TARGET_BUNDLE_DIR:Framework>\")\n")
