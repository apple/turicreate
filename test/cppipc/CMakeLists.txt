project(cppipc_test)

# cppipc tests only a couple of these are testable
make_executable(ping_client_test SOURCES ping_client_test.cpp REQUIRES unity_shared)
make_executable(ping_server_test SOURCES ping_server_test.cpp REQUIRES unity_shared)
make_executable(client_test SOURCES client_test.cpp REQUIRES unity_shared)
make_executable(server_test SOURCES server_test.cpp REQUIRES unity_shared)
make_boost_test(dispatch_test.cxx REQUIRES unity_shared)
make_boost_test(issue_test.cxx REQUIRES unity_shared)
make_boost_test(garbage_collect_test.cxx REQUIRES unity_shared)
make_boost_test(inproc_connect_test.cxx REQUIRES unity_shared)
make_boost_test(long_file_name_test.cxx REQUIRES unity_shared)
