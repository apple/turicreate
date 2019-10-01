cmake_minimum_required (VERSION 3.0)
project (RegexClear NONE)

function (output_results msg)
  message("results from: ${msg}")
  message("CMAKE_MATCH_0: -->${CMAKE_MATCH_0}<--")
  message("CMAKE_MATCH_1: -->${CMAKE_MATCH_1}<--")
  message("CMAKE_MATCH_2: -->${CMAKE_MATCH_2}<--")
  message("CMAKE_MATCH_COUNT: -->${CMAKE_MATCH_COUNT}<--")
endfunction ()

set(haystack "Some::Scope")

string(REGEX MATCHALL "^([^:]+)(::)?" matches "${haystack}")
message("matches: ${matches}")
output_results("string(REGEX MATCHALL)")

string(REGEX REPLACE "^([^:]+)(::)?" "[\\1]" replace "${haystack}")
message("replace: ${replace}")
output_results("string(REGEX REPLACE)")
