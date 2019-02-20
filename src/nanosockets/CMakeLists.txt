project(nanosockets)

make_library(nanosockets OBJECT
  SOURCES
    async_reply_socket.cpp
    async_request_socket.cpp
    print_zmq_error.cpp
    zmq_msg_vector.cpp
    socket_config.cpp
    subscribe_socket.cpp
    publish_socket.cpp
    get_next_port_number.cpp
  REQUIRES
    network
    util
    timer
    parallel
    logger
    globals
    nanomsg
)
