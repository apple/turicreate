project(Turi)

make_library(exceptions OBJECT
  SOURCES
    TuriErrorCode.cpp
    TuriException.cpp
  REQUIRES
  EXTERNAL_VISIBILITY
)
