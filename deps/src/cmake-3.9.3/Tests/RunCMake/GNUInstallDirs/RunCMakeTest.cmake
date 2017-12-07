include(RunCMake)

if(SYSTEM_NAME MATCHES "^(.*BSD|DragonFly)$")
  set(EXPECT_BSD 1)
endif()

foreach(case
    Opt
    Root
    Usr
    UsrLocal
    )
  if(EXPECT_BSD)
    set(RunCMake-stderr-file ${case}-BSD-stderr.txt)
  endif()
  run_cmake(${case})
endforeach()
