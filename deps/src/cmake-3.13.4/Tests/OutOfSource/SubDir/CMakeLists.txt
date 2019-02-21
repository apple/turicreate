project(ANOTHER_PROJ)

# subdir to an out of source and out of binary directory
add_subdirectory(${OutOfSource_SOURCE_DIR}/../OutOfBinary
                 ${OutOfSource_BINARY_DIR}/../OutOfBinary)

# subdir to a sibling dir
add_subdirectory(${OutOfSource_SOURCE_DIR}/${KEN}OutOfSourceSubdir OutOfSourceSubdir )

add_library(subdir subdir.c)
