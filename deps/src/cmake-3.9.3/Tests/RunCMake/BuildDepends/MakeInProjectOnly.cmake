enable_language(C)
get_filename_component(include_dir "${CMAKE_BINARY_DIR}" PATH)
include_directories("${include_dir}")
add_executable(MakeInProjectOnly MakeInProjectOnly.c)
set(CMAKE_DEPENDS_IN_PROJECT_ONLY 1)
file(GENERATE OUTPUT check-$<LOWER_CASE:$<CONFIG>>.cmake CONTENT "
if (check_step EQUAL 1)
  set(check_pairs
    \"$<TARGET_FILE:MakeInProjectOnly>|${include_dir}/MakeInProjectOnly.h\"
    )
else()
  set(check_pairs
    \"${include_dir}/MakeInProjectOnly.h|\$<TARGET_FILE:MakeInProjectOnly>\"
    )
endif()
")
