file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h*)

add_library(eth-core STATIC ${SOURCES} ${HEADERS})

target_include_directories(eth-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/Protos)

target_link_libraries(
    eth-core
    walletcore
    common
    CONAN_PKG::rust-eth-lib
    Qt5::Core)

target_compile_options(eth-core PRIVATE
     $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
