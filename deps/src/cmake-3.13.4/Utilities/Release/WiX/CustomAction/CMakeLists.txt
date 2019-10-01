foreach(CONFIG DEBUG MINSIZEREL RELEASE RELWITHDEBINFO)
  string(REPLACE "/MD" "/MT"
    "CMAKE_CXX_FLAGS_${CONFIG}"
    "${CMAKE_CXX_FLAGS_${CONFIG}}"
  )
endforeach()

add_library(CMakeWiXCustomActions MODULE
  detect_nsis_overwrite.cpp
  exports.def
)

target_link_libraries(CMakeWiXCustomActions PRIVATE msi)
