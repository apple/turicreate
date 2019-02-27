set(csProjectFile "${RunCMake_TEST_BINARY_DIR}/foo.csproj")
if(NOT EXISTS "${csProjectFile}")
  set(RunCMake_TEST_FAILED "Project file ${csProjectFile} does not exist.")
  return()
endif()

# test VS_CSHARP_* for the following extensions
set(fileExtensions
  "cs"
  "png"
  "jpg"
  "xml"
  "settings")

#
set(tagName "MyCustomTag")
set(tagValue "MyCustomValue")

file(STRINGS "${csProjectFile}" lines)

foreach(e ${fileExtensions})
  string(TOUPPER ${e} eUC)
  set(tagFound FALSE)
  foreach(line IN LISTS lines)
    if(line MATCHES "^ *<${tagName}${eUC}>${tagValue}${eUC}</${tagName}${eUC}>")
      message(STATUS "foo.csproj has tag ${tagName}${eUC} with value ${tagValue}${eUC} defined")
      set(tagFound TRUE)
    endif()
  endforeach()
  if(NOT tagFound)
    set(RunCMake_TEST_FAILED "Source file tag ${tagName}${eUC} with value ${tagValue}${eUC} not found.")
    return()
  endif()
endforeach()
