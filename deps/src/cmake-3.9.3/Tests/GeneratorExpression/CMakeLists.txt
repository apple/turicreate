cmake_minimum_required (VERSION 2.8.8)
project(GeneratorExpression)

include(CTest)

# This test is split into multiple parts as needed to avoid NMake command
# length limits.

add_custom_target(check-part1 ALL
  COMMAND ${CMAKE_COMMAND}
    -Dtest_0=$<0:nothing>
    -Dtest_0_with_comma=$<0:-Wl,--no-undefined>
    -Dtest_1=$<1:content>
    -Dtest_1_with_comma=$<1:-Wl,--no-undefined>
    -Dconfig=$<CONFIGURATION>
    -Dshort_config=$<CONFIG>
    -Dtest_and_0=$<AND:0>
    -Dtest_and_0_0=$<AND:0,0>
    -Dtest_and_0_1=$<AND:0,1>
    -Dtest_and_1=$<AND:1>
    -Dtest_and_1_0=$<AND:1,0>
    -Dtest_and_1_1=$<AND:1,1>
    # Ordinarily, the 'invalidcontent' would cause an error, but
    # the '0' makes the AND abort early.
    -Dtest_and_0_invalidcontent=$<AND:0,invalidcontent>
    -Dtest_config_0=$<CONFIG:$<CONFIGURATION>x>
    -Dtest_config_1=$<CONFIG:$<CONFIGURATION>>
    -Dtest_config_debug=$<CONFIG:Debug>$<CONFIG:DEBUG>$<CONFIG:DeBuG>
    -Dtest_config_release=$<CONFIG:Release>$<CONFIG:RELEASE>$<CONFIG:ReLeAsE>
    -Dtest_config_relwithdebinfo=$<CONFIG:RelWithDebInfo>$<CONFIG:RELWITHDEBINFO>$<CONFIG:relwithdebinfo>
    -Dtest_config_minsizerel=$<CONFIG:MinSizeRel>$<CONFIG:MINSIZEREL>$<CONFIG:minsizerel>
    -Dtest_not_0=$<NOT:0>
    -Dtest_not_1=$<NOT:1>
    -Dtest_or_0=$<OR:0>
    -Dtest_or_0_0=$<OR:0,0>
    -Dtest_or_0_1=$<OR:0,1>
    -Dtest_or_1=$<OR:1>
    -Dtest_or_1_0=$<OR:1,0>
    -Dtest_or_1_1=$<OR:1,1>
    -Dtest_or_1_invalidcontent=$<OR:1,invalidcontent>
    -Dtest_bool_notfound=$<BOOL:NOTFOUND>
    -Dtest_bool_foo_notfound=$<BOOL:Foo-NOTFOUND>
    -Dtest_bool_true=$<BOOL:True>
    -Dtest_bool_false=$<BOOL:False>
    -Dtest_bool_on=$<BOOL:On>
    -Dtest_bool_off=$<BOOL:Off>
    -Dtest_bool_no=$<BOOL:No>
    -Dtest_bool_n=$<BOOL:N>
    -Dtest_bool_empty=$<BOOL:>
    -Dtest_strequal_yes_yes=$<STREQUAL:Yes,Yes>
    -Dtest_strequal_yes_yes_cs=$<STREQUAL:Yes,yes>
    -Dtest_strequal_yes_no=$<STREQUAL:Yes,No>
    -Dtest_strequal_no_yes=$<STREQUAL:No,Yes>
    -Dtest_strequal_angle_r=$<STREQUAL:$<ANGLE-R>,$<ANGLE-R>>
    -Dtest_strequal_comma=$<STREQUAL:$<COMMA>,$<COMMA>>
    -Dtest_strequal_semicolon=$<STREQUAL:$<SEMICOLON>,$<SEMICOLON>>
    -Dtest_strequal_angle_r_comma=$<STREQUAL:$<ANGLE-R>,$<COMMA>>
    -Dtest_strequal_both_empty=$<STREQUAL:,>
    -Dtest_strequal_one_empty=$<STREQUAL:something,>
    -Dtest_angle_r=$<ANGLE-R>
    -Dtest_comma=$<COMMA>
    -Dtest_semicolon=$<SEMICOLON>
    -Dtest_colons_1=$<1::>
    -Dtest_colons_2=$<1:::>
    -Dtest_colons_3=$<1:Qt5::Core>
    -Dtest_colons_4=$<1:C:\\CMake>
    -Dtest_colons_5=$<1:C:/CMake>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-part1.cmake
  COMMAND ${CMAKE_COMMAND} -E echo "check done (part 1 of 4)"
  VERBATIM
  )

add_library(empty1 empty.cpp)
target_include_directories(empty1 PUBLIC /empty1/public)
target_include_directories(empty1 PRIVATE /empty1/private)

add_library(empty2 empty.cpp)
target_include_directories(empty2 PUBLIC /empty2/public)

add_library(empty3 empty.cpp)
target_include_directories(empty3 PUBLIC /empty3/public)
target_include_directories(empty3 PRIVATE /empty3/private)

add_library(empty4 empty.cpp)
target_include_directories(empty4 PUBLIC /empty4/public)

target_link_libraries(empty1 LINK_PUBLIC empty2)
target_link_libraries(empty2 LINK_PUBLIC empty3 empty4)
target_link_libraries(empty3 LINK_PUBLIC empty2 empty4)

add_library(empty5 empty.cpp)
target_include_directories(empty5 PRIVATE /empty5/private1 /empty5/private2)

add_custom_target(check-part2 ALL
  COMMAND ${CMAKE_COMMAND}
    -Dtest_incomplete_1=$<
    -Dtest_incomplete_2=$<something
    -Dtest_incomplete_3=$<something:
    -Dtest_incomplete_4=$<something:,
    -Dtest_incomplete_5=$something:,>
    -Dtest_incomplete_6=<something:,>
    -Dtest_incomplete_7=$<something::
    -Dtest_incomplete_8=$<something:,
    -Dtest_incomplete_9=$<something:,,
    -Dtest_incomplete_10=$<something:,:
    -Dtest_incomplete_11=$<something,,
    -Dtest_incomplete_12=$<,,
    -Dtest_incomplete_13=$<some$<1:special>thing
    -Dtest_incomplete_14=$<$<ANGLE-R>
    -Dtest_incomplete_15=$<some$<thing
    -Dtest_incomplete_16=$<BOOL:something
    -Dtest_incomplete_17=$<1:some$thing>
    -Dtest_incomplete_18=$<1:some,thing
    -Dtest_incomplete_19=$<1:some,thing$<ANGLE-R>
    -Dtest_incomplete_20=$<CONFIGURATION$<ANGLE-R>
    -Dtest_incomplete_21=$<BOOL:something$<ANGLE-R>
    -Dtest_build_interface=$<BUILD_INTERFACE:build>
    -Dtest_install_interface=$<INSTALL_INTERFACE:install>
    -Dtest_target_name_1=$<TARGET_NAME:tgt,ok>
    -Dtest_target_name_2=$<TARGET_NAME:tgt:ok>
    -Dtest_target_includes1=$<TARGET_PROPERTY:empty1,INTERFACE_INCLUDE_DIRECTORIES>
    -Dtest_target_includes2=$<TARGET_PROPERTY:empty2,INTERFACE_INCLUDE_DIRECTORIES>
    -Dtest_target_includes3=$<TARGET_PROPERTY:empty3,INTERFACE_INCLUDE_DIRECTORIES>
    -Dtest_target_includes4=$<TARGET_PROPERTY:empty1,INCLUDE_DIRECTORIES>
    -Dtest_target_includes5=$<TARGET_PROPERTY:empty2,INCLUDE_DIRECTORIES>
    -Dtest_target_includes6=$<TARGET_PROPERTY:empty3,INCLUDE_DIRECTORIES>
    -Dtest_target_includes7=$<TARGET_PROPERTY:empty1,INTERFACE_INCLUDE_DIRECTORIES>
    -Dtest_target_includes8=$<TARGET_PROPERTY:empty5,INCLUDE_DIRECTORIES>
    -Dtest_arbitrary_content_comma_1=$<1:a,>
    -Dtest_arbitrary_content_comma_2=$<1:,a>
    -Dtest_arbitrary_content_comma_3=$<1:a,,>
    -Dtest_arbitrary_content_comma_4=$<1:,>
    -Dtest_arbitrary_content_comma_5=$<1:,,>
    -Dtest_arbitrary_content_comma_6=$<1:,,,>
    -Dtest_arbitrary_content_comma_7=$<1:,,a>
    -Dtest_arbitrary_content_comma_8=$<1:a,,b>
    -Dtest_arbitrary_content_comma_9=$<1:a,,b,,>
    -Dtest_arbitrary_content_comma_10=$<1:,,a,,b,,>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-part2.cmake
  COMMAND ${CMAKE_COMMAND} -E echo "check done (part 2 of 4)"
  VERBATIM
)

add_library(imported1 SHARED IMPORTED)
set_property(TARGET imported1 PROPERTY IMPORTED_LOCATION_RELEASE release_loc)
set_property(TARGET imported1 PROPERTY IMPORTED_LOCATION_DEBUG debug_loc)
set_property(TARGET imported1 PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
set_property(TARGET imported1 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "/imported1/include/with space")

add_library(imported2 SHARED IMPORTED)
set_property(TARGET imported2 PROPERTY IMPORTED_LOCATION_RELEASE release_loc)
set_property(TARGET imported2 PROPERTY IMPORTED_LOCATION_DEBUG debug_loc)
set_property(TARGET imported2 PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
set_property(TARGET imported2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "/imported2/include/with space")

add_library(imported3 SHARED IMPORTED)
set_property(TARGET imported3 PROPERTY IMPORTED_LOCATION_RELEASE release_loc)
set_property(TARGET imported3 PROPERTY IMPORTED_LOCATION_DEBUG debug_loc)
# Both Debug and Release should not be true when this is evaluated.
set_property(TARGET imported3 APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES $<$<CONFIG:DEBUG>:$<TARGET_PROPERTY:imported1,INTERFACE_INCLUDE_DIRECTORIES>>)
set_property(TARGET imported3 APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES $<$<CONFIG:RELEASE>:$<TARGET_PROPERTY:imported2,INTERFACE_INCLUDE_DIRECTORIES>>)
set_property(TARGET imported3 APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES $<$<CONFIG:RELWITHDEBINFO>:$<TARGET_PROPERTY:imported2,INTERFACE_INCLUDE_DIRECTORIES>>)
set_property(TARGET imported3 APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES $<$<CONFIG:MINSIZEREL>:$<TARGET_PROPERTY:imported2,INTERFACE_INCLUDE_DIRECTORIES>>)

add_library(imported4 SHARED IMPORTED)
set_property(TARGET imported4 APPEND PROPERTY
  INCLUDE_DIRECTORIES $<TARGET_PROPERTY:imported3,INTERFACE_INCLUDE_DIRECTORIES>)

add_executable(someexe $<1:empty.cpp> $<0:does_not_exist>)
add_executable(Alias::SomeExe ALIAS someexe)

add_library(Alias::SomeLib ALIAS empty1)

add_library(importedFallback STATIC IMPORTED)
set_property(TARGET importedFallback PROPERTY IMPORTED_LOCATION_DEBUG debug_loc)
set_property(TARGET importedFallback PROPERTY IMPORTED_LOCATION_RELEASE release_loc)
set_property(TARGET importedFallback PROPERTY IMPORTED_LOCATION fallback_loc)
set_property(TARGET importedFallback PROPERTY MAP_IMPORTED_CONFIG_DEBUG "" DEBUG)
set_property(TARGET importedFallback PROPERTY MAP_IMPORTED_CONFIG_RELEASE "")

add_custom_target(check-part3 ALL
  COMMAND ${CMAKE_COMMAND}
    -Dtest_version_greater_1=$<VERSION_GREATER:1.0,1.1.1>
    -Dtest_version_greater_2=$<VERSION_GREATER:1.1.1,1.0>
    -Dtest_version_less_1=$<VERSION_LESS:1.1.1,1.0>
    -Dtest_version_less_2=$<VERSION_LESS:1.0,1.1.1>
    -Dtest_version_equal_1=$<VERSION_EQUAL:1.0.1,1.1>
    -Dtest_version_equal_2=$<VERSION_EQUAL:1.1,1.1>
    -Dconfig=$<CONFIGURATION>
    -Dtest_imported_includes=$<TARGET_PROPERTY:imported4,INCLUDE_DIRECTORIES>
    -Dtest_imported_fallback=$<STREQUAL:$<TARGET_FILE_NAME:importedFallback>,fallback_loc>
    -Dtest_alias_file_exe=$<STREQUAL:$<TARGET_FILE:Alias::SomeExe>,$<TARGET_FILE:someexe>>
    -Dtest_alias_file_lib=$<STREQUAL:$<TARGET_FILE:Alias::SomeLib>,$<TARGET_FILE:empty1>>
    -Dtest_alias_target_name=$<STREQUAL:$<TARGET_PROPERTY:Alias::SomeLib,NAME>,$<TARGET_PROPERTY:empty1,NAME>>
    -Dtest_early_termination_1=$<$<1:>:
    -Dtest_early_termination_2=$<$<1:>:,
    -Dsystem_name=${CMAKE_HOST_SYSTEM_NAME}
    -Dtest_platform_id=$<PLATFORM_ID>
    -Dtest_platform_id_Linux=$<PLATFORM_ID:Linux>
    -Dtest_platform_id_Windows=$<PLATFORM_ID:Windows>
    -Dtest_platform_id_Darwin=$<PLATFORM_ID:Darwin>
    -Dlower_case=$<LOWER_CASE:Mi,XeD>
    -Dupper_case=$<UPPER_CASE:MiX,eD>
    -Dmake_c_identifier=$<MAKE_C_IDENTIFIER:4f,oo:+bar-$>
    -Dequal1=$<EQUAL:1,2>
    -Dequal2=$<EQUAL:1,1>
    -Dequal3=$<EQUAL:0x1,1>
    -Dequal4=$<EQUAL:0X1,2>
    -Dequal5=$<EQUAL:0xA,0xa>
    -Dequal6=$<EQUAL:0xA,10>
    -Dequal7=$<EQUAL:0xA,012>
    -Dequal8=$<EQUAL:10,012>
    -Dequal9=$<EQUAL:10,010>
    -Dequal10=$<EQUAL:10,0b1010>
    -Dequal11=$<EQUAL:-10,-0xa>
    -Dequal12=$<EQUAL:10,+0xa>
    -Dequal13=$<EQUAL:+10,+0Xa>
    -Dequal14=$<EQUAL:+10,0xa>
    -Dequal15=$<EQUAL:-10,-0Xa>
    -Dequal16=$<EQUAL:-10,-0b1010>
    -Dequal17=$<EQUAL:-10,+0b1010>
    -Dequal18=$<EQUAL:10,+0B1010>
    -Dequal19=$<EQUAL:10,-0B1010>
    -Dequal20=$<EQUAL:10,0B1010>
    -Dequal21=$<EQUAL:10,+012>
    -Dequal22=$<EQUAL:10,-012>
    -Dequal23=$<EQUAL:-10,-012>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-part3.cmake
  COMMAND ${CMAKE_COMMAND} -E echo "check done (part 3 of 4)"
  VERBATIM
  )

if(WIN32)
  set(test_shell_path c:/shell/path)
else()
  set(test_shell_path /shell/path)
endif()
set(path_prefix BYPASS_FURTHER_CONVERSION)

add_custom_target(check-part4 ALL
  COMMAND ${CMAKE_COMMAND}
    # Prefix path to bypass its further conversion when being processed by
    # CMake as command-line argument
    -Dtest_shell_path=${path_prefix}$<SHELL_PATH:${test_shell_path}>
    -Dpath_prefix=${path_prefix}
    -Dif_1=$<IF:1,a,b>
    -Dif_2=$<IF:0,a,b>
    -Dif_3=$<IF:$<EQUAL:10,30>,a,b>
    -Dif_4=$<IF:$<EQUAL:30,30>,a,b>
    -DWIN32=${WIN32}
    -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check-part4.cmake
  COMMAND ${CMAKE_COMMAND} -E echo "check done (part 4 of 4)"
  VERBATIM
  )

#-----------------------------------------------------------------------------
# Cover test properties with generator expressions.
add_executable(echo echo.c)
add_executable(pwd pwd.c)
set_property(SOURCE echo.c PROPERTY COMPILE_FLAGS $<1:-DSRC_GENEX_WORKS>)

add_test(NAME echo-configuration COMMAND echo $<CONFIGURATION>)
set_property(TEST echo-configuration PROPERTY
  PASS_REGULAR_EXPRESSION "^$<CONFIGURATION>\n$")

add_test(NAME echo-target-file COMMAND echo $<TARGET_FILE:echo>)
set_property(TEST echo-target-file PROPERTY
  PASS_REGULAR_EXPRESSION "/echo${CMAKE_EXECUTABLE_SUFFIX}\n$")
set_property(TEST echo-target-file PROPERTY
  REQUIRED_FILES "$<TARGET_FILE:echo>")

add_test(NAME working-dir-arg
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/WorkingDirectory/$<CONFIGURATION>"
  COMMAND pwd)
set_property(TEST working-dir-arg PROPERTY
  PASS_REGULAR_EXPRESSION "WorkingDirectory/$<CONFIGURATION>\n$")
foreach(c ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/WorkingDirectory/${c}")
endforeach()

add_test(echo-old-style echo "\$<CONFIGURATION>")
set_property(TEST echo-old-style PROPERTY
    PASS_REGULAR_EXPRESSION "^\\$<CONFIGURATION>\n$")

add_subdirectory(CMP0044 ${CMAKE_BINARY_DIR}/CMP0044-WARN)
set(CMP0044_TYPE NEW)
add_subdirectory(CMP0044 ${CMAKE_BINARY_DIR}/CMP0044-NEW)
set(CMP0044_TYPE OLD)
add_subdirectory(CMP0044 ${CMAKE_BINARY_DIR}/CMP0044-OLD)

if(NOT CMAKE_GENERATOR STREQUAL Xcode OR NOT CMAKE_OSX_ARCHITECTURES MATCHES "[;$]")
  add_library(objlib OBJECT objlib1.c objlib2.c)
  file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/objlib_files_$<CONFIGURATION>"
    CONTENT "$<JOIN:$<TARGET_OBJECTS:objlib>,\n>\n"
  )

  add_custom_target(check_object_files ALL
    COMMAND ${CMAKE_COMMAND}
      "-DOBJLIB_LISTFILE=${CMAKE_CURRENT_BINARY_DIR}/objlib_files_$<CONFIGURATION>"
      -DEXPECTED_NUM_OBJECTFILES=2
      -P "${CMAKE_CURRENT_SOURCE_DIR}/check_object_files.cmake"
    DEPENDS objlib
  )
endif()
