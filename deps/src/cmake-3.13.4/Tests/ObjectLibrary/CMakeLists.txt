cmake_minimum_required(VERSION 2.8)
project(ObjectLibrary C)

add_subdirectory(A)
add_subdirectory(B)

add_library(Cstatic STATIC c.c $<TARGET_OBJECTS:A> $<TARGET_OBJECTS:B>)
add_executable(UseCstatic main.c)
target_link_libraries(UseCstatic Cstatic)

add_library(Cshared SHARED c.c $<TARGET_OBJECTS:A> $<TARGET_OBJECTS:Bexport>)
add_executable(UseCshared main.c)
set_property(TARGET UseCshared PROPERTY COMPILE_DEFINITIONS SHARED_C)
target_link_libraries(UseCshared Cshared)
add_custom_command(TARGET UseCshared POST_BUILD COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/A/a.cmake)

add_executable(UseCinternal main.c c.c $<TARGET_OBJECTS:A> $<TARGET_OBJECTS:B>)

if("${CMAKE_GENERATOR}" MATCHES "Xcode")
  # Xcode does not seem to support targets without sources.
  set(dummy dummy.c)
endif()

# Test static library without its own sources.
add_library(ABstatic STATIC ${dummy} $<TARGET_OBJECTS:A> $<TARGET_OBJECTS:B>)
target_include_directories(ABstatic PUBLIC $<TARGET_PROPERTY:B,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(ABstatic PUBLIC $<TARGET_PROPERTY:B,INTERFACE_COMPILE_DEFINITIONS>)

add_executable(UseABstatic mainAB.c)
target_link_libraries(UseABstatic ABstatic)

# Test module definition file to export object library symbols in the test
# below if the platform needs and supports it.
set(ABshared_SRCS $<TARGET_OBJECTS:A>)
if(CMAKE_LINK_DEF_FILE_FLAG OR NOT WIN32)
  list(APPEND ABshared_SRCS $<TARGET_OBJECTS:B> AB.def)
else()
  set(NO_A NO_A)
  list(APPEND ABshared_SRCS $<TARGET_OBJECTS:Bexport>)
endif()

# Test shared library without its own sources.
add_library(ABshared SHARED ${dummy} ${ABshared_SRCS})
target_include_directories(ABshared PUBLIC $<TARGET_PROPERTY:B,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(ABshared PUBLIC $<TARGET_PROPERTY:B,INTERFACE_COMPILE_DEFINITIONS>)

add_executable(UseABshared mainAB.c)
set_property(TARGET UseABshared PROPERTY COMPILE_DEFINITIONS SHARED_B ${NO_A})
target_link_libraries(UseABshared ABshared)

# Test executable without its own sources.
add_library(ABmain OBJECT mainAB.c)
target_include_directories(ABmain PUBLIC $<TARGET_PROPERTY:B,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(ABmain PUBLIC $<TARGET_PROPERTY:B,INTERFACE_COMPILE_DEFINITIONS>)
add_executable(UseABinternal ${dummy}
  $<TARGET_OBJECTS:ABmain> $<TARGET_OBJECTS:A> $<TARGET_OBJECTS:B>
  )

# Test target-level dependencies of executable without sources.
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/UseABinternalDep.cmake)
add_custom_target(UseABinternalDep COMMAND ${CMAKE_COMMAND} -E touch UseABinternalDep.cmake)
add_custom_command(TARGET UseABinternal POST_BUILD COMMAND ${CMAKE_COMMAND} -P UseABinternalDep.cmake)
add_dependencies(UseABinternal UseABinternalDep)

add_subdirectory(ExportLanguages)
