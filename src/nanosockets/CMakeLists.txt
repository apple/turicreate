project(nanosockets)


# Libraries should not link both fault_comm_zookeeper and fault_comm together
# They are not compatible
make_library(nanosockets
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
