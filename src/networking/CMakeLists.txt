file(GLOB SOURCES Networking/*.cpp)
file(GLOB HEADERS Networking/*.hpp)

add_library(networking STATIC ${SOURCES} ${HEADERS})

target_include_directories(networking PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(networking
    walletcore
    eth-core
    common
    jcon
    Qt5::Network
    Qt5::Concurrent
    Qt5::WebSockets)
