file(GLOB SOURCES GRPCTools/*.cpp)
file(GLOB HEADERS GRPCTools/*.hpp)

add_library(qgrpc STATIC ${SOURCES} ${HEADERS})

target_include_directories(qgrpc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(qgrpc
    common
    Boost::boost
    gRPC::grpc
    gRPC::grpc_unsecure
    gRPC::grpc++
    gRPC::grpc++_unsecure)

