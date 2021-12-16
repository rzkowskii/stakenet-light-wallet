file(GLOB SOURCES *.cpp)
file(GLOB HEADERS *.hpp)

add_library(libupdater STATIC ${SOURCES} ${HEADERS})

# We need this directory, and users of our library will need it too
target_include_directories(libupdater PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(libupdater networking)
