project(cppipc)

if(WIN32)
  SET(PLATFORM_SOURCES client/console_cancel_handler_win.cpp)
else()
  SET(PLATFORM_SOURCES client/console_cancel_handler_unix.cpp)
endif()

# Libraries should not link both cppipc and cppipc_zookeeper together
# They are not compatible
add_library(cppipc OBJECT
  client/comm_client.cpp
  ${PLATFORM_SOURCES}
  common/message_types.cpp
  common/object_factory.cpp
  common/object_factory_impl.cpp
  common/ipc_deserializer.cpp
  server/comm_server.cpp
  ipc_object_base.cpp
)
add_dependencies(cppipc boost)

add_library(cancel_serverside_ops OBJECT
  server/cancel_ops.cpp
)
add_dependencies(cancel_serverside_ops boost)
