
include(CMakeParseArguments)

# shared_library_from_static([library name] REQUIRES [bunch of stuff] [DIRECT])
# 
# This is performed in 2 stages.
# Stage 1 collects the set of archives by building a dummy executable in them
# but intercepting the link stage to find the set of archives.
#
# Stage 2 builds the actual shared library. It merges the archives listed
# by stage 1 and uses that to put together the shared library 
#
# Stage 1:
# .1 What we do first is to identify all the archive files. That is done by
#   creating an executable. essentiallly generating make_executable(REQUIRES
#   [bunch of stuff]) using a dummy cpp file.  
# 2. We then intercept the link stage using (RULE_LAUNCH_LINK)
#    redirecting it to a shell script called dump_library_list.sh
# 3. dump_library_list (aside from completing the link), also generates 2
# files. 
#     collect_archives.txt --> contains all libraries which live inside the
#                               build directory 
#     collect_libs.txt --> contains all libraries which live elsewhere.
#
#  Stage 2:
#  1. The 2nd stage, does the same thing, create a dummy file, and generates
#      make_library(REQUIRES [bunch of stuff])
#  2.  And then once again, intercepts the link stage redirecting it to a
#      shell script called collect_archive.sh
#  3. This script will then:
#     - unpack all the archives in collect_archives.txt, merging them into 
#       a single library called collect.a
#     - appends the contents of collect_libs.txt 
#     - combine it with the original shared library link line
#       as well as instructing the linker to take the complete contents
#       of collect.a (ex: -Wl,-whole-archive collect.a -Wl,-no-whole-archive)
#     -  complete the link 
#
# If DIRECT is set, only direct dependencies in REQUIRES collected. Recursive
# dependencies are not collected. 
#
#
function(make_shared_library_from_static NAME)
        set(options DIRECT)
        set(one_value_args EXPORT_MAP OUTPUT_NAME)
        set(multi_value_args REQUIRES)
        CMAKE_PARSE_ARGUMENTS(args "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})


        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir)

        if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy.cpp)
                file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy.cpp "int main(){}")
        endif()
        if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy2.cpp)
                file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy2.cpp "")
        endif()

        if (${args_DIRECT})
                # if we are only collecting direct dependencies, 
                # compile a string of all the targets targets
                set(directfilelist)
                foreach(req ${args_REQUIRES})
                        set(directfilelist "${directfilelist} $<TARGET_FILE:${req}>")
                endforeach()
        endif()

        make_executable(${NAME}_collect
                SOURCES
                ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy.cpp
                REQUIRES ${args_REQUIRES})

        set_target_properties(${NAME}_collect PROPERTIES RULE_LAUNCH_LINK "env WORKINGDIR=${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir BUILD_PREFIX=${CMAKE_BINARY_DIR} bash ${CMAKE_SOURCE_DIR}/scripts/dump_library_list.sh")

        add_custom_command(
          OUTPUT
          ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/collect_archives.txt
          ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/collect_libs.txt
          COMMAND true
          COMMENT "Dumping library/archive list for ${NAME}"
          DEPENDS ${NAME}_collect
          VERBATIM
        )

        if (${args_DIRECT})
                # add a custom command after link to override the collected list
                add_custom_command(TARGET ${NAME}_collect
                        POST_BUILD
                        COMMAND bash -c "\"echo '${directfilelist}' > ${NAME}_collect_dir/collect_archives.txt\""
                        )
        endif()

        if (args_OUTPUT_NAME)
                make_library(${NAME} SHARED
                        SOURCES
                        ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy2.cpp
                        REQUIRES ${args_REQUIRES}
                        OUTPUT_NAME ${args_OUTPUT_NAME})
        else()
                make_library(${NAME} SHARED
                        SOURCES
                        ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir/dummy2.cpp
                        REQUIRES ${args_REQUIRES})
        endif()
        if(NOT ${args_EXPORT_MAP} STREQUAL "")
          set_target_properties(${NAME} PROPERTIES RULE_LAUNCH_LINK "env WORKINGDIR=${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir BUILD_PREFIX=${CMAKE_BINARY_DIR} EXPORT_MAP=${args_EXPORT_MAP} bash ${CMAKE_SOURCE_DIR}/scripts/collect_archive.sh")
          set_property(TARGET ${NAME} APPEND PROPERTY LINK_DEPENDS "${args_EXPORT_MAP}")
        else()
          set_target_properties(${NAME} PROPERTIES RULE_LAUNCH_LINK "env WORKINGDIR=${CMAKE_CURRENT_BINARY_DIR}/${NAME}_collect_dir BUILD_PREFIX=${CMAKE_BINARY_DIR} bash ${CMAKE_SOURCE_DIR}/scripts/collect_archive.sh")
        endif()
        add_dependencies(${NAME} ${NAME}_collect)

endfunction()
