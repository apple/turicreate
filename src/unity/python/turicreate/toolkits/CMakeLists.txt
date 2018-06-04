project(unity)

if(APPLE)
  make_copy_target(
    tcmps_copy
    TARGETS
    tcmps
  )
  add_dependencies(tcmps_copy tcmps)
endif()
