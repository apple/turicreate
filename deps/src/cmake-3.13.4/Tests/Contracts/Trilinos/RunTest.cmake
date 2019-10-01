# ValidateBuild.cmake is configured into this location when the test is built:
set(dir "${CMAKE_CURRENT_BINARY_DIR}/Contracts/${project}")

set(exe "${CMAKE_COMMAND}")
set(args -P "${dir}/ValidateBuild.cmake")

set(Trilinos_RUN_TEST ${exe} ${args})
