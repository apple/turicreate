cmake_minimum_required(VERSION 3.8)
project(TestFindDoxygen VERSION 1.0 LANGUAGES NONE)

# Testing backward compatible signature
find_package(Doxygen REQUIRED)

if(TARGET Doxygen::doxygen)
    # Check backward compatibility
    if(NOT DOXYGEN_EXECUTABLE)
        message(FATAL_ERROR "Backward compatibility broken: DOXYGEN_EXECUTABLE not set")
    endif()
    if(NOT DOXYGEN)
        message(FATAL_ERROR "Backward compatibility broken: DOXYGEN not set")
    endif()
    # Check presence of expected generated files
    foreach(file CMakeDoxyfile.in CMakeDoxygenDefaults.cmake)
        if(NOT EXISTS "${PROJECT_BINARY_DIR}/${file}")
            message(FATAL_ERROR "Missing generated file: ${file}")
        endif()
    endforeach()
else()
    message(FATAL_ERROR "Import target Doxygen::doxygen not defined")
endif()

doxygen_add_docs(docsNoArgs)
if(NOT EXISTS "${PROJECT_BINARY_DIR}/Doxyfile.docsNoArgs")
    message(FATAL_ERROR "Missing generated file: Doxyfile.docsNoArgs")
endif()
if(NOT TARGET docsNoArgs)
    message(FATAL_ERROR "Target docsNoArgs not created")
endif()

configure_file(spaces_in_name.cpp.in "spaces in name.cpp" COPYONLY)
doxygen_add_docs(docsWithArgs
    "${CMAKE_CURRENT_BINARY_DIR}/spaces in name.cpp"
    main.cpp
)
if(NOT EXISTS "${PROJECT_BINARY_DIR}/Doxyfile.docsWithArgs")
    message(FATAL_ERROR "Missing generated file: Doxyfile.docsWithArgs")
endif()
if(NOT TARGET docsWithArgs)
    message(FATAL_ERROR "Target docsWithArgs not created")
endif()
# Note that CMake inserts at least one entry into SOURCES when a COMMAND or
# DEPENDS option is given to add_custom_target(), so rather than looking for an
# exact match, we only verify that the files we expect to be there are present
get_target_property(srcList docsWithArgs SOURCES)
set(expectedList
    "${CMAKE_CURRENT_BINARY_DIR}/spaces in name.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
foreach(f IN LISTS expectedList)
    if(NOT f IN_LIST srcList)
        message(FATAL_ERROR "SOURCES missing file: ${f}")
    endif()
endforeach()

add_custom_target(allDocTargets)
add_dependencies(allDocTargets docsNoArgs docsWithArgs)
