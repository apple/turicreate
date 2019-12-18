#
# Demonstration CMakeLists.txt for nanomsg demos.
#
# This file shows how one might use nanomsg from a another project
# that is also CMake-driven.
#
# Thanks for the idea goes to @maddouri.
#
cmake_minimum_required (VERSION 2.8.7)

project(nanomsg-demo)

# Call this from your own project's makefile.
find_package(nanomsg CONFIG REQUIRED)

add_executable(async_demo async_demo.c)
target_link_libraries(async_demo nanomsg)

add_executable(device_demo device_demo.c)
target_link_libraries(device_demo nanomsg)

add_executable(pthread_demo pthread_demo.c)
target_link_libraries(pthread_demo nanomsg)

add_executable(pubsub_demo pubsub_demo.c)
target_link_libraries(pubsub_demo nanomsg)

add_executable(rpc_demo rpc_demo.c)
target_link_libraries(rpc_demo nanomsg)
