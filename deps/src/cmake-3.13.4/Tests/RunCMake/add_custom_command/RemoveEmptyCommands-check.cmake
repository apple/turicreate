set(vcProjectFile "${RunCMake_TEST_BINARY_DIR}/exe.vcxproj")
if(NOT EXISTS "${vcProjectFile}")
  set(RunCMake_TEST_FAILED "Project file ${vcProjectFile} does not exist.")
  return()
endif()

set(inGroup FALSE)
set(inCommand FALSE)

set(expected_Debug
  "cmd_1 cmd_1_arg"
  "cmd_1_dbg cmd_1_dbg_arg"
  "cmd_2_dbg cmd_2_dbg_arg"
  "cmd_3_dbg cmd_3_dbg_arg")

set(expected_Release
  "cmd_1 cmd_1_arg"
  "cmd_3_rel cmd_3_rel_arg")

# extract build events
file(STRINGS "${vcProjectFile}" lines)
foreach(line IN LISTS lines)
  if(line MATCHES "^ *<ItemDefinitionGroup Condition=.*Configuration.*Platform.*>$")
    set(inGroup TRUE)
    string(REGEX MATCH "=='(.*)\\|(.*)'" out ${line})
    set(config ${CMAKE_MATCH_1})
  elseif(line MATCHES "^ *</ItemDefinitionGroup>$")
    set(inGroup FALSE)
  elseif(inGroup)
    if(line MATCHES "^ *<Command>.*$")
      set(inCommand TRUE)
      string(REGEX MATCH "<Command>(.*)" cmd ${line})
      set(currentCommand ${CMAKE_MATCH_1})
    elseif(line MATCHES "^(.*)</Command>$")
      string(REGEX MATCH "(.*)</Command>" cmd ${line})
      list(APPEND currentCommand ${CMAKE_MATCH_1})
      set(command_${config} ${currentCommand})
      set(inCommand FALSE)
    elseif(inCommand)
      list(APPEND currentCommand ${line})
    endif()
  endif()
endforeach()

foreach(config "Debug" "Release")
  set(currentName command_${config})
  set(expectedName expected_${config})
  set(strippedCommand "")
  if(DEFINED ${currentName})
    foreach(v ${${currentName}})
      if(${v} MATCHES "cmd_")
        list(APPEND strippedCommand ${v})
      endif()
    endforeach()
    if(NOT "${strippedCommand}" STREQUAL
      "${${expectedName}}")
      message(" - ${strippedCommand}")
      message(" + ${${expectedName}}")
      set(RunCMake_TEST_FAILED "build event command does not match")
      return()
    endif()
  endif()
endforeach()
