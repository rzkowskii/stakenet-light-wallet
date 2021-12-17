file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE PROTOS  *.proto)

add_custom_target(core-protos SOURCES ${PROTOS})
add_library(core STATIC ${SOURCES} ${HEADERS})

target_include_directories(core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(core
    lightningswaps
    networking
    tradingbot
    CONAN_PKG::ZeroMQ)

target_compile_options(core PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wall>
     $<$<CXX_COMPILER_ID:MSVC>:/W4 /bigobj>)
