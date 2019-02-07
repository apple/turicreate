project(cppipc_test)

# cppipc tests only a couple of these are testable
if(NOT WIN32)
    make_executable(shm_ping_client_test SOURCES shm_ping_client_test.cpp REQUIRES unity_shared)
    make_executable(shm_ping_server_test SOURCES shm_ping_server_test.cpp REQUIRES unity_shared)
    make_boost_test(shmipc_test.cxx REQUIRES unity_shared)
endif()
