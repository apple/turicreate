project(nanosockets)

add_library(nanosockets OBJECT
  async_reply_socket.cpp
  async_request_socket.cpp
  print_zmq_error.cpp
  zmq_msg_vector.cpp
  socket_config.cpp
  subscribe_socket.cpp
  publish_socket.cpp
  get_next_port_number.cpp
)
add_dependencies(nanosockets boost)
