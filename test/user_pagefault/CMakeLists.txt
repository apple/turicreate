project(user_pf_test)

# cppipc tests only a couple of these are testable
if (NOT WIN32)
    make_executable(user_pf_test SOURCES user_pf_test.cpp REQUIRES unity_shared)
    make_boost_test(typed_heuristic_encode_test.cxx REQUIRES unity_shared)
endif()
