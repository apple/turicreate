set(csProjectFile "${RunCMake_TEST_BINARY_DIR}/foo.csproj")
if(NOT EXISTS "${csProjectFile}")
  set(RunCMake_TEST_FAILED "Project file ${csProjectFile} does not exist.")
  return()
endif()

set(tagFound FALSE)

set(tagName "MyCustomTag")
set(tagValue "MyCustomValue")

file(STRINGS "${csProjectFile}" lines)
foreach(line IN LISTS lines)
  if(line MATCHES "^ *<${tagName}>${tagValue}</${tagName}>")
    message(STATUS "foo.csproj has tag ${tagName} with value ${tagValue} defined")
    set(tagFound TRUE)
  endif()
endforeach()

if(NOT tagFound)
  set(RunCMake_TEST_FAILED "Source file tag ${tagName} with value ${tagValue} not found.")
  return()
endif()
