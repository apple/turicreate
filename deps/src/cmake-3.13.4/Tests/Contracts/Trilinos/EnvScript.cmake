# Site specific settings:
#
if(CTEST_SITE MATCHES "faraway")
  set(CTEST_SITE "faraway.kitware")
  set(ENV{CTEST_SITE} "${CTEST_SITE}")
endif()

if(CTEST_SITE STREQUAL "HUT11")
  set(CTEST_SITE "hut11.kitware")
  set(ENV{CTEST_SITE} "${CTEST_SITE}")

  set(ENV{CLAPACK_DIR} "C:/T/clapack/b/clapack-prefix/src/clapack-build")
endif()

if(CTEST_SITE MATCHES "qwghlm")
  set(CTEST_SITE "qwghlm.kitware")
  set(ENV{CTEST_SITE} "${CTEST_SITE}")

  set(ENV{PATH} "/opt/local/bin:$ENV{PATH}")
  set(ENV{CC} "gcc-mp-4.3")
  set(ENV{CXX} "g++-mp-4.3")
  set(ENV{FC} "gfortran-mp-4.3")
endif()

# Submit to alternate CDash server:
#
#set(ENV{CTEST_DROP_SITE} "localhost")
#set(ENV{CTEST_DROP_LOCATION} "/CDash/submit.php?project=Trilinos")

# Limit packages built:
#
set(ENV{Trilinos_PACKAGES} "Teuchos;Kokkos")
