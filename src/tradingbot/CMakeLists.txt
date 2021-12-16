file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE PROTOS  *.proto)

add_library(tradingbot STATIC ${SOURCES} ${HEADERS})

target_include_directories(tradingbot PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(tradingbot
    lightningswaps
    networking)

target_compile_options(tradingbot PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wall>
     $<$<CXX_COMPILER_ID:MSVC>:/W4 /bigobj>)
