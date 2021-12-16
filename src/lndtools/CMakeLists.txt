file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE PROTOS  *.proto)

add_custom_target(lndtools-protos SOURCES ${PROTOS})
add_library(lndtools STATIC ${SOURCES} ${HEADERS})

target_include_directories(lndtools PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/Protos)
target_link_libraries(lndtools qgrpc walletcore networking Qt5::Network Qt5::Qml)

target_compile_options(lndtools PRIVATE
     $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
