
enable_language(CXX)

add_executable(objlibuser
    empty.cpp
    $<TARGET_OBJECTS:bar>
)

get_target_property(_location objlibuser LOCATION)

add_library(bar OBJECT
    empty.cpp
)
