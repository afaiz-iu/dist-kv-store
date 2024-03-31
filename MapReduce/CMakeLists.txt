cmake_minimum_required(VERSION 3.8)

project(MapReduce)

include(cmake/common.cmake)

# Proto file
get_filename_component(mapreduce_proto "src/proto/mapreduce.proto" ABSOLUTE)
get_filename_component(mapreduce_proto_path "${mapreduce_proto}" PATH)

# Generated sources
set(mapreduce_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/mapreduce.pb.cc")
set(mapreduce_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mapreduce.pb.h")
set(mapreduce_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/mapreduce.grpc.pb.cc")
set(mapreduce_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/mapreduce.grpc.pb.h")
add_custom_command(
      OUTPUT "${mapreduce_proto_srcs}" "${mapreduce_proto_hdrs}" "${mapreduce_grpc_srcs}" "${mapreduce_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${mapreduce_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${mapreduce_proto}"
      DEPENDS "${mapreduce_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# mapreduce_grpc_proto
add_library(mapreduce_grpc_proto
  ${mapreduce_grpc_srcs}
  ${mapreduce_grpc_hdrs}
  ${mapreduce_proto_srcs}
  ${mapreduce_proto_hdrs})
target_link_libraries(mapreduce_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# Build the server
add_executable(server
  src/server.cc
  src/utils.cc)
target_link_libraries(server
  mapreduce_grpc_proto)

# Build the client
add_executable(client
  src/client.cc)
target_link_libraries(client
  mapreduce_grpc_proto)