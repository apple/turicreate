enable_language(CXX)

set(CMAKE_VS_GLOBALS
    "DefaultLanguage=en-US"
    "MinimumVisualStudioVersion=14.0"
)

add_library(foo foo.cpp)
