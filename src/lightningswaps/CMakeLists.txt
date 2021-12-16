file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE PROTOS  *.proto)

add_custom_target(lightningswaps-protos SOURCES ${PROTOS})
add_library(lightningswaps STATIC ${SOURCES} ${HEADERS})

target_include_directories(lightningswaps PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(lightningswaps
    lndtools
    walletcore
    networking
    common
    httplib
    )

target_compile_options(lightningswaps PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
