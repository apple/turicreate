project(unity)

if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
  make_copy_target(
    tcmps_copy
    TARGETS
    tcmps
  )
  add_dependencies(tcmps_copy tcmps)
endif()
