file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.h *.hpp)

find_program(PROTOC NAMES protoc PATHS "${CMAKE_BINARY_DIR}/bin")
find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin PATHS "${CMAKE_BINARY_DIR}/bin")

file(GLOB PROTOBUF_DEFINITION_FILES "testdata/*.proto")
set(PROTOBUF_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gen-grpc")
make_directory(${PROTOBUF_OUTPUT_DIRECTORY})
foreach(file ${PROTOBUF_DEFINITION_FILES})
    execute_process(COMMAND ${PROTOC} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/testdata --cpp_out=${PROTOBUF_OUTPUT_DIRECTORY} "${file}"
            RESULT_VARIABLE PROTOBUF_RESULT
            OUTPUT_VARIABLE PROTOBUF_OUTPUT_VARIABLE
            ERROR_VARIABLE  PROTOBUF_OUTPUT_VARIABLE)

    execute_process(COMMAND ${PROTOC} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/testdata --grpc_out=${PROTOBUF_OUTPUT_DIRECTORY} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} "${file}"
            RESULT_VARIABLE PROTOBUF_RESULT
            OUTPUT_VARIABLE PROTOBUF_OUTPUT_VARIABLE
            ERROR_VARIABLE  PROTOBUF_OUTPUT_VARIABLE)
endforeach()

file(GLOB PROTOC_GENERATED_SOURCES ${PROTOBUF_OUTPUT_DIRECTORY}/*.cc)
file(GLOB PROTOC_GENERATED_HEADERS ${PROTOBUF_OUTPUT_DIRECTORY}/*.h)

add_executable(core-tests ${SOURCES} ${HEADERS} ${PROTOC_GENERATED_SOURCES} ${PROTOC_GENERATED_HEADERS})

mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

target_link_libraries(core-tests
    core
    tradingbot
    gtest
    gmock
    Qt5::Test
    Qt5::Quick)

configure_file("${CMAKE_SOURCE_DIR}/app/assets/assets_conf.json" "assets_conf.json" COPYONLY)

