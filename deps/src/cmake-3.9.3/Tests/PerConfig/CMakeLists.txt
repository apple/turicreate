project(PerConfig C)

# Targets with per-configuration names.
add_library(pcStatic STATIC pcStatic.c)
set_property(TARGET pcStatic PROPERTY RELEASE_POSTFIX -opt)
set_property(TARGET pcStatic PROPERTY DEBUG_POSTFIX -dbg)
add_library(pcShared SHARED pcShared.c)
set_property(TARGET pcShared PROPERTY RELEASE_POSTFIX -opt)
set_property(TARGET pcShared PROPERTY DEBUG_POSTFIX -dbg)
set_property(TARGET pcShared PROPERTY VERSION 1.2)
set_property(TARGET pcShared PROPERTY SOVERSION 3)
if(UNIX AND NOT CYGWIN)
  set(soname_file -DpcShared_soname_file=$<TARGET_SONAME_FILE:pcShared>)
endif()
add_executable(perconfig perconfig.c)
target_link_libraries(perconfig pcStatic pcShared)
set_property(TARGET perconfig PROPERTY RELEASE_POSTFIX -opt)
set_property(TARGET perconfig PROPERTY DEBUG_POSTFIX -dbg)

set(PerConfig_COMMAND
  ${CMAKE_COMMAND}
    -Dconfiguration=$<CONFIGURATION>
    -Dperconfig_file_dir=$<TARGET_FILE_DIR:perconfig>
    -Dperconfig_file_name=$<TARGET_FILE_NAME:perconfig>
    -Dperconfig_file=$<TARGET_FILE:perconfig>
    -DpcStatic_file=$<TARGET_FILE:pcStatic>
    -DpcStatic_linker_file=$<TARGET_LINKER_FILE:pcStatic>
    -DpcShared_file=$<TARGET_FILE:pcShared>
    -DpcShared_linker_file=$<TARGET_LINKER_FILE:pcShared>
    ${soname_file}
    -P ${PerConfig_SOURCE_DIR}/perconfig.cmake
    )
set(PerConfig_COMMAND "${PerConfig_COMMAND}" PARENT_SCOPE)
set(PerConfig_DEPENDS ${PerConfig_SOURCE_DIR}/perconfig.cmake perconfig pcStatic pcShared)
