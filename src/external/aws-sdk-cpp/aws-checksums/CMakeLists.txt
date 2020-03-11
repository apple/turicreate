cmake_minimum_required (VERSION 3.1)

option(STATIC_CRT "Windows specific option that to specify static/dynamic run-time library" OFF)

project (aws-checksums C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AwsSharedLibSetup)

file(GLOB AWS_CHECKSUMS_HEADERS
     "include/aws/checksums/*.h"
)

file(GLOB AWS_CHECKSUMS_PRIV_HEADERS
     "include/aws/checksums/private/*.h"
)

file(GLOB AWS_CHECKSUMS_SRC
    "source/*.c"
)

file(GLOB AWS_ARCH_SRC
    "source/intel/*.c"
    "source/arm/*.c"
)

if(WIN32)

     if(MSVC)
         file(GLOB AWS_CHECKSUMS_VISUALC_SOURCE
             "source/visualc/*.c"
         )        

         source_group("Header Files\\aws\\checksums" FILES ${AWS_CHECKSUMS_HEADERS})

         source_group("Source Files" FILES ${AWS_CHECKSUMS_SRC})
         source_group("Source Files\\visualc" FILES ${AWS_CHECKSUMS_VISUALC_SOURCE})
         source_group("Source Files\\arch" FILES ${AWS_ARCH_SRC})
          
         file(GLOB AWS_CHECKSUMS_PLATFORM_SOURCE
             ${AWS_CHECKSUMS_VISUALC_SOURCE}
         )
     endif()
endif()

file(GLOB CHECKSUMS_HEADERS
    ${AWS_CHECKSUMS_HEADERS}
    ${AWS_CHECKSUMS_PRIV_HEADERS}
)

file(GLOB CHECKSUMS_SRC
    ${AWS_CHECKSUMS_SRC}
    ${AWS_CHECKSUMS_PLATFORM_SOURCE}
    ${AWS_ARCH_SRC}
)

if(BUILD_SHARED_LIBS)
    set(LIBTYPE SHARED)
    if(WIN32)
        add_definitions("-DUSE_IMPORT_EXPORT")
    endif()
else()
    set(LIBTYPE STATIC)
endif()

if (MSVC)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _CMAKE_BUILD_TYPE)
    if(STATIC_CRT)
        string(REPLACE "/MD" "/MT" _FLAGS "${CMAKE_C_FLAGS_${_CMAKE_BUILD_TYPE}}")
    else()
        string(REPLACE "/MT" "/MD" _FLAGS "${CMAKE_C_FLAGS_${_CMAKE_BUILD_TYPE}}")
    endif()
    string(REPLACE " " ";" _FLAGS "${_FLAGS}")
endif()

add_library(aws-checksums ${LIBTYPE} ${CHECKSUMS_HEADERS} ${CHECKSUMS_SRC})
target_compile_options(aws-checksums PRIVATE ${_FLAGS})
set_target_properties(aws-checksums PROPERTIES LINKER_LANGUAGE C)
set(CMAKE_C_FLAGS_DEBUGOPT "")
set_property(TARGET aws-checksums PROPERTY C_STANDARD 99)

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(aws-checksums PRIVATE "-DAWS_CHECKSUMS_EXPORTS")    
endif()

if(NOT MSVC)
    target_compile_options(aws-checksums PRIVATE -fPIC)
endif()

if(BUILD_JNI_BINDINGS)
    find_package(JNI)
    include_directories(${JNI_INCLUDE_DIRS})
    set(PLATFORM_LIBS ${PLATFORM_LIBS} ${JNI_LIBRARIES})
    target_compile_definitions(aws-checksums PRIVATE "-DBUILD_JNI_BINDINGS")    
endif()

if(CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(aws-checksums PRIVATE "-DDEBUG_BUILD")
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(aws-checksums ${PLATFORM_LIBS})

file(GLOB TEST_SRC "tests/*.c")
file(GLOB TEST_HDRS "tests/*.h")
file(GLOB TESTS ${TEST_HDRS} ${TEST_SRC})
  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_executable(aws-checksums-tests ${TESTS})
target_compile_options(aws-checksums-tests PRIVATE ${_FLAGS})
target_link_libraries(aws-checksums-tests aws-checksums)
set_target_properties(aws-checksums-tests PROPERTIES LINKER_LANGUAGE C)
set_property(TARGET aws-checksums-tests PROPERTY C_STANDARD 99)


include(CTest)
enable_testing()
add_test(crc32c aws-checksums-tests crc32c)
add_test(crc32 aws-checksums-tests crc32)

set(LIBRARY_DIRECTORY lib)
# Set the default lib installation path on GNU systems with GNUInstallDirs
if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(LIBRARY_DIRECTORY ${CMAKE_INSTALL_LIBDIR})
endif()

install(FILES ${AWS_CHECKSUMS_HEADERS} DESTINATION "include/aws/checksums")
aws_prepare_shared_lib_exports(${CMAKE_PROJECT_NAME})

if (BUILD_SHARED_LIBS)
   set (TARGET_DIR "shared")
else()
   set (TARGET_DIR "static")
endif()

install(EXPORT "${CMAKE_PROJECT_NAME}-targets"
    DESTINATION "${LIBRARY_DIRECTORY}/${CMAKE_PROJECT_NAME}/cmake/${TARGET_DIR}/"
    NAMESPACE AWS::)

configure_file("cmake/${CMAKE_PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
    @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
    DESTINATION "${LIBRARY_DIRECTORY}/${CMAKE_PROJECT_NAME}/cmake/")

