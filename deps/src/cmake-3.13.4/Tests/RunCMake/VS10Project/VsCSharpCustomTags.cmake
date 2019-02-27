enable_language(CSharp)

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

set(fileNames)
foreach(e ${fileExtensions})
  set(currentFile "${CMAKE_CURRENT_BINARY_DIR}/foo.${e}")
  list(APPEND fileNames ${currentFile})
  file(TOUCH "${currentFile}")
  string(TOUPPER ${e} eUC)
  set_source_files_properties("${currentFile}"
    PROPERTIES
    VS_CSHARP_${tagName}${eUC} "${tagValue}${eUC}")
endforeach()

add_library(foo ${fileNames})
