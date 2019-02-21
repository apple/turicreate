# Find a home in which to build.
if(NOT DEFINED HOME)
  if(DEFINED ENV{CTEST_REAL_HOME})
    set(HOME "$ENV{CTEST_REAL_HOME}")
  else()
    set(HOME "$ENV{HOME}")
  endif()

  if(NOT HOME AND WIN32)
    # Try for USERPROFILE as HOME equivalent:
    string(REPLACE "\\" "/" HOME "$ENV{USERPROFILE}")

    # But just use root of SystemDrive if USERPROFILE contains any spaces:
    # (Default on XP and earlier...)
    if(HOME MATCHES " ")
      string(REPLACE "\\" "/" HOME "$ENV{SystemDrive}")
    endif()
  endif()
endif()
