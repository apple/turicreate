cmake_minimum_required(VERSION 2.8)
project(Architecture C)

function(test_for_xcode4 result_var)
  set(${result_var} 0 PARENT_SCOPE)
  if(APPLE)
    execute_process(COMMAND xcodebuild -version
      OUTPUT_VARIABLE ov RESULT_VARIABLE rv
      )
    if("${rv}" STREQUAL "0" AND ov MATCHES "^Xcode ([0-9]+)\\.")
      if(NOT CMAKE_MATCH_1 VERSION_LESS 4)
        set(${result_var} 1 PARENT_SCOPE)
      endif()
    endif()
  endif()
endfunction()

test_for_xcode4(is_xcode4)

set(arch0 i386)
set(arch1 ppc)

if(is_xcode4)
  # Xcode 4, use modern architectures as defaults
  # Arch 'ppc' no longer works: tools no longer available starting with Xcode 4
  set(arch0 i386)
  set(arch1 x86_64)
endif()

add_library(foo foo.c)
if(CMAKE_OSX_ARCHITECTURES)
  get_property(archs TARGET foo PROPERTY OSX_ARCHITECTURES)
  if(NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "${archs}")
    message(FATAL_ERROR
      "OSX_ARCHITECTURES property not initialized by CMAKE_OSX_ARCHITECTURES.\n"
      "Expected [${CMAKE_OSX_ARCHITECTURES}], got [${archs}]."
      )
  endif()
  list(LENGTH archs archs_len)
  if(archs_len GREATER 1)
    list(GET archs 0 arch0)
    list(GET archs 1 arch1)
  endif()
endif()

message("is_xcode4='${is_xcode4}'")
message("archs='${archs}'")
message("arch0='${arch0}'")
message("arch1='${arch1}'")

set_property(TARGET foo PROPERTY OSX_ARCHITECTURES ${arch0})
set_property(TARGET foo PROPERTY OSX_ARCHITECTURES_DEBUG ${arch1})

add_executable(bar bar.c)
target_link_libraries(bar foo)

set_property(TARGET bar PROPERTY OUTPUT_NAME Architecture)
set_property(TARGET bar PROPERTY OSX_ARCHITECTURES ${arch1})
set_property(TARGET bar PROPERTY OSX_ARCHITECTURES_DEBUG ${arch0})
