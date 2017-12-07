
find_path(_inc_prefix bar.h PATH_SUFFIXES bar)
set(Bar_INCLUDE_DIRS ${_inc_prefix})

find_library(Bar_LIBRARY bar)
set(Bar_LIBRARIES ${Bar_LIBRARY})
