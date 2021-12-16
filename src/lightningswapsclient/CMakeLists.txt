file(GLOB SOURCES ../lightningswapsdaemon/Protos/*.cc *.cpp)
file(GLOB HEADERS ../lightningswapsdaemon/Protos/*.h)

add_executable(lssd-cli ${SOURCES} ${HEADERS})
target_include_directories(lssd-cli PRIVATE ../lightningswapsdaemon)
target_link_libraries(lssd-cli lightningswaps)

#set_target_properties(lssd-cli
#    PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lightningswapsdaemon/appdir/usr/bin"
#)
